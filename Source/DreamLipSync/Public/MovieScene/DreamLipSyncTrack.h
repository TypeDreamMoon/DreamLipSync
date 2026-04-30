// Copyright 2026 DreamDev, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MovieSceneNameableTrack.h"
#include "DreamLipSyncTrack.generated.h"

UCLASS(BlueprintType)
class DREAMLIPSYNC_API UDreamLipSyncTrack : public UMovieSceneNameableTrack
{
	GENERATED_BODY()

public:
	UDreamLipSyncTrack(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual bool SupportsType(TSubclassOf<UMovieSceneSection> SectionClass) const override;
	virtual UMovieSceneSection* CreateNewSection() override;
	virtual bool SupportsMultipleRows() const override;

	virtual void RemoveAllAnimationData() override;
	virtual bool HasSection(const UMovieSceneSection& Section) const override;
	virtual void AddSection(UMovieSceneSection& Section) override;
	virtual void RemoveSection(UMovieSceneSection& Section) override;
	virtual void RemoveSectionAt(int32 SectionIndex) override;
	virtual bool IsEmpty() const override;
	virtual const TArray<UMovieSceneSection*>& GetAllSections() const override;

#if WITH_EDITORONLY_DATA
	virtual FText GetDefaultDisplayName() const override;
#endif

protected:
	UPROPERTY()
	TArray<TObjectPtr<UMovieSceneSection>> Sections;
};
