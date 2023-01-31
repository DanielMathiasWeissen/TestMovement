// Fill out your copyright notice in the Description page of Project Settings.


#include "MyCameraManager.h"
#include "TestMovementCharacter.h"
#include "MyCharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"


AMyCameraManager::AMyCameraManager()
{
}

void AMyCameraManager::UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime)
{
	Super::UpdateViewTarget(OutVT, DeltaTime);

	AdjustCameraForCrouching(OutVT, DeltaTime);
}

void AMyCameraManager::AdjustCameraForCrouching(FTViewTarget& OutVT, float DeltaTime)
{
	if (ATestMovementCharacter* TestMovementCharacter = Cast<ATestMovementCharacter>(GetOwningPlayerController()->GetPawn()))
	{
		UMyCharacterMovementComponent* ZMC = TestMovementCharacter->GetMyCharacterMovementComponent();
		FVector TargetCrouchOffset = FVector(
			0,
			0,
			ZMC->GetCrouchedHalfHeight() - TestMovementCharacter->GetClass()->GetDefaultObject<ACharacter>()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()
		);
		FVector Offset = FMath::Lerp(FVector::ZeroVector, TargetCrouchOffset, FMath::Clamp(CrouchBlendTime / CrouchBlendDuration, 0.f, 1.f));

		if (ZMC->IsCrouching())
		{
			CrouchBlendTime = FMath::Clamp(CrouchBlendTime + DeltaTime, 0.f, CrouchBlendDuration);
			Offset -= TargetCrouchOffset;
		}
		else
		{
			CrouchBlendTime = FMath::Clamp(CrouchBlendTime - DeltaTime, 0.f, CrouchBlendDuration);
		}

		if (ZMC->IsMovingOnGround())
		{
			OutVT.POV.Location += Offset;
		}
	}
}
