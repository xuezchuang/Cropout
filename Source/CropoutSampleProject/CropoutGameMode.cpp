// Copyright (c) Cropout contributors. Licensed under the project license.

#include "CropoutGameMode.h"

#include "CropoutGameInstance.h"
#include "CropoutSaveGame.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Engine/GameInstance.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "NavigationSystem.h"

namespace
{
	TMap<ECropoutResourceType, int32> ToTypedResources(const TMap<int32, int32>& Resources)
	{
		TMap<ECropoutResourceType, int32> TypedResources;
		TypedResources.Reserve(Resources.Num());
		for (const TPair<int32, int32>& Entry : Resources)
		{
			TypedResources.Add(static_cast<ECropoutResourceType>(Entry.Key), Entry.Value);
		}
		return TypedResources;
	}

	UCropoutSaveGame* GetNativeSaveGame(UGameInstance* GameInstance)
	{
		if (UCropoutGameInstance* CropoutGI = Cast<UCropoutGameInstance>(GameInstance))
		{
			return Cast<UCropoutSaveGame>(CropoutGI->SaveRef);
		}
		return nullptr;
	}

	void CallChangeJobIfAvailable(APawn* Pawn, FName JobTag)
	{
		if (!Pawn || JobTag.IsNone())
		{
			return;
		}

		if (UFunction* ChangeJob = Pawn->GetClass()->FindFunctionByName(TEXT("ChangeJob")))
		{
			struct FChangeJobParams
			{
				FName NewJob = NAME_None;
			};
			FChangeJobParams Params;
			Params.NewJob = JobTag;
			Pawn->ProcessEvent(ChangeJob, &Params);
		}
	}

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
}

// ============================================================================
// Constructor
// ============================================================================

ACropoutGameMode::ACropoutGameMode()
{
	// Empty constructor: AGameModeBase's defaults apply.
	// BP_GM's class-default overrides (DefaultPawnClass = BP_Player_C,
	// PlayerControllerClass = BP_PC_C, etc.) remain on the BP side
	// and survive the inheritance.
}

void ACropoutGameMode::BeginPlay()
{
	Super::BeginPlay();
	HandleEventBeginPlay();
}

// ============================================================================
// 7 EventGraph handler bodies (BP DSL -> C++)
// ============================================================================

void ACropoutGameMode::HandleEventBeginPlay()
{
	// BP DSL:
	//   _gi = BPF_Cropout.GetCropoutGI
	//   BP_GI.TransitionOut(_gi)
	//   BP_GI.SetStartGameOffset(GetGameTimeInSeconds(), _gi)
	//   ClearRenderTarget2D(RT_GrassMove)
	//   CreateWidget(UI_Layer_Game)
	//   AddToViewport(_returnvalue)
	//   SetUI_HUD(_returnvalue)
	//   ActivateWidget(UI_HUD)
	StartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	if (UCropoutGameInstance* CropoutGI = Cast<UCropoutGameInstance>(GetGameInstance()))
	{
		CallNoParamFunction(CropoutGI, TEXT("TransitionOut"));
		CropoutGI->StartGameOffset = StartTime;
	}

	static const TSoftObjectPtr<UTextureRenderTarget2D> GrassMoveRenderTarget(
		FSoftObjectPath(TEXT("/Game/Blueprint/Core/Extras/RT_GrassMove.RT_GrassMove")));
	if (UTextureRenderTarget2D* RenderTarget = GrassMoveRenderTarget.LoadSynchronous())
	{
		UKismetRenderingLibrary::ClearRenderTarget2D(this, RenderTarget, FLinearColor::Transparent);
	}

	GetSpawnRef();

	// UI_Layer_Game widget creation. Round 5+ can promote
	// "/Game/UI/Game/UI_Layer_Game.UI_Layer_Game_C" to a UPROPERTY
	// soft class pointer; for now hard-code the path so the round-5
	// translation is verifiable end-to-end.
	static const TSoftClassPtr<UUserWidget> HUDWidgetClass(
		FSoftObjectPath(TEXT("/Game/UI/Game/UI_Layer_Game.UI_Layer_Game_C")));
	if (UClass* LoadedClass = HUDWidgetClass.LoadSynchronous())
	{
		if (UUserWidget* NewWidget = CreateWidget<UUserWidget>(GetWorld(), LoadedClass))
		{
			NewWidget->AddToViewport();
			NativeUI_HUD = NewWidget;
			CallNoParamFunction(NativeUI_HUD, TEXT("ActivateWidget"));
		}
	}
}

