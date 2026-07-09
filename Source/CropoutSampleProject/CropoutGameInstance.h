// Copyright (c) Cropout contributors. Licensed under the project license.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "CropoutGameInstanceInterface.h"
#include "CropoutGameInstance.generated.h"

class UUserWidget;
class USaveGame;
class USoundMix;
class APawn;
class AActor;

/**
 * UCropoutGameInstance
 *
 * C++ parent class for BP_GI.
 *
 * Round 2B scope: empty Blueprintable shell, no state, no logic.
 * Round 5 scope: unchanged.
 * Round 7 scope (current):
 *  - 8 UPROPERTYs mirror BP_GI's 8 BP-side variables. The SaveRef
 *    type is `TObjectPtr<USaveGame>` (base class) — the BP-side
 *    `BPSaveGM` USaveGame subclass stays BP-defined for now; round 8+
 *    will lift `BPSaveGM` to a native `UCropoutSaveGame` class.
 *  - 9 event handler UFUNCTIONs mirror BP_GI's 9 EventGraph entry
 *    points (SetSaveData|EventUpdateAllInteractables, EventInit,
 *    Custom|OpenLevel, SetSaveData|EventUpdateAllResources,
 *    SetSaveData|EventClearSave, EventLoadLevel,
 *    SetSaveData|EventUpdateAllVillagers, SetSaveData|EventSaveGame,
 *    Custom|AddLoadingUI).
 *  - 5 ICropoutGameInstanceInterface `_Implementation` overrides.
 *
 *  The class implements the round-5 native GameInstance interface
 *  transitively. Cross-BP calls into `BPSaveGM` (SetResources,
 *  GetResources, GetInteractables, GetVillagers) are invoked via
 *  UFunction reflection on the BP-side USaveGame subclass until
 *  round 8+ lifts BPSaveGM to native.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API UCropoutGameInstance
	: public UGameInstance
	, public ICropoutGameInstanceInterface
{
	GENERATED_BODY()

public:
	UCropoutGameInstance();

	virtual void Init() override;

	// -----------------------------------------------------------------
	// UPROPERTYs (mirror BP_GI's 8 BP-side variables).
	// -----------------------------------------------------------------

	/** BP parity: `TargetSeed` (int). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cropout|GameInstance")
	int32 TargetSeed = 0;

	/** BP parity: `UI_Transition` (UI widget ref for transition overlays). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cropout|GameInstance")
	TObjectPtr<UUserWidget> UI_Transition = nullptr;

	/** BP parity: `Has Save` (bool). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cropout|GameInstance")
	bool HasSave = false;

	/** BP parity: `SaveRef` (USaveGame subclass ref). The actual type is `BPSaveGM`; round 8+ will lift to `UCropoutSaveGame`. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cropout|GameInstance")
	TObjectPtr<USaveGame> SaveRef = nullptr;

	/** BP parity: `Start Game Offset` (game-time seconds at EventInit). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cropout|GameInstance")
	float StartGameOffset = 0.0f;

	/** BP parity: `Audio` (USoundMix ref). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cropout|GameInstance")
	TObjectPtr<USoundMix> Audio = nullptr;

	/** BP parity: `Music Playing` (bool). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cropout|GameInstance")
	bool MusicPlaying = false;

	/** BP parity: `SoundMixes` (slider volume values). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cropout|GameInstance")
	TArray<float> SoundMixes;

	// -----------------------------------------------------------------
	// Event handlers (mirror BP_GI's 9 EventGraph entry points).
	// -----------------------------------------------------------------

	/** BP parity: `SetSaveData|EventUpdateAllInteractables`. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameInstance|Event")
	void HandleEventUpdateAllInteractables();

	/** BP parity: `EventInit`. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameInstance|Event")
	void HandleEventInit();

	/** BP parity: `Custom|OpenLevel (Level)`. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameInstance|Event")
	void HandleOpenLevel(TSoftObjectPtr<UWorld> Level);

	/** BP parity: `SetSaveData|EventUpdateAllResources (NewParam)`. Round 12.C: key type is `ECropoutResourceType`. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameInstance|Event")
	void HandleEventUpdateAllResources(const TMap<ECropoutResourceType, int32>& Resources);

	/** BP parity: `SetSaveData|EventClearSave (ClearSeed)`. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameInstance|Event")
	void HandleEventClearSave(bool ClearSeed);

	/** BP parity: `EventLoadLevel`. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameInstance|Event")
	void HandleEventLoadLevel();

	/** BP parity: `SetSaveData|EventUpdateAllVillagers`. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameInstance|Event")
	void HandleEventUpdateAllVillagers();

	/** BP parity: `SetSaveData|EventSaveGame`. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameInstance|Event")
	void HandleEventSaveGame();

	/** BP parity: `Custom|AddLoadingUI`. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameInstance|Event")
	void HandleAddLoadingUI();

	/**
	 * BP parity: `LoadGame` (called by `HandleEventInit` body after widget setup).
	 *
	 * Native equivalent of the BP-side LoadGame event graph: if a save
	 * exists at slot "SAVE", synchronously load it into SaveRef and set
	 * HasSave=true. Round 9+ replaces the reflection call in
	 * `HandleEventInit` with a direct call; the BP graph can then
	 * delete its own LoadGraph and route `Custom|LoadGame` to
	 * `Call Parent.HandleLoadGame`.
	 */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameInstance|Event")
	void HandleLoadGame();

	// -----------------------------------------------------------------
	// BP_GI local logic lifted to C++ (Round 9). These mirror the
	// functions that previously lived only in BP_GI's Blueprint graph.
	// -----------------------------------------------------------------

	/** BP parity: `Play Music (Audio, Volume, Persist)`. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameInstance")
	void PlayMusic(USoundBase* Sound, float Volume, bool bPersist);

	/** BP parity: `Stop Music`. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameInstance")
	void StopMusic();

	/**
	 * BP parity: `TransitionIn` — ensures UI_Transition is in viewport and calls TransIn.
	 * Renamed from `TransitionIn` so BP_GI's now-empty local `TransitionIn` graph
	 * does not shadow this implementation. Internal callers use the C++ name.
	 */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameInstance")
	void DoTransitionIn();

	/** BP-call compatibility: old callers reference `TransitionIn`; implementation lives in C++. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameInstance")
	void TransitionIn();

	/**
	 * BP parity: `TransitionOut` — ensures UI_Transition is in viewport and calls TransOut.
	 * Renamed from `TransitionOut` for the same reason as `DoTransitionIn`.
	 */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameInstance")
	void DoTransitionOut();

	/** BP-call compatibility: old callers reference `TransitionOut`; implementation lives in C++. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameInstance")
	void TransitionOut();

	/** BP parity: `Check Save Bool` — returns whether a save slot exists. */
	UFUNCTION(BlueprintPure, Category = "Cropout|GameInstance")
	bool CheckSaveBool() const;

	/** BP parity: `Get Save` — returns the current SaveRef. */
	UFUNCTION(BlueprintPure, Category = "Cropout|GameInstance")
	USaveGame* GetSave() const;

	/** BP parity: `Get All Interactables` — returns SaveRef.Interactables. */
	UFUNCTION(BlueprintPure, Category = "Cropout|GameInstance")
	TArray<FCropoutSaveInteract> GetAllInteractables() const;

	/** BP parity: `Get Seed` — returns SaveRef.Seed. */
	UFUNCTION(BlueprintPure, Category = "Cropout|GameInstance")
	int32 GetSeed() const;

	/** BP parity: `Island Seed` — returns SaveRef.Seed. */
	UFUNCTION(BlueprintPure, Category = "Cropout|GameInstance")
	int32 IslandSeed() const;

	/** BP parity: `Clear Save` — wipes SaveRef in memory without writing the slot. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameInstance")
	void ClearSaveAsset();

	/** BP parity: `Update Save Asset` — snapshots pawns / interactables / resources into SaveRef. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameInstance")
	void UpdateSaveAsset();

	/** BP parity: `Start New Level`. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameInstance")
	void StartNewLevel();

	/** BP parity: `Update Save Data`. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameInstance")
	void UpdateSaveData();

	/** BP parity: `Island Gen Complete`. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameInstance")
	void IslandGenComplete();

	/** BP parity: `Scale UP (Delay)`. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|GameInstance")
	void ScaleUp(float Delay);

	// -----------------------------------------------------------------
	// ICropoutGameInstanceInterface (5 BlueprintNativeEvent implementations).
	// -----------------------------------------------------------------

	virtual void UpdateAllResources_Implementation(const TMap<ECropoutResourceType, int32>& Resources) override;
	virtual void UpdateAllVillagers_Implementation() override;
	virtual void UpdateAllInteractables_Implementation() override;
	virtual void SaveGame_Implementation() override;
	virtual void ClearSave_Implementation(bool ClearSeed) override;
};
