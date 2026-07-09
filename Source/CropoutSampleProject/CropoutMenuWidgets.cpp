// Copyright (c) Cropout contributors. Licensed under the project license.

#include "CropoutMenuWidgets.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "CommonActionWidget.h"
#include "CommonButtonBase.h"
#include "CommonTextBlock.h"
#include "Animation/WidgetAnimation.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/Widget.h"
#include "AudioModulationStatics.h"
#include "CropoutGameInstance.h"
#include "CropoutGameMode.h"
#include "CropoutInteractable.h"
#include "CropoutPlayerController.h"
#include "CropoutPlayerPawn.h"
#include "CropoutResourceInterface.h"
#include "CropoutResourceType.h"
#include "Engine/DataTable.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/OnlinePurchaseInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "OnlineError.h"
#include "GameFramework/OnlineReplStructs.h"
#include "OnlineSubsystem.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"
#include "SoundControlBus.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UnrealType.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

#include <initializer_list>

namespace
{
	UWidget* FindNamedWidget(const UCommonActivatableWidget* Widget, const TCHAR* Name)
	{
		return Widget ? Widget->GetWidgetFromName(FName(Name)) : nullptr;
	}

	bool GetBlueprintBoolProperty(const UObject* Object, std::initializer_list<const TCHAR*> CandidateNames)
	{
		if (!Object)
		{
			return false;
		}

		for (const TCHAR* CandidateName : CandidateNames)
		{
			if (const FBoolProperty* BoolProperty = FindFProperty<FBoolProperty>(Object->GetClass(), FName(CandidateName)))
			{
				return BoolProperty->GetPropertyValue_InContainer(Object);
			}
		}

		return false;
	}

	void SetBlueprintBoolProperty(UObject* Object, std::initializer_list<const TCHAR*> CandidateNames, bool bValue)
	{
		if (!Object)
		{
			return;
		}

		for (const TCHAR* CandidateName : CandidateNames)
		{
			if (FBoolProperty* BoolProperty = FindFProperty<FBoolProperty>(Object->GetClass(), FName(CandidateName)))
			{
				BoolProperty->SetPropertyValue_InContainer(Object, bValue);
				return;
			}
		}
	}

	bool GetBlueprintTextProperty(const UObject* Object, std::initializer_list<const TCHAR*> CandidateNames, FText& OutValue)
	{
		if (!Object)
		{
			return false;
		}

		for (const TCHAR* CandidateName : CandidateNames)
		{
			if (const FTextProperty* TextProperty = FindFProperty<FTextProperty>(Object->GetClass(), FName(CandidateName)))
			{
				OutValue = TextProperty->GetPropertyValue_InContainer(Object);
				return true;
			}
		}

		return false;
	}

	bool GetBlueprintUint8Property(const UObject* Object, FName PropertyName, uint8& OutValue)
	{
		if (!Object)
		{
			return false;
		}

		FProperty* Property = FindFProperty<FProperty>(Object->GetClass(), PropertyName);
		if (FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
		{
			OutValue = ByteProperty->GetPropertyValue_InContainer(Object);
			return true;
		}

		if (FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
		{
			if (FNumericProperty* UnderlyingProperty = EnumProperty->GetUnderlyingProperty())
			{
				const void* ValuePtr = EnumProperty->ContainerPtrToValuePtr<void>(Object);
				OutValue = static_cast<uint8>(UnderlyingProperty->GetUnsignedIntPropertyValue(ValuePtr));
				return true;
			}
		}

		return false;
	}

	UObject* GetBlueprintObjectPropertyValue(const UObject* Object, FName PropertyName)
	{
		if (!Object)
		{
			return nullptr;
		}

		const FObjectPropertyBase* ObjectProperty = FindFProperty<FObjectPropertyBase>(Object->GetClass(), PropertyName);
		return ObjectProperty ? ObjectProperty->GetObjectPropertyValue_InContainer(Object) : nullptr;
	}

	void SetBlueprintObjectPropertyValue(UObject* Object, FName PropertyName, UObject* Value)
	{
		if (!Object)
		{
			return;
		}

		if (FObjectPropertyBase* ObjectProperty = FindFProperty<FObjectPropertyBase>(Object->GetClass(), PropertyName))
		{
			ObjectProperty->SetObjectPropertyValue_InContainer(Object, Value);
		}
	}

	void CallNoParamFunction(UObject* Target, FName FunctionName)
	{
		if (!Target)
		{
			return;
		}

		if (UFunction* Function = Target->FindFunction(FunctionName))
		{
			Target->ProcessEvent(Function, nullptr);
		}
	}

	struct FBuildCostEntry
	{
		int32 ResourceKey = 0;
		int32 Cost = 0;
	};

	struct FTableDataView
	{
		const FStructProperty* Property = nullptr;
		const void* Value = nullptr;
	};

	FTableDataView GetTableDataView(const UObject* Object)
	{
		FTableDataView Result;
		if (!Object)
		{
			return Result;
		}

		Result.Property = FindFProperty<FStructProperty>(Object->GetClass(), TEXT("TableData"));
		Result.Value = Result.Property ? Result.Property->ContainerPtrToValuePtr<void>(Object) : nullptr;
		return Result;
	}

	FProperty* FindStructFieldByPrefix(const FStructProperty* StructProperty, const TCHAR* Prefix)
	{
		if (!StructProperty || !StructProperty->Struct)
		{
			return nullptr;
		}

		const FString PrefixString(Prefix);
		for (TFieldIterator<FProperty> It(StructProperty->Struct); It; ++It)
		{
			FProperty* Property = *It;
			const FString PropertyName = Property->GetName();
			if (PropertyName == PrefixString || PropertyName.StartsWith(PrefixString + TEXT("_")))
			{
				return Property;
			}
		}

		return nullptr;
	}

	int32 ReadIntegerLikeProperty(const FProperty* Property, const void* ValuePtr)
	{
		if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
		{
			if (const FNumericProperty* UnderlyingProperty = EnumProperty->GetUnderlyingProperty())
			{
				return static_cast<int32>(UnderlyingProperty->GetUnsignedIntPropertyValue(ValuePtr));
			}
		}

		if (const FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
		{
			return static_cast<int32>(ByteProperty->GetPropertyValue(ValuePtr));
		}

		if (const FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property))
		{
			return static_cast<int32>(NumericProperty->GetSignedIntPropertyValue(ValuePtr));
		}

		return 0;
	}

	void WriteIntegerLikeProperty(const FProperty* Property, void* ValuePtr, int32 Value)
	{
		if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
		{
			if (FNumericProperty* UnderlyingProperty = EnumProperty->GetUnderlyingProperty())
			{
				UnderlyingProperty->SetIntPropertyValue(ValuePtr, static_cast<int64>(Value));
			}
			return;
		}

		if (const FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property))
		{
			NumericProperty->SetIntPropertyValue(ValuePtr, static_cast<int64>(Value));
		}
	}

	bool ReadTableText(const FTableDataView& TableData, const TCHAR* FieldPrefix, FText& OutValue)
	{
		FProperty* Field = FindStructFieldByPrefix(TableData.Property, FieldPrefix);
		const FTextProperty* TextProperty = CastField<FTextProperty>(Field);
		if (!TextProperty || !TableData.Value)
		{
			return false;
		}

		OutValue = TextProperty->GetPropertyValue(TextProperty->ContainerPtrToValuePtr<void>(TableData.Value));
		return true;
	}

	bool ReadTableColor(const FTableDataView& TableData, const TCHAR* FieldPrefix, FLinearColor& OutValue)
	{
		const FStructProperty* ColorProperty = CastField<FStructProperty>(FindStructFieldByPrefix(TableData.Property, FieldPrefix));
		if (!ColorProperty || !TableData.Value)
		{
			return false;
		}

		const void* ColorPtr = ColorProperty->ContainerPtrToValuePtr<void>(TableData.Value);
		if (ColorProperty->Struct == TBaseStructure<FColor>::Get())
		{
			OutValue = FLinearColor(*static_cast<const FColor*>(ColorPtr));
			return true;
		}
		if (ColorProperty->Struct == TBaseStructure<FLinearColor>::Get())
		{
			OutValue = *static_cast<const FLinearColor*>(ColorPtr);
			return true;
		}

		return false;
	}

	UObject* LoadTableSoftObject(const FTableDataView& TableData, const TCHAR* FieldPrefix)
	{
		const FProperty* Field = FindStructFieldByPrefix(TableData.Property, FieldPrefix);
		if (!Field || !TableData.Value)
		{
			return nullptr;
		}

		const void* FieldPtr = Field->ContainerPtrToValuePtr<void>(TableData.Value);
		if (const FSoftObjectProperty* SoftObjectProperty = CastField<FSoftObjectProperty>(Field))
		{
			return SoftObjectProperty->LoadObjectPropertyValue(FieldPtr);
		}
		if (const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Field))
		{
			return ObjectProperty->GetObjectPropertyValue(FieldPtr);
		}

		return nullptr;
	}

