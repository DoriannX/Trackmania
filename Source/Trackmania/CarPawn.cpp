// Fill out your copyright notice in the Description page of Project Settings.

#include "CarPawn.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputTriggers.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

ACarPawn::ACarPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	CarBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CarBody"));
	RootComponent = CarBody;
	PivotObject = CreateDefaultSubobject<USceneComponent>(TEXT("Pivot"));
	PivotObject->SetupAttachment(RootComponent);
	CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
	CameraArm->SetupAttachment(PivotObject);
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	Camera->SetupAttachment(CameraArm);
	
}

void ACarPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveForwardAction && RotateAction && LookAction && InverseAction)
		{
			EnhancedInputComponent->BindAction(MoveForwardAction, ETriggerEvent::Triggered, this, &ACarPawn::StartMove);
			EnhancedInputComponent->BindAction(MoveForwardAction, ETriggerEvent::Canceled, this, &ACarPawn::StopMove);
			EnhancedInputComponent->BindAction(MoveForwardAction, ETriggerEvent::Completed, this, &ACarPawn::StopMove);
			EnhancedInputComponent->BindAction(RotateAction, ETriggerEvent::Triggered, this, &ACarPawn::StartRotate);
			EnhancedInputComponent->BindAction(RotateAction, ETriggerEvent::Canceled, this, &ACarPawn::StopRotate);
			EnhancedInputComponent->BindAction(RotateAction, ETriggerEvent::Completed, this, &ACarPawn::StopRotate);
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ACarPawn::StartLook);
			EnhancedInputComponent->BindAction(InverseAction, ETriggerEvent::Started, this, &ACarPawn::InverseGravity);
		}
	}
}

void ACarPawn::BeginPlay()
{
	Super::BeginPlay();
	DefaultAcceleration = Acceleration;
}

void ACarPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	ApplyGravity(DeltaTime);
	Decelerate(DeltaTime, 1);
	TryApplyVelocity(DeltaTime);
	ApplyRotation(DeltaTime);
	SmoothlyResetLook(DeltaTime);
	
	/*CameraArm->SetRelativeRotation(UKismetMathLibrary::RLerp(CameraArm->GetRelativeRotation(), FRotator(FMath::Clamp(CameraArm->GetRelativeRotation().Pitch, (GravityScale > 0) ? -50 : 0, GravityScale > 0 ? 0 : 50),
										 CameraArm->GetRelativeRotation().Yaw, CameraArm->GetRelativeRotation().Roll), DeltaTime * 20, true));*/
}

#pragma region Movement
void ACarPawn::StartMove(const FInputActionValue& Value)
{
	MovementValue = Value.Get<float>();
}

void ACarPawn::StopMove(const FInputActionValue& Value)
{
	MovementValue = 0;
}

void ACarPawn::InverseGravity(const FInputActionValue& Value)
{
	if (IsGrounded())
	{
		CarBody->SetPhysicsLinearVelocity(FVector(GetVelocity().X, GetVelocity().Y, JumpForce * GravityScale));
	}
	GravityScale = -GravityScale;
	PivotObject->SetWorldRotation(FRotator(0 , PivotObject->GetComponentRotation().Yaw, PivotObject->GetComponentRotation().Roll + 180));
	CameraArm->SetRelativeRotation(FRotator(-CameraArm->GetRelativeRotation().Pitch ,
											 CameraArm->GetRelativeRotation().Yaw, CameraArm->GetRelativeRotation().Roll));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Gravity Inversed"));
}

void ACarPawn::ApplyGravity(const float DeltaTime)
{
	CarBody->SetPhysicsLinearVelocity(GetVelocity() + GravityScale * DeltaTime * FVector(0, 0, -980));
}

void ACarPawn::Decelerate(const float DeltaTime, const float Multiplier = 1.0f)
{
	CarBody->SetPhysicsLinearVelocity(
		GetVelocity() - GetVelocity() * DecelerationForce * DeltaTime / 100 * Multiplier);
}

void ACarPawn::TryApplyVelocity(float DeltaTime)
{
	if (!IsGrounded())
	{
		return;
	}

	CarBody->SetPhysicsLinearVelocity(
		GetVelocity() + GetActorForwardVector() * MovementValue * ((MovementValue > 0)
			                                                           ? Acceleration
			                                                           : Acceleration * .5f) * DeltaTime * 100);
}

bool ACarPawn::IsGrounded() const
{
	const float CarExtent = CarBody->Bounds.BoxExtent.Z/2;
	const FVector Start = GetActorLocation() + GetActorUpVector() * CarExtent;
	const FVector End = Start - PivotObject->GetUpVector() * (CarExtent *2+ 50);
	
	FHitResult HitResult1;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	
	const bool bGrounded = GetWorld()->LineTraceSingleByChannel(
		HitResult1,
		Start,
		End,
		ECC_WorldStatic,
		QueryParams
	);
	
	DrawDebugLine(GetWorld(), Start, End, bGrounded ? FColor::Green : FColor::Red, true, 0.1f);
	
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

void ACarPawn::Accelerate(const float Multiplier)
{
	Acceleration *= Multiplier;
}

void ACarPawn::ResetAcceleration()
{
	Acceleration = DefaultAcceleration;
}
#pragma endregion

#pragma region Rotation
void ACarPawn::StartRotate(const FInputActionValue& Value)
{
	UWorld* WorldContext = GetWorld();
	float value = Value.Get<float>();
	if (WorldContext == nullptr)
	{
		return;
	}
	RotationSpeedValue = value * GravityScale;
	if (IsGrounded())
	{
		if (FMath::Abs(GetVelocity().Size()) > 0.1)
        {
			IncreaseBoostFuel();
        }
		Decelerate(WorldContext->GetDeltaSeconds(), 1.0f);
	}
}

void ACarPawn::StopRotate(const FInputActionValue& Value)
{
	RotationSpeedValue = 0;
}

void ACarPawn::ApplyRotation(const float DeltaTime)
{
	const float MaxSpeed = (Acceleration * 10000) / DecelerationForce;
	const float Rotation = RotationSpeedValue * RotationAcceleration * DeltaTime * RotationSpeedCurve->GetFloatValue(
		FMath::Clamp(FMath::Abs(GetVelocity().Size()) / MaxSpeed, 0.0f, 1.0f)) * ((MovementValue == 0)
		? 1
		: MovementValue);
	CurrentRotation += Rotation;
	SetActorRotation(UKismetMathLibrary::RotatorFromAxisAndAngle(GetActorUpVector(), CurrentRotation));
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
	FRotator CurrentRotation = CameraArm->GetRelativeRotation();
	CameraArm->SetRelativeRotation(FRotator(FMath::Clamp(CurrentRotation.Pitch + LookValue.Y * GravityScale, (GravityScale > 0) ? -50 : 0, GravityScale > 0 ? 0 : 50),
	                                     CurrentRotation.Yaw + LookValue.X, CurrentRotation.Roll));
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
