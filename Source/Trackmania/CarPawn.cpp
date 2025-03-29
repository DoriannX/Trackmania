// Fill out your copyright notice in the Description page of Project Settings.

#include "CarPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputTriggers.h"

ACarPawn::ACarPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollision"));
	RootComponent = BoxCollision;
	Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	Visual->SetupAttachment(RootComponent);
	CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	Camera->SetupAttachment(CameraArm);
	CameraArm->SetupAttachment(RootComponent);

	for (int i = 0; i < 2; i++)
	{
		Wheels.Add(CreateDefaultSubobject<UCapsuleComponent>(FName(*FString::Printf(TEXT("Wheel%d"), i + 1))));
		Wheels[i]->SetupAttachment(RootComponent);
	}
}

void ACarPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveForwardAction)
		{
			EnhancedInputComponent->BindAction(MoveForwardAction, ETriggerEvent::Triggered, this, &ACarPawn::StartMove);
			EnhancedInputComponent->BindAction(RotateAction, ETriggerEvent::Triggered, this, &ACarPawn::StartRotate);
		}
	}
}

void ACarPawn::BeginPlay()
{
	Super::BeginPlay();
}

void ACarPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Decelerate(DeltaTime);
	TryApplyVelocity(DeltaTime);

	DecelerateRotation(DeltaTime);
	ApplyRotation(DeltaTime);
	AddActorWorldRotation(FRotator(0, 180* DeltaTime, 0));
}

#pragma region Movement
void ACarPawn::StartMove(const FInputActionValue& Value)
{
	UWorld* WorldContext = GetWorld();
	if (WorldContext == nullptr)
	{
		return;
	}
	const float ForwardValue = Value.Get<float>();
	MovementValue = ForwardValue;
	ApplySpeed(WorldContext->GetDeltaSeconds());
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("TargetSpeed: %f"), MovementValue));
}

void ACarPawn::Decelerate(const float DeltaTime)
{
	CurrentSpeed -= CurrentSpeed * DecelerationForce * DeltaTime / 100;
}

void ACarPawn::ApplySpeed(const float DeltaTime)
{
	CurrentDirection = FVector::DotProduct(LastAppliedVel.GetSafeNormal(), GetActorForwardVector());
	const float CurrentVelocityMagnitude = GetVelocity().Size() * (FMath::Abs(CurrentDirection) > 0.01f
		                                                               ? CurrentDirection
		                                                               : 1.0f);
	CurrentSpeed = CurrentVelocityMagnitude + MovementValue * ((CurrentDirection > 0)
		                                                           ? Acceleration
		                                                           : Acceleration * .5f) * DeltaTime * 100;
}

void ACarPawn::TryApplyVelocity(float DeltaTime)
{
	if (!IsGrounded())
	{
		return;
	}
	FVector NewVel = CurrentSpeed * GetActorForwardVector();
	BoxCollision->SetPhysicsLinearVelocity(FVector(NewVel.X, NewVel.Y, GetVelocity().Z));
	for (UCapsuleComponent* Wheel : Wheels)
	{
		if (Wheel && Wheel->IsSimulatingPhysics())
		{
			Wheel->SetPhysicsLinearVelocity(FVector(NewVel.X, NewVel.Y, GetVelocity().Z));
		}
	}

	LastAppliedVel = NewVel;
	LastAppliedVel = NewVel;
}

bool ACarPawn::IsGrounded() const
{
	// Track if all wheels are touching the ground
	bool bAllWheelsGrounded = true;

	// Check each wheel
	for (int i = 0; i < Wheels.Num(); i++)
	{
		FHitResult HitResult;
		// Get wheel's position
		const FVector WheelPosition = Wheels[i]->GetComponentLocation();
		// Cast ray downward from the wheel
		const FVector Start = WheelPosition;
		// Trace slightly below the wheel's lowest point
		const FVector End = Start - FVector(0, 0, Wheels[i]->GetScaledCapsuleHalfHeight() + 10);

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);

		const bool bWheelHit = GetWorld()->LineTraceSingleByChannel(
			HitResult,
			Start,
			End,
			ECC_WorldStatic,
			QueryParams
		);

		// Draw debug line for each wheel
		DrawDebugLine(GetWorld(), Start, End, bWheelHit ? FColor::Green : FColor::Red, false, 0.1f);

		// If any wheel isn't grounded, the vehicle isn't grounded
		if (!bWheelHit)
		{
			bAllWheelsGrounded = false;
			// Optional: Add debug message for each ungrounded wheel
			GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Yellow,
			                                 FString::Printf(TEXT("Wheel %d not grounded"), i + 1));
		}
	}

	// Debug message for overall grounded state
	if (bAllWheelsGrounded)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Green, TEXT("All Wheels Grounded"));
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Red, TEXT("Not All Wheels Grounded"));
	}

	return bAllWheelsGrounded;
}
#pragma endregion

#pragma region Rotation
void ACarPawn::StartRotate(const FInputActionValue& Value)
{
	UWorld* WorldContext = GetWorld();
	if (WorldContext == nullptr)
	{
		return;
	}
	RotationSpeedValue = Value.Get<float>();
	ApplyRotationSpeed(WorldContext->GetDeltaSeconds());
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
	                                 FString::Printf(TEXT("RotationSpeed: %f"), RotationSpeedValue));
}

void ACarPawn::ApplyRotationSpeed(const float DeltaTime)
{
	CurrentRotationSpeed += RotationSpeedValue * RotationAcceleration * DeltaTime;
}

void ACarPawn::ApplyRotation(const float DeltaTime)
{
	float MaxSpeed = (Acceleration * 10000) / DecelerationForce;
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("MaxSpeed: %f"), MaxSpeed));
	float Rotation = CurrentRotationSpeed * DeltaTime * RotationSpeedCurve->GetFloatValue(
		FMath::Clamp(FMath::Abs(CurrentSpeed) / MaxSpeed, 0.0f, 1.0f)) * CurrentDirection;
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("Rotation: %f"), Rotation));
	AddActorWorldRotation(FRotator(0, Rotation, 0));
}

void ACarPawn::DecelerateRotation(const float DeltaTime)
{
	CurrentRotationSpeed -= CurrentRotationSpeed * RotationDeceleration * DeltaTime;
}
#pragma endregion
