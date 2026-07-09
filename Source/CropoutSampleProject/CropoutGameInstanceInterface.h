// Copyright (c) Cropout contributors. Licensed under the project license.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CropoutResourceType.h"
#include "CropoutGameInstanceInterface.generated.h"

class AActor;
class APawn;

/**
 * UCropoutGameInstanceInterface / ICropoutGameInstanceInterface
 *
 * Native mirror of the BP-side `BPI_GI` Blueprint Interface
 * (`Content/Blueprint/Core/Save/BPI_GI.uasset`).
 *
 * Round 3 scope (foundations, new interface):
 *  - Defines the canonical native contract for GameInstance-side
 *    save / load / level-orchestration hooks. BP_GM's `BPI_GI_C`
 *    message calls (e.g. `SetSaveData|UpdateAllResources`,
 *    `SetSaveData|UpdateAllVillagers`,
 *    `SetSaveData|UpdateAllInteractables`, `SetSaveData|SaveGame`,
 *    `SetSaveData|ClearSave`) translate to these five
 *    `UFUNCTION(BlueprintNativeEvent)` methods.
 *  - BP_GI continues to implement `BPI_GI_C` via its BP graph this
 *    round. Round 4+ will move those BP implementations into
 *    `_Implementation` overrides on `UCropoutGameInstance`, and
 *    `BPI_GI.uasset` will be deleted or kept as a BP-side shim.
 *
 * Method signatures were inferred from the BP_GI EventGraph DSL:
 *  - `EventUpdateAllResources(ResourcesMap)` — UpdateAllResources takes
 *    the resources map and the GI persists it.
 *  - `EventUpdateAllVillagers` — UpdateAllVillagers has no params; the
 *    GI scans for `Pawn` actors and persists their locations / tags.
 *  - `EventUpdateAllInteractables` — UpdateAllInteractables has no
 *    params; the GI scans for `BP_Interactable` actors and persists
 *    their transforms / classes / progression.
 *  - `EventSaveGame` — SaveGame has no params; the GI does an async
 *    `AsyncSaveGameToSlot` and sets `HasSave = true` on completion.
 *  - `EventClearSave(ClearSeed: bool)` — ClearSave's signature was
 *    confirmed via the BP-side function graph read.
 *
 *  Out of scope this round:
 *   - Adding any implementor (`UCropoutGameInstance` implements this
 *     interface in round 4 alongside the BP_GM logic lift).
 *   - Touching Build.cs, configs, or any BP asset.
 */
UINTERFACE(MinimalAPI, Blueprintable)
class UCropoutGameInstanceInterface : public UInterface
{
	GENERATED_BODY()
};

class ICropoutGameInstanceInterface
{
	GENERATED_BODY()

public:
	/**
	 * Persist the resources map to the GI's SaveRef.
	 * BP parity: `EventUpdateAllResources(ResourcesMap)`.
	 * Round 12.C: key type is `ECropoutResourceType` (aligned to BP
	 * internal values Wood=0, None=1, Stone=5, Food=6). BP callers
	 * pass `TMap<E_ResourceType, int32>` and the GI bridges the value
	 * into native enum at the call boundary (see
	 * `UCropoutGameInstance::HandleEventUpdateAllResources`).
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Cropout|GameInstance|Save")
	void UpdateAllResources(const TMap<ECropoutResourceType, int32>& Resources);

	/**
	 * Scan world for `APawn` actors (excluding the player) and persist
	 * their locations / tags to the GI's SaveRef.
	 * BP parity: `EventUpdateAllVillagers`.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Cropout|GameInstance|Save")
	void UpdateAllVillagers();

	/**
	 * Scan world for `BP_Interactable` actors and persist their
	 * transforms / classes / progression to the GI's SaveRef.
	 * BP parity: `EventUpdateAllInteractables`.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Cropout|GameInstance|Save")
	void UpdateAllInteractables();

	/**
	 * Asynchronously write the SaveRef to the "SAVE" slot and set
	 * `HasSave = true` on completion.
	 * BP parity: `EventSaveGame`.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Cropout|GameInstance|Save")
	void SaveGame();

	/**
	 * Clear the save slot. If `ClearSeed` is true, also pick a new
	 * random seed for island generation.
	 * BP parity: `EventClearSave(ClearSeed: bool)`.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Cropout|GameInstance|Save")
	void ClearSave(bool ClearSeed);
};