// Valdorso - I tasti da cambiare (vedi ValdorsoTasti.h). Scritto da Claude l'11/11/2026.

#include "ValdorsoTasti.h"
#include "Valdorso.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Misc/ConfigCacheIni.h"

#define LOCTEXT_NAMESPACE "ValdorsoTasti"

namespace
{
	const TCHAR* TastiSezione = TEXT("Valdorso.Tasti");

	struct FTastoDiSerie
	{
		EValdorsoTasto Azione;
		const TCHAR* Chiave;
		FKey Tasto;
		/** Per i tasti del corpo: un pezzo del nome dell'azione dell'Input Mapping Context ("Move", "Jump"...). */
		const TCHAR* AzioneIMC;
	};

	const TArray<FTastoDiSerie>& TastiTabella()
	{
		static const TArray<FTastoDiSerie> Tabella = {
			{ EValdorsoTasto::Avanti, TEXT("Avanti"), EKeys::W, TEXT("Move") },
			{ EValdorsoTasto::Indietro, TEXT("Indietro"), EKeys::S, TEXT("Move") },
			{ EValdorsoTasto::Sinistra, TEXT("Sinistra"), EKeys::A, TEXT("Move") },
			{ EValdorsoTasto::Destra, TEXT("Destra"), EKeys::D, TEXT("Move") },
			{ EValdorsoTasto::Salto, TEXT("Salto"), EKeys::SpaceBar, TEXT("Jump") },
			{ EValdorsoTasto::Corsa, TEXT("Corsa"), EKeys::LeftShift, TEXT("Sprint") },
			{ EValdorsoTasto::Schivata, TEXT("Schivata"), EKeys::C, TEXT("Schivata") },
			{ EValdorsoTasto::Colpo, TEXT("Colpo"), EKeys::LeftMouseButton, TEXT("Pugno") },
			{ EValdorsoTasto::Immergiti, TEXT("Immergiti"), EKeys::LeftControl, nullptr },
			{ EValdorsoTasto::Interagisci, TEXT("Interagisci"), EKeys::E, nullptr },
			{ EValdorsoTasto::Guardia, TEXT("Guardia"), EKeys::G, nullptr },
			{ EValdorsoTasto::Aggancio, TEXT("Aggancio"), EKeys::Tab, nullptr },
			{ EValdorsoTasto::Riponi, TEXT("Riponi"), EKeys::Z, nullptr },
			{ EValdorsoTasto::Zaino, TEXT("Zaino"), EKeys::I, nullptr },
			{ EValdorsoTasto::Scheda, TEXT("Scheda"), EKeys::K, nullptr },
			{ EValdorsoTasto::Taccuino, TEXT("Taccuino"), EKeys::J, nullptr },
			{ EValdorsoTasto::Mappa, TEXT("Mappa"), EKeys::M, nullptr },
			{ EValdorsoTasto::Quaderno, TEXT("Quaderno"), EKeys::R, nullptr },
			{ EValdorsoTasto::Foto, TEXT("Foto"), EKeys::P, nullptr },
			{ EValdorsoTasto::Chat, TEXT("Chat"), EKeys::Enter, nullptr },
		};
		return Tabella;
	}

	bool bTastiLetti = false;
	TMap<EValdorsoTasto, FKey> TastiScelti;

	const FTastoDiSerie* TastiVoce(EValdorsoTasto Azione)
	{
		return TastiTabella().FindByPredicate([Azione](const FTastoDiSerie& V) { return V.Azione == Azione; });
	}

	void TastiLeggi()
	{
		if (bTastiLetti)
		{
			return;
		}
		bTastiLetti = true;
		if (!GConfig)
		{
			return;
		}
		for (const FTastoDiSerie& V : TastiTabella())
		{
			FString Scritto;
			if (GConfig->GetString(TastiSezione, V.Chiave, Scritto, GGameUserSettingsIni) && !Scritto.IsEmpty())
			{
				const FKey Letto(*Scritto);
				if (Letto.IsValid() && !ValdorsoTasti::Riservato(Letto))
				{
					TastiScelti.Add(V.Azione, Letto);
				}
			}
		}
	}

