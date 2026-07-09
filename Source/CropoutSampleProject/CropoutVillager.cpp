// Copyright (c) Cropout contributors. Licensed under the project license.

#include "CropoutVillager.h"

#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CropoutBlueprintLibrary.h"
#include "CropoutGameInstanceInterface.h"
#include "Engine/DataTable.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TimerManager.h"
#include "UObject/SoftObjectPtr.h"
#include "UObject/UnrealType.h"

namespace
{
	FString NormalisePropertyName(const FString& Name)
	{
		FString Result;
		Result.Reserve(Name.Len());
		for (TCHAR Character : Name)
		{
			if (Character != TEXT(' ') && Character != TEXT('_'))
			{
				Result.AppendChar(FChar::ToLower(Character));
			}
		}
		return Result;
	}

	FProperty* FindPropertyByCandidates(const UStruct* Struct, std::initializer_list<const TCHAR*> Names)
	{
		if (!Struct)
		{
			return nullptr;
		}

		TArray<FString> NormalisedNames;
		for (const TCHAR* Name : Names)
		{
			NormalisedNames.Add(NormalisePropertyName(Name));
		}

		for (TFieldIterator<FProperty> It(Struct); It; ++It)
		{
			FProperty* Property = *It;
			const FString NormalisedPropertyName = NormalisePropertyName(Property->GetName());
			for (const FString& NormalisedName : NormalisedNames)
			{
				if (NormalisedPropertyName == NormalisedName || NormalisedPropertyName.StartsWith(NormalisedName))
				{
					return Property;
				}
			}
		}

		return nullptr;
	}

	FProperty* FindBlueprintProperty(const UObject* Object, std::initializer_list<const TCHAR*> Names)
	{
		return Object ? FindPropertyByCandidates(Object->GetClass(), Names) : nullptr;
	}

	template <typename ComponentType>
	ComponentType* FindNamedComponent(const AActor* Actor, FName ComponentName)
	{
		if (!Actor)
		{
			return nullptr;
		}

		TArray<ComponentType*> Components;
		Actor->GetComponents<ComponentType>(Components);
		for (ComponentType* Component : Components)
		{
			if (Component && Component->GetFName() == ComponentName)
			{
				return Component;
			}
		}
		return nullptr;
	}

	FName GetNamePropertyValue(const UObject* Object, std::initializer_list<const TCHAR*> Names, FName DefaultValue)
	{
		const FNameProperty* NameProperty = CastField<FNameProperty>(FindBlueprintProperty(Object, Names));
		return NameProperty ? NameProperty->GetPropertyValue_InContainer(Object) : DefaultValue;
	}

	void SetNamePropertyValue(UObject* Object, std::initializer_list<const TCHAR*> Names, FName Value)
	{
		if (FNameProperty* NameProperty = CastField<FNameProperty>(FindBlueprintProperty(Object, Names)))
		{
			NameProperty->SetPropertyValue_InContainer(Object, Value);
		}
	}

