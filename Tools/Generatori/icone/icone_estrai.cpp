// Valdorso - Le icone degli oggetti, passo 1 (13/11/2026, scritto da Claude).
// Compila il codice VERO dei disegni degli oggetti (la parte anonima di Source/Valdorso/ValdorsoOggetti.cpp: il catalogo e
// le linee a penna) fuori da Unreal, con icone_shim.h al posto dei tipi di Unreal, e scrive in JSON ogni oggetto con le
// sue linee. Così le icone ad acquerello (icone.py) partono dagli stessi disegni del gioco, senza ricopiarli a mano.
// Lo lancia icone.py da solo (serve g++).
#include "icone_shim.h"
#include <cstdio>
#include "oggetti_estratti.inc"

static void Esc(const std::string& S)
{
	for (char C : S)
	{
		if (C == '"' || C == '\\') putchar('\\');
		putchar(C);
	}
}

static void Linee(const FLineeOggetto& L)
{
	for (size_t a = 0; a < L.size(); ++a)
	{
		printf("%s[", a ? "," : "");
		for (size_t b = 0; b < L[a].size(); ++b) printf("%s[%.4f,%.4f]", b ? "," : "", L[a][b].X, L[a][b].Y);
		printf("]");
	}
}

int main()
{
	auto Cat = OggCreaCatalogo();
	printf("[\n");
	for (size_t i = 0; i < Cat.size(); ++i)
	{
		auto& T = Cat[i];
		printf("{\"id\":\""); Esc(T.Id.S); printf("\",\"nome\":\""); Esc(T.Nome);
		printf("\",\"uso\":%d,\"posto\":%d,\"ricordo\":%d,\"linee\":[", (int)T.Uso, (int)T.Posto, (int)T.bRicordo);
		Linee(OggDisegnoDi(T.Id));
		printf("]},\n");
	}
	printf("{\"id\":\"Moneta\",\"nome\":\"Aureus\",\"uso\":0,\"posto\":0,\"ricordo\":0,\"linee\":[");
	Linee(OggCreaDisegnoMoneta());
	printf("]}\n]\n");
}