	UTexture2D* LoadResourceIcon(ECropoutResourceType ResourceType)
	{
		switch (ResourceType)
		{
		case ECropoutResourceType::Wood:
		{
			static const TSoftObjectPtr<UTexture2D> Texture(
				FSoftObjectPath(TEXT("/Game/UI/Materials/Textures/T_Rsrc_Wood_DA.T_Rsrc_Wood_DA")));
			return Texture.LoadSynchronous();
		}
		case ECropoutResourceType::Stone:
		{
			static const TSoftObjectPtr<UTexture2D> Texture(
				FSoftObjectPath(TEXT("/Game/UI/Materials/Textures/T_Rsrc_Stone_DA.T_Rsrc_Stone_DA")));
			return Texture.LoadSynchronous();
		}
		case ECropoutResourceType::Food:
		{
			static const TSoftObjectPtr<UTexture2D> Texture(
				FSoftObjectPath(TEXT("/Game/UI/Materials/Textures/T_Rsrc_Food_DA.T_Rsrc_Food_DA")));
			return Texture.LoadSynchronous();
		}
		case ECropoutResourceType::None:
		default:
			return nullptr;
		}
	}

	const FMapProperty* GetCostMapProperty(const FTableDataView& TableData)
	{
		return CastField<FMapProperty>(FindStructFieldByPrefix(TableData.Property, TEXT("Cost")));
	}

	TArray<FBuildCostEntry> ReadBuildCosts(const FTableDataView& TableData)
	{
		TArray<FBuildCostEntry> Costs;
		const FMapProperty* CostMapProperty = GetCostMapProperty(TableData);
		if (!CostMapProperty || !TableData.Value)
		{
			return Costs;
		}

		const void* CostMapPtr = CostMapProperty->ContainerPtrToValuePtr<void>(TableData.Value);
		FScriptMapHelper Helper(CostMapProperty, CostMapPtr);
		Costs.Reserve(Helper.Num());

		for (int32 Index = 0; Index < Helper.GetMaxIndex(); ++Index)
		{
			if (!Helper.IsValidIndex(Index))
			{
				continue;
			}

			FBuildCostEntry Entry;
			Entry.ResourceKey = ReadIntegerLikeProperty(CostMapProperty->KeyProp, Helper.GetKeyPtr(Index));
			Entry.Cost = ReadIntegerLikeProperty(CostMapProperty->ValueProp, Helper.GetValuePtr(Index));
			Costs.Add(Entry);
		}

		return Costs;
	}

	void SetNamedBoolProperty(UObject* Object, FName PropertyName, bool bValue)
	{
		if (Object)
		{
			if (FBoolProperty* BoolProperty = FindFProperty<FBoolProperty>(Object->GetClass(), PropertyName))
			{
				BoolProperty->SetPropertyValue_InContainer(Object, bValue);
			}
		}
	}

	void SetNamedClassProperty(UObject* Object, FName PropertyName, UClass* Value)
	{
		if (Object)
		{
			if (FClassProperty* ClassProperty = FindFProperty<FClassProperty>(Object->GetClass(), PropertyName))
			{
				ClassProperty->SetPropertyValue_InContainer(Object, Value);
			}
		}
	}

	void SetCostWidgetProperties(UUserWidget* CostWidget, const FBuildCostEntry& Entry)
	{
		if (!CostWidget)
		{
			return;
		}

		if (FProperty* ResourceProperty = FindFProperty<FProperty>(CostWidget->GetClass(), TEXT("Resource")))
		{
			WriteIntegerLikeProperty(ResourceProperty, ResourceProperty->ContainerPtrToValuePtr<void>(CostWidget), Entry.ResourceKey);
		}
		if (FProperty* CostProperty = FindFProperty<FProperty>(CostWidget->GetClass(), TEXT("Cost")))
		{
			WriteIntegerLikeProperty(CostProperty, CostProperty->ContainerPtrToValuePtr<void>(CostWidget), Entry.Cost);
		}
	}

	void CopyCostMapToFunctionParams(const FTableDataView& TableData, UFunction* Function, void* Params)
	{
		const FMapProperty* SourceCostMapProperty = GetCostMapProperty(TableData);
		if (!SourceCostMapProperty || !TableData.Value || !Function || !Params)
		{
			return;
		}

		const TArray<FBuildCostEntry> Costs = ReadBuildCosts(TableData);
		for (TFieldIterator<FProperty> It(Function); It; ++It)
		{
			FProperty* ParamProperty = *It;
			if (!ParamProperty->HasAnyPropertyFlags(CPF_Parm))
			{
				continue;
			}

			if (FMapProperty* DestMapProperty = CastField<FMapProperty>(ParamProperty))
			{
				FScriptMapHelper DestHelper(DestMapProperty, DestMapProperty->ContainerPtrToValuePtr<void>(Params));
				DestHelper.EmptyValues();
				for (const FBuildCostEntry& Entry : Costs)
				{
					const int32 Index = DestHelper.AddDefaultValue_Invalid_NeedsRehash();
					WriteIntegerLikeProperty(DestMapProperty->KeyProp, DestHelper.GetKeyPtr(Index), Entry.ResourceKey);
					WriteIntegerLikeProperty(DestMapProperty->ValueProp, DestHelper.GetValuePtr(Index), Entry.Cost);
				}
				DestHelper.Rehash();
				return;
			}
		}
	}

	void SetTargetClassParam(UFunction* Function, void* Params, UClass* TargetClass)
	{
		if (!Function || !Params)
		{
			return;
		}

		for (TFieldIterator<FProperty> It(Function); It; ++It)
		{
			FProperty* ParamProperty = *It;
			if (!ParamProperty->HasAnyPropertyFlags(CPF_Parm))
			{
				continue;
			}

			if (FClassProperty* ClassProperty = CastField<FClassProperty>(ParamProperty))
			{
				ClassProperty->SetPropertyValue_InContainer(Params, TargetClass);
				return;
			}
		}
	}

	bool SetFirstBoolLikeFunctionParam(UFunction* Function, void* Params, bool bValue)
	{
		if (!Function || !Params)
		{
			return false;
		}

		for (TFieldIterator<FProperty> It(Function); It; ++It)
		{
			FProperty* ParamProperty = *It;
			if (!ParamProperty->HasAnyPropertyFlags(CPF_Parm) || ParamProperty->HasAnyPropertyFlags(CPF_ReturnParm))
			{
				continue;
			}

			void* ParamPtr = ParamProperty->ContainerPtrToValuePtr<void>(Params);
			if (FBoolProperty* BoolProperty = CastField<FBoolProperty>(ParamProperty))
			{
				BoolProperty->SetPropertyValue(ParamPtr, bValue);
				return true;
			}
			if (FEnumProperty* EnumProperty = CastField<FEnumProperty>(ParamProperty))
			{
				if (FNumericProperty* UnderlyingProperty = EnumProperty->GetUnderlyingProperty())
				{
					UnderlyingProperty->SetIntPropertyValue(ParamPtr, static_cast<int64>(bValue ? 1 : 0));
					return true;
				}
			}
			if (FNumericProperty* NumericProperty = CastField<FNumericProperty>(ParamProperty))
			{
				NumericProperty->SetIntPropertyValue(ParamPtr, static_cast<int64>(bValue ? 1 : 0));
				return true;
			}
		}

		return false;
	}

