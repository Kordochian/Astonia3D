#include "AstoniaHUD.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"

void AAstoniaHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas) return;

	static const FString Credit = TEXT("Based on the Astonia 3 engine by Daniel Brockhaus - brockhaus.org");
	UFont* Font = GEngine->GetSmallFont();
	float W = 0.f, H = 0.f;
	GetTextSize(Credit, W, H, Font);
	DrawText(Credit, FLinearColor(1.f, 1.f, 1.f, 0.6f), Canvas->ClipX - W - 12.f, Canvas->ClipY - H - 8.f, Font);
}
