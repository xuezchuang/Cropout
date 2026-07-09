// Copyright (c) Cropout contributors. Licensed under the project license.

#include "CropoutBTTasks.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "CropoutBlueprintLibrary.h"
#include "CropoutInteractable.h"
#include "CropoutVillager.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "UObject/SoftObjectPtr.h"

namespace
{
	UClass* LoadActorClass(const TCHAR* Path)
	{
		return TSoftClassPtr<AActor>(FSoftObjectPath(Path)).LoadSynchronous();
	}

	AActor* GetFirstActorOfClass(const UObject* WorldContextObject, UClass* ActorClass)
	{
		if (!WorldContextObject || !ActorClass)
		{
			return nullptr;
		}

		TArray<AActor*> Actors;
		UGameplayStatics::GetAllActorsOfClass(WorldContextObject, ActorClass, Actors);
		return Actors.IsValidIndex(0) ? Actors[0] : nullptr;
	}
}

UCropoutFindNearestOfClassTask::UCropoutFindNearestOfClassTask()
{
	NodeName = TEXT("Find Nearest Of Class");
}

void UCropoutFindNearestOfClassTask::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (Asset.BlackboardAsset)
	{
		Target.ResolveSelectedKey(*Asset.BlackboardAsset);
		NearestTo.ResolveSelectedKey(*Asset.BlackboardAsset);
		TargetClass.ResolveSelectedKey(*Asset.BlackboardAsset);
		BlackboardTag.ResolveSelectedKey(*Asset.BlackboardAsset);
	}
}

EBTNodeResult::Type UCropoutFindNearestOfClassTask::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComponent)
	{
		return EBTNodeResult::Failed;
	}

	if (BlackboardComponent->GetValueAsObject(Target.SelectedKeyName))
	{
		return EBTNodeResult::Succeeded;
	}

	const AActor* NearestToActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(NearestTo.SelectedKeyName));
	if (!NearestToActor)
	{
		return EBTNodeResult::Failed;
	}

	UClass* TargetActorClass = nullptr;
	if (bUseBlackboardClass)
	{
		TargetActorClass = BlackboardComponent->GetValueAsClass(TargetClass.SelectedKeyName);
	}
	else
	{
		TargetActorClass = ManualClass.Get();
	}

	if (!TargetActorClass || !TargetActorClass->IsChildOf(AActor::StaticClass()))
	{
		return EBTNodeResult::Failed;
	}

	const FName SelectedTag = bUseBlackboardTag
		? BlackboardComponent->GetValueAsName(BlackboardTag.SelectedKeyName)
		: TagFilter;

	TArray<AActor*> PossibleActors;
	const UObject* WorldContextObject = OwnerComp.GetOwner();
	if (SelectedTag.IsNone())
	{
		UGameplayStatics::GetAllActorsOfClass(WorldContextObject, TargetActorClass, PossibleActors);
	}
	else
	{
		UGameplayStatics::GetAllActorsOfClassWithTag(WorldContextObject, TargetActorClass, SelectedTag, PossibleActors);
	}

	if (PossibleActors.IsEmpty())
	{
		return EBTNodeResult::Failed;
	}

	UWorld* World = OwnerComp.GetWorld();
	const FVector Origin = NearestToActor->GetActorLocation();
	while (!PossibleActors.IsEmpty())
	{
		float Distance = 0.0f;
		AActor* NearestActor = UGameplayStatics::FindNearestActor(Origin, PossibleActors, Distance);
		if (!NearestActor)
		{
			break;
		}

		const UNavigationPath* Path = UNavigationSystemV1::FindPathToActorSynchronously(World, Origin, NearestActor, 100.0f);
		if (Path && !Path->IsPartial())
		{
			BlackboardComponent->SetValueAsObject(Target.SelectedKeyName, NearestActor);
			return EBTNodeResult::Succeeded;
		}

		PossibleActors.RemoveSingleSwap(NearestActor);
	}

	return EBTNodeResult::Failed;
}

UCropoutStuckRecoverTask::UCropoutStuckRecoverTask()
{
	NodeName = TEXT("Stuck Recover");
}

void UCropoutStuckRecoverTask::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (Asset.BlackboardAsset)
	{
		RecoveryPosition.ResolveSelectedKey(*Asset.BlackboardAsset);
	}
}

