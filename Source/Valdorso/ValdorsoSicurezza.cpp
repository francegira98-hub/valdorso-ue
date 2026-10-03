// Valdorso - Strumenti di sicurezza dell'archivista (vedi il .h).

#include "ValdorsoSicurezza.h"
#include "Valdorso.h"
#include "Misc/Base64.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#endif
THIRD_PARTY_INCLUDES_START
#define UI UI_ST
#include <openssl/bio.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
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

	// --- Chiavi RSA --------------------------------------------------------------------------------

	namespace
	{
		constexpr int32 BitChiaveServer = 3072;

		/** Il testo di un BIO di memoria. */
		FString TestoDaBio(BIO* Bio)
		{
			char* Dati = nullptr;
			const long Lunghezza = BIO_get_mem_data(Bio, &Dati);
			if (Lunghezza <= 0 || Dati == nullptr)
			{
				return FString();
			}
			const FUTF8ToTCHAR Conversione(reinterpret_cast<const UTF8CHAR*>(Dati), static_cast<int32>(Lunghezza));
			return FString(Conversione.Length(), Conversione.Get());
		}

		EVP_PKEY* LeggiChiave(const FString& Pem, bool bPrivata)
		{
			FTCHARToUTF8 Utf8(*Pem);
			BIO* Bio = BIO_new_mem_buf(Utf8.Get(), Utf8.Length());
			if (!Bio)
			{
				return nullptr;
			}
			EVP_PKEY* Chiave = bPrivata ? PEM_read_bio_PrivateKey(Bio, nullptr, nullptr, nullptr) : PEM_read_bio_PUBKEY(Bio, nullptr, nullptr, nullptr);
			BIO_free(Bio);
			return Chiave;
		}

		/** Prepara RSA-OAEP con SHA-256 su un contesto già inizializzato. */
		bool ImpostaOaep(EVP_PKEY_CTX* Contesto)
		{
			return EVP_PKEY_CTX_set_rsa_padding(Contesto, RSA_PKCS1_OAEP_PADDING) > 0
				&& EVP_PKEY_CTX_set_rsa_oaep_md(Contesto, EVP_sha256()) > 0
				&& EVP_PKEY_CTX_set_rsa_mgf1_md(Contesto, EVP_sha256()) > 0;
		}
	}

	bool CreaCoppiaChiavi(FString& OutPrivataPem, FString& OutPubblicaPem)
	{
		bool bOk = false;
		EVP_PKEY* Chiave = nullptr;
		EVP_PKEY_CTX* Contesto = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
		if (Contesto && EVP_PKEY_keygen_init(Contesto) > 0 && EVP_PKEY_CTX_set_rsa_keygen_bits(Contesto, BitChiaveServer) > 0
			&& EVP_PKEY_keygen(Contesto, &Chiave) > 0)
		{
			BIO* Privata = BIO_new(BIO_s_mem());
			BIO* Pubblica = BIO_new(BIO_s_mem());
			if (Privata && Pubblica
				&& PEM_write_bio_PrivateKey(Privata, Chiave, nullptr, nullptr, 0, nullptr, nullptr) > 0
				&& PEM_write_bio_PUBKEY(Pubblica, Chiave) > 0)
			{
				OutPrivataPem = TestoDaBio(Privata);
				OutPubblicaPem = TestoDaBio(Pubblica);
				bOk = !OutPrivataPem.IsEmpty() && !OutPubblicaPem.IsEmpty();
			}
			BIO_free(Privata);
			BIO_free(Pubblica);
		}
		EVP_PKEY_free(Chiave);
		EVP_PKEY_CTX_free(Contesto);
		if (!bOk)
		{
			UE_LOG(LogValdorso, Error, TEXT("[Valdorso] Sicurezza: non riesco a creare la coppia di chiavi del server."));
		}
		return bOk;
	}

	bool CifraConPubblica(const FString& PubblicaPem, const TArray<uint8>& Dati, TArray<uint8>& Out)
	{
		Out.Reset();
		EVP_PKEY* Chiave = LeggiChiave(PubblicaPem, false);
		if (!Chiave)
		{
			UE_LOG(LogValdorso, Error, TEXT("[Valdorso] Sicurezza: la chiave pubblica del server non è valida."));
			return false;
		}
		bool bOk = false;
		EVP_PKEY_CTX* Contesto = EVP_PKEY_CTX_new(Chiave, nullptr);
		size_t Lunghezza = 0;
		if (Contesto && EVP_PKEY_encrypt_init(Contesto) > 0 && ImpostaOaep(Contesto)
			&& EVP_PKEY_encrypt(Contesto, nullptr, &Lunghezza, Dati.GetData(), Dati.Num()) > 0)
		{
			Out.SetNumZeroed(static_cast<int32>(Lunghezza));
			if (EVP_PKEY_encrypt(Contesto, Out.GetData(), &Lunghezza, Dati.GetData(), Dati.Num()) > 0)
			{
				Out.SetNum(static_cast<int32>(Lunghezza));
				bOk = true;
			}
		}
		EVP_PKEY_CTX_free(Contesto);
		EVP_PKEY_free(Chiave);
		return bOk;
	}

	bool DecifraConPrivata(const FString& PrivataPem, const TArray<uint8>& Cifrati, TArray<uint8>& Out)
	{
		Out.Reset();
		EVP_PKEY* Chiave = LeggiChiave(PrivataPem, true);
		if (!Chiave)
		{
			UE_LOG(LogValdorso, Error, TEXT("[Valdorso] Sicurezza: la chiave privata del server non è valida."));
			return false;
		}
		bool bOk = false;
		EVP_PKEY_CTX* Contesto = EVP_PKEY_CTX_new(Chiave, nullptr);
		size_t Lunghezza = 0;
		if (Contesto && EVP_PKEY_decrypt_init(Contesto) > 0 && ImpostaOaep(Contesto)
			&& EVP_PKEY_decrypt(Contesto, nullptr, &Lunghezza, Cifrati.GetData(), Cifrati.Num()) > 0)
		{
			Out.SetNumZeroed(static_cast<int32>(Lunghezza));
			if (EVP_PKEY_decrypt(Contesto, Out.GetData(), &Lunghezza, Cifrati.GetData(), Cifrati.Num()) > 0)
			{
				Out.SetNum(static_cast<int32>(Lunghezza));
				bOk = true;
			}
		}
		EVP_PKEY_CTX_free(Contesto);
		EVP_PKEY_free(Chiave);
		return bOk;
	}

	FString Base64PerIndirizzo(const TArray<uint8>& Dati)
	{
		FString Testo = FBase64::Encode(Dati);
		Testo.ReplaceCharInline(TEXT('+'), TEXT('-'));
		Testo.ReplaceCharInline(TEXT('/'), TEXT('_'));
		while (Testo.EndsWith(TEXT("=")))
		{
			Testo.LeftChopInline(1);
		}
		return Testo;
	}

	bool DaBase64PerIndirizzo(const FString& Testo, TArray<uint8>& Out)
	{
		FString Normale = Testo;
		Normale.ReplaceCharInline(TEXT('-'), TEXT('+'));
		Normale.ReplaceCharInline(TEXT('_'), TEXT('/'));
		while (Normale.Len() % 4 != 0)
		{
			Normale.AppendChar(TEXT('='));
		}
		return FBase64::Decode(Normale, Out);
	}
}
