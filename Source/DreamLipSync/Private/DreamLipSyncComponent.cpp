// Copyright 2026 DreamDev, Inc. All Rights Reserved.

#include "DreamLipSyncComponent.h"

#include "Components/AudioComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DreamLipSyncBlueprintLibrary.h"
#include "DreamLipSyncClip.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

UDreamLipSyncComponent::UDreamLipSyncComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UDreamLipSyncComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoPlay && DefaultClip)
	{
		PlayLipSyncClip(DefaultClip, true, 0.f, 1.f);
	}
}

void UDreamLipSyncComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopLipSync(bResetMorphTargetsOnStop, true);

	Super::EndPlay(EndPlayReason);
}

void UDreamLipSyncComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bPlaying || !ActiveClip)
	{
		SetComponentTickEnabled(false);
		return;
	}

	PlaybackTime += DeltaTime * PlaybackRate;

	const float Duration = ActiveClip->GetResolvedDuration();
	if (Duration > 0.f && PlaybackTime > Duration)
	{
		StopLipSync(bResetMorphTargetsOnStop, true);
		return;
	}

	ApplyCurrentFrame();
}

void UDreamLipSyncComponent::PlayLipSyncClip(UDreamLipSyncClip* InClip, bool bPlayAudio, float StartTime, float InPlaybackRate)
{
	if (!InClip)
	{
		StopLipSync(bResetMorphTargetsOnStop, true);
		return;
	}

	if (bPlaying && ActiveClip && bResetMorphTargetsOnStop)
	{
		if (USkeletalMeshComponent* Mesh = ResolveTargetMesh())
		{
			UDreamLipSyncBlueprintLibrary::ResetLipSyncMorphTargets(Mesh, ActiveClip);
		}
	}

	ActiveClip = InClip;
	PlaybackTime = FMath::Max(0.f, StartTime);
	PlaybackRate = InPlaybackRate > KINDA_SMALL_NUMBER ? InPlaybackRate : 1.f;
	bPlaying = true;

	ApplyCurrentFrame();
	SetComponentTickEnabled(true);

	if (bPlayAudio)
	{
		StartClipAudio(PlaybackTime);
	}
}

void UDreamLipSyncComponent::StopLipSync(bool bResetMorphTargets, bool bStopAudio)
{
	if (bResetMorphTargets && ActiveClip)
	{
		if (USkeletalMeshComponent* Mesh = ResolveTargetMesh())
		{
			UDreamLipSyncBlueprintLibrary::ResetLipSyncMorphTargets(Mesh, ActiveClip);
		}
	}

	if (bStopAudio)
	{
		StopClipAudio();
	}

	bPlaying = false;
	PlaybackTime = 0.f;
	ActiveClip = nullptr;
	SetComponentTickEnabled(false);
}

void UDreamLipSyncComponent::SetLipSyncPlaybackTime(float NewTime, bool bApplyImmediately)
{
	PlaybackTime = FMath::Max(0.f, NewTime);

	if (bApplyImmediately)
	{
		ApplyCurrentFrame();
	}
}

USkeletalMeshComponent* UDreamLipSyncComponent::ResolveTargetMesh() const
{
	if (TargetMesh)
	{
		return TargetMesh;
	}

	return UDreamLipSyncBlueprintLibrary::ResolveSkeletalMeshComponent(GetOwner());
}

UAudioComponent* UDreamLipSyncComponent::ResolveAudioComponent() const
{
	if (AudioComponent)
	{
		return AudioComponent;
	}

	return GetOwner() ? GetOwner()->FindComponentByClass<UAudioComponent>() : nullptr;
}

void UDreamLipSyncComponent::ApplyCurrentFrame()
{
	if (!ActiveClip)
	{
		return;
	}

	if (USkeletalMeshComponent* Mesh = ResolveTargetMesh())
	{
		UDreamLipSyncBlueprintLibrary::ApplyLipSyncClipAtTime(Mesh, ActiveClip, PlaybackTime, WeightScale, bResetControlledMorphTargetsEachFrame);
	}
}

void UDreamLipSyncComponent::StartClipAudio(float StartTime)
{
	if (!ActiveClip || !ActiveClip->Sound)
	{
		return;
	}

	StopClipAudio();

	if (UAudioComponent* ExistingAudioComponent = ResolveAudioComponent())
	{
		ExistingAudioComponent->SetSound(ActiveClip->Sound);
		ExistingAudioComponent->Play(StartTime);
		ManagedAudioComponent = ExistingAudioComponent;
		return;
	}

	USceneComponent* AttachComponent = ResolveTargetMesh();
	if (!AttachComponent && GetOwner())
	{
		AttachComponent = GetOwner()->GetRootComponent();
	}

	if (AttachComponent)
	{
		ManagedAudioComponent = UGameplayStatics::SpawnSoundAttached(
			ActiveClip->Sound,
			AttachComponent,
			NAME_None,
			FVector::ZeroVector,
			EAttachLocation::KeepRelativeOffset,
			false,
			1.f,
			1.f,
			StartTime);
	}
	else
	{
		ManagedAudioComponent = UGameplayStatics::SpawnSound2D(this, ActiveClip->Sound, 1.f, 1.f, StartTime);
	}
}

void UDreamLipSyncComponent::StopClipAudio()
{
	if (ManagedAudioComponent)
	{
		ManagedAudioComponent->Stop();
		ManagedAudioComponent = nullptr;
	}
}
