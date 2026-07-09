// Copyright (c) Cropout contributors. Licensed under the project license.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "TimerManager.h"
#include "CropoutBTTasks.generated.h"

class AActor;
class ACropoutInteractable;
class UBehaviorTree;
class UBehaviorTreeComponent;
class UNiagaraSystem;

/**
 * Native parent for BTT_FindNearestOfClass.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API UCropoutFindNearestOfClassTask : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UCropoutFindNearestOfClassTask();

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector Target;

	UPROPERTY(EditAnywhere, Category = "Blackboard", meta = (DisplayName = "Nearest To"))
	FBlackboardKeySelector NearestTo;

	UPROPERTY(EditAnywhere, Category = "Class", meta = (DisplayName = "Use Blackboard Class"))
	bool bUseBlackboardClass = false;

	UPROPERTY(EditAnywhere, Category = "Class", meta = (DisplayName = "Target Class", EditCondition = "bUseBlackboardClass"))
	FBlackboardKeySelector TargetClass;

	UPROPERTY(EditAnywhere, Category = "Class", meta = (DisplayName = "Manual Class", EditCondition = "!bUseBlackboardClass"))
	TSubclassOf<AActor> ManualClass;

	UPROPERTY(EditAnywhere, Category = "Tag", meta = (DisplayName = "Use Blackboard Tag"))
	bool bUseBlackboardTag = true;

	UPROPERTY(EditAnywhere, Category = "Tag", meta = (DisplayName = "Blackboard Tag", EditCondition = "bUseBlackboardTag"))
	FBlackboardKeySelector BlackboardTag;

	UPROPERTY(EditAnywhere, Category = "Tag", meta = (DisplayName = "Tag Filter", EditCondition = "!bUseBlackboardTag"))
	FName TagFilter;

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};

/**
 * Native parent for BTT_FindBounds.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API UCropoutFindBoundsTask : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UCropoutFindBoundsTask();

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector Target;

	UPROPERTY(EditAnywhere, Category = "Blackboard", meta = (DisplayName = "BB Bound"))
	FBlackboardKeySelector BBBound;

	UPROPERTY(EditAnywhere, Category = "Bounds", meta = (DisplayName = "Additional Bounds"))
	float AdditionalBounds = 20.0f;

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};

/**
 * Native parent for BTT_DefaultBT.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API UCropoutDefaultBehaviorTask : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UCropoutDefaultBehaviorTask();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};

/**
 * Native parent for BTT_InitialCollectResource.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API UCropoutInitialCollectResourceTask : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UCropoutInitialCollectResourceTask();

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector Key_Resource;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector Key_ResourceClass;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector Key_ResourceTag;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector Key_CollectionClass;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector Key_TownHall;

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};

/**
 * Native parent for BTT_InitialFarmingTarget.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API UCropoutInitialFarmingTargetTask : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UCropoutInitialFarmingTargetTask();

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector Key_Resource;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector Key_ResourceClass;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector Key_CollectionClass;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector Key_TownHall;

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};

/**
 * Native parent for BTT_TransferResource.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API UCropoutTransferResourceTask : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UCropoutTransferResourceTask();

	UPROPERTY(EditAnywhere, Category = "Blackboard", meta = (DisplayName = "Take From"))
	FBlackboardKeySelector TakeFrom;

	UPROPERTY(EditAnywhere, Category = "Blackboard", meta = (DisplayName = "Give To"))
	FBlackboardKeySelector GiveTo;

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};

/**
 * Native parent for BTT_Work.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API UCropoutWorkTask : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UCropoutWorkTask();

	UPROPERTY(EditAnywhere, Category = "Blackboard", meta = (DisplayName = "Take From"))
	FBlackboardKeySelector TakeFrom;

	UPROPERTY(EditAnywhere, Category = "Blackboard", meta = (DisplayName = "Give To"))
	FBlackboardKeySelector GiveTo;

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

private:
	void CompleteWork(UBehaviorTreeComponent& OwnerComp) const;
	void OnWorkDelayFinished();

	TWeakObjectPtr<UBehaviorTreeComponent> ActiveOwnerComp;
	TWeakObjectPtr<ACropoutInteractable> ActiveInteractable;
	FTimerHandle WorkTimerHandle;
};

/**
 * Native parent for BTT_DeliverRessource.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API UCropoutDeliverResourceTask : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UCropoutDeliverResourceTask();

	UPROPERTY(EditAnywhere, Category = "Blackboard", meta = (DisplayName = "Take From"))
	FBlackboardKeySelector TakeFrom;

	UPROPERTY(EditAnywhere, Category = "Blackboard", meta = (DisplayName = "Give To"))
	FBlackboardKeySelector GiveTo;

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

private:
	void CompleteDelivery(UBehaviorTreeComponent& OwnerComp) const;
	void OnDeliveryDelayFinished();

	TWeakObjectPtr<UBehaviorTreeComponent> ActiveOwnerComp;

	FTimerHandle DeliveryTimerHandle;
};

/**
 * Native parent for BTT_ProgressConstruction.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API UCropoutProgressConstructionTask : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UCropoutProgressConstructionTask();

	UPROPERTY(EditAnywhere, Category = "Blackboard", meta = (DisplayName = "TargetBuild"))
	FBlackboardKeySelector TargetBuild;

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

private:
	void CompleteConstructionProgress(UBehaviorTreeComponent& OwnerComp) const;
	void OnConstructionDelayFinished();

	TWeakObjectPtr<UBehaviorTreeComponent> ActiveOwnerComp;
	FTimerHandle ConstructionTimerHandle;
};

/**
 * Native parent for BTT_ProgressFarmingSeq.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API UCropoutProgressFarmingSequenceTask : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UCropoutProgressFarmingSequenceTask();

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector Crop;

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

private:
	EBTNodeResult::Type CompleteFarmingSequence(UBehaviorTreeComponent& OwnerComp) const;
	void OnFarmingDelayFinished();

	TWeakObjectPtr<UBehaviorTreeComponent> ActiveOwnerComp;
	FName TagState = NAME_None;
	FTimerHandle FarmingTimerHandle;
};

/**
 * Native parent for BTT_PlayNiagara.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API UCropoutPlayNiagaraTask : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UCropoutPlayNiagaraTask();

	UPROPERTY(EditAnywhere, Category = "Niagara")
	TObjectPtr<UNiagaraSystem> System = nullptr;

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};

/**
 * Native parent for BTT_StuckRecover.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API UCropoutStuckRecoverTask : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UCropoutStuckRecoverTask();

	UPROPERTY(EditAnywhere, Category = "Blackboard", meta = (DisplayName = "Recovery Position"))
	FBlackboardKeySelector RecoveryPosition;

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