void ACropoutGameMode::EventAddResource(int32 Resource, int32 Value)
{
	HandleEventAddResource(Resource, Value);
}

void ACropoutGameMode::HandleEventAddResource(int32 Resource, int32 Value)
{
	// BP DSL:
	//   _resources = GetResources()
	//   _value     = Map_Find(_resources, Resource)
	//   Map_Add(_resources, Resource, Value + _value)
	//   Call_UpdateResources(Resource, _value)
	//   UpdateAllResources(GameInstance, GetResources())
	const int32 CurrentValue = Resources.FindRef(Resource) + Value;
	Resources.Add(Resource, CurrentValue);
	UpdateResources.Broadcast(Resource, CurrentValue);
	PersistResources();
}

void ACropoutGameMode::AddResource(int32 Resource, int32 Value)
{
	HandleEventAddResource(Resource, Value);
}

void ACropoutGameMode::AddResourceTyped(ECropoutResourceType ResourceType, int32 Value)
{
	HandleEventAddResource(static_cast<int32>(ResourceType), Value);
}

void ACropoutGameMode::GetSpawnRef()
{
	static const TSoftClassPtr<AActor> SpawnerClass(
		FSoftObjectPath(TEXT("/IslandGenerator/Spawner/BP_Spawner.BP_Spawner_C")));
	UClass* LoadedClass = SpawnerClass.LoadSynchronous();
	if (!LoadedClass)
	{
		SpawnRef = nullptr;
		return;
	}

	TArray<AActor*> Spawners;
	UGameplayStatics::GetAllActorsOfClass(this, LoadedClass, Spawners);
	SpawnRef = Spawners.Num() > 0 ? Spawners[0] : nullptr;
}

void ACropoutGameMode::HandleEventIslandGenComplete()
{
	// BP DSL:
	//   _gi = GetGameInstance()
	//   DelayUntilNextTick
	//   _save_exist = CheckSaveBool(_gi)
	//   if _save_exist:
	//     SpawnLoadedInteractables()
	//     SetResources(GetResources(_save_data))
	//     SpawnMeshOnly(SpawnRef)
	//     LoadVillagers()
	//     SetGlobalControlBusMixValue(...)
	//   else:
	//     BeginASyncSpawning()
	//     SetGlobalControlBusMixValue(...)
	//
	if (UCropoutGameInstance* CropoutGI = Cast<UCropoutGameInstance>(GetGameInstance()))
	{
		if (CropoutGI->HasSave)
		{
			SpawnLoadedInteractables();

			if (UCropoutSaveGame* SaveGame = GetNativeSaveGame(CropoutGI))
			{
				Resources.Reset();
				for (const TPair<ECropoutResourceType, int32>& Entry : SaveGame->Resources)
				{
					const int32 Key = static_cast<int32>(Entry.Key);
					Resources.Add(Key, Entry.Value);
					UpdateResources.Broadcast(Key, Entry.Value);
				}
			}
			CallSpawnerFunction(TEXT("SpawnMeshOnly"));
			LoadVillagers();
			return;
		}
	}

	HandleBeginASyncSpawning();
}

void ACropoutGameMode::EventIslandGenComplete()
{
	HandleEventIslandGenComplete();
}

void ACropoutGameMode::HandleEndGame(bool Win)
{
	// BP DSL: DoOnce { Delay 3.0; EndGame(UI_HUD, Win) }
	if (bEndGameRequested)
	{
		return;
	}

	bEndGameRequested = true;

	if (UWorld* World = GetWorld())
	{
		FTimerDelegate Delegate;
		Delegate.BindUObject(this, &ACropoutGameMode::CallHudEndGame, Win);
		World->GetTimerManager().SetTimer(EndGameTimerHandle, Delegate, 3.0f, false);
	}
}

void ACropoutGameMode::EndGame(bool Win)
{
	HandleEndGame(Win);
}

void ACropoutGameMode::HandleEventRemoveCurrentUILayer()
{
	// BP DSL: UILayerGame.PullCurrentActiveWidget(UI_HUD)
	CallNoParamFunction(NativeUI_HUD, TEXT("PullCurrentActiveWidget"));
}

void ACropoutGameMode::EventRemoveCurrentUILayer()
{
	HandleEventRemoveCurrentUILayer();
}