	int32 GetIntPropertyValue(const UObject* Object, std::initializer_list<const TCHAR*> Names, int32 DefaultValue)
	{
		const FProperty* Property = FindBlueprintProperty(Object, Names);
		if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
		{
			const FNumericProperty* UnderlyingProperty = EnumProperty->GetUnderlyingProperty();
			return static_cast<int32>(UnderlyingProperty->GetSignedIntPropertyValue(UnderlyingProperty->ContainerPtrToValuePtr<void>(Object)));
		}
		if (const FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property))
		{
			return static_cast<int32>(NumericProperty->GetSignedIntPropertyValue(NumericProperty->ContainerPtrToValuePtr<void>(Object)));
		}
		return DefaultValue;
	}

	void SetIntPropertyValue(UObject* Object, std::initializer_list<const TCHAR*> Names, int32 Value)
	{
		FProperty* Property = FindBlueprintProperty(Object, Names);
		if (FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
		{
			FNumericProperty* UnderlyingProperty = EnumProperty->GetUnderlyingProperty();
			UnderlyingProperty->SetIntPropertyValue(UnderlyingProperty->ContainerPtrToValuePtr<void>(Object), static_cast<int64>(Value));
		}
		else if (FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property))
		{
			NumericProperty->SetIntPropertyValue(NumericProperty->ContainerPtrToValuePtr<void>(Object), static_cast<int64>(Value));
		}
	}

	UObject* GetObjectPropertyValue(const UObject* Object, std::initializer_list<const TCHAR*> Names)
	{
		const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(FindBlueprintProperty(Object, Names));
		return ObjectProperty ? ObjectProperty->GetObjectPropertyValue_InContainer(Object) : nullptr;
	}

	void SetObjectPropertyValue(UObject* Object, std::initializer_list<const TCHAR*> Names, UObject* Value)
	{
		if (FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(FindBlueprintProperty(Object, Names)))
		{
			ObjectProperty->SetObjectPropertyValue_InContainer(Object, Value);
		}
	}

	UDataTable* GetJobsTable()
	{
		static const TSoftObjectPtr<UDataTable> JobsTable(FSoftObjectPath(TEXT("/Game/Blueprint/Villagers/DT_Jobs.DT_Jobs")));
		return JobsTable.LoadSynchronous();
	}

	template <typename AssetType>
	AssetType* LoadSoftRowAsset(const UDataTable* DataTable, const uint8* RowData, std::initializer_list<const TCHAR*> FieldNames)
	{
		if (!DataTable || !RowData)
		{
			return nullptr;
		}

		FProperty* Property = FindPropertyByCandidates(DataTable->GetRowStruct(), FieldNames);
		if (!Property)
		{
			return nullptr;
		}

		const void* ValuePtr = Property->ContainerPtrToValuePtr<void>(RowData);
		if (const FSoftObjectProperty* SoftObjectProperty = CastField<FSoftObjectProperty>(Property))
		{
			const FSoftObjectPtr SoftObjectPtr = SoftObjectProperty->GetPropertyValue(ValuePtr);
			return Cast<AssetType>(SoftObjectPtr.LoadSynchronous());
		}
		if (const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(Property))
		{
			return Cast<AssetType>(ObjectProperty->GetObjectPropertyValue(ValuePtr));
		}
		return nullptr;
	}
}

ACropoutVillager::ACropoutVillager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ACropoutVillager::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (USkeletalMeshComponent* MeshComponent = GetVillagerMesh())
	{
		MeshComponent->SetCustomPrimitiveDataFloat(0, FMath::FRand());
		MeshComponent->SetCustomPrimitiveDataFloat(1, FMath::FRand());
	}
}

void ACropoutVillager::BeginPlay()
{
	Super::BeginPlay();

	if (UCapsuleComponent* CapsuleComponent = GetCapsuleComponent())
	{
		AddActorWorldOffset(FVector(0.0, 0.0, CapsuleComponent->GetScaledCapsuleHalfHeight()));
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(EatTimerHandle, this, &ThisClass::Eat, 24.0f, true);
	}

	ChangeJob(TEXT("Idle"));

	if (USkeletalMeshComponent* HairComponent = GetHairComponent())
	{
		if (USkeletalMesh* HairMesh = HairPick().LoadSynchronous())
		{
			HairComponent->SetSkeletalMeshAsset(HairMesh);
		}
		HairComponent->SetCustomPrimitiveDataFloat(0, FMath::FRand());
	}
}

void ACropoutVillager::Eat()
{
	UCropoutBlueprintLibrary::RemoveGameModeResource(this, static_cast<uint8>(ECropoutResourceType::Wood), 3);
}

void ACropoutVillager::ResetJobState()
{
	StopJob();

	if (USkeletalMeshComponent* HatComponent = GetHatComponent())
	{
		HatComponent->SetSkeletalMeshAsset(nullptr);
		HatComponent->SetVisibility(false);
	}

	if (UStaticMeshComponent* ToolComponent = GetToolComponent())
	{
		ToolComponent->SetStaticMesh(nullptr);
	}

	SetTargetToolValue(nullptr);
}

void ACropoutVillager::StopJob()
{
	if (UStaticMeshComponent* ToolComponent = GetToolComponent())
	{
		ToolComponent->SetVisibility(false);
	}

	if (USkeletalMeshComponent* MeshComponent = GetVillagerMesh())
	{
		if (UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance())
		{
			AnimInstance->Montage_StopGroupByName(0.0f, TEXT("DefaultGroup"));
		}
	}

	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		AIController->StopMovement();
	}

	SetQuantityValue(0);
}

