// Copyright (c) Cropout contributors. Licensed under the project license.

#pragma once

#include "CoreMinimal.h"
#include "CommonBorder.h"
#include "CommonButtonBase.h"
#include "CommonInputBaseTypes.h"
#include "CommonTextBlock.h"
#include "CropoutCommonStyleData.generated.h"

/** Native parent for CUI_BaseControllerData. */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API UCropoutCommonInputBaseControllerData : public UCommonInputBaseControllerData
{
	GENERATED_BODY()
};

/** Native parent for CUI_InputData. */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API UCropoutCommonUIInputData : public UCommonUIInputData
{
	GENERATED_BODY()
};

/** Native parent for CommonUI border style assets. */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API UCropoutCommonBorderStyle : public UCommonBorderStyle
{
	GENERATED_BODY()
};

/** Native parent for CommonUI button style assets. */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API UCropoutCommonButtonStyle : public UCommonButtonStyle
{
	GENERATED_BODY()
};

/** Native parent for CommonUI text style assets. */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API UCropoutCommonTextStyle : public UCommonTextStyle
{
	GENERATED_BODY()
};
