// Copyright 2026 DreamDev, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "DreamLipSyncAceProviderOptions.generated.h"

UCLASS()
class DREAMLIPSYNCEDITOR_API UDreamLipSyncAceProviderOptions : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	static TArray<FName> GetAceProviderOptions();
};
