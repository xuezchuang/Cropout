// Copyright (c) Cropout contributors. Licensed under the project license.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CropoutResourceType.h"
#include "CropoutResourceInterface.generated.h"

/**
 * UCropoutResourceInterface / ICropoutResourceInterface
 *
 * Native mirror of the BP-side `BPI_Resource` Blueprint Interface
 * (`Content/Blueprint/Core/GameMode/BPI_Resource.uasset`).
 *
 * Round 20 scope (final shape):
 *  - 4 BlueprintNativeEvent overrides take `ECropoutResourceType`
 *    keys so callers (handlers in `UCropoutGameInstance`,
 *    `ACropoutGameMode`, `ACropoutResource`) operate on the native
 *    enum. BP-side `E_ResourceType` (UserDefinedEnum) values are
 *    auto-bridgeable to `ECropoutResourceType` because their
 *    underlying integer values align (Wood=0, None=1, Stone=5,
 *    Food=6 — see `CropoutResourceType.h`).
 *  - The BP-side `BPI_Resource` Blueprint Interface is left
 *    untouched; it is the BP-graph contract for `BPI_Resource_C`
 *    implementors and remains authoritative for BP-only callers.
 *    C++ implementors expose the same methods through this
 *    interface.
 *  - The interface does NOT share a binding with BPI_Resource —
 *    BP-graph and C++-native implementations are independent and
 *    may diverge at call sites.
 *
 * Out of scope this round:
 *  - BodyLift of any specific implementor's function graph (handled
 *    per round 18+ via `read_graph_dsl`).
 *  - Touching Build.cs, configs, or any BP asset.
 */
UINTERFACE(MinimalAPI, Blueprintable)
class UCropoutResourceInterface : public UInterface
{
	GENERATED_BODY()
};

class ICropoutResourceInterface
{
	GENERATED_BODY()

public:
	/**
	 * Remove `Amount` of `ResourceType` from the implementor's pool.
	 * BP parity: `RemoveResource(E_ResourceType, int) -> (E_ResourceType, int)`.
	 * In C++ the new count is reflected by mutating the implementor's
	 * internal map; the broadcast / save side-effect lives on the
	 * implementor.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Cropout|Resource")
	void RemoveResource(ECropoutResourceType ResourceType, int32 Amount);

	/**
	 * Return the list of resource types currently held by the
	 * implementor (typically keys of the resources map with non-zero
	 * count).
	 * BP parity: `GetCurrentResources() -> TArray<E_ResourceType>`.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Cropout|Resource")
	TArray<ECropoutResourceType> GetCurrentResources() const;

	/**
	 * Remove `Amount` of resource associated with the `Target` actor
	 * (e.g. building storage, villager inventory).
	 * BP parity: `RemoveTargetResource(AActor, int)`.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Cropout|Resource")
	void RemoveTargetResource(AActor* Target, int32 Amount);

	/**
	 * Check whether the implementor has the resource type. Out param
	 * receives the current count.
	 * BP parity: `CheckResource(E_ResourceType) -> (bool, int)`.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Cropout|Resource")
	bool CheckResource(ECropoutResourceType ResourceType, int32& OutAvailableCount) const;
};
