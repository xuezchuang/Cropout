// Copyright (c) Cropout contributors. Licensed under the project license.

#include "CropoutInputModifiers.h"

FInputActionValue UCropoutInputModifierNormalize::ModifyRaw_Implementation(const UEnhancedPlayerInput* PlayerInput, FInputActionValue CurrentValue, float DeltaTime)
{
	const FVector Value = CurrentValue.Get<FVector>();
	const FVector Normalized(
		FMath::Clamp(Value.X / NormalizeRange, -1.0f, 1.0f),
		FMath::Clamp(Value.Y / NormalizeRange, -1.0f, 1.0f),
		FMath::Clamp(Value.Z / NormalizeRange, -1.0f, 1.0f));
	return Super::ModifyRaw_Implementation(PlayerInput, FInputActionValue(CurrentValue.GetValueType(), Normalized), DeltaTime);
}

FInputActionValue UCropoutInputModifierOffset::ModifyRaw_Implementation(const UEnhancedPlayerInput* PlayerInput, FInputActionValue CurrentValue, float DeltaTime)
{
	const FVector Value = CurrentValue.Get<FVector>();
	const FVector OffsetValue(Value.X + Offset, Value.Y + Offset, Value.Z + Offset);
	return Super::ModifyRaw_Implementation(PlayerInput, FInputActionValue(CurrentValue.GetValueType(), OffsetValue), DeltaTime);
}
