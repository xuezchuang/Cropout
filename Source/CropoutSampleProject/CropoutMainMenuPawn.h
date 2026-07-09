// Copyright (c) Cropout contributors. Licensed under the project license.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "CropoutMainMenuPawn.generated.h"

/**
 * Native empty parent for BP_MenuPawn.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API ACropoutMainMenuPawn : public APawn
{
	GENERATED_BODY()

public:
	ACropoutMainMenuPawn();
};
