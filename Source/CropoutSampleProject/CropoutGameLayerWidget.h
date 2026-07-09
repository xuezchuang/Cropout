// Copyright (c) Cropout contributors. Licensed under the project license.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "TimerManager.h"
#include "CropoutGameLayerWidget.generated.h"

class UCommonActivatableWidgetStack;
class UCommonButtonBase;
class UCommonTextBlock;
class UUserWidget;
class UVerticalBox;
class ACropoutPlayerController;

/**
 * Native parent for UI_Layer_Game.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API UCropoutGameLayerWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Cropout|UI")
	void AddResource();

	UFUNCTION(BlueprintCallable, Category = "Cropout|UI")
	void AddStackItem(TSubclassOf<UUserWidget> ActivatableWidgetClass);

	UFUNCTION(BlueprintCallable, Category = "Cropout|UI")
	void PullCurrentActiveWidget();

	UFUNCTION(BlueprintCallable, Category = "Cropout|UI")
	void EndGame(bool Win);

	UFUNCTION(BlueprintCallable, Category = "Cropout|UI")
	void UpdateVillagerDetails(int32 VillagerCount);

	UFUNCTION(BlueprintCallable, Category = "Cropout|UI")
	void HidePause(uint8 NewInput);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Cropout|UI")
	TObjectPtr<UCommonTextBlock> villagerCounter = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Cropout|UI")
	TObjectPtr<UVerticalBox> resourceContainer = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Cropout|UI")
	TObjectPtr<UCommonActivatableWidgetStack> mainStack = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Cropout|UI")
	TObjectPtr<UCommonButtonBase> BTN_Pause = nullptr;

private:
	void BindGameModeEvents();
	void BindPlayerControllerEvents();
	void StopResourceTimer();
	void SetResourceProperty(UUserWidget* ResourceWidget, uint8 ResourceValue) const;
	UCommonActivatableWidget* PushActivatableWidgetClass(TSubclassOf<UUserWidget> WidgetClass);

	void HandlePauseClicked();

	FTimerHandle AddResourceTimerHandle;
	int32 ResourceSequenceIndex = 0;
	TWeakObjectPtr<ACropoutPlayerController> KeySwitchSource;
};
