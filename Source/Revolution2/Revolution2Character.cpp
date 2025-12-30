// Copyright Epic Games, Inc. All Rights Reserved.

#include "Revolution2Character.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Revolution2.h"

ARevolution2Character::ARevolution2Character()
{
	// Enable tick for top down aim updates
	PrimaryActorTick.bCanEverTick = true;
	
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
	
	// Create the first person mesh that will be viewed only by this character's owner
	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));

	FirstPersonMesh->SetupAttachment(GetMesh());
	FirstPersonMesh->SetOnlyOwnerSee(true);
	FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));

	// Create the Camera Component	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
	FirstPersonCameraComponent->SetupAttachment(FirstPersonMesh, FName("head"));
	FirstPersonCameraComponent->SetRelativeLocationAndRotation(FVector(-2.8f, 5.89f, 0.0f), FRotator(0.0f, 90.0f, -90.0f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
	FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
	FirstPersonCameraComponent->bEnableFirstPersonScale = true;
	FirstPersonCameraComponent->FirstPersonFieldOfView = 70.0f;
	FirstPersonCameraComponent->FirstPersonScale = 0.6f;

	// configure the character comps
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;

	GetCapsuleComponent()->SetCapsuleSize(34.0f, 96.0f);

	// Configure character movement
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
	GetCharacterMovement()->AirControl = 0.5f;

	// Create top down spring arm
	TopDownSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Top Down Spring Arm"));
	TopDownSpringArm->SetupAttachment(GetCapsuleComponent());
	TopDownSpringArm->bUsePawnControlRotation = false;
	TopDownSpringArm->bInheritPitch = false;
	TopDownSpringArm->bInheritYaw = false;
	TopDownSpringArm->bInheritRoll = false;
	TopDownSpringArm->bDoCollisionTest = false;

	// Create top down camera
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Top Down Camera"));
	TopDownCameraComponent->SetupAttachment(TopDownSpringArm);
	TopDownCameraComponent->bUsePawnControlRotation = false;
	TopDownCameraComponent->FieldOfView = 90.0f;
	
	// Initially deactivate top down camera (first person is default)
	TopDownCameraComponent->SetActive(false);

	// Initialize view mode
	CurrentViewMode = EViewMode::FirstPerson;
}

void ARevolution2Character::BeginPlay()
{
	Super::BeginPlay();

	// Ensure components are valid
	if (!FirstPersonCameraComponent)
	{
		UE_LOG(LogRevolution2, Error, TEXT("FirstPersonCameraComponent is null in BeginPlay!"));
	}
	if (!FirstPersonMesh)
	{
		UE_LOG(LogRevolution2, Error, TEXT("FirstPersonMesh is null in BeginPlay!"));
	}
	if (!TopDownCameraComponent)
	{
		UE_LOG(LogRevolution2, Error, TEXT("TopDownCameraComponent is null in BeginPlay!"));
	}

	ApplyTopDownCameraSettings();

	// Initialize view mode
	SetViewMode(CurrentViewMode);
}

void ARevolution2Character::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyTopDownCameraSettings();
}

void ARevolution2Character::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// In top down mode, continuously update character rotation based on mouse position
	if (CurrentViewMode == EViewMode::TopDown)
	{
		UpdateTopDownAim();
		UpdateClickMove();
	}
}