EBTNodeResult::Type UCropoutStuckRecoverTask::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	const AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* ControlledPawn = AIController ? AIController->GetPawn() : nullptr;
	if (!BlackboardComponent || !ControlledPawn)
	{
		return EBTNodeResult::Failed;
	}

	const FVector TargetLocation = BlackboardComponent->GetValueAsVector(RecoveryPosition.SelectedKeyName)
		+ FVector(0.0, 0.0, 45.0);
	return ControlledPawn->SetActorLocation(TargetLocation) ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
}

UCropoutDefaultBehaviorTask::UCropoutDefaultBehaviorTask()
{
	NodeName = TEXT("Default BT");
}

EBTNodeResult::Type UCropoutDefaultBehaviorTask::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* ControlledPawn = AIController ? AIController->GetPawn() : nullptr;
	UCropoutBlueprintLibrary::ReturnVillagerToDefaultBT(ControlledPawn);
	return EBTNodeResult::Succeeded;
}

UCropoutFindBoundsTask::UCropoutFindBoundsTask()
{
	NodeName = TEXT("Find Bounds");
}

void UCropoutFindBoundsTask::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (Asset.BlackboardAsset)
	{
		Target.ResolveSelectedKey(*Asset.BlackboardAsset);
		BBBound.ResolveSelectedKey(*Asset.BlackboardAsset);
	}
}

EBTNodeResult::Type UCropoutFindBoundsTask::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComponent)
	{
		return EBTNodeResult::Failed;
	}

	AActor* TargetActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(Target.SelectedKeyName));
	if (!TargetActor)
	{
		return EBTNodeResult::Failed;
	}

	FVector Origin;
	FVector BoxExtent;
	TargetActor->GetActorBounds(false, Origin, BoxExtent, false);
	BlackboardComponent->SetValueAsFloat(BBBound.SelectedKeyName, FMath::Min(BoxExtent.X, BoxExtent.Y) + AdditionalBounds);
	return EBTNodeResult::Succeeded;
}

UCropoutInitialCollectResourceTask::UCropoutInitialCollectResourceTask()
{
	NodeName = TEXT("Initial Collect Resource");
}

void UCropoutInitialCollectResourceTask::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (Asset.BlackboardAsset)
	{
		Key_Resource.ResolveSelectedKey(*Asset.BlackboardAsset);
		Key_ResourceClass.ResolveSelectedKey(*Asset.BlackboardAsset);
		Key_ResourceTag.ResolveSelectedKey(*Asset.BlackboardAsset);
		Key_CollectionClass.ResolveSelectedKey(*Asset.BlackboardAsset);
		Key_TownHall.ResolveSelectedKey(*Asset.BlackboardAsset);
	}
}

EBTNodeResult::Type UCropoutInitialCollectResourceTask::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	const AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* ControlledPawn = AIController ? AIController->GetPawn() : nullptr;
	if (!BlackboardComponent || !ControlledPawn || !ControlledPawn->Tags.IsValidIndex(0))
	{
		return EBTNodeResult::Failed;
	}

	UClass* ResourceClass = LoadActorClass(TEXT("/Game/Blueprint/Interactable/Resources/BP_Resource.BP_Resource_C"));
	if (!ResourceClass)
	{
		return EBTNodeResult::Failed;
	}

	const FName ResourceTag = ControlledPawn->Tags[0];
	TArray<AActor*> ResourceActors;
	UGameplayStatics::GetAllActorsOfClassWithTag(OwnerComp.GetOwner(), ResourceClass, ResourceTag, ResourceActors);
	if (ResourceActors.IsEmpty())
	{
		return EBTNodeResult::Failed;
	}

	BlackboardComponent->SetValueAsName(Key_ResourceTag.SelectedKeyName, ResourceTag);
	BlackboardComponent->SetValueAsClass(Key_ResourceClass.SelectedKeyName, ResourceClass);

	if (const ACropoutVillager* Villager = Cast<ACropoutVillager>(ControlledPawn))
	{
		BlackboardComponent->SetValueAsObject(Key_Resource.SelectedKeyName, Villager->GetTargetRefValue());
	}
	else
	{
		BlackboardComponent->SetValueAsObject(Key_Resource.SelectedKeyName, nullptr);
	}

	UClass* TownCenterClass = LoadActorClass(TEXT("/Game/Blueprint/Interactable/Building/BPC_TownCenter.BPC_TownCenter_C"));
	BlackboardComponent->SetValueAsObject(Key_TownHall.SelectedKeyName, GetFirstActorOfClass(OwnerComp.GetOwner(), TownCenterClass));
	return EBTNodeResult::Succeeded;
}

