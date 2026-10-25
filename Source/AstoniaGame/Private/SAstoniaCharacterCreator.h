#pragma once

#include "CoreMinimal.h"
#include "AstoniaAppearance.h"
#include "Widgets/SCompoundWidget.h"

class UAstoniaAppearanceComponent;
class SEditableTextBox;

/**
 * Character creation panel: name, body, skin tone, eye colour, hair style and colour, eyebrows, beard
 * and moustache. Edits the character's appearance component live; the caller saves on Done.
 */
class SAstoniaCharacterCreator : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_OneParam(FOnRotate, float /*Degrees*/);
	DECLARE_DELEGATE_OneParam(FOnDone, const FString& /*Name*/);

	SLATE_BEGIN_ARGS(SAstoniaCharacterCreator) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UAstoniaAppearanceComponent>, Appearance)
		SLATE_ARGUMENT(FString, InitialName)
		SLATE_EVENT(FOnRotate, OnRotate)
		SLATE_EVENT(FOnDone, OnDone)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	struct FSwatch
	{
		FLinearColor Shown;
		TFunction<void(FAstoniaAppearance&)> Apply;
	};

	TSharedRef<SWidget> MakeRow(const FText& Label, TSharedRef<SWidget> Content) const;
	TSharedRef<SWidget> MakeSwatches(TArray<FSwatch> Swatches, bool bRebuildBody);
	TSharedRef<SWidget> MakeStylePicker(EAstoniaGroomKind Kind, bool bAllowNone);
	TSharedRef<SWidget> MakeButton(const FText& Text, TFunction<void()> OnClick) const;
	FAstoniaAppearance* Look() const;
	void Changed(bool bRebuildBody) const;

	TWeakObjectPtr<UAstoniaAppearanceComponent> Appearance;
	TSharedPtr<SEditableTextBox> NameBox;
	FOnRotate OnRotate;
	FOnDone OnDone;
};