TSoftObjectPtr<USkeletalMesh> ACropoutVillager::HairPick() const
{
	static const TArray<TSoftObjectPtr<USkeletalMesh>> HairMeshes = {
		TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/Characters/Meshes/Hair/SKM_Hair01.SKM_Hair01"))),
		TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/Characters/Meshes/Hair/SKM_Hair02.SKM_Hair02"))),
		TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/Characters/Meshes/Hair/SKM_Hair03.SKM_Hair03"))),
		TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/Characters/Meshes/Hair/SKM_Hair04.SKM_Hair04"))),
		TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/Characters/Meshes/Hair/SKM_Hair05.SKM_Hair05"))),
		TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(TEXT("/Game/Characters/Meshes/Hair/SKM_Hair06.SKM_Hair06"))),
	};

	return HairMeshes[FMath::RandRange(0, HairMeshes.Num() - 1)];
}

void ACropoutVillager::ChangeJob(FName NewJob)
{
	SetNewJobValue(NewJob);

	UDataTable* JobsTable = GetJobsTable();
	const uint8* RowData = JobsTable ? JobsTable->FindRowUnchecked(GetNewJobValue()) : nullptr;
	if (!JobsTable || !RowData)
	{
		UKismetSystemLibrary::PrintString(this, TEXT("ERROR: Failed to Load Job"), true, true, FLinearColor::Red, 2.0f);
		return;
	}

	if (Tags.Contains(GetNewJobValue()))
	{
		return;
	}

	Tags.Reset();
	Tags.Add(GetNewJobValue());
	ResetJobState();

	UBehaviorTree* BehaviorTree = LoadSoftRowAsset<UBehaviorTree>(JobsTable, RowData, { TEXT("BehaviourTree"), TEXT("BehaviorTree") });
	if (BehaviorTree)
	{
		if (AAIController* AIController = Cast<AAIController>(GetController()))
		{
			AIController->RunBehaviorTree(BehaviorTree);
		}
		SetActiveBehaviorValue(BehaviorTree);
		if (AActor* TargetRef = GetTargetRefValue())
		{
			SetBlackboardTargetIfAvailable(TargetRef);
		}
	}

	SetWorkAnimValue(LoadSoftRowAsset<UAnimMontage>(JobsTable, RowData, { TEXT("WorkAnim"), TEXT("Work Anim") }));

	if (USkeletalMesh* HatMesh = LoadSoftRowAsset<USkeletalMesh>(JobsTable, RowData, { TEXT("Hat") }))
	{
		if (USkeletalMeshComponent* HatComponent = GetHatComponent())
		{
			HatComponent->SetSkeletalMeshAsset(HatMesh);
			HatComponent->SetVisibility(true);
		}
	}

	SetTargetToolValue(LoadSoftRowAsset<UStaticMesh>(JobsTable, RowData, { TEXT("Tool") }));
}

void ACropoutVillager::Action(AActor* NewParam)
{
	if (!IsValid(NewParam))
	{
		return;
	}

	SetTargetRefValue(NewParam);
	if (NewParam->Tags.IsValidIndex(0))
	{
		ChangeJob(NewParam->Tags[0]);
		return;
	}

	if (UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this))
	{
		if (GameInstance->GetClass()->ImplementsInterface(UCropoutGameInstanceInterface::StaticClass()))
		{
			ICropoutGameInstanceInterface::Execute_UpdateAllVillagers(GameInstance);
		}
	}
}

void ACropoutVillager::ReturnToDefaultBT()
{
	ChangeJob(TEXT("Idle"));
}

void ACropoutVillager::ReturntoIdle()
{
	ChangeJob(TEXT("Idle"));
}

void ACropoutVillager::PlayWorkAnim(double Delay)
{
	PlayVillagerAnim(GetWorkAnimValue(), Delay);

	if (UStaticMeshComponent* ToolComponent = GetToolComponent())
	{
		ToolComponent->SetStaticMesh(GetTargetToolValue());
		ToolComponent->SetVisibility(true);
	}
}

double ACropoutVillager::PlayDeliverAnim()
{
	static const TSoftObjectPtr<UAnimMontage> PutDownMontage(FSoftObjectPath(TEXT("/Game/Characters/Animations/AM_PutDown.AM_PutDown")));
	PlayVillagerAnim(PutDownMontage.LoadSynchronous(), 1.0);
	return 1.0;
}

double ACropoutVillager::ProgressBuilding(double /*InvestedTime*/)
{
	return 0.0;
}

