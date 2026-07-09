// Copyright (c) Cropout contributors. Licensed under the project license.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "CropoutBlueprintInterfaceRemoveCommandlet.generated.h"

UCLASS()
class UCropoutBlueprintInterfaceRemoveCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	virtual int32 Main(const FString& Params) override;
};