UCropoutInitialFarmingTargetTask::UCropoutInitialFarmingTargetTask()
{
	NodeName = TEXT("Initial Farming Target");
}

void UCropoutInitialFarmingTargetTask::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (Asset.BlackboardAsset)
	{
		Key_Resource.ResolveSelectedKey(*Asset.BlackboardAsset);
		Key_ResourceClass.ResolveSelectedKey(*Asset.BlackboardAsset);
		Key_CollectionClass.ResolveSelectedKey(*Asset.BlackboardAsset);
		Key_TownHall.ResolveSelectedKey(*Asset.BlackboardAsset);
	}
}

EBTNodeResult::Type UCropoutInitialFarmingTargetTask::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComponent)
	{
		return EBTNodeResult::Failed;
	}

	UClass* BaseCropClass = LoadActorClass(TEXT("/Game/Blueprint/Interactable/Resources/BP_BaseCrop.BP_BaseCrop_C"));
	UClass* BuildingBaseClass = LoadActorClass(TEXT("/Game/Blueprint/Interactable/Building/BP_BuildingBase.BP_BuildingBase_C"));
	UClass* TownCenterClass = LoadActorClass(TEXT("/Game/Blueprint/Interactable/Building/BPC_TownCenter.BPC_TownCenter_C"));
	if (!BaseCropClass || !BuildingBaseClass)
	{
		return EBTNodeResult::Failed;
	}

	AActor* ResourceActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(Key_Resource.SelectedKeyName));
	if (!IsValid(ResourceActor))
	{
		TArray<AActor*> ReadyCrops;
		UGameplayStatics::GetAllActorsOfClassWithTag(OwnerComp.GetOwner(), BaseCropClass, TEXT("Ready"), ReadyCrops);
		if (ReadyCrops.IsEmpty())
		{
			return EBTNodeResult::Failed;
		}

		const AAIController* AIController = OwnerComp.GetAIOwner();
		const APawn* ControlledPawn = AIController ? AIController->GetPawn() : nullptr;
		const FVector Origin = ControlledPawn ? ControlledPawn->GetActorLocation() : FVector::ZeroVector;
		float Distance = 0.0f;
		ResourceActor = UGameplayStatics::FindNearestActor(Origin, ReadyCrops, Distance);
		BlackboardComponent->SetValueAsObject(Key_Resource.SelectedKeyName, ResourceActor);
	}

	BlackboardComponent->SetValueAsClass(Key_ResourceClass.SelectedKeyName, BaseCropClass);
	BlackboardComponent->SetValueAsClass(Key_CollectionClass.SelectedKeyName, BuildingBaseClass);
	BlackboardComponent->SetValueAsObject(Key_TownHall.SelectedKeyName, GetFirstActorOfClass(OwnerComp.GetOwner(), TownCenterClass));
	return EBTNodeResult::Succeeded;
}

UCropoutTransferResourceTask::UCropoutTransferResourceTask()
{
	NodeName = TEXT("Transfer Resource");
}

void UCropoutTransferResourceTask::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (Asset.BlackboardAsset)
	{
		TakeFrom.ResolveSelectedKey(*Asset.BlackboardAsset);
		GiveTo.ResolveSelectedKey(*Asset.BlackboardAsset);
	}
}

EBTNodeResult::Type UCropoutTransferResourceTask::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComponent)
	{
		return EBTNodeResult::Failed;
	}

	AActor* TakeFromActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(TakeFrom.SelectedKeyName));
	AActor* GiveToActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(GiveTo.SelectedKeyName));
	if (!IsValid(TakeFromActor) || !IsValid(GiveToActor))
	{
		return EBTNodeResult::Failed;
	}

	uint8 Resource = 0;
	int32 Value = 0;
	UCropoutBlueprintLibrary::CollectActorResource(TakeFromActor, Resource, Value);
	UCropoutBlueprintLibrary::AddActorResource(GiveToActor, Resource, Value);
	return EBTNodeResult::Succeeded;
}

UCropoutWorkTask::UCropoutWorkTask()
{
	NodeName = TEXT("Work");
	bCreateNodeInstance = true;
}

