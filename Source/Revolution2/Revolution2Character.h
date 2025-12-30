// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Revolution2Character.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class USpringArmComponent;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UENUM(BlueprintType)
enum class EViewMode : uint8
{
	FirstPerson,
	TopDown
};

/**
 *  A basic first person character
 */
UCLASS(abstract)
class ARevolution2Character : public ACharacter
{
	GENERATED_BODY()

public:

	/** Pawn mesh: first person view (arms; seen only by self) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* FirstPersonMesh;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

	/** Top down spring arm for top down camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* TopDownSpringArm;

	/** Top down camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* TopDownCameraComponent;

protected:

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category ="Input")
	class UInputAction* MouseLookAction;

	/** Toggle view mode input action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* ToggleViewAction;

	/** Click move input action for top down mode */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* ClickMoveAction;

	/** Current view mode */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera")
	EViewMode CurrentViewMode = EViewMode::FirstPerson;

	/** Top down camera height */
	UPROPERTY(EditAnywhere, Category="Camera|TopDown", meta = (ClampMin = 100, ClampMax = 10000, Units = "cm"))
	float TopDownCameraHeight = 1000.0f;

	/** Top down camera angle (pitch) */
	UPROPERTY(EditAnywhere, Category="Camera|TopDown", meta = (ClampMin = -90, ClampMax = -10, Units = "Degrees"))
	float TopDownCameraAngle = -45.0f;
	
public:
	ARevolution2Character();

protected:

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Called every frame */
	virtual void Tick(float DeltaTime) override;

	/** Called from Input Actions for movement input */
	void MoveInput(const FInputActionValue& Value);

	/** Called from Input Actions for looking input */
	void LookInput(const FInputActionValue& Value);

	/** Handles aim inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoAim(float Yaw, float Pitch);

	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles jump start inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump end inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

	/** Toggle between first person and top down view */
	void ToggleViewMode();

	/** Set the view mode */
	void SetViewMode(EViewMode NewViewMode);

	/** Get current view mode */
	EViewMode GetCurrentViewMode() const { return CurrentViewMode; }

	/** Handle click move input for top down mode */
	void OnClickMove(const FInputActionValue& Value);

	/** Move to target location */
	void MoveToLocation(const FVector& TargetLocation);

	/** Update top down aim direction based on mouse position */
	void UpdateTopDownAim();

protected:

	/** Set up input action bindings */
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	

public:

	/** Returns the first person mesh **/
	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }

	/** Returns first person camera component **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

	/** Returns top down camera component **/
	UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent; }

	/** Returns the active camera component based on current view mode **/
	UFUNCTION(BlueprintCallable, Category="Camera")
	UCameraComponent* GetActiveCameraComponent() const;

};

