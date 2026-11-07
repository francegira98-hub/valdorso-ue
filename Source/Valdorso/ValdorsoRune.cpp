// Valdorso - La lingua delle rune (vedi il .h). Scritto da Claude il 07/11/2026.

#include "ValdorsoRune.h"
#include "Rendering/DrawElements.h"

#define LOCTEXT_NAMESPACE "ValdorsoRune"

namespace
{
#include "ValdorsoRuneTratti.inl"

	// Le misure di una parola, come in alfabeto_rune.py: il posto di una runa e il trattino tra due rune (unità della
	// runa, che è alta 2).
	constexpr float PassoRuna = 2.0f;
	constexpr float GiuntoRune = 0.9f;

	const TArray<TArray<TArray<FVector2D>>>& TutteLeLinee()
	{
		static const TArray<TArray<TArray<FVector2D>>> Tutte = []()
		{
			TArray<TArray<TArray<FVector2D>>> Fuori;
			Fuori.SetNum(static_cast<int32>(EValdorsoRuna::Num));
			int32 Indice = 0;
			const int32 Totale = UE_ARRAY_COUNT(GValdorsoRuneDati);
			for (int32 Runa = 0; Runa < static_cast<int32>(EValdorsoRuna::Num) && Indice < Totale; ++Runa)
			{
				while (Indice < Totale)
				{
					const int32 Punti = FMath::RoundToInt(GValdorsoRuneDati[Indice++]);
					if (Punti <= 0)
					{
						break;  // fine della runa
					}
					TArray<FVector2D>& Linea = Fuori[Runa].AddDefaulted_GetRef();
					for (int32 k = 0; k < Punti && Indice + 1 < Totale; ++k, Indice += 2)
					{
						Linea.Add(FVector2D(GValdorsoRuneDati[Indice], GValdorsoRuneDati[Indice + 1]));
					}
				}
			}
			return Fuori;
		}();
		return Tutte;
	}

	const TCHAR* const NomiRune[] = {
		TEXT("Aeth"), TEXT("Nir"), TEXT("Dorn"), TEXT("Sil"), TEXT("Lume"),
		TEXT("Var"), TEXT("Ka"), TEXT("Ren"), TEXT("Vel"), TEXT("Tor"), TEXT("Ur") };
	static_assert(UE_ARRAY_COUNT(NomiRune) == static_cast<int32>(EValdorsoRuna::Num), "un nome per ogni runa");
}

namespace ValdorsoRune
{
	const TArray<TArray<FVector2D>>& Linee(EValdorsoRuna Runa)
	{
		static const TArray<TArray<FVector2D>> Vuota;
		const int32 i = static_cast<int32>(Runa);
		return TutteLeLinee().IsValidIndex(i) ? TutteLeLinee()[i] : Vuota;
	}

	FString Nome(EValdorsoRuna Runa)
	{
		const int32 i = static_cast<int32>(Runa);
		return i >= 0 && i < static_cast<int32>(EValdorsoRuna::Num) ? FString(NomiRune[i]) : FString();
	}

	FText Significato(EValdorsoRuna Runa)
	{
		switch (Runa)
		{
		case EValdorsoRuna::Aeth: return LOCTEXT("Aeth", "il fuoco");
		case EValdorsoRuna::Nir:  return LOCTEXT("Nir", "l'acqua");
		case EValdorsoRuna::Dorn: return LOCTEXT("Dorn", "la terra");
		case EValdorsoRuna::Sil:  return LOCTEXT("Sil", "il vento");
		case EValdorsoRuna::Lume: return LOCTEXT("Lume", "la luce");
		case EValdorsoRuna::Var:  return LOCTEXT("Var", "andare, scagliare");
		case EValdorsoRuna::Ka:   return LOCTEXT("Ka", "proteggere");
		case EValdorsoRuna::Ren:  return LOCTEXT("Ren", "guarire");
		case EValdorsoRuna::Vel:  return LOCTEXT("Vel", "vedere, guidare");
		case EValdorsoRuna::Tor:  return LOCTEXT("Tor", "fermare");
		case EValdorsoRuna::Ur:   return LOCTEXT("Ur", "orso, custode, la porta");
		default:                  return FText::GetEmpty();
		}
	}

	bool Elemento(EValdorsoRuna Runa)
	{
		return Runa >= EValdorsoRuna::Aeth && Runa <= EValdorsoRuna::Lume;
	}

