// Copyright (c) Cropout contributors. Licensed under the project license.

#include "CropoutInteractable.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Canvas.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialInterface.h"
#include "TimerManager.h"
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

	FProperty* FindBlueprintProperty(const UObject* Object, std::initializer_list<const TCHAR*> Names)
	{
		if (!Object)
		{
			return nullptr;
		}

		TArray<FString> NormalisedNames;
		for (const TCHAR* Name : Names)
		{
			NormalisedNames.Add(NormalisePropertyName(Name));
		}

		for (TFieldIterator<FProperty> It(Object->GetClass()); It; ++It)
		{
			FProperty* Property = *It;
			const FString PropertyName = Property->GetName();
			const FString NormalisedPropertyName = NormalisePropertyName(PropertyName);

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

	template <typename ComponentType>
	ComponentType* FindNamedOrFirstComponent(const AActor* Actor, FName ComponentName)
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

		return Components.Num() > 0 ? Components[0] : nullptr;
	}
}

ACropoutInteractable::ACropoutInteractable()
{
	PrimaryActorTick.bCanEverTick = false;

	NativeScene = CreateDefaultSubobject<USceneComponent>(TEXT("Scene"));
	SetRootComponent(NativeScene);

	NativeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	NativeMesh->SetupAttachment(NativeScene);

	NativeBox = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	NativeBox->SetupAttachment(NativeScene);
	NativeBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	NativeBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	NativeBox->SetGenerateOverlapEvents(true);
}

void ACropoutInteractable::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	UStaticMeshComponent* MeshComponent = GetMeshComponent();
	UBoxComponent* BoxComponent = GetBoxComponent();
	if (!MeshComponent || !BoxComponent)
	{
		return;
	}

	FVector LocalMin = FVector::ZeroVector;
	FVector LocalMax = FVector::ZeroVector;
	MeshComponent->GetLocalBounds(LocalMin, LocalMax);

	const FVector LocalSize = LocalMax - LocalMin;
	const float Step = GetFloatPropertyValue({ TEXT("Step") }, 100.0f);
	const float BoundGapValue = GetFloatPropertyValue({ TEXT("BoundGap"), TEXT("Bound Gap") }, 0.0f);

	const float RoundedX = FMath::RoundToFloat(LocalSize.X / 100.0f) * Step;
	const float RoundedY = FMath::RoundToFloat(LocalSize.Y / 100.0f) * Step;
	const float RoundedZ = FMath::RoundToFloat(LocalSize.Z / 100.0f) * Step;
	const float HorizontalExtent = FMath::Max(FMath::Max(RoundedX, RoundedY), 100.0f);
	const FVector Extent(
		HorizontalExtent,
		HorizontalExtent,
		FMath::Max(RoundedZ, 100.0f));

	BoxComponent->SetBoxExtent(Extent + FVector(BoundGapValue * 100.0f));
	BoxComponent->SetWorldRotation(FRotationMatrix::MakeFromX(FVector(1.0, 0.0, 0.0)).Rotator());
}

void ACropoutInteractable::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(this, &ThisClass::HandleBeginPlayNextTick);
	}
}

void ACropoutInteractable::PlacementMode_Implementation()
{
	SetBoolPropertyValue({ TEXT("EnableGroundBlend"), TEXT("Enable Ground Blend") }, false);

	UStaticMeshComponent* MeshComponent = GetMeshComponent();
	const TArray<UStaticMesh*> CurrentMeshList = GetStaticMeshArrayPropertyValue({ TEXT("MeshList"), TEXT("Mesh List") });
	if (MeshComponent && CurrentMeshList.Num() > 0)
	{
		MeshComponent->SetStaticMesh(CurrentMeshList[0]);
	}

	Tags.Reset();
	Tags.Add(TEXT("PlacementMode"));
}

