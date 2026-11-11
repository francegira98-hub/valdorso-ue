// Valdorso - I permessi dello staff (v0.1.4, 10/11/2026). Scritto da Claude, chiesto da Fra: "inizia a prevedere dei
// ruoli admin per ogni cosa".
//
// Ogni account ha un ruolo (Giocatore, Staff, Amministratore: ValdorsoArchivista.h). Lo staff ha in piu' i suoi permessi,
// uno per cosa: l'Amministratore li ha tutti, un Giocatore nessuno. Li da' l'Amministratore dalla finestra dello staff
// (F9, scheda Permessi) o dalla console del server: Valdorso.Account.Permessi <nome> <gilda,bacheche,...|tutti|nessuno>.
// Il server li controlla a ogni azione (UValdorsoStaffComponent::ServerAzione, ValdorsoStaff.h).
// Quelli nuovi si aggiungono in fondo (i numeri restano: stanno nei file degli account).

#pragma once

#include "CoreMinimal.h"

namespace ValdorsoPermessi
{
	/** I gradi della Gilda, i punti e il capo gilda dei giocatori. */
	inline constexpr int32 Gilda = 1 << 0;
	/** Le bacheche: incarichi, proclami e leggi, la cassa del villaggio. */
	inline constexpr int32 Bacheche = 1 << 1;
	/** Il registro eventi (chi ha fatto cosa, dove). */
	inline constexpr int32 Registro = 1 << 2;
	/** Le prove sui coloni: oggetti, Aureus, cure, armeria, incarichi fatti. */
	inline constexpr int32 Prove = 1 << 3;
	/** Il mondo: ora, tempo, animali, raccolta, campane, fase del villaggio, sigillo. */
	inline constexpr int32 Mondo = 1 << 4;
	/** I giocatori: andare da un colono, chiamarlo, portarlo in un luogo. */
	inline constexpr int32 Giocatori = 1 << 5;
	/** I permessi e i ruoli degli altri (solo l'Amministratore). */
	inline constexpr int32 Permessi = 1 << 6;
	/** (11/11) Le abilita' dei coloni: impostarne i punti (per le prove, o per rimediare a un errore). */
	inline constexpr int32 Abilita = 1 << 7;

	inline constexpr int32 Quanti = 8;
	inline constexpr int32 Tutti = (1 << Quanti) - 1;
	/** Quelli di uno Staff nuovo (o di un file di prima): Gilda, Bacheche, Registro. */
	inline constexpr int32 DiSerieStaff = Gilda | Bacheche | Registro;

	/** Il nome del permesso numero Indice (0..Quanti-1): "gilda", "bacheche"... */
	inline const TCHAR* Nome(int32 Indice)
	{
		static const TCHAR* Nomi[Quanti] = { TEXT("gilda"), TEXT("bacheche"), TEXT("registro"), TEXT("prove"), TEXT("mondo"), TEXT("giocatori"), TEXT("permessi"), TEXT("abilita") };
		return Indice >= 0 && Indice < Quanti ? Nomi[Indice] : TEXT("?");
	}

	/** "gilda, bacheche" (o "nessuno", "tutti"). */
	inline FString Testo(int32 Maschera)
	{
		if ((Maschera & Tutti) == Tutti)
		{
			return TEXT("tutti");
		}
		TArray<FString> Parti;
		for (int32 i = 0; i < Quanti; ++i)
		{
			if (Maschera & (1 << i))
			{
				Parti.Add(Nome(i));
			}
		}
		return Parti.IsEmpty() ? FString(TEXT("nessuno")) : FString::Join(Parti, TEXT(", "));
	}

	/** Da "gilda,bacheche" (o "tutti", "nessuno"): falso se c'e' un nome che non esiste. */
	inline bool DaTesto(const FString& Lista, int32& OutMaschera)
	{
		OutMaschera = 0;
		TArray<FString> Parti;
		Lista.ParseIntoArray(Parti, TEXT(","), true);
		for (FString Parte : Parti)
		{
			Parte = Parte.TrimStartAndEnd().ToLower();
			if (Parte == TEXT("tutti"))
			{
				OutMaschera = Tutti;
				continue;
			}
			if (Parte == TEXT("nessuno"))
			{
				continue;
			}
			bool bTrovato = false;
			for (int32 i = 0; i < Quanti; ++i)
			{
				if (Parte == Nome(i))
				{
					OutMaschera |= 1 << i;
					bTrovato = true;
				}
			}
			if (!bTrovato)
			{
				return false;
			}
		}
		return true;
	}
}
