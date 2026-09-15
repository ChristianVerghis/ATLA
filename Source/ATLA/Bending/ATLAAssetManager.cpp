// Copyright Epic Games, Inc. All Rights Reserved.

#include "ATLAAssetManager.h"
#include "AbilitySystemGlobals.h"

void UATLAAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();

	UAbilitySystemGlobals::Get().InitGlobalData();
}