void ACropoutInteractable::SetProgressionsState_Implementation(float Progression)
{
	const float CurrentProgressionState = GetFloatPropertyValue({ TEXT("ProgressionState"), TEXT("Progression State") }, 0.0f);
	const int32 MeshIndex = FMath::FloorToInt(CurrentProgressionState);
	const TArray<UStaticMesh*> CurrentMeshList = GetStaticMeshArrayPropertyValue({ TEXT("MeshList"), TEXT("Mesh List") });
	UStaticMesh* SelectedMesh = CurrentMeshList.IsValidIndex(MeshIndex) ? CurrentMeshList[MeshIndex] : nullptr;

	SetFloatPropertyValue({ TEXT("ProgressionState"), TEXT("Progression State") }, Progression);
	if (GetBoolPropertyValue({ TEXT("RequireBuild"), TEXT("Require Build") }))
	{
		Tags.Add(TEXT("Build"));
		return;
	}

	if (SelectedMesh)
	{
		if (UStaticMeshComponent* MeshComponent = GetMeshComponent())
		{
			MeshComponent->SetStaticMesh(SelectedMesh);
		}
	}
}

void ACropoutInteractable::Interact_Implementation(float& NewParam)
{
	NewParam = 0.0f;
}

void ACropoutInteractable::PlayWobble(FVector NewParam)
{
	if (UStaticMeshComponent* MeshComponent = GetMeshComponent())
	{
		const FVector Direction = (NewParam - GetActorLocation()).GetSafeNormal();
		MeshComponent->SetVectorParameterValueOnMaterials(TEXT(" Wobble Vector"), Direction);
	}

	TargetWobbleValue = 1.0f;
	StartWobbleTimer();
}

void ACropoutInteractable::EndWobble()
{
	TargetWobbleValue = 0.0f;
	StartWobbleTimer();
}

void ACropoutInteractable::HandleBeginPlayNextTick()
{
	if (GetBoolPropertyValue({ TEXT("EnableGroundBlend"), TEXT("Enable Ground Blend") }))
	{
		DrawGroundBlend();
	}

	DestroyDuplicateOverlaps();
}

void ACropoutInteractable::DrawGroundBlend()
{
	UTextureRenderTarget2D* RenderTarget = Cast<UTextureRenderTarget2D>(
		GetObjectPropertyValue({ TEXT("RT_Draw"), TEXT("RTDraw") }));
	if (!RenderTarget)
	{
		static const TSoftObjectPtr<UTextureRenderTarget2D> DefaultRenderTarget(
			FSoftObjectPath(TEXT("/Game/Blueprint/Core/Extras/RT_GrassMove.RT_GrassMove")));
		RenderTarget = DefaultRenderTarget.LoadSynchronous();
	}
	if (!RenderTarget)
	{
		return;
	}

	static TSoftObjectPtr<UMaterialInterface> ShapeDrawMaterial(
		FSoftObjectPath(TEXT("/Game/Environment/Materials/M_ShapeDraw.M_ShapeDraw")));
	UMaterialInterface* Material = ShapeDrawMaterial.LoadSynchronous();
	if (!Material)
	{
		return;
	}

	UCanvas* Canvas = nullptr;
	FVector2D Size = FVector2D::ZeroVector;
	FDrawToRenderTargetContext Context;
	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, RenderTarget, Canvas, Size, Context);
	if (Canvas)
	{
		FVector2D ScreenPosition = FVector2D::ZeroVector;
		FVector2D ScreenSize = FVector2D::ZeroVector;
		TransformToTexture(Size, ScreenPosition, ScreenSize);
		Canvas->K2_DrawMaterial(
			Material,
			ScreenPosition,
			ScreenSize,
			FVector2D::ZeroVector,
			FVector2D(1.0, 1.0),
			0.0f,
			FVector2D(0.5, 0.5));
	}
	UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, Context);
}

