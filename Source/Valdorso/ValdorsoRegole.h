// Valdorso - Le regole degli account e i file dell'archivio, separati dall'archivista perché i test
// automatici li possano controllare da soli. Spostati da ValdorsoArchivista.cpp da Claude il 04/10/2026,
// senza cambiare niente di come funzionano.
//
//   ValdorsoRegole        nomi, password, codici d'invito, durata dei blocchi
//   ValdorsoArchivioFile  scrittura sicura (.tmp, .bak1, .bak2) e lettura con le copie di riserva

#pragma once

#include "CoreMinimal.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Valdorso.h"

namespace ValdorsoRegole
{
	/** Password sbagliate di fila prima del blocco dell'account. */
	inline constexpr int32 TentativiPrimaDelBlocco = 5;

	/** Il primo blocco dura 15 minuti, ogni blocco successivo il doppio, fino a 24 ore. */
	inline constexpr int64 BloccoBaseSecondi = 15 * 60;
	inline constexpr int64 BloccoMassimoSecondi = 24 * 60 * 60;

	/** Il rientro senza password vale per 15 minuti dopo che il collegamento si è interrotto (v0.1.2). */
	inline constexpr int64 DurataRientroSecondi = 15 * 60;

	/** Privacy: gli indirizzi IP nel registro degli accessi si tengono al massimo 30 giorni, poi si oscurano. */
	inline constexpr int32 GiorniIndirizzi = 30;

	/** Caratteri casuali di un codice d'invito (VALD-XXXX-XXXX-XXXX). */
	inline constexpr int32 LunghezzaCodice = 12;

	/** Nome dell'account: 3-20 caratteri, lettere, cifre, . - _; inizia con una lettera. */
	VALDORSO_API bool NomeValido(const FString& Nome);

	/** Nomi che nessun giocatore può prendere (admin, staff, valdorso...). Vuole il nome in minuscolo. */
	VALDORSO_API bool NomeRiservato(const FString& NomeChiave);

	/** Vuoto se la password va bene, altrimenti il motivo da mostrare al giocatore. */
	VALDORSO_API FString ProblemaPassword(const FString& Password, const FString& Nome);

	/** Toglie trattini, spazi e "VALD" davanti; tutto maiuscolo. */
	VALDORSO_API FString NormalizzaCodice(const FString& Codice);

	/** Quanto dura il blocco numero BlocchiDiFila (1 = il primo), in secondi. */
	VALDORSO_API int64 DurataBlocco(int32 BlocchiDiFila);
}

namespace ValdorsoArchivioFile
{
	/** Scrive il file in modo che non resti mai a metà: prima un .tmp, poi le copie (.bak1, .bak2), poi la sostituzione. */
	VALDORSO_API void ScriviAtomico(const FString& Percorso, const FString& Testo);

	/** Sostituisce il file senza lasciare copie (.bak): per il registro, dove le copie terrebbero i dati tolti. */
	VALDORSO_API void SostituisciSenzaCopie(const FString& Percorso, const FString& Testo);

	/** Cancella un file dell'archivio con le sue copie (.bak1, .bak2) e l'eventuale .tmp. */
	VALDORSO_API void CancellaConCopie(const FString& Percorso);

	/**
	 * Privacy: nelle righe del registro più vecchie di Giorni, "| ip 1.2.3.4" diventa "| ip [rimosso]".
	 * Le righe restano (quando, cosa, chi): sparisce solo l'indirizzo. OutCambiate = quante righe sono cambiate.
	 */
	VALDORSO_API FString OscuraIndirizziVecchi(const FString& Testo, const FDateTime& Ora, int32 Giorni, int32& OutCambiate);

	/** Privacy: nel registro, ogni campo (tra " | ") uguale a Nome, maiuscole o no, diventa Con. */
	VALDORSO_API FString SostituisciNome(const FString& Testo, const FString& Nome, const FString& Con, int32& OutCambiate);

	/** Legge un file dell'archivio; se è rovinato prova le due copie precedenti. */
	template <typename TipoDati>
	bool LeggiFile(const FString& Percorso, TipoDati& Out)
	{
		const TArray<FString> Prove = { Percorso, Percorso + TEXT(".bak1"), Percorso + TEXT(".bak2") };
		for (const FString& Prova : Prove)
		{
			FString Testo;
			TipoDati Letto;
			if (FFileHelper::LoadFileToString(Testo, *Prova) && FJsonObjectConverter::JsonObjectStringToUStruct(Testo, &Letto, 0, 0))
			{
				if (Prova != Percorso)
				{
					UE_LOG(LogValdorso, Warning, TEXT("[Valdorso] Archivista: %s era rovinato, letto dalla copia %s"), *Percorso, *Prova);
				}
				Out = MoveTemp(Letto);
				return true;
			}
		}
		return false;
	}
}
