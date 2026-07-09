// Copyright (c) Cropout contributors. Licensed under the project license.

#include "CropoutBuildingBase.h"

#include "Components/ActorComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CropoutGameInstanceInterface.h"
#include "CropoutGameMode.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TimerManager.h"

ACropoutBuildingBase::ACropoutBuildingBase()
{
	RequireBuild = true;
	OutlineDraw = 1.0;
}

void ACropoutBuildingBase::SpawnInBuildMode_Implementation(float Progression)
{
	const TArray<UStaticMesh*> CurrentMeshList = GetStaticMeshArrayPropertyValue({ TEXT("MeshList"), TEXT("Mesh List") });
	const float CurrentProgressionState = GetFloatPropertyValue({ TEXT("ProgressionState"), TEXT("Progression State") }, 0.0f);
	const int32 MeshIndex = FMath::TruncToInt(CurrentProgressionState * static_cast<float>(CurrentMeshList.Num()));
	UStaticMesh* SelectedMesh = CurrentMeshList.IsValidIndex(MeshIndex) ? CurrentMeshList[MeshIndex] : nullptr;

	SetFloatPropertyValue({ TEXT("ProgressionState"), TEXT("Progression State") }, Progression);
	Tags.Add(TEXT("Build"));

	if (SelectedMesh)
	{
		if (UStaticMeshComponent* MeshComponent = GetMeshComponent())
		{
			MeshComponent->SetStaticMesh(SelectedMesh);
		}

		UKismetSystemLibrary::PrintString(
			this,
			FString::FromInt(MeshIndex),
			true,
			true,
			FLinearColor(0.0f, 0.66f, 1.0f, 1.0f),
			2.0f);
	}
}

void ACropoutBuildingBase::ConstructionComplete_Implementation()
{
	Tags.Remove(TEXT("Build"));
	NotifyInteractablesChanged();
}

float ACropoutBuildingBase::ProgressConstruct_Implementation(float InvestedTime)
{
	const float OldProgressionState = GetFloatPropertyValue({ TEXT("ProgressionState"), TEXT("Progression State") }, 0.0f);
	const int32 OldProgressionFloor = FMath::FloorToInt(OldProgressionState);
	const int32 CurrentStageValue = GetIntPropertyValue({ TEXT("CurrentStage"), TEXT("Current Stage") }, CurrentStage);
	const TArray<UStaticMesh*> CurrentMeshList = GetStaticMeshArrayPropertyValue({ TEXT("MeshList"), TEXT("Mesh List") });
	UStaticMesh* StageMesh = CurrentMeshList.IsValidIndex(CurrentStageValue) ? CurrentMeshList[CurrentStageValue] : nullptr;
	const int32 LastIndex = LastMeshIndex();

	const float CurrentBuildDifficulty = FMath::Max(
		GetFloatPropertyValue({ TEXT("BuildDifficulty"), TEXT("Build Difficulty") }, static_cast<float>(BuildDifficulty)),
		KINDA_SMALL_NUMBER);
	const float NewProgressionState = OldProgressionState + (InvestedTime / CurrentBuildDifficulty);
	SetFloatPropertyValue({ TEXT("ProgressionState"), TEXT("Progression State") }, NewProgressionState);

	if (NewProgressionState >= static_cast<float>(LastIndex))
	{
		ConstructionComplete();
		SetMeshByIndex(LastIndex);
	}
	else if (OldProgressionFloor > CurrentStageValue)
	{
		SetIntPropertyValue({ TEXT("CurrentStage"), TEXT("Current Stage") }, OldProgressionFloor);
		if (StageMesh)
		{
			if (UStaticMeshComponent* MeshComponent = GetMeshComponent())
			{
				MeshComponent->SetStaticMesh(StageMesh);
			}
			NotifyInteractablesChanged();
		}
	}

	return static_cast<float>(LastIndex) - GetFloatPropertyValue({ TEXT("ProgressionState"), TEXT("Progression State") }, 0.0f);
}

void ACropoutBuildingBase::Interact_Implementation(float& NewParam)
{
	Super::Interact_Implementation(NewParam);
	NewParam = ProgressConstruct(0.4f);
}

void ACropoutBuildingBase::NotifyInteractablesChanged() const
{
	UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);
	if (GameInstance && GameInstance->Implements<UCropoutGameInstanceInterface>())
	{
		ICropoutGameInstanceInterface::Execute_UpdateAllInteractables(GameInstance);
	}
}

void ACropoutBuildingBase::SetMeshByIndex(int32 MeshIndex)
{
	const TArray<UStaticMesh*> CurrentMeshList = GetStaticMeshArrayPropertyValue({ TEXT("MeshList"), TEXT("Mesh List") });
	if (!CurrentMeshList.IsValidIndex(MeshIndex))
	{
		return;
	}

	if (UStaticMeshComponent* MeshComponent = GetMeshComponent())
	{
		MeshComponent->SetStaticMesh(CurrentMeshList[MeshIndex]);
	}
}

int32 ACropoutBuildingBase::LastMeshIndex() const
{
	return GetStaticMeshArrayPropertyValue({ TEXT("MeshList"), TEXT("Mesh List") }).Num() - 1;
}

void ACropoutHouse::PlacementMode_Implementation()
{
	TArray<UActorComponent*> Components;
	GetComponents(Components);
	for (UActorComponent* Component : Components)
	{
		if (Component && Component->GetFName() == TEXT("NavBlocker"))
		{
			Component->DestroyComponent();
			return;
		}
	}
}

void ACropoutHouse::ConstructionComplete_Implementation()
{
	Super::ConstructionComplete_Implementation();
	SpawnVillagers();
}

void ACropoutHouse::SpawnVillagers()
{
	if (bSpawnVillagersExecuted)
	{
		return;
	}

	bSpawnVillagersExecuted = true;

	if (ACropoutGameMode* CropoutGameMode = Cast<ACropoutGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		CropoutGameMode->SpawnVillagers(GetIntPropertyValue({ TEXT("VillagerCapacity"), TEXT("Villager Capacity") }, VillagerCapacity));
	}
}

void ACropoutTownCenter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Tags.AddUnique(TEXT("All Resources"));
}

void ACropoutTownCenter::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(MovePlayerTimerHandle, this, &ThisClass::MovePlayerToTownCenter, 1.0f, false);
	}
}

void ACropoutTownCenter::MovePlayerToTownCenter()
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	APawn* ControlledPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (ControlledPawn)
	{
		ControlledPawn->SetActorLocation(GetActorLocation());
	}
}

void ACropoutEndGameBuilding::ConstructionComplete_Implementation()
{
	Super::ConstructionComplete_Implementation();

	if (ACropoutGameMode* CropoutGameMode = Cast<ACropoutGameMode>(UGameplayStatics::GetGameMode(this)))
	{
		CropoutGameMode->EndGame(true);
	}
}