	void TastiScrivi()
	{
		if (!GConfig)
		{
			return;
		}
		for (const FTastoDiSerie& V : TastiTabella())
		{
			const FKey* Scelto = TastiScelti.Find(V.Azione);
			if (Scelto && *Scelto != V.Tasto)
			{
				GConfig->SetString(TastiSezione, V.Chiave, *Scelto->GetFName().ToString(), GGameUserSettingsIni);
			}
			else
			{
				GConfig->RemoveKey(TastiSezione, V.Chiave, GGameUserSettingsIni);
			}
		}
		GConfig->Flush(false, GGameUserSettingsIni);
	}
}

namespace ValdorsoTasti
{
	FKey DiSerie(EValdorsoTasto Azione)
	{
		const FTastoDiSerie* V = TastiVoce(Azione);
		return V ? V->Tasto : EKeys::Invalid;
	}

	FKey Tasto(EValdorsoTasto Azione)
	{
		TastiLeggi();
		if (const FKey* Scelto = TastiScelti.Find(Azione))
		{
			return *Scelto;
		}
		return DiSerie(Azione);
	}

	FText Nome(EValdorsoTasto Azione)
	{
		switch (Azione)
		{
		case EValdorsoTasto::Avanti: return LOCTEXT("Avanti", "Avanti");
		case EValdorsoTasto::Indietro: return LOCTEXT("Indietro", "Indietro");
		case EValdorsoTasto::Sinistra: return LOCTEXT("Sinistra", "A sinistra");
		case EValdorsoTasto::Destra: return LOCTEXT("Destra", "A destra");
		case EValdorsoTasto::Salto: return LOCTEXT("Salto", "Salta (in acqua: risali)");
		case EValdorsoTasto::Corsa: return LOCTEXT("Corsa", "Corri (in acqua: nuota veloce)");
		case EValdorsoTasto::Schivata: return LOCTEXT("Schivata", "Schiva");
		case EValdorsoTasto::Colpo: return LOCTEXT("Colpo", "Colpisci");
		case EValdorsoTasto::Immergiti: return LOCTEXT("Immergiti", "In acqua: scendi");
		case EValdorsoTasto::Interagisci: return LOCTEXT("Interagisci", "Interagisci");
		case EValdorsoTasto::Guardia: return LOCTEXT("Guardia", "Cambia guardia");
		case EValdorsoTasto::Aggancio: return LOCTEXT("Aggancio", "Aggancia chi hai davanti");
		case EValdorsoTasto::Riponi: return LOCTEXT("Riponi", "Riponi o riprendi le armi");
		case EValdorsoTasto::Zaino: return LOCTEXT("Zaino", "Zaino");
		case EValdorsoTasto::Scheda: return LOCTEXT("Scheda", "Scheda del personaggio");
		case EValdorsoTasto::Taccuino: return LOCTEXT("Taccuino", "Taccuino");
		case EValdorsoTasto::Mappa: return LOCTEXT("Mappa", "Mappa");
		case EValdorsoTasto::Quaderno: return LOCTEXT("Quaderno", "Quaderno");
		case EValdorsoTasto::Foto: return LOCTEXT("Foto", "Modalita' foto");
		case EValdorsoTasto::Chat: return LOCTEXT("Chat", "Chat");
		default: return FText::GetEmpty();
		}
	}

	FText NomeTasto(const FKey& Tasto)
	{
		struct FNomeItaliano
		{
			FKey Tasto;
			const TCHAR* Nome;
		};
		static const FNomeItaliano Nomi[] = {
			{ EKeys::LeftShift, TEXT("Maiusc sinistro") }, { EKeys::RightShift, TEXT("Maiusc destro") },
			{ EKeys::LeftControl, TEXT("Ctrl sinistro") }, { EKeys::RightControl, TEXT("Ctrl destro") },
			{ EKeys::LeftAlt, TEXT("Alt sinistro") }, { EKeys::RightAlt, TEXT("Alt Gr") },
			{ EKeys::SpaceBar, TEXT("Spazio") }, { EKeys::Enter, TEXT("Invio") }, { EKeys::Tab, TEXT("Tab") },
			{ EKeys::BackSpace, TEXT("Cancella") }, { EKeys::CapsLock, TEXT("Bloc Maiusc") },
			{ EKeys::LeftMouseButton, TEXT("Clic sinistro") }, { EKeys::RightMouseButton, TEXT("Clic destro") },
			{ EKeys::MiddleMouseButton, TEXT("Tasto centrale del mouse") },
			{ EKeys::ThumbMouseButton, TEXT("Tasto laterale del mouse") }, { EKeys::ThumbMouseButton2, TEXT("Secondo tasto laterale") },
			{ EKeys::Up, TEXT("Freccia su") }, { EKeys::Down, TEXT("Freccia giu'") }, { EKeys::Left, TEXT("Freccia sinistra") },
			{ EKeys::Right, TEXT("Freccia destra") },
		};
		for (const FNomeItaliano& N : Nomi)
		{
			if (N.Tasto == Tasto)
			{
				return FText::FromString(N.Nome);
			}
		}
		return Tasto.IsValid() ? Tasto.GetDisplayName() : LOCTEXT("Nessuno", "nessuno");
	}

