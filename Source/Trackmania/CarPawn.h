// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
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
	void StopMove(const FInputActionValue& Value);

	UFUNCTION()
	void InverseGravity(const FInputActionValue& Value);

	UFUNCTION()
	void ApplyGravity(const float DeltaTime);

	UFUNCTION()
	void TryApplyVelocity(float DeltaTime);

	UFUNCTION()
	void Decelerate(float DeltaTime, float Multiplier);

	UFUNCTION(BlueprintCallable)
	bool IsGrounded() const;

	UFUNCTION(BlueprintCallable, Category="Movement")
	void Accelerate(float Multiplier);
	
	UFUNCTION(BlueprintCallable, Category="Movement")
	void ResetAcceleration();

	UFUNCTION(BlueprintImplementableEvent, Category="Movement")
	void IncreaseBoostFuel();
#pragma endregion

#pragma region Rotation
	UFUNCTION()
	void StartRotate(const FInputActionValue& Value);

	UFUNCTION()
	void StopRotate(const FInputActionValue& Value);

	UFUNCTION()
	void ApplyRotation(float DeltaTime);
#pragma endregion

#pragma region Look

	UFUNCTION()
	void StartLook(const FInputActionValue& InputActionValue);
	
	UFUNCTION()
	void SmoothlyResetLook(const float DeltaTime) const;
	
#pragma endregion
	
protected:
#pragma region Input
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess="true"))
	class UInputAction* MoveForwardAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess="true"))
	UInputAction* RotateAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess="true"))
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess="true"))
	UInputAction* InverseAction;
#pragma endregion
	
#pragma region Components

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Component, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Component, meta=(AllowPrivateAccess = "true"))
	UStaticMeshComponent* CarBody;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Component, meta=(AllowPrivateAccess = "true"))
	USceneComponent* PivotObject;
	
#pragma endregion 

#pragma region Movement
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Movement, meta=(AllowPrivateAccess="true"))
	float MovementValue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Movement, meta=(AllowPrivateAccess="true"))
	float Acceleration;
	
	UPROPERTY()
	float DefaultAcceleration;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Movement, meta=(AllowPrivateAccess="true"))
	float DecelerationForce;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Movement, meta=(AllowPrivateAccess="true"));
	float GravityScale;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Movement, meta=(AllowPrivateAccess="true"))
	float JumpForce;
#pragma endregion

#pragma region Rotation
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Rotation, meta=(AllowPrivateAccess="true"))
	float RotationSpeedValue;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Rotation, meta=(AllowPrivateAccess="true"))
	float RotationAcceleration;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Rotation, meta=(AllowPrivateAccess="true"))
	UCurveFloat* RotationSpeedCurve;

	UPROPERTY()
	FVector GroundNormalVector;

	UPROPERTY()
	float CurrentRotation;
#pragma endregion
};

