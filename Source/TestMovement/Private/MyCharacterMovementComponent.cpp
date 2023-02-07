// Fill out your copyright notice in the Description page of Project Settings.


#include "MyCharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/ScopedMovementUpdate.h"

#include "GameFramework/Character.h"


#pragma region Saved Move

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
	Saved_bPrevWantsToCrouch = CharacterMovement->Safe_bPrevWantsToCrouch;
}

//Take data in savedmove (from SetMove) and apply it to the current iteration of character
void UMyCharacterMovementComponent::FSavedMove_MyCharacter::PrepMoveFor(ACharacter* C)
{
	FSavedMove_Character::PrepMoveFor(C);

	UMyCharacterMovementComponent* CharacterMovement = Cast<UMyCharacterMovementComponent>(C->GetCharacterMovement());

	CharacterMovement->Safe_bWantsToSprint = Saved_bWantsToSprint;
	CharacterMovement->Safe_bPrevWantsToCrouch = Saved_bPrevWantsToCrouch;
}

void UMyCharacterMovementComponent::FSavedMove_MyCharacter::PostUpdate(ACharacter* C, EPostUpdateMode PostUpdateMode)
{
	Super::PostUpdate(C, PostUpdateMode);

	//SavedControlRotation = C->GetActorRotation();
}

#pragma endregion

#pragma region Client Network Prediction Data

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

#pragma endregion

#pragma region CMC
UMyCharacterMovementComponent::UMyCharacterMovementComponent()
{
	NavAgentProps.bCanCrouch = true;

	SetNetworkMoveDataContainer(MyDefaultNetworkMoveDataContainer);
}

void UMyCharacterMovementComponent::InitializeComponent()
{
	Super::InitializeComponent();

	TestMovmentCharacterOwner = Cast<ATestMovementCharacter>(GetOwner());
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
	Safe_bPrevWantsToCrouch = bWantsToCrouch;
}

void UMyCharacterMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	if (MovementMode == MOVE_Walking && !bWantsToCrouch && Safe_bPrevWantsToCrouch)
	{
		FHitResult PotentialSlideSurface;
		if (Velocity.SizeSquared() > pow(Slide_MinSpeed, 2) && GetSlideSurface(PotentialSlideSurface))
		{
			EnterSlide();
		}
	}

	if (IsCustomMovementMode(CMOVE_Slide) && !bWantsToCrouch)
	{
		SetMovementMode(MOVE_Walking);
	}

	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
}

bool UMyCharacterMovementComponent::IsMovingOnGround() const
{
	return Super::IsMovingOnGround() || IsCustomMovementMode(CMOVE_Slide);
}

bool UMyCharacterMovementComponent::CanCrouchInCurrentState() const
{
	return Super::CanCrouchInCurrentState() && IsMovingOnGround();
}

void UMyCharacterMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	Super::PhysCustom(deltaTime, Iterations);
	Super::PhysCustom(deltaTime, Iterations);

	switch (CustomMovementMode)
	{
	case CMOVE_Slide:
		PhysSlide(deltaTime, Iterations);
		break;
	default:
		UE_LOG(LogTemp, Fatal, TEXT("Invalid Movement Mode"))
	}
}

void UMyCharacterMovementComponent::EnterSlide()
{
	bWantsToCrouch = true;
	Velocity += Velocity.GetSafeNormal2D() * Slide_EnterImpulse;
	SetMovementMode(MOVE_Custom, CMOVE_Slide);
}

//apparently this is safe and thats important
void UMyCharacterMovementComponent::ExitSlide()
{

	bWantsToCrouch = false;

	FQuat NewRotation = FRotationMatrix::MakeFromXZ(UpdatedComponent->GetForwardVector().GetSafeNormal2D(), FVector::UpVector).ToQuat();
	FHitResult Hit;
	SafeMoveUpdatedComponent(FVector::ZeroVector, NewRotation, true, Hit);
	SetMovementMode(MOVE_Walking);
}


bool UMyCharacterMovementComponent::GetSlideSurface(FHitResult& Hit) const
{
	FVector Start = UpdatedComponent->GetComponentLocation();
	FVector End = Start + CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 2.f * FVector::DownVector;
	FName ProfileName = TEXT("BlockAll");
	return GetWorld()->LineTraceSingleByProfile(Hit, Start, End, ProfileName, TestMovmentCharacterOwner->GetIgnoreCharacterParams());
}

