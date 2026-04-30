// Copyright 2026 DreamDev, Inc. All Rights Reserved.

#include "MovieScene/DreamLipSyncSection.h"

#include "DreamLipSyncBlueprintLibrary.h"
#include "DreamLipSyncClip.h"
#include "EntitySystem/MovieSceneSharedPlaybackState.h"
#include "MovieScene.h"
#include "UObject/UnrealType.h"

UDreamLipSyncSection::UDreamLipSyncSection(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bRequiresRangedHook = true;
	bRequiresTriggerHooks = false;
	bSupportsInfiniteRange = false;

	SetRange(TRange<FFrameNumber>(FFrameNumber(0), FFrameNumber(120)));
	EvalOptions.EnableAndSetCompletionMode(EMovieSceneCompletionMode::KeepState);
}

void UDreamLipSyncSection::Begin(TSharedRef<FSharedPlaybackState> SharedPlaybackState, const UE::MovieScene::FEvaluationHookParams& Params) const
{
	ApplyAtCurrentTime(SharedPlaybackState, Params);
}

void UDreamLipSyncSection::Update(TSharedRef<FSharedPlaybackState> SharedPlaybackState, const UE::MovieScene::FEvaluationHookParams& Params) const
{
	ApplyAtCurrentTime(SharedPlaybackState, Params);
}

void UDreamLipSyncSection::End(TSharedRef<FSharedPlaybackState> SharedPlaybackState, const UE::MovieScene::FEvaluationHookParams& Params) const
{
	if (bResetMorphTargetsOnEnd)
	{
		ResetBoundTargets(SharedPlaybackState, Params);
	}
}

TOptional<TRange<FFrameNumber>> UDreamLipSyncSection::GetAutoSizeRange() const
{
	if (!Clip || !HasStartFrame())
	{
		return TOptional<TRange<FFrameNumber>>();
	}

	const FFrameNumber StartFrame = GetInclusiveStartFrame();
	const FFrameNumber DurationFrames = GetNaturalSectionFrameCount();
	if (DurationFrames.Value <= 0)
	{
		return TOptional<TRange<FFrameNumber>>();
	}

	return TRange<FFrameNumber>(
		TRangeBound<FFrameNumber>::Inclusive(StartFrame),
		TRangeBound<FFrameNumber>::Exclusive(StartFrame + DurationFrames));
}

void UDreamLipSyncSection::SetStartFrame(TRangeBound<FFrameNumber> NewStartFrame)
{
	if (bLockSectionLengthToClipDuration && !bStretchClipToSection && Clip && HasEndFrame() && NewStartFrame.IsClosed())
	{
		const FFrameNumber DurationFrames = GetNaturalSectionFrameCount();
		if (DurationFrames.Value > 0)
		{
			const FFrameNumber MinStartFrame = GetExclusiveEndFrame() - DurationFrames;
			if (NewStartFrame.GetValue() < MinStartFrame)
			{
				NewStartFrame = TRangeBound<FFrameNumber>::Inclusive(MinStartFrame);
			}
		}
	}

	Super::SetStartFrame(NewStartFrame);
}

void UDreamLipSyncSection::SetEndFrame(TRangeBound<FFrameNumber> NewEndFrame)
{
	if (bLockSectionLengthToClipDuration && !bStretchClipToSection && Clip && HasStartFrame() && NewEndFrame.IsClosed())
	{
		const FFrameNumber DurationFrames = GetNaturalSectionFrameCount();
		if (DurationFrames.Value > 0)
		{
			const FFrameNumber MaxEndFrame = GetInclusiveStartFrame() + DurationFrames;
			if (NewEndFrame.GetValue() > MaxEndFrame)
			{
				NewEndFrame = TRangeBound<FFrameNumber>::Exclusive(MaxEndFrame);
			}
		}
	}

	Super::SetEndFrame(NewEndFrame);
}

#if WITH_EDITOR
void UDreamLipSyncSection::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (bAutoSizeToClipDuration &&
		(PropertyName == GET_MEMBER_NAME_CHECKED(UDreamLipSyncSection, Clip) ||
		 PropertyName == GET_MEMBER_NAME_CHECKED(UDreamLipSyncSection, StartOffset) ||
		 PropertyName == GET_MEMBER_NAME_CHECKED(UDreamLipSyncSection, TimeScale) ||
		 PropertyName == GET_MEMBER_NAME_CHECKED(UDreamLipSyncSection, bAutoSizeToClipDuration)))
	{
		AutoSizeToClipDuration();
	}
}
#endif

void UDreamLipSyncSection::SetClip(UDreamLipSyncClip* InClip)
{
	Clip = InClip;
	if (bAutoSizeToClipDuration)
	{
		AutoSizeToClipDuration();
	}
}

