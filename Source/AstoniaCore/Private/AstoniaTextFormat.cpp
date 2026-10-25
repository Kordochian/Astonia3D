#include "AstoniaTextFormat.h"
#include "AstoniaCore.h"

namespace
{
	const TCHAR* MapFlagNames[] = {
		TEXT("MF_MOVEBLOCK"), TEXT("MF_SIGHTBLOCK"), TEXT("MF_TMOVEBLOCK"), TEXT("MF_TSIGHTBLOCK"),
		TEXT("MF_INDOORS"), TEXT("MF_RESTAREA"), TEXT("MF_DOOR"), TEXT("MF_SOUNDBLOCK"),
		TEXT("MF_TSOUNDBLOCK"), TEXT("MF_SHOUTBLOCK"), TEXT("MF_CLAN"), TEXT("MF_ARENA"),
		TEXT("MF_PEACE"), TEXT("MF_NEUTRAL"), TEXT("MF_FIRETHRU"), TEXT("MF_SLOWDEATH"),
		TEXT("MF_NOLIGHT"), TEXT("MF_NOMAGIC"), TEXT("MF_UNDERWATER"), TEXT("MF_NOREGEN"),
	};

	bool IsWordChar(TCHAR C) { return FChar::IsAlnum(C) || C == '_' || C == '-'; }

	enum class ETokenKind { Pair, Block, End, Comment, Eof, Error };

	/** Port of get_text(): reads one `name=value`, `name:`, `;` or `# comment`. */
	struct FTokenizer
	{
		const FString& Text;
		int32 Pos = 0;
		int32 Line = 1;

		explicit FTokenizer(const FString& InText) : Text(InText) {}

		TCHAR Peek() const { return Pos < Text.Len() ? Text[Pos] : 0; }

		void SkipSpace()
		{
			while (Pos < Text.Len() && FChar::IsWhitespace(Text[Pos]))
			{
				if (Text[Pos] == '\n') Line++;
				Pos++;
			}
		}

		ETokenKind Next(FString& Name, FString& Value)
		{
			Name.Reset();
			Value.Reset();
			SkipSpace();
			const TCHAR C = Peek();
			if (C == 0) return ETokenKind::Eof;
			if (C == ';') { Pos++; return ETokenKind::End; }
			if (C == '#')
			{
				while (Pos < Text.Len() && Text[Pos] != '\n' && Text[Pos] != '\r') Pos++;
				return ETokenKind::Comment;
			}
			while (IsWordChar(Peek())) Name.AppendChar(Text[Pos++]);
			SkipSpace();
			if (Peek() == ':') { Pos++; return ETokenKind::Block; }
			if (Peek() != '=') return ETokenKind::Error;
			Pos++;
			SkipSpace();
			if (Peek() == '"')
			{
				Pos++;
				while (Pos < Text.Len() && Text[Pos] != '"')
				{
					if (Text[Pos] == '\n') Line++;
					Value.AppendChar(Text[Pos++]);
				}
				if (Peek() != '"') return ETokenKind::Error;
				Pos++;
				return ETokenKind::Pair;
			}
			while (IsWordChar(Peek())) Value.AppendChar(Text[Pos++]);
			return ETokenKind::Pair;
		}
	};

	bool SplitComma(const FString& Value, int32& A, int32& B)
	{
		FString L, R;
		if (!Value.Split(TEXT(","), &L, &R)) return false;
		A = FCString::Atoi(*L.TrimStartAndEnd());
		B = FCString::Atoi(*R.TrimStartAndEnd());
		return true;
	}
}

const FString* FAstoniaTemplate::Find(const FString& Key) const
{
	for (const FAstoniaKeyValue& E : Entries)
	{
		if (E.Key.Equals(Key, ESearchCase::IgnoreCase)) return &E.Value;
	}
	return nullptr;
}

void FAstoniaTemplate::FindAll(const FString& Key, TArray<FString>& Out) const
{
	for (const FAstoniaKeyValue& E : Entries)
	{
		if (E.Key.Equals(Key, ESearchCase::IgnoreCase)) Out.Add(E.Value);
	}
}

int32 FAstoniaTemplate::GetInt(const FString& Key, int32 Default) const
{
	const FString* V = Find(Key);
	return V ? FCString::Atoi(**V) : Default;
}

int32 AstoniaTextFormat::LookupMapFlag(const FString& Name)
{
	for (int32 i = 0; i < UE_ARRAY_COUNT(MapFlagNames); i++)
	{
		if (Name.Equals(MapFlagNames[i], ESearchCase::IgnoreCase)) return i;
	}
	return INDEX_NONE;
}