void ARevolution2Character::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ARevolution2Character::DoJumpStart);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ARevolution2Character::DoJumpEnd);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ARevolution2Character::MoveInput);

		// Looking/Aiming
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ARevolution2Character::LookInput);
		EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &ARevolution2Character::LookInput);

		// Toggle view mode
		if (ToggleViewAction)
		{
			EnhancedInputComponent->BindAction(ToggleViewAction, ETriggerEvent::Started, this, &ARevolution2Character::ToggleViewMode);
			UE_LOG(LogRevolution2, Warning, TEXT("ToggleViewAction bound successfully"));
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, TEXT("ToggleViewAction 输入绑定成功"));
			}
		}
		else
		{
			UE_LOG(LogRevolution2, Error, TEXT("ToggleViewAction is null! Please assign it in the character blueprint."));
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("错误: ToggleViewAction 未设置！请在角色蓝图中分配 Input Action"));
			}
		}

		// Click move for top down mode
		if (ClickMoveAction)
		{
			EnhancedInputComponent->BindAction(ClickMoveAction, ETriggerEvent::Started, this, &ARevolution2Character::OnClickMove);
		}
	}
	else
	{
		UE_LOG(LogRevolution2, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}


void ARevolution2Character::MoveInput(const FInputActionValue& Value)
{
	// get the Vector2D move axis
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (CurrentViewMode == EViewMode::TopDown && !MovementVector.IsNearlyZero())
	{
		bHasClickMoveTarget = false;
	}

	// pass the axis values to the move input
	DoMove(MovementVector.X, MovementVector.Y);

}

void ARevolution2Character::LookInput(const FInputActionValue& Value)
{
	// get the Vector2D look axis
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// pass the axis values to the aim input
	DoAim(LookAxisVector.X, LookAxisVector.Y);

}

void ARevolution2Character::DoAim(float Yaw, float Pitch)
{
	if (GetController())
	{
		if (CurrentViewMode == EViewMode::FirstPerson)
		{
			// First person mode: rotate camera
			AddControllerYawInput(Yaw);
			AddControllerPitchInput(Pitch);
		}
		else
		{
			// Top down mode: update character rotation based on mouse position
			UpdateTopDownAim();
		}
	}
}

void ARevolution2Character::DoMove(float Right, float Forward)
{
	if (GetController())
	{
		if (CurrentViewMode == EViewMode::FirstPerson)
		{
			// First person mode: move relative to character orientation
			AddMovementInput(GetActorRightVector(), Right);
			AddMovementInput(GetActorForwardVector(), Forward);
		}
		else
		{
			// Top down mode: move in world space (fixed directions)
			AddMovementInput(FVector::RightVector, Right);
			AddMovementInput(FVector::ForwardVector, Forward);
		}
	}
}

void ARevolution2Character::DoJumpStart()
{
	// pass Jump to the character
	Jump();
}

void ARevolution2Character::DoJumpEnd()
{
	// pass StopJumping to the character
	StopJumping();
}

void ARevolution2Character::ToggleViewMode()
{
	UE_LOG(LogRevolution2, Warning, TEXT("ToggleViewMode called, current mode: %d"), (int32)CurrentViewMode);
	
	if (CurrentViewMode == EViewMode::FirstPerson)
	{
		UE_LOG(LogRevolution2, Warning, TEXT("Switching to TopDown view mode"));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, TEXT("切换到俯视角模式"));
		}
		SetViewMode(EViewMode::TopDown);
	}
	else
	{
		UE_LOG(LogRevolution2, Warning, TEXT("Switching to FirstPerson view mode"));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, TEXT("切换到第一人称模式"));
		}
		SetViewMode(EViewMode::FirstPerson);
	}
}

void ARevolution2Character::SetViewMode(EViewMode NewViewMode)
{
	UE_LOG(LogRevolution2, Warning, TEXT("SetViewMode called with: %d"), (int32)NewViewMode);
	CurrentViewMode = NewViewMode;
	bHasClickMoveTarget = false;
	bHasTopDownAimLocation = false;

	if (CurrentViewMode == EViewMode::FirstPerson)
	{
		ApplyFirstPersonView(Cast<APlayerController>(GetController()));
	}
	else // TopDown
	{
		ApplyTopDownView(Cast<APlayerController>(GetController()));
	}

	OnViewModeChanged.Broadcast(CurrentViewMode);
}

bool ARevolution2Character::GetTopDownAimLocation(FVector& OutAimLocation) const
{
	if (!bHasTopDownAimLocation)
	{
		return false;
	}

	OutAimLocation = TopDownAimLocation;
	return true;
}