	bool Verbo(EValdorsoRuna Runa)
	{
		return Runa >= EValdorsoRuna::Var && Runa <= EValdorsoRuna::Tor;
	}

	bool DaNome(const FString& Testo, EValdorsoRuna& Fuori)
	{
		const FString Pulito = Testo.TrimStartAndEnd();
		for (int32 i = 0; i < static_cast<int32>(EValdorsoRuna::Num); ++i)
		{
			if (Pulito.Equals(NomiRune[i], ESearchCase::IgnoreCase))
			{
				Fuori = static_cast<EValdorsoRuna>(i);
				return true;
			}
		}
		return false;
	}

	bool LeggiParola(const FString& Testo, TArray<EValdorsoRuna>& Fuori)
	{
		Fuori.Reset();
		TArray<FString> Pezzi;
		Testo.ParseIntoArray(Pezzi, TEXT("-"), true);
		for (const FString& Pezzo : Pezzi)
		{
			EValdorsoRuna Runa;
			if (!DaNome(Pezzo, Runa))
			{
				Fuori.Reset();
				return false;
			}
			Fuori.Add(Runa);
		}
		return Fuori.Num() > 0;
	}

	FString ScriviParola(const TArray<EValdorsoRuna>& Parola)
	{
		TArray<FString> Nomi;
		for (EValdorsoRuna Runa : Parola)
		{
			Nomi.Add(Nome(Runa));
		}
		return FString::Join(Nomi, TEXT("-"));
	}

	bool IncantesimoValido(const TArray<EValdorsoRuna>& Parola)
	{
		if (Parola.Num() < 2 || Parola.Num() > 3 || !Elemento(Parola[0]))
		{
			return false;
		}
		for (int32 i = 1; i < Parola.Num(); ++i)
		{
			if (!Verbo(Parola[i]))
			{
				return false;
			}
		}
		return Parola.Num() < 3 || Parola[1] != Parola[2];
	}

	FText NomeIncantesimo(const TArray<EValdorsoRuna>& Parola)
	{
		const FString Scritta = ScriviParola(Parola);
		for (const auto& Voce : GValdorsoRuneParole)
		{
			if (Scritta.Equals(Voce[0], ESearchCase::IgnoreCase))
			{
				return FText::FromString(Voce[1]);
			}
		}
		return FText::GetEmpty();
	}

	int32 Grado(const TArray<EValdorsoRuna>& Parola)
	{
		return IncantesimoValido(Parola) ? Parola.Num() : 0;
	}

	TArray<TPair<FString, FText>> ParoleConNome()
	{
		TArray<TPair<FString, FText>> Fuori;
		for (const auto& Voce : GValdorsoRuneParole)
		{
			Fuori.Emplace(FString(Voce[0]), FText::FromString(Voce[1]));
		}
		return Fuori;
	}

	TArray<FString> RigheQuaderno()
	{
		auto Riga = [](std::initializer_list<EValdorsoRuna> Rune)
		{
			TArray<FString> Nomi;
			TArray<FString> Didascalie;
			for (EValdorsoRuna Runa : Rune)
			{
				Nomi.Add(Nome(Runa));
				Didascalie.Add(FString::Printf(TEXT("%s, %s"), *Nome(Runa), *Significato(Runa).ToString()));
			}
			return FString::Printf(TEXT("%s %s | %s"), PrefissoRiga, *FString::Join(Nomi, TEXT(" ")),
				*FString::Join(Didascalie, TEXT(" · ")));
		};
		TArray<FString> Righe;
		Righe.Add(LOCTEXT("QElementi", "Le rune degli elementi stanno sulla linea di base.").ToString());
		Righe.Add(Riga({ EValdorsoRuna::Aeth, EValdorsoRuna::Nir, EValdorsoRuna::Dorn }));
		Righe.Add(Riga({ EValdorsoRuna::Sil, EValdorsoRuna::Lume }));
		Righe.Add(LOCTEXT("QVerbi", "Le rune verbo non hanno la base: sono un gesto.").ToString());
		Righe.Add(Riga({ EValdorsoRuna::Var, EValdorsoRuna::Ka, EValdorsoRuna::Ren }));
		Righe.Add(Riga({ EValdorsoRuna::Vel, EValdorsoRuna::Tor }));
		Righe.Add(LOCTEXT("QUr", "Ur si scrive da sola, sopra. Non entra nelle parole degli incantesimi.").ToString());
		Righe.Add(Riga({ EValdorsoRuna::Ur }));
		Righe.Add(LOCTEXT("QParole", "Un incantesimo è un elemento e un verbo. Una terza runa lo piega: sono le parole di grado III, e si imparano scrivendole con calma, lontano dagli scontri.").ToString());
		for (const TPair<FString, FText>& Parola : ParoleConNome())
		{
			Righe.Add(FString::Printf(TEXT("%s %s | %s · %s"), PrefissoRiga, *Parola.Key, *Parola.Key, *Parola.Value.ToString()));
		}
		return Righe;
	}

