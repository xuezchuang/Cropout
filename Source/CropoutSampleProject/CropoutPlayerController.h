// Copyright (c) Cropout contributors. Licensed under the project license.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CropoutPlayerController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCropoutKeySwitchSignature, uint8, NewInput);

/**
 * ACropoutPlayerController
 *
 * C++ parent class for BP_PC.
 *
 * Current migration scope:
 *  - C++ owns BP_PC's connected EventGraph logic for the `MouseMove` axis.
 *  - C++ owns the former BP_PC `KeySwitch` dispatcher contract.
 *  - BP_PC still owns the BP-defined `InputType` enum variable until the enum
 *    contract is lifted in a later pass.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API ACropoutPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ACropoutPlayerController();

	/** Native replacement for BP_PC's former `KeySwitch(New Input)` dispatcher. */
	UPROPERTY(BlueprintAssignable, Category = "Cropout|Input")
	FCropoutKeySwitchSignature KeySwitch;

protected:
	virtual void SetupInputComponent() override;

private:
	void HandleMouseMoveAxis(float AxisValue);
	bool GetInputTypeValue(uint8& OutValue) const;
	bool SetInputTypeValue(uint8 NewValue);
	void BroadcastKeySwitch(uint8 NewValue);
};
