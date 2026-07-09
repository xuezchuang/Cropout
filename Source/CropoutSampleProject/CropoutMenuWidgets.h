// Copyright (c) Cropout contributors. Licensed under the project license.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "CommonButtonBase.h"
#include "CropoutResourceType.h"
#include "Engine/DataTable.h"
#include "Kismet/KismetMathLibrary.h"
#include "CropoutMenuWidgets.generated.h"

class UWidget;
class UUserWidget;
class UWidgetAnimation;
class UDataTable;
class UCommonActionWidget;
class UCommonButtonStyle;
class UCommonTextBlock;
class UCommonActivatableWidgetStack;
class UImage;
class USlider;
class USoundClass;
class USoundMix;
class USizeBox;
class APawn;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCropoutPromptAction);

/**
 * Native parent for UI_MainMenu's desired-focus override.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API UCropoutMainMenuWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnActivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Cropout|MainMenu")
	TObjectPtr<class UCommonButtonBase> BTN_Continue = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Cropout|MainMenu")
	TObjectPtr<class UCommonButtonBase> BTN_NewGame = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Cropout|MainMenu")
	TObjectPtr<class UCommonButtonBase> BTN_Quit = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Cropout|MainMenu")
	TObjectPtr<class UCommonButtonBase> BTN_Donate = nullptr;

public:
	UFUNCTION(BlueprintCallable, Category = "Cropout|MainMenu")
	void OpenLevel();

	UFUNCTION(BlueprintCallable, Category = "Cropout|MainMenu")
	void ConfirmNewGame();

	UFUNCTION(BlueprintCallable, Category = "Cropout|MainMenu")
	void ConfirmQuit();

	UFUNCTION(BlueprintCallable, Category = "Cropout|MainMenu")
	void CallDonate();

	UFUNCTION(BlueprintCallable, Category = "Cropout|MainMenu")
	void ConfirmDonate();

private:
	void HandleContinueClicked();
	void HandleNewGameClicked();
	void HandleQuitClicked();
	void HandleDonateClicked();
	void OpenVillage();
	void UpdateSaveButtonState();
	void UpdateDonateVisibility();
	void PushDonationPrompt(const FText& PromptQuestion);
	void HandleDonationCheckoutComplete(bool bWasSuccessful, const FString& TransactionIdentifier, const FString& ValidationInfo);
};

/**
 * Native parent for UI_Layer_Menu.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API UCropoutMainMenuLayerWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Add Stack Item"), Category = "Cropout|MainMenu")
	void AddStackItem(TSubclassOf<UUserWidget> ActivatableWidgetClass);

protected:
	virtual void NativeOnActivated() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Cropout|MainMenu")
	TObjectPtr<UCommonActivatableWidgetStack> MainStack = nullptr;

private:
	UCommonActivatableWidget* PushActivatableWidgetClass(TSubclassOf<UUserWidget> ActivatableWidgetClass);
	void AssignStackRef(UCommonActivatableWidget* Widget) const;
};

/**
 * Native parent for UI_Build's desired-focus override.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API UCropoutBuildWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnActivated() override;
	virtual void NativePreConstruct() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Cropout|Build")
	TObjectPtr<class UCommonButtonBase> BTN_Back = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Cropout|Build")
	TObjectPtr<class UHorizontalBox> Container = nullptr;

private:
	void HandleBackClicked();
	void RebuildBuildItems();
	void CallSwitchBuildMode(APawn* ControlledPawn, bool bEnabled);
};

/**
 * Native parent for UI_EndGame's desired-focus override.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API UCropoutEndGameWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Cropout|UI")
	void EndGame(bool bWin);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnActivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Cropout|UI")
	TObjectPtr<class UCommonButtonBase> BTN_Continue = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Cropout|UI")
	TObjectPtr<class UCommonButtonBase> BTN_MainMenu = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Cropout|UI")
	TObjectPtr<class UCommonButtonBase> BTN_Retry = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Cropout|UI")
	TObjectPtr<class UCommonTextBlock> MainText = nullptr;

private:
	void HandleContinueClicked();
	void HandleMainMenuClicked();
	void HandleRetryClicked();
	void OpenLevelWithGameInstance(const TCHAR* LevelPath);
};

/**
 * Native parent for UI_Pause's desired-focus override.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API UCropoutPauseWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnActivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Cropout|UI")
	TObjectPtr<class UCommonButtonBase> BTN_Resume = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Cropout|UI")
	TObjectPtr<class UCommonButtonBase> BTN_Restart = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Cropout|UI")
	TObjectPtr<class UCommonButtonBase> BTN_MainMenu = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Cropout|UI")
	TObjectPtr<UUserWidget> Slider_Music = nullptr;

private:
	void HandleResumeClicked();
	void HandleRestartClicked();
	void HandleMainMenuClicked();
	void OpenLevelWithGameInstance(const TCHAR* LevelPath);
	void UpdateMusicSlider();
};

/**
 * Native parent for UI_GameMain.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API UCropoutGameMainWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Cropout|UI")
	FText Get_VillagerCount_Text() const;

	UFUNCTION(BlueprintCallable, Category = "Cropout|Input")
	void UI_GameMain_AutoGenFunc(uint8 NewInput);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnActivated() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cropout|UI")
	TSubclassOf<UUserWidget> BuildWidgetClass;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Cropout|UI")
	TObjectPtr<class UCommonButtonBase> CUI_Button_55 = nullptr;

private:
	void HandleBuildClicked();
	void BindPlayerControllerEvents();
	void UnbindPlayerControllerEvents();
	bool ReadPlayerInputType(uint8& OutInputType) const;

	TWeakObjectPtr<class ACropoutPlayerController> KeySwitchSource;
};

/**
 * Native parent for UI_Prompt.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API UCropoutPromptWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cropout|UI")
	FText PromptQuestion;

	UPROPERTY(BlueprintAssignable, Category = "Cropout|UI")
	FCropoutPromptAction Confirm;

	UPROPERTY(BlueprintAssignable, Category = "Cropout|UI")
	FCropoutPromptAction Back;

	UFUNCTION(BlueprintCallable, Category = "Cropout|UI")
	void SetPromptQuestion(const FText& InPromptQuestion);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnActivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Cropout|UI")
	TObjectPtr<class UCommonButtonBase> BTN_Pos = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Cropout|UI")
	TObjectPtr<class UCommonButtonBase> BTN_Neg = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Cropout|UI")
	TObjectPtr<class UCommonTextBlock> Title = nullptr;

private:
	void HandlePositiveClicked();
	void HandleNegativeClicked();
	void ClearPromptDelegates();
};

/**
 * Native parent for UI_BuildConfirm.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API UCropoutBuildConfirmWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Cropout|Input")
	void UI_GameMain_AutoGenFunc(uint8 NewInput);

	UFUNCTION(BlueprintCallable, Category = "Cropout|UI")
	void Confirm();

	UFUNCTION(BlueprintCallable, Category = "Cropout|UI")
	void Back();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnActivated() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Cropout|Build")
	TObjectPtr<class UCommonButtonBase> BTN_Pos = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Cropout|Build")
	TObjectPtr<class UCommonButtonBase> BTN_Pos_1 = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Cropout|Build")
	TObjectPtr<class UCommonButtonBase> BTN_Neg = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Cropout|Build")
	TObjectPtr<UWidget> CommonBorder_1 = nullptr;

private:
	void HandleBuildClicked();
	void HandleRotateClicked();
	void HandleCancelClicked();
	void BindPlayerControllerEvents();
	void UnbindPlayerControllerEvents();
	bool ReadPlayerInputType(uint8& OutInputType) const;
	UObject* GetBuildSpawn() const;
	FVector2D CalculateBorderPosition() const;
	void SetBorderTranslation(const FVector2D& Translation);
	void CallControlledPawnFunction(FName FunctionName);

	TWeakObjectPtr<class ACropoutPlayerController> KeySwitchSource;
	FVectorSpringState SpringState;
};

/**
 * Native parent for CUI_Button.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API UCropoutCommonButton : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cropout|UI", meta = (DisplayName = "Button Text"))
	FText ButtonText;

protected:
	virtual void NativePreConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Cropout|UI")
	TObjectPtr<UCommonTextBlock> ButtonTitle = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Cropout|UI")
	TObjectPtr<USizeBox> SizeBox_42 = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Cropout|UI")
	TObjectPtr<UCommonActionWidget> GamepadIcon = nullptr;
};

/**
 * Native parent for UIE_Cost.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API UCropoutCostWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cropout|UI")
	int32 Cost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cropout|UI")
	ECropoutResourceType Resource = ECropoutResourceType::None;

protected:
	virtual void NativePreConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Cropout|UI")
	TObjectPtr<UCommonTextBlock> C_Cost = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Cropout|UI")
	TObjectPtr<UImage> Image_17 = nullptr;
};

/**
 * Native parent for UIE_Resource.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API UCropoutResourceElementWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cropout|UI", meta = (DisplayName = "Resource Type"))
	ECropoutResourceType ResourceType = ECropoutResourceType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cropout|UI")
	int32 Value = 0;

	UFUNCTION(BlueprintCallable, Category = "Cropout|UI")
	void UpdateValue();

	UFUNCTION(BlueprintCallable, Category = "Cropout|UI")
	void UpdateResources_Event(int32 ResourceTypeValue, int32 NewValue);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Cropout|UI")
	TObjectPtr<UImage> Image_24 = nullptr;

private:
	void RefreshValueFromGameMode();
};

/**
 * Native parent for UIE_Slider.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API UCropoutAudioSliderWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UCropoutAudioSliderWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cropout|Audio", meta = (DisplayName = "In Sound Class"))
	TObjectPtr<USoundClass> InSoundClass = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cropout|Audio", meta = (DisplayName = "Sound Class Title"))
	FText SoundClassTitle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cropout|Audio", meta = (DisplayName = "In Sound Mix Modifier"))
	TObjectPtr<USoundMix> InSoundMixModifier = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cropout|Audio")
	int32 Index = 0;

	UFUNCTION(BlueprintCallable, Category = "Cropout|Audio")
	void UpdateSlider();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	virtual void NativePreConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Cropout|Audio")
	TObjectPtr<UCommonTextBlock> MixDescriptor = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Cropout|Audio")
	TObjectPtr<USlider> Slider_67 = nullptr;

private:
	UFUNCTION()
	void HandleSliderValueChanged(float NewValue);
};

/**
 * Native parent for CUI_BuildItem.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API UCropoutBuildItemButton : public UCommonButtonBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Cropout|Build")
	void ResourceUpdateCheck();

	UFUNCTION(BlueprintCallable, Category = "Cropout|Build")
	void UpdateResources_Event(int32 ResourceType, int32 NewValue);

	bool InitializeFromDataTableRow(const UDataTable* DataTable, FName RowName, TSubclassOf<UCommonButtonStyle> ButtonStyle);

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeOnHovered() override;
	virtual void NativeOnUnhovered() override;
	virtual void NativeOnClicked() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Cropout|Build")
	TObjectPtr<class USizeBox> BaseSize = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Cropout|Build")
	TObjectPtr<class UCommonTextBlock> Txt_Title = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Cropout|Build")
	TObjectPtr<class UImage> Img_Icon = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Cropout|Build")
	TObjectPtr<class UHorizontalBox> CostContainer = nullptr;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Cropout|Build")
	TObjectPtr<class UBorder> BG = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, meta = (BindWidgetAnimOptional), Category = "Cropout|Build")
	TObjectPtr<UWidgetAnimation> Loop_Hover = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, meta = (BindWidgetAnimOptional), Category = "Cropout|Build")
	TObjectPtr<UWidgetAnimation> Highlight_In = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, meta = (BindWidgetAnimOptional), Category = "Cropout|Build")
	TObjectPtr<UWidgetAnimation> Hightlight_Out = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"), Category = "Cropout|Build")
	TSubclassOf<class ACropoutInteractable> NativeHardClassRef;

private:
	void RefreshFromTableData();
	bool ResourceCheck();
	void ApplyResourceCheck();
	UClass* LoadTargetClassFromTableData() const;
	void SetHardClassRef(UClass* TargetClass);
	void CallBeginBuild(APawn* ControlledPawn, UClass* TargetClass);
};
