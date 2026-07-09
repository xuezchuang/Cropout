// Copyright (c) Cropout contributors. Licensed under the project license.

#pragma once

#include "CoreMinimal.h"
#include "CropoutInteractable.h"
#include "CropoutResourceInterface.h"
#include "CropoutResourceType.h"
#include "TimerManager.h"
#include "CropoutResource.generated.h"

/**
 * Native parent for BP_Resource.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API ACropoutResource : public ACropoutInteractable, public ICropoutResourceInterface
{
	GENERATED_BODY()

public:
	ACropoutResource();

	/** BP parity: `Resource Type` (E_ResourceType enum). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cropout|Resource", meta = (DisplayName = "Resource Type"))
	ECropoutResourceType NativeResourceType = ECropoutResourceType::Food;

	/** BP parity: `Resource Amount` (float). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cropout|Resource", meta = (DisplayName = "Resource Amount"))
	float NativeResourceAmount = 100.0f;

	/** BP parity: `Collection Time` (float). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cropout|Resource", meta = (DisplayName = "Collection Time"))
	float NativeCollectionTime = 3.0f;

	/** BP parity: `CollectionValue` (float). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cropout|Resource", meta = (DisplayName = "CollectionValue"))
	float NativeCollectionValue = 10.0f;

	/** BP parity: `Use Random Mesh` (bool). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cropout|Resource", meta = (DisplayName = "Use Random Mesh"))
	bool bNativeUseRandomMesh = false;

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Interact_Implementation(float& NewParam) override;

	/** BP parity: `Death()`. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|Resource")
	void Death();

	/** BP parity: `Island Seed()` / BPI_IslandPlugin. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|Resource|Island")
	FRandomStream IslandSeed();

	/** Native replacement for BP_Resource's no-input `Remove Resource` gather body. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|Resource", meta = (DisplayName = "Collect Resource"))
	void CollectResource(ECropoutResourceType& TargetResource, int32& Value);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Cropout|Resource", meta = (DisplayName = "Get Resource Type"))
	ECropoutResourceType GetResourceTypeValue() const;

	/** BP parity: IslandGenerator `EventScaleUP`. */
	UFUNCTION(BlueprintCallable, Category = "Cropout|Resource|Island", meta = (DisplayName = "Event Scale UP"))
	void EventScaleUP(float Delay);

	/** ICropoutResourceInterface inventory-style override; resource actors keep BP's no-op behavior here. */
	virtual void RemoveResource_Implementation(ECropoutResourceType RT, int32 Amount) override;

	virtual void RemoveTargetResource_Implementation(AActor* Target, int32 Amount) override;

	virtual TArray<ECropoutResourceType> GetCurrentResources_Implementation() const override;

	virtual bool CheckResource_Implementation(ECropoutResourceType RT, int32& OutAvailableCount) const override;
};

/**
 * Native parent for BP_BaseCrop.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API ACropoutBaseCrop : public ACropoutResource
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cropout|Crop", meta = (DisplayName = "Cooldown Time"))
	float NativeCooldownTime = 3.0f;

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void SetProgressionsState_Implementation(float Progression) override;
	virtual void Interact_Implementation(float& NewParam) override;

	UFUNCTION(BlueprintCallable, Category = "Cropout|Crop", meta = (DisplayName = "Set Ready"))
	void SetReady();

	UFUNCTION(BlueprintCallable, Category = "Cropout|Crop", meta = (DisplayName = "Switch Stage"))
	void SwitchStage();

	UFUNCTION(BlueprintCallable, Category = "Cropout|Crop")
	float FarmingProgress();

	UFUNCTION(BlueprintCallable, Category = "Cropout|Crop")
	void PopFarmPlot();

private:
	void NotifyInteractablesChanged() const;

	FTimerHandle SwitchStageTimerHandle;
	FTimerHandle SetReadyTimerHandle;
};
