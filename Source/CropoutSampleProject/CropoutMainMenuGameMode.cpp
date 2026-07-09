// Copyright (c) Cropout contributors. Licensed under the project license.

#include "CropoutMainMenuGameMode.h"

#include "AudioModulationStatics.h"
#include "CropoutGameInstance.h"
#include "Blueprint/UserWidget.h"
#include "Engine/LocalPlayer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/OnlineExternalUIInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "OnlineSubsystem.h"
#include "Sound/SoundBase.h"
#include "SoundControlBus.h"

namespace
{
	void CallNoParamFunction(UObject* Target, const TCHAR* FunctionName)
	{
		if (!Target)
		{
			return;
		}

		if (UFunction* Function = Target->GetClass()->FindFunctionByName(FunctionName))
		{
			Target->ProcessEvent(Function, nullptr);
		}
	}

	void SetControlBusValue(const UObject* WorldContextObject, const TCHAR* BusPath, float Value, float FadeTime)
	{
		if (USoundControlBus* Bus = LoadObject<USoundControlBus>(nullptr, BusPath))
		{
			UAudioModulationStatics::SetGlobalBusMixValue(WorldContextObject, Bus, Value, FadeTime);
		}
	}

	void ShowExternalLoginUI(UObject* WorldContextObject, APlayerController* PlayerController)
	{
		if (!WorldContextObject || !PlayerController)
		{
			return;
		}

		IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
		if (!OnlineSubsystem)
		{
			return;
		}

		IOnlineExternalUIPtr ExternalUI = OnlineSubsystem->GetExternalUIInterface();
		ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
		if (!ExternalUI.IsValid() || !LocalPlayer)
		{
			return;
		}

		const TWeakObjectPtr<UObject> WeakWorldContext(WorldContextObject);
		const TWeakObjectPtr<APlayerController> WeakPlayerController(PlayerController);
		ExternalUI->ShowLoginUI(
			LocalPlayer->GetControllerId(),
			false,
			false,
			FOnLoginUIClosedDelegate::CreateLambda(
				[WeakWorldContext, WeakPlayerController](FUniqueNetIdPtr UniqueId, const int, const FOnlineError&)
				{
					UObject* Context = WeakWorldContext.Get();
					APlayerController* Controller = WeakPlayerController.Get();
					if (!Context || !Controller || !UniqueId.IsValid() || !UniqueId->IsValid())
					{
						return;
					}

					UKismetSystemLibrary::PrintString(Context, TEXT("Show login success"));
					UKismetSystemLibrary::PrintString(Context, UKismetSystemLibrary::GetDisplayName(Controller));
				}));
	}
}

void ACropoutMainMenuGameMode::BeginPlay()
{
	Super::BeginPlay();

	static const TSoftClassPtr<UUserWidget> MenuLayerClass(
		FSoftObjectPath(TEXT("/Game/UI/MainMenu/UI_Layer_Menu.UI_Layer_Menu_C")));
	if (UClass* LoadedClass = MenuLayerClass.LoadSynchronous())
	{
		if (UUserWidget* MenuLayer = CreateWidget<UUserWidget>(GetWorld(), LoadedClass))
		{
			MenuLayer->AddToViewport();
			CallNoParamFunction(MenuLayer, TEXT("ActivateWidget"));
		}
	}

	UCropoutGameInstance* CropoutGI = Cast<UCropoutGameInstance>(GetGameInstance());
	if (CropoutGI)
	{
		CropoutGI->TransitionOut();
	}

	static const TSoftObjectPtr<UTextureRenderTarget2D> GrassMoveRenderTarget(
		FSoftObjectPath(TEXT("/Game/Blueprint/Core/Extras/RT_GrassMove.RT_GrassMove")));
	if (UTextureRenderTarget2D* RenderTarget = GrassMoveRenderTarget.LoadSynchronous())
	{
		UKismetRenderingLibrary::ClearRenderTarget2D(this, RenderTarget, FLinearColor::Transparent);
	}

	ShowExternalLoginUI(this, UGameplayStatics::GetPlayerController(this, 0));

	SetControlBusValue(this, TEXT("/Game/Audio/DATA/ControlBus/Cropout_Music_Piano_Vol.Cropout_Music_Piano_Vol"), 0.0f, 1.0f);
	SetControlBusValue(this, TEXT("/Game/Audio/DATA/ControlBus/Cropout_Music_Perc_Vol.Cropout_Music_Perc_Vol"), 0.0f, 1.0f);
	SetControlBusValue(this, TEXT("/Game/Audio/DATA/ControlBus/Cropout_Music_Strings_Delay.Cropout_Music_Strings_Delay"), 0.5f, 1.0f);
	SetControlBusValue(this, TEXT("/Game/Audio/DATA/ControlBus/Cropout_Music_WinLose.Cropout_Music_WinLose"), 0.5f, 10.0f);

	if (CropoutGI)
	{
		USoundBase* MainMenuMusic = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/MUSIC/MUS_Main_MSS.MUS_Main_MSS"));
		CropoutGI->PlayMusic(MainMenuMusic, 1.0f, true);
	}
}