void ACropoutVillager::AddResource(uint8 Resource, int32 Value)
{
	static const TSoftObjectPtr<UStaticMesh> CrateMesh(FSoftObjectPath(TEXT("/Game/Characters/Meshes/Tools/Crate.Crate")));

	SetResourcesHeldValue(static_cast<ECropoutResourceType>(Resource));
	SetQuantityValue(Value);

	if (UStaticMeshComponent* ToolComponent = GetToolComponent())
	{
		ToolComponent->SetVisibility(true);
		ToolComponent->SetStaticMesh(CrateMesh.LoadSynchronous());
	}
}

void ACropoutVillager::TakeHeldResource(uint8& Resource, int32& Value)
{
	SetCacheResourceValue(GetResourcesHeldValue());
	SetCacheValueValue(GetQuantityValue());

	SetResourcesHeldValue(ECropoutResourceType::None);
	SetQuantityValue(0);

	if (UStaticMeshComponent* ToolComponent = GetToolComponent())
	{
		ToolComponent->SetVisibility(false);
		ToolComponent->SetStaticMesh(nullptr);
	}

	Resource = static_cast<uint8>(GetCacheResourceValue());
	Value = GetCacheValueValue();
}

USkeletalMeshComponent* ACropoutVillager::GetVillagerMesh() const
{
	return FindNamedComponent<USkeletalMeshComponent>(this, TEXT("SkeletalMesh"));
}

USkeletalMeshComponent* ACropoutVillager::GetHairComponent() const
{
	return FindNamedComponent<USkeletalMeshComponent>(this, TEXT("Hair"));
}

USkeletalMeshComponent* ACropoutVillager::GetHatComponent() const
{
	return FindNamedComponent<USkeletalMeshComponent>(this, TEXT("Hat"));
}

UStaticMeshComponent* ACropoutVillager::GetToolComponent() const
{
	return FindNamedComponent<UStaticMeshComponent>(this, TEXT("Tool"));
}

UCapsuleComponent* ACropoutVillager::GetCapsuleComponent() const
{
	return FindNamedComponent<UCapsuleComponent>(this, TEXT("Capsule"));
}

void ACropoutVillager::PlayVillagerAnim(UAnimMontage* Montage, double Length)
{
	if (Montage)
	{
		if (USkeletalMeshComponent* MeshComponent = GetVillagerMesh())
		{
			if (UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance())
			{
				AnimInstance->Montage_Play(Montage);
			}
		}
	}

	if (UWorld* World = GetWorld())
	{
		const float Delay = FMath::Max(0.0f, static_cast<float>(Length));
		World->GetTimerManager().SetTimer(StopMontageTimerHandle, this, &ThisClass::StopVillagerMontageAndHideTool, Delay, false);
	}
}

void ACropoutVillager::StopVillagerMontageAndHideTool()
{
	if (USkeletalMeshComponent* MeshComponent = GetVillagerMesh())
	{
		if (UAnimInstance* AnimInstance = MeshComponent->GetAnimInstance())
		{
			AnimInstance->Montage_StopGroupByName(0.0f, TEXT("DefaultGroup"));
		}
	}

	if (UStaticMeshComponent* ToolComponent = GetToolComponent())
	{
		ToolComponent->SetVisibility(false);
	}
}

void ACropoutVillager::SetBlackboardTargetIfAvailable(AActor* TargetActor)
{
	AAIController* AIController = Cast<AAIController>(GetController());
	UBlackboardComponent* BlackboardComponent = AIController ? AIController->GetBlackboardComponent() : nullptr;
	if (BlackboardComponent && TargetActor)
	{
		BlackboardComponent->SetValueAsObject(TEXT("Target"), TargetActor);
	}
}

FName ACropoutVillager::GetNewJobValue() const
{
	return GetNamePropertyValue(this, { TEXT("NewJob"), TEXT("New Job"), TEXT("NativeNewJob") }, NativeNewJob);
}

void ACropoutVillager::SetNewJobValue(FName Value)
{
	NativeNewJob = Value;
	SetNamePropertyValue(this, { TEXT("NewJob"), TEXT("New Job"), TEXT("NativeNewJob") }, Value);
}

AActor* ACropoutVillager::GetTargetRefValue() const
{
	return Cast<AActor>(GetObjectPropertyValue(this, { TEXT("TargetRef"), TEXT("Target Ref"), TEXT("NativeTargetRef") }))
		? Cast<AActor>(GetObjectPropertyValue(this, { TEXT("TargetRef"), TEXT("Target Ref"), TEXT("NativeTargetRef") }))
		: NativeTargetRef.Get();
}

