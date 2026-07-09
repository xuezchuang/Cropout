// Copyright (c) Cropout contributors. Licensed under the project license.

#pragma once

#include "CoreMinimal.h"
#include "InputModifiers.h"
#include "CropoutInputModifiers.generated.h"

/**
 * Native replacement for /Game/Blueprint/Core/Player/Input/IM_Normalize.
 */
UCLASS(Blueprintable, EditInlineNew, CollapseCategories, Config = Input)
class CROPOUTSAMPLEPROJECT_API UCropoutInputModifierNormalize : public UInputModifier
{
	GENERATED_BODY()

public:
	/** BP parity: `normalize Range`, default 90. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cropout|Input", meta = (DisplayName = "Normalize Range"))
	float NormalizeRange = 90.0f;

protected:
	virtual FInputActionValue ModifyRaw_Implementation(const UEnhancedPlayerInput* PlayerInput, FInputActionValue CurrentValue, float DeltaTime) override;
};

/**
 * Native replacement for /Game/Blueprint/Core/Player/Input/IM_Offset.
 */
UCLASS(Blueprintable, EditInlineNew, CollapseCategories, Config = Input)
class CROPOUTSAMPLEPROJECT_API UCropoutInputModifierOffset : public UInputModifier
{
	GENERATED_BODY()

public:
	/** BP parity: `Offset`, default 0.2. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cropout|Input")
	float Offset = 0.2f;

protected:
	virtual FInputActionValue ModifyRaw_Implementation(const UEnhancedPlayerInput* PlayerInput, FInputActionValue CurrentValue, float DeltaTime) override;
};
