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
}

void ACarPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveForwardAction)
		{
			EnhancedInputComponent->BindAction(MoveForwardAction, ETriggerEvent::Triggered, this, &ACarPawn::StartMove);
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
}

void ACarPawn::StartMove(const FInputActionValue& Value)
{
	UWorld* WorldContext = GetWorld();
	if (WorldContext == nullptr )
	{
		return;
	}
	const float ForwardValue = Value.Get<float>();
	TargetSpeed = ForwardValue;
	ApplySpeed(WorldContext->GetDeltaSeconds());
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("TargetSpeed: %f"), TargetSpeed));
}

void ACarPawn::Decelerate(const float DeltaTime)
{
	CurrentSpeed -= CurrentSpeed * DecelerationForce * DeltaTime / 100;
}

void ACarPawn::ApplySpeed(const float DeltaTime)
{
	float currentDirection = FVector::DotProduct(LastAppliedVel.GetSafeNormal(), GetActorForwardVector());
	float currentVelocityMagnitude = GetVelocity().Size() * (FMath::Abs(currentDirection) > 0.01f ? currentDirection : 1.0f);
	CurrentSpeed = currentVelocityMagnitude + TargetSpeed * ((currentDirection > 0) ? Acceleration : Acceleration * .5f) * DeltaTime * 100;
}

void ACarPawn::TryApplyVelocity(float DeltaTime)
{
	if (!IsGrounded())
	{
		return;
	}
	FVector NewVel = CurrentSpeed * GetActorForwardVector();
	BoxCollision->SetPhysicsLinearVelocity(FVector(NewVel.X, NewVel.Y, GetVelocity().Z));
	LastAppliedVel = NewVel;
}

bool ACarPawn::IsGrounded() const
{
	FHitResult HitResult;
	const FVector Start = GetActorLocation();
	const FVector End = Start - FVector(0, 0, BoxCollision->GetScaledBoxExtent().Z+10);
	    
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	    
	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_WorldStatic,
		QueryParams
	);

	DrawDebugLine(GetWorld(), Start, End, bHit ? FColor::Green : FColor::Red, false, 0.1f);
	    
	if (!bHit)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Red, TEXT("Not Grounded"));
		return false;
	}
	    
	
	GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Green, TEXT("Grounded"));
	return true;
}

