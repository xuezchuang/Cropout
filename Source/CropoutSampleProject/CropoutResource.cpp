// Copyright (c) Cropout contributors. Licensed under the project license.

#include "CropoutResource.h"

#include "Components/StaticMeshComponent.h"
#include "CropoutGameInstanceInterface.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

namespace
{
	FName ResourceTypeToTag(ECropoutResourceType ResourceType)
	{
		switch (ResourceType)
		{
		case ECropoutResourceType::Wood:
			return TEXT("Wood");
		case ECropoutResourceType::Stone:
			return TEXT("Stone");
		case ECropoutResourceType::Food:
			return TEXT("Food");
		case ECropoutResourceType::None:
		default:
			return TEXT("None");
		}
	}

}

ACropoutResource::ACropoutResource()
{
	NativeResourceAmount = 100.0f;
	NativeCollectionTime = 3.0f;
	NativeCollectionValue = 10.0f;
	bNativeUseRandomMesh = false;
}

void ACropoutResource::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	Tags.AddUnique(ResourceTypeToTag(GetResourceTypeValue()));

	if (GetBoolPropertyValue({ TEXT("UseRandomMesh"), TEXT("Use Random Mesh"), TEXT("bNativeUseRandomMesh") }, bNativeUseRandomMesh))
	{
		const TArray<UStaticMesh*> CurrentMeshList = GetStaticMeshArrayPropertyValue({ TEXT("MeshList"), TEXT("Mesh List") });
		if (CurrentMeshList.Num() > 0)
		{
			if (UStaticMeshComponent* MeshComponent = GetMeshComponent())
			{
				MeshComponent->SetStaticMesh(CurrentMeshList[FMath::RandRange(0, CurrentMeshList.Num() - 1)]);
			}
		}
	}
}

void ACropoutResource::Interact_Implementation(float& NewParam)
{
	Super::Interact_Implementation(NewParam);
	NewParam = GetFloatPropertyValue({ TEXT("CollectionTime"), TEXT("Collection Time"), TEXT("NativeCollectionTime") }, NativeCollectionTime);
}

void ACropoutResource::Death()
{
	Destroy();
}

FRandomStream ACropoutResource::IslandSeed()
{
	return FRandomStream(0);
}

void ACropoutResource::CollectResource(ECropoutResourceType& TargetResource, int32& Value)
{
	EndWobble();

	TargetResource = GetResourceTypeValue();
	Value = FMath::TruncToInt(GetFloatPropertyValue({ TEXT("CollectionValue"), TEXT("Collection Value"), TEXT("NativeCollectionValue") }, NativeCollectionValue));

	const float CurrentAmount = GetFloatPropertyValue({ TEXT("ResourceAmount"), TEXT("Resource Amount"), TEXT("NativeResourceAmount") }, NativeResourceAmount);
	if (!FMath::IsNearlyEqual(CurrentAmount, -1.0f))
	{
		const float NewAmount = FMath::Max(0.0f, CurrentAmount - static_cast<float>(Value));
		SetFloatPropertyValue({ TEXT("ResourceAmount"), TEXT("Resource Amount"), TEXT("NativeResourceAmount") }, NewAmount);
		if (NewAmount <= 0.0f)
		{
			Death();
		}
	}
}

ECropoutResourceType ACropoutResource::GetResourceTypeValue() const
{
	return static_cast<ECropoutResourceType>(
		GetIntPropertyValue({ TEXT("ResourceType"), TEXT("Resource Type"), TEXT("NativeResourceType") }, static_cast<int32>(NativeResourceType)));
}

void ACropoutResource::EventScaleUP(float Delay)
{
	UStaticMeshComponent* MeshComponent = GetMeshComponent();
	if (!MeshComponent)
	{
		return;
	}

	MeshComponent->SetHiddenInGame(true);
	if (UWorld* World = GetWorld())
	{
		FTimerDelegate Delegate;
		TWeakObjectPtr<ACropoutResource> WeakThis(this);
		Delegate.BindLambda([WeakThis]()
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			if (UStaticMeshComponent* DelayedMesh = WeakThis->GetMeshComponent())
			{
				DelayedMesh->SetRelativeScale3D(FVector::OneVector);
				DelayedMesh->SetHiddenInGame(false);
			}
		});

		FTimerHandle TimerHandle;
		World->GetTimerManager().SetTimer(TimerHandle, Delegate, FMath::Max(0.0f, Delay), false);
	}
}

