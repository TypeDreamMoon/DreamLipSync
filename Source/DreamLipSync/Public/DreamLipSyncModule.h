// Copyright 2026 DreamDev, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FDreamLipSyncModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
