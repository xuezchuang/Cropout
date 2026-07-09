// Copyright (c) Cropout contributors. Licensed under the project license.

#include "CropoutEqsContexts.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"
#include "GameFramework/Actor.h"

void UCropoutTargetTownHallEqsContext::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	const UWorld* World = QueryInstance.World.Get();
	if (!World || !TownCenterClass)
	{
		return;
	}

	if (TActorIterator<AActor> It(World, TownCenterClass); It)
	{
		UEnvQueryItemType_Point::SetContextHelper(ContextData, It->GetActorLocation());
	}
}

void UCropoutCollectionTargetEqsContext::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	AActor* QuerierActor = Cast<AActor>(QueryInstance.Owner.Get());
	if (!QuerierActor)
	{
		return;
	}

	const AAIController* AIController = UAIBlueprintHelperLibrary::GetAIController(QuerierActor);
	const UBlackboardComponent* BlackboardComponent = AIController ? AIController->GetBlackboardComponent() : nullptr;
	UObject* TargetObject = BlackboardComponent ? BlackboardComponent->GetValueAsObject(KeyName) : nullptr;

	if (const AActor* TargetActor = Cast<AActor>(TargetObject))
	{
		UEnvQueryItemType_Point::SetContextHelper(ContextData, TargetActor->GetActorLocation());
	}
}