void ACropoutGameMode::HandleEventAddUI(TSubclassOf<UUserWidget> NewParam)
{
	// BP DSL: UILayerGame.AddStackItem(UI_HUD, NewParam)
	if (!NativeUI_HUD)
	{
		return;
	}

	if (UFunction* AddStackItem = NativeUI_HUD->GetClass()->FindFunctionByName(TEXT("AddStackItem")))
	{
		struct FAddStackItemParams
		{
			TSubclassOf<UUserWidget> ActivatableWidgetClass;
		};
		FAddStackItemParams Params;
		Params.ActivatableWidgetClass = NewParam;
		NativeUI_HUD->ProcessEvent(AddStackItem, &Params);
	}
}

void ACropoutGameMode::EventAddUI(TSubclassOf<UUserWidget> NewParam)
{
	HandleEventAddUI(NewParam);
}

void ACropoutGameMode::HandleSpawnVillagers(int32 Add)
{
	// BP DSL:
	//   for _ in range(1, Add+1):
	//     SpawnVillager()
	//   CallUpdateVillagers(GetVillagerCount())
	//   UpdateAllVillagers(GameInstance)
	int32 SpawnedCount = 0;
	for (int32 Index = 0; Index < Add; ++Index)
	{
		if (SpawnVillager())
		{
			++SpawnedCount;
		}
	}

	VillagerCount += SpawnedCount;
	UpdateVillagers.Broadcast(VillagerCount);
	PersistVillagers();
}

void ACropoutGameMode::SpawnVillagers(int32 Add)
{
	HandleSpawnVillagers(Add);
}

APawn* ACropoutGameMode::SpawnVillager()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	UClass* VillagerClass = Villager_Ref.LoadSynchronous();
	if (!VillagerClass || !VillagerClass->IsChildOf(APawn::StaticClass()))
	{
		return nullptr;
	}

	FVector SpawnLocation = FVector::ZeroVector;
	float SpawnRadius = 500.0f;
	if (TownHall)
	{
		FVector BoundsOrigin = FVector::ZeroVector;
		FVector BoundsExtent = FVector::ZeroVector;
		TownHall->GetActorBounds(false, BoundsOrigin, BoundsExtent);
		SpawnLocation = BoundsOrigin;
		SpawnRadius = FMath::Max(SpawnRadius, FMath::Min(BoundsExtent.X, BoundsExtent.Y) * 2.0f);
	}

	if (UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
	{
		FNavLocation NavLocation;
		if (NavSystem->GetRandomReachablePointInRadius(SpawnLocation, SpawnRadius, NavLocation))
		{
			SpawnLocation = NavLocation.Location;
		}
	}

	return UAIBlueprintHelperLibrary::SpawnAIFromClass(
		this,
		TSubclassOf<APawn>(VillagerClass),
		nullptr,
		SpawnLocation,
		FRotator::ZeroRotator,
		false,
		this);
}

void ACropoutGameMode::LoadVillagers()
{
	UCropoutSaveGame* SaveGame = GetNativeSaveGame(GetGameInstance());
	if (!SaveGame)
	{
		VillagerCount = 0;
		UpdateVillagers.Broadcast(VillagerCount);
		return;
	}

	int32 SpawnedCount = 0;
	for (const FCropoutSaveVillager& SavedVillager : SaveGame->Villagers)
	{
		if (!SavedVillager.bIsValid)
		{
			continue;
		}

		if (APawn* SpawnedPawn = SpawnSavedVillager(SavedVillager))
		{
			++SpawnedCount;
		}
	}

	VillagerCount = SpawnedCount;
	UpdateVillagers.Broadcast(VillagerCount);
}

void ACropoutGameMode::SpawnLoadedInteractables()
{
	UCropoutSaveGame* SaveGame = GetNativeSaveGame(GetGameInstance());
	UWorld* World = GetWorld();
	if (!SaveGame || !World)
	{
		return;
	}

	UClass* TownHallClass = TownHall_Ref.LoadSynchronous();
	for (const FCropoutSaveInteract& SavedInteractable : SaveGame->Interactables)
	{
		if (!SavedInteractable.bIsValid || !SavedInteractable.Class)
		{
			continue;
		}

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AActor* SpawnedActor = World->SpawnActor<AActor>(
			SavedInteractable.Class,
			SavedInteractable.Transform,
			Params);
		if (!SpawnedActor)
		{
			continue;
		}

		if (!SavedInteractable.Tag.IsNone() && !SpawnedActor->Tags.Contains(SavedInteractable.Tag))
		{
			SpawnedActor->Tags.Add(SavedInteractable.Tag);
		}

		SetInteractableRequireBuild(SpawnedActor, SavedInteractable.Tag == FName(TEXT("Build")));
		SetInteractableProgressionState(SpawnedActor, SavedInteractable.ProgressionState);

		if (TownHallClass && SavedInteractable.Class == TownHallClass)
		{
			TownHall = SpawnedActor;
		}
	}
}

