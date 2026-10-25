#include "SAstoniaCharacterCreator.h"
#include "AstoniaAppearance.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "AstoniaCharacterCreator"

namespace
{
	FSlateFontInfo Font(int32 Size, bool bBold = false)
	{
		return FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size);
	}
}

void SAstoniaCharacterCreator::Construct(const FArguments& InArgs)
{
	Appearance = InArgs._Appearance;
	OnRotate = InArgs._OnRotate;
	OnDone = InArgs._OnDone;

	// Skin: brightness and saturation applied to the MetaHuman's baked skin, lightest to darkest.
	TArray<FSwatch> Skin;
	const float Shades[] = {1.35f, 1.2f, 1.05f, 0.9f, 0.75f, 0.6f, 0.47f, 0.35f};
	for (float Shade : Shades)
	{
		const FLinearColor Shown = FLinearColor(0.85f, 0.62f, 0.48f) * Shade;
		Skin.Add({Shown, [Shade](FAstoniaAppearance& A) { A.SkinShade = Shade; A.SkinSaturation = Shade < 0.7f ? 1.1f : 1.f; }});
	}

	TArray<FSwatch> Eyes;
	const FLinearColor EyeColors[] = {
		{0.25f, 0.45f, 0.85f}, {0.35f, 0.65f, 0.40f}, {0.55f, 0.45f, 0.25f}, {0.45f, 0.28f, 0.12f},
		{0.22f, 0.13f, 0.07f}, {0.55f, 0.60f, 0.65f}, {0.85f, 0.55f, 0.15f}};
	for (const FLinearColor& C : EyeColors)
	{
		Eyes.Add({C, [C](FAstoniaAppearance& A) { A.EyeColor = FLinearColor(C.R * 1.6f, C.G * 1.6f, C.B * 1.6f, 1.f); }});
	}

	// Hair: MetaHuman melanin (dark) and redness, black to platinum.
	TArray<FSwatch> Hair;
	const FVector2f HairColors[] = {{1.f, 0.1f}, {0.85f, 0.2f}, {0.65f, 0.3f}, {0.45f, 0.35f}, {0.55f, 0.8f}, {0.3f, 1.f}, {0.18f, 0.3f}, {0.04f, 0.1f}};
	const FLinearColor HairShown[] = {
		{0.03f, 0.025f, 0.02f}, {0.12f, 0.07f, 0.04f}, {0.25f, 0.14f, 0.07f}, {0.42f, 0.27f, 0.14f},
		{0.45f, 0.17f, 0.07f}, {0.75f, 0.28f, 0.08f}, {0.80f, 0.62f, 0.35f}, {0.92f, 0.88f, 0.78f}};
	for (int32 i = 0; i < UE_ARRAY_COUNT(HairColors); i++)
	{
		const FVector2f MR = HairColors[i];
		Hair.Add({HairShown[i], [MR](FAstoniaAppearance& A) { A.HairMelanin = MR.X; A.HairRedness = MR.Y; }});
	}

	ChildSlot
	.HAlign(HAlign_Left)
	.VAlign(VAlign_Center)
	.Padding(40.f)
	[
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.03f, 0.82f))
		.Padding(24.f)
		[
			SNew(SBox)
			.WidthOverride(520.f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 16)
				[
					SNew(STextBlock).Text(LOCTEXT("Title", "Create your character")).Font(Font(22, true)).ColorAndOpacity(FLinearColor::White)
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					MakeRow(LOCTEXT("Name", "Name"),
						SAssignNew(NameBox, SEditableTextBox).Text(FText::FromString(InArgs._InitialName)).Font(Font(13)).MinDesiredWidth(220.f))
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					MakeRow(LOCTEXT("Body", "Body"),
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)
						[ MakeButton(LOCTEXT("Male", "Male"), [this]() { if (FAstoniaAppearance* A = Look()) { A->bFemale = false; A->Beard = NAME_None; } Changed(true); }) ]
						+ SHorizontalBox::Slot().AutoWidth()
						[ MakeButton(LOCTEXT("Female", "Female"), [this]() { if (FAstoniaAppearance* A = Look()) { A->bFemale = true; A->Beard = NAME_None; A->Mustache = NAME_None; } Changed(true); }) ])
				]
				+ SVerticalBox::Slot().AutoHeight() [ MakeRow(LOCTEXT("Skin", "Skin tone"), MakeSwatches(Skin, false)) ]
				+ SVerticalBox::Slot().AutoHeight() [ MakeRow(LOCTEXT("Eyes", "Eye colour"), MakeSwatches(Eyes, false)) ]
				+ SVerticalBox::Slot().AutoHeight() [ MakeRow(LOCTEXT("Hair", "Hairstyle"), MakeStylePicker(EAstoniaGroomKind::Hair, true)) ]
				+ SVerticalBox::Slot().AutoHeight() [ MakeRow(LOCTEXT("HairColour", "Hair colour"), MakeSwatches(Hair, false)) ]
				+ SVerticalBox::Slot().AutoHeight() [ MakeRow(LOCTEXT("Brows", "Eyebrows"), MakeStylePicker(EAstoniaGroomKind::Eyebrows, true)) ]
				+ SVerticalBox::Slot().AutoHeight() [ MakeRow(LOCTEXT("Beard", "Beard"), MakeStylePicker(EAstoniaGroomKind::Beards, true)) ]
				+ SVerticalBox::Slot().AutoHeight() [ MakeRow(LOCTEXT("Mustache", "Moustache"), MakeStylePicker(EAstoniaGroomKind::Mustaches, true)) ]
				+ SVerticalBox::Slot().AutoHeight()
				[
					MakeRow(LOCTEXT("Turn", "Turn"),
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)
						[ MakeButton(LOCTEXT("Left", "<  Left"), [this]() { OnRotate.ExecuteIfBound(-30.f); }) ]
						+ SHorizontalBox::Slot().AutoWidth()
						[ MakeButton(LOCTEXT("Right", "Right  >"), [this]() { OnRotate.ExecuteIfBound(30.f); }) ])
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 20, 0, 0).HAlign(HAlign_Right)
				[
					MakeButton(LOCTEXT("Enter", "Enter Cameron"), [this]()
					{
						const FString Name = NameBox.IsValid() ? NameBox->GetText().ToString().TrimStartAndEnd() : FString();
						OnDone.ExecuteIfBound(Name.IsEmpty() ? TEXT("Adventurer") : Name);
					})
				]
			]
		]
	];
}

