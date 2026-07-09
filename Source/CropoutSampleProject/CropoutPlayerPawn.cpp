// Fill out your copyright notice in the Description page of Project Settings.

#include "CropoutPlayerPawn.h"

#include "Components/ActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "CropoutBlueprintLibrary.h"
#include "CropoutGameInstanceInterface.h"
#include "CropoutInteractable.h"
#include "CropoutPlayerController.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Curves/CurveFloat.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputActionValue.h"
#include "GameFramework/Actor.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputCoreTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialParameterCollection.h"
#include "NiagaraComponent.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "UObject/UnrealType.h"

namespace
{
	UObject* GetBlueprintObjectPropertyValue(const UObject* Object, FName PropertyName)
	{
		if (!Object)
		{
			return nullptr;
		}

		const FObjectPropertyBase* ObjectProperty = FindFProperty<FObjectPropertyBase>(Object->GetClass(), PropertyName);
		return ObjectProperty ? ObjectProperty->GetObjectPropertyValue_InContainer(Object) : nullptr;
	}

	bool GetBlueprintBoolPropertyValue(const UObject* Object, FName PropertyName)
	{
		const FBoolProperty* BoolProperty = Object ? FindFProperty<FBoolProperty>(Object->GetClass(), PropertyName) : nullptr;
		return BoolProperty ? BoolProperty->GetPropertyValue_InContainer(Object) : false;
	}

	void SetBlueprintBoolPropertyValue(UObject* Object, FName PropertyName, bool Value)
	{
		FBoolProperty* BoolProperty = Object ? FindFProperty<FBoolProperty>(Object->GetClass(), PropertyName) : nullptr;
		if (BoolProperty)
		{
			BoolProperty->SetPropertyValue_InContainer(Object, Value);
		}
	}

	float GetBlueprintFloatPropertyValue(const UObject* Object, FName PropertyName)
	{
		const FProperty* Property = Object ? FindFProperty<FProperty>(Object->GetClass(), PropertyName) : nullptr;
		if (const FFloatProperty* FloatProperty = CastField<FFloatProperty>(Property))
		{
			return FloatProperty->GetPropertyValue_InContainer(Object);
		}
		if (const FDoubleProperty* DoubleProperty = CastField<FDoubleProperty>(Property))
		{
			return static_cast<float>(DoubleProperty->GetPropertyValue_InContainer(Object));
		}
		if (const FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property))
		{
			return static_cast<float>(NumericProperty->GetFloatingPointPropertyValue(NumericProperty->ContainerPtrToValuePtr<void>(Object)));
		}
		return 0.0f;
	}

	void SetBlueprintFloatPropertyValue(UObject* Object, FName PropertyName, float Value)
	{
		FProperty* Property = Object ? FindFProperty<FProperty>(Object->GetClass(), PropertyName) : nullptr;
		if (FFloatProperty* FloatProperty = CastField<FFloatProperty>(Property))
		{
			FloatProperty->SetPropertyValue_InContainer(Object, Value);
		}
		else if (FDoubleProperty* DoubleProperty = CastField<FDoubleProperty>(Property))
		{
			DoubleProperty->SetPropertyValue_InContainer(Object, static_cast<double>(Value));
		}
		else if (FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property))
		{
			NumericProperty->SetFloatingPointPropertyValue(NumericProperty->ContainerPtrToValuePtr<void>(Object), Value);
		}
	}

	void SetBlueprintUint8PropertyValue(UObject* Object, FName PropertyName, uint8 Value)
	{
		FProperty* Property = Object ? FindFProperty<FProperty>(Object->GetClass(), PropertyName) : nullptr;
		if (FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
		{
			ByteProperty->SetPropertyValue_InContainer(Object, Value);
		}
		else if (FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
		{
			if (FNumericProperty* UnderlyingProperty = EnumProperty->GetUnderlyingProperty())
			{
				UnderlyingProperty->SetIntPropertyValue(
					EnumProperty->ContainerPtrToValuePtr<void>(Object),
					static_cast<uint64>(Value));
			}
		}
		else if (FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property))
		{
			NumericProperty->SetIntPropertyValue(
				NumericProperty->ContainerPtrToValuePtr<void>(Object),
				static_cast<uint64>(Value));
		}
	}

	uint8 GetBlueprintUint8PropertyValue(const UObject* Object, FName PropertyName)
	{
		FProperty* Property = Object ? FindFProperty<FProperty>(Object->GetClass(), PropertyName) : nullptr;
		if (const FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
		{
			return ByteProperty->GetPropertyValue_InContainer(Object);
		}
		if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
		{
			if (const FNumericProperty* UnderlyingProperty = EnumProperty->GetUnderlyingProperty())
			{
				return static_cast<uint8>(UnderlyingProperty->GetUnsignedIntPropertyValue(
					EnumProperty->ContainerPtrToValuePtr<void>(Object)));
			}
		}
		if (const FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property))
		{
			return static_cast<uint8>(NumericProperty->GetUnsignedIntPropertyValue(
				NumericProperty->ContainerPtrToValuePtr<void>(Object)));
		}
		return 0;
	}

	UClass* GetBlueprintClassPropertyValue(const UObject* Object, FName PropertyName)
	{
		const FClassProperty* ClassProperty = Object ? FindFProperty<FClassProperty>(Object->GetClass(), PropertyName) : nullptr;
		return ClassProperty ? Cast<UClass>(ClassProperty->GetObjectPropertyValue_InContainer(Object)) : nullptr;
	}

	void SetBlueprintClassPropertyValue(UObject* Object, FName PropertyName, UClass* Value)
	{
		FClassProperty* ClassProperty = Object ? FindFProperty<FClassProperty>(Object->GetClass(), PropertyName) : nullptr;
		if (ClassProperty)
		{
			ClassProperty->SetPropertyValue_InContainer(Object, Value);
		}
	}

	void SetBlueprintObjectPropertyValue(UObject* Object, FName PropertyName, UObject* Value)
	{
		if (!Object)
		{
			return;
		}

		FObjectPropertyBase* ObjectProperty = FindFProperty<FObjectPropertyBase>(Object->GetClass(), PropertyName);
		if (ObjectProperty)
		{
			ObjectProperty->SetObjectPropertyValue_InContainer(Object, Value);
		}
	}

	void WriteIntegerLikeProperty(const FProperty* Property, void* ValuePtr, int64 Value)
	{
		if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
		{
			if (FNumericProperty* UnderlyingProperty = EnumProperty->GetUnderlyingProperty())
			{
				UnderlyingProperty->SetIntPropertyValue(ValuePtr, Value);
			}
			return;
		}

		if (const FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property))
		{
			NumericProperty->SetIntPropertyValue(ValuePtr, Value);
		}
	}

	void SetBlueprintResourceCostPropertyValue(
		UObject* Object,
		FName PropertyName,
		const TMap<ECropoutResourceType, int32>& Value)
	{
		FMapProperty* MapProperty = Object ? FindFProperty<FMapProperty>(Object->GetClass(), PropertyName) : nullptr;
		if (!MapProperty)
		{
			return;
		}

		FScriptMapHelper Helper(MapProperty, MapProperty->ContainerPtrToValuePtr<void>(Object));
		Helper.EmptyValues();
		for (const TPair<ECropoutResourceType, int32>& Entry : Value)
		{
			const int32 Index = Helper.AddDefaultValue_Invalid_NeedsRehash();
			WriteIntegerLikeProperty(MapProperty->KeyProp, Helper.GetKeyPtr(Index), static_cast<uint8>(Entry.Key));
			WriteIntegerLikeProperty(MapProperty->ValueProp, Helper.GetValuePtr(Index), Entry.Value);
		}
		Helper.Rehash();
	}

	void SetBlueprintVectorArrayPropertyValue(UObject* Object, FName PropertyName, const TArray<FVector>& Value)
	{
		if (!Object)
		{
			return;
		}

		FArrayProperty* ArrayProperty = FindFProperty<FArrayProperty>(Object->GetClass(), PropertyName);
		FStructProperty* InnerStructProperty = ArrayProperty ? CastField<FStructProperty>(ArrayProperty->Inner) : nullptr;
		if (!ArrayProperty || !InnerStructProperty || InnerStructProperty->Struct != TBaseStructure<FVector>::Get())
		{
			return;
		}

		FScriptArrayHelper ArrayHelper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(Object));
		ArrayHelper.Resize(Value.Num());
		for (int32 Index = 0; Index < Value.Num(); ++Index)
		{
			InnerStructProperty->CopyCompleteValue(ArrayHelper.GetRawPtr(Index), &Value[Index]);
		}
	}

	bool GetBlueprintTransformPropertyValue(const UObject* Object, FName PropertyName, FTransform& OutValue)
	{
		const FStructProperty* StructProperty = Object ? FindFProperty<FStructProperty>(Object->GetClass(), PropertyName) : nullptr;
		if (!StructProperty || StructProperty->Struct != TBaseStructure<FTransform>::Get())
		{
			return false;
		}

		OutValue = *StructProperty->ContainerPtrToValuePtr<FTransform>(Object);
		return true;
	}

	void SetBlueprintTransformPropertyValue(UObject* Object, FName PropertyName, const FTransform& Value)
	{
		FStructProperty* StructProperty = Object ? FindFProperty<FStructProperty>(Object->GetClass(), PropertyName) : nullptr;
		if (StructProperty && StructProperty->Struct == TBaseStructure<FTransform>::Get())
		{
			StructProperty->CopyCompleteValue(StructProperty->ContainerPtrToValuePtr<void>(Object), &Value);
		}
	}

	void CallNoParamFunctionByName(UObject* Object, FName FunctionName)
	{
		if (!Object)
		{
			return;
		}

		if (UFunction* Function = Object->FindFunction(FunctionName))
		{
			Object->ProcessEvent(Function, nullptr);
		}
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

	UInputMappingContext* LoadInputMappingContext(const TCHAR* Path)
	{
		return LoadObject<UInputMappingContext>(nullptr, Path);
	}

	UInputAction* LoadInputAction(const TCHAR* Path)
	{
		return LoadObject<UInputAction>(nullptr, Path);
	}

	void CallActionMessage(UObject* Target, AActor* NewParam)
	{
		if (!Target)
		{
			return;
		}

		if (UFunction* Function = Target->FindFunction(TEXT("Action")))
		{
			struct FActionParams
			{
				AActor* NewParam = nullptr;
			};

			FActionParams Params;
			Params.NewParam = NewParam;
			Target->ProcessEvent(Function, &Params);
		}
	}
}