void ACropoutResource::RemoveResource_Implementation(ECropoutResourceType /*RT*/, int32 /*Amount*/) {}
void ACropoutResource::RemoveTargetResource_Implementation(AActor* /*Target*/, int32 /*Amount*/) {}

TArray<ECropoutResourceType> ACropoutResource::GetCurrentResources_Implementation() const
{
	return { GetResourceTypeValue() };
}

bool ACropoutResource::CheckResource_Implementation(ECropoutResourceType /*RT*/, int32& OutAvailableCount) const
{
	OutAvailableCount = 0;
	return false;
}

void ACropoutBaseCrop::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	Tags.Reset();
	Tags.Add(TEXT("Farming"));
	Tags.Add(TEXT("Ready"));
}

void ACropoutBaseCrop::SetProgressionsState_Implementation(float Progression)
{
	Super::SetProgressionsState_Implementation(Progression);
	SetReady();
}

void ACropoutBaseCrop::Interact_Implementation(float& NewParam)
{
	Super::Interact_Implementation(NewParam);
	NewParam = FarmingProgress();
}

float ACropoutBaseCrop::FarmingProgress()
{
	const float CurrentCollectionTime = GetFloatPropertyValue(
		{ TEXT("CollectionTime"), TEXT("Collection Time"), TEXT("NativeCollectionTime") },
		NativeCollectionTime);

	if (Tags.Num() > 1)
	{
		Tags.RemoveAt(1);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(SwitchStageTimerHandle, this, &ThisClass::SwitchStage, CurrentCollectionTime, false);
	}

	const float CurrentProgression = GetFloatPropertyValue({ TEXT("ProgressionState"), TEXT("Progression State") }, 0.0f);
	const int32 CurrentIndex = FMath::TruncToInt(CurrentProgression);
	const int32 LastIndex = GetStaticMeshArrayPropertyValue({ TEXT("MeshList"), TEXT("Mesh List") }).Num() - 1;
	const int32 NextIndex = CurrentIndex >= LastIndex ? 0 : CurrentIndex + 1;
	SetFloatPropertyValue({ TEXT("ProgressionState"), TEXT("Progression State") }, static_cast<float>(NextIndex));

	return CurrentCollectionTime;
}

void ACropoutBaseCrop::SetReady()
{
	const TArray<UStaticMesh*> CurrentMeshList = GetStaticMeshArrayPropertyValue({ TEXT("MeshList"), TEXT("Mesh List") });
	const int32 LastIndex = CurrentMeshList.Num() - 1;
	const int32 MeshIndex = FMath::TruncToInt(GetFloatPropertyValue({ TEXT("ProgressionState"), TEXT("Progression State") }, 0.0f));

	while (Tags.Num() < 2)
	{
		Tags.Add(NAME_None);
	}
	Tags[1] = (MeshIndex == LastIndex) ? TEXT("Harvest") : TEXT("Ready");

	if (CurrentMeshList.IsValidIndex(MeshIndex))
	{
		if (UStaticMeshComponent* MeshComponent = GetMeshComponent())
		{
			MeshComponent->SetStaticMesh(CurrentMeshList[MeshIndex]);
		}
	}

	NotifyInteractablesChanged();
	PopFarmPlot();
}

void ACropoutBaseCrop::SwitchStage()
{
	const int32 CurrentStage = FMath::FloorToInt(GetFloatPropertyValue({ TEXT("ProgressionState"), TEXT("Progression State") }, 0.0f));
	if (CurrentStage == 0)
	{
		SetReady();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		const float CurrentCooldown = GetFloatPropertyValue({ TEXT("CooldownTime"), TEXT("Cooldown Time"), TEXT("NativeCooldownTime") }, NativeCooldownTime);
		World->GetTimerManager().SetTimer(SetReadyTimerHandle, this, &ThisClass::SetReady, CurrentCooldown, false);
	}
}

void ACropoutBaseCrop::PopFarmPlot()
{
	if (UStaticMeshComponent* MeshComponent = GetMeshComponent())
	{
		const FVector CurrentScale = MeshComponent->GetRelativeScale3D();
		MeshComponent->SetRelativeScale3D(FVector(CurrentScale.X, CurrentScale.Y, 1.0f));
	}
}

void ACropoutBaseCrop::NotifyInteractablesChanged() const
{
	UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);
	if (GameInstance && GameInstance->Implements<UCropoutGameInstanceInterface>())
	{
		ICropoutGameInstanceInterface::Execute_UpdateAllInteractables(GameInstance);
	}
}