void UCropoutWorkTask::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (Asset.BlackboardAsset)
	{
		TakeFrom.ResolveSelectedKey(*Asset.BlackboardAsset);
		GiveTo.ResolveSelectedKey(*Asset.BlackboardAsset);
	}
}

EBTNodeResult::Type UCropoutWorkTask::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComponent)
	{
		return EBTNodeResult::Failed;
	}

	AActor* TakeFromActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(TakeFrom.SelectedKeyName));
	AActor* GiveToActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(GiveTo.SelectedKeyName));
	if (!IsValid(TakeFromActor) || !IsValid(GiveToActor))
	{
		return EBTNodeResult::Failed;
	}

	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* ControlledPawn = AIController ? AIController->GetPawn() : nullptr;
	ACropoutInteractable* Interactable = Cast<ACropoutInteractable>(TakeFromActor);
	ActiveInteractable = Interactable;
	if (Interactable)
	{
		const FVector ControlledPawnLocation = ControlledPawn ? ControlledPawn->GetActorLocation() : FVector::ZeroVector;
		Interactable->PlayWobble(ControlledPawnLocation);
	}

	float Delay = 0.0f;
	if (Interactable)
	{
		Interactable->Interact(Delay);
	}

	UCropoutBlueprintLibrary::PlayVillagerWorkAnim(ControlledPawn, Delay);
	if (Delay <= 0.0f)
	{
		CompleteWork(OwnerComp);
		ActiveInteractable = nullptr;
		return EBTNodeResult::Succeeded;
	}

	UWorld* World = OwnerComp.GetWorld();
	if (!World)
	{
		ActiveInteractable = nullptr;
		return EBTNodeResult::Failed;
	}

	ActiveOwnerComp = &OwnerComp;
	World->GetTimerManager().SetTimer(
		WorkTimerHandle,
		this,
		&UCropoutWorkTask::OnWorkDelayFinished,
		Delay,
		false);
	return EBTNodeResult::InProgress;
}

EBTNodeResult::Type UCropoutWorkTask::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (UWorld* World = OwnerComp.GetWorld())
	{
		World->GetTimerManager().ClearTimer(WorkTimerHandle);
	}

	WorkTimerHandle.Invalidate();
	ActiveOwnerComp = nullptr;
	if (ACropoutInteractable* Interactable = ActiveInteractable.Get())
	{
		Interactable->EndWobble();
	}

	ActiveInteractable = nullptr;
	return EBTNodeResult::Aborted;
}

void UCropoutWorkTask::CompleteWork(UBehaviorTreeComponent& OwnerComp) const
{
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComponent)
	{
		return;
	}

	AActor* TakeFromActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(TakeFrom.SelectedKeyName));
	AActor* GiveToActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(GiveTo.SelectedKeyName));

	uint8 Resource = 0;
	int32 Value = 0;
	UCropoutBlueprintLibrary::CollectActorResource(TakeFromActor, Resource, Value);
	UCropoutBlueprintLibrary::AddActorResource(GiveToActor, Resource, Value);
}

void UCropoutWorkTask::OnWorkDelayFinished()
{
	UBehaviorTreeComponent* OwnerComp = ActiveOwnerComp.Get();
	ActiveOwnerComp = nullptr;
	WorkTimerHandle.Invalidate();
	ActiveInteractable = nullptr;
	if (!OwnerComp)
	{
		return;
	}

	CompleteWork(*OwnerComp);
	FinishLatentTask(*OwnerComp, EBTNodeResult::Succeeded);
}

UCropoutDeliverResourceTask::UCropoutDeliverResourceTask()
{
	NodeName = TEXT("Deliver Ressource");
	bCreateNodeInstance = true;
}

void UCropoutDeliverResourceTask::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (Asset.BlackboardAsset)
	{
		TakeFrom.ResolveSelectedKey(*Asset.BlackboardAsset);
		GiveTo.ResolveSelectedKey(*Asset.BlackboardAsset);
	}
}