	bool CopyDataTableRowToStructProperty(UObject* Target, FName PropertyName, const UDataTable* DataTable, FName RowName)
	{
		if (!Target || !DataTable)
		{
			return false;
		}

		FStructProperty* StructProperty = FindFProperty<FStructProperty>(Target->GetClass(), PropertyName);
		if (!StructProperty || StructProperty->Struct != DataTable->GetRowStruct())
		{
			return false;
		}

		const uint8* RowData = DataTable->FindRowUnchecked(RowName);
		if (!RowData)
		{
			return false;
		}

		StructProperty->CopyCompleteValue(StructProperty->ContainerPtrToValuePtr<void>(Target), RowData);
		return true;
	}
}

UWidget* UCropoutMainMenuWidget::NativeGetDesiredFocusTarget() const
{
	const bool bHasSave = GetBlueprintBoolProperty(this, { TEXT("Hassave"), TEXT("Has save"), TEXT("HasSave"), TEXT("bHasSave") });
	return FindNamedWidget(this, bHasSave ? TEXT("BTN_Continue") : TEXT("BTN_NewGame"));
}

void UCropoutMainMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (BTN_Continue)
	{
		BTN_Continue->OnClicked().AddUObject(this, &ThisClass::HandleContinueClicked);
	}
	if (BTN_NewGame)
	{
		BTN_NewGame->OnClicked().AddUObject(this, &ThisClass::HandleNewGameClicked);
	}
	if (BTN_Quit)
	{
		BTN_Quit->OnClicked().AddUObject(this, &ThisClass::HandleQuitClicked);
	}
	if (BTN_Donate)
	{
		BTN_Donate->OnClicked().AddUObject(this, &ThisClass::HandleDonateClicked);
	}
}