void UMyCharacterMovementComponent::PhysSlide(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}

	RestorePreAdditiveRootMotionVelocity();

	FHitResult SurfaceHit;
	if (!GetSlideSurface(SurfaceHit) || Velocity.SizeSquared() < pow(Slide_MinSpeed, 2))
	{
		ExitSlide();
		StartNewPhysics(deltaTime, Iterations);
		return;
	}

	// Surface Gravity
	Velocity += Slide_GravityForce * FVector::DownVector * deltaTime;

	// Strafe
	if (FMath::Abs(FVector::DotProduct(Acceleration.GetSafeNormal(), UpdatedComponent->GetRightVector())) > .5)
	{
		Acceleration = Acceleration.ProjectOnTo(UpdatedComponent->GetRightVector());
	}
	else
	{
		Acceleration = FVector::ZeroVector;
	}

	// Calc Velocity
	if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		CalcVelocity(deltaTime, Slide_Friction, true, GetMaxBrakingDeceleration());
	}
	ApplyRootMotionToVelocity(deltaTime);

	// Perform Move
	Iterations++;
	bJustTeleported = false;

	FVector OldLocation = UpdatedComponent->GetComponentLocation();
	FQuat OldRotation = UpdatedComponent->GetComponentRotation().Quaternion();
	FHitResult Hit(1.f);
	FVector Adjusted = Velocity * deltaTime;
	FVector VelPlaneDir = FVector::VectorPlaneProject(Velocity, SurfaceHit.Normal).GetSafeNormal();
	FQuat NewRotation = FRotationMatrix::MakeFromXZ(VelPlaneDir, SurfaceHit.Normal).ToQuat();
	SafeMoveUpdatedComponent(Adjusted, NewRotation, true, Hit);

	if (Hit.Time < 1.f)
	{
		HandleImpact(Hit, deltaTime, Adjusted);
		SlideAlongSurface(Adjusted, (1.f - Hit.Time), Hit.Normal, Hit, true);
	}

	FHitResult NewSurfaceHit;
	if (!GetSlideSurface(NewSurfaceHit) || Velocity.SizeSquared() < pow(Slide_MinSpeed, 2))
	{
		ExitSlide();
	}

	// Update Outgoing Velocity & Acceleration
	if (!bJustTeleported && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / deltaTime;
	}
}

bool UMyCharacterMovementComponent::IsCustomMovementMode(ECustomMovementMode InCustomMovementMode) const
{
	return MovementMode == MOVE_Custom && CustomMovementMode == InCustomMovementMode;
}

#pragma endregion


#pragma region Input

void UMyCharacterMovementComponent::SprintPressed()
{
	Safe_bWantsToSprint = true;
}

void UMyCharacterMovementComponent::SprintReleased()
{
	Safe_bWantsToSprint = false;
}

void UMyCharacterMovementComponent::CrouchPressed()
{
	bWantsToCrouch = ~bWantsToCrouch;
}
#pragma endregion