ACropoutPlayerPawn::ACropoutPlayerPawn()
{
	PrimaryActorTick.bCanEverTick = true;
}

// ============================================================================
// 21 BP_Player-parity UFUNCTIONs.
//
// Most bodies still remain temporary shells while BP_Player owns the matching
// graph body. Functions migrated in later passes get their native bodies here
// before the BP graph is removed.
// ============================================================================

// UserConstructionScript is a BlueprintImplementableEvent on AActor (not a
// C++ virtual), so there is no C++ implementation here. BP_Player's
// Construction Script graph implements it directly.
void ACropoutPlayerPawn::InputSwitch(uint8 NewInput)
{
	SetBlueprintUint8PropertyValue(this, TEXT("InputType"), NewInput);

	switch (NewInput)
	{
	case 1:
		if (USceneComponent* Cursor = Cast<USceneComponent>(GetBlueprintObjectPropertyValue(this, TEXT("Cursor"))))
		{
			Cursor->SetHiddenInGame(false, true);
		}
		break;
	case 2:
		if (USceneComponent* Collision = Cast<USceneComponent>(GetBlueprintObjectPropertyValue(this, TEXT("Collision"))))
		{
			Collision->SetRelativeLocation(FVector(0.0, 0.0, 10.0));
		}
		break;
	case 3:
		if (USceneComponent* Collision = Cast<USceneComponent>(GetBlueprintObjectPropertyValue(this, TEXT("Collision"))))
		{
			Collision->SetRelativeLocation(FVector(0.0, 0.0, -500.0));
		}
		break;
	default:
		break;
	}
}
void ACropoutPlayerPawn::UpdateBuildAsset()
{
	FVector2D ScreenPos = FVector2D::ZeroVector;
	FVector Intersection = FVector::ZeroVector;
	if (!ProjectMouseTouch1ToGroundPlane(ScreenPos, Intersection))
	{
		return;
	}

	AActor* Spawn = Cast<AActor>(GetBlueprintObjectPropertyValue(this, TEXT("Spawn")));
	if (!Spawn)
	{
		return;
	}

	const FVector TargetLocation = UCropoutBlueprintLibrary::ConvertToSteppedPos(Intersection);
	const FVector NewLocation = UKismetMathLibrary::VInterpTo(
		Spawn->GetActorLocation(),
		TargetLocation,
		UGameplayStatics::GetWorldDeltaSeconds(this),
		18.0f);
	Spawn->SetActorLocation(NewLocation, false, nullptr, ETeleportType::None);

	TArray<AActor*> OverlappingActors;
	Spawn->GetOverlappingActors(OverlappingActors, ACropoutInteractable::StaticClass());
	const bool bCanDrop = OverlappingActors.Num() == 0 && CornersInNav();
	SetBlueprintBoolPropertyValue(this, TEXT("CanDrop"), bCanDrop);

	UMaterialParameterCollection* CropoutCollection = LoadObject<UMaterialParameterCollection>(
		nullptr,
		TEXT("/Game/Blueprint/Core/Extras/MPC_Cropout.MPC_Cropout"));
	if (CropoutCollection)
	{
		UKismetMaterialLibrary::SetVectorParameterValue(
			this,
			CropoutCollection,
			TEXT("Target Position"),
			FLinearColor(NewLocation.X, NewLocation.Y, NewLocation.Z, bCanDrop ? 1.0f : 0.0f));
	}
}
void ACropoutPlayerPawn::UpdateCursorPosition()
{
	const uint8 InputType = GetBlueprintUint8PropertyValue(this, TEXT("InputType"));
	if (InputType == 0)
	{
		return;
	}

	FTransform Target = FTransform::Identity;
	GetBlueprintTransformPropertyValue(this, TEXT("Target"), Target);

	if (InputType == 1 || InputType == 2)
	{
		if (AActor* HoverActor = Cast<AActor>(GetBlueprintObjectPropertyValue(this, TEXT("HoverActor"))))
		{
			FVector Origin = FVector::ZeroVector;
			FVector BoxExtent = FVector::ZeroVector;
			HoverActor->GetActorBounds(true, Origin, BoxExtent, false);

			const double BoxScale = FMath::Max(FMath::Abs(BoxExtent.X), FMath::Abs(BoxExtent.Y)) / 50.0;
			const double PulseScale = FMath::Sin(UGameplayStatics::GetTimeSeconds(this) * 5.0f) * 0.25;
			const double CursorScale = BoxScale + PulseScale;
			Target = FTransform(
				FRotator::ZeroRotator,
				FVector(Origin.X, Origin.Y, 20.0),
				FVector(CursorScale, CursorScale, 1.0));
		}
		else if (const USceneComponent* Collision = Cast<USceneComponent>(GetBlueprintObjectPropertyValue(this, TEXT("Collision"))))
		{
			Target = FTransform(
				Collision->GetComponentRotation(),
				Collision->GetComponentLocation(),
				FVector(2.0, 2.0, 1.0));
		}
		else
		{
			Target = FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector(2.0, 2.0, 1.0));
		}
	}
	else if (InputType == 3)
	{
		FVector2D ScreenPos = FVector2D::ZeroVector;
		FVector Intersection = FVector::ZeroVector;
		const bool bProjected = ProjectMouseTouch1ToGroundPlane(ScreenPos, Intersection);
		const FVector CurrentTargetLocation = Target.GetLocation();
		const FVector FallbackLocation(CurrentTargetLocation.X, CurrentTargetLocation.Y, -100.0);

		Target = FTransform(
			FRotator::ZeroRotator,
			bProjected ? Intersection : FallbackLocation,
			FVector(2.0, 2.0, 1.0));
	}
	else
	{
		return;
	}

	SetBlueprintTransformPropertyValue(this, TEXT("Target"), Target);

	if (USceneComponent* Cursor = Cast<USceneComponent>(GetBlueprintObjectPropertyValue(this, TEXT("Cursor"))))
	{
		const FTransform InterpolatedTransform = UKismetMathLibrary::TInterpTo(
			Cursor->GetComponentTransform(),
			Target,
			UGameplayStatics::GetWorldDeltaSeconds(this),
			12.0f);
		Cursor->SetWorldTransform(InterpolatedTransform, false, nullptr, ETeleportType::None);
	}
}
void ACropoutPlayerPawn::UpdateZoom()
{
	const float ZoomValue = GetBlueprintFloatPropertyValue(this, TEXT("ZoomValue"));
	const UCurveFloat* ZoomCurve = Cast<UCurveFloat>(GetBlueprintObjectPropertyValue(this, TEXT("ZoomCurve")));
	const float CurveValue = ZoomCurve ? ZoomCurve->GetFloatValue(ZoomValue) : 0.0f;

	const float NewZoomValue = FMath::Clamp(
		ZoomValue + (GetBlueprintFloatPropertyValue(this, TEXT("ZoomDirection")) * 0.01f),
		0.0f,
		1.0f);
	SetBlueprintFloatPropertyValue(this, TEXT("ZoomValue"), NewZoomValue);

	if (USpringArmComponent* SpringArm = Cast<USpringArmComponent>(GetBlueprintObjectPropertyValue(this, TEXT("SpringArm"))))
	{
		SpringArm->TargetArmLength = FMath::Lerp(800.0f, 40000.0f, CurveValue);
		SpringArm->SetRelativeRotation(FRotator(FMath::Lerp(-40.0f, -55.0f, CurveValue), 0.0f, 0.0f));
	}

	if (UFloatingPawnMovement* Movement = Cast<UFloatingPawnMovement>(GetBlueprintObjectPropertyValue(this, TEXT("FloatingPawnMovement"))))
	{
		Movement->MaxSpeed = FMath::Lerp(1000.0f, 6000.0f, CurveValue);
	}

	Dof();

	if (UCameraComponent* Camera = Cast<UCameraComponent>(GetBlueprintObjectPropertyValue(this, TEXT("Camera"))))
	{
		Camera->SetFieldOfView(FMath::Lerp(20.0f, 15.0f, CurveValue));
	}
}
void ACropoutPlayerPawn::MoveTracking()
{
	const FVector ActorLocation = GetActorLocation();
	const FVector NormalizedActorLocation = ActorLocation.GetSafeNormal(0.0001);
	const FVector CenterDirection(-NormalizedActorLocation.X, -NormalizedActorLocation.Y, 0.0);
	const float CenterStrength = static_cast<float>(FMath::Max((ActorLocation.Size() - 9000.0) / 5000.0, 0.0));
	AddMovementInput(CenterDirection, CenterStrength, false);

	FVector EdgeDirection = FVector::ZeroVector;
	double EdgeStrength = 0.0;
	EdgeMove(EdgeDirection, EdgeStrength);
	AddMovementInput(EdgeDirection, static_cast<float>(EdgeStrength), false);

	USceneComponent* Collision = Cast<USceneComponent>(GetBlueprintObjectPropertyValue(this, TEXT("Collision")));
	if (Collision)
	{
		FVector2D ScreenPos = FVector2D::ZeroVector;
		FVector Intersection = FVector::ZeroVector;
		const bool bProjected = ProjectMouseTouch1ToGroundPlane(ScreenPos, Intersection);

		const FVector GroundLocation = Intersection + FVector(0.0, 0.0, 10.0);
		FVector NewCollisionLocation = GroundLocation;
		if (GetBlueprintUint8PropertyValue(this, TEXT("InputType")) == 3)
		{
			const FVector CurrentCollisionLocation = Collision->GetComponentLocation();
			const FVector HiddenTargetLocation(CurrentCollisionLocation.X, CurrentCollisionLocation.Y, -500.0);
			NewCollisionLocation = bProjected
				? Intersection
				: UKismetMathLibrary::VInterpTo(
					CurrentCollisionLocation,
					HiddenTargetLocation,
					UGameplayStatics::GetWorldDeltaSeconds(this),
					12.0f);
		}

		Collision->SetWorldLocation(NewCollisionLocation, false, nullptr, ETeleportType::None);
	}

	UpdateCursorPosition();
}

