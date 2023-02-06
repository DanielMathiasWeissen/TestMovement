// Fill out your copyright notice in the Description page of Project Settings.


#include "MyCharacterMovementComponent.h"
#include "MyCharacterMovementReplication.h"


void FMyCharacterNetworkMoveData::ClientFillNetworkMoveData(const FSavedMove_Character& ClientMove, ENetworkMoveType MoveType)
{
	NetworkMoveType = MoveType;

	TimeStamp = ClientMove.TimeStamp;
	Acceleration = ClientMove.Acceleration;
	ControlRotation = ClientMove.SavedControlRotation;
	CompressedMoveFlags = ClientMove.GetCompressedFlags();
	MovementMode = ClientMove.EndPackedMovementMode;

	// Location, relative movement base, and ending movement mode is only used for error checking, so only fill in the more complex parts if actually required.
	if (MoveType == ENetworkMoveType::NewMove)
	{
		// Determine if we send absolute or relative location
		UPrimitiveComponent* ClientMovementBase = ClientMove.EndBase.Get();
		const bool bDynamicBase = MovementBaseUtility::UseRelativeLocation(ClientMovementBase);
		const FVector SendLocation = bDynamicBase ? ClientMove.SavedRelativeLocation : FRepMovement::RebaseOntoZeroOrigin(ClientMove.SavedLocation, ClientMove.CharacterOwner->GetCharacterMovement());

		Location = SendLocation;
		MovementBase = bDynamicBase ? ClientMovementBase : nullptr;
		MovementBaseBoneName = bDynamicBase ? ClientMove.EndBoneName : NAME_None;
	}
	else
	{
		Location = ClientMove.SavedLocation;
		MovementBase = nullptr;
		MovementBaseBoneName = NAME_None;
	}
}