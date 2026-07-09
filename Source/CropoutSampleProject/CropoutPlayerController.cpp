// Copyright (c) Cropout contributors. Licensed under the project license.

#include "CropoutPlayerController.h"

#include "Components/InputComponent.h"
#include "UObject/UnrealType.h"

namespace
{
	constexpr uint8 BPPC_InputType_NewEnumerator0 = 0;
	constexpr uint8 BPPC_InputType_NewEnumerator1 = 1;
}

ACropoutPlayerController::ACropoutPlayerController()
{
}

void ACropoutPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		InputComponent->BindAxis(TEXT("MouseMove"), this, &ACropoutPlayerController::HandleMouseMoveAxis);
	}
}

void ACropoutPlayerController::HandleMouseMoveAxis(float AxisValue)
{
	// BP_PC DSL:
	//   InputAxisMouseMove(AxisValue)
	//     if AxisValue != 0 and InputType == NewEnumerator0:
	//       InputType = NewEnumerator1
	//       KeySwitch(InputType)
	if (AxisValue == 0.0f)
	{
		return;
	}

	uint8 CurrentInputType = 0;
	if (!GetInputTypeValue(CurrentInputType) || CurrentInputType != BPPC_InputType_NewEnumerator0)
	{
		return;
	}

	if (SetInputTypeValue(BPPC_InputType_NewEnumerator1))
	{
		BroadcastKeySwitch(BPPC_InputType_NewEnumerator1);
	}
}

bool ACropoutPlayerController::GetInputTypeValue(uint8& OutValue) const
{
	FProperty* Property = FindFProperty<FProperty>(GetClass(), TEXT("InputType"));
	if (FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
	{
		OutValue = ByteProperty->GetPropertyValue_InContainer(this);
		return true;
	}

	if (FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
	{
		if (FNumericProperty* UnderlyingProperty = EnumProperty->GetUnderlyingProperty())
		{
			const void* ValuePtr = EnumProperty->ContainerPtrToValuePtr<void>(this);
			OutValue = static_cast<uint8>(UnderlyingProperty->GetUnsignedIntPropertyValue(ValuePtr));
			return true;
		}
	}

	return false;
}

bool ACropoutPlayerController::SetInputTypeValue(uint8 NewValue)
{
	FProperty* Property = FindFProperty<FProperty>(GetClass(), TEXT("InputType"));
	if (FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
	{
		ByteProperty->SetPropertyValue_InContainer(this, NewValue);
		return true;
	}

	if (FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
	{
		if (FNumericProperty* UnderlyingProperty = EnumProperty->GetUnderlyingProperty())
		{
			void* ValuePtr = EnumProperty->ContainerPtrToValuePtr<void>(this);
			UnderlyingProperty->SetIntPropertyValue(ValuePtr, static_cast<uint64>(NewValue));
			return true;
		}
	}

	return false;
}

void ACropoutPlayerController::BroadcastKeySwitch(uint8 NewValue)
{
	KeySwitch.Broadcast(NewValue);
}