bool ACropoutPlayerPawn::SingleTouchCheck()
{
	const APlayerController* PlayerController = GetPlayerController();
	if (!PlayerController)
	{
		return true;
	}

	float LocationX = 0.0f;
	float LocationY = 0.0f;
	bool bIsCurrentlyPressed = false;
	PlayerController->GetInputTouchState(ETouchIndex::Touch2, LocationX, LocationY, bIsCurrentlyPressed);
	return !bIsCurrentlyPressed;
}

void ACropoutPlayerPawn::CursorDistFromViewportCenter(FVector2D A, FVector& Direction, double& Strength)
{
	Direction = FVector::ZeroVector;
	Strength = 1.0;

	const APlayerController* PlayerController = GetPlayerController();
	int32 ViewportSizeX = 0;
	int32 ViewportSizeY = 0;
	if (PlayerController)
	{
		PlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);
	}

	const double EdgeMoveDistance = static_cast<double>(GetBlueprintFloatPropertyValue(this, TEXT("EdgeMoveDistance")));
	const uint8 InputType = GetBlueprintUint8PropertyValue(this, TEXT("InputType"));
	double InputScale = 0.0;
	switch (InputType)
	{
	case 1:
		InputScale = 1.0;
		break;
	case 2:
	case 3:
		InputScale = 2.0;
		break;
	default:
		break;
	}

	const double ActiveEdgeDistance = EdgeMoveDistance * InputScale;
	const double ThresholdX = static_cast<double>(ViewportSizeX) * 0.5 - ActiveEdgeDistance;
	const double ThresholdY = static_cast<double>(ViewportSizeY) * 0.5 - ActiveEdgeDistance;
	const double DistanceX = FMath::Max(FMath::Abs(A.X) - ThresholdX, 0.0);
	const double DistanceY = FMath::Max(FMath::Abs(A.Y) - ThresholdY, 0.0);
	if (FMath::IsNearlyZero(EdgeMoveDistance))
	{
		return;
	}

	Direction = FVector(
		-FMath::Sign(A.Y) * (DistanceY / EdgeMoveDistance),
		FMath::Sign(A.X) * (DistanceX / EdgeMoveDistance),
		0.0);
}