void ARevolution2Character::UpdateTopDownAim()
{
	if (CurrentViewMode != EViewMode::TopDown || !GetController())
	{
		return;
	}

	FVector AimLocation;
	if (!TryGetTopDownAimLocation(AimLocation))
	{
		return;
	}

	SetTopDownAimLocation(AimLocation);

	// Calculate direction from character to hit point
	FVector CharacterLocation = GetActorLocation();
	FVector Direction = (AimLocation - CharacterLocation);
	Direction.Z = 0.0f;
	Direction.Normalize();

	if (!Direction.IsNearlyZero())
	{
		// Calculate Yaw rotation (only horizontal rotation)
		FRotator TargetRotation = Direction.Rotation();
		FRotator NewRotation = FRotator(0.0f, TargetRotation.Yaw, 0.0f);
		GetController()->SetControlRotation(NewRotation);
	}
}

void ARevolution2Character::UpdateClickMove()
{
	if (CurrentViewMode != EViewMode::TopDown || !bHasClickMoveTarget)
	{
		return;
	}

	const FVector CurrentLocation = GetActorLocation();
	const float DistanceToTarget = FVector::Dist(CurrentLocation, ClickMoveTargetLocation);
	if (DistanceToTarget <= ClickMoveAcceptanceRadius)
	{
		bHasClickMoveTarget = false;
		return;
	}

	MoveToLocation(ClickMoveTargetLocation);
}

void ARevolution2Character::OnClickMove(const FInputActionValue& Value)
{
	(void)Value;

	if (CurrentViewMode != EViewMode::TopDown || !GetController())
	{
		return;
	}

	FVector AimLocation;
	if (!TryGetTopDownAimLocation(AimLocation))
	{
		return;
	}

	ClickMoveTargetLocation = AimLocation;
	bHasClickMoveTarget = true;
	MoveToLocation(ClickMoveTargetLocation);
}

void ARevolution2Character::MoveToLocation(const FVector& TargetLocation)
{
	if (!GetController() || !GetWorld())
	{
		return;
	}

	// For player-controlled characters, calculate direction and move directly
	FVector CurrentLocation = GetActorLocation();
	FVector Direction = (TargetLocation - CurrentLocation).GetSafeNormal();
	
	// Only move if we're not already at the target (within a small threshold)
	float DistanceToTarget = FVector::Dist(CurrentLocation, TargetLocation);
	if (DistanceToTarget > ClickMoveAcceptanceRadius)
	{
		// Calculate movement direction in world space
		FVector ForwardDirection = Direction;
		ForwardDirection.Z = 0.0f; // Keep movement horizontal
		ForwardDirection.Normalize();
		
		// Apply movement input
		AddMovementInput(ForwardDirection, 1.0f);
	}
}

void ARevolution2Character::ApplyFirstPersonView(APlayerController* PC)
{
	UE_LOG(LogRevolution2, Warning, TEXT("View mode set to FirstPerson"));
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, TEXT("视角模式: 第一人称"));
	}

	SetCameraActive(TopDownCameraComponent, false);
	SetCameraActive(FirstPersonCameraComponent, true);

	if (FirstPersonMesh)
	{
		FirstPersonMesh->SetOnlyOwnerSee(true);
		FirstPersonMesh->SetVisibility(true);
		FirstPersonMesh->SetHiddenInGame(false);
	}
	if (GetMesh())
	{
		GetMesh()->SetOwnerNoSee(true);
	}

	SetMouseInputMode(PC, false);

	// In first person mode, character should rotate with camera/controller
	bUseControllerRotationYaw = true;
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bOrientRotationToMovement = false;
	}
}

