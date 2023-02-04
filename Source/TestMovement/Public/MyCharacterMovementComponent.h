// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TestMovementCharacter.h"


#include "MyCharacterMovementComponent.generated.h"


UENUM(BlueprintType)
enum ECustomMovementMode
{
	CMOVE_None			UMETA(Hidden),
	CMOVE_Slide			UMETA(DisplayName = "Slide"),
	CMOVE_MAX			UMETA(Hidden),
};

/**
 * 
 */
UCLASS()
class TESTMOVEMENT_API UMyCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()


	class FSavedMove_MyCharacter : public FSavedMove_Character
	{

		typedef FSavedMove_Character Super;

		uint8 Saved_bWantsToSprint : 1;

		virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override;
		virtual void Clear() override;
		virtual uint8 GetCompressedFlags() const override;
		virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData) override;
		virtual void PrepMoveFor(ACharacter* C) override;
	};

	class FNetworkPredictionData_Client_MyCharacter : public FNetworkPredictionData_Client_Character
	{
	public:
		FNetworkPredictionData_Client_MyCharacter(const UCharacterMovementComponent& ClientMovement);

		typedef FNetworkPredictionData_Client_Character Super;

		virtual FSavedMovePtr AllocateNewMove() override;
	};

	UPROPERTY(EditDefaultsOnly) float Sprint_MaxWalkSpeed;
	UPROPERTY(EditDefaultsOnly) float Walk_MaxWalkSpeed;

	UPROPERTY(EditDefaultsOnly) float MinSlideSpeed = 400.f;
	UPROPERTY(EditDefaultsOnly) float MaxSlideSpeed = 400.f;
	UPROPERTY(EditDefaultsOnly) float SlideEnterImpulse = 400.f;
	UPROPERTY(EditDefaultsOnly) float SlideGravityForce = 4000.f;
	UPROPERTY(EditDefaultsOnly) float SlideFrictionFactor = .06f;
	UPROPERTY(EditDefaultsOnly) float BrakingDecelerationSliding = 1000.f;

	UPROPERTY(Transient) ATestMovementCharacter* TestMovmentCharacterOwner;

	//safe variable can be called from unfase function on client, not on server
	//unsafe variable can not be used in safe function
	bool Safe_bWantsToSprint;

public:
	UMyCharacterMovementComponent();

protected:
	virtual void InitializeComponent() override;

public:
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
protected:
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;

	//called at end of every perform move, allows us to write movement logic regardless of movement mode
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;


private:
	void EnterSlide(EMovementMode PrevMode, ECustomMovementMode PrevCustomMode);
	void ExitSlide();
	bool CanSlide() const;
	//important function that defines movementmode
	void PhysSlide(float deltaTime, int32 Iterations);

public:
	UFUNCTION(BlueprintCallable) void SprintPressed();
	UFUNCTION(BlueprintCallable) void SprintReleased();

	UFUNCTION(BlueprintCallable) void CrouchPressed();

	UFUNCTION(BlueprintPure) bool IsCustomMovementMode(ECustomMovementMode InCustomMovementMode) const;

	void ServerMove_HandleMoveData(const FCharacterNetworkMoveDataContainer& MoveDataContainer) override;
	
};