void ACropoutPlayerPawn::EdgeMove(FVector& Direction, double& Strength)
{
	Direction = FVector::ZeroVector;
	Strength = 0.0;

	const APlayerController* PlayerController = GetPlayerController();
	if (!PlayerController)
	{
		return;
	}

	int32 ViewportSizeX = 0;
	int32 ViewportSizeY = 0;
	PlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);
	const FVector2D ViewportSize(static_cast<double>(ViewportSizeX), static_cast<double>(ViewportSizeY));
	const FVector2D ViewportCenter = ViewportSize / 2.0;

	FVector2D ScreenPosition = ViewportCenter;
	const uint8 InputType = GetBlueprintUint8PropertyValue(this, TEXT("InputType"));
	if (InputType == 1)
	{
		float MouseX = 0.0f;
		float MouseY = 0.0f;
		if (PlayerController->GetMousePosition(MouseX, MouseY))
		{
			ScreenPosition = FVector2D(MouseX, MouseY);
		}
	}
	else if (InputType == 3)
	{
		float TouchX = 0.0f;
		float TouchY = 0.0f;
		bool bTouchPressed = false;
		PlayerController->GetInputTouchState(ETouchIndex::Touch1, TouchX, TouchY, bTouchPressed);
		if (bTouchPressed)
		{
			ScreenPosition = FVector2D(TouchX, TouchY);
		}
	}

	FVector LocalDirection = FVector::ZeroVector;
	CursorDistFromViewportCenter(ScreenPosition - ViewportCenter, LocalDirection, Strength);
	Direction = UKismetMathLibrary::TransformDirection(GetActorTransform(), LocalDirection);
}

bool ACropoutPlayerPawn::ProjectMouseTouch1ToGroundPlane(FVector2D& ScreenPos, FVector& Intersection)
{
	ScreenPos = FVector2D::ZeroVector;
	Intersection = FVector::ZeroVector;

	const APlayerController* PlayerController = GetPlayerController();
	if (!PlayerController)
	{
		return false;
	}

	int32 ViewportSizeX = 0;
	int32 ViewportSizeY = 0;
	PlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);
	const FVector2D ViewportCenter(static_cast<double>(ViewportSizeX) * 0.5, static_cast<double>(ViewportSizeY) * 0.5);

	float MouseX = 0.0f;
	float MouseY = 0.0f;
	const bool bMousePositionValid = PlayerController->GetMousePosition(MouseX, MouseY);

	float TouchX = 0.0f;
	float TouchY = 0.0f;
	bool bTouchPressed = false;
	PlayerController->GetInputTouchState(ETouchIndex::Touch1, TouchX, TouchY, bTouchPressed);

	const uint8 InputType = GetBlueprintUint8PropertyValue(this, TEXT("InputType"));
	bool bReturnValue = false;
	double ZOffset = 0.0;
	switch (InputType)
	{
	case 1:
		ScreenPos = bMousePositionValid ? FVector2D(MouseX, MouseY) : ViewportCenter;
		bReturnValue = bMousePositionValid;
		break;
	case 2:
		ScreenPos = ViewportCenter;
		bReturnValue = true;
		break;
	case 3:
		ScreenPos = bTouchPressed ? FVector2D(TouchX, TouchY) : ViewportCenter;
		bReturnValue = bTouchPressed;
		ZOffset = bTouchPressed ? 0.0 : -500.0;
		break;
	default:
		ScreenPos = ViewportCenter;
		break;
	}

	FVector WorldPosition = FVector::ZeroVector;
	FVector WorldDirection = FVector::ZeroVector;
	const APlayerController* DeprojectController = UGameplayStatics::GetPlayerController(this, 0);
	if (!DeprojectController)
	{
		DeprojectController = PlayerController;
	}

	if (UGameplayStatics::DeprojectScreenToWorld(DeprojectController, ScreenPos, WorldPosition, WorldDirection))
	{
		Intersection = FMath::LinePlaneIntersection(
			WorldPosition,
			WorldPosition + WorldDirection * 100000.0,
			FVector::ZeroVector,
			FVector::UpVector);
	}
	Intersection.Z += ZOffset;
	return bReturnValue;
}