	bool Riservato(const FKey& Tasto)
	{
		static const FKey Fissi[] = { EKeys::Escape, EKeys::F10, EKeys::F9, EKeys::F7, EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four,
			EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::MouseX, EKeys::MouseY, EKeys::Mouse2D, EKeys::MouseWheelAxis,
			EKeys::MouseScrollUp, EKeys::MouseScrollDown };
		if (!Tasto.IsValid() || Tasto.IsGamepadKey() || Tasto.IsTouch() || Tasto.IsAxis1D() || Tasto.IsAxis2D())
		{
			return true;
		}
		for (const FKey& Fisso : Fissi)
		{
			if (Fisso == Tasto)
			{
				return true;
			}
		}
		return false;
	}

	EValdorsoTasto ChiHa(const FKey& Tasto)
	{
		for (const FTastoDiSerie& V : TastiTabella())
		{
			if (ValdorsoTasti::Tasto(V.Azione) == Tasto)
			{
				return V.Azione;
			}
		}
		return EValdorsoTasto::Numero;
	}

	bool Imposta(EValdorsoTasto Azione, const FKey& Nuovo)
	{
		TastiLeggi();
		if (Riservato(Nuovo) || !TastiVoce(Azione))
		{
			return false;
		}
		const FKey Vecchio = Tasto(Azione);
		if (Vecchio == Nuovo)
		{
			return true;
		}
		const EValdorsoTasto Altro = ChiHa(Nuovo);
		TastiScelti.Add(Azione, Nuovo);
		if (Altro != EValdorsoTasto::Numero && Altro != Azione)
		{
			TastiScelti.Add(Altro, Vecchio);
		}
		TastiScrivi();
		Cambiati().Broadcast();
		return true;
	}

	void RimettiDiSerie()
	{
		TastiLeggi();
		TastiScelti.Reset();
		TastiScrivi();
		Cambiati().Broadcast();
	}

	bool CorpoCambiato()
	{
		for (const FTastoDiSerie& V : TastiTabella())
		{
			if (V.AzioneIMC && Tasto(V.Azione) != V.Tasto)
			{
				return true;
			}
		}
		return false;
	}

	UInputMappingContext* Rimappa(UInputMappingContext* Originale, UObject* Outer)
	{
		if (!Originale || !Outer || !CorpoCambiato())
		{
			return Originale;
		}
		UInputMappingContext* Copia = NewObject<UInputMappingContext>(Outer, NAME_None, RF_Transient);
		int32 NumeroCambiati = 0;
		for (const FEnhancedActionKeyMapping& Vecchia : Originale->GetMappings())
		{
			const UInputAction* Azione = Vecchia.Action.Get();
			if (!Azione)
			{
				continue;
			}
			FKey Usato = Vecchia.Key;
			const FString NomeAzione = Azione->GetName();
			for (const FTastoDiSerie& V : TastiTabella())
			{
				if (V.AzioneIMC && Vecchia.Key == V.Tasto && NomeAzione.Contains(V.AzioneIMC))
				{
					Usato = Tasto(V.Azione);
					break;
				}
			}
			if (Usato != Vecchia.Key)
			{
				++NumeroCambiati;
			}
			FEnhancedActionKeyMapping& Nuova = Copia->MapKey(Azione, Usato);
			Nuova.Modifiers = Vecchia.Modifiers;
			Nuova.Triggers = Vecchia.Triggers;
		}
		UE_LOG(LogValdorso, Log, TEXT("[Valdorso] Tasti: copia di %s con %d tasti cambiati"), *Originale->GetName(), NumeroCambiati);
		return Copia;
	}

	FSimpleMulticastDelegate& Cambiati()
	{
		static FSimpleMulticastDelegate Delegato;
		return Delegato;
	}
}

#undef LOCTEXT_NAMESPACE
