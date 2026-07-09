// Copyright (c) Cropout contributors. Licensed under the project license.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "CropoutResourceType.h"
#include "CropoutSaveGame.generated.h"

class UClass;
class AActor;

/**
 * FCropoutSaveInteract
 *
 * Per-actor snapshot stored in `UCropoutSaveGame::Interactables`.
 *
 * BP DSL source (BP_GI.HandleEventUpdateAllInteractables, captured
 * in `CropoutGameInstance.cpp`):
 *
 *   SetArrayElem(SaveRef.Interactables,
 *                Actor,
 *                GetActorTransform(Actor),       // -> Transform
 *                GetClass(Actor),                // -> Class
 *                BPInteractable.GetProgressionState(Actor),  // -> ProgressionState
 *                Get(Actor.Tags, index 0),       // -> Tag
 *                true)                           // -> bIsValid
 */
USTRUCT(BlueprintType)
struct CROPOUTSAMPLEPROJECT_API FCropoutSaveInteract
{
	GENERATED_BODY()

	/** World transform of the actor at save time. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cropout|SaveGame")
	FTransform Transform = FTransform::Identity;

	/** Class of the actor (preserves derived type so re-spawn uses the right blueprint). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cropout|SaveGame")
	TObjectPtr<UClass> Class = nullptr;

	/** Progression state pushed by `BP_Interactable.GetProgressionState`. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cropout|SaveGame")
	int32 ProgressionState = 0;

	/** First tag from `Actor.Tags`. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cropout|SaveGame")
	FName Tag = NAME_None;

	/** Sanity flag — mirrors the trailing `true` literal in the BP graph. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cropout|SaveGame")
	bool bIsValid = true;
};

/**
 * FCropoutSaveVillager
 *
 * Per-villager snapshot stored in `UCropoutSaveGame::Villagers`.
 *
 * BP DSL source (BP_GI.HandleEventUpdateAllVillagers):
 *
 *   SetArrayElem(SaveRef.Villagers,
 *                Actor,
 *                GetActorLocation(Actor),        // -> Location
 *                Get(Actor.Tags, 0),             // -> Tag
 *                true)                           // -> bIsValid
 */
USTRUCT(BlueprintType)
struct CROPOUTSAMPLEPROJECT_API FCropoutSaveVillager
{
	GENERATED_BODY()

