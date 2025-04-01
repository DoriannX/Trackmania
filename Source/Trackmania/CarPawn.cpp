// Fill out your copyright notice in the Description page of Project Settings.

#include "CarPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputTriggers.h"
#include "Kismet/KismetMathLibrary.h"

ACarPawn::ACarPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	PhysicsRootComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PhysicsRootComponent"));
	PhysicsRootComponent->SetSimulatePhysics(true);
	RootComponent = PhysicsRootComponent;
	Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	Visual->SetupAttachment(RootComponent);
	CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
	CameraArm->SetupAttachment(RootComponent);
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	Camera->SetupAttachment(CameraArm);
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
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACarPawn::StartLook);
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
	Decelerate(DeltaTime, 1);
	TryApplyVelocity(DeltaTime);

	DecelerateRotation(DeltaTime);
	ApplyRotation(DeltaTime);
	SmoothlyResetLook(DeltaTime);
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
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("TargetSpeed: %f"), MovementValue));
}

void ACarPawn::Decelerate(const float DeltaTime, const float Multiplier = 1.0f)
{
	CurrentSpeed -= CurrentSpeed * DecelerationForce * DeltaTime / 100 * Multiplier;
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
	float DriftMultiplier = 20;
	float Drift = (1 - GetActorForwardVector().Dot(GetVelocity().GetSafeNormal())) * DriftMultiplier;
	if (!IsGrounded())
	{
		bDrifted = false;
		TargetVel = CurrentSpeed / FMath::Max(Drift, 1);
		return;
	}

	/*if (!bDrifted)
	{
		PhysicsRootComponent->SetPhysicsLinearVelocity(GetVelocity() - GetVelocity().GetSafeNormal() * Drift);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("Drift: %f"), Drift));

		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, FString::Printf(TEXT("TargetVel: %f"), TargetVel));
		if (FMath::Abs(GetVelocity().Size()) <= TargetVel)
		{
			bDrifted = true;
		}
		return;
	}*/

	FVector NewVel = GetActorForwardVector() * CurrentSpeed;
	PhysicsRootComponent->SetPhysicsLinearVelocity(FVector(NewVel.X, NewVel.Y, GetVelocity().Z));
	LastAppliedVel = NewVel;
}

bool ACarPawn::IsGrounded() const
{
	const FVector CarPosition = PhysicsRootComponent->GetComponentLocation();
	//float CapsuleExtent = CapsuleCollision->GetScaledCapsuleRadius();
	const FVector Start = CarPosition - FVector(0, 0, 0);
	const FVector End = Start - FVector(0, 0, 25);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	const bool bGrounded = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_WorldStatic,
		QueryParams
	);

	//DrawDebugLine(GetWorld(), Start, End, bGrounded ? FColor::Green : FColor::Red, false, 0.1f);

	if (bGrounded)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Green, TEXT("Vehicle Grounded"));
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Red, TEXT("Vehicle Not Grounded"));
	}

	return bGrounded;
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
	if (IsGrounded())
	{
		Decelerate(WorldContext->GetDeltaSeconds(), 1.0f);
	}
	ApplyRotationSpeed(WorldContext->GetDeltaSeconds());
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,FString::Printf(TEXT("RotationSpeed: %f"), RotationSpeedValue));
}

void ACarPawn::ApplyRotationSpeed(const float DeltaTime)
{
	CurrentRotationSpeed += RotationSpeedValue * RotationAcceleration * DeltaTime;
}

void ACarPawn::ApplyRotation(const float DeltaTime)
{
	float MaxSpeed = (Acceleration * 10000) / DecelerationForce;
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("MaxSpeed: %f"), MaxSpeed));
	float Rotation = CurrentRotationSpeed * DeltaTime * RotationSpeedCurve->GetFloatValue(
		FMath::Clamp(FMath::Abs(CurrentSpeed) / MaxSpeed, 0.0f, 1.0f)) * CurrentDirection;
	//GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("Rotation: %f"), Rotation));
	CurrentRotation += Rotation;

	//SetActorRotation(FMath::RInterpTo(GetActorRotation(), UKismetMathLibrary::RotatorFromAxisAndAngle(GroundNormalVector, CurrentRotation), DeltaTime, 5.0f));
	SetActorRotation(UKismetMathLibrary::RotatorFromAxisAndAngle(GetActorUpVector(), CurrentRotation));
	/*SetActorRotation(FMath::RInterpTo(,
	                                  UKismetMathLibrary::RotatorFromAxisAndAngle(FVector::UpVector, CurrentRotation),
	                                  DeltaTime, 5.0f));*/
}

void ACarPawn::DecelerateRotation(const float DeltaTime)
{
	CurrentRotationSpeed -= CurrentRotationSpeed * RotationDeceleration * DeltaTime;
}
#pragma endregion

#pragma region Look

void ACarPawn::StartLook(const FInputActionValue& InputActionValue)
{
	const FVector LookValue = InputActionValue.Get<FVector>();
	if (CameraArm == nullptr || LookValue.IsZero())
	{
		return;
	}
	FRotator CurrentRotation = CameraArm->GetComponentRotation();
	CameraArm->SetWorldRotation(FRotator(FMath::Clamp(CurrentRotation.Pitch + LookValue.Y, -50, 50),
	                                     CurrentRotation.Yaw + LookValue.X, 0));
}

void ACarPawn::SmoothlyResetLook(const float DeltaTime) const
{
	if (CameraArm == nullptr)
	{
		return;
	}
	CameraArm->SetRelativeRotation(FMath::RInterpTo(CameraArm->GetRelativeRotation(),
	                                                FRotator(CameraArm->GetRelativeRotation().Pitch,
	                                                         UKismetMathLibrary::MakeRotFromX(GetActorForwardVector()).
	                                                         Yaw, CameraArm->GetRelativeRotation().Roll),
	                                                DeltaTime, 7.5f));
}

#pragma endregion
