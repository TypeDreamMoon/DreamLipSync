// Copyright 2026 DreamDev, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DreamLipSyncBlueprintLibrary.generated.h"

class UDreamLipSyncClip;
class USkeletalMeshComponent;

UCLASS()
class DREAMLIPSYNC_API UDreamLipSyncBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Dream Lip Sync")
	static int32 ApplyLipSyncClipAtTime(USkeletalMeshComponent* TargetMesh, const UDreamLipSyncClip* Clip, float Time, float WeightScale = 1.f, bool bResetControlledMorphTargets = true);

	UFUNCTION(BlueprintCallable, Category = "Dream Lip Sync")
	static int32 ResetLipSyncMorphTargets(USkeletalMeshComponent* TargetMesh, const UDreamLipSyncClip* Clip);

	UFUNCTION(BlueprintPure, Category = "Dream Lip Sync")
	static USkeletalMeshComponent* ResolveSkeletalMeshComponent(UObject* Object);
};
