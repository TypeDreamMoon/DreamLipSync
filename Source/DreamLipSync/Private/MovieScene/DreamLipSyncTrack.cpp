// Copyright 2026 DreamDev, Inc. All Rights Reserved.

#include "MovieScene/DreamLipSyncTrack.h"

#include "MovieScene/DreamLipSyncSection.h"

UDreamLipSyncTrack::UDreamLipSyncTrack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	EvalOptions.bCanEvaluateNearestSection = false;
}

bool UDreamLipSyncTrack::SupportsType(TSubclassOf<UMovieSceneSection> SectionClass) const
{
	return SectionClass == UDreamLipSyncSection::StaticClass();
}

UMovieSceneSection* UDreamLipSyncTrack::CreateNewSection()
{
	return NewObject<UDreamLipSyncSection>(this, NAME_None, RF_Transactional);
}

bool UDreamLipSyncTrack::SupportsMultipleRows() const
{
	return true;
}

void UDreamLipSyncTrack::RemoveAllAnimationData()
{
	Sections.Empty();
}

bool UDreamLipSyncTrack::HasSection(const UMovieSceneSection& Section) const
{
	return Sections.Contains(&Section);
}

void UDreamLipSyncTrack::AddSection(UMovieSceneSection& Section)
{
	Sections.Add(&Section);
}

void UDreamLipSyncTrack::RemoveSection(UMovieSceneSection& Section)
{
	Sections.Remove(&Section);
}

void UDreamLipSyncTrack::RemoveSectionAt(int32 SectionIndex)
{
	if (Sections.IsValidIndex(SectionIndex))
	{
		Sections.RemoveAt(SectionIndex);
	}
}

bool UDreamLipSyncTrack::IsEmpty() const
{
	return Sections.IsEmpty();
}

const TArray<UMovieSceneSection*>& UDreamLipSyncTrack::GetAllSections() const
{
	return Sections;
}

#if WITH_EDITORONLY_DATA
FText UDreamLipSyncTrack::GetDefaultDisplayName() const
{
	return NSLOCTEXT("DreamLipSync", "DreamLipSyncTrackName", "Dream Lip Sync");
}
#endif
