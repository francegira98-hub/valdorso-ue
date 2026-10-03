// Valdorso - Strumenti di sicurezza dell'archivista (vedi il .h).

#include "ValdorsoSicurezza.h"
#include "Valdorso.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#endif
THIRD_PARTY_INCLUDES_START
#define UI UI_ST
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#undef UI
THIRD_PARTY_INCLUDES_END
#if PLATFORM_WINDOWS
#include "Windows/HideWindowsPlatformTypes.h"
#endif

namespace ValdorsoSicurezza
{
	bool BytesCasuali(TArray<uint8>& Out, int32 Quanti)
	{
		Out.SetNumZeroed(FMath::Max(Quanti, 0));
		if (Quanti <= 0)
		{
			return true;
		}
		if (RAND_bytes(Out.GetData(), Quanti) != 1)
		{
			UE_LOG(LogValdorso, Error, TEXT("[Valdorso] Sicurezza: il generatore di numeri casuali di OpenSSL non risponde."));
			return false;
		}
		return true;
	}

	bool CalcolaImpronta(const FString& Password, const TArray<uint8>& Sale, int32 Iterazioni, TArray<uint8>& Out)
	{
		Out.SetNumZeroed(LunghezzaImpronta);
		if (Iterazioni < 1 || Sale.Num() == 0)
		{
			return false;
		}

		FTCHARToUTF8 Utf8(*Password);
		const char* Testo = reinterpret_cast<const char*>(Utf8.Get());
		const int Risultato = PKCS5_PBKDF2_HMAC(Testo, Utf8.Length(), Sale.GetData(), Sale.Num(), Iterazioni,
			EVP_sha256(), LunghezzaImpronta, Out.GetData());

		// La copia in UTF-8 della password non resta in memoria.
		FMemory::Memzero(const_cast<char*>(Testo), Utf8.Length());

		if (Risultato != 1)
		{
			UE_LOG(LogValdorso, Error, TEXT("[Valdorso] Sicurezza: calcolo dell'impronta non riuscito."));
			return false;
		}
		return true;
	}

	bool UgualiTempoCostante(const TArray<uint8>& A, const TArray<uint8>& B)
	{
		if (A.Num() != B.Num() || A.Num() == 0)
		{
			return false;
		}
		return CRYPTO_memcmp(A.GetData(), B.GetData(), A.Num()) == 0;
	}

	FString Sha256Esadecimale(const FString& Testo)
	{
		FTCHARToUTF8 Utf8(*Testo);
		unsigned char Risultato[EVP_MAX_MD_SIZE];
		unsigned int Lunghezza = 0;
		if (EVP_Digest(Utf8.Get(), Utf8.Length(), Risultato, &Lunghezza, EVP_sha256(), nullptr) != 1)
		{
			UE_LOG(LogValdorso, Error, TEXT("[Valdorso] Sicurezza: SHA-256 non riuscito."));
			return FString();
		}
		return BytesToHex(Risultato, static_cast<int32>(Lunghezza));
	}

	FString TestoCasuale(const TCHAR* Alfabeto, int32 Quanti)
	{
		const int32 N = FCString::Strlen(Alfabeto);
		if (N < 2 || N > 256 || Quanti <= 0)
		{
			return FString();
		}

		// Si scartano i byte oltre l'ultimo multiplo intero di N: così ogni carattere ha la stessa probabilità.
		const int32 Limite = 256 - (256 % N);
		FString Risultato;
		Risultato.Reserve(Quanti);
		TArray<uint8> Scorta;
		while (Risultato.Len() < Quanti)
		{
			if (!BytesCasuali(Scorta, 64))
			{
				return FString();
			}
			for (const uint8 Byte : Scorta)
			{
				if (Byte < Limite)
				{
					Risultato.AppendChar(Alfabeto[Byte % N]);
					if (Risultato.Len() == Quanti)
					{
						break;
					}
				}
			}
		}
		return Risultato;
	}
}