void ACropoutPlayerPawn::SteppedLocAndRot(FVector& Location, FRotator& Rotation)
{
	const USceneComponent* Collision = Cast<USceneComponent>(GetBlueprintObjectPropertyValue(this, TEXT("Collision")));
	Location = Collision ? UCropoutBlueprintLibrary::ConvertToSteppedPos(Collision->GetComponentLocation()) : FVector::ZeroVector;

	const float StepRotation = GetBlueprintFloatPropertyValue(this, TEXT("StepRotation"));
	const float Yaw = GetActorRotation().Yaw;
	const float QuantizedYaw = FMath::IsNearlyZero(StepRotation)
		? 0.0f
		: (static_cast<float>(FMath::FloorToInt((Yaw / 360.0f) * StepRotation)) / StepRotation) * 360.0f;
	const FVector XAxis = FVector(1.0f, 0.0f, 0.0f).RotateAngleAxis(QuantizedYaw, FVector::UpVector);
	Rotation = UKismetMathLibrary::MakeRotFromXZ(XAxis, GetActorUpVector());
}

bool ACropoutPlayerPawn::CornersInNav()
{
	AActor* Spawn = Cast<AActor>(GetBlueprintObjectPropertyValue(this, TEXT("Spawn")));
	UBoxComponent* Box = FindNamedOrFirstComponent<UBoxComponent>(Spawn, TEXT("Box"));
	if (!Spawn || !Box)
	{
		SetBlueprintBoolPropertyValue(this, TEXT("OnIsland"), false);
		return false;
	}

	FVector Origin = FVector::ZeroVector;
	FVector BoxExtent = FVector::ZeroVector;
	float SphereRadius = 0.0f;
	UKismetSystemLibrary::GetComponentBounds(Box, Origin, BoxExtent, SphereRadius);

	const double OffsetX = BoxExtent.X * 1.05;
	const double OffsetY = BoxExtent.Y * 1.05;
	const FVector CornerOffsets[] = {
		FVector(OffsetX, OffsetY, 0.0),
		FVector(-OffsetX, OffsetY, 0.0),
		FVector(OffsetY, -OffsetY, 0.0),
		FVector(-OffsetX, -OffsetY, 0.0),
	};

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(Spawn);

	bool bOnIsland = true;
	for (const FVector& CornerOffset : CornerOffsets)
	{
		const FVector Corner = Origin + CornerOffset;
		TArray<FHitResult> Hits;
		if (!UKismetSystemLibrary::LineTraceMulti(
			this,
			FVector(Corner.X, Corner.Y, 100.0),
			FVector(Corner.X, Corner.Y, -1.0),
			UEngineTypes::ConvertToTraceType(ECC_Visibility),
			false,
			ActorsToIgnore,
			EDrawDebugTrace::None,
			Hits,
			true,
			FLinearColor::Red,
			FLinearColor::Green,
			5.0f))
		{
			bOnIsland = false;
			break;
		}
	}

	SetBlueprintBoolPropertyValue(this, TEXT("OnIsland"), bOnIsland);
	return bOnIsland;
}
void ACropoutPlayerPawn::CreateBuildOverlay()
{
	if (IsValid(Cast<UStaticMeshComponent>(GetBlueprintObjectPropertyValue(this, TEXT("SpawnOverlay")))))
	{
		return;
	}

	AActor* Spawn = Cast<AActor>(GetBlueprintObjectPropertyValue(this, TEXT("Spawn")));
	UStaticMeshComponent* ParentMesh = FindNamedOrFirstComponent<UStaticMeshComponent>(Spawn, TEXT("Mesh"));
	if (!Spawn || !ParentMesh)
	{
		return;
	}

	FVector Origin = FVector::ZeroVector;
	FVector BoxExtent = FVector::ZeroVector;
	Spawn->GetActorBounds(false, Origin, BoxExtent, false);

	UStaticMeshComponent* Overlay = NewObject<UStaticMeshComponent>(this, UStaticMeshComponent::StaticClass(), TEXT("StaticMeshCube"));
	if (!Overlay)
	{
		return;
	}

	if (UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
	{
		Overlay->SetStaticMesh(CubeMesh);
	}
	Overlay->SetRelativeTransform(FTransform(FRotator::ZeroRotator, FVector::ZeroVector, BoxExtent / 50.0));
	Overlay->SetMobility(EComponentMobility::Movable);

	AddInstanceComponent(Overlay);
	Overlay->OnComponentCreated();
	Overlay->RegisterComponent();
	Overlay->AttachToComponent(
		ParentMesh,
		FAttachmentTransformRules(EAttachmentRule::SnapToTarget, EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld, true),
		NAME_None);

	SetBlueprintObjectPropertyValue(this, TEXT("SpawnOverlay"), Overlay);
	CallNoParamFunctionByName(this, TEXT("UpdateBuildAsset"));
}

void ACropoutPlayerPawn::DestroySpawn()
{
	if (AActor* Spawn = Cast<AActor>(GetBlueprintObjectPropertyValue(this, TEXT("Spawn"))))
	{
		Spawn->Destroy();
	}

	if (UActorComponent* SpawnOverlay = Cast<UActorComponent>(GetBlueprintObjectPropertyValue(this, TEXT("SpawnOverlay"))))
	{
		SpawnOverlay->DestroyComponent();
	}
}

void ACropoutPlayerPawn::ClosestHoverCheck()
{
	UPrimitiveComponent* Collision = Cast<UPrimitiveComponent>(GetBlueprintObjectPropertyValue(this, TEXT("Collision")));
	if (!Collision)
	{
		return;
	}

	TArray<AActor*> OverlappingActors;
	Collision->GetOverlappingActors(OverlappingActors, AActor::StaticClass());
	if (OverlappingActors.Num() == 0)
	{
		UKismetSystemLibrary::K2_PauseTimer(this, TEXT("Closest Hover Check"));
		SetBlueprintObjectPropertyValue(this, TEXT("HoverActor"), nullptr);
		return;
	}

	const FVector CollisionLocation = Collision->GetComponentLocation();
	AActor* NewHover = nullptr;
	float BestDistanceSquared = TNumericLimits<float>::Max();
	for (AActor* Actor : OverlappingActors)
	{
		if (!Actor)
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(Actor->GetActorLocation(), CollisionLocation);
		if (!NewHover || DistanceSquared < BestDistanceSquared)
		{
			NewHover = Actor;
			BestDistanceSquared = DistanceSquared;
		}
	}

	SetBlueprintObjectPropertyValue(this, TEXT("NewHover"), NewHover);

	AActor* HoverActor = Cast<AActor>(GetBlueprintObjectPropertyValue(this, TEXT("HoverActor")));
	if (HoverActor != NewHover)
	{
		SetBlueprintObjectPropertyValue(this, TEXT("HoverActor"), NewHover);
	}
}

// R16: empty BlueprintNativeEvent _Implementation bodies.
// The BP_Player BP-side function graphs for these four names
// will be removed in this round via `remove_function_graph`,
// leaving the C++ stub as the authoritative entry point. Body
// migration from the BP-side graphs (camera post-process,
// edge-scroll move, position validity, BeginBuild interface
// handler) is deferred to a later round.
void ACropoutPlayerPawn::Dof_Implementation() {}
void ACropoutPlayerPawn::TrackMove_Implementation() {}

APlayerController* ACropoutPlayerPawn::GetPlayerController()
{
	return Cast<APlayerController>(GetController());
}

void ACropoutPlayerPawn::UpdatePath()
{
	const USceneComponent* Collision = Cast<USceneComponent>(GetBlueprintObjectPropertyValue(this, TEXT("Collision")));
	const AActor* Selected = Cast<AActor>(GetBlueprintObjectPropertyValue(this, TEXT("Selected")));
	if (!Collision || !Selected)
	{
		return;
	}

	UNavigationPath* NavigationPath = UNavigationSystemV1::FindPathToLocationSynchronously(
		this,
		Collision->GetComponentLocation(),
		Selected->GetActorLocation());
	if (!NavigationPath || NavigationPath->PathPoints.Num() <= 1)
	{
		return;
	}

	TArray<FVector> PathPoints = NavigationPath->PathPoints;
	PathPoints[0] = Collision->GetComponentLocation();
	PathPoints[PathPoints.Num() - 1] = Selected->GetActorLocation();
	SetBlueprintVectorArrayPropertyValue(this, TEXT("PathPoints"), PathPoints);

	if (UNiagaraComponent* PathComponent = Cast<UNiagaraComponent>(GetBlueprintObjectPropertyValue(this, TEXT("NS_Path"))))
	{
		UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(PathComponent, TEXT("TargetPath"), PathPoints);
	}
}

void ACropoutPlayerPawn::RotateSpawn()
{
	if (AActor* Spawn = Cast<AActor>(GetBlueprintObjectPropertyValue(this, TEXT("Spawn"))))
	{
		Spawn->SetActorRotation(UKismetMathLibrary::ComposeRotators(Spawn->GetActorRotation(), FRotator(0.0, 90.0, 0.0)));
	}
}

bool ACropoutPlayerPawn::OverlapCheck()
{
	const UPrimitiveComponent* Collision = Cast<UPrimitiveComponent>(GetBlueprintObjectPropertyValue(this, TEXT("Collision")));
	const AActor* Spawn = Cast<AActor>(GetBlueprintObjectPropertyValue(this, TEXT("Spawn")));
	if (!Collision || !Spawn)
	{
		return false;
	}

	TArray<AActor*> OverlappingActors;
	UClass* InteractableClass = StaticLoadClass(
		AActor::StaticClass(),
		nullptr,
		TEXT("/Game/Blueprint/Interactable/BP_Interactable.BP_Interactable_C"));
	Collision->GetOverlappingActors(OverlappingActors, InteractableClass ? InteractableClass : AActor::StaticClass());
	return OverlappingActors.Contains(Spawn);
}

void ACropoutPlayerPawn::VillagerSelect(AActor* Selected)
{
	SetBlueprintObjectPropertyValue(this, TEXT("Selected"), Selected);

	UNiagaraSystem* TargetSystem = Cast<UNiagaraSystem>(StaticLoadObject(
		UNiagaraSystem::StaticClass(),
		nullptr,
		TEXT("/Game/VFX/NS_Target.NS_Target")));
	USceneComponent* DefaultSceneRoot = Cast<USceneComponent>(GetBlueprintObjectPropertyValue(this, TEXT("DefaultSceneRoot")));
	UNiagaraComponent* PathComponent = TargetSystem && DefaultSceneRoot
		? UNiagaraFunctionLibrary::SpawnSystemAttached(
			TargetSystem,
			DefaultSceneRoot,
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			false,
			true,
			ENCPoolMethod::None,
			true)
		: nullptr;
	SetBlueprintObjectPropertyValue(this, TEXT("NS_Path"), PathComponent);

	UKismetSystemLibrary::K2_SetTimer(this, TEXT("Update Path"), 0.01f, true);
}

void ACropoutPlayerPawn::VillagerRelease()
{
	UKismetSystemLibrary::K2_PauseTimer(this, TEXT("Update Path"));

	if (UActorComponent* PathComponent = Cast<UActorComponent>(GetBlueprintObjectPropertyValue(this, TEXT("NS_Path"))))
	{
		PathComponent->DestroyComponent();
	}

	SetBlueprintObjectPropertyValue(this, TEXT("Selected"), nullptr);
}

void ACropoutPlayerPawn::RemoveResources()
{
	const bool bCanAffordAnotherBuild = UCropoutBlueprintLibrary::ApplyGameModeBuildResourceCost(this, this);
	if (!bCanAffordAnotherBuild)
	{
		UCropoutBlueprintLibrary::NotifyRemoveCurrentUILayer(this);
	}
	else
	{
		DestroySpawn();
	}
}

void ACropoutPlayerPawn::SpawnBuildTarget()
{
	if (!GetBlueprintBoolPropertyValue(this, TEXT("CanDrop")))
	{
		return;
	}

	AActor* Spawn = Cast<AActor>(GetBlueprintObjectPropertyValue(this, TEXT("Spawn")));
	UClass* TargetSpawnClass = GetBlueprintClassPropertyValue(this, TEXT("TargetSpawnClass"));
	UWorld* World = GetWorld();
	if (!Spawn || !TargetSpawnClass || !World)
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::Undefined;
	AActor* BuiltActor = World->SpawnActor<AActor>(TargetSpawnClass, Spawn->GetActorTransform(), SpawnParameters);
	if (ACropoutInteractable* Interactable = Cast<ACropoutInteractable>(BuiltActor))
	{
		Interactable->SetProgressionsState(0.0f);
	}
	else if (BuiltActor)
	{
		if (UFunction* Function = BuiltActor->FindFunction(TEXT("SetProgressionsState")))
		{
			struct FSetProgressionsStateParams
			{
				float Progression = 0.0f;
			};
			FSetProgressionsStateParams Params;
			BuiltActor->ProcessEvent(Function, &Params);
		}
	}

	RemoveResources();

	UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);
	if (GameInstance && GameInstance->Implements<UCropoutGameInstanceInterface>())
	{
		ICropoutGameInstanceInterface::Execute_UpdateAllInteractables(GameInstance);
	}

	CallNoParamFunctionByName(this, TEXT("UpdateBuildAsset"));
}
void ACropoutPlayerPawn::PositionCheck_Implementation() {}