bool AstoniaTextFormat::ParseTemplates(const FString& Text, TArray<FAstoniaTemplate>& Out, FString& OutError)
{
	FTokenizer Tok(Text);
	FString Name, Value;
	FAstoniaTemplate* Current = nullptr;

	for (;;)
	{
		switch (Tok.Next(Name, Value))
		{
		case ETokenKind::Eof:
			return true;
		case ETokenKind::Comment:
			break;
		case ETokenKind::Block:
			Current = &Out.AddDefaulted_GetRef();
			Current->Name = Name;
			break;
		case ETokenKind::End:
			Current = nullptr;
			break;
		case ETokenKind::Pair:
			if (!Current)
			{
				OutError = FString::Printf(TEXT("line %d: %s= outside a template"), Tok.Line, *Name);
				return false;
			}
			Current->Entries.Add({Name, Value});
			break;
		case ETokenKind::Error:
			OutError = FString::Printf(TEXT("line %d: syntax error near \"%s\""), Tok.Line, *Name);
			return false;
		}
	}
}

bool AstoniaTextFormat::ParseMap(const FString& Text, FAstoniaMap& Out, FString& OutError)
{
	Out.Size = AstoniaAreaSize;
	Out.Tiles.Reset();
	Out.Tiles.SetNum(Out.Size * Out.Size);

	FTokenizer Tok(Text);
	FString Name, Value;
	int32 OX = 0, OY = 0, X = 0, Y = 0, FX = 0, FY = 0;

	auto Fail = [&OutError, &Tok](const TCHAR* What)
	{
		OutError = FString::Printf(TEXT("line %d: %s"), Tok.Line, What);
		return false;
	};
	auto InBounds = [&Out](int32 TX, int32 TY) { return TX >= 0 && TY >= 0 && TX < Out.Size && TY < Out.Size; };

	for (;;)
	{
		const ETokenKind Kind = Tok.Next(Name, Value);
		if (Kind == ETokenKind::Eof) return true;
		if (Kind == ETokenKind::Comment) continue;
		if (Kind != ETokenKind::Pair) return Fail(TEXT("syntax error"));

		FAstoniaMapTile& Tile = Out.Tiles[X + Y * Out.Size];

		if (Name.Equals(TEXT("origin"), ESearchCase::IgnoreCase))
		{
			if (!SplitComma(Value, OX, OY)) return Fail(TEXT("origin needs x,y"));
		}
		else if (Name.Equals(TEXT("field"), ESearchCase::IgnoreCase))
		{
			if (!SplitComma(Value, X, Y)) return Fail(TEXT("field needs x,y"));
			X += OX;
			Y += OY;
			if (!InBounds(X, Y)) return Fail(TEXT("field outside the map"));
			Out.Tiles[X + Y * Out.Size] = FAstoniaMapTile();
		}
		else if (Name.Equals(TEXT("from"), ESearchCase::IgnoreCase))
		{
			if (!SplitComma(Value, FX, FY)) return Fail(TEXT("from needs x,y"));
			FX += OX;
			FY += OY;
		}
		else if (Name.Equals(TEXT("to"), ESearchCase::IgnoreCase))
		{
			// Copies the current field into every tile of the from..to rectangle (field2square()).
			int32 TX, TY;
			if (!SplitComma(Value, TX, TY)) return Fail(TEXT("to needs x,y"));
			TX += OX;
			TY += OY;
			if (!InBounds(FX, FY) || !InBounds(TX, TY)) return Fail(TEXT("from/to outside the map"));
			const FAstoniaMapTile Source = Tile;
			for (int32 YY = FY; YY <= TY; YY++)
			{
				for (int32 XX = FX; XX <= TX; XX++)
				{
					Out.Tiles[XX + YY * Out.Size] = Source;
				}
			}
		}
		else if (Name.Equals(TEXT("gsprite"), ESearchCase::IgnoreCase)) Tile.GroundSprite = FCString::Atoi(*Value);
		else if (Name.Equals(TEXT("fsprite"), ESearchCase::IgnoreCase)) Tile.ForegroundSprite = FCString::Atoi(*Value);
		else if (Name.Equals(TEXT("it"), ESearchCase::IgnoreCase)) Tile.Item = Value;
		else if (Name.Equals(TEXT("ch"), ESearchCase::IgnoreCase)) Tile.Character = Value;
		else if (Name.Equals(TEXT("flag"), ESearchCase::IgnoreCase))
		{
			const int32 Bit = LookupMapFlag(Value);
			if (Bit == INDEX_NONE) return Fail(*FString::Printf(TEXT("unknown map flag %s"), *Value));
			Tile.Flags |= EAstoniaMapFlag(1u << Bit);
		}
		else
		{
			return Fail(*FString::Printf(TEXT("unknown key %s"), *Name));
		}
	}
}