EBTNodeResult::Type UCropoutDeliverResourceTask::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComponent)
	{
		return EBTNodeResult::Failed;
	}

	AActor* TakeFromActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(TakeFrom.SelectedKeyName));
	AActor* GiveToActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(GiveTo.SelectedKeyName));
	if (!IsValid(TakeFromActor) || !IsValid(GiveToActor))
	{
		return EBTNodeResult::Failed;
	}

	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* ControlledPawn = AIController ? AIController->GetPawn() : nullptr;
	const double Delay = UCropoutBlueprintLibrary::PlayVillagerDeliverAnim(ControlledPawn);
	if (Delay <= 0.0)
	{
		CompleteDelivery(OwnerComp);
		return EBTNodeResult::Succeeded;
	}

	UWorld* World = OwnerComp.GetWorld();
	if (!World)
	{
		return EBTNodeResult::Failed;
	}

	ActiveOwnerComp = &OwnerComp;
	World->GetTimerManager().SetTimer(
		DeliveryTimerHandle,
		this,
		&UCropoutDeliverResourceTask::OnDeliveryDelayFinished,
		static_cast<float>(Delay),
		false);
	return EBTNodeResult::InProgress;
}

EBTNodeResult::Type UCropoutDeliverResourceTask::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (UWorld* World = OwnerComp.GetWorld())
	{
		World->GetTimerManager().ClearTimer(DeliveryTimerHandle);
	}

	DeliveryTimerHandle.Invalidate();
	ActiveOwnerComp = nullptr;
	return EBTNodeResult::Aborted;
}

void UCropoutDeliverResourceTask::CompleteDelivery(UBehaviorTreeComponent& OwnerComp) const
{
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	AActor* TakeFromActor = BlackboardComponent
		? Cast<AActor>(BlackboardComponent->GetValueAsObject(TakeFrom.SelectedKeyName))
		: nullptr;

	uint8 Resource = 0;
	int32 Value = 0;
	UCropoutBlueprintLibrary::CollectActorResource(TakeFromActor, Resource, Value);
	UCropoutBlueprintLibrary::AddGameModeResource(&OwnerComp, Resource, Value);
}

void UCropoutDeliverResourceTask::OnDeliveryDelayFinished()
{
	UBehaviorTreeComponent* OwnerComp = ActiveOwnerComp.Get();
	ActiveOwnerComp = nullptr;
	DeliveryTimerHandle.Invalidate();
	if (!OwnerComp)
	{
		return;
	}

	CompleteDelivery(*OwnerComp);
	FinishLatentTask(*OwnerComp, EBTNodeResult::Succeeded);
}

UCropoutProgressConstructionTask::UCropoutProgressConstructionTask()
{
	NodeName = TEXT("Progress Construction");
	bCreateNodeInstance = true;
}

void UCropoutProgressConstructionTask::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (Asset.BlackboardAsset)
	{
		TargetBuild.ResolveSelectedKey(*Asset.BlackboardAsset);
	}
}

EBTNodeResult::Type UCropoutProgressConstructionTask::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* ControlledPawn = AIController ? AIController->GetPawn() : nullptr;
	UCropoutBlueprintLibrary::PlayVillagerWorkAnim(ControlledPawn, 1.0);

	UWorld* World = OwnerComp.GetWorld();
	if (!World)
	{
		return EBTNodeResult::Failed;
	}

	ActiveOwnerComp = &OwnerComp;
	World->GetTimerManager().SetTimer(
		ConstructionTimerHandle,
		this,
		&UCropoutProgressConstructionTask::OnConstructionDelayFinished,
		1.0f,
		false);
	return EBTNodeResult::InProgress;
}

EBTNodeResult::Type UCropoutProgressConstructionTask::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (UWorld* World = OwnerComp.GetWorld())
	{
		World->GetTimerManager().ClearTimer(ConstructionTimerHandle);
	}

	ConstructionTimerHandle.Invalidate();
	ActiveOwnerComp = nullptr;
	return EBTNodeResult::Aborted;
}

void UCropoutProgressConstructionTask::CompleteConstructionProgress(UBehaviorTreeComponent& OwnerComp) const
{
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComponent)
	{
		return;
	}

	AActor* TargetActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(TargetBuild.SelectedKeyName));
	ACropoutInteractable* Interactable = Cast<ACropoutInteractable>(TargetActor);

	float NewParam = 0.0f;
	if (Interactable)
	{
		Interactable->Interact(NewParam);
	}

	if (NewParam <= 0.0f)
	{
		BlackboardComponent->SetValueAsObject(TargetBuild.SelectedKeyName, nullptr);
	}
}