void UCropoutMainMenuWidget::NativeDestruct()
{
	if (BTN_Continue)
	{
		BTN_Continue->OnClicked().RemoveAll(this);
	}
	if (BTN_NewGame)
	{
		BTN_NewGame->OnClicked().RemoveAll(this);
	}
	if (BTN_Quit)
	{
		BTN_Quit->OnClicked().RemoveAll(this);
	}
	if (BTN_Donate)
	{
		BTN_Donate->OnClicked().RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UCropoutMainMenuWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	UpdateSaveButtonState();
	UpdateDonateVisibility();

	if (UWidget* DesiredFocusTarget = GetDesiredFocusTarget())
	{
		DesiredFocusTarget->SetFocus();
	}
}

void UCropoutMainMenuWidget::OpenLevel()
{
}

void UCropoutMainMenuWidget::ConfirmNewGame()
{
	if (UCropoutGameInstance* CropoutGI = GetGameInstance<UCropoutGameInstance>())
	{
		CropoutGI->HandleEventClearSave(true);
	}
	OpenVillage();
}

void UCropoutMainMenuWidget::ConfirmQuit()
{
	UKismetSystemLibrary::QuitGame(this, UGameplayStatics::GetPlayerController(this, 0), EQuitPreference::Quit, false);
}

void UCropoutMainMenuWidget::CallDonate()
{
	ConfirmDonate();
}

void UCropoutMainMenuWidget::ConfirmDonate()
{
	if (GetBlueprintBoolProperty(this, { TEXT("bDisableIAP"), TEXT("DisableIAP") }))
	{
		return;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	if (!PlayerController || !LocalPlayer || !PlayerController->PlayerState)
	{
		PushDonationPrompt(NSLOCTEXT("[66A3B11349A449FCD81992919E791CF8]", "3F6DF20C4C2A65230A9461B777070659", "Unfortunately payment could not be processed"));
		return;
	}

	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::IsLoaded() ? IOnlineSubsystem::Get() : nullptr;
	IOnlinePurchasePtr PurchaseInterface = OnlineSub ? OnlineSub->GetPurchaseInterface() : nullptr;
	if (!PurchaseInterface.IsValid())
	{
		PushDonationPrompt(NSLOCTEXT("[66A3B11349A449FCD81992919E791CF8]", "3F6DF20C4C2A65230A9461B777070659", "Unfortunately payment could not be processed"));
		return;
	}

	FUniqueNetIdRepl PurchasingPlayer = LocalPlayer->GetUniqueNetIdFromCachedControllerId();
	if (!PurchasingPlayer.IsValid())
	{
		PushDonationPrompt(NSLOCTEXT("[66A3B11349A449FCD81992919E791CF8]", "3F6DF20C4C2A65230A9461B777070659", "Unfortunately payment could not be processed"));
		return;
	}

	FPurchaseCheckoutRequest CheckoutRequest;
	CheckoutRequest.AddPurchaseOffer(TEXT(""), TEXT("demogame_donate"), 1, false);

	FUniqueNetIdPtr PurchasingPlayerPtr = PurchasingPlayer.GetUniqueNetId();
	PurchaseInterface->Checkout(
		*PurchasingPlayerPtr,
		CheckoutRequest,
		FOnPurchaseCheckoutComplete::CreateWeakLambda(
			this,
			[this, PurchaseInterface, PurchasingPlayerPtr](const FOnlineError& Result, const TSharedRef<FPurchaseReceipt>& Receipt)
			{
				FString TransactionIdentifier;
				FString ValidationInfo;
				if (Result.WasSuccessful())
				{
					TransactionIdentifier = Receipt->TransactionId;
					if (!Receipt->ReceiptOffers.IsEmpty() && !Receipt->ReceiptOffers[0].LineItems.IsEmpty())
					{
						ValidationInfo = Receipt->ReceiptOffers[0].LineItems[0].ValidationInfo;
					}
				}

				if (Result.WasSuccessful() && PurchaseInterface.IsValid() && PurchasingPlayerPtr.IsValid())
				{
					PurchaseInterface->FinalizePurchase(*PurchasingPlayerPtr, TransactionIdentifier, ValidationInfo);
				}

				HandleDonationCheckoutComplete(Result.WasSuccessful(), TransactionIdentifier, ValidationInfo);
			}));
}

void UCropoutMainMenuWidget::HandleContinueClicked()
{
	OpenVillage();
}

void UCropoutMainMenuWidget::HandleNewGameClicked()
{
	ConfirmNewGame();
}

void UCropoutMainMenuWidget::HandleQuitClicked()
{
	ConfirmQuit();
}

void UCropoutMainMenuWidget::HandleDonateClicked()
{
	ConfirmDonate();
}

void UCropoutMainMenuWidget::OpenVillage()
{
	if (UCropoutGameInstance* CropoutGI = GetGameInstance<UCropoutGameInstance>())
	{
		CropoutGI->HandleOpenLevel(TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/Village.Village"))));
	}
}

void UCropoutMainMenuWidget::UpdateSaveButtonState()
{
	bool bHasSave = false;
	if (UCropoutGameInstance* CropoutGI = GetGameInstance<UCropoutGameInstance>())
	{
		bHasSave = CropoutGI->CheckSaveBool();
	}

	SetBlueprintBoolProperty(this, { TEXT("Hassave"), TEXT("Has save"), TEXT("HasSave"), TEXT("bHasSave") }, bHasSave);
	if (BTN_Continue)
	{
		BTN_Continue->SetIsEnabled(bHasSave);
	}
}

void UCropoutMainMenuWidget::UpdateDonateVisibility()
{
	if (!BTN_Donate)
	{
		return;
	}

	const FString PlatformName = UGameplayStatics::GetPlatformName();
	const bool bDisableIAP = GetBlueprintBoolProperty(this, { TEXT("bDisableIAP"), TEXT("DisableIAP") });
	if (PlatformName.Equals(TEXT("Android"), ESearchCase::CaseSensitive))
	{
		if (!bDisableIAP)
		{
			BTN_Donate->SetVisibility(ESlateVisibility::Visible);
		}
	}
	else if (!PlatformName.Equals(TEXT("IOS"), ESearchCase::CaseSensitive))
	{
		BTN_Donate->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UCropoutMainMenuWidget::PushDonationPrompt(const FText& PromptQuestion)
{
	UCommonActivatableWidgetStack* StackRef = Cast<UCommonActivatableWidgetStack>(GetBlueprintObjectPropertyValue(this, TEXT("StackRef")));
	if (!StackRef)
	{
		return;
	}

	static const TSoftClassPtr<UCommonActivatableWidget> PromptWidgetClass(
		FSoftObjectPath(TEXT("/Game/UI/UI_Elements/UI_Prompt.UI_Prompt_C")));
	UClass* LoadedPromptClass = PromptWidgetClass.LoadSynchronous();
	if (!LoadedPromptClass)
	{
		return;
	}

	if (UCropoutPromptWidget* PromptWidget = Cast<UCropoutPromptWidget>(StackRef->AddWidget(TSubclassOf<UCommonActivatableWidget>(LoadedPromptClass))))
	{
		PromptWidget->SetPromptQuestion(PromptQuestion);
	}
}

void UCropoutMainMenuWidget::HandleDonationCheckoutComplete(bool bWasSuccessful, const FString& /*TransactionIdentifier*/, const FString& /*ValidationInfo*/)
{
	PushDonationPrompt(bWasSuccessful
		? NSLOCTEXT("[66A3B11349A449FCD81992919E791CF8]", "8070496548F7E013D3539CB4A26AA2E1", "Thank you for your contribution! We have added a gold star to your main menu.")
		: NSLOCTEXT("[66A3B11349A449FCD81992919E791CF8]", "3F6DF20C4C2A65230A9461B777070659", "Unfortunately payment could not be processed"));
}

void UCropoutMainMenuLayerWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	static const TSoftClassPtr<UUserWidget> MainMenuWidgetClass(
		FSoftObjectPath(TEXT("/Game/UI/MainMenu/UI_MainMenu.UI_MainMenu_C")));

	if (UCommonActivatableWidget* MainMenuWidget = PushActivatableWidgetClass(MainMenuWidgetClass.LoadSynchronous()))
	{
		AssignStackRef(MainMenuWidget);
	}
}

void UCropoutMainMenuLayerWidget::AddStackItem(TSubclassOf<UUserWidget> ActivatableWidgetClass)
{
	PushActivatableWidgetClass(ActivatableWidgetClass);
}

UCommonActivatableWidget* UCropoutMainMenuLayerWidget::PushActivatableWidgetClass(TSubclassOf<UUserWidget> ActivatableWidgetClass)
{
	if (!MainStack || !ActivatableWidgetClass || !ActivatableWidgetClass->IsChildOf(UCommonActivatableWidget::StaticClass()))
	{
		return nullptr;
	}

	return MainStack->AddWidget(TSubclassOf<UCommonActivatableWidget>(ActivatableWidgetClass.Get()));
}

void UCropoutMainMenuLayerWidget::AssignStackRef(UCommonActivatableWidget* Widget) const
{
	SetBlueprintObjectPropertyValue(Widget, TEXT("StackRef"), MainStack);
}

UWidget* UCropoutBuildWidget::NativeGetDesiredFocusTarget() const
{
	return FindNamedWidget(this, TEXT("BTN_Back"));
}

void UCropoutBuildWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (BTN_Back)
	{
		BTN_Back->OnClicked().AddUObject(this, &ThisClass::HandleBackClicked);
	}
}

void UCropoutBuildWidget::NativeDestruct()
{
	if (BTN_Back)
	{
		BTN_Back->OnClicked().RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UCropoutBuildWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	APawn* ControlledPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	UWidget* DesiredFocusTarget = GetDesiredFocusTarget();
	if (PlayerController)
	{
		FInputModeUIOnly InputMode;
		if (DesiredFocusTarget)
		{
			InputMode.SetWidgetToFocus(DesiredFocusTarget->TakeWidget());
		}
		PlayerController->SetInputMode(InputMode);
	}
	if (DesiredFocusTarget)
	{
		DesiredFocusTarget->SetFocus();
	}

	if (ControlledPawn)
	{
		ControlledPawn->SetActorTickEnabled(true);
		ControlledPawn->DisableInput(PlayerController);
		CallSwitchBuildMode(ControlledPawn, true);
	}
}

void UCropoutBuildWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	RebuildBuildItems();
}

void UCropoutBuildWidget::HandleBackClicked()
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	APawn* ControlledPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (ControlledPawn)
	{
		ControlledPawn->EnableInput(PlayerController);
		CallSwitchBuildMode(ControlledPawn, false);
	}

	DeactivateWidget();
}

void UCropoutBuildWidget::RebuildBuildItems()
{
	if (!Container)
	{
		return;
	}

	Container->ClearChildren();

	static const TSoftObjectPtr<UDataTable> BuildablesTable(
		FSoftObjectPath(TEXT("/Game/Blueprint/Interactable/Extras/DT_Buidables.DT_Buidables")));
	UDataTable* DataTable = BuildablesTable.LoadSynchronous();
	if (!DataTable)
	{
		return;
	}

	static const TSoftClassPtr<UCropoutBuildItemButton> BuildItemClass(
		FSoftObjectPath(TEXT("/Game/UI/Common/CUI_BuildItem.CUI_BuildItem_C")));
	UClass* LoadedBuildItemClass = BuildItemClass.LoadSynchronous();
	if (!LoadedBuildItemClass)
	{
		return;
	}

	static const TSoftClassPtr<UCommonButtonStyle> BuildStyleClass(
		FSoftObjectPath(TEXT("/Game/UI/Common/CUI_Style_Build.CUI_Style_Build_C")));
	TSubclassOf<UCommonButtonStyle> LoadedBuildStyleClass = BuildStyleClass.LoadSynchronous();

	for (const FName RowName : DataTable->GetRowNames())
	{
		UCropoutBuildItemButton* BuildItem = CreateWidget<UCropoutBuildItemButton>(this, LoadedBuildItemClass);
		if (!BuildItem)
		{
			continue;
		}

		BuildItem->InitializeFromDataTableRow(DataTable, RowName, LoadedBuildStyleClass);
		if (UHorizontalBoxSlot* HorizontalSlot = Container->AddChildToHorizontalBox(BuildItem))
		{
			HorizontalSlot->SetVerticalAlignment(VAlign_Bottom);
			HorizontalSlot->SetPadding(FMargin(8.0f, 0.0f, 8.0f, 5.0f));
		}
	}
}

void UCropoutBuildWidget::CallSwitchBuildMode(APawn* ControlledPawn, bool bEnabled)
{
	if (!ControlledPawn)
	{
		return;
	}

	UFunction* SwitchBuildModeFunction = ControlledPawn->FindFunction(TEXT("SwitchBuildMode"));
	if (!SwitchBuildModeFunction)
	{
		SwitchBuildModeFunction = ControlledPawn->FindFunction(TEXT("Switch Build Mode"));
	}
	if (!SwitchBuildModeFunction)
	{
		return;
	}

	uint8* Params = static_cast<uint8*>(FMemory_Alloca(SwitchBuildModeFunction->ParmsSize));
	FMemory::Memzero(Params, SwitchBuildModeFunction->ParmsSize);
	SwitchBuildModeFunction->InitializeStruct(Params);

	SetFirstBoolLikeFunctionParam(SwitchBuildModeFunction, Params, bEnabled);

	ControlledPawn->ProcessEvent(SwitchBuildModeFunction, Params);
	SwitchBuildModeFunction->DestroyStruct(Params);
}

UWidget* UCropoutEndGameWidget::NativeGetDesiredFocusTarget() const
{
	return FindNamedWidget(this, TEXT("BTN_Continue"));
}

void UCropoutEndGameWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (BTN_Continue)
	{
		BTN_Continue->OnClicked().AddUObject(this, &ThisClass::HandleContinueClicked);
	}
	if (BTN_MainMenu)
	{
		BTN_MainMenu->OnClicked().AddUObject(this, &ThisClass::HandleMainMenuClicked);
	}
	if (BTN_Retry)
	{
		BTN_Retry->OnClicked().AddUObject(this, &ThisClass::HandleRetryClicked);
	}
}

void UCropoutEndGameWidget::NativeDestruct()
{
	if (BTN_Continue)
	{
		BTN_Continue->OnClicked().RemoveAll(this);
	}
	if (BTN_MainMenu)
	{
		BTN_MainMenu->OnClicked().RemoveAll(this);
	}
	if (BTN_Retry)
	{
		BTN_Retry->OnClicked().RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UCropoutEndGameWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	UWidget* DesiredFocusTarget = GetDesiredFocusTarget();
	if (PlayerController)
	{
		FInputModeUIOnly InputMode;
		if (DesiredFocusTarget)
		{
			InputMode.SetWidgetToFocus(DesiredFocusTarget->TakeWidget());
		}
		PlayerController->SetInputMode(InputMode);

		if (APawn* ControlledPawn = PlayerController->GetPawn())
		{
			ControlledPawn->DisableInput(PlayerController);
		}
	}
	if (DesiredFocusTarget)
	{
		DesiredFocusTarget->SetFocus();
	}

	UGameplayStatics::SetGamePaused(this, true);
}

void UCropoutEndGameWidget::EndGame(bool bWin)
{
	SetBlueprintBoolProperty(this, { TEXT("WIN"), TEXT("Win"), TEXT("bWin") }, bWin);

	FText ResultText;
	if (GetBlueprintTextProperty(this, { bWin ? TEXT("WinText") : TEXT("LoseText") }, ResultText) && MainText)
	{
		MainText->SetText(ResultText);
	}

	static const TCHAR* WinLoseBusPath = TEXT("/Game/Audio/DATA/ControlBus/Cropout_Music_WinLose.Cropout_Music_WinLose");
	if (USoundControlBus* WinLoseBus = LoadObject<USoundControlBus>(nullptr, WinLoseBusPath))
	{
		UAudioModulationStatics::SetGlobalBusMixValue(this, WinLoseBus, bWin ? 1.0f : 0.0f, 0.0f);
	}

	if (UCropoutGameInstance* CropoutGI = GetGameInstance<UCropoutGameInstance>())
	{
		CropoutGI->StopMusic();
	}
}

void UCropoutEndGameWidget::HandleContinueClicked()
{
	UGameplayStatics::SetGamePaused(this, false);
	DeactivateWidget();
}

void UCropoutEndGameWidget::HandleMainMenuClicked()
{
	OpenLevelWithGameInstance(TEXT("/Game/MainMenu.MainMenu"));
	UGameplayStatics::SetGamePaused(this, false);
}

void UCropoutEndGameWidget::HandleRetryClicked()
{
	if (UCropoutGameInstance* CropoutGI = GetGameInstance<UCropoutGameInstance>())
	{
		CropoutGI->HandleEventClearSave(false);
	}
	OpenLevelWithGameInstance(TEXT("/Game/Village.Village"));
	UGameplayStatics::SetGamePaused(this, false);
}

void UCropoutEndGameWidget::OpenLevelWithGameInstance(const TCHAR* LevelPath)
{
	if (UCropoutGameInstance* CropoutGI = GetGameInstance<UCropoutGameInstance>())
	{
		CropoutGI->HandleOpenLevel(TSoftObjectPtr<UWorld>(FSoftObjectPath(LevelPath)));
	}
}

UWidget* UCropoutPauseWidget::NativeGetDesiredFocusTarget() const
{
	return FindNamedWidget(this, TEXT("BTN_Resume"));
}

void UCropoutPauseWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (BTN_Resume)
	{
		BTN_Resume->OnClicked().AddUObject(this, &ThisClass::HandleResumeClicked);
	}
	if (BTN_Restart)
	{
		BTN_Restart->OnClicked().AddUObject(this, &ThisClass::HandleRestartClicked);
	}
	if (BTN_MainMenu)
	{
		BTN_MainMenu->OnClicked().AddUObject(this, &ThisClass::HandleMainMenuClicked);
	}
}

void UCropoutPauseWidget::NativeDestruct()
{
	if (BTN_Resume)
	{
		BTN_Resume->OnClicked().RemoveAll(this);
	}
	if (BTN_Restart)
	{
		BTN_Restart->OnClicked().RemoveAll(this);
	}
	if (BTN_MainMenu)
	{
		BTN_MainMenu->OnClicked().RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UCropoutPauseWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	UWidget* DesiredFocusTarget = GetDesiredFocusTarget();
	if (PlayerController)
	{
		FInputModeUIOnly InputMode;
		if (DesiredFocusTarget)
		{
			InputMode.SetWidgetToFocus(DesiredFocusTarget->TakeWidget());
		}
		PlayerController->SetInputMode(InputMode);
	}
	if (DesiredFocusTarget)
	{
		DesiredFocusTarget->SetFocus();
	}

	UGameplayStatics::SetGamePaused(this, true);
	UpdateMusicSlider();
}

void UCropoutPauseWidget::HandleResumeClicked()
{
	UGameplayStatics::SetGamePaused(this, false);
	UWidgetBlueprintLibrary::SetFocusToGameViewport();
	DeactivateWidget();
}

void UCropoutPauseWidget::HandleRestartClicked()
{
	if (UCropoutGameInstance* CropoutGI = GetGameInstance<UCropoutGameInstance>())
	{
		CropoutGI->HandleEventClearSave(false);
	}
	UGameplayStatics::SetGamePaused(this, false);
	OpenLevelWithGameInstance(TEXT("/Game/Village.Village"));
}

void UCropoutPauseWidget::HandleMainMenuClicked()
{
	UGameplayStatics::SetGamePaused(this, false);
	OpenLevelWithGameInstance(TEXT("/Game/MainMenu.MainMenu"));
}

void UCropoutPauseWidget::OpenLevelWithGameInstance(const TCHAR* LevelPath)
{
	if (UCropoutGameInstance* CropoutGI = GetGameInstance<UCropoutGameInstance>())
	{
		CropoutGI->HandleOpenLevel(TSoftObjectPtr<UWorld>(FSoftObjectPath(LevelPath)));
	}
}

void UCropoutPauseWidget::UpdateMusicSlider()
{
	CallNoParamFunction(Slider_Music, TEXT("UpdateSlider"));
}

FText UCropoutGameMainWidget::Get_VillagerCount_Text() const
{
	return FText::GetEmpty();
}

void UCropoutGameMainWidget::UI_GameMain_AutoGenFunc(uint8 NewInput)
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController)
	{
		return;
	}

	PlayerController->bShowMouseCursor = false;
	switch (NewInput)
	{
	case 0:
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		PlayerController->SetInputMode(InputMode);
		UWidgetBlueprintLibrary::SetFocusToGameViewport();
		break;
	}
	case 1:
		PlayerController->bShowMouseCursor = true;
		break;
	case 2:
	{
		FInputModeGameOnly InputMode;
		InputMode.SetConsumeCaptureMouseDown(true);
		PlayerController->SetInputMode(InputMode);
		break;
	}
	default:
		break;
	}
}

void UCropoutGameMainWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (CUI_Button_55)
	{
		CUI_Button_55->OnClicked().AddUObject(this, &ThisClass::HandleBuildClicked);
	}
}

void UCropoutGameMainWidget::NativeDestruct()
{
	if (CUI_Button_55)
	{
		CUI_Button_55->OnClicked().RemoveAll(this);
	}
	UnbindPlayerControllerEvents();

	Super::NativeDestruct();
}

void UCropoutGameMainWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	uint8 InputType = 0;
	if (ReadPlayerInputType(InputType))
	{
		UI_GameMain_AutoGenFunc(InputType);
	}

	BindPlayerControllerEvents();

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController)
	{
		return;
	}

	if (APawn* ControlledPawn = PlayerController->GetPawn())
	{
		ControlledPawn->EnableInput(PlayerController);
		ControlledPawn->SetActorTickEnabled(true);
	}

	UWidgetBlueprintLibrary::SetFocusToGameViewport();
}

void UCropoutGameMainWidget::HandleBuildClicked()
{
	if (!BuildWidgetClass)
	{
		return;
	}

	if (ACropoutGameMode* CropoutGM = Cast<ACropoutGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		CropoutGM->EventAddUI(BuildWidgetClass);
	}
}

void UCropoutGameMainWidget::BindPlayerControllerEvents()
{
	ACropoutPlayerController* PlayerController = Cast<ACropoutPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	if (!PlayerController)
	{
		return;
	}

	PlayerController->KeySwitch.AddUniqueDynamic(this, &ThisClass::UI_GameMain_AutoGenFunc);
	KeySwitchSource = PlayerController;
}