void UDreamLipSyncSection::AutoSizeToClipDuration()
{
	TOptional<TRange<FFrameNumber>> AutoSizeRange = GetAutoSizeRange();
	if (AutoSizeRange.IsSet())
	{
		SetRange(AutoSizeRange.GetValue());
	}
}

void UDreamLipSyncSection::ApplyAtCurrentTime(TSharedRef<FSharedPlaybackState> SharedPlaybackState, const UE::MovieScene::FEvaluationHookParams& Params) const
{
	if (!Clip || (!bEvaluateWhenSilent && Params.Context.IsSilent()))
	{
		return;
	}

	const float LocalTime = GetLocalLipSyncTime(Params);
	if (bClampEvaluationToClipDuration && !bStretchClipToSection)
	{
		const float ClipDuration = Clip->GetResolvedDuration();
		if (ClipDuration > 0.f && LocalTime > ClipDuration)
		{
			ResetBoundTargets(SharedPlaybackState, Params);
			return;
		}
	}

	TArrayView<TWeakObjectPtr<>> BoundObjects = SharedPlaybackState->FindBoundObjects(Params.ObjectBindingID, Params.SequenceID);

	for (TWeakObjectPtr<> WeakObject : BoundObjects)
	{
		UObject* BoundObject = WeakObject.Get();
		USkeletalMeshComponent* Mesh = UDreamLipSyncBlueprintLibrary::ResolveSkeletalMeshComponent(BoundObject);
		if (Mesh)
		{
			UDreamLipSyncBlueprintLibrary::ApplyLipSyncClipAtTime(Mesh, Clip, LocalTime, WeightScale, bResetControlledMorphTargetsEachFrame);
		}
	}
}

void UDreamLipSyncSection::ResetBoundTargets(TSharedRef<FSharedPlaybackState> SharedPlaybackState, const UE::MovieScene::FEvaluationHookParams& Params) const
{
	if (!Clip)
	{
		return;
	}

	TArrayView<TWeakObjectPtr<>> BoundObjects = SharedPlaybackState->FindBoundObjects(Params.ObjectBindingID, Params.SequenceID);

	for (TWeakObjectPtr<> WeakObject : BoundObjects)
	{
		UObject* BoundObject = WeakObject.Get();
		USkeletalMeshComponent* Mesh = UDreamLipSyncBlueprintLibrary::ResolveSkeletalMeshComponent(BoundObject);
		if (Mesh)
		{
			UDreamLipSyncBlueprintLibrary::ResetLipSyncMorphTargets(Mesh, Clip);
		}
	}
}

float UDreamLipSyncSection::GetLocalLipSyncTime(const UE::MovieScene::FEvaluationHookParams& Params) const
{
	const FFrameRate FrameRate = Params.Context.GetFrameRate();
	const double StartSeconds = HasStartFrame() ? FrameRate.AsSeconds(FFrameTime(GetInclusiveStartFrame())) : 0.0;
	const double CurrentSeconds = FrameRate.AsSeconds(Params.Context.GetTime());
	const double LocalSeconds = FMath::Max(0.0, CurrentSeconds - StartSeconds);

	if (bStretchClipToSection && Clip && HasStartFrame() && HasEndFrame())
	{
		const double SectionDurationSeconds = FrameRate.AsSeconds(FFrameTime(GetExclusiveEndFrame() - GetInclusiveStartFrame()));
		const float ClipDuration = Clip->GetResolvedDuration();
		const float PlayableClipDuration = FMath::Max(0.f, ClipDuration - StartOffset);
		if (SectionDurationSeconds > KINDA_SMALL_NUMBER && PlayableClipDuration > 0.f)
		{
			const double NormalizedTime = FMath::Clamp(LocalSeconds / SectionDurationSeconds, 0.0, 1.0);
			return StartOffset + static_cast<float>(NormalizedTime) * PlayableClipDuration;
		}
	}

	return StartOffset + static_cast<float>(LocalSeconds) * TimeScale;
}

float UDreamLipSyncSection::GetNaturalSectionDuration() const
{
	if (!Clip)
	{
		return 0.f;
	}

	const float ClipDuration = Clip->GetResolvedDuration();
	if (ClipDuration <= 0.f)
	{
		return 0.f;
	}

	const float PlayableDuration = FMath::Max(0.f, ClipDuration - StartOffset);
	return PlayableDuration / FMath::Max(KINDA_SMALL_NUMBER, TimeScale);
}

FFrameNumber UDreamLipSyncSection::GetNaturalSectionFrameCount() const
{
	const float NaturalDuration = GetNaturalSectionDuration();
	if (NaturalDuration <= 0.f)
	{
		return FFrameNumber(0);
	}

	const UMovieScene* MovieScene = GetTypedOuter<UMovieScene>();
	const FFrameRate FrameRate = MovieScene ? MovieScene->GetTickResolution() : FFrameRate(30, 1);
	return FMath::Max(FFrameNumber(1), FrameRate.AsFrameNumber(NaturalDuration));
}