void ACropoutGameMode::LoseCheck()
{
}

void ACropoutGameMode::HandleBeginASyncSpawning()
{
	// BP DSL:
	//   _class = AsyncLoadClassAsset(GetTownHall_Ref())
	//   on _class:
	//     _outactors = GetAllActorsOfClass(BP_SpawnMarker)
	//     _returnvalue = SpawnActor(_class, ToTransform(SteppedPosition(
	//                                GetActorLocation(RandomArrayItem(_outactors)))),
	//                                "Undefined", "MultiplyWithRoot", 0, 0)
	//     SetTownHall(_returnvalue)
	//     for _ in range(3): SpawnVillager()
	//     CallUpdateVillagers(GetVillagerCount())
	//     UpdateAllVillagers(GameInstance)
	//     SpawnRandom(GetSpawnRef())
	if (UClass* TownHallClass = TownHall_Ref.LoadSynchronous())
	{
		TArray<AActor*> SpawnMarkers;
		static const TSoftClassPtr<AActor> SpawnMarkerClass(
			FSoftObjectPath(TEXT("/IslandGenerator/Misc/BP_SpawnMarker.BP_SpawnMarker_C")));
		UClass* LoadedSpawnMarkerClass = SpawnMarkerClass.LoadSynchronous();
		if (LoadedSpawnMarkerClass)
		{
			UGameplayStatics::GetAllActorsOfClass(this, LoadedSpawnMarkerClass, SpawnMarkers);
		}
		if (SpawnMarkers.Num() > 0)
		{
			const int32 PickIdx = FMath::RandRange(0, SpawnMarkers.Num() - 1);
			const FVector Loc = SpawnMarkers[PickIdx]->GetActorLocation();
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			TownHall = GetWorld()->SpawnActor<AActor>(TownHallClass, Loc, FRotator::ZeroRotator, Params);
		}
	}
	for (int32 Index = 0; Index < 3; ++Index)
	{
		if (SpawnVillager())
		{
			++VillagerCount;
		}
	}
	UpdateVillagers.Broadcast(VillagerCount);

	PersistVillagers();
	CallSpawnerFunction(TEXT("SpawnRandom"));
}

void ACropoutGameMode::BeginASyncSpawning()
{
	HandleBeginASyncSpawning();
}

// ============================================================================
// BPI_IslandPlugin implementation
// ============================================================================

FRandomStream ACropoutGameMode::IslandSeed()
{
	return GetIslandSeedStream();
}

FRandomStream ACropoutGameMode::GetIslandSeedStream() const
{
	if (UCropoutGameInstance* CropoutGI = Cast<UCropoutGameInstance>(GetGameInstance()))
	{
		return FRandomStream(CropoutGI->TargetSeed);
	}
	return FRandomStream(0);
}

// ============================================================================
// ICropoutResourceInterface implementations
// ============================================================================

void ACropoutGameMode::RemoveResource_Implementation(ECropoutResourceType ResourceType, int32 Amount)
{
	const int32 Key = static_cast<int32>(ResourceType);
	const int32 CurrentValue = FMath::Max(0, Resources.FindRef(Key) - Amount);
	Resources.Add(Key, CurrentValue);
	UpdateResources.Broadcast(Key, CurrentValue);
	PersistResources();
}

TArray<ECropoutResourceType> ACropoutGameMode::GetCurrentResources_Implementation() const
{
	TArray<ECropoutResourceType> Result;
	Result.Reserve(Resources.Num());
	for (const TPair<int32, int32>& Entry : Resources)
	{
		if (Entry.Value > 0)
		{
			Result.Add(static_cast<ECropoutResourceType>(Entry.Key));
		}
	}
	return Result;
}

void ACropoutGameMode::RemoveTargetResource_Implementation(AActor* Target, int32 Amount)
{
	if (!Target || Amount <= 0)
	{
		return;
	}

	if (Target->Implements<UCropoutResourceInterface>())
	{
		for (ECropoutResourceType ResourceType : ICropoutResourceInterface::Execute_GetCurrentResources(Target))
		{
			RemoveResource_Implementation(ResourceType, Amount);
		}
	}
}

