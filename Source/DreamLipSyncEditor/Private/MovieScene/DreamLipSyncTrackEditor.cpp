// Copyright 2026 DreamDev, Inc. All Rights Reserved.

#include "MovieScene/DreamLipSyncTrackEditor.h"

#include "DreamLipSyncBlueprintLibrary.h"
#include "DreamLipSyncClip.h"
#include "DreamLipSyncMfaGenerator.h"
#include "DreamLipSyncRhubarbGenerator.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "ISequencer.h"
#include "ISequencerSection.h"
#include "Misc/MessageDialog.h"
#include "MovieScene.h"
#include "MovieScene/DreamLipSyncSection.h"
#include "MovieScene/DreamLipSyncTrack.h"
#include "MVVM/Views/ViewUtilities.h"
#include "ScopedTransaction.h"
#include "SequencerSectionPainter.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "DreamLipSyncTrackEditor"

class FDreamLipSyncSectionInterface : public FSequencerSection
{
public:
	explicit FDreamLipSyncSectionInterface(UMovieSceneSection& InSectionObject)
		: FSequencerSection(InSectionObject)
	{
	}

	virtual FText GetSectionTitle() const override
	{
		const UDreamLipSyncSection* Section = Cast<UDreamLipSyncSection>(WeakSection.Get());
		if (!Section)
		{
			return LOCTEXT("InvalidLipSyncSection", "Lip Sync");
		}

		if (const UDreamLipSyncClip* Clip = Section->GetClip())
		{
			return FText::FromString(Clip->GetName());
		}

		return LOCTEXT("EmptyLipSyncSection", "Lip Sync");
	}

	virtual FText GetSectionToolTip() const override
	{
		return GetSectionTitle();
	}

	virtual int32 OnPaintSection(FSequencerSectionPainter& Painter) const override
	{
		return Painter.PaintSectionBackground();
	}

	virtual void BuildSectionContextMenu(FMenuBuilder& MenuBuilder, const FGuid& ObjectBinding) override
	{
		FSequencerSection::BuildSectionContextMenu(MenuBuilder, ObjectBinding);

		TWeakObjectPtr<UMovieSceneSection> WeakMovieSceneSection = WeakSection;
		MenuBuilder.BeginSection(TEXT("DreamLipSyncSectionTools"), LOCTEXT("DreamLipSyncSectionTools", "Dream Lip Sync"));
		MenuBuilder.AddMenuEntry(
			LOCTEXT("AutoSizeLipSyncSectionToClip", "Set Length To Clip Duration"),
			LOCTEXT("AutoSizeLipSyncSectionToClipTooltip", "Resize this lip sync section to the assigned clip's resolved duration."),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Sequencer.AutoFit"),
			FUIAction(FExecuteAction::CreateLambda([WeakMovieSceneSection]()
			{
				if (UDreamLipSyncSection* Section = Cast<UDreamLipSyncSection>(WeakMovieSceneSection.Get()))
				{
					const FScopedTransaction Transaction(LOCTEXT("AutoSizeDreamLipSyncSectionTransaction", "Set Dream Lip Sync Section Length To Clip Duration"));
					Section->Modify();
					Section->AutoSizeToClipDuration();
				}
			}))
		);
		MenuBuilder.EndSection();
	}
};

FDreamLipSyncTrackEditor::FDreamLipSyncTrackEditor(TSharedRef<ISequencer> InSequencer)
	: FMovieSceneTrackEditor(InSequencer)
{
}

TSharedRef<ISequencerTrackEditor> FDreamLipSyncTrackEditor::CreateTrackEditor(TSharedRef<ISequencer> InSequencer)
{
	return MakeShareable(new FDreamLipSyncTrackEditor(InSequencer));
}

