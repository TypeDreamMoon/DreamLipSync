// Copyright 2026 DreamDev, Inc. All Rights Reserved.

#include "DreamLipSyncSettings.h"

const UDreamLipSyncSettings* UDreamLipSyncSettings::Get()
{
	return GetDefault<UDreamLipSyncSettings>();
}

FName UDreamLipSyncSettings::GetCategoryName() const
{
	return TEXT("DreamPlugin");
}