/*

//this is a dangerous mess, do not use
 void UMyCharacterMovementComponent::ServerMove_PerformMovement(const FCharacterNetworkMoveData& MoveData)
{
//	SCOPE_CYCLE_COUNTER(STAT_CharacterMovementServerMove);
//	CSV_SCOPED_TIMING_STAT(CharacterMovement, CharacterMovementServerMove);

	if (!HasValidData() || !IsActive())
	{
		return;
	}

	const float ClientTimeStamp = MoveData.TimeStamp;
	FVector_NetQuantize10 ClientAccel = MoveData.Acceleration;
	const uint8 ClientMoveFlags = MoveData.CompressedMoveFlags;
	const FRotator ClientControlRotation = MoveData.ControlRotation;

	FNetworkPredictionData_Server_Character* ServerData = GetPredictionData_Server_Character();
	check(ServerData);

	if (!VerifyClientTimeStamp(ClientTimeStamp, *ServerData))
	{
		const float ServerTimeStamp = ServerData->CurrentClientTimeStamp;
		// This is more severe if the timestamp has a large discrepancy and hasn't been recently reset.
		if (ServerTimeStamp > 1.0f && FMath::Abs(ServerTimeStamp - ClientTimeStamp) > 1.0f)
		{
			UE_LOG(LogNetPlayerMovement, Warning, TEXT("ServerMove: TimeStamp expired: %f, CurrentTimeStamp: %f, Character: %s"), ClientTimeStamp, ServerTimeStamp, *GetNameSafe(CharacterOwner));
		}
		else
		{
			UE_LOG(LogNetPlayerMovement, Log, TEXT("ServerMove: TimeStamp expired: %f, CurrentTimeStamp: %f, Character: %s"), ClientTimeStamp, ServerTimeStamp, *GetNameSafe(CharacterOwner));
		}
		return;
	}

	bool bServerReadyForClient = true;
	APlayerController* PC = Cast<APlayerController>(CharacterOwner->GetController());
	if (PC)
	{
		bServerReadyForClient = PC->NotifyServerReceivedClientData(CharacterOwner, ClientTimeStamp);
		if (!bServerReadyForClient)
		{
			ClientAccel = FVector::ZeroVector;
		}
	}

	const UWorld* MyWorld = GetWorld();
	const float DeltaTime = ServerData->GetServerMoveDeltaTime(ClientTimeStamp, CharacterOwner->GetActorTimeDilation(*MyWorld));

	if (DeltaTime > 0.f)
	{
		ServerData->CurrentClientTimeStamp = ClientTimeStamp;
		ServerData->ServerAccumulatedClientTimeStamp += DeltaTime;
		ServerData->ServerTimeStamp = MyWorld->GetTimeSeconds();
		ServerData->ServerTimeStampLastServerMove = ServerData->ServerTimeStamp;

		if (AController* CharacterController = Cast<AController>(CharacterOwner->GetController()))
		{
		//	CharacterOwner->SetActorRotation(ClientControlRotation);
			CharacterController->SetControlRotation(ClientControlRotation);
		}

		if (!bServerReadyForClient)
		{
			return;
		}

		// Perform actual movement
		if ((MyWorld->GetWorldSettings()->GetPauserPlayerState() == NULL))
		{
			if (PC)
			{
				PC->UpdateRotation(DeltaTime);
			}

			MoveAutonomous(ClientTimeStamp, DeltaTime, ClientMoveFlags, ClientAccel);
		}

		UE_CLOG(CharacterOwner && UpdatedComponent, LogNetPlayerMovement, VeryVerbose, TEXT("ServerMove Time %f Acceleration %s Velocity %s Position %s Rotation %s DeltaTime %f Mode %s MovementBase %s.%s (Dynamic:%d)"),
			ClientTimeStamp, *ClientAccel.ToString(), *Velocity.ToString(), *UpdatedComponent->GetComponentLocation().ToString(), *UpdatedComponent->GetComponentRotation().ToCompactString(), DeltaTime, *GetMovementName(),
			*GetNameSafe(GetMovementBase()), *CharacterOwner->GetBasedMovement().BoneName.ToString(), MovementBaseUtility::IsDynamicBase(GetMovementBase()) ? 1 : 0);
	}

	// Validate move only after old and first dual portion, after all moves are completed.
	if (MoveData.NetworkMoveType == FCharacterNetworkMoveData::ENetworkMoveType::NewMove)
	{
		ServerMoveHandleClientError(ClientTimeStamp, DeltaTime, ClientAccel, MoveData.Location, MoveData.MovementBase, MoveData.MovementBaseBoneName, MoveData.MovementMode);
	}
}

void UMyCharacterMovementComponent::ServerMove_HandleMoveData(const FCharacterNetworkMoveDataContainer& MoveDataContainer)
{
	// Optional "old move"
	if (MoveDataContainer.bHasOldMove)
	{
		if (FCharacterNetworkMoveData* OldMove = MoveDataContainer.GetOldMoveData())
		{
			SetCurrentNetworkMoveData(OldMove);
			ServerMove_PerformMovement(*OldMove);
		}
	}

	// Optional scoped movement update for dual moves to combine moves for cheaper performance on the server.
	const bool bMoveAllowsScopedDualMove = MoveDataContainer.bHasPendingMove && !MoveDataContainer.bDisableCombinedScopedMove;
	FScopedMovementUpdate ScopedMovementUpdate(UpdatedComponent, (bMoveAllowsScopedDualMove && bEnableServerDualMoveScopedMovementUpdates && bEnableScopedMovementUpdates) ? EScopedUpdate::DeferredUpdates : EScopedUpdate::ImmediateUpdates);

	// Optional pending move as part of "dual move"
	if (MoveDataContainer.bHasPendingMove)
	{
		if (FCharacterNetworkMoveData* PendingMove = MoveDataContainer.GetPendingMoveData())
		{
			CharacterOwner->bServerMoveIgnoreRootMotion = MoveDataContainer.bIsDualHybridRootMotionMove && CharacterOwner->IsPlayingNetworkedRootMotionMontage();
			SetCurrentNetworkMoveData(PendingMove);
			ServerMove_PerformMovement(*PendingMove);
			CharacterOwner->bServerMoveIgnoreRootMotion = false;
		}
	}

	// Final standard move
	if (FCharacterNetworkMoveData* NewMove = MoveDataContainer.GetNewMoveData())
	{
		FVector_NetQuantize Accel = NewMove->Acceleration;
		FRotator Control = NewMove->ControlRotation;

			if (GEngine) {
				//GEngine->AddOnScreenDebugMessage(-1, .01f, FColor::Red, FString::Printf(TEXT("Accel.x = %f, Accel.y = %f, Accel.z = %f"), Accel.X, Accel.Y, Accel.Z));
				//GEngine->AddOnScreenDebugMessage(-1, .01f, FColor::Red, FString::Printf(TEXT("Control.Pitch = %f, Control.Yaw = %f, Control.Roll = %f"), Control.Pitch, Control.Yaw, Control.Roll));
			}

		SetCurrentNetworkMoveData(NewMove);
		ServerMove_PerformMovement(*NewMove);
	}

	SetCurrentNetworkMoveData(nullptr);
}

*/