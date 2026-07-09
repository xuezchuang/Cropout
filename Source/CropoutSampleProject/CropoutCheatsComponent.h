// Copyright (c) Cropout contributors. Licensed under the project license.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CropoutCheatsComponent.generated.h"

/**
 * C++ parent for BPC_Cheats.
 *
 * The migrated Blueprint graph contains no executable logic: its key input
 * nodes and old resource message nodes are all disconnected.
 */
UCLASS(Blueprintable, ClassGroup=(Cropout), meta=(BlueprintSpawnableComponent))
class CROPOUTSAMPLEPROJECT_API UCropoutCheatsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCropoutCheatsComponent();
};
