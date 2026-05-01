// Copyright 2026 DreamDev, Inc. All Rights Reserved.

#include "DreamLipSyncAceProviderOptions.h"

#if WITH_DREAMLIPSYNC_ACE
#include "A2FProvider.h"
#endif

TArray<FName> UDreamLipSyncAceProviderOptions::GetAceProviderOptions()
{
	TArray<FName> Options;
	Options.Add(TEXT("Default"));

#if WITH_DREAMLIPSYNC_ACE
	TArray<FName> ProviderNames = IA2FProvider::GetAvailableProviderNames();
	ProviderNames.Sort([](const FName& A, const FName& B)
	{
		return A.LexicalLess(B);
	});

	for (const FName& ProviderName : ProviderNames)
	{
		Options.AddUnique(ProviderName);
	}
#endif

	return Options;
}