void UCropoutGameMainWidget::UnbindPlayerControllerEvents()
{
	if (ACropoutPlayerController* Source = KeySwitchSource.Get())
	{
		Source->KeySwitch.RemoveDynamic(this, &ThisClass::UI_GameMain_AutoGenFunc);
	}
	KeySwitchSource.Reset();
}

bool UCropoutGameMainWidget::ReadPlayerInputType(uint8& OutInputType) const
{
	return GetBlueprintUint8Property(UGameplayStatics::GetPlayerController(this, 0), TEXT("InputType"), OutInputType);
}

void UCropoutPromptWidget::SetPromptQuestion(const FText& InPromptQuestion)
{
	PromptQuestion = InPromptQuestion;
	if (Title)
	{
		Title->SetText(PromptQuestion);
	}
}

void UCropoutPromptWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (BTN_Pos)
	{
		BTN_Pos->OnClicked().AddUObject(this, &ThisClass::HandlePositiveClicked);
	}
	if (BTN_Neg)
	{
		BTN_Neg->OnClicked().AddUObject(this, &ThisClass::HandleNegativeClicked);
	}
}

void UCropoutPromptWidget::NativeDestruct()
{
	if (BTN_Pos)
	{
		BTN_Pos->OnClicked().RemoveAll(this);
	}
	if (BTN_Neg)
	{
		BTN_Neg->OnClicked().RemoveAll(this);
	}

	ClearPromptDelegates();

	Super::NativeDestruct();
}

void UCropoutPromptWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	if (UWidget* DesiredFocusTarget = GetDesiredFocusTarget())
	{
		DesiredFocusTarget->SetFocus();
	}
	if (Title)
	{
		Title->SetText(PromptQuestion);
	}
}

UWidget* UCropoutPromptWidget::NativeGetDesiredFocusTarget() const
{
	return FindNamedWidget(this, TEXT("BTN_Neg"));
}

void UCropoutPromptWidget::HandlePositiveClicked()
{
	Confirm.Broadcast();
	ClearPromptDelegates();
	DeactivateWidget();
}

void UCropoutPromptWidget::HandleNegativeClicked()
{
	Back.Broadcast();
	ClearPromptDelegates();
	DeactivateWidget();
}

void UCropoutPromptWidget::ClearPromptDelegates()
{
	Confirm.Clear();
	Back.Clear();
}

void UCropoutBuildConfirmWidget::UI_GameMain_AutoGenFunc(uint8 NewInput)
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController)
	{
		return;
	}

	PlayerController->bShowMouseCursor = false;
	switch (NewInput)
	{
	case 0:
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		PlayerController->SetInputMode(InputMode);
		UWidgetBlueprintLibrary::SetFocusToGameViewport();
		break;
	}
	case 1:
		PlayerController->bShowMouseCursor = true;
		break;
	case 2:
	{
		FInputModeGameOnly InputMode;
		InputMode.SetConsumeCaptureMouseDown(true);
		PlayerController->SetInputMode(InputMode);
		break;
	}
	default:
		break;
	}
}

void UCropoutBuildConfirmWidget::Confirm()
{
}

void UCropoutBuildConfirmWidget::Back()
{
}

void UCropoutBuildConfirmWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (BTN_Pos)
	{
		BTN_Pos->OnClicked().AddUObject(this, &ThisClass::HandleBuildClicked);
	}
	if (BTN_Pos_1)
	{
		BTN_Pos_1->OnClicked().AddUObject(this, &ThisClass::HandleRotateClicked);
	}
	if (BTN_Neg)
	{
		BTN_Neg->OnClicked().AddUObject(this, &ThisClass::HandleCancelClicked);
	}
}

void UCropoutBuildConfirmWidget::NativeDestruct()
{
	if (BTN_Pos)
	{
		BTN_Pos->OnClicked().RemoveAll(this);
	}
	if (BTN_Pos_1)
	{
		BTN_Pos_1->OnClicked().RemoveAll(this);
	}
	if (BTN_Neg)
	{
		BTN_Neg->OnClicked().RemoveAll(this);
	}
	UnbindPlayerControllerEvents();

	Super::NativeDestruct();
}

void UCropoutBuildConfirmWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	uint8 InputType = 0;
	if (ReadPlayerInputType(InputType))
	{
		UI_GameMain_AutoGenFunc(InputType);
	}

	BindPlayerControllerEvents();

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController)
	{
		return;
	}

	if (APawn* ControlledPawn = PlayerController->GetPawn())
	{
		ControlledPawn->EnableInput(PlayerController);
		ControlledPawn->SetActorTickEnabled(true);
	}

	UWidgetBlueprintLibrary::SetFocusToGameViewport();
	SetBorderTranslation(CalculateBorderPosition());
}

void UCropoutBuildConfirmWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!CommonBorder_1 || !GetBuildSpawn())
	{
		return;
	}

	const FWidgetTransform& CurrentTransform = CommonBorder_1->GetRenderTransform();
	const FVector Current(CurrentTransform.Translation.X, CurrentTransform.Translation.Y, 0.0);
	const FVector2D Target2D = CalculateBorderPosition();
	const FVector Target(Target2D.X, Target2D.Y, 0.0);
	const float DeltaTime = GetWorld() ? GetWorld()->GetDeltaSeconds() : InDeltaTime;
	const FVector Interpolated = UKismetMathLibrary::VectorSpringInterp(
		Current,
		Target,
		SpringState,
		50.0f,
		0.9f,
		DeltaTime,
		1.0f,
		0.75f,
		false,
		FVector(-1.0),
		FVector(1.0),
		false);

	SetBorderTranslation(FVector2D(Interpolated.X, Interpolated.Y));
}

void UCropoutBuildConfirmWidget::HandleBuildClicked()
{
	CallControlledPawnFunction(TEXT("SpawnBuildTarget"));
}

void UCropoutBuildConfirmWidget::HandleRotateClicked()
{
	CallControlledPawnFunction(TEXT("RotateSpawn"));
}

