// Copyright (c) Cropout contributors. Licensed under the project license.

#include "CropoutBlueprintLibrary.h"

#include "CropoutGameInstance.h"
#include "CropoutGameInstanceInterface.h"
#include "CropoutGameMode.h"
#include "CropoutResource.h"
#include "CropoutVillager.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"
#include "GameFramework/GameModeBase.h"
#include "UObject/UnrealType.h"

namespace
{
	bool ReadByteIntMapProperty(const UObject* Object, FName PropertyName, TMap<uint8, int32>& OutMap)
	{
		OutMap.Reset();
		if (!Object)
		{
			return false;
		}

		const FMapProperty* MapProperty = CastField<FMapProperty>(Object->GetClass()->FindPropertyByName(PropertyName));
		if (!MapProperty)
		{
			return false;
		}

		const FNumericProperty* KeyNumericProperty = CastField<FNumericProperty>(MapProperty->KeyProp);
		if (const FEnumProperty* KeyEnumProperty = CastField<FEnumProperty>(MapProperty->KeyProp))
		{
			KeyNumericProperty = KeyEnumProperty->GetUnderlyingProperty();
		}
		const FNumericProperty* ValueNumericProperty = CastField<FNumericProperty>(MapProperty->ValueProp);
		if (!KeyNumericProperty || !ValueNumericProperty)
		{
			return false;
		}

		const void* MapPtr = MapProperty->ContainerPtrToValuePtr<void>(Object);
		FScriptMapHelper Helper(MapProperty, MapPtr);
		for (int32 Index = 0; Index < Helper.GetMaxIndex(); ++Index)
		{
			if (!Helper.IsValidIndex(Index))
			{
				continue;
			}

			const uint8 Key = static_cast<uint8>(KeyNumericProperty->GetUnsignedIntPropertyValue(Helper.GetKeyPtr(Index)));
			const int32 Value = static_cast<int32>(ValueNumericProperty->GetSignedIntPropertyValue(Helper.GetValuePtr(Index)));
			OutMap.Add(Key, Value);
		}
		return true;
	}
}

FVector UCropoutBlueprintLibrary::SteppedPosition(FVector Position)
{
	constexpr float GridSize = 200.0f;
	return FVector(
		FMath::RoundToFloat(Position.X / GridSize) * GridSize,
		FMath::RoundToFloat(Position.Y / GridSize) * GridSize,
		0.0f);
}

FVector UCropoutBlueprintLibrary::ConvertToSteppedPos(FVector Position)
{
	return SteppedPosition(Position);
}

UCropoutGameInstance* UCropoutBlueprintLibrary::GetCropoutGI(const UObject* WorldContextObject)
{
	// BP DSL (BPF_Cropout.Get Cropout GI):
	//   GetGameInstance(WorldContext)
	//   K2Node_DynamicCast -> BP_GI (input: WorldContext + return of GetGameInstance)
	//   return AsBP_GI (or null on bSuccess=false)
	if (!WorldContextObject)
	{
		return nullptr;
	}
	UGameInstance* GI = UGameplayStatics::GetGameInstance(WorldContextObject);
	return Cast<UCropoutGameInstance>(GI);
}

ACropoutGameMode* UCropoutBlueprintLibrary::GetCropoutGM(const UObject* WorldContextObject)
{
	// BP DSL (BPF_Cropout.Get Cropout GM):
	//   GetGameMode(WorldContext)
	//   K2Node_DynamicCast -> BP_GM
	//   return AsBP_GM (or null on bSuccess=false)
	if (!WorldContextObject)
	{
		return nullptr;
	}
	AGameModeBase* GM = UGameplayStatics::GetGameMode(WorldContextObject);
	return Cast<ACropoutGameMode>(GM);
}

FRandomStream UCropoutBlueprintLibrary::GetCropoutIslandSeedStream(const UObject* WorldContextObject)
{
	if (const UCropoutGameInstance* CropoutGI = GetCropoutGI(WorldContextObject))
	{
		return FRandomStream(CropoutGI->TargetSeed);
	}
	return FRandomStream(0);
}

void UCropoutBlueprintLibrary::NotifyIslandGenComplete(const UObject* WorldContextObject)
{
	if (ACropoutGameMode* CropoutGM = GetCropoutGM(WorldContextObject))
	{
		CropoutGM->EventIslandGenComplete();
	}
}

void UCropoutBlueprintLibrary::NotifyRemoveCurrentUILayer(const UObject* WorldContextObject)
{
	if (ACropoutGameMode* CropoutGM = GetCropoutGM(WorldContextObject))
	{
		CropoutGM->EventRemoveCurrentUILayer();
	}
}

void UCropoutBlueprintLibrary::NotifyUpdateAllInteractables(const UObject* WorldContextObject)
{
	UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContextObject);
	if (GameInstance && GameInstance->Implements<UCropoutGameInstanceInterface>())
	{
		ICropoutGameInstanceInterface::Execute_UpdateAllInteractables(GameInstance);
	}
}

void UCropoutBlueprintLibrary::AddGameModeResource(const UObject* WorldContextObject, uint8 Resource, int32 Value)
{
	if (ACropoutGameMode* CropoutGM = GetCropoutGM(WorldContextObject))
	{
		CropoutGM->AddResource(static_cast<int32>(Resource), Value);
	}
}

