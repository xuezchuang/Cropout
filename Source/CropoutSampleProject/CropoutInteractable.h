// Copyright (c) Cropout contributors. Licensed under the project license.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include <initializer_list>

#include "CropoutInteractable.generated.h"

class UBoxComponent;
class UMaterialInterface;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UTextureRenderTarget2D;

/**
 * Native parent for BP_Interactable.
 */
UCLASS(Blueprintable)
class CROPOUTSAMPLEPROJECT_API ACropoutInteractable : public AActor
{
	GENERATED_BODY()

public:
	ACropoutInteractable();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cropout|Interactable|Components")
	TObjectPtr<USceneComponent> NativeScene;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cropout|Interactable|Components")
	TObjectPtr<UStaticMeshComponent> NativeMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cropout|Interactable|Components")
	TObjectPtr<UBoxComponent> NativeBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cropout|Interactable")
	double BoundGap = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cropout|Interactable", meta = (DisplayName = "Require Build"))
	bool RequireBuild = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cropout|Interactable", meta = (DisplayName = "RT_Draw"))
	TObjectPtr<UTextureRenderTarget2D> RT_Draw;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cropout|Interactable")
	double OutlineDraw = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cropout|Interactable", meta = (DisplayName = "Enable Ground Blend"))
	bool EnableGroundBlend = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cropout|Interactable", meta = (DisplayName = "Progression State"))
	double ProgressionState = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cropout|Interactable", meta = (DisplayName = "Mesh List"))
	TArray<TObjectPtr<UStaticMesh>> MeshList;

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Cropout|Interactable", meta = (DisplayName = "Placement Mode"))
	void PlacementMode();
	virtual void PlacementMode_Implementation();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Cropout|Interactable", meta = (DisplayName = "Set Progressions State"))
	void SetProgressionsState(float Progression);
	virtual void SetProgressionsState_Implementation(float Progression);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Cropout|Interactable")
	void Interact(UPARAM(DisplayName = "New Param") float& NewParam);
	virtual void Interact_Implementation(float& NewParam);

	UFUNCTION(BlueprintCallable, Category = "Cropout|Interactable", meta = (DisplayName = "Play Wobble"))
	void PlayWobble(FVector NewParam);

	UFUNCTION(BlueprintCallable, Category = "Cropout|Interactable", meta = (DisplayName = "End Wobble"))
	void EndWobble();

protected:
	UStaticMeshComponent* GetMeshComponent() const;
	UBoxComponent* GetBoxComponent() const;

	bool GetBoolPropertyValue(std::initializer_list<const TCHAR*> Names, bool DefaultValue = false) const;
	void SetBoolPropertyValue(std::initializer_list<const TCHAR*> Names, bool Value);
	int32 GetIntPropertyValue(std::initializer_list<const TCHAR*> Names, int32 DefaultValue = 0) const;
	void SetIntPropertyValue(std::initializer_list<const TCHAR*> Names, int32 Value);
	float GetFloatPropertyValue(std::initializer_list<const TCHAR*> Names, float DefaultValue = 0.0f) const;
	void SetFloatPropertyValue(std::initializer_list<const TCHAR*> Names, float Value);
	UObject* GetObjectPropertyValue(std::initializer_list<const TCHAR*> Names) const;
	TArray<UStaticMesh*> GetStaticMeshArrayPropertyValue(std::initializer_list<const TCHAR*> Names) const;

private:
	void HandleBeginPlayNextTick();
	void DrawGroundBlend();
	void DestroyDuplicateOverlaps();

	void TransformToTexture(const FVector2D& InSize, FVector2D& OutPosition, FVector2D& OutSize) const;
	void SetWobbleValue(float Value);
	void UpdateWobble();
	void StartWobbleTimer();

	FTimerHandle WobbleTimerHandle;
	float CurrentWobbleValue = 0.0f;
	float TargetWobbleValue = 0.0f;
};
