#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AstoniaCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UAstoniaAppearanceComponent;

/**
 * Player character with a free third-person orbit camera. The body is Epic's UE5 mannequin with the
 * default human skin; clothes, armour, hair and the like are pieces of the appearance component.
 */
UCLASS()
class ASTONIAGAME_API AAstoniaCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AAstoniaCharacter();

	void DoMove(float Right, float Forward);
	void DoLook(float Yaw, float Pitch);

	USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

protected:
	/**
	 * Development only: -AstoniaGoto=X,Y,Z,Yaw[,Pitch] puts the player there on start, -AstoniaArm=<cm>
	 * sets the camera distance (0 = from the head), -AstoniaShot takes a
	 * 1280x720 screenshot three seconds later (-AstoniaShotDelay=<s> to wait longer; Saved/Screenshots). Used to check builds headlessly.
	 * -AstoniaFly hovers at the spot; -AstoniaRoute=X,Y;X,Y;... walks the waypoints and logs where it got to
	 * (LogAstoniaCore "AstoniaRoute"), to check that stairs and doorways can be walked.
	 */
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void StepRoute();
	void LogPerf();

	/** -AstoniaPerf: frame and GPU times of the last frames, logged ("AstoniaPerf") before the screenshot. */
	bool bRecordPerf = false;
	TArray<float> FrameMs;
	TArray<float> GpuMs;

	TArray<FVector2D> Route;
	int32 RouteIndex = 0;
	float RouteStuck = 0.f;
	float RouteBest = MAX_flt;
	float RouteTime = 0.f;

protected:

public:
	UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	UAstoniaAppearanceComponent* GetAppearance() const { return Appearance; }

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UCameraComponent> FollowCamera;

	/** Default human skin plus worn pieces (hair, clothes, armour, jewellery, weapons). */
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UAstoniaAppearanceComponent> Appearance;
};
