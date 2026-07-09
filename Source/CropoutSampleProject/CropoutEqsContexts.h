// Copyright (c) Cropout contributors. Licensed under the project license.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "CropoutEqsContexts.generated.h"

class AActor;

/**
 * Native parent for EQC_TargetTownHall.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API UCropoutTargetTownHallEqsContext : public UEnvQueryContext
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cropout|EQS")
	TSubclassOf<AActor> TownCenterClass;

	virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;
};

/**
 * Native parent for EQC_CollectionTarget.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API UCropoutCollectionTargetEqsContext : public UEnvQueryContext
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cropout|EQS")
	FName KeyName = TEXT("Target");

	virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;
};