void UCropoutBuildConfirmWidget::HandleCancelClicked()
{
	CallControlledPawnFunction(TEXT("DestroySpawn"));
	DeactivateWidget();
}

void UCropoutBuildConfirmWidget::BindPlayerControllerEvents()
{
	ACropoutPlayerController* PlayerController = Cast<ACropoutPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	if (!PlayerController)
	{
		return;
	}

	PlayerController->KeySwitch.AddUniqueDynamic(this, &ThisClass::UI_GameMain_AutoGenFunc);
	KeySwitchSource = PlayerController;
}

void UCropoutBuildConfirmWidget::UnbindPlayerControllerEvents()
{
	if (ACropoutPlayerController* Source = KeySwitchSource.Get())
	{
		Source->KeySwitch.RemoveDynamic(this, &ThisClass::UI_GameMain_AutoGenFunc);
	}
	KeySwitchSource.Reset();
}

bool UCropoutBuildConfirmWidget::ReadPlayerInputType(uint8& OutInputType) const
{
	return GetBlueprintUint8Property(UGameplayStatics::GetPlayerController(this, 0), TEXT("InputType"), OutInputType);
}

UObject* UCropoutBuildConfirmWidget::GetBuildSpawn() const
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	APawn* ControlledPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	return GetBlueprintObjectPropertyValue(ControlledPawn, TEXT("Spawn"));
}

FVector2D UCropoutBuildConfirmWidget::CalculateBorderPosition() const
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	AActor* SpawnActor = Cast<AActor>(GetBuildSpawn());
	if (!PlayerController || !SpawnActor)
	{
		return FVector2D::ZeroVector;
	}

	FVector2D ScreenPosition = FVector2D::ZeroVector;
	PlayerController->ProjectWorldLocationToScreen(SpawnActor->GetActorLocation(), ScreenPosition, true);

	const float ViewportScale = FMath::Max(UWidgetLayoutLibrary::GetViewportScale(this), UE_KINDA_SMALL_NUMBER);
	const FVector2D ScaledScreenPosition = ScreenPosition / ViewportScale;
	const FVector2D ScaledViewportSize = UWidgetLayoutLibrary::GetViewportSize(this) / ViewportScale;

	return FVector2D(
		FMath::Clamp(ScaledScreenPosition.X, 150.0, ScaledViewportSize.X - 350.0),
		FMath::Clamp(ScaledScreenPosition.Y, 0.0, ScaledViewportSize.Y - 150.0));
}

void UCropoutBuildConfirmWidget::SetBorderTranslation(const FVector2D& Translation)
{
	if (!CommonBorder_1)
	{
		return;
	}

	FWidgetTransform Transform;
	Transform.Translation = Translation;
	Transform.Scale = FVector2D(1.0, 1.0);
	Transform.Shear = FVector2D::ZeroVector;
	Transform.Angle = 0.0f;
	CommonBorder_1->SetRenderTransform(Transform);
}

void UCropoutBuildConfirmWidget::CallControlledPawnFunction(FName FunctionName)
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	APawn* ControlledPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!ControlledPawn)
	{
		return;
	}

	if (UFunction* Function = ControlledPawn->FindFunction(FunctionName))
	{
		ControlledPawn->ProcessEvent(Function, nullptr);
	}
}

void UCropoutCommonButton::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (ButtonTitle)
	{
		ButtonTitle->SetText(ButtonText);
	}

	if (SizeBox_42)
	{
		const FString PlatformName = UGameplayStatics::GetPlatformName();
		const bool bUseMobileHeight = PlatformName == TEXT("Android") || PlatformName == TEXT("IOS");
		SizeBox_42->SetMinDesiredHeight(static_cast<float>(MinHeight) * (bUseMobileHeight ? 1.5f : 1.0f));
	}

	if (GamepadIcon)
	{
		GamepadIcon->SetInputAction(TriggeringInputAction);
	}
}

void UCropoutCostWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (C_Cost)
	{
		C_Cost->SetText(FText::AsNumber(Cost));
	}

	if (Image_17)
	{
		Image_17->SetBrushFromTexture(LoadResourceIcon(Resource), false);
	}
}

void UCropoutResourceElementWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (Image_24)
	{
		Image_24->SetVisibility(ESlateVisibility::Visible);
	}
}

void UCropoutResourceElementWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (Image_24)
	{
		static const TSoftObjectPtr<UTexture2D> FoodTexture(
			FSoftObjectPath(TEXT("/Game/UI/Materials/Textures/T_Rsrc_Food_DA.T_Rsrc_Food_DA")));
		Image_24->SetBrushFromTexture(FoodTexture.LoadSynchronous(), false);
	}
}

void UCropoutResourceElementWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshValueFromGameMode();
}

void UCropoutResourceElementWidget::UpdateValue()
{
	Value = Value;
}

void UCropoutResourceElementWidget::UpdateResources_Event(int32 ResourceTypeValue, int32 NewValue)
{
	(void)ResourceTypeValue;
	(void)NewValue;
	UpdateValue();
}

void UCropoutResourceElementWidget::RefreshValueFromGameMode()
{
	int32 AvailableCount = 0;
	if (const ACropoutGameMode* CropoutGM = Cast<ACropoutGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		CropoutGM->CheckResource_Implementation(ResourceType, AvailableCount);
	}

	Value = AvailableCount;
}

UCropoutAudioSliderWidget::UCropoutAudioSliderWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SoundClassTitle = NSLOCTEXT("Cropout", "DefaultSoundClassTitle", "Mix Name");

	static ConstructorHelpers::FObjectFinder<USoundMix> DefaultSoundMix(
		TEXT("/Game/Audio/DATA/SoundClass/SMX_Lead.SMX_Lead"));
	if (DefaultSoundMix.Succeeded())
	{
		InSoundMixModifier = DefaultSoundMix.Object;
	}
}

void UCropoutAudioSliderWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (Slider_67)
	{
		Slider_67->OnValueChanged.AddUniqueDynamic(this, &ThisClass::HandleSliderValueChanged);
	}
}

void UCropoutAudioSliderWidget::NativeDestruct()
{
	if (Slider_67)
	{
		Slider_67->OnValueChanged.RemoveDynamic(this, &ThisClass::HandleSliderValueChanged);
	}

	Super::NativeDestruct();
}

void UCropoutAudioSliderWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (MixDescriptor)
	{
		MixDescriptor->SetText(SoundClassTitle);
	}
}

void UCropoutAudioSliderWidget::UpdateSlider()
{
	if (!Slider_67)
	{
		return;
	}

	const UCropoutGameInstance* CropoutGI = Cast<UCropoutGameInstance>(UGameplayStatics::GetGameInstance(this));
	if (!CropoutGI || !CropoutGI->SoundMixes.IsValidIndex(Index))
	{
		return;
	}

	Slider_67->SetValue(CropoutGI->SoundMixes[Index]);
}

void UCropoutAudioSliderWidget::HandleSliderValueChanged(float NewValue)
{
	if (UCropoutGameInstance* CropoutGI = Cast<UCropoutGameInstance>(UGameplayStatics::GetGameInstance(this)))
	{
		if (Index >= 0)
		{
			CropoutGI->SoundMixes.SetNum(FMath::Max(CropoutGI->SoundMixes.Num(), Index + 1));
			CropoutGI->SoundMixes[Index] = NewValue;
		}
	}

	if (InSoundMixModifier && InSoundClass)
	{
		UGameplayStatics::SetSoundMixClassOverride(this, InSoundMixModifier, InSoundClass, NewValue, 1.0f, 1.0f, true);
	}
}

bool UCropoutBuildItemButton::InitializeFromDataTableRow(const UDataTable* DataTable, FName RowName, TSubclassOf<UCommonButtonStyle> ButtonStyle)
{
	const bool bCopiedTableData = CopyDataTableRowToStructProperty(this, TEXT("TableData"), DataTable, RowName);
	if (ButtonStyle)
	{
		SetStyle(ButtonStyle);
	}
	if (bCopiedTableData)
	{
		RefreshFromTableData();
		ApplyResourceCheck();
	}

	return bCopiedTableData;
}

