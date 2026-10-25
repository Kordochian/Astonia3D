#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AstoniaNpc.generated.h"

class UTextRenderComponent;
class UAstoniaAppearanceComponent;

UENUM(BlueprintType)
enum class EAstoniaNpcRole : uint8
{
	/** Ordinary attackable NPC (still a marker until M3/M6). */
	Monster,
	/** Named, non-attackable character who talks to the player (Gwendylon, Lydia...). */
	QuestGiver,
	/** Named shop keeper. */
	Merchant,
	/** Lydia's thieves in the west woods. */
	Thief,
};

/**
 * A 3D NPC standing where the original map placed it. For now it only stands there with an idle
 * animation, a name tag and (for quest givers) a "!" marker; dialogue and AI come in M3.
 * Body: Epic's UE5 mannequin (Manny, or Quinn for female characters) dressed by an appearance component.
 */
UCLASS()
class ASTONIAGAME_API AAstoniaNpc : public ACharacter
{
	GENERATED_BODY()

public:
	AAstoniaNpc();

	/** Call right after spawning. Skin = row of DT_Skins (default human body, see AstoniaSkin.h). */
	void Setup(FName InTemplate, const FString& InDisplayName, EAstoniaNpcRole InRole, bool bFemale, FName Skin);

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astonia")
	FName Template;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astonia")
	EAstoniaNpcRole NpcRole = EAstoniaNpcRole::Monster;

private:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UTextRenderComponent> NameTag;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UTextRenderComponent> QuestMarker;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UAstoniaAppearanceComponent> Appearance;
};