bool ACropoutGameMode::CheckResource_Implementation(ECropoutResourceType ResourceType, int32& OutAvailableCount) const
{
	const int32 WantKey = static_cast<int32>(ResourceType);
	OutAvailableCount = Resources.FindRef(WantKey);
	return OutAvailableCount > 0;
}

void ACropoutGameMode::PersistResources()
{
	if (UCropoutGameInstance* CropoutGI = Cast<UCropoutGameInstance>(GetGameInstance()))
	{
		if (CropoutGI->Implements<UCropoutGameInstanceInterface>())
		{
			ICropoutGameInstanceInterface::Execute_UpdateAllResources(CropoutGI, ToTypedResources(Resources));
		}
	}
}

void ACropoutGameMode::PersistVillagers()
{
	if (UCropoutGameInstance* CropoutGI = Cast<UCropoutGameInstance>(GetGameInstance()))
	{
		if (CropoutGI->Implements<UCropoutGameInstanceInterface>())
		{
			ICropoutGameInstanceInterface::Execute_UpdateAllVillagers(CropoutGI);
		}
	}
}

APawn* ACropoutGameMode::SpawnSavedVillager(const FCropoutSaveVillager& SavedVillager)
{
	UClass* VillagerClass = Villager_Ref.LoadSynchronous();
	if (!VillagerClass || !VillagerClass->IsChildOf(APawn::StaticClass()))
	{
		return nullptr;
	}

	APawn* SpawnedPawn = UAIBlueprintHelperLibrary::SpawnAIFromClass(
		this,
		TSubclassOf<APawn>(VillagerClass),
		nullptr,
		SavedVillager.Location,
		FRotator::ZeroRotator,
		false,
		this);

	if (SpawnedPawn)
	{
		if (!SavedVillager.Tag.IsNone() && !SpawnedPawn->Tags.Contains(SavedVillager.Tag))
		{
			SpawnedPawn->Tags.Add(SavedVillager.Tag);
		}
		CallChangeJobIfAvailable(SpawnedPawn, SavedVillager.Tag);
	}

	return SpawnedPawn;
}

void ACropoutGameMode::CallSpawnerFunction(FName FunctionName)
{
	if (!SpawnRef)
	{
		GetSpawnRef();
	}

	if (!SpawnRef)
	{
		return;
	}

	if (UFunction* Function = SpawnRef->GetClass()->FindFunctionByName(FunctionName))
	{
		struct FSpawnerParams
		{
			AActor* SpawnRef = nullptr;
		};
		FSpawnerParams Params;
		Params.SpawnRef = SpawnRef;
		SpawnRef->ProcessEvent(Function, &Params);
	}
}

void ACropoutGameMode::CallHudEndGame(bool Win)
{
	if (!NativeUI_HUD)
	{
		return;
	}

	if (UFunction* EndGameFunction = NativeUI_HUD->GetClass()->FindFunctionByName(TEXT("EndGame")))
	{
		struct FEndGameParams
		{
			bool Win = false;
		};
		FEndGameParams Params;
		Params.Win = Win;
		NativeUI_HUD->ProcessEvent(EndGameFunction, &Params);
	}
}

void ACropoutGameMode::SetInteractableRequireBuild(AActor* Actor, bool bRequireBuild) const
{
	if (!Actor)
	{
		return;
	}

	if (UFunction* Function = Actor->GetClass()->FindFunctionByName(TEXT("SetRequireBuild")))
	{
		struct FSetRequireBuildParams
		{
			bool bRequireBuild = false;
		};
		FSetRequireBuildParams Params;
		Params.bRequireBuild = bRequireBuild;
		Actor->ProcessEvent(Function, &Params);
	}
}

void ACropoutGameMode::SetInteractableProgressionState(AActor* Actor, int32 ProgressionState) const
{
	if (!Actor)
	{
		return;
	}

	if (UFunction* Function = Actor->GetClass()->FindFunctionByName(TEXT("SetProgressionsState")))
	{
		struct FSetProgressionStateParams
		{
			int32 ProgressionState = 0;
		};
		FSetProgressionStateParams Params;
		Params.ProgressionState = ProgressionState;
		Actor->ProcessEvent(Function, &Params);
	}
}
