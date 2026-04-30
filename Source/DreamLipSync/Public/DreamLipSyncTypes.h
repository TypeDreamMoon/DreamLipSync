// Copyright 2026 DreamDev, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DreamLipSyncTypes.generated.h"

UENUM(BlueprintType)
enum class EDreamLipSyncDataMode : uint8
{
	VisemeKeys UMETA(DisplayName = "Viseme Keys"),
	MorphFrames UMETA(DisplayName = "Morph Frames")
};

USTRUCT(BlueprintType)
struct DREAMLIPSYNC_API FDreamLipSyncMorphWeight
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	FName MorphTargetName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float Weight = 0.f;
};

USTRUCT(BlueprintType)
struct DREAMLIPSYNC_API FDreamLipSyncMorphFrame
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Time = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	TArray<FDreamLipSyncMorphWeight> Weights;
};

USTRUCT(BlueprintType)
struct DREAMLIPSYNC_API FDreamLipSyncVisemeKey
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Time = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	FName Viseme = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0"))
	float Weight = 1.f;
};

USTRUCT(BlueprintType)
struct DREAMLIPSYNC_API FDreamLipSyncVisemeMapping
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	FName Viseme = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	FName MorphTargetName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Scale = 1.f;
};

USTRUCT(BlueprintType)
struct DREAMLIPSYNC_API FDreamLipSyncVisemeRecipeMix
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	FName Viseme = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Scale = 1.f;
};

USTRUCT(BlueprintType)
struct DREAMLIPSYNC_API FDreamLipSyncVisemeRecipe
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	FName SourceViseme = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	bool bIncludeBaseViseme = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bIncludeBaseViseme"))
	float BaseScale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	TArray<FDreamLipSyncVisemeRecipeMix> VisemeMixes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	TArray<FDreamLipSyncMorphWeight> ExtraMorphWeights;
};

USTRUCT(BlueprintType)
struct DREAMLIPSYNC_API FDreamLipSyncSourceMorphMapping
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	FName SourceName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync")
	FName MorphTargetName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lip Sync", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Scale = 1.f;
};
