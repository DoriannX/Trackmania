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
	const float CarExtent = CarBody->Bounds.BoxExtent.Z/2;
	const FVector Start = GetActorLocation() + GetActorUpVector() * CarExtent;
	const FVector End = Start + PivotObject->GetUpVector() * 9999;
	
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	
	const bool bIsCeilingOnTop = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_WorldStatic,
		QueryParams
	);
	if (!bIsCeilingOnTop)
	{
		return;
	}


	
	GravityScale = -GravityScale;
	PivotObject->SetWorldRotation(FRotator(0 , PivotObject->GetComponentRotation().Yaw, PivotObject->GetComponentRotation().Roll + 180));
	CameraArm->SetRelativeRotation(FRotator(-CameraArm->GetRelativeRotation().Pitch ,
											 CameraArm->GetRelativeRotation().Yaw, CameraArm->GetRelativeRotation().Roll));
	CarBody->SetWorldLocation(HitResult.Location + HitResult.Normal * CarExtent * 2);
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
		GetVelocity() + CarForward * MovementValue * ((MovementValue > 0)
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
	float Direction = (FMath::Abs(MovementValue) < KINDA_SMALL_NUMBER) ? 0.0f : (FVector::DotProduct(GetVelocity(), CarForward) >= 0 ? 1.0f : -1.0f);
	const float MaxSpeed = (Acceleration * 10000) / DecelerationForce;
	const float Rotation = RotationSpeedValue * RotationAcceleration * DeltaTime * RotationSpeedCurve->GetFloatValue(
		FMath::Clamp(FMath::Abs(GetVelocity().Size()) / MaxSpeed, 0.0f, 1.0f)) * ((MovementValue == 0)
		? 1
		: FMath::Abs(MovementValue) * Direction);
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
	FRotator CurrentRotationLook = CameraArm->GetRelativeRotation();
	CameraArm->SetRelativeRotation(FRotator(FMath::Clamp(CurrentRotationLook.Pitch + LookValue.Y * GravityScale, (GravityScale > 0) ? -50 : 0, GravityScale > 0 ? 0 : 50),
	                                     CurrentRotationLook.Yaw + LookValue.X, CurrentRotationLook.Roll));
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

FString ACarPawn::GetFormatedTime(const float Time)
{
	return FString::Printf(TEXT("%02lld:%02lld:%02lld"), FMath::FloorToInt(fmod(Time / 60, 60)), FMath::FloorToInt(fmod(Time, 60)),
	FMath::FloorToInt(fmod(Time * 100, 100)));
}
