// Copyright (c) Cropout contributors. Licensed under the project license.

#include "CropoutGameLayerWidget.h"

#include "CommonButtonBase.h"
#include "CommonTextBlock.h"
#include "CropoutGameMode.h"
#include "CropoutPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Components/VerticalBox.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UnrealType.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

namespace
{
	static constexpr uint8 ResourceWidgetValues[] =
	{
		static_cast<uint8>(ECropoutResourceType::Food),
		static_cast<uint8>(ECropoutResourceType::Wood),
		static_cast<uint8>(ECropoutResourceType::Stone),
	};
}

void UCropoutGameLayerWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (resourceContainer)
	{
		resourceContainer->ClearChildren();
	}

	ResourceSequenceIndex = 0;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(AddResourceTimerHandle, this, &ThisClass::AddResource, 0.1f, true);
	}

	BindGameModeEvents();
	BindPlayerControllerEvents();

	if (BTN_Pause)
	{
		BTN_Pause->OnClicked().AddUObject(this, &ThisClass::HandlePauseClicked);
	}
}

void UCropoutGameLayerWidget::NativeDestruct()
{
	StopResourceTimer();

	if (ACropoutGameMode* CropoutGM = Cast<ACropoutGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		CropoutGM->UpdateVillagers.RemoveDynamic(this, &ThisClass::UpdateVillagerDetails);
	}

	if (BTN_Pause)
	{
		BTN_Pause->OnClicked().RemoveAll(this);
	}

	if (ACropoutPlayerController* Source = KeySwitchSource.Get())
	{
		Source->KeySwitch.RemoveDynamic(this, &ThisClass::HidePause);
	}

	Super::NativeDestruct();
}

void UCropoutGameLayerWidget::AddResource()
{
	if (!resourceContainer || ResourceSequenceIndex >= UE_ARRAY_COUNT(ResourceWidgetValues))
	{
		StopResourceTimer();
		return;
	}

	static const TSoftClassPtr<UUserWidget> ResourceWidgetClass(
		FSoftObjectPath(TEXT("/Game/UI/UI_Elements/UIE_Resource.UIE_Resource_C")));
	UClass* LoadedClass = ResourceWidgetClass.LoadSynchronous();
	if (!LoadedClass)
	{
		return;
	}

	APlayerController* OwningPlayer = GetOwningPlayer();
	if (!OwningPlayer)
	{
		OwningPlayer = UGameplayStatics::GetPlayerController(this, 0);
	}

	UUserWidget* ResourceWidget = CreateWidget<UUserWidget>(OwningPlayer, LoadedClass);
	if (!ResourceWidget)
	{
		return;
	}

	SetResourceProperty(ResourceWidget, ResourceWidgetValues[ResourceSequenceIndex]);
	++ResourceSequenceIndex;
	resourceContainer->AddChild(ResourceWidget);
}

void UCropoutGameLayerWidget::AddStackItem(TSubclassOf<UUserWidget> ActivatableWidgetClass)
{
	PushActivatableWidgetClass(ActivatableWidgetClass);
}

void UCropoutGameLayerWidget::PullCurrentActiveWidget()
{
	if (!mainStack)
	{
		return;
	}

	if (UCommonActivatableWidget* ActiveWidget = mainStack->GetActiveWidget())
	{
		mainStack->RemoveWidget(*ActiveWidget);
	}
}

void UCropoutGameLayerWidget::EndGame(bool Win)
{
	static const TSoftClassPtr<UUserWidget> EndGameWidgetClass(
		FSoftObjectPath(TEXT("/Game/UI/UI_Elements/UI_EndGame.UI_EndGame_C")));

	UCommonActivatableWidget* EndGameWidget = PushActivatableWidgetClass(EndGameWidgetClass.LoadSynchronous());
	if (!EndGameWidget)
	{
		return;
	}

	if (UFunction* EndGameFunction = EndGameWidget->GetClass()->FindFunctionByName(TEXT("EndGame")))
	{
		struct FEndGameParams
		{
			bool Win = false;
		};
		FEndGameParams Params;
		Params.Win = Win;
		EndGameWidget->ProcessEvent(EndGameFunction, &Params);
	}

	EndGameWidget->ActivateWidget();
}

void UCropoutGameLayerWidget::UpdateVillagerDetails(int32 VillagerCount)
{
	if (villagerCounter)
	{
		villagerCounter->SetText(FText::AsNumber(VillagerCount));
	}
}

void UCropoutGameLayerWidget::HidePause(uint8 NewInput)
{
	if (BTN_Pause)
	{
		BTN_Pause->SetRenderOpacity(NewInput == 2 ? 0.0f : 1.0f);
	}
}

void UCropoutGameLayerWidget::BindGameModeEvents()
{
	if (ACropoutGameMode* CropoutGM = Cast<ACropoutGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		CropoutGM->UpdateVillagers.AddUniqueDynamic(this, &ThisClass::UpdateVillagerDetails);
		UpdateVillagerDetails(CropoutGM->VillagerCount);
	}
}

void UCropoutGameLayerWidget::BindPlayerControllerEvents()
{
	ACropoutPlayerController* PlayerController = Cast<ACropoutPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	if (!PlayerController)
	{
		return;
	}

	PlayerController->KeySwitch.AddUniqueDynamic(this, &ThisClass::HidePause);
	KeySwitchSource = PlayerController;
}

void UCropoutGameLayerWidget::StopResourceTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AddResourceTimerHandle);
	}
}

void UCropoutGameLayerWidget::SetResourceProperty(UUserWidget* ResourceWidget, uint8 ResourceValue) const
{
	if (!ResourceWidget)
	{
		return;
	}

	FProperty* ResourceProperty = ResourceWidget->GetClass()->FindPropertyByName(TEXT("ResourceType"));
	if (!ResourceProperty)
	{
		ResourceProperty = ResourceWidget->GetClass()->FindPropertyByName(TEXT("Resource"));
	}
	if (FByteProperty* ByteProperty = CastField<FByteProperty>(ResourceProperty))
	{
		ByteProperty->SetPropertyValue_InContainer(ResourceWidget, ResourceValue);
	}
	else if (FEnumProperty* EnumProperty = CastField<FEnumProperty>(ResourceProperty))
	{
		EnumProperty->GetUnderlyingProperty()->SetIntPropertyValue(EnumProperty->ContainerPtrToValuePtr<void>(ResourceWidget), static_cast<int64>(ResourceValue));
	}
	else if (FIntProperty* IntProperty = CastField<FIntProperty>(ResourceProperty))
	{
		IntProperty->SetPropertyValue_InContainer(ResourceWidget, ResourceValue);
	}
}

UCommonActivatableWidget* UCropoutGameLayerWidget::PushActivatableWidgetClass(TSubclassOf<UUserWidget> WidgetClass)
{
	if (!mainStack || !WidgetClass || !WidgetClass->IsChildOf(UCommonActivatableWidget::StaticClass()))
	{
		return nullptr;
	}

	return mainStack->AddWidget(TSubclassOf<UCommonActivatableWidget>(WidgetClass.Get()));
}

void UCropoutGameLayerWidget::HandlePauseClicked()
{
	static const TSoftClassPtr<UUserWidget> PauseWidgetClass(
		FSoftObjectPath(TEXT("/Game/UI/UI_Elements/UI_Pause.UI_Pause_C")));

	PushActivatableWidgetClass(PauseWidgetClass.LoadSynchronous());
}
