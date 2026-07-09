// Copyright (c) Cropout contributors. Licensed under the project license.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CropoutMainMenuGameMode.generated.h"

/**
 * C++ parent for BP_MainMenuGM.
 *
 * Owns the former BP_MainMenuGM EventGraph BeginPlay flow. The Blueprint child
 * should keep asset defaults only.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API ACropoutMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
};
