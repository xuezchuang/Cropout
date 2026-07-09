// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CropoutResourceType.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "CropoutPlayerPawn.generated.h"

class UInputAction;
class UInputMappingContext;
class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UNiagaraComponent;
class UNiagaraSystem;
class APlayerController;
class AActor;
struct FInputActionValue;

/**
 * ACropoutPlayerPawn
 *
 * C++ base class for BP_Player.
 *
 * Round 1 (X-core 缩减版) scope:
 *  - This class is the C++ parent that BP_Player inherits from.
 *  - BP_Player keeps all component default values, all per-instance properties,
 *    all graph nodes, and the IA binding flow (BP_Player's Movement graph already
 *    binds the 6 IA assets via EnhancedInputActionIA_* nodes).
 *  - This class only:
 *      1. Forwards lifecycle (BeginPlay/EndPlay/SetupPlayerInputComponent/Tick)
 *         so a BP_Player subclass override can call Super::* safely.
 *      2. Exposes 21 BlueprintCallable UFUNCTIONs that match the BP_Player
 *         function-graph names 1:1, so a future round can route BP_Player's
 *         graph nodes to "Call Parent:<FuncName>" without renaming.
 *      3. Adds a small set of IA / IMC asset reference UPROPERTYs so this
 *         class is input-ready if a future round lifts the binding from
 *         BP_Player's Movement graph into C++.
 *
 *  Out of scope this round:
 *   - All per-instance gameplay state (zoom, build mod, picker mode, etc.)
 *     remains on BP_Player. We do NOT shadow these on the C++ parent.
 *   - Business logic body. All 21 UFUNCTION bodies are empty shells this
 *     round; the BP_Player graph still owns the real implementation.
 *   - BPF_Cropout / BPF_Shared / BPI_* / BP_Interactable: not migrated
 *     this round. The C++ parent calls them through BP-side state.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API ACropoutPlayerPawn : public APawn
{
	GENERATED_BODY()

public:
	ACropoutPlayerPawn();

	// ---------------------------------------------------------------------
	// Input asset references.
	//
	// These are kept on the C++ parent so a future round (Y) can lift the
	// Enhanced Input binding from BP_Player's Movement graph into this class.
	// The class-default values are intentionally left blank here; the BP
	// subclass will fill them in via the editor.
	// ---------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cropout|Input")
	TObjectPtr<UInputMappingContext> BaseInputMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cropout|Input")
	TObjectPtr<UInputMappingContext> BuildModeInputMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cropout|Input")
	TObjectPtr<UInputMappingContext> DragMoveInputMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cropout|Input")
	TObjectPtr<UInputMappingContext> VillagerModeInputMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cropout|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cropout|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cropout|Input")
	TObjectPtr<UInputAction> ZoomAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cropout|Input")
	TObjectPtr<UInputAction> SpinAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cropout|Input")
	TObjectPtr<UInputAction> BuildMoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cropout|Input")
	TObjectPtr<UInputAction> DragMoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cropout|Input")
	TObjectPtr<UInputAction> VillagerAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cropout|Input")
	int32 BaseMappingPriority = 0;

	// ---------------------------------------------------------------------
	// BP_Player function-graph parity surface.
	//
	// 21 UFUNCTIONs whose names match BP_Player's function graphs 1:1.
	// Empty shells this round; the BP_Player graph still owns the body.
	// Marked BlueprintCallable so the BP subclass can call them via
	// "Call Parent:<FuncName>" if a future round routes graph nodes to here.
	// ---------------------------------------------------------------------

	// 1) Construction — NOTE intentionally omitted.
	// AActor::UserConstructionScript is a BlueprintImplementableEvent (NOT a
	// C++ virtual), so C++ subclasses cannot override it. The BP_Player child
	// implements it via its Construction Script graph directly, which is the
	// only legal surface for this event. Earlier revisions of this header
	// declared a C++ override with a UFUNCTION() — UHT rejects that because
	// it collides with the parent's UFUNCTION metadata on the inherited
	// BlueprintImplementableEvent.

	// 2) Input dispatch hub (BP_Player wires IA events into this graph)
	UFUNCTION(BlueprintCallable, Category = "Cropout|Player")
	void InputSwitch(uint8 NewInput);

	// 3) Build-asset swap / preview sync
	UFUNCTION(BlueprintCallable, Category = "Cropout|Player|Build")
	void UpdateBuildAsset();

	// 4) Depth-of-field update
	// R16: re-added as BlueprintNativeEvent with empty _Implementation
	// body. BP_Player's BP-side function graph `Dof` is removed in
	// round 16 (via remove_function_graph); the C++ stub then becomes
	// the authoritative implementation entry. A future round will
	// migrate the BP-side logic (camera post-process settings on
	// spring-arm target length = 150mm, f-stop = 3.0) into the
	// _Implementation body.
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Cropout|Player|Camera")
	void Dof();
	virtual void Dof_Implementation();

	// 5) Cursor / hover trace tick
	UFUNCTION(BlueprintCallable, Category = "Cropout|Player|Cursor")
	void UpdateCursorPosition();

	// 6) Edge-scroll / drag movement tick
	// R16: re-added as BlueprintNativeEvent (see Dof header).
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Cropout|Player|Movement")
	void TrackMove();
	virtual void TrackMove_Implementation();

	// 7) Spring arm zoom interpolation
	UFUNCTION(BlueprintCallable, Category = "Cropout|Player|Camera")
	void UpdateZoom();

	// 8) Mouse-drag camera pan
	UFUNCTION(BlueprintCallable, Category = "Cropout|Player|Movement")
	void MoveTracking();

	UFUNCTION(BlueprintCallable, Category = "Cropout|Player|Input", meta = (ExpandBoolAsExecs = "ReturnValue"))
	bool SingleTouchCheck();

	UFUNCTION(BlueprintPure, Category = "Cropout|Player|Cursor")
	void CursorDistFromViewportCenter(FVector2D A, FVector& Direction, double& Strength);

	UFUNCTION(BlueprintPure, Category = "Cropout|Player|Movement")
	void EdgeMove(FVector& Direction, double& Strength);

	UFUNCTION(BlueprintPure, Category = "Cropout|Player|Cursor")
	bool ProjectMouseTouch1ToGroundPlane(
		UPARAM(DisplayName = "Screen Pos") FVector2D& ScreenPos,
		UPARAM(DisplayName = "Intersection") FVector& Intersection);

	UFUNCTION(BlueprintCallable, Category = "Cropout|Player|Build")
	void SteppedLocAndRot(FVector& Location, FRotator& Rotation);

	// 9) Nav-mesh corner check
	UFUNCTION(BlueprintCallable, Category = "Cropout|Player|Build")
	bool CornersInNav();

	// 10) Spawn the build ghost
	UFUNCTION(BlueprintCallable, Category = "Cropout|Player|Build")
	void CreateBuildOverlay();

	// 11) Tear down the build ghost
	UFUNCTION(BlueprintCallable, Category = "Cropout|Player|Build")
	void DestroySpawn();

	// 12) Pick the closest hover target
	UFUNCTION(BlueprintCallable, Category = "Cropout|Player|Cursor")
	void ClosestHoverCheck();

	// 13) PlayerController fetch helper
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cropout|Player")
	APlayerController* GetPlayerController();

	// 14) Niagara path preview
	UFUNCTION(BlueprintCallable, Category = "Cropout|Player|Path")
	void UpdatePath();

	// 15) R-axis / qe rotation of the ghost
	UFUNCTION(BlueprintCallable, Category = "Cropout|Player|Build")
	void RotateSpawn();

	// 16) Spatial overlap check for placement
	UFUNCTION(BlueprintCallable, Category = "Cropout|Player|Build")
	bool OverlapCheck();

	// 17) Click-select villager
	UFUNCTION(BlueprintCallable, Category = "Cropout|Player|Villager")
	void VillagerSelect(AActor* Selected);

	// 18) Release selected villager
	UFUNCTION(BlueprintCallable, Category = "Cropout|Player|Villager")
	void VillagerRelease();

	// 19) Resource debit on build confirm
	UFUNCTION(BlueprintCallable, Category = "Cropout|Player|Build")
	void RemoveResources();

	// 20) Spawn the real build target
	UFUNCTION(BlueprintCallable, Category = "Cropout|Player|Build")
	void SpawnBuildTarget();

	void BeginBuildNative(UClass* TargetClass, const TMap<ECropoutResourceType, int32>& ResourceCost);

	// 21) Position / rotation / validity check
	// R16: re-added as BlueprintNativeEvent (see Dof header).
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Cropout|Player|Build")
	void PositionCheck();
	virtual void PositionCheck_Implementation();

	// ---------------------------------------------------------------------
	// Game lifecycle events. The BP_Player graph may override any of these
	// and is expected to call Super::* to keep the C++ baseline behavior.
	// ---------------------------------------------------------------------
	UFUNCTION(BlueprintCallable, Category = "Cropout|Player")
	void EndBuild();

	UFUNCTION(BlueprintCallable, Category = "Cropout|Player")
	void SwitchBuildMode(bool bSwitchToBuildMode);

	UFUNCTION(BlueprintCallable, Category = "Cropout|Player")
	void UpdateSelectedVillager(bool bNewSelection);

	UFUNCTION(BlueprintCallable, Category = "Cropout|Player")
	void ResetPicker();

	UFUNCTION(BlueprintCallable, Category = "Cropout|Player|Villager", meta = (ExpandBoolAsExecs = "ReturnValue"))
	bool VillagerOverlapCheck(UPARAM(DisplayName = "Output") AActor*& Output);

protected:
	// APawn overrides ----------------------------------------------------
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	void HandleMoveTriggered(const FInputActionValue& ActionValue);
	void HandleSpinTriggered(const FInputActionValue& ActionValue);
	void HandleZoomTriggered(const FInputActionValue& ActionValue);
	void HandleBuildMoveTriggered(const FInputActionValue& ActionValue);
	void HandleBuildMoveCompleted(const FInputActionValue& ActionValue);
	void HandleVillagerStarted(const FInputActionValue& ActionValue);
	void HandleVillagerEnded(const FInputActionValue& ActionValue);
	void HandleDragMoveTriggered(const FInputActionValue& ActionValue);
	void SetDragMoveMappingContext(bool bEnabled);

	FTimerHandle MoveTrackingTimerHandle;
};