void FDreamLipSyncTrackEditor::BuildObjectBindingTrackMenu(FMenuBuilder& MenuBuilder, const TArray<FGuid>& ObjectBindings, const UClass* ObjectClass)
{
	if (ObjectBindings.IsEmpty() || !ObjectClass || !BindingCanResolveSkeletalMesh(ObjectBindings))
	{
		return;
	}

	MenuBuilder.AddMenuEntry(
		LOCTEXT("AddDreamLipSyncTrack", "Dream Lip Sync Track"),
		LOCTEXT("AddDreamLipSyncTrackTooltip", "Adds a morph-target lip sync track to this binding."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Sequencer.Tracks.Audio"),
		FUIAction(FExecuteAction::CreateRaw(this, &FDreamLipSyncTrackEditor::HandleAddLipSyncTrackMenuEntryExecute, ObjectBindings))
	);
}

TSharedPtr<SWidget> FDreamLipSyncTrackEditor::BuildOutlinerEditWidget(const FGuid& ObjectBinding, UMovieSceneTrack* Track, const FBuildEditWidgetParams& Params)
{
	UDreamLipSyncTrack* LipSyncTrack = Cast<UDreamLipSyncTrack>(Track);
	if (!LipSyncTrack)
	{
		return nullptr;
	}

	const int32 RowIndex = Params.TrackInsertRowIndex;
	TWeakObjectPtr<UDreamLipSyncTrack> WeakTrack = LipSyncTrack;

	return UE::Sequencer::MakeAddButton(
		LOCTEXT("AddLipSyncSectionButton", "LipSync"),
		FOnClicked::CreateLambda([this, WeakTrack, RowIndex]()
		{
			if (UDreamLipSyncTrack* PinnedTrack = WeakTrack.Get())
			{
				CreateLipSyncSection(PinnedTrack, RowIndex);
			}
			return FReply::Handled();
		}),
		Params.ViewModel
	);
}

void FDreamLipSyncTrackEditor::BuildTrackContextMenu(FMenuBuilder& MenuBuilder, UMovieSceneTrack* Track)
{
	FMovieSceneTrackEditor::BuildTrackContextMenu(MenuBuilder, Track);

	UDreamLipSyncTrack* LipSyncTrack = Cast<UDreamLipSyncTrack>(Track);
	if (!LipSyncTrack)
	{
		return;
	}

	MenuBuilder.BeginSection(TEXT("DreamLipSyncTools"), LOCTEXT("DreamLipSyncToolsSection", "Dream Lip Sync"));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("GenerateTrackClipsFromRhubarb", "Generate Clips From Rhubarb"),
		LOCTEXT("GenerateTrackClipsFromRhubarbTooltip", "Run Rhubarb for every clip referenced by this track's sections."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Refresh"),
		FUIAction(FExecuteAction::CreateRaw(this, &FDreamLipSyncTrackEditor::GenerateTrackClipsFromRhubarb, LipSyncTrack))
	);
	MenuBuilder.AddMenuEntry(
		LOCTEXT("GenerateTrackClipsFromMfa", "Generate Clips From MFA"),
		LOCTEXT("GenerateTrackClipsFromMfaTooltip", "Run Montreal Forced Aligner for every clip referenced by this track's sections."),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Refresh"),
		FUIAction(FExecuteAction::CreateRaw(this, &FDreamLipSyncTrackEditor::GenerateTrackClipsFromMfa, LipSyncTrack))
	);
	MenuBuilder.EndSection();
}

TSharedRef<ISequencerSection> FDreamLipSyncTrackEditor::MakeSectionInterface(UMovieSceneSection& SectionObject, UMovieSceneTrack& Track, FGuid ObjectBinding)
{
	return MakeShareable(new FDreamLipSyncSectionInterface(SectionObject));
}

bool FDreamLipSyncTrackEditor::SupportsType(TSubclassOf<UMovieSceneTrack> Type) const
{
	return Type == UDreamLipSyncTrack::StaticClass();
}

bool FDreamLipSyncTrackEditor::SupportsSequence(UMovieSceneSequence* InSequence) const
{
	return InSequence != nullptr;
}

const FSlateBrush* FDreamLipSyncTrackEditor::GetIconBrush() const
{
	return FAppStyle::GetBrush("Sequencer.Tracks.Audio");
}

void FDreamLipSyncTrackEditor::HandleAddLipSyncTrackMenuEntryExecute(TArray<FGuid> ObjectBindings)
{
	UMovieScene* MovieScene = GetFocusedMovieScene();
	if (!MovieScene || MovieScene->IsReadOnly())
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("AddDreamLipSyncTrackTransaction", "Add Dream Lip Sync Track"));
	MovieScene->Modify();

	TArray<UMovieSceneTrack*> NewTracks;
	for (const FGuid& BindingId : ObjectBindings)
	{
		UDreamLipSyncTrack* Track = nullptr;
		const TArray<UMovieSceneTrack*> ExistingTracks = MovieScene->FindTracks(UDreamLipSyncTrack::StaticClass(), BindingId);
		if (!ExistingTracks.IsEmpty())
		{
			Track = Cast<UDreamLipSyncTrack>(ExistingTracks[0]);
		}

		if (!Track)
		{
			Track = Cast<UDreamLipSyncTrack>(MovieScene->AddTrack(UDreamLipSyncTrack::StaticClass(), BindingId));
			if (Track)
			{
#if WITH_EDITORONLY_DATA
				Track->SetDisplayName(LOCTEXT("DreamLipSyncTrackName", "Dream Lip Sync"));
#endif
				NewTracks.Add(Track);
			}
		}

		CreateLipSyncSection(Track, INDEX_NONE);
	}

	if (GetSequencer().IsValid())
	{
		if (!NewTracks.IsEmpty())
		{
			GetSequencer()->OnAddTrack(NewTracks[0], FGuid());
		}
		GetSequencer()->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);
	}
}