void UCropoutBlueprintLibrary::RemoveGameModeResource(const UObject* WorldContextObject, uint8 Resource, int32 Amount)
{
	if (ACropoutGameMode* CropoutGM = GetCropoutGM(WorldContextObject))
	{
		CropoutGM->RemoveResource_Implementation(static_cast<ECropoutResourceType>(Resource), Amount);
	}
}

bool UCropoutBlueprintLibrary::CheckGameModeResource(const UObject* WorldContextObject, uint8 Resource, int32& OutAvailableCount)
{
	OutAvailableCount = 0;
	if (const ACropoutGameMode* CropoutGM = GetCropoutGM(WorldContextObject))
	{
		return CropoutGM->CheckResource_Implementation(static_cast<ECropoutResourceType>(Resource), OutAvailableCount);
	}
	return false;
}

bool UCropoutBlueprintLibrary::ApplyGameModeBuildResourceCost(const UObject* WorldContextObject, const UObject* ResourceCostOwner)
{
	ACropoutGameMode* CropoutGM = GetCropoutGM(WorldContextObject);
	if (!CropoutGM)
	{
		return false;
	}

	TMap<uint8, int32> ResourceCost;
	if (!ReadByteIntMapProperty(ResourceCostOwner, TEXT("ResourceCost"), ResourceCost))
	{
		return false;
	}

	for (const TPair<uint8, int32>& Entry : ResourceCost)
	{
		CropoutGM->RemoveResource_Implementation(static_cast<ECropoutResourceType>(Entry.Key), Entry.Value);
	}

	for (const TPair<uint8, int32>& Entry : ResourceCost)
	{
		int32 AvailableCount = 0;
		CropoutGM->CheckResource_Implementation(static_cast<ECropoutResourceType>(Entry.Key), AvailableCount);
		if (AvailableCount < Entry.Value)
		{
			return false;
		}
	}
	return true;
}

void UCropoutBlueprintLibrary::CollectActorResource(AActor* Target, uint8& Resource, int32& Value)
{
	Resource = static_cast<uint8>(ECropoutResourceType::None);
	Value = 0;

	if (ACropoutResource* CropoutResource = Cast<ACropoutResource>(Target))
	{
		ECropoutResourceType ResourceType = ECropoutResourceType::None;
		CropoutResource->CollectResource(ResourceType, Value);
		Resource = static_cast<uint8>(ResourceType);
		return;
	}

	if (ACropoutVillager* CropoutVillager = Cast<ACropoutVillager>(Target))
	{
		CropoutVillager->TakeHeldResource(Resource, Value);
	}
}

void UCropoutBlueprintLibrary::AddActorResource(AActor* Target, uint8 Resource, int32 Value)
{
	if (!Target)
	{
		return;
	}

	if (ACropoutGameMode* CropoutGM = Cast<ACropoutGameMode>(Target))
	{
		CropoutGM->AddResource(static_cast<int32>(Resource), Value);
		return;
	}

	if (ACropoutVillager* CropoutVillager = Cast<ACropoutVillager>(Target))
	{
		CropoutVillager->AddResource(Resource, Value);
		return;
	}

	if (UFunction* AddResourceFunction = Target->GetClass()->FindFunctionByName(TEXT("AddResource")))
	{
		struct FAddResourceParams
		{
			uint8 Resource = 0;
			int32 Value = 0;
		};

		FAddResourceParams Params;
		Params.Resource = Resource;
		Params.Value = Value;
		Target->ProcessEvent(AddResourceFunction, &Params);
	}
}

void UCropoutBlueprintLibrary::ReturnVillagerToDefaultBT(AActor* Target)
{
	if (ACropoutVillager* CropoutVillager = Cast<ACropoutVillager>(Target))
	{
		CropoutVillager->ReturnToDefaultBT();
		return;
	}

	if (Target)
	{
		if (UFunction* Function = Target->GetClass()->FindFunctionByName(TEXT("ReturnToDefaultBT")))
		{
			Target->ProcessEvent(Function, nullptr);
		}
	}
}

double UCropoutBlueprintLibrary::PlayVillagerDeliverAnim(AActor* Target)
{
	if (ACropoutVillager* CropoutVillager = Cast<ACropoutVillager>(Target))
	{
		return CropoutVillager->PlayDeliverAnim();
	}

	double Delay = 0.0;
	if (Target)
	{
		if (UFunction* Function = Target->GetClass()->FindFunctionByName(TEXT("PlayDeliverAnim")))
		{
			struct FPlayDeliverAnimParams
			{
				double Delay = 0.0;
			};

			FPlayDeliverAnimParams Params;
			Target->ProcessEvent(Function, &Params);
			Delay = Params.Delay;
		}
	}
	return Delay;
}

void UCropoutBlueprintLibrary::PlayVillagerWorkAnim(AActor* Target, double Delay)
{
	if (ACropoutVillager* CropoutVillager = Cast<ACropoutVillager>(Target))
	{
		CropoutVillager->PlayWorkAnim(Delay);
		return;
	}

	if (Target)
	{
		if (UFunction* Function = Target->GetClass()->FindFunctionByName(TEXT("PlayWorkAnim")))
		{
			struct FPlayWorkAnimParams
			{
				double Delay = 0.0;
			};

			FPlayWorkAnimParams Params;
			Params.Delay = Delay;
			Target->ProcessEvent(Function, &Params);
		}
	}
}
