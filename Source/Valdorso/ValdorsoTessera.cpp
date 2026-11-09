// Valdorso - La tessera del colono (vedi il .h). Scritto da Claude il 09/11/2026.

#include "ValdorsoTessera.h"
#include "ValdorsoRegistro.h"

namespace ValdorsoTessera
{
	FString Numero(const FString& IdPersonaggio)
	{
		// quattro segni facili da leggere (niente 0/O, 1/I), presi dall'impronta dell'Id: sempre uguali
		static const TCHAR Segni[] = TEXT("23456789ABCDEFGHJKLMNPQRSTUVWXYZ");
		constexpr int32 QuantiSegni = UE_ARRAY_COUNT(Segni) - 1;
		uint32 Impronta = FCrc::StrCrc32(*IdPersonaggio.ToUpper());
		FString Coda;
		for (int32 i = 0; i < 4; ++i)
		{
			Coda.AppendChar(Segni[Impronta % QuantiSegni]);
			Impronta /= QuantiSegni;
		}
		return FString::Printf(TEXT("VO-312-%s"), *Coda);
	}

	FValdorsoTessera Nuova(const FString& IdPersonaggio)
	{
		FValdorsoTessera Tessera;
		Tessera.Numero = Numero(IdPersonaggio);
		Tessera.Qualifica = TEXT("Colono della valle dell'Orso");
		Tessera.Rilascio = TEXT("Al passo, con la carovana di Pietralta");
		Tessera.Timbri.Add(TEXT("Visto al passo: L. Serra, capitana"));
		return Tessera;
	}

	void Sistema(FValdorsoTessera& Tessera, const FString& IdPersonaggio)
	{
		if (Tessera.Numero.IsEmpty())
		{
			Tessera = Nuova(IdPersonaggio);
			return;
		}
		Tessera.Numero = Tessera.Numero.Left(24);
		Tessera.Qualifica = Tessera.Qualifica.Left(MassimaLunghezza);
		Tessera.Rilascio = Tessera.Rilascio.Left(MassimaLunghezza);
		if (Tessera.Qualifica.IsEmpty())
		{
			Tessera.Qualifica = TEXT("Colono della valle dell'Orso");
		}
		if (Tessera.Timbri.Num() > MassimoTimbri)
		{
			Tessera.Timbri.SetNum(MassimoTimbri);
		}
		if (Tessera.Note.Num() > MassimoNote)
		{
			Tessera.Note.SetNum(MassimoNote);
		}
		for (FString& Timbro : Tessera.Timbri)
		{
			Timbro = Timbro.Left(MassimaLunghezza);
		}
		for (FString& Nota : Tessera.Note)
		{
			Nota = Nota.Left(MassimaLunghezza);
		}
	}

	FValdorsoTesseraVista Vista(const FString& Nome, const FValdorsoRegistro* Registro, const FValdorsoTessera& Tessera)
	{
		FValdorsoTesseraVista Fuori;
		Fuori.Nome = Nome;
		Fuori.Tessera = Tessera;
		if (!Registro)
		{
			return Fuori;
		}
		Fuori.Sesso = Registro->Sesso;
		Fuori.Eta = Registro->Eta;
		Fuori.Origine = ValdorsoRegistro::Testo(ValdorsoRegistro::EDomanda::Origine, Registro->Origine, Registro->Sesso);
		Fuori.Mestiere = ValdorsoRegistro::Testo(ValdorsoRegistro::EDomanda::Mestiere, Registro->Mestiere, Registro->Sesso);
		// la fede, corta: "Nereia, l'Acqua: il mare e i viaggi" diventa "Nereia"
		FString Fede = ValdorsoRegistro::Testo(ValdorsoRegistro::EDomanda::Fede, Registro->Fede, Registro->Sesso);
		int32 Virgola = INDEX_NONE;
		if (Fede.FindChar(TEXT(','), Virgola))
		{
			Fede = Fede.Left(Virgola);
		}
		Fuori.Fede = Fede;
		Fuori.Firma = Registro->Firma;
		return Fuori;
	}
}
