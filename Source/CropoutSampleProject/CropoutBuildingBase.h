// Copyright (c) Cropout contributors. Licensed under the project license.

#pragma once

#include "CoreMinimal.h"
#include "CropoutInteractable.h"
#include "CropoutBuildingBase.generated.h"

/**
 * Native parent for BP_BuildingBase.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API ACropoutBuildingBase : public ACropoutInteractable
{
	GENERATED_BODY()

public:
	ACropoutBuildingBase();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cropout|Building", meta = (DisplayName = "Current Stage"))
	int32 CurrentStage = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cropout|Building", meta = (DisplayName = "Build Difficulty"))
	double BuildDifficulty = 1.0;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Cropout|Building", meta = (DisplayName = "1: Spawn In Build Mode"))
	void SpawnInBuildMode(float Progression);
	virtual void SpawnInBuildMode_Implementation(float Progression);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Cropout|Building", meta = (DisplayName = "3: Construction Complete"))
	void ConstructionComplete();
	virtual void ConstructionComplete_Implementation();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Cropout|Building", meta = (DisplayName = "Progress Construct"))
	float ProgressConstruct(float InvestedTime);
	virtual float ProgressConstruct_Implementation(float InvestedTime);

	virtual void Interact_Implementation(float& NewParam) override;

protected:
	void NotifyInteractablesChanged() const;
	void SetMeshByIndex(int32 MeshIndex);
	int32 LastMeshIndex() const;
};

/**
 * Native parent for BPC_House.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API ACropoutHouse : public ACropoutBuildingBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cropout|Building", meta = (DisplayName = "Villager Capacity"))
	int32 VillagerCapacity = 2;

	virtual void PlacementMode_Implementation() override;
	virtual void ConstructionComplete_Implementation() override;

	UFUNCTION(BlueprintCallable, Category = "Cropout|Building", meta = (DisplayName = "Spawn Villagers"))
	void SpawnVillagers();

private:
	bool bSpawnVillagersExecuted = false;
};

/**
 * Native parent for BPC_TownCenter.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API ACropoutTownCenter : public ACropoutBuildingBase
{
	GENERATED_BODY()

public:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

private:
	void MovePlayerToTownCenter();

	FTimerHandle MovePlayerTimerHandle;
};

/**
 * Native parent for BPC_EndGame.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API ACropoutEndGameBuilding : public ACropoutBuildingBase
{
	GENERATED_BODY()

public:
	virtual void ConstructionComplete_Implementation() override;
};
