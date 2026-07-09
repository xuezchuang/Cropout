// Copyright (c) Cropout contributors. Licensed under the project license.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CropoutBlueprintLibrary.generated.h"

class UCropoutGameInstance;
class ACropoutGameMode;
class ACropoutVillager;
class AActor;

/**
 * UCropoutBlueprintLibrary
 *
 * C++ replacement for `BPF_Cropout.uasset`'s user-defined
 * functions:
 *
 *   - `Stepped Position` / `SteppedPosition`
 *     — snap an XY position to the 200-unit placement grid.
 *
 *   - `Get Cropout GI` (BP display name) / `GetCropoutGI` (BP internal name)
 *     — static helper that returns the current world's GameInstance cast
 *     to UCropoutGameInstance. BP-side body was: GetGameInstance →
 *     K2Node_DynamicCast (AsBP_GI).
 *
 *   - `Get Cropout GM` / `GetCropoutGM`
 *     — static helper that returns the current world's GameMode cast to
 *     ACropoutGameMode. BP-side body was: GetGameMode →
 *     K2Node_DynamicCast (AsBP_GM).
 *
 * It also carries `BPF_Shared.uasset`'s matching `Convert To Stepped Pos`
 * helper so both Blueprint function libraries have native equivalents before
 * callers are rewired.
 *
 * Round 12.A scope: 1:1 mirror the two BP-side functions as native
 * static UFUNCTIONs on a `UBlueprintFunctionLibrary`-derived
 * `UCLASS`. Existing BP callers (BP_GM, BP_GI, BP_Spawner, BP_Villager)
 * resolve to the new static UFUNCTIONs by signature once `BPF_Cropout`
 * is no longer reparented/rewired; today BP-side still calls go
 * through `BPF_Cropout_C` which continues to work.
 *
 * `WorldContextObject` is the standard `UBlueprintFunctionLibrary`
 * pattern: callers must supply a valid UObject so the library can
 * reach `UGameplayStatics::GetGameInstance/GetGameMode` without
 * needing a default world.
 */
UCLASS()
class CROPOUTSAMPLEPROJECT_API UCropoutBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** BP parity: `Stepped Position` (BPF_Cropout). Snaps XY to the 200-unit grid and returns Z=0. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cropout|Grid")
	static FVector SteppedPosition(FVector Position);

	/** BP parity: `Convert To Stepped Pos` (BPF_Shared). Same grid snap as SteppedPosition. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cropout|Grid")
	static FVector ConvertToSteppedPos(FVector Position);

	/** BP parity: `Get Cropout GI` (BPF_Cropout). Returns GameInstance cast to UCropoutGameInstance (or nullptr if not castable). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cropout|GameInstance", meta = (WorldContext = "WorldContextObject"))
	static UCropoutGameInstance* GetCropoutGI(const UObject* WorldContextObject);

	/** BP parity: `Get Cropout GM` (BPF_Cropout). Returns GameMode cast to ACropoutGameMode (or nullptr if not castable). */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cropout|GameMode", meta = (WorldContext = "WorldContextObject"))
	static ACropoutGameMode* GetCropoutGM(const UObject* WorldContextObject);

	/** Native replacement for IslandGenerator's BPI_IslandPlugin.IslandSeed message call. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cropout|GameInstance", meta = (WorldContext = "WorldContextObject"))
	static FRandomStream GetCropoutIslandSeedStream(const UObject* WorldContextObject);

	/** Native replacement for IslandGenerator's BPI_IslandPlugin.IslandGenComplete message call. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameMode|Event", meta = (WorldContext = "WorldContextObject"))
	static void NotifyIslandGenComplete(const UObject* WorldContextObject);

	/** Native replacement for BPI_Player.RemoveCurrentUILayer calls targeting BP_GM. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameMode|Event", meta = (WorldContext = "WorldContextObject"))
	static void NotifyRemoveCurrentUILayer(const UObject* WorldContextObject);

	/** Native replacement for BPI_GI.UpdateAllInteractables message calls targeting the active GameInstance. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameInstance|Save", meta = (WorldContext = "WorldContextObject"))
	static void NotifyUpdateAllInteractables(const UObject* WorldContextObject);

	/** BP-compatible replacement for BPI_Resource.AddResource calls targeting BP_GM. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameMode|Resource", meta = (WorldContext = "WorldContextObject"))
	static void AddGameModeResource(const UObject* WorldContextObject, uint8 Resource, int32 Value);

	/** BP-compatible replacement for BPI_Resource.RemoveTargetResource calls targeting BP_GM. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameMode|Resource", meta = (WorldContext = "WorldContextObject"))
	static void RemoveGameModeResource(const UObject* WorldContextObject, uint8 Resource, int32 Amount);

	/** BP-compatible replacement for BPI_Resource.CheckResource calls targeting BP_GM. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameMode|Resource", meta = (WorldContext = "WorldContextObject"))
	static bool CheckGameModeResource(const UObject* WorldContextObject, uint8 Resource, int32& OutAvailableCount);

	/** BP_Player.RemoveResources parity: debit ResourceCost from GM and report whether another build is still affordable. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameMode|Resource", meta = (WorldContext = "WorldContextObject"))
	static bool ApplyGameModeBuildResourceCost(const UObject* WorldContextObject, const UObject* ResourceCostOwner);

	/** Native replacement for BPI_Resource.RemoveResource calls targeting resource actors. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|Resource")
	static void CollectActorResource(AActor* Target, uint8& Resource, int32& Value);

	/** Native replacement for BPI_Resource.AddResource calls targeting actor inventories. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|Resource")
	static void AddActorResource(AActor* Target, uint8 Resource, int32 Value);

	/** Native replacement for BPI_Villager.ReturnToDefaultBT calls. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|Villager")
	static void ReturnVillagerToDefaultBT(AActor* Target);

	/** Native replacement for BPI_Villager.PlayDeliverAnim calls. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|Villager")
	static double PlayVillagerDeliverAnim(AActor* Target);

	/** Native replacement for BPI_Villager.PlayWorkAnim calls. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|Villager")
	static void PlayVillagerWorkAnim(AActor* Target, double Delay);
};
