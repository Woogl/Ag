// Copyright Woogle. All Rights Reserved.

#include "Character/AgPlayerCharacter.h"

#include "Ag.h"
#include "Camera/CameraComponent.h"
#include "Core/AgSettings.h"
#include "Data/AgCameraData.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"

AAgPlayerCharacter::AAgPlayerCharacter()
{
	// The character faces where it moves; the camera turns with the mouse (플레이어 사양 '이동').
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
}

void AAgPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	ApplyCameraData();
}

void AAgPlayerCharacter::ApplyCameraData()
{
	const UAgCameraData* CameraData = UAgSettings::Get()->CameraData.LoadSynchronous();
	if (!CameraData)
	{
		UE_LOG(LogAg, Error, TEXT("Camera data is not set (Project Settings > Game > Ag)."));
		return;
	}

	CameraBoom->TargetArmLength = CameraData->Distance;
	CameraBoom->SetRelativeLocation(FVector(0.f, 0.f, CameraData->PivotHeight));

	// The arm lags exponentially; the follow lag time is its time constant (about 63% caught up after that time).
	CameraBoom->bEnableCameraLag = CameraData->FollowLagTime > 0.f;
	CameraBoom->CameraLagSpeed = CameraBoom->bEnableCameraLag ? 1.f / CameraData->FollowLagTime : 0.f;
	CameraBoom->bEnableCameraRotationLag = false;

	FollowCamera->SetFieldOfView(CameraData->FieldOfView);
}

void AAgPlayerCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	if (const APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			InputSubsystem->AddMappingContext(InputMapping, 0);
		}
	}
}

void AAgPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Move);
	EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::Look);
}

void AAgPlayerCharacter::Move(const FInputActionValue& Value)
{
	if (!Controller)
	{
		return;
	}

	// X is right, Y is forward, relative to where the camera looks.
	const FVector2D Input = Value.Get<FVector2D>();
	const FRotationMatrix YawMatrix(FRotator(0.f, Controller->GetControlRotation().Yaw, 0.f));
	AddMovementInput(YawMatrix.GetUnitAxis(EAxis::X), Input.Y);
	AddMovementInput(YawMatrix.GetUnitAxis(EAxis::Y), Input.X);
}

void AAgPlayerCharacter::Look(const FInputActionValue& Value)
{
	// Applied directly with no smoothing (카메라 '자유 시점' 회전 반응).
	const FVector2D Input = Value.Get<FVector2D>();
	AddControllerYawInput(Input.X);
	AddControllerPitchInput(Input.Y);
}
