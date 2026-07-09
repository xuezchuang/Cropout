// Copyright (c) Cropout contributors. Licensed under the project license.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TimerManager.h"
#include "CropoutResourceInterface.h"
#include "CropoutGameInstanceInterface.h"
#include "CropoutGameMode.generated.h"

class UUserWidget;
class APawn;
struct FCropoutSaveVillager;

/** Dispatched after BP_GM's `Resources` map is mutated. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnCropoutUpdateResources,
	int32, ResourceType,
	int32, NewValue);

/** Dispatched after `VillagerCount` changes. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnCropoutUpdateVillagers,
	int32, NewVillagerCount);

/**
 * ACropoutGameMode
 *
 * C++ parent class for BP_GM.
 *
 * Round 5 scope (full C++ surface, int32-keyed for BP enum bridge):
 *  - 8 UPROPERTYs mirror BP_GM's 8 BP-side variables. Resource-type
 *    keys use `int32` (not the native `ECropoutResourceType` UENUM)
 *    because the BP graph's map nodes pass the BP-side
 *    `E_ResourceType` UserDefinedEnum and the two enums are not
 *    auto-bridgeable at BP graph pin connections. The underlying
 *    integer value semantics align (BP enums are uint8 / int32-backed).
 *  - 2 dynamic multicast dispatchers mirror BP_GM's dispatchers.
 *  - 8 UFUNCTION event handlers mirror BP_GM's EventGraph entry points.
 *  - 4 ICropoutResourceInterface `_Implementation` overrides.
 *  - BPI_IslandPlugin `IslandSeed` UFUNCTION stub.
 *
 *  The class also implements ICropoutGameInstanceInterface transitively
 *  via cross-class casts; UCropoutGameInstance implements the actual
 *  save/load methods.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API ACropoutGameMode
	: public AGameModeBase
	, public ICropoutResourceInterface
{
	GENERATED_BODY()

public:
	ACropoutGameMode();

protected:
	virtual void BeginPlay() override;

public:

	// -----------------------------------------------------------------
	// UPROPERTYs (mirror BP_GM's 7 BP-side variables that are
	// type-compatible with C++ UFUNCTION signatures, plus the
	// round-20 lift of `Resources`).
	//
	// Round 20: `Resources` (Map<ECropoutResourceType, int32>) is
	// now a C++ UPROPERTY so BP-side Map / ForEachLoop nodes can
	// auto-relink to the inherited variable after the BP-local
	// `Resources` variable was deleted in R20.A. The reflective
	// `FScriptMapHelper` reads in `CropoutGameMode.cpp` still
	// resolve to the same backing store (the property is now
	// reflected directly off the C++ class).
	// -----------------------------------------------------------------

	/** BP parity: `Villager Count` (int). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cropout|GameMode")
	int32 VillagerCount = 0;

	/** Native HUD widget cache. BP_GM keeps its concrete `UI_HUD` widget variable until the UI layer is migrated. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cropout|GameMode")
	TObjectPtr<UUserWidget> NativeUI_HUD = nullptr;

	/** BP parity: `Start Time` (game-time seconds at EventBeginPlay). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cropout|GameMode")
	float StartTime = 0.0f;

	/** BP parity: `SpawnRef` (runtime BP_Spawner actor ref). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cropout|GameMode")
	TObjectPtr<AActor> SpawnRef = nullptr;

	/** BP parity: `Town Hall` (runtime-spawned town hall actor). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cropout|GameMode")
	TObjectPtr<AActor> TownHall = nullptr;

	/** BP parity: `TownHall_Ref` (soft class ref to the town hall BP). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cropout|GameMode")
	TSoftClassPtr<AActor> TownHall_Ref;

	/** BP parity: `Villager_Ref` (soft class ref to the villager BP). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cropout|GameMode")
	TSoftClassPtr<APawn> Villager_Ref;

	/** BP parity: `Resources` (BP-side `TMap<E_ResourceType, int32>`).
	 *  Round 20.A: this property became orphaned after the BP-local
	 *  `Resources` variable was deleted via `remove_variable`. To give
	 *  BP-side Map / ForEachLoop nodes a stable inherited target on
	 *  BP compile, the property is canonicalised in C++ with
	 *  `int32` keys (BP UserDefinedEnum and `int32` are interchangeable
	 *  at BP graph pin level — the BP enum is `uint8`-backed, the
	 *  integer values align with `ECropoutResourceType`).
	 *  The round-12 `ECropoutResourceType` interface methods cast to /
	 *  from `int32` at the boundary in
	 *  `CropoutGameMode.cpp::RemoveResource_Implementation` etc. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cropout|GameMode")
	TMap<int32, int32> Resources;

	// -----------------------------------------------------------------
	// Dynamic multicast dispatchers (mirror BP_GM's 2 dispatchers).
	// -----------------------------------------------------------------

	UPROPERTY(BlueprintAssignable, Category = "Cropout|GameMode")
	FOnCropoutUpdateResources UpdateResources;

	UPROPERTY(BlueprintAssignable, Category = "Cropout|GameMode")
	FOnCropoutUpdateVillagers UpdateVillagers;

	// -----------------------------------------------------------------
	// Event handlers (mirror BP_GM's 8 EventGraph entry points).
	// -----------------------------------------------------------------

	/** BP parity: `EventBeginPlay`. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameMode|Event")
	void HandleEventBeginPlay();

	/** BP parity: `EventAddResource(Resource, Value)`. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameMode|Event")
	void EventAddResource(int32 Resource, int32 Value);

	/** BP parity: `EventAddResource(Resource, Value)`. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameMode|Event")
	void HandleEventAddResource(int32 Resource, int32 Value);

	/** BP parity: `BPI_Resource.Add Resource(Resource, Value)`. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameMode|Resource")
	void AddResource(int32 Resource, int32 Value);

	void AddResourceTyped(ECropoutResourceType ResourceType, int32 Value);

	/** BP parity: `Get Spawn Ref`. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameMode|Spawner")
	void GetSpawnRef();

	/** BP parity: `EventIslandGenComplete` (BPI_IslandPlugin interface event). */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameMode|Event")
	void HandleEventIslandGenComplete();

	/** BP parity: `EventIslandGenComplete` (BPI_IslandPlugin interface event). */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameMode|Event")
	void EventIslandGenComplete();

	/** BP parity: `Custom|EndGame(Win)`. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameMode|Event")
	void HandleEndGame(bool Win);

	/** BP parity: `Custom|EndGame(Win)`. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameMode|Event")
	void EndGame(bool Win);

	/** BP parity: `EventRemoveCurrentUILayer`. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameMode|Event")
	void HandleEventRemoveCurrentUILayer();

	/** BP parity: `EventRemoveCurrentUILayer`. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameMode|Event")
	void EventRemoveCurrentUILayer();

	/** BP parity: `EventAddUI(NewParam)`. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameMode|Event")
	void HandleEventAddUI(TSubclassOf<UUserWidget> NewParam);

	/** BP parity: `EventAddUI(NewParam)`. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameMode|Event")
	void EventAddUI(TSubclassOf<UUserWidget> NewParam);

	/** BP parity: `Custom|SpawnVillagers(Add)`. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameMode|Event")
	void HandleSpawnVillagers(int32 Add);

	/** BP parity: `Custom|SpawnVillagers(Add)`. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameMode|Event")
	void SpawnVillagers(int32 Add);

	/** BP parity: `Spawn Villager`. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameMode|Villagers")
	APawn* SpawnVillager();

	/** BP parity: `Load Villagers`. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameMode|Villagers")
	void LoadVillagers();

	/** BP parity: `Spawn Loaded Interactables`. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameMode|Interactables")
	void SpawnLoadedInteractables();

	/** BP parity: `Lose Check`. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameMode")
	void LoseCheck();

	/** BP parity: `Custom|BeginASyncSpawning`. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameMode|Event")
	void HandleBeginASyncSpawning();

	/** BP parity: `Custom|BeginASyncSpawning`. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameMode|Event")
	void BeginASyncSpawning();

	// -----------------------------------------------------------------
	// BPI_IslandPlugin implementation.
	// The interface itself lives in the IslandGenerator plugin; this
	// C++ stub matches the BP_GM implementation signature.
	// -----------------------------------------------------------------

	/** BP parity: BPI_IslandPlugin.IslandSeed() -> RandomStream. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameMode|Island")
	FRandomStream IslandSeed();

	/** Native implementation target for the BP-side BPI_IslandPlugin bridge. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameMode|Island")
	FRandomStream GetIslandSeedStream() const;

	// -----------------------------------------------------------------
	// ICropoutResourceInterface (4 BlueprintNativeEvent implementations).
	// Round 20: parameter and return types are `ECropoutResourceType`
	// (round-12 enum), not the round-3 int32 placeholder.
	// -----------------------------------------------------------------

	virtual void RemoveResource_Implementation(ECropoutResourceType ResourceType, int32 Amount) override;
	virtual TArray<ECropoutResourceType> GetCurrentResources_Implementation() const override;
	virtual void RemoveTargetResource_Implementation(AActor* Target, int32 Amount) override;
	virtual bool CheckResource_Implementation(ECropoutResourceType ResourceType, int32& OutAvailableCount) const override;

private:
	void PersistResources();
	void PersistVillagers();
	APawn* SpawnSavedVillager(const FCropoutSaveVillager& SavedVillager);
	void CallSpawnerFunction(FName FunctionName);
	void CallHudEndGame(bool Win);
	void SetInteractableRequireBuild(AActor* Actor, bool bRequireBuild) const;
	void SetInteractableProgressionState(AActor* Actor, int32 ProgressionState) const;

	FTimerHandle EndGameTimerHandle;
	bool bEndGameRequested = false;
};
