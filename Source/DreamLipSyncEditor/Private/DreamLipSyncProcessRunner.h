// Copyright 2026 DreamDev, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FDreamLipSyncProcessResult
{
	bool bStarted = false;
	bool bCanceled = false;
	int32 ReturnCode = 0;
	FString StdOut;
	FString StdErr;
};

struct FDreamLipSyncProcessRunner
{
	static bool RunWithProgress(
		const FString& ExecutablePath,
		const FString& Params,
		const FString& WorkingDirectory,
		const FText& Title,
		const FText& RunningMessage,
		FDreamLipSyncProcessResult& OutResult);
};
