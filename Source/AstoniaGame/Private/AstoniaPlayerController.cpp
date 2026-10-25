#include "AstoniaPlayerController.h"
#include "AstoniaCharacter.h"
#include "AstoniaInteractable.h"
#include "EngineUtils.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"

namespace
{
	UInputModifierSwizzleAxis* MakeSwizzle(UObject* Outer)
	{
		UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(Outer);
		Swizzle->Order = EInputAxisSwizzle::YXZ;
		return Swizzle;
	}

	UInputModifierNegate* MakeNegate(UObject* Outer, bool bX, bool bY)
	{
		UInputModifierNegate* Negate = NewObject<UInputModifierNegate>(Outer);
		Negate->bX = bX;
		Negate->bY = bY;
		Negate->bZ = false;
		return Negate;
	}
}

void AAstoniaPlayerController::BuildDefaultInput()
{
	MoveAction = NewObject<UInputAction>(this, TEXT("IA_Move"));
	MoveAction->ValueType = EInputActionValueType::Axis2D;
	LookAction = NewObject<UInputAction>(this, TEXT("IA_Look"));
	LookAction->ValueType = EInputActionValueType::Axis2D;
	JumpAction = NewObject<UInputAction>(this, TEXT("IA_Jump"));
	JumpAction->ValueType = EInputActionValueType::Boolean;

	DefaultContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Default"));

	// Move: X = right, Y = forward.
	DefaultContext->MapKey(MoveAction, EKeys::D);
	DefaultContext->MapKey(MoveAction, EKeys::A).Modifiers.Add(MakeNegate(this, true, false));
	DefaultContext->MapKey(MoveAction, EKeys::W).Modifiers.Add(MakeSwizzle(this));
	{
		FEnhancedActionKeyMapping& Back = DefaultContext->MapKey(MoveAction, EKeys::S);
		Back.Modifiers.Add(MakeSwizzle(this));
		Back.Modifiers.Add(MakeNegate(this, true, true));
	}
	DefaultContext->MapKey(MoveAction, EKeys::Gamepad_Left2D);

	// Look: mouse Y is inverted so pushing the mouse forward looks up.
	DefaultContext->MapKey(LookAction, EKeys::Mouse2D).Modifiers.Add(MakeNegate(this, false, true));
	DefaultContext->MapKey(LookAction, EKeys::Gamepad_Right2D).Modifiers.Add(MakeNegate(this, false, true));

	DefaultContext->MapKey(JumpAction, EKeys::SpaceBar);
	DefaultContext->MapKey(JumpAction, EKeys::Gamepad_FaceButton_Bottom);

	InteractAction = NewObject<UInputAction>(this, TEXT("IA_Interact"));
	InteractAction->ValueType = EInputActionValueType::Boolean;
	DefaultContext->MapKey(InteractAction, EKeys::E);
	DefaultContext->MapKey(InteractAction, EKeys::Gamepad_FaceButton_Left);
}

AActor* AAstoniaPlayerController::FindInteractable() const
{
	const APawn* MyPawn = GetPawn();
	if (!MyPawn) return nullptr;

	const FVector From = MyPawn->GetActorLocation();
	AActor* Best = nullptr;
	float BestDist = TNumericLimits<float>::Max();
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		IAstoniaInteractable* Usable = Cast<IAstoniaInteractable>(*It);
		if (!Usable || !Usable->CanInteract()) continue;
		const float Dist = FVector::Dist(From, Usable->GetInteractLocation());
		if (Dist <= Usable->GetInteractRange() && Dist < BestDist)
		{
			Best = *It;
			BestDist = Dist;
		}
	}
	return Best;
}

void AAstoniaPlayerController::OnInteract()
{
	if (AActor* Target = FindInteractable())
	{
		Cast<IAstoniaInteractable>(Target)->Interact(GetPawn());
	}
}

void AAstoniaPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!IsLocalPlayerController()) return;

	BuildDefaultInput();

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultContext, 0);
	}

	if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(InputComponent))
	{
		Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AAstoniaPlayerController::OnMove);
		Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &AAstoniaPlayerController::OnLook);
		Input->BindAction(JumpAction, ETriggerEvent::Started, this, &AAstoniaPlayerController::OnJumpStarted);
		Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &AAstoniaPlayerController::OnJumpCompleted);
		Input->BindAction(InteractAction, ETriggerEvent::Started, this, &AAstoniaPlayerController::OnInteract);
	}
}

void AAstoniaPlayerController::OnMove(const FInputActionValue& Value)
{
	if (AAstoniaCharacter* Char = GetPawn<AAstoniaCharacter>())
	{
		const FVector2D V = Value.Get<FVector2D>();
		Char->DoMove(V.X, V.Y);
	}
}

void AAstoniaPlayerController::OnLook(const FInputActionValue& Value)
{
	if (AAstoniaCharacter* Char = GetPawn<AAstoniaCharacter>())
	{
		const FVector2D V = Value.Get<FVector2D>();
		Char->DoLook(V.X, V.Y);
	}
}

void AAstoniaPlayerController::OnJumpStarted()
{
	if (ACharacter* Char = GetPawn<ACharacter>()) Char->Jump();
}

void AAstoniaPlayerController::OnJumpCompleted()
{
	if (ACharacter* Char = GetPawn<ACharacter>()) Char->StopJumping();
}