	/** World location of the villager pawn at save time. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cropout|SaveGame")
	FVector Location = FVector::ZeroVector;

	/** First tag from `Actor.Tags`. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cropout|SaveGame")
	FName Tag = NAME_None;

	/** Sanity flag — mirrors the trailing `true` literal in the BP graph. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cropout|SaveGame")
	bool bIsValid = true;
};

/**
 * UCropoutSaveGame
 *
 * C++ parent class for `BP_SaveGM`.
 *
 * Round 8 scope (current):
 *  - Mirror `BP_SaveGM`'s 3 BP-side variables (`Interactables`,
 *    `Resources`, `Villagers`) + 1 derived seed field (`Seed`).
 *  - Mirror the 7 BP-side functions invoked by `UCropoutGameInstance`
 *    via `UFunction` reflection (`ClearInteractables`,
 *    `AddInteractable`, `ClearVillagers`, `AddVillager`,
 *    `SetResources`, `ClearSave`, `SetSeed`). Once `BP_SaveGM`
 *    is reparented to this class and the round-7 reflection calls
 *    are rewritten to direct method dispatch, the runtime cost
 *    of `FCallBPSaveGameFuncWithMap` / `FAddInteractable_Params`
 *    goes away.
 *
 * Note: BP-side `Resources` is `TMap<E_ResourceType, int32>`. The
 * C++ surface uses `TMap<int32, int32>` because BP UserDefinedEnum
 * <-> native UENUM has no compile-time bridge (round 4 rollback).
 * Once `BP_SaveGM` is reparented, callers must continue passing
 * the int32-cast value of `E_ResourceType` enum entries.
 *
 * The persisted `Seed` field tracks `PCGSettings.SetSeed` calls
 * performed during `ClearSave(bClearSeed=true)` flow.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API UCropoutSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UCropoutSaveGame();

	// -----------------------------------------------------------------
	// State (mirror BP_SaveGM's BP-side variables).
	// -----------------------------------------------------------------

	/** BP parity: `Interactables` — one entry per `BP_Interactable` actor in the world. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cropout|SaveGame")
	TArray<FCropoutSaveInteract> Interactables;

	/** BP parity: `Villagers` — one entry per non-player pawn. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cropout|SaveGame")
	TArray<FCropoutSaveVillager> Villagers;

	/** BP parity: `Resources` — `TMap<E_ResourceType, int32>`. Round 12.C stores as a native `TMap<ECropoutResourceType, int32>` (key enum aligned with BP internal values: Wood=0, None=1, Stone=5, Food=6). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cropout|SaveGame")
	TMap<ECropoutResourceType, int32> Resources;

	/** Derived state: `PCGSettings.SetSeed` argument used in `ClearSave(bClearSeed=true)`. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cropout|SaveGame")
	int32 Seed = 0;

	/** BP parity: `Play Time` (seconds since level start). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cropout|SaveGame")
	float PlayTime = 0.0f;

	// -----------------------------------------------------------------
	// BP-parity functions (mirror the 7 BP-side functions invoked by
	// BP_GI via UFunction reflection; round 7 source: CropoutGameInstance.cpp).
	// -----------------------------------------------------------------

	/** BP parity: `ClearInteractables` (no params, no return). */
	UFUNCTION(BlueprintCallable, Category = "Cropout|SaveGame")
	void ClearInteractables();

	/** BP parity: `AddInteractable(Transform, Class, ProgressionState, Tag, Source)`. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|SaveGame")
	void AddInteractable(const FTransform& Transform, UClass* Class, int32 ProgressionState, FName Tag, AActor* Source);

	/** BP parity: `ClearVillagers` (no params, no return). */
	UFUNCTION(BlueprintCallable, Category = "Cropout|SaveGame")
	void ClearVillagers();

	/** BP parity: `AddVillager(Location, Tag, Source)`. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|SaveGame")
	void AddVillager(const FVector& Location, FName Tag, AActor* Source);

	/** BP parity: `SetResources(NewParam)`. Round 12.C key type is `ECropoutResourceType`. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|SaveGame")
	void SetResources(const TMap<ECropoutResourceType, int32>& InResources);

	/** BP parity: `GetResources` (getter for the Resources map). */
	UFUNCTION(BlueprintPure, Category = "Cropout|SaveGame")
	TMap<ECropoutResourceType, int32> GetResources() const;

	/** BP parity: `GetInteractables` (getter for the Interactables array). */
	UFUNCTION(BlueprintPure, Category = "Cropout|SaveGame")
	TArray<FCropoutSaveInteract> GetInteractables() const;

	/** BP parity: `GetVillagers` (getter for the Villagers array). */
	UFUNCTION(BlueprintPure, Category = "Cropout|SaveGame")
	TArray<FCropoutSaveVillager> GetVillagers() const;

	/** BP parity: `ClearSave` (no params, no return). */
	UFUNCTION(BlueprintCallable, Category = "Cropout|SaveGame")
	void ClearSave();

	/** BP parity: `SetSeed` — initialises `Seed` from a fresh RNG. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|SaveGame")
	void SetSeed();

	/** BP parity: `GetSeed` — returns the current island seed. */
	UFUNCTION(BlueprintPure, Category = "Cropout|SaveGame")
	int32 GetSeed() const;

	/** BP parity: `SetPlayTime` — sets `PlayTime` snapshot. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|SaveGame")
	void SetPlayTime(float InPlayTime);
};
