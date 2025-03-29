// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Pawn.h"
#include "CarPawn.generated.h"

class UBoxComponent;
class UCameraComponent;
class USpringArmComponent;
class UFloatingPawnMovement;

UCLASS()
class TRACKMANIA_API ACarPawn : public APawn
{
	GENERATED_BODY()

public:
	ACarPawn();

	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	virtual void BeginPlay() override;
	
#pragma region Movement
	UFUNCTION()
	void StartMove(const FInputActionValue& Value);

	UFUNCTION()
	void ApplySpeed(float DeltaTime);

	UFUNCTION()
	void TryApplyVelocity(float DeltaTime);

	UFUNCTION()
	void Decelerate(float DeltaTime);

	UFUNCTION()
	bool IsGrounded() const;
#pragma endregion

#pragma region Rotation
	UFUNCTION()
	void StartRotate(const FInputActionValue& Value);

	UFUNCTION()
	void ApplyRotationSpeed(float DeltaTime);

	UFUNCTION()
	void ApplyRotation(float DeltaTime);

	UFUNCTION()
	void DecelerateRotation(float DeltaTime);
#pragma endregion
	
protected:
#pragma region Input
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess="true"))
	class UInputAction* MoveForwardAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess="true"))
	class UInputAction* RotateAction;
#pragma endregion
	
#pragma region Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	UMeshComponent* Visual;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	UBoxComponent* BoxCollision;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	TArray<UCapsuleComponent*> Wheels;
#pragma endregion 

#pragma region Movement
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Movement, meta=(AllowPrivateAccess="true"))
	float MovementValue;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Movement, meta=(AllowPrivateAccess="true"))
	float CurrentSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Movement, meta=(AllowPrivateAccess="true"))
	float Acceleration;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Movement, meta=(AllowPrivateAccess="true"))
	float DecelerationForce;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Movement, meta=(AllowPrivateAccess="true"))
	FVector LastAppliedVel;

	UPROPERTY()
	float CurrentDirection;
#pragma endregion

#pragma region Rotation
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Rotation, meta=(AllowPrivateAccess="true"))
	float RotationSpeedValue;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Rotation, meta=(AllowPrivateAccess="true"))
	float CurrentRotationSpeed;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Rotation, meta=(AllowPrivateAccess="true"))
	float RotationAcceleration;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Rotation, meta=(AllowPrivateAccess="true"))
	float RotationDeceleration;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Rotation, meta=(AllowPrivateAccess="true"))
	UCurveFloat* RotationSpeedCurve;
#pragma endregion
};

