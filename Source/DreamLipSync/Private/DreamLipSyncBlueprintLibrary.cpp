// Copyright 2026 DreamDev, Inc. All Rights Reserved.

#include "DreamLipSyncBlueprintLibrary.h"

#include "Components/SkeletalMeshComponent.h"
#include "DreamLipSyncClip.h"
#include "GameFramework/Actor.h"

int32 UDreamLipSyncBlueprintLibrary::ApplyLipSyncClipAtTime(USkeletalMeshComponent* TargetMesh, const UDreamLipSyncClip* Clip, float Time, float WeightScale, bool bResetControlledMorphTargets)
{
	if (!TargetMesh || !Clip)
	{
		return 0;
	}

	TSet<FName> ControlledMorphTargets;
	Clip->GetControlledMorphTargets(ControlledMorphTargets);

	if (bResetControlledMorphTargets)
	{
		for (const FName& MorphTargetName : ControlledMorphTargets)
		{
			TargetMesh->SetMorphTarget(MorphTargetName, 0.f, false);
		}
	}

	TMap<FName, float> EvaluatedWeights;
	Clip->EvaluateAtTime(Time, EvaluatedWeights);

	int32 AppliedCount = 0;
	for (const TPair<FName, float>& Pair : EvaluatedWeights)
	{
		if (Pair.Key.IsNone())
		{
			continue;
		}

		const float FinalWeight = FMath::Clamp(Pair.Value * WeightScale, 0.f, 1.f);
		TargetMesh->SetMorphTarget(Pair.Key, FinalWeight, false);
		++AppliedCount;
	}

#if WITH_EDITOR
	TargetMesh->SetUpdateAnimationInEditor(true);
#endif

	return AppliedCount;
}

int32 UDreamLipSyncBlueprintLibrary::ResetLipSyncMorphTargets(USkeletalMeshComponent* TargetMesh, const UDreamLipSyncClip* Clip)
{
	if (!TargetMesh || !Clip)
	{
		return 0;
	}

	TSet<FName> ControlledMorphTargets;
	Clip->GetControlledMorphTargets(ControlledMorphTargets);

	for (const FName& MorphTargetName : ControlledMorphTargets)
	{
		TargetMesh->SetMorphTarget(MorphTargetName, 0.f, false);
	}

#if WITH_EDITOR
	TargetMesh->SetUpdateAnimationInEditor(true);
#endif

	return ControlledMorphTargets.Num();
}

USkeletalMeshComponent* UDreamLipSyncBlueprintLibrary::ResolveSkeletalMeshComponent(UObject* Object)
{
	if (!Object)
	{
		return nullptr;
	}

	if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(Object))
	{
		return SkeletalMeshComponent;
	}

	if (AActor* Actor = Cast<AActor>(Object))
	{
		return Actor->FindComponentByClass<USkeletalMeshComponent>();
	}

	if (UActorComponent* ActorComponent = Cast<UActorComponent>(Object))
	{
		if (AActor* Owner = ActorComponent->GetOwner())
		{
			return Owner->FindComponentByClass<USkeletalMeshComponent>();
		}
	}

	return nullptr;
}
