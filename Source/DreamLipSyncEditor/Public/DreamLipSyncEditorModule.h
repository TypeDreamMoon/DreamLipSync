// Copyright 2026 DreamDev, Inc. All Rights Reserved.

#pragma once

#include "AssetTypeActions_Base.h"
#include "Modules/ModuleManager.h"

class FDreamLipSyncEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterMenus();

	FDelegateHandle TrackEditorHandle;
	TArray<TSharedPtr<FAssetTypeActions_Base>> CreatedAssetTypeActions;
};
