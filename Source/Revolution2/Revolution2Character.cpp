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
	TopDownSpringArm->SetRelativeLocation(FVector(0.0f, 0.0f, TopDownCameraHeight));
	TopDownSpringArm->TargetArmLength = 0.0f;
	TopDownSpringArm->bUsePawnControlRotation = false;
	TopDownSpringArm->bInheritPitch = false;
	TopDownSpringArm->bInheritYaw = false;
	TopDownSpringArm->bInheritRoll = false;
	TopDownSpringArm->bDoCollisionTest = false;

	// Create top down camera
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Top Down Camera"));
	TopDownCameraComponent->SetupAttachment(TopDownSpringArm);
	TopDownCameraComponent->SetRelativeRotation(FRotator(TopDownCameraAngle, 0.0f, 0.0f));
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

	// Initialize view mode
	SetViewMode(CurrentViewMode);
}

void ARevolution2Character::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// In top down mode, continuously update character rotation based on mouse position
	if (CurrentViewMode == EViewMode::TopDown)
	{
		UpdateTopDownAim();
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

	if (CurrentViewMode == EViewMode::FirstPerson)
	{
		UE_LOG(LogRevolution2, Warning, TEXT("View mode set to FirstPerson"));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, TEXT("视角模式: 第一人称"));
		}
		// Deactivate top down camera first to avoid conflicts
		if (TopDownCameraComponent)
		{
			TopDownCameraComponent->SetActive(false);
			TopDownCameraComponent->SetHiddenInGame(true);
		}
		
		// Activate first person camera
		if (FirstPersonCameraComponent)
		{
			FirstPersonCameraComponent->SetActive(true);
			FirstPersonCameraComponent->SetHiddenInGame(false);
		}

		// Update mesh visibility
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

		// Update character movement settings
		// In first person mode, character should rotate with camera/controller
		bUseControllerRotationYaw = true;
		if (GetCharacterMovement())
		{
			GetCharacterMovement()->bOrientRotationToMovement = false;
		}
	}
	else // TopDown
	{
		UE_LOG(LogRevolution2, Warning, TEXT("View mode set to TopDown"));
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, TEXT("视角模式: 俯视角"));
		}
		// Deactivate first person camera first to avoid conflicts
		if (FirstPersonCameraComponent)
		{
			FirstPersonCameraComponent->SetActive(false);
			FirstPersonCameraComponent->SetHiddenInGame(true);
		}
		
		// Activate top down camera
		if (TopDownCameraComponent)
		{
			TopDownCameraComponent->SetActive(true);
			TopDownCameraComponent->SetHiddenInGame(false);
		}

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
			
			UE_LOG(LogRevolution2, Warning, TEXT("TopDown: Mesh visibility set - HiddenInGame: %d"), 
				GetMesh()->bHiddenInGame ? 1 : 0);
		}
		else
		{
			UE_LOG(LogRevolution2, Error, TEXT("GetMesh() is null in TopDown mode!"));
		}

		// Update character movement settings
		bUseControllerRotationYaw = true;
		if (GetCharacterMovement())
		{
			GetCharacterMovement()->bOrientRotationToMovement = false; // Mouse controls rotation, not movement
		}
	}
}

void ARevolution2Character::UpdateTopDownAim()
{
	if (CurrentViewMode != EViewMode::TopDown || !GetController())
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	// Get mouse position
	float MouseX, MouseY;
	if (!PC->GetMousePosition(MouseX, MouseY))
	{
		return;
	}

	// Deproject screen position to world
	FVector WorldLocation, WorldDirection;
	if (PC->DeprojectScreenPositionToWorld(MouseX, MouseY, WorldLocation, WorldDirection))
	{
		// Perform line trace to find ground
		FHitResult HitResult;
		FVector Start = WorldLocation;
		FVector End = Start + (WorldDirection * 10000.0f);

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);

		if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, QueryParams))
		{
			// Calculate direction from character to hit point
			FVector CharacterLocation = GetActorLocation();
			FVector TargetLocation = HitResult.ImpactPoint;
			FVector Direction = (TargetLocation - CharacterLocation).GetSafeNormal();

			// Calculate Yaw rotation (only horizontal rotation)
			FRotator TargetRotation = Direction.Rotation();
			
			// Set only Yaw, keep Pitch and Roll at 0
			FRotator NewRotation = FRotator(0.0f, TargetRotation.Yaw, 0.0f);
			
			// Use controller rotation for smooth rotation
			if (GetController())
			{
				GetController()->SetControlRotation(NewRotation);
			}
			else
			{
				SetActorRotation(NewRotation);
			}
		}
	}
}

void ARevolution2Character::OnClickMove(const FInputActionValue& Value)
{
	if (CurrentViewMode != EViewMode::TopDown || !GetController())
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	// Get mouse position
	float MouseX, MouseY;
	if (!PC->GetMousePosition(MouseX, MouseY))
	{
		return;
	}

	// Deproject screen position to world
	FVector WorldLocation, WorldDirection;
	if (PC->DeprojectScreenPositionToWorld(MouseX, MouseY, WorldLocation, WorldDirection))
	{
		// Perform line trace to find ground
		FHitResult HitResult;
		FVector Start = WorldLocation;
		FVector End = Start + (WorldDirection * 10000.0f);

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);

		if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, QueryParams))
		{
			// Move to target location
			MoveToLocation(HitResult.ImpactPoint);
		}
	}
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
	if (DistanceToTarget > 50.0f) // 50cm threshold
	{
		// Calculate movement direction in world space
		FVector ForwardDirection = Direction;
		ForwardDirection.Z = 0.0f; // Keep movement horizontal
		ForwardDirection.Normalize();
		
		// Apply movement input
		AddMovementInput(ForwardDirection, 1.0f);
	}
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