// ============================================================================
// Game lifecycle events.
// ============================================================================

void ACropoutPlayerPawn::BeginBuildNative(UClass* TargetClass, const TMap<ECropoutResourceType, int32>& ResourceCost)
{
	UClass* TargetSpawnClass = TargetClass;
	SetBlueprintClassPropertyValue(this, TEXT("TargetSpawnClass"), TargetSpawnClass);
	SetBlueprintResourceCostPropertyValue(this, TEXT("ResourceCost"), ResourceCost);

	if (!TargetSpawnClass)
	{
		return;
	}

	if (AActor* ExistingSpawn = Cast<AActor>(GetBlueprintObjectPropertyValue(this, TEXT("Spawn"))))
	{
		ExistingSpawn->Destroy();
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::Undefined;
	AActor* NewSpawn = World->SpawnActor<AActor>(TargetSpawnClass, FTransform(GetActorLocation()), SpawnParameters);
	SetBlueprintObjectPropertyValue(this, TEXT("Spawn"), NewSpawn);

	if (ACropoutInteractable* Interactable = Cast<ACropoutInteractable>(NewSpawn))
	{
		Interactable->PlacementMode();
	}
	else
	{
		CallNoParamFunctionByName(NewSpawn, TEXT("PlacementMode"));
	}

	CreateBuildOverlay();
}

void ACropoutPlayerPawn::EndBuild() {}
void ACropoutPlayerPawn::SwitchBuildMode(bool bSwitchToBuildMode)
{
	APlayerController* PlayerController = GetPlayerController();
	ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	if (!InputSubsystem)
	{
		return;
	}

	FModifyContextOptions Options;
	Options.bIgnoreAllPressedKeysUntilRelease = true;
	Options.bForceImmediately = false;
	Options.bNotifyUserSettings = false;

	UInputMappingContext* BuildModeContext = LoadInputMappingContext(TEXT("/Game/Blueprint/Core/Player/Input/IMC_BuildMode.IMC_BuildMode"));
	UInputMappingContext* VillagerModeContext = LoadInputMappingContext(TEXT("/Game/Blueprint/Core/Player/Input/IMC_Villager_Mode.IMC_Villager_Mode"));
	if (bSwitchToBuildMode)
	{
		if (VillagerModeContext)
		{
			InputSubsystem->RemoveMappingContext(VillagerModeContext, Options);
		}
		if (BuildModeContext)
		{
			InputSubsystem->AddMappingContext(BuildModeContext, 0, Options);
		}
	}
	else
	{
		if (BuildModeContext)
		{
			InputSubsystem->RemoveMappingContext(BuildModeContext, Options);
		}
		if (VillagerModeContext)
		{
			InputSubsystem->AddMappingContext(VillagerModeContext, 0, Options);
		}
	}
}
void ACropoutPlayerPawn::UpdateSelectedVillager(bool /*bNewSelection*/) {}
void ACropoutPlayerPawn::ResetPicker() {}

bool ACropoutPlayerPawn::VillagerOverlapCheck(AActor*& Output)
{
	Output = nullptr;

	UPrimitiveComponent* Collision = Cast<UPrimitiveComponent>(GetBlueprintObjectPropertyValue(this, TEXT("Collision")));
	if (!Collision)
	{
		return false;
	}

	TArray<AActor*> OverlappingActors;
	Collision->GetOverlappingActors(OverlappingActors, APawn::StaticClass());
	if (OverlappingActors.Num() == 0)
	{
		return false;
	}

	Output = OverlappingActors[0];
	return IsValid(Output);
}

// ============================================================================
// APawn lifecycle overrides.
//
// BeginPlay now owns BP_Player's former startup graph: initial zoom update,
// MoveTracking timer, and the default input mapping contexts.
// ============================================================================

void ACropoutPlayerPawn::BeginPlay()
{
	Super::BeginPlay();

	UpdateZoom();
	GetWorldTimerManager().SetTimer(
		MoveTrackingTimerHandle,
		this,
		&ACropoutPlayerPawn::MoveTracking,
		0.016667f,
		true,
		0.0f);

	APlayerController* PlayerController = GetPlayerController();
	ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	if (!InputSubsystem)
	{
		return;
	}

	FModifyContextOptions Options;
	Options.bIgnoreAllPressedKeysUntilRelease = true;
	Options.bForceImmediately = false;
	Options.bNotifyUserSettings = false;

	UInputMappingContext* BaseContext = BaseInputMappingContext
		? BaseInputMappingContext.Get()
		: LoadInputMappingContext(TEXT("/Game/Blueprint/Core/Player/Input/IMC_BaseInput.IMC_BaseInput"));
	UInputMappingContext* VillagerModeContext = VillagerModeInputMappingContext
		? VillagerModeInputMappingContext.Get()
		: LoadInputMappingContext(TEXT("/Game/Blueprint/Core/Player/Input/IMC_Villager_Mode.IMC_Villager_Mode"));

	if (BaseContext)
	{
		InputSubsystem->AddMappingContext(BaseContext, 0, Options);
	}
	if (VillagerModeContext)
	{
		InputSubsystem->AddMappingContext(VillagerModeContext, 0, Options);
	}
}

void ACropoutPlayerPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(MoveTrackingTimerHandle);

	Super::EndPlay(EndPlayReason);
}

