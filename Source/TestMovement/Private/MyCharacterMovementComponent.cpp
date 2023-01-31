// Fill out your copyright notice in the Description page of Project Settings.


#include "MyCharacterMovementComponent.h"

#include "GameFramework/Character.h"




UMyCharacterMovementComponent::UMyCharacterMovementComponent()
{
}
//combine moves if they are identical -> optimization
bool UMyCharacterMovementComponent::FSavedMove_MyCharacter::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const
{
	const FSavedMove_MyCharacter* NewZippyMove = static_cast<FSavedMove_MyCharacter*>(NewMove.Get());
	
	if (Saved_bWantsToSprint != NewZippyMove->Saved_bWantsToSprint)
	{
		return false;
	}

	return FSavedMove_Character::CanCombineWith(NewMove, InCharacter, MaxDelta);
}

//clear move, they are stored in array, free up space for new move
void UMyCharacterMovementComponent::FSavedMove_MyCharacter::Clear()
{
	Saved_bWantsToSprint = 0;

	FSavedMove_Character::Clear();
}

//This is what we actually send to the server, 8 Flags, 4 Custom ones we can use
uint8 UMyCharacterMovementComponent::FSavedMove_MyCharacter::GetCompressedFlags() const
{
	uint8 Result = FSavedMove_Character::GetCompressedFlags();

	if (Saved_bWantsToSprint) Result |= FLAG_Custom_0;

	return Result;
}

//Sets saved move for current iteration of character
void UMyCharacterMovementComponent::FSavedMove_MyCharacter::SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
	FSavedMove_Character::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);

	UMyCharacterMovementComponent* CharacterMovement = Cast<UMyCharacterMovementComponent>(C->GetCharacterMovement());

	Saved_bWantsToSprint = CharacterMovement->Safe_bWantsToSprint;
}

//Take data in savedmove (from SetMove) and apply it to the current iteration of character
void UMyCharacterMovementComponent::FSavedMove_MyCharacter::PrepMoveFor(ACharacter* C)
{
	FSavedMove_Character::PrepMoveFor(C);

	UMyCharacterMovementComponent* CharacterMovement = Cast<UMyCharacterMovementComponent>(C->GetCharacterMovement());

	CharacterMovement->Safe_bWantsToSprint = Saved_bWantsToSprint;
}

UMyCharacterMovementComponent::FNetworkPredictionData_Client_MyCharacter::FNetworkPredictionData_Client_MyCharacter(const UCharacterMovementComponent& ClientMovement) : Super(ClientMovement)
{

}

//use our custom savedMove
FSavedMovePtr UMyCharacterMovementComponent::FNetworkPredictionData_Client_MyCharacter::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_MyCharacter);
}

//use our custom FNetworkPredictionData_Client_MyCharacter
FNetworkPredictionData_Client* UMyCharacterMovementComponent::GetPredictionData_Client() const
{
	//Not in Unreal sourcecode, from video https://www.youtube.com/watch?v=17D4SzewYZ0
	check(PawnOwner != nullptr)

		if (ClientPredictionData == nullptr)
		{
			UMyCharacterMovementComponent* MutableThis = const_cast<UMyCharacterMovementComponent*>(this);

			MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_MyCharacter(*this);
			//Not in Unreal sourcecode, from video https://www.youtube.com/watch?v=17D4SzewYZ0
			MutableThis->ClientPredictionData->MaxSmoothNetUpdateDist = 92.f;
			MutableThis->ClientPredictionData->NoSmoothNetUpdateDist = 140.f;
		}
	return ClientPredictionData;
}

//Set state of movementComponent based on Flags
void UMyCharacterMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);

	//set equal to value of FLAG_Custom_0
	Safe_bWantsToSprint = (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
}

void UMyCharacterMovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity)
{
	Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);
	
	if (MovementMode == MOVE_Walking)
	{
		if (Safe_bWantsToSprint)
		{
			MaxWalkSpeed = Sprint_MaxWalkSpeed;
		}
		else
		{
			MaxWalkSpeed = Walk_MaxWalkSpeed;
		}
	}
}

void UMyCharacterMovementComponent::SprintPressed()
{
	Safe_bWantsToSprint = true;
}

void UMyCharacterMovementComponent::SprintReleased()
{
	Safe_bWantsToSprint = false;
}
