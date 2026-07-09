// Copyright (c) Cropout contributors. Licensed under the project license.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CropoutTransitionWidget.generated.h"

class UWidgetAnimation;

/**
 * C++ parent for UI_Transition.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API UCropoutTransitionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Cropout|UI")
	void TransIn();

	UFUNCTION(BlueprintCallable, Category = "Cropout|UI")
	void TransOut();

protected:
	virtual void NativeDestruct() override;

	UPROPERTY(Transient, BlueprintReadOnly, meta = (BindWidgetAnimOptional), Category = "Cropout|UI")
	TObjectPtr<UWidgetAnimation> NewAnimation = nullptr;

private:
	FTimerHandle TransitionOutRemovalTimer;

	void RemoveAfterTransitionOut();
};