void ACropoutPlayerPawn::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (IsValid(GetBlueprintObjectPropertyValue(this, TEXT("HoverActor"))))
	{
		return;
	}

	SetBlueprintObjectPropertyValue(this, TEXT("HoverActor"), OtherActor);
	UKismetSystemLibrary::K2_SetTimer(this, TEXT("Closest Hover Check"), 0.01f, true);
}

void ACropoutPlayerPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (ACropoutPlayerController* CropoutPlayerController = Cast<ACropoutPlayerController>(NewController))
	{
		CropoutPlayerController->KeySwitch.AddUniqueDynamic(this, &ACropoutPlayerPawn::InputSwitch);
	}
}

void ACropoutPlayerPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void ACropoutPlayerPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInput)
	{
		return;
	}

	UInputAction* Move = MoveAction ? MoveAction.Get() : LoadInputAction(TEXT("/Game/Blueprint/Core/Player/Input/IA_Move.IA_Move"));
	UInputAction* Spin = SpinAction ? SpinAction.Get() : LoadInputAction(TEXT("/Game/Blueprint/Core/Player/Input/IA_Spin.IA_Spin"));
	UInputAction* Zoom = ZoomAction ? ZoomAction.Get() : LoadInputAction(TEXT("/Game/Blueprint/Core/Player/Input/IA_Zoom.IA_Zoom"));
	UInputAction* BuildMove = BuildMoveAction ? BuildMoveAction.Get() : LoadInputAction(TEXT("/Game/Blueprint/Core/Player/Input/IA_Build_Move.IA_Build_Move"));
	UInputAction* Villager = VillagerAction ? VillagerAction.Get() : LoadInputAction(TEXT("/Game/Blueprint/Core/Player/Input/IA_Villager.IA_Villager"));
	UInputAction* DragMove = DragMoveAction ? DragMoveAction.Get() : LoadInputAction(TEXT("/Game/Blueprint/Core/Player/Input/IA_DragMove.IA_DragMove"));

	if (Move)
	{
		EnhancedInput->BindAction(Move, ETriggerEvent::Triggered, this, &ThisClass::HandleMoveTriggered);
	}
	if (Spin)
	{
		EnhancedInput->BindAction(Spin, ETriggerEvent::Triggered, this, &ThisClass::HandleSpinTriggered);
	}
	if (Zoom)
	{
		EnhancedInput->BindAction(Zoom, ETriggerEvent::Triggered, this, &ThisClass::HandleZoomTriggered);
	}
	if (BuildMove)
	{
		EnhancedInput->BindAction(BuildMove, ETriggerEvent::Triggered, this, &ThisClass::HandleBuildMoveTriggered);
		EnhancedInput->BindAction(BuildMove, ETriggerEvent::Completed, this, &ThisClass::HandleBuildMoveCompleted);
	}
	if (Villager)
	{
		EnhancedInput->BindAction(Villager, ETriggerEvent::Started, this, &ThisClass::HandleVillagerStarted);
		EnhancedInput->BindAction(Villager, ETriggerEvent::Canceled, this, &ThisClass::HandleVillagerEnded);
		EnhancedInput->BindAction(Villager, ETriggerEvent::Completed, this, &ThisClass::HandleVillagerEnded);
	}
	if (DragMove)
	{
		EnhancedInput->BindAction(DragMove, ETriggerEvent::Triggered, this, &ThisClass::HandleDragMoveTriggered);
	}
}