void ACropoutInteractable::DestroyDuplicateOverlaps()
{
	TArray<AActor*> OverlappingActors;
	UClass* InteractableClass = StaticLoadClass(
		AActor::StaticClass(),
		nullptr,
		TEXT("/Game/Blueprint/Interactable/BP_Interactable.BP_Interactable_C"));
	GetOverlappingActors(OverlappingActors, InteractableClass ? InteractableClass : StaticClass());

	for (AActor* Actor : OverlappingActors)
	{
		if (!Actor || Actor == this || Actor->ActorHasTag(TEXT("PlacementMode")))
		{
			continue;
		}

		if (Actor->GetActorLocation().Equals(GetActorLocation(), 5.0))
		{
			Actor->Destroy();
		}
	}
}

UStaticMeshComponent* ACropoutInteractable::GetMeshComponent() const
{
	return FindNamedOrFirstComponent<UStaticMeshComponent>(this, TEXT("Mesh"));
}

UBoxComponent* ACropoutInteractable::GetBoxComponent() const
{
	return FindNamedOrFirstComponent<UBoxComponent>(this, TEXT("Box"));
}

bool ACropoutInteractable::GetBoolPropertyValue(std::initializer_list<const TCHAR*> Names, bool DefaultValue) const
{
	const FBoolProperty* BoolProperty = CastField<FBoolProperty>(FindBlueprintProperty(this, Names));
	return BoolProperty ? BoolProperty->GetPropertyValue_InContainer(this) : DefaultValue;
}

void ACropoutInteractable::SetBoolPropertyValue(std::initializer_list<const TCHAR*> Names, bool Value)
{
	if (FBoolProperty* BoolProperty = CastField<FBoolProperty>(FindBlueprintProperty(this, Names)))
	{
		BoolProperty->SetPropertyValue_InContainer(this, Value);
	}
}

int32 ACropoutInteractable::GetIntPropertyValue(std::initializer_list<const TCHAR*> Names, int32 DefaultValue) const
{
	const FProperty* Property = FindBlueprintProperty(this, Names);
	if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
	{
		const FNumericProperty* UnderlyingProperty = EnumProperty->GetUnderlyingProperty();
		return static_cast<int32>(UnderlyingProperty->GetSignedIntPropertyValue(UnderlyingProperty->ContainerPtrToValuePtr<void>(this)));
	}
	if (const FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property))
	{
		return static_cast<int32>(NumericProperty->GetSignedIntPropertyValue(NumericProperty->ContainerPtrToValuePtr<void>(this)));
	}

	return DefaultValue;
}

void ACropoutInteractable::SetIntPropertyValue(std::initializer_list<const TCHAR*> Names, int32 Value)
{
	FProperty* Property = FindBlueprintProperty(this, Names);
	if (FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
	{
		FNumericProperty* UnderlyingProperty = EnumProperty->GetUnderlyingProperty();
		UnderlyingProperty->SetIntPropertyValue(UnderlyingProperty->ContainerPtrToValuePtr<void>(this), static_cast<int64>(Value));
	}
	else if (FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property))
	{
		NumericProperty->SetIntPropertyValue(NumericProperty->ContainerPtrToValuePtr<void>(this), static_cast<int64>(Value));
	}
}

float ACropoutInteractable::GetFloatPropertyValue(std::initializer_list<const TCHAR*> Names, float DefaultValue) const
{
	const FProperty* Property = FindBlueprintProperty(this, Names);
	if (const FFloatProperty* FloatProperty = CastField<FFloatProperty>(Property))
	{
		return FloatProperty->GetPropertyValue_InContainer(this);
	}
	if (const FDoubleProperty* DoubleProperty = CastField<FDoubleProperty>(Property))
	{
		return static_cast<float>(DoubleProperty->GetPropertyValue_InContainer(this));
	}
	if (const FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property))
	{
		return static_cast<float>(NumericProperty->GetFloatingPointPropertyValue(NumericProperty->ContainerPtrToValuePtr<void>(this)));
	}

	return DefaultValue;
}

