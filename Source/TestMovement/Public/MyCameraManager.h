// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "MyCameraManager.generated.h"

/**
 * 
 */
UCLASS()
class TESTMOVEMENT_API AMyCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly) float CrouchBlendDuration = .2f;
	float CrouchBlendTime;

public:
	AMyCameraManager();

	virtual void UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime) override;

	void AdjustCameraForCrouching(FTViewTarget& OutVT, float DeltaTime);
	
};
