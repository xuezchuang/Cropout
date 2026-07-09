// Copyright (c) Cropout contributors. Licensed under the project license.

#pragma once

#include "CoreMinimal.h"
#include "CropoutResourceType.h"
#include "GameFramework/Pawn.h"
#include "CropoutVillager.generated.h"

class UAnimMontage;
class UBehaviorTree;
class UCapsuleComponent;
class USkeletalMesh;
class USkeletalMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * Native parent for BP_Villager.
 *
 * The Blueprint keeps its SCS components and data variables. Native code looks
 * them up by component/variable name so reparenting can remove function graphs
 * without duplicating component defaults in C++.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API ACropoutVillager : public APawn
{
	GENERATED_BODY()

public:
	ACropoutVillager();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Cropout|Villager")
	void Eat();

	UFUNCTION(BlueprintCallable, Category = "Cropout|Villager")
	void ResetJobState();

	UFUNCTION(BlueprintCallable, Category = "Cropout|Villager")
	void StopJob();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cropout|Villager", meta = (DisplayName = "Hair Pick"))
	TSoftObjectPtr<USkeletalMesh> HairPick() const;

	UFUNCTION(BlueprintCallable, Category = "Cropout|Villager")
	void ChangeJob(FName NewJob);

	UFUNCTION(BlueprintCallable, Category = "Cropout|Villager")
	void Action(AActor* NewParam);

	UFUNCTION(BlueprintCallable, Category = "Cropout|Villager")
	void ReturnToDefaultBT();

	UFUNCTION(BlueprintCallable, Category = "Cropout|Villager", meta = (DisplayName = "Return to Idle"))
	void ReturntoIdle();

	UFUNCTION(BlueprintCallable, Category = "Cropout|Villager")
	void PlayWorkAnim(double Delay);

	UFUNCTION(BlueprintCallable, Category = "Cropout|Villager")
	double PlayDeliverAnim();

	UFUNCTION(BlueprintCallable, Category = "Cropout|Villager")
	double ProgressBuilding(double InvestedTime);

	UFUNCTION(BlueprintCallable, Category = "Cropout|Villager")
	void AddResource(uint8 Resource, int32 Value);

	UFUNCTION(BlueprintCallable, Category = "Cropout|Villager", meta = (DisplayName = "Remove Resource"))
	void TakeHeldResource(uint8& Resource, int32& Value);

	AActor* GetTargetRefValue() const;

private:
	USkeletalMeshComponent* GetVillagerMesh() const;
	USkeletalMeshComponent* GetHairComponent() const;
	USkeletalMeshComponent* GetHatComponent() const;
	UStaticMeshComponent* GetToolComponent() const;
	UCapsuleComponent* GetCapsuleComponent() const;

	void PlayVillagerAnim(UAnimMontage* Montage, double Length);
	void StopVillagerMontageAndHideTool();
	void SetBlackboardTargetIfAvailable(AActor* TargetActor);

	FName GetNewJobValue() const;
	void SetNewJobValue(FName Value);
	void SetTargetRefValue(AActor* Value);
	UAnimMontage* GetWorkAnimValue() const;
	void SetWorkAnimValue(UAnimMontage* Value);
	UStaticMesh* GetTargetToolValue() const;
	void SetTargetToolValue(UStaticMesh* Value);
	void SetActiveBehaviorValue(UBehaviorTree* Value);
	ECropoutResourceType GetResourcesHeldValue() const;
	void SetResourcesHeldValue(ECropoutResourceType Value);
	int32 GetQuantityValue() const;
	void SetQuantityValue(int32 Value);
	void SetCacheResourceValue(ECropoutResourceType Value);
	ECropoutResourceType GetCacheResourceValue() const;
	void SetCacheValueValue(int32 Value);
	int32 GetCacheValueValue() const;

	UPROPERTY(Transient)
	TObjectPtr<AActor> NativeTargetRef;

	UPROPERTY(Transient)
	TObjectPtr<UBehaviorTree> NativeActiveBehavior;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> NativeWorkAnim;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> NativeTargetTool;

	FName NativeNewJob = NAME_None;
	ECropoutResourceType NativeResourcesHeld = ECropoutResourceType::None;
	ECropoutResourceType NativeCacheResource = ECropoutResourceType::None;
	int32 NativeQuantity = 0;
	int32 NativeCacheValue = 0;
	FTimerHandle EatTimerHandle;
	FTimerHandle StopMontageTimerHandle;
};