void UCropoutBuildItemButton::NativePreConstruct()
{
	Super::NativePreConstruct();
	RefreshFromTableData();
}

void UCropoutBuildItemButton::NativeConstruct()
{
	Super::NativeConstruct();
	ApplyResourceCheck();
}

void UCropoutBuildItemButton::NativeOnHovered()
{
	if (BaseSize)
	{
		BaseSize->SetMinDesiredHeight(300.0f);
	}
	if (Loop_Hover)
	{
		PlayAnimation(Loop_Hover, 0.0f, 0);
	}
	if (Highlight_In)
	{
		PlayAnimation(Highlight_In);
	}

	Super::NativeOnHovered();
}

void UCropoutBuildItemButton::NativeOnUnhovered()
{
	if (BaseSize)
	{
		BaseSize->SetMinDesiredHeight(250.0f);
	}
	if (Loop_Hover)
	{
		StopAnimation(Loop_Hover);
	}
	if (Hightlight_Out)
	{
		PlayAnimation(Hightlight_Out);
	}

	Super::NativeOnUnhovered();
}

void UCropoutBuildItemButton::NativeOnClicked()
{
	UClass* TargetClass = LoadTargetClassFromTableData();
	if (!TargetClass)
	{
		TargetClass = NativeHardClassRef.Get();
	}
	if (!TargetClass)
	{
		if (FClassProperty* HardClassProperty = FindFProperty<FClassProperty>(GetClass(), TEXT("HardClassRef")))
		{
			TargetClass = Cast<UClass>(HardClassProperty->GetObjectPropertyValue_InContainer(this));
		}
	}

	if (APlayerController* OwningPlayer = GetOwningPlayer())
	{
		CallBeginBuild(OwningPlayer->GetPawn(), TargetClass);
	}

	static const TSoftClassPtr<UUserWidget> BuildConfirmClass(
		FSoftObjectPath(TEXT("/Game/UI/UI_Elements/UI_BuildConfirm.UI_BuildConfirm_C")));
	if (ACropoutGameMode* CropoutGM = Cast<ACropoutGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		CropoutGM->EventAddUI(BuildConfirmClass.LoadSynchronous());
	}

	Super::NativeOnClicked();
}

void UCropoutBuildItemButton::ResourceUpdateCheck()
{
	ApplyResourceCheck();
}

void UCropoutBuildItemButton::UpdateResources_Event(int32 ResourceType, int32 NewValue)
{
	(void)ResourceType;
	(void)NewValue;
}

void UCropoutBuildItemButton::RefreshFromTableData()
{
	const FTableDataView TableData = GetTableDataView(this);
	if (!TableData.Property || !TableData.Value)
	{
		return;
	}

	FText Title;
	if (Txt_Title && ReadTableText(TableData, TEXT("Title"), Title))
	{
		Txt_Title->SetText(Title);
	}

	if (Img_Icon)
	{
		if (UTexture2D* Texture = Cast<UTexture2D>(LoadTableSoftObject(TableData, TEXT("UIIcon"))))
		{
			Img_Icon->SetBrushFromTexture(Texture, false);
		}
	}

	FLinearColor TabColor;
	if (BG && ReadTableColor(TableData, TEXT("TabCol"), TabColor))
	{
		BG->SetBrushColor(TabColor);
	}

	UClass* TargetClass = LoadTargetClassFromTableData();
	SetHardClassRef(TargetClass);

	if (CostContainer)
	{
		CostContainer->ClearChildren();
		static const TSoftClassPtr<UUserWidget> CostWidgetClass(
			FSoftObjectPath(TEXT("/Game/UI/UI_Elements/UIE_Cost.UIE_Cost_C")));
		UClass* LoadedCostWidgetClass = CostWidgetClass.LoadSynchronous();
		if (LoadedCostWidgetClass)
		{
			for (const FBuildCostEntry& Entry : ReadBuildCosts(TableData))
			{
				UUserWidget* CostWidget = CreateWidget<UUserWidget>(GetOwningPlayer(), LoadedCostWidgetClass);
				if (!CostWidget)
				{
					continue;
				}

				SetCostWidgetProperties(CostWidget, Entry);
				if (UHorizontalBoxSlot* HorizontalSlot = CostContainer->AddChildToHorizontalBox(CostWidget))
				{
					FSlateChildSize Size;
					Size.SizeRule = ESlateSizeRule::Fill;
					Size.Value = 1.0f;
					HorizontalSlot->SetSize(Size);
				}
			}
		}
	}
}

bool UCropoutBuildItemButton::ResourceCheck()
{
	const TArray<FBuildCostEntry> Costs = ReadBuildCosts(GetTableDataView(this));
	if (Costs.IsEmpty())
	{
		SetNamedBoolProperty(this, TEXT("Enable Build"), false);
		return false;
	}

	UObject* GameMode = UGameplayStatics::GetGameMode(this);
	if (!GameMode || !GameMode->Implements<UCropoutResourceInterface>())
	{
		SetNamedBoolProperty(this, TEXT("Enable Build"), false);
		return false;
	}

	for (const FBuildCostEntry& Entry : Costs)
	{
		int32 AvailableCount = 0;
		const bool bHasResource = ICropoutResourceInterface::Execute_CheckResource(
			GameMode,
			static_cast<ECropoutResourceType>(Entry.ResourceKey),
			AvailableCount);
		if (!bHasResource || AvailableCount < Entry.Cost)
		{
			SetNamedBoolProperty(this, TEXT("Enable Build"), false);
			return false;
		}
	}

	SetNamedBoolProperty(this, TEXT("Enable Build"), true);
	return true;
}

void UCropoutBuildItemButton::ApplyResourceCheck()
{
	SetIsInteractionEnabled(ResourceCheck());
}

UClass* UCropoutBuildItemButton::LoadTargetClassFromTableData() const
{
	return Cast<UClass>(LoadTableSoftObject(GetTableDataView(this), TEXT("TargetClass")));
}

void UCropoutBuildItemButton::SetHardClassRef(UClass* TargetClass)
{
	NativeHardClassRef = TargetClass;
	SetNamedClassProperty(this, TEXT("HardClassRef"), TargetClass);
}

void UCropoutBuildItemButton::CallBeginBuild(APawn* ControlledPawn, UClass* TargetClass)
{
	if (!ControlledPawn || !TargetClass)
	{
		return;
	}

	if (ACropoutPlayerPawn* CropoutPlayerPawn = Cast<ACropoutPlayerPawn>(ControlledPawn))
	{
		TMap<ECropoutResourceType, int32> ResourceCost;
		for (const FBuildCostEntry& Entry : ReadBuildCosts(GetTableDataView(this)))
		{
			ResourceCost.Add(static_cast<ECropoutResourceType>(Entry.ResourceKey), Entry.Cost);
		}
		CropoutPlayerPawn->BeginBuildNative(TargetClass, ResourceCost);
		return;
	}

	UFunction* BeginBuildFunction = ControlledPawn->GetClass()->FindFunctionByName(TEXT("BeginBuild"));
	if (!BeginBuildFunction)
	{
		return;
	}

	uint8* Params = static_cast<uint8*>(FMemory_Alloca(BeginBuildFunction->ParmsSize));
	FMemory::Memzero(Params, BeginBuildFunction->ParmsSize);
	BeginBuildFunction->InitializeStruct(Params);

	SetTargetClassParam(BeginBuildFunction, Params, TargetClass);
	CopyCostMapToFunctionParams(GetTableDataView(this), BeginBuildFunction, Params);

	ControlledPawn->ProcessEvent(BeginBuildFunction, Params);
	BeginBuildFunction->DestroyStruct(Params);
}
