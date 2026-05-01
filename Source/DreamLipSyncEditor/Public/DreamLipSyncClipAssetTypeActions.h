// Copyright 2026 DreamDev, Inc. All Rights Reserved.

#pragma once

#include "AssetTypeActions_Base.h"
#include "CoreMinimal.h"

class UDreamLipSyncClip;

class FDreamLipSyncClipAssetTypeActions : public FAssetTypeActions_Base
{
public:
	virtual FText GetName() const override;
	virtual FColor GetTypeColor() const override;
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override;
	virtual void GetActions(const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder) override;

private:
	void GenerateFromAce(TArray<TWeakObjectPtr<UDreamLipSyncClip>> Clips);
	void GenerateFromRhubarb(TArray<TWeakObjectPtr<UDreamLipSyncClip>> Clips);
	void ImportRhubarbJson(TArray<TWeakObjectPtr<UDreamLipSyncClip>> Clips);
	void GenerateFromMfa(TArray<TWeakObjectPtr<UDreamLipSyncClip>> Clips);
	void ImportMfaTextGrid(TArray<TWeakObjectPtr<UDreamLipSyncClip>> Clips);
};