void ARevolution2Character::ApplyTopDownView(APlayerController* PC)
{
	UE_LOG(LogRevolution2, Warning, TEXT("View mode set to TopDown"));
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, TEXT("视角模式: 俯视角"));
	}

	ApplyTopDownCameraSettings();

	SetCameraActive(FirstPersonCameraComponent, false);
	SetCameraActive(TopDownCameraComponent, true);

	// Update mesh visibility
	// Keep FirstPersonMesh accessible but hidden for blueprint access
	if (FirstPersonMesh)
	{
		FirstPersonMesh->SetOnlyOwnerSee(false);
		FirstPersonMesh->SetVisibility(false);
		FirstPersonMesh->SetHiddenInGame(true);
	}
	// In top down mode, make the character mesh visible
	if (GetMesh())
	{
		GetMesh()->SetOwnerNoSee(false);
		GetMesh()->SetOnlyOwnerSee(false);
		GetMesh()->SetVisibility(true);
		GetMesh()->SetHiddenInGame(false);
		// Reset FirstPersonPrimitiveType to ensure mesh is visible in third person
		GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;
	}
	else
	{
		UE_LOG(LogRevolution2, Error, TEXT("GetMesh() is null in TopDown mode!"));
	}

	SetMouseInputMode(PC, true);

	// Update character movement settings
	bUseControllerRotationYaw = true;
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bOrientRotationToMovement = false; // Mouse controls rotation, not movement
	}
}

void ARevolution2Character::ApplyTopDownCameraSettings()
{
	if (!TopDownSpringArm || !TopDownCameraComponent)
	{
		return;
	}

	const float ClampedAngle = FMath::Clamp(TopDownCameraAngle, -90.0f, -10.0f);
	TopDownSpringArm->TargetArmLength = TopDownCameraHeight;
	TopDownSpringArm->SetRelativeRotation(FRotator(ClampedAngle, 0.0f, 0.0f));
	TopDownSpringArm->SetRelativeLocation(FVector::ZeroVector);
	TopDownCameraComponent->SetRelativeLocation(FVector::ZeroVector);
	TopDownCameraComponent->SetRelativeRotation(FRotator::ZeroRotator);
}

bool ARevolution2Character::TryGetTopDownAimLocation(FVector& OutAimLocation) const
{
	const APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return false;
	}

	// Get mouse position
	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (!PC->GetMousePosition(MouseX, MouseY))
	{
		return false;
	}

	// Deproject screen position to world
	FVector WorldLocation;
	FVector WorldDirection;
	if (!PC->DeprojectScreenPositionToWorld(MouseX, MouseY, WorldLocation, WorldDirection))
	{
		return false;
	}

	// Perform line trace to find ground
	FHitResult HitResult;
	const FVector Start = WorldLocation;
	const FVector End = Start + (WorldDirection * 10000.0f);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	if (!World->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, QueryParams))
	{
		return false;
	}

	OutAimLocation = HitResult.ImpactPoint;
	return true;
}

void ARevolution2Character::SetTopDownAimLocation(const FVector& AimLocation)
{
	TopDownAimLocation = AimLocation;
	bHasTopDownAimLocation = true;
	OnTopDownAimLocationUpdated(AimLocation);
}

void ARevolution2Character::OnTopDownAimLocationUpdated(const FVector& AimLocation)
{
}

void ARevolution2Character::SetMouseInputMode(APlayerController* PC, bool bEnableMouse)
{
	if (!PC)
	{
		return;
	}

	PC->bShowMouseCursor = bEnableMouse;

	if (bEnableMouse)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
	}
	else
	{
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
	}
}

void ARevolution2Character::SetCameraActive(UCameraComponent* Camera, bool bActive)
{
	if (!Camera)
	{
		return;
	}

	Camera->SetActive(bActive);
	Camera->SetHiddenInGame(!bActive);
}

UCameraComponent* ARevolution2Character::GetActiveCameraComponent() const
{
	if (CurrentViewMode == EViewMode::TopDown)
	{
		return TopDownCameraComponent;
	}
	else
	{
		return FirstPersonCameraComponent;
	}
}
