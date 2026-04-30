// Copyright 2026 DreamDev, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MovieSceneTrackEditor.h"

class UDreamLipSyncTrack;

class FDreamLipSyncTrackEditor : public FMovieSceneTrackEditor
{
public:
	explicit FDreamLipSyncTrackEditor(TSharedRef<ISequencer> InSequencer);

	static TSharedRef<ISequencerTrackEditor> CreateTrackEditor(TSharedRef<ISequencer> InSequencer);

	virtual void BuildObjectBindingTrackMenu(FMenuBuilder& MenuBuilder, const TArray<FGuid>& ObjectBindings, const UClass* ObjectClass) override;
	virtual TSharedPtr<SWidget> BuildOutlinerEditWidget(const FGuid& ObjectBinding, UMovieSceneTrack* Track, const FBuildEditWidgetParams& Params) override;
	virtual void BuildTrackContextMenu(FMenuBuilder& MenuBuilder, UMovieSceneTrack* Track) override;
	virtual TSharedRef<ISequencerSection> MakeSectionInterface(UMovieSceneSection& SectionObject, UMovieSceneTrack& Track, FGuid ObjectBinding) override;
	virtual bool SupportsType(TSubclassOf<UMovieSceneTrack> Type) const override;
	virtual bool SupportsSequence(UMovieSceneSequence* InSequence) const override;
	virtual const FSlateBrush* GetIconBrush() const override;

private:
	void HandleAddLipSyncTrackMenuEntryExecute(TArray<FGuid> ObjectBindings);
	void CreateLipSyncSection(UDreamLipSyncTrack* Track, int32 RowIndex);
	void GenerateTrackClipsFromRhubarb(UDreamLipSyncTrack* Track);
	void GenerateTrackClipsFromMfa(UDreamLipSyncTrack* Track);
	bool BindingCanResolveSkeletalMesh(const TArray<FGuid>& ObjectBindings) const;
};
