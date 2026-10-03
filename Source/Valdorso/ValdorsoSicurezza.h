// Valdorso - Strumenti di sicurezza dell'archivista: numeri casuali sicuri, impronta delle password,
// confronti a tempo costante. Tutto passa da OpenSSL, che è già dentro Unreal.
// Scritto da Claude il 03/10/2026 (v0.1.2, passo 2.1).
//
// Perché un'impronta e non la password: il server non deve mai poter rileggere la password di nessuno.
// Salva solo il risultato di PBKDF2-SHA256 (600.000 giri, sale casuale): se qualcuno ruba l'archivio,
// per ogni password deve rifare tutti i giri, una prova alla volta.

#pragma once

#include "CoreMinimal.h"

namespace ValdorsoSicurezza
{
	/** Giri di PBKDF2-SHA256 per le password nuove (indicazione OWASP 2023). Si possono alzare: le impronte vecchie si rifanno da sole al login. */
	inline constexpr int32 IterazioniCorrenti = 600000;

	/** Byte di sale casuale per ogni password. */
	inline constexpr int32 LunghezzaSale = 16;

	/** Byte dell'impronta. */
	inline constexpr int32 LunghezzaImpronta = 32;

	/** Nome dell'algoritmo, scritto in ogni account. */
	inline const TCHAR* const Algoritmo = TEXT("pbkdf2-sha256");

	/** Riempie Out con Quanti byte casuali sicuri. Falso se il generatore non risponde. */
	VALDORSO_API bool BytesCasuali(TArray<uint8>& Out, int32 Quanti);

	/** Calcola l'impronta di una password. Lento di proposito (circa 0,3 s): si chiama solo fuori dal filo principale. */
	VALDORSO_API bool CalcolaImpronta(const FString& Password, const TArray<uint8>& Sale, int32 Iterazioni, TArray<uint8>& Out);

	/** Confronta due impronte nello stesso tempo, che siano uguali o no (così dai tempi non si indovina niente). */
	VALDORSO_API bool UgualiTempoCostante(const TArray<uint8>& A, const TArray<uint8>& B);

	/** SHA-256 di un testo, in esadecimale. Serve per gli inviti: il server ne tiene solo l'impronta. */
	VALDORSO_API FString Sha256Esadecimale(const FString& Testo);

	/** Un testo casuale con i caratteri dell'alfabeto dato, senza preferenze tra un carattere e l'altro. Vuoto se il generatore non risponde. */
	VALDORSO_API FString TestoCasuale(const TCHAR* Alfabeto, int32 Quanti);

	// --- La chiave del server (passo 2.3) ----------------------------------------------------------
	// Il server ha una coppia di chiavi RSA da 3072 bit. La privata resta sul server (Saved/Server/Chiavi);
	// la pubblica viaggia con il gioco. Il client inventa una chiave di sessione, la chiude con la pubblica
	// (RSA-OAEP con SHA-256) e la manda: solo il server la può aprire.

	/** Crea una coppia di chiavi nuova, in formato PEM. */
	VALDORSO_API bool CreaCoppiaChiavi(FString& OutPrivataPem, FString& OutPubblicaPem);

	/** Chiude Dati con la chiave pubblica del server. */
	VALDORSO_API bool CifraConPubblica(const FString& PubblicaPem, const TArray<uint8>& Dati, TArray<uint8>& Out);

	/** Apre con la chiave privata del server quello che il client ha chiuso. */
	VALDORSO_API bool DecifraConPrivata(const FString& PrivataPem, const TArray<uint8>& Cifrati, TArray<uint8>& Out);

	/** Base64 senza + / = (si può mettere in un indirizzo). */
	VALDORSO_API FString Base64PerIndirizzo(const TArray<uint8>& Dati);
	VALDORSO_API bool DaBase64PerIndirizzo(const FString& Testo, TArray<uint8>& Out);
}