FAstoniaAppearance* SAstoniaCharacterCreator::Look() const
{
	return Appearance.IsValid() ? &Appearance->Appearance : nullptr;
}

void SAstoniaCharacterCreator::Changed(bool bRebuildBody) const
{
	if (!Appearance.IsValid()) return;
	if (bRebuildBody) Appearance->Apply();
	else Appearance->RefreshLook();
}

TSharedRef<SWidget> SAstoniaCharacterCreator::MakeRow(const FText& Label, TSharedRef<SWidget> Content) const
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 6)
		[
			SNew(SBox).WidthOverride(120.f)
			[
				SNew(STextBlock).Text(Label).Font(Font(13)).ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f))
			]
		]
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(0, 6)
		[
			Content
		];
}

TSharedRef<SWidget> SAstoniaCharacterCreator::MakeButton(const FText& Text, TFunction<void()> OnClick) const
{
	return SNew(SButton)
		.ContentPadding(FMargin(12.f, 4.f))
		.OnClicked_Lambda([OnClick]() { OnClick(); return FReply::Handled(); })
		[
			SNew(STextBlock).Text(Text).Font(Font(12, true))
		];
}

TSharedRef<SWidget> SAstoniaCharacterCreator::MakeSwatches(TArray<FSwatch> Swatches, bool bRebuildBody)
{
	TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
	for (FSwatch& Swatch : Swatches)
	{
		TFunction<void(FAstoniaAppearance&)> Apply = MoveTemp(Swatch.Apply);
		Row->AddSlot().AutoWidth().Padding(0, 0, 3, 0)
		[
			SNew(SButton)
			.ContentPadding(FMargin(2.f))
			.OnClicked_Lambda([this, Apply, bRebuildBody]()
			{
				if (FAstoniaAppearance* A = Look()) Apply(*A);
				Changed(bRebuildBody);
				return FReply::Handled();
			})
			[
				SNew(SBox).WidthOverride(22.f).HeightOverride(22.f)
				[
					SNew(SColorBlock).Color(Swatch.Shown)
				]
			]
		];
	}
	return Row;
}

TSharedRef<SWidget> SAstoniaCharacterCreator::MakeStylePicker(EAstoniaGroomKind Kind, bool bAllowNone)
{
	// Cycles through None (if allowed) and every installed style of this kind.
	auto Current = [this, Kind]() -> FName*
	{
		FAstoniaAppearance* A = Look();
		if (!A) return nullptr;
		switch (Kind)
		{
		case EAstoniaGroomKind::Hair: return &A->HairStyle;
		case EAstoniaGroomKind::Eyebrows: return &A->Eyebrows;
		case EAstoniaGroomKind::Beards: return &A->Beard;
		default: return &A->Mustache;
		}
	};
	auto Step = [this, Kind, bAllowNone, Current](int32 Dir)
	{
		FName* Value = Current();
		if (!Value) return;
		TArray<FName> Options;
		if (bAllowNone) Options.Add(NAME_None);
		Options.Append(AstoniaGrooms::Styles(Kind));
		const int32 Index = FMath::Max(0, Options.IndexOfByKey(*Value));
		*Value = Options[(Index + Dir + Options.Num()) % Options.Num()];
		Changed(false);
	};

	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth()
		[ MakeButton(FText::FromString(TEXT("<")), [Step]() { Step(-1); }) ]
		+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).HAlign(HAlign_Center).Padding(8, 0)
		[
			SNew(STextBlock)
			.Font(Font(12))
			.ColorAndOpacity(FLinearColor::White)
			.Text_Lambda([Current]() { FName* V = Current(); return V ? AstoniaGrooms::DisplayName(*V) : FText::GetEmpty(); })
		]
		+ SHorizontalBox::Slot().AutoWidth()
		[ MakeButton(FText::FromString(TEXT(">")), [Step]() { Step(1); }) ];
}

#undef LOCTEXT_NAMESPACE
