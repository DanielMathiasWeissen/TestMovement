// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TestMovementCharacter.h"

struct FMyCharacterNetworkMoveData : public FCharacterNetworkMoveData {
	typedef FCharacterNetworkMoveData Super;

	FMyCharacterNetworkMoveData()
	{
		Super::FCharacterNetworkMoveData();
	}

	virtual void ClientFillNetworkMoveData(const FSavedMove_Character& ClientMove, ENetworkMoveType MoveType) override;
};

struct FMyFCharacterNetworkMoveDataContainer : public FCharacterNetworkMoveDataContainer {

	typedef FCharacterNetworkMoveDataContainer Super;

	FMyFCharacterNetworkMoveDataContainer()
	{
		Super::FCharacterNetworkMoveDataContainer();
		NewMoveData = &MyBaseDefaultMoveData[0];
		PendingMoveData = &MyBaseDefaultMoveData[1];
		OldMoveData = &MyBaseDefaultMoveData[2];
	}


//	virtual void ClientFillNetworkMoveData(const FSavedMove_Character* ClientNewMove, const FSavedMove_Character* ClientPendingMove, const FSavedMove_Character* ClientOldMove);

private:

	FMyCharacterNetworkMoveData MyBaseDefaultMoveData[3];
};