void ACropoutVillager::SetTargetRefValue(AActor* Value)
{
	NativeTargetRef = Value;
	SetObjectPropertyValue(this, { TEXT("TargetRef"), TEXT("Target Ref"), TEXT("NativeTargetRef") }, Value);
}

UAnimMontage* ACropoutVillager::GetWorkAnimValue() const
{
	return Cast<UAnimMontage>(GetObjectPropertyValue(this, { TEXT("WorkAnim"), TEXT("Work Anim"), TEXT("NativeWorkAnim") }))
		? Cast<UAnimMontage>(GetObjectPropertyValue(this, { TEXT("WorkAnim"), TEXT("Work Anim"), TEXT("NativeWorkAnim") }))
		: NativeWorkAnim.Get();
}

void ACropoutVillager::SetWorkAnimValue(UAnimMontage* Value)
{
	NativeWorkAnim = Value;
	SetObjectPropertyValue(this, { TEXT("WorkAnim"), TEXT("Work Anim"), TEXT("NativeWorkAnim") }, Value);
}

UStaticMesh* ACropoutVillager::GetTargetToolValue() const
{
	return Cast<UStaticMesh>(GetObjectPropertyValue(this, { TEXT("TargetTool"), TEXT("Target Tool"), TEXT("NativeTargetTool") }))
		? Cast<UStaticMesh>(GetObjectPropertyValue(this, { TEXT("TargetTool"), TEXT("Target Tool"), TEXT("NativeTargetTool") }))
		: NativeTargetTool.Get();
}

void ACropoutVillager::SetTargetToolValue(UStaticMesh* Value)
{
	NativeTargetTool = Value;
	SetObjectPropertyValue(this, { TEXT("TargetTool"), TEXT("Target Tool"), TEXT("NativeTargetTool") }, Value);
}

void ACropoutVillager::SetActiveBehaviorValue(UBehaviorTree* Value)
{
	NativeActiveBehavior = Value;
	SetObjectPropertyValue(this, { TEXT("ActiveBehavior"), TEXT("Active Behavior"), TEXT("NativeActiveBehavior") }, Value);
}

ECropoutResourceType ACropoutVillager::GetResourcesHeldValue() const
{
	return static_cast<ECropoutResourceType>(GetIntPropertyValue(
		this,
		{ TEXT("ResourcesHeld"), TEXT("Resources Held"), TEXT("NativeResourcesHeld") },
		static_cast<int32>(NativeResourcesHeld)));
}

void ACropoutVillager::SetResourcesHeldValue(ECropoutResourceType Value)
{
	NativeResourcesHeld = Value;
	SetIntPropertyValue(this, { TEXT("ResourcesHeld"), TEXT("Resources Held"), TEXT("NativeResourcesHeld") }, static_cast<int32>(Value));
}

int32 ACropoutVillager::GetQuantityValue() const
{
	return GetIntPropertyValue(this, { TEXT("Quantity"), TEXT("NativeQuantity") }, NativeQuantity);
}

void ACropoutVillager::SetQuantityValue(int32 Value)
{
	NativeQuantity = Value;
	SetIntPropertyValue(this, { TEXT("Quantity"), TEXT("NativeQuantity") }, Value);
}

void ACropoutVillager::SetCacheResourceValue(ECropoutResourceType Value)
{
	NativeCacheResource = Value;
	SetIntPropertyValue(this, { TEXT("CacheResource"), TEXT("Cache Resource"), TEXT("NativeCacheResource") }, static_cast<int32>(Value));
}

ECropoutResourceType ACropoutVillager::GetCacheResourceValue() const
{
	return static_cast<ECropoutResourceType>(GetIntPropertyValue(
		this,
		{ TEXT("CacheResource"), TEXT("Cache Resource"), TEXT("NativeCacheResource") },
		static_cast<int32>(NativeCacheResource)));
}

void ACropoutVillager::SetCacheValueValue(int32 Value)
{
	NativeCacheValue = Value;
	SetIntPropertyValue(this, { TEXT("CacheValue"), TEXT("Cache Value"), TEXT("NativeCacheValue") }, Value);
}

int32 ACropoutVillager::GetCacheValueValue() const
{
	return GetIntPropertyValue(this, { TEXT("CacheValue"), TEXT("Cache Value"), TEXT("NativeCacheValue") }, NativeCacheValue);
}