	bool LeggiRiga(const FString& Riga, TArray<TArray<EValdorsoRuna>>& Parole)
	{
		Parole.Reset();
		FString SenzaDidascalia = Riga;
		int32 Barra = INDEX_NONE;
		if (SenzaDidascalia.FindChar(TEXT('|'), Barra))
		{
			SenzaDidascalia.LeftInline(Barra);
		}
		const FString Pulita = SenzaDidascalia.TrimStart();
		if (!Pulita.StartsWith(PrefissoRiga, ESearchCase::IgnoreCase))
		{
			return false;
		}
		TArray<FString> Pezzi;
		Pulita.Mid(FCString::Strlen(PrefissoRiga)).ParseIntoArrayWS(Pezzi);
		for (const FString& Pezzo : Pezzi)
		{
			TArray<EValdorsoRuna> Parola;
			if (LeggiParola(Pezzo, Parola))
			{
				Parole.Add(MoveTemp(Parola));
			}
		}
		return Parole.Num() > 0;
	}
}

// ------------------------------------------------------------------------------------------------- SValdorsoRuna

void SValdorsoRuna::Construct(const FArguments& InArgs)
{
	Parola = InArgs._Parola;
	Altezza = FMath::Max(4.f, InArgs._Altezza);
	Spessore = FMath::Max(0.5f, InArgs._Spessore);
	Colore = InArgs._Colore;
}

void SValdorsoRuna::ImpostaParola(const TArray<EValdorsoRuna>& Nuova)
{
	Parola = Nuova;
	Invalidate(EInvalidateWidgetReason::Layout);
}

FVector2D SValdorsoRuna::ComputeDesiredSize(float) const
{
	const float Scala = Altezza / 2.f;
	const int32 n = FMath::Max(1, Parola.Num());
	return FVector2D((n * PassoRuna + (n - 1) * GiuntoRune) * Scala + Spessore * 2.f, Altezza + Spessore * 2.f);
}

int32 SValdorsoRuna::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	if (Parola.Num() == 0)
	{
		return LayerId;
	}
	const FVector2f Misura = FVector2f(AllottedGeometry.GetLocalSize());
	const float Scala = Altezza / 2.f;
	const float Larghezza = (Parola.Num() * PassoRuna + (Parola.Num() - 1) * GiuntoRune) * Scala;
	// la parola sta al centro dello spazio che le danno
	const float Inizio = (Misura.X - Larghezza) / 2.f;
	const float Mezzo = Misura.Y / 2.f;
	const FLinearColor Tinta = Colore.Get() * InWidgetStyle.GetColorAndOpacityTint();

	auto Punto = [&](float Centro, const FVector2D& P)
	{
		return FVector2f(Centro + static_cast<float>(P.X) * Scala, Mezzo - static_cast<float>(P.Y) * Scala);
	};
	for (int32 k = 0; k < Parola.Num(); ++k)
	{
		const float Centro = Inizio + Scala * (PassoRuna / 2.f + k * (PassoRuna + GiuntoRune));
		for (const TArray<FVector2D>& Linea : ValdorsoRune::Linee(Parola[k]))
		{
			TArray<FVector2f> Punti;
			Punti.Reserve(Linea.Num());
			for (const FVector2D& P : Linea)
			{
				Punti.Add(Punto(Centro, P));
			}
			if (Punti.Num() >= 2)
			{
				FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Punti,
					ESlateDrawEffect::None, Tinta, true, Spessore);
			}
		}
		if (k < Parola.Num() - 1)
		{
			// il trattino che unisce le rune di una parola, a metà altezza
			const float Da = Centro + (PassoRuna / 2.f - 0.05f) * Scala;
			const float A = Centro + (PassoRuna / 2.f + GiuntoRune + 0.05f) * Scala;
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
				TArray<FVector2f>{ FVector2f(Da, Mezzo), FVector2f(A, Mezzo) }, ESlateDrawEffect::None, Tinta, true, Spessore);
		}
	}
	return LayerId;
}

#undef LOCTEXT_NAMESPACE