void ACropoutPlayerPawn::HandleMoveTriggered(const FInputActionValue& ActionValue)
{
	const FVector2D Axis = ActionValue.Get<FVector2D>();
	AddMovementInput(GetActorForwardVector(), Axis.Y, false);
	AddMovementInput(GetActorRightVector(), Axis.X, false);
}

void ACropoutPlayerPawn::HandleSpinTriggered(const FInputActionValue& ActionValue)
{
	AddActorLocalRotation(FRotator(0.0, ActionValue.Get<float>(), 0.0), false, nullptr, ETeleportType::None);
}

void ACropoutPlayerPawn::HandleZoomTriggered(const FInputActionValue& ActionValue)
{
	SetBlueprintFloatPropertyValue(this, TEXT("ZoomDirection"), ActionValue.Get<float>());
	UpdateZoom();
	Dof();
}

void ACropoutPlayerPawn::HandleBuildMoveTriggered(const FInputActionValue& /*ActionValue*/)
{
	UpdateBuildAsset();
}

void ACropoutPlayerPawn::HandleBuildMoveCompleted(const FInputActionValue& /*ActionValue*/)
{
	AActor* Spawn = Cast<AActor>(GetBlueprintObjectPropertyValue(this, TEXT("Spawn")));
	if (!IsValid(Spawn))
	{
		return;
	}

	Spawn->SetActorLocation(
		UCropoutBlueprintLibrary::ConvertToSteppedPos(Spawn->GetActorLocation()),
		false,
		nullptr,
		ETeleportType::None);
}

void ACropoutPlayerPawn::HandleVillagerStarted(const FInputActionValue& /*ActionValue*/)
{
	if (!SingleTouchCheck())
	{
		return;
	}

	PositionCheck();

	AActor* OverlappedVillager = nullptr;
	if (VillagerOverlapCheck(OverlappedVillager))
	{
		VillagerSelect(OverlappedVillager);
	}
	else
	{
		SetDragMoveMappingContext(true);
	}
}

void ACropoutPlayerPawn::HandleVillagerEnded(const FInputActionValue& /*ActionValue*/)
{
	SetDragMoveMappingContext(false);

	AActor* Selected = Cast<AActor>(GetBlueprintObjectPropertyValue(this, TEXT("Selected")));
	if (!IsValid(Selected))
	{
		return;
	}

	CallActionMessage(Selected, Cast<AActor>(GetBlueprintObjectPropertyValue(this, TEXT("HoverActor"))));
	VillagerRelease();
}

void ACropoutPlayerPawn::HandleDragMoveTriggered(const FInputActionValue& /*ActionValue*/)
{
	if (SingleTouchCheck())
	{
		TrackMove();
	}
	else
	{
		SetDragMoveMappingContext(false);
	}
}

void ACropoutPlayerPawn::SetDragMoveMappingContext(bool bEnabled)
{
	APlayerController* PlayerController = GetPlayerController();
	ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	UInputMappingContext* DragMoveContext = DragMoveInputMappingContext
		? DragMoveInputMappingContext.Get()
		: LoadInputMappingContext(TEXT("/Game/Blueprint/Core/Player/Input/IMC_DragMove.IMC_DragMove"));
	if (!InputSubsystem || !DragMoveContext)
	{
		return;
	}

	FModifyContextOptions Options;
	Options.bIgnoreAllPressedKeysUntilRelease = true;
	Options.bForceImmediately = false;
	Options.bNotifyUserSettings = false;

	if (bEnabled)
	{
		InputSubsystem->AddMappingContext(DragMoveContext, 0, Options);
	}
	else
	{
		InputSubsystem->RemoveMappingContext(DragMoveContext, Options);
	}
}
