#include "AstoniaPlayerController.h"
#include "AstoniaAppearance.h"
#include "AstoniaCharacter.h"
#include "AstoniaInteractable.h"
#include "AstoniaSaveGame.h"
#include "SAstoniaCharacterCreator.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
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

void AAstoniaPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	AAstoniaCharacter* Char = Cast<AAstoniaCharacter>(InPawn);
	if (!Char || !IsLocalController()) return;

	// Development screenshots (-AstoniaGoto) skip the creator unless -AstoniaCreator asks for it.
	const TCHAR* Cmd = FCommandLine::Get();
	const bool bForce = FParse::Param(Cmd, TEXT("AstoniaCreator"));
	const bool bSkip = !bForce && FCString::Strifind(Cmd, TEXT("AstoniaGoto=")) != nullptr;

	if (UAstoniaSaveGame* Save = UAstoniaSaveGame::Load())
	{
		CharacterName = Save->CharacterName;
		Char->GetAppearance()->Appearance = Save->Appearance;
		Char->GetAppearance()->Apply();
		if (!bForce) return;
	}
	if (!bSkip) CreateCharacter();
}

void AAstoniaPlayerController::CreateCharacter()
{
	AAstoniaCharacter* Char = GetPawn<AAstoniaCharacter>();
	if (!Char || CreatorWidget.IsValid() || !GEngine || !GEngine->GameViewport) return;

	// Close-up from the front.
	USpringArmComponent* Boom = Char->GetCameraBoom();
	SavedArmLength = Boom->TargetArmLength;
	SavedSocketOffset = Boom->SocketOffset;
	Boom->TargetArmLength = 230.f;
	Boom->SocketOffset = FVector(0.f, -60.f, 50.f);
	// Turn the character to face the camera (the control rotation is reset during possession).
	Char->SetActorRotation(FRotator(0.f, GetControlRotation().Yaw + 180.f, 0.f));
	SetControlRotation(FRotator(-8.f, GetControlRotation().Yaw, 0.f));
	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);

	TWeakObjectPtr<AAstoniaPlayerController> Self(this);
	CreatorWidget = SNew(SAstoniaCharacterCreator)
		.Appearance(Char->GetAppearance())
		.InitialName(CharacterName)
		.OnRotate_Lambda([Self](float Degrees) { if (Self.IsValid()) Self->RotatePreview(Degrees); })
		.OnDone_Lambda([Self](const FString& Name) { if (Self.IsValid()) Self->FinishCharacter(Name); });
	GEngine->GameViewport->AddViewportWidgetContent(CreatorWidget.ToSharedRef(), 10);

	FInputModeUIOnly Mode;
	Mode.SetWidgetToFocus(CreatorWidget);
	SetInputMode(Mode);
	SetShowMouseCursor(true);
}

void AAstoniaPlayerController::RotatePreview(float Degrees)
{
	if (APawn* P = GetPawn()) P->AddActorWorldRotation(FRotator(0.f, Degrees, 0.f));
}

void AAstoniaPlayerController::FinishCharacter(const FString& Name)
{
	AAstoniaCharacter* Char = GetPawn<AAstoniaCharacter>();
	if (GEngine && GEngine->GameViewport && CreatorWidget.IsValid())
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(CreatorWidget.ToSharedRef());
	}
	CreatorWidget.Reset();

	CharacterName = Name;
	if (Char)
	{
		UAstoniaSaveGame* Save = Cast<UAstoniaSaveGame>(UGameplayStatics::CreateSaveGameObject(UAstoniaSaveGame::StaticClass()));
		Save->CharacterName = Name;
		Save->Appearance = Char->GetAppearance()->Appearance;
		Save->Save();

		Char->GetCameraBoom()->TargetArmLength = SavedArmLength;
		Char->GetCameraBoom()->SocketOffset = SavedSocketOffset;
		Char->SetActorRotation(FRotator(0.f, GetControlRotation().Yaw, 0.f));
		SetControlRotation(FRotator(-10.f, GetControlRotation().Yaw, 0.f));
	}

	SetIgnoreMoveInput(false);
	SetIgnoreLookInput(false);
	SetInputMode(FInputModeGameOnly());
	SetShowMouseCursor(false);
}