void ACropoutInteractable::SetFloatPropertyValue(std::initializer_list<const TCHAR*> Names, float Value)
{
	FProperty* Property = FindBlueprintProperty(this, Names);
	if (FFloatProperty* FloatProperty = CastField<FFloatProperty>(Property))
	{
		FloatProperty->SetPropertyValue_InContainer(this, Value);
	}
	else if (FDoubleProperty* DoubleProperty = CastField<FDoubleProperty>(Property))
	{
		DoubleProperty->SetPropertyValue_InContainer(this, static_cast<double>(Value));
	}
	else if (FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property))
	{
		NumericProperty->SetFloatingPointPropertyValue(NumericProperty->ContainerPtrToValuePtr<void>(this), Value);
	}
}

UObject* ACropoutInteractable::GetObjectPropertyValue(std::initializer_list<const TCHAR*> Names) const
{
	const FObjectPropertyBase* ObjectProperty = CastField<FObjectPropertyBase>(FindBlueprintProperty(this, Names));
	return ObjectProperty ? ObjectProperty->GetObjectPropertyValue_InContainer(this) : nullptr;
}

TArray<UStaticMesh*> ACropoutInteractable::GetStaticMeshArrayPropertyValue(std::initializer_list<const TCHAR*> Names) const
{
	TArray<UStaticMesh*> Result;
	const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(FindBlueprintProperty(this, Names));
	const FObjectPropertyBase* InnerObjectProperty = ArrayProperty ? CastField<FObjectPropertyBase>(ArrayProperty->Inner) : nullptr;
	if (!ArrayProperty || !InnerObjectProperty)
	{
		return Result;
	}

	const void* ArrayPtr = ArrayProperty->ContainerPtrToValuePtr<void>(this);
	FScriptArrayHelper Helper(ArrayProperty, ArrayPtr);
	for (int32 Index = 0; Index < Helper.Num(); ++Index)
	{
		Result.Add(Cast<UStaticMesh>(InnerObjectProperty->GetObjectPropertyValue(Helper.GetRawPtr(Index))));
	}
	return Result;
}

void ACropoutInteractable::TransformToTexture(const FVector2D& InSize, FVector2D& OutPosition, FVector2D& OutSize) const
{
	const FVector TextureSpace = ((GetActorLocation() + FVector(10000.0)) / 20000.0) * InSize.X;

	FVector BoundsOrigin = FVector::ZeroVector;
	FVector BoundsExtent = FVector::ZeroVector;
	GetActorBounds(false, BoundsOrigin, BoundsExtent, false);

	const float BoundsMin = FMath::Min(BoundsExtent.X, BoundsExtent.Y);
	const float DrawSize = (BoundsMin / 10000.0f) * InSize.X * GetFloatPropertyValue({ TEXT("OutlineDraw"), TEXT("Outline Draw") }, 0.0f);

	OutPosition = FVector2D(TextureSpace.X - (DrawSize / 2.0f), TextureSpace.Y - (DrawSize / 2.0f));
	OutSize = FVector2D(DrawSize, DrawSize);
}

void ACropoutInteractable::SetWobbleValue(float Value)
{
	CurrentWobbleValue = Value;
	if (UStaticMeshComponent* MeshComponent = GetMeshComponent())
	{
		MeshComponent->SetScalarParameterValueOnMaterials(TEXT("Wobble"), CurrentWobbleValue);
	}
}

void ACropoutInteractable::UpdateWobble()
{
	const float NewValue = FMath::FInterpTo(CurrentWobbleValue, TargetWobbleValue, 1.0f / 60.0f, 12.0f);
	SetWobbleValue(NewValue);

	if (FMath::IsNearlyEqual(CurrentWobbleValue, TargetWobbleValue, 0.01f))
	{
		SetWobbleValue(TargetWobbleValue);
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(WobbleTimerHandle);
		}
	}
}

void ACropoutInteractable::StartWobbleTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(WobbleTimerHandle, this, &ThisClass::UpdateWobble, 1.0f / 60.0f, true);
	}
}
