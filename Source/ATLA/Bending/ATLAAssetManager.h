// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "ATLAAssetManager.generated.h"

/**
 * Project asset manager. Exists to initialize GAS global data at the right
 * point in startup (required for gameplay ability target data replication).
 * Registered in DefaultEngine.ini under [/Script/Engine.Engine].
 */
UCLASS()
class ATLA_API UATLAAssetManager : public UAssetManager
{
	GENERATED_BODY()

protected:
	virtual void StartInitialLoading() override;
};