void FDreamLipSyncTrackEditor::CreateLipSyncSection(UDreamLipSyncTrack* Track, int32 RowIndex)
{
	if (!Track)
	{
		return;
	}

	TSharedPtr<ISequencer> Sequencer = GetSequencer();
	UMovieSceneSequence* Sequence = Sequencer.IsValid() ? Sequencer->GetFocusedMovieSceneSequence() : nullptr;
	UMovieScene* MovieScene = Sequence ? Sequence->GetMovieScene() : nullptr;

	if (!MovieScene || MovieScene->IsReadOnly())
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("AddDreamLipSyncSectionTransaction", "Add Dream Lip Sync Section"));
	Track->Modify();

	UDreamLipSyncSection* NewSection = Cast<UDreamLipSyncSection>(Track->CreateNewSection());
	if (!NewSection)
	{
		return;
	}

	const FFrameNumber StartFrame = Sequencer.IsValid() ? Sequencer->GetLocalTime().Time.FrameNumber : FFrameNumber(0);
	const int32 DurationFrames = FMath::Max(1, MovieScene->GetTickResolution().AsFrameNumber(2.0).Value);

	const TArray<UMovieSceneSection*>& ExistingSections = Track->GetAllSections();
	if (RowIndex != INDEX_NONE)
	{
		NewSection->InitialPlacementOnRow(ExistingSections, StartFrame, DurationFrames, RowIndex);
	}
	else
	{
		NewSection->InitialPlacement(ExistingSections, StartFrame, DurationFrames, Track->SupportsMultipleRows());
	}

	Track->AddSection(*NewSection);

	if (Sequencer.IsValid())
	{
		Sequencer->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);
	}
}

void FDreamLipSyncTrackEditor::GenerateTrackClipsFromRhubarb(UDreamLipSyncTrack* Track)
{
	if (!Track)
	{
		return;
	}

	int32 SuccessCount = 0;
	int32 ClipCount = 0;
	TArray<FString> Messages;
	TSet<UDreamLipSyncClip*> ProcessedClips;

	for (UMovieSceneSection* Section : Track->GetAllSections())
	{
		const UDreamLipSyncSection* LipSyncSection = Cast<UDreamLipSyncSection>(Section);
		UDreamLipSyncClip* Clip = LipSyncSection ? LipSyncSection->GetClip() : nullptr;
		if (!Clip || ProcessedClips.Contains(Clip))
		{
			continue;
		}

		ProcessedClips.Add(Clip);
		++ClipCount;

		FString Message;
		if (FDreamLipSyncRhubarbGenerator::GenerateClipFromRhubarb(Clip, Message))
		{
			++SuccessCount;
		}

		Messages.Add(FString::Printf(TEXT("%s: %s"), *Clip->GetName(), *Message));
	}

	if (ClipCount == 0)
	{
		Messages.Add(TEXT("No DreamLipSyncClip assets are assigned to this track."));
	}

	FMessageDialog::Open(
		EAppMsgType::Ok,
		FText::FromString(FString::Join(Messages, TEXT("\n"))),
		SuccessCount == ClipCount && ClipCount > 0
			? LOCTEXT("GenerateTrackClipsSucceeded", "Dream Lip Sync")
			: LOCTEXT("GenerateTrackClipsIssues", "Dream Lip Sync - Issues"));
}

void FDreamLipSyncTrackEditor::GenerateTrackClipsFromMfa(UDreamLipSyncTrack* Track)
{
	if (!Track)
	{
		return;
	}

	int32 SuccessCount = 0;
	int32 ClipCount = 0;
	TArray<FString> Messages;
	TSet<UDreamLipSyncClip*> ProcessedClips;

	for (UMovieSceneSection* Section : Track->GetAllSections())
	{
		const UDreamLipSyncSection* LipSyncSection = Cast<UDreamLipSyncSection>(Section);
		UDreamLipSyncClip* Clip = LipSyncSection ? LipSyncSection->GetClip() : nullptr;
		if (!Clip || ProcessedClips.Contains(Clip))
		{
			continue;
		}

		ProcessedClips.Add(Clip);
		++ClipCount;

		FString Message;
		if (FDreamLipSyncMfaGenerator::GenerateClipFromMfa(Clip, Message))
		{
			++SuccessCount;
		}

		Messages.Add(FString::Printf(TEXT("%s: %s"), *Clip->GetName(), *Message));
	}

	if (ClipCount == 0)
	{
		Messages.Add(TEXT("No DreamLipSyncClip assets are assigned to this track."));
	}

	FMessageDialog::Open(
		EAppMsgType::Ok,
		FText::FromString(FString::Join(Messages, TEXT("\n"))),
		SuccessCount == ClipCount && ClipCount > 0
			? LOCTEXT("GenerateTrackClipsFromMfaSucceeded", "Dream Lip Sync")
			: LOCTEXT("GenerateTrackClipsFromMfaIssues", "Dream Lip Sync - Issues"));
}

bool FDreamLipSyncTrackEditor::BindingCanResolveSkeletalMesh(const TArray<FGuid>& ObjectBindings) const
{
	TSharedPtr<ISequencer> Sequencer = GetSequencer();
	if (!Sequencer.IsValid())
	{
		return false;
	}

	for (const FGuid& BindingId : ObjectBindings)
	{
		TArrayView<TWeakObjectPtr<>> Objects = Sequencer->FindObjectsInCurrentSequence(BindingId);
		for (TWeakObjectPtr<> WeakObject : Objects)
		{
			if (UDreamLipSyncBlueprintLibrary::ResolveSkeletalMeshComponent(WeakObject.Get()))
			{
				return true;
			}
		}
	}

	return false;
}

#undef LOCTEXT_NAMESPACE