void UCropoutProgressConstructionTask::OnConstructionDelayFinished()
{
	UBehaviorTreeComponent* OwnerComp = ActiveOwnerComp.Get();
	ActiveOwnerComp = nullptr;
	ConstructionTimerHandle.Invalidate();
	if (!OwnerComp)
	{
		return;
	}

	CompleteConstructionProgress(*OwnerComp);
	FinishLatentTask(*OwnerComp, EBTNodeResult::Succeeded);
}

UCropoutProgressFarmingSequenceTask::UCropoutProgressFarmingSequenceTask()
{
	NodeName = TEXT("Progress Farming Seq");
	bCreateNodeInstance = true;
}

void UCropoutProgressFarmingSequenceTask::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (Asset.BlackboardAsset)
	{
		Crop.ResolveSelectedKey(*Asset.BlackboardAsset);
	}
}

EBTNodeResult::Type UCropoutProgressFarmingSequenceTask::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComponent)
	{
		return EBTNodeResult::Failed;
	}

	AActor* CropActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(Crop.SelectedKeyName));
	static const FName ReadyTag(TEXT("Ready"));
	static const FName HarvestTag(TEXT("Harvest"));
	if (!CropActor || (!CropActor->ActorHasTag(ReadyTag) && !CropActor->ActorHasTag(HarvestTag)))
	{
		BlackboardComponent->SetValueAsObject(Crop.SelectedKeyName, nullptr);
		return EBTNodeResult::Succeeded;
	}

	TagState = CropActor->Tags.IsValidIndex(1) ? CropActor->Tags[1] : NAME_None;

	float Delay = 0.0f;
	if (ACropoutInteractable* Interactable = Cast<ACropoutInteractable>(CropActor))
	{
		Interactable->Interact(Delay);
	}

	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* ControlledPawn = AIController ? AIController->GetPawn() : nullptr;
	UCropoutBlueprintLibrary::PlayVillagerWorkAnim(ControlledPawn, Delay);
	if (Delay <= 0.0f)
	{
		return CompleteFarmingSequence(OwnerComp);
	}

	UWorld* World = OwnerComp.GetWorld();
	if (!World)
	{
		return EBTNodeResult::Failed;
	}

	ActiveOwnerComp = &OwnerComp;
	World->GetTimerManager().SetTimer(
		FarmingTimerHandle,
		this,
		&UCropoutProgressFarmingSequenceTask::OnFarmingDelayFinished,
		Delay,
		false);
	return EBTNodeResult::InProgress;
}

EBTNodeResult::Type UCropoutProgressFarmingSequenceTask::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (UWorld* World = OwnerComp.GetWorld())
	{
		World->GetTimerManager().ClearTimer(FarmingTimerHandle);
	}

	FarmingTimerHandle.Invalidate();
	ActiveOwnerComp = nullptr;
	TagState = NAME_None;
	return EBTNodeResult::Aborted;
}

EBTNodeResult::Type UCropoutProgressFarmingSequenceTask::CompleteFarmingSequence(UBehaviorTreeComponent& OwnerComp) const
{
	static const FName HarvestTag(TEXT("Harvest"));
	if (TagState == HarvestTag)
	{
		return EBTNodeResult::Failed;
	}

	if (UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent())
	{
		BlackboardComponent->SetValueAsObject(Crop.SelectedKeyName, nullptr);
	}

	return EBTNodeResult::Succeeded;
}

void UCropoutProgressFarmingSequenceTask::OnFarmingDelayFinished()
{
	UBehaviorTreeComponent* OwnerComp = ActiveOwnerComp.Get();
	ActiveOwnerComp = nullptr;
	FarmingTimerHandle.Invalidate();
	if (!OwnerComp)
	{
		return;
	}

	const EBTNodeResult::Type Result = CompleteFarmingSequence(*OwnerComp);
	TagState = NAME_None;
	FinishLatentTask(*OwnerComp, Result);
}

UCropoutPlayNiagaraTask::UCropoutPlayNiagaraTask()
{
	NodeName = TEXT("Play Niagara");
}

EBTNodeResult::Type UCropoutPlayNiagaraTask::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const AAIController* AIController = OwnerComp.GetAIOwner();
	const APawn* ControlledPawn = AIController ? AIController->GetPawn() : nullptr;
	if (System && ControlledPawn && ControlledPawn->GetRootComponent())
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			System,
			ControlledPawn->GetRootComponent(),
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset,
			false,
			true,
			ENCPoolMethod::None,
			true);
	}

	return EBTNodeResult::Succeeded;
}
