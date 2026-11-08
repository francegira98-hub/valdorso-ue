// Valdorso - I testi dei dialoghi (v0.1.3): Gaspare al margine del bosco e le due guardie della Corona al passo;
// (08/11) Nonna Edda nella palude, Bruno alla locanda e Padre Anselmo nel Tempio del Cuore.
// Scritto da Claude il 07/11/2026. Si include solo in ValdorsoDialoghi.cpp.
//
// Come si scrive un dialogo (per chi lo ritocca):
//   D.Nodo(TEXT("id"), { TEXT("battuta"), TEXT("(una riga tra parentesi è un gesto: si scrive in corsivo)") })
//       .Scelta(TEXT("cosa risponde il colono"), TEXT("id del nodo dopo"))
//       .Scelta(TEXT("me ne vado"), NAME_None);                    <- chiude il dialogo
//   D.Nodo(TEXT("fine"), { ... }).Fine();                          <- ultima battuta, senza risposte
//   Una risposta con un evento: .Scelta(TEXT("..."), TEXT("nodo"), TEXT("Corruzione")) (finisce nel registro eventi).
//   (08/11) I segni: .SceltaSegna(TEXT("segno")) dopo una risposta (chi la sceglie riceve il segno), .Segno(TEXT("segno"))
//   su un nodo (chi ci arriva lo riceve), .SceltaNascostaSe(TEXT("segno")) dopo una risposta (non si mostra più a chi ha
//   il segno). Per personaggio e per PNG, come "chi ha già parlato" ((08/11) nel file del personaggio). Così la moneta
//   si offre una volta.
//   Al massimo 4 risposte per nodo. Una riga "rune: Ur | didascalia" si mostra con le rune disegnate.
// I test (Valdorso.Dialoghi.*) controllano che ogni nodo esista, ci si arrivi e porti a una fine.
//
// Gaspare (dal Design): burbero, solitario, lo prendono per matto. Ha visto l'ombra dell'Orso più di chiunque altro.
// Il suo segreto (trent'anni fa l'Orso gli salvò la vita, e da allora caccia solo ciò che serve) NON si dice: si lascia
// solo intuire. Ama chi rispetta il bosco, detesta chi caccia per divertimento. Lui e Nonna Edda sono gli unici che
// credono all'Orso. Prepara la missione "Dove l'ho vista" e le lezioni sulle tracce ("torna quando avrai imparato a
// stare zitto nel bosco").
// Le guardie (Tebaldo, il veterano, e Fulvio, il giovane): poche parole, leali alla Corona, non si comprano.
// (08/11) Prima si chiamavano Corrado e Ottone, come il Balivo (Corrado Valli) e il Gran Inquisitore (Ottone Vesper)
// delle Cronache: cambiati i nomi. Gli Id restano "GuardiaCorrado" e "GuardiaOttone" (nel livello e nei taccuini
// salvati), così chi ci ha già parlato se lo ricorda.
//
// (08/11) Tre PNG nuovi, scritti da Claude l'08/11/2026:
// - Nonna Edda, la vecchia della palude (capanna su pali a sud del lago): guaritrice, un po' strega. Lei e Gaspare sono
//   gli unici che credono all'Orso. Maestro Elric la disprezza ma ha bisogno di lei. Insegna di nascosto a una ragazza
//   del villaggio (Iole: NON si dice il nome, si intuisce soltanto). Sa qualche parola di rune (Nir-Ren, la Guarigione
//   lieve; Nir-Ren-Ka, Acqua che veglia, come ninnananna). "Chi va da Nonna Edda porti il dovuto": chiede cosa le hai
//   portato (niente inventario ancora: "Niente" o "Solo rispetto", ricordati con un segno per le missioni future). Sul
//   fondo del lago non si fanno domande: chi la fa viene congedato, e la domanda non si ripresenta.
// - Bruno il locandiere (Locanda di Bruno, a ovest della piazza): vecchio sergente, compagno d'armi della capitana
//   Livia Serra; detesta il Balivo (Corrado Valli) e ne parla con prudenza; sa tutte le voci (ne fa girare quattro: il
//   frammento che ha saltato un battito, il sigillo che trema, Gaspare e l'ombra, le luci alle rovine); accenna a un
//   passaggio sotto la torre che conosce Vera (solo un accenno). Offre letto e scodella: niente monete ancora, "la prima
//   la offre la casa" (una volta sola: il segno PrimaScodella).
// - Padre Anselmo (Tempio del Cuore, accanto all'altare con la pietra di Val d'Orso): custode del frammento del Cuore;
//   è il sacerdote che ha scritto il colono nel Registro. Protegge Tobia, l'orfano; diffida di Nico senza sapere
//   perché. Il Cuore e il battito (due colpi e una pausa), la pietra e il territorio, le fedi (i Cinque e i Vecchi Dei),
//   la cerimonia dell'anno e un giorno e la veglia del frammento (accennate: arriveranno più avanti).

namespace
{
	FValdorsoDialogo DialoghiGaspare()
	{
		FValdorsoDialogo D;
		D.Id = TEXT("Gaspare");
		D.NomeBreve = TEXT("Gaspare");
		D.Titolo = TEXT("Gaspare, il vecchio cacciatore");
		D.Inizio = TEXT("primo");
		D.Ritorno = TEXT("ritorno");

		D.Nodo(TEXT("primo"), {
			TEXT("Fermo lì. No, il piede non sul muschio: sotto la felce c'è un nido di allodola. Ecco. Così."),
			TEXT("(Il vecchio non si alza dal ceppo. Ha una balestra sulle ginocchia e le mani più ferme della balestra.)"),
			TEXT("Un colono nuovo. Si vede dalle scarpe: troppo pulite per la valle e troppo rumorose per il bosco."),
			})
			.Scelta(TEXT("Chi sei?"), TEXT("chi"))
			.Scelta(TEXT("Cosa fai qui, al margine del bosco?"), TEXT("bosco"))
			.Scelta(TEXT("Al villaggio dicono che tu veda l'ombra di un orso."), TEXT("orso"))
			.Scelta(TEXT("Scusa il disturbo. Me ne vado."), TEXT("congedo"));

		D.Nodo(TEXT("ritorno"), {
			TEXT("(Gaspare non si volta.)"),
			TEXT("Ancora tu. Stavolta ti ho sentito arrivare da cento passi. Un cinghiale ferito fa meno rumore."),
			TEXT("Be'? Parla piano, che il bosco ascolta."),
			})
			.Scelta(TEXT("Raccontami ancora di te."), TEXT("chi"))
			.Scelta(TEXT("Parlami del bosco."), TEXT("bosco"))
			.Scelta(TEXT("L'hai rivista, l'ombra?"), TEXT("orso"))
			.Scelta(TEXT("Passavo soltanto. Ti lascio ai tuoi alberi."), TEXT("congedo"));

		D.Nodo(TEXT("ancora"), {
			TEXT("(Gaspare sputa un filo d'erba.)"),
			TEXT("Altro? Fai in fretta. Tra un'ora passano i caprioli, e con te qui non passano."),
			})
			.Scelta(TEXT("Parlami del bosco e della caccia."), TEXT("bosco"))
			.Scelta(TEXT("L'ombra dell'Orso..."), TEXT("orso"))
			.Scelta(TEXT("Le rovine degli Antichi: le conosci?"), TEXT("rovine"))
			.Scelta(TEXT("Basta così. Ti lascio ai tuoi alberi."), TEXT("congedo"));

		// --- Chi è ------------------------------------------------------------------------------------------------
		D.Nodo(TEXT("chi"), {
			TEXT("Gaspare. Il cognome l'ho lasciato giù a sud, con tutto il resto. Caccio in questa valle da quando il Balivo non era nato, e quello prima di lui portava ancora le brache corte."),
			TEXT("Quest'anno, il trecentododici, la Corona ha mandato su più coloni che pecore. Bene per il mercato. Male per il silenzio."),
			TEXT("Al villaggio dicono che sono matto. Ai bambini fanno paura con me: \"se non dormi viene Gaspare con la balestra\". Mi sta bene. La gente che ha paura non viene a calpestarmi i lacci."),
			})
			.Scelta(TEXT("Perché vivi qui fuori, da solo?"), TEXT("solo"))
			.Scelta(TEXT("Sei matto davvero?"), TEXT("matto"))
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"))
			.Scelta(TEXT("Ti lascio in pace."), TEXT("congedo"));

		D.Nodo(TEXT("solo"), {
			TEXT("Perché gli alberi non chiedono niente e non raccontano niente in giro. Al villaggio, se starnutisci all'alba, a mezzogiorno lo sa anche il prete del Tempio del Cuore."),
			TEXT("E perché qualcuno deve stare sul margine. Tra le case e il bosco c'è una riga, anche se non si vede. Io ci sto sopra."),
			})
			.Scelta(TEXT("Una riga? Che riga?"), TEXT("riga"))
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"));

		D.Nodo(TEXT("riga"), {
			TEXT("Quella che non si passa senza chiedere permesso. Il bosco è di chi c'era prima di noi. Noi siamo ospiti, e gli ospiti educati campano di più."),
			TEXT("Il resto lo capirai da solo, o non lo capirai. Tutt'e due vanno bene, per il bosco."),
			})
			.Scelta(TEXT("Chi c'era prima di noi?"), TEXT("orso"))
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"));

		D.Nodo(TEXT("matto"), {
			TEXT("(Gaspare ride: una specie di tosse.)"),
			TEXT("Matto è chi entra nel bosco di notte senza sapere cosa cerca. Io lo so sempre, cosa cerco."),
			TEXT("Ma se al villaggio è più comodo dire matto, che lo dicano. Così non mi chiedono di fare da guida ai figli dei mercanti. E la milizia mi lascia in pace: a tirare con l'arco, a metà di loro, gliel'ho insegnato io."),
			})
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"))
			.Scelta(TEXT("Ti lascio in pace."), TEXT("congedo"));

		// --- Il bosco e la caccia ---------------------------------------------------------------------------------
		D.Nodo(TEXT("bosco"), {
			TEXT("Il bosco grande comincia qui e finisce dove finiscono le gambe. A ovest le rovine, a nord la collina. In mezzo cervi, cinghiali, lupi. E cose che non hanno un nome, perché nessuno è tornato a dargliene uno."),
			TEXT("Ci sono delle regole. Poche, ma ci sono."),
			})
			.Scelta(TEXT("Quali regole?"), TEXT("regole"))
			.Scelta(TEXT("Io caccio per divertimento. Dove trovo le prede più belle?"), TEXT("divertimento"))
			.Scelta(TEXT("Mi insegneresti a cacciare? A leggere le tracce?"), TEXT("insegna"))
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"));

		D.Nodo(TEXT("regole"), {
			TEXT("Uno: prendi solo quello che mangi, o quello che ti scalda. Due: la femmina con i piccoli non si tocca, mai. Tre: se su un albero trovi un segno che non hai fatto tu, torni indietro e non fai domande."),
			TEXT("Chi le rispetta, il bosco lo lascia passare. Chi no... be'. Il bosco ha più denti di me."),
			})
			.Scelta(TEXT("Che segni sugli alberi?"), TEXT("segni"))
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"));

		D.Nodo(TEXT("segni"), {
			TEXT("Graffi. Alti. Più alti di dove arrivi tu con il braccio teso, e io con la lancia."),
			TEXT("(Gaspare guarda gli alberi un momento di troppo.)"),
			TEXT("Te l'ho detto: si torna indietro e non si fanno domande. Vale anche per me."),
			})
			.Scelta(TEXT("Graffi d'orso?"), TEXT("orso"))
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"));

		D.Nodo(TEXT("divertimento"), {
			TEXT("(Gaspare smette di sorridere. Non che sorridesse, prima.)"),
			TEXT("Per divertimento. Senti, colono. Il cervo che ammazzi per divertimento non lo mangi. La pelle la butti, le corna le appendi sopra il camino perché le vedano gli amici. E il bosco se lo ricorda."),
			TEXT("Se trovo i tuoi lacci sul mio sentiero, ci appendo te. Per i piedi."),
			})
			.Scelta(TEXT("Ho detto una sciocchezza. Caccio per mangiare."), TEXT("scusa"))
			.Scelta(TEXT("(Te ne vai senza rispondere.)"), NAME_None);

		D.Nodo(TEXT("scusa"), {
			TEXT("(Ti squadra a lungo, dalle scarpe ai capelli.)"),
			TEXT("Mh. Allora impara a dirle meglio, le cose. Nel bosco le sciocchezze si pagano, e non con le monete."),
			})
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"))
			.Scelta(TEXT("Me ne ricorderò."), TEXT("congedo"));

		D.Nodo(TEXT("insegna"), {
			TEXT("Insegnarti? Tu fai più rumore di un carro di pentole."),
			TEXT("Prima impara a stare zitto nel bosco. Zitto davvero: con i piedi, con il fiato, con la testa. Le tracce parlano piano, e chi fa chiasso non le sente."),
			TEXT("Torna quando avrai imparato a stare zitto nel bosco. Allora, forse, ti faccio vedere come si legge un sentiero. Forse."),
			})
			.Scelta(TEXT("Tornerò. E farò meno rumore."), TEXT("ancora"))
			.Scelta(TEXT("Ci conto. A presto, Gaspare."), TEXT("congedo"));

		// --- L'Orso -----------------------------------------------------------------------------------------------
		D.Nodo(TEXT("orso"), {
			TEXT("(Gaspare non risponde subito. Si gira verso il bosco, come se qualcuno l'avesse chiamato per nome.)"),
			TEXT("Al villaggio ne parlano come di una favola per far stare buoni i bambini. Padre Anselmo dice che l'Orso è caduto sulla collina, tanto tempo fa, e che adesso dorme nella terra e nei canti. Va bene. Lo dice il prete."),
			TEXT("Io dico solo quello che ho visto. Nelle notti di nebbia, tra gli abeti, un'ombra più grande di una casa. Non fa rumore. Non lascia impronte. Ma gli uccelli smettono di cantare, quando passa."),
			})
			.Scelta(TEXT("L'hai vista davvero?"), TEXT("vista"))
			.Scelta(TEXT("Ti crede qualcuno?"), TEXT("credono"))
			.Scelta(TEXT("Hai paura dell'Orso?"), TEXT("paura"))
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"));

		D.Nodo(TEXT("vista"), {
			TEXT("Più di chiunque altro in questa valle. Più volte di quante ne racconto, e ne racconto poche."),
			TEXT("Una volta vicina come sei tu adesso. No, non ti dico dove. Né quando."),
			TEXT("Ci sono posti dove l'ombra torna, come l'acqua torna sempre nello stesso letto. Di notte."),
			})
			.Scelta(TEXT("Portami a vederla."), TEXT("portami"))
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"));

		D.Nodo(TEXT("portami"), {
			TEXT("Ah! Sentilo. \"Portami a vederla\", come si chiede di andare alla fiera."),
			TEXT("Non stanotte. Non con quelle scarpe. Torna quando avrai imparato a stare zitto nel bosco, e quando non avrai più tanta fretta di vedere le cose. Allora ne riparliamo."),
			})
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"))
			.Scelta(TEXT("Allora tornerò."), TEXT("congedo"));

		D.Nodo(TEXT("credono"), {
			TEXT("Edda. Nonna Edda, giù nella palude, sul lago. Lei ci crede, e sa cose che io non so. Siamo in due, in tutta la valle. Due matti, dicono."),
			TEXT("Se vai da lei, salutala. Chi non saluta torna a mani vuote. E non farle domande sul fondo del lago."),
			})
			.Scelta(TEXT("Perché proprio voi due?"), TEXT("duematti"))
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"));

		D.Nodo(TEXT("duematti"), {
			TEXT("Perché tutti e due abbiamo vissuto abbastanza lontano dalle campane da sentire il resto."),
			TEXT("(Tace. Poi aggiunge, più piano:)"),
			TEXT("E perché a certe cose si crede quando ti hanno guardato in faccia. Basta così."),
			})
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"))
			.Scelta(TEXT("Non ti chiedo altro."), TEXT("congedo"));

		D.Nodo(TEXT("paura"), {
			TEXT("Paura?"),
			TEXT("(Senza accorgersene, Gaspare si passa il pollice su una vecchia cicatrice, sotto l'orecchio.)"),
			TEXT("No. Rispetto. È diverso. Chi ha paura scappa, e chi scappa nel bosco cade. Chi rispetta sta fermo, e guarda, e aspetta. E a volte quello che guardi decide di lasciarti vivere."),
			TEXT("Da allora prendo solo quello che serve. Non chiedermi da quando."),
			})
			.Scelta(TEXT("Da quando?"), TEXT("quando"))
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"));

		D.Nodo(TEXT("quando"), {
			TEXT("Ti ho detto di non chiedermelo."),
			TEXT("(Per un po' si sente solo il vento negli abeti. Poi Gaspare prende un rametto e traccia un segno nella terra, tra i suoi stivali.)"),
			TEXT("rune: Ur | Il segno che Gaspare ha tracciato nella terra"),
			TEXT("Questo gli Antichi lo incidevano sopra le porte che non si aprono. Al villaggio non lo sanno leggere. Io nemmeno, del tutto. Ma so cosa vuol dire."),
			TEXT("(Lo cancella con la suola dello stivale.)"),
			})
			.Scelta(TEXT("Cosa vuol dire?"), TEXT("ur"))
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"));

		D.Nodo(TEXT("ur"), {
			TEXT("Ur. Chi sta davanti alla porta. Chi tiene, quando tutti gli altri scappano. Custode, dicono i vecchi. E orso: per loro era la stessa parola."),
			TEXT("Il resto, se lo vuoi, va cercato alle rovine. Io ti ho già detto troppo, e il bosco ascolta."),
			})
			.Scelta(TEXT("Parlami delle rovine."), TEXT("rovine"))
			.Scelta(TEXT("Grazie, Gaspare."), TEXT("congedo"));

		// --- Le rovine e le rune ----------------------------------------------------------------------------------
		D.Nodo(TEXT("rovine"), {
			TEXT("Le rovine degli Antichi stanno nel cuore del bosco, a ovest. Pietre in cerchio, una stella incisa nel mezzo, e cinque segni che nessuno sa più leggere del tutto."),
			TEXT("rune: Aeth Nir Dorn Sil Lume | Il fuoco, l'acqua, la terra, il vento, la luce"),
			TEXT("Così dicono i pochi che le studiano. Di notte si accendono da sole. Sotto c'è una scala, chiusa da una lastra; e la lastra non si apre."),
			})
			.Scelta(TEXT("Tu sai leggere le rune?"), TEXT("leggere"))
			.Scelta(TEXT("Cosa c'è sotto la lastra?"), TEXT("lastra"))
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"))
			.Scelta(TEXT("Andrò a vederle."), TEXT("congedo"));

		D.Nodo(TEXT("leggere"), {
			TEXT("Io leggo le tracce. Anche le rune sono tracce: di qualcuno che è passato e voleva farsi ricordare."),
			TEXT("Qualcosa so. Le rune con la base sono cose: fuoco, acqua, terra, vento, luce. Quelle senza base sono gesti: andare, proteggere, guarire, vedere, fermare."),
			TEXT("rune: Var Ka Ren Vel Tor | Var, Ka, Ren, Vel, Tor: i gesti"),
			TEXT("Se ne vuoi sapere di più, scrivile sul tuo quaderno e guarda le pietre con calma. Le pietre hanno pazienza. Più di me."),
			})
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"))
			.Scelta(TEXT("Grazie, Gaspare."), TEXT("congedo"));

		D.Nodo(TEXT("lastra"), {
			TEXT("Niente di buono. O niente di nostro, che è lo stesso."),
			TEXT("C'è chi ci ha provato con la leva, e la leva si è spezzata. Le rune sulla lastra battono di notte, due colpi e una pausa, come la pietra del tempio. A una cosa che batte così io non metto le mani addosso."),
			})
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"))
			.Scelta(TEXT("Starò attento."), TEXT("congedo"));

		// --- Il congedo -------------------------------------------------------------------------------------------
		D.Nodo(TEXT("congedo"), {
			TEXT("Va'. E cammina sui sassi, non sulle foglie."),
			TEXT("(Gaspare torna a guardare il bosco, come se tu non ci fossi già più.)"),
			}).Fine();

		return D;
	}

	/** Tebaldo, il veterano del posto di guardia: due parole quando bastano due, una quando basta una. */
	FValdorsoDialogo DialoghiCorrado()
	{
		FValdorsoDialogo D;
		D.Id = TEXT("GuardiaCorrado");
		D.NomeBreve = TEXT("Tebaldo");
		D.Titolo = TEXT("Tebaldo, guardia della Corona");
		D.Inizio = TEXT("primo");
		D.Ritorno = TEXT("ritorno");

		D.Nodo(TEXT("primo"), {
			TEXT("(Una guardia grande come una porta ti guarda dall'alto in basso. La lancia non si muove di un dito.)"),
			TEXT("Alt. Posto di guardia della Corona. Nome e merci."),
			})
			.Scelta(TEXT("Voglio passare il sigillo."), TEXT("passare"))
			.Scelta(TEXT("Cos'è quel velo di luce?"), TEXT("sigillo"))
			.Scelta(TEXT("Chi comanda qui?"), TEXT("comanda"))
			.Scelta(TEXT("Niente. Me ne vado."), TEXT("congedo"));

		D.Nodo(TEXT("ritorno"), {
			TEXT("Di nuovo tu."),
			TEXT("Il passo è chiuso come ieri. Come sarà domani."),
			})
			.Scelta(TEXT("Voglio passare il sigillo."), TEXT("passare"))
			.Scelta(TEXT("Cos'è quel velo di luce?"), TEXT("sigillo"))
			.Scelta(TEXT("Chi comanda qui?"), TEXT("comanda"))
			.Scelta(TEXT("Niente. Me ne vado."), TEXT("congedo"));

		D.Nodo(TEXT("altro"), {
			TEXT("Altro?"),
			})
			.Scelta(TEXT("Voglio passare il sigillo."), TEXT("passare"))
			.Scelta(TEXT("Cos'è quel velo di luce?"), TEXT("sigillo"))
			.Scelta(TEXT("Chi comanda qui?"), TEXT("comanda"))
			.Scelta(TEXT("No. Me ne vado."), TEXT("congedo"));

		D.Nodo(TEXT("passare"), {
			TEXT("No."),
			TEXT("(Aspetti che aggiunga qualcosa. Non aggiunge niente.)"),
			})
			.Scelta(TEXT("Perché?"), TEXT("perche"))
			.Scelta(TEXT("(Offri una moneta) Magari con questa ci ripensi."), TEXT("moneta"), TEXT("Corruzione"))
			.SceltaSegna(TEXT("MonetaOfferta")).SceltaNascostaSe(TEXT("MonetaOfferta"))  // (08/11) una volta sola
			.Scelta(TEXT("E se passassi lo stesso?"), TEXT("forza"))
			.Scelta(TEXT("Va bene."), TEXT("altro"));

		D.Nodo(TEXT("perche"), {
			TEXT("Ordine della Corona. Il Balivo lo fa rispettare al villaggio. La capitana Livia lo fa rispettare a me."),
			TEXT("Nessuno passa il sigillo. Né per uscire né per entrare. Nemmeno le lettere."),
			})
			.Scelta(TEXT("Fino a quando?"), TEXT("finoaquando"))
			.Scelta(TEXT("Capito."), TEXT("altro"));

		D.Nodo(TEXT("finoaquando"), {
			TEXT("Finché la Corona vuole."),
			TEXT("Quando lo toglieranno, lo saprai. Lo sapranno tutti. Suoneranno le campane del tempio."),
			})
			.Scelta(TEXT("Altro."), TEXT("altro"))
			.Scelta(TEXT("Me ne vado."), TEXT("congedo"));

		D.Nodo(TEXT("moneta"), {
			TEXT("(Tebaldo guarda la moneta. Poi guarda te. La moneta resta dov'è.)"),
			TEXT("Mettila via. Adesso."),
			TEXT("Questa va nel registro della capitana. La prossima volta che allunghi la mano verso una guardia della Corona, la mano la lasci qui."),
			})
			.Scelta(TEXT("Era uno scherzo."), TEXT("scherzo"))
			.Scelta(TEXT("(Rimetti via la moneta e te ne vai.)"), NAME_None);

		D.Nodo(TEXT("scherzo"), {
			TEXT("Allora non era divertente."),
			TEXT("Il sigillo non si compra. Noi nemmeno. Torna alla valle, colono."),
			}).Fine();

		D.Nodo(TEXT("forza"), {
			TEXT("(Tebaldo fa un cenno con il mento verso il velo, senza voltarsi.)"),
			TEXT("Provaci. Il sigillo ti rimanda indietro come un sasso dalla fionda. Poi noi ti raccogliamo e ti scriviamo nel registro. Due volte, se fai lo spiritoso."),
			})
			.Scelta(TEXT("Altro."), TEXT("altro"))
			.Scelta(TEXT("Me ne vado."), TEXT("congedo"));

		D.Nodo(TEXT("sigillo"), {
			TEXT("Il sigillo della Corona. Rune. Da parete a parete, fino in cima alla gola."),
			TEXT("L'hanno messo i maghi della Corona. Come funziona non lo so, e non mi pagano per saperlo. Mi pagano per stare qui."),
			TEXT("Di notte trema, ogni tanto. Non è niente. Così dice la capitana."),
			})
			.Scelta(TEXT("Trema?"), TEXT("trema"))
			.Scelta(TEXT("Altro."), TEXT("altro"));

		D.Nodo(TEXT("trema"), {
			TEXT("Ho detto che non è niente."),
			TEXT("(Per la prima volta, Tebaldo guarda il velo alle sue spalle. Un attimo solo.)"),
			})
			.Scelta(TEXT("Altro."), TEXT("altro"))
			.Scelta(TEXT("Me ne vado."), TEXT("congedo"));

		D.Nodo(TEXT("comanda"), {
			TEXT("Qui la capitana Livia. Sopra di lei il Balivo, al villaggio. Sopra il Balivo la Corona di Aurelia."),
			TEXT("Sopra la Corona il cielo. Ma il cielo ordini non ne dà."),
			})
			.Scelta(TEXT("Altro."), TEXT("altro"))
			.Scelta(TEXT("Me ne vado."), TEXT("congedo"));

		D.Nodo(TEXT("congedo"), {
			TEXT("Torna alla valle, colono. E stai lontano dal velo."),
			}).Fine();

		return D;
	}

	/** Fulvio, la guardia giovane: viene dalla capitale, è qui da primavera, ha giurato e ci crede. */
	FValdorsoDialogo DialoghiOttone()
	{
		FValdorsoDialogo D;
		D.Id = TEXT("GuardiaOttone");
		D.NomeBreve = TEXT("Fulvio");
		D.Titolo = TEXT("Fulvio, guardia della Corona");
		D.Inizio = TEXT("primo");
		D.Ritorno = TEXT("ritorno");

		D.Nodo(TEXT("primo"), {
			TEXT("(La guardia più giovane si raddrizza quando ti vede arrivare, e stringe la lancia come gliel'hanno insegnato.)"),
			TEXT("Fermo dove sei. Il passo per Aurelia è chiuso per ordine della Corona. Non si passa."),
			})
			.Scelta(TEXT("Da dove vieni?"), TEXT("chi"))
			.Scelta(TEXT("Perché è chiuso?"), TEXT("perche"))
			.Scelta(TEXT("(Offri una moneta) Per un amico una moneta si trova sempre, no?"), TEXT("moneta"), TEXT("Corruzione"))
			.SceltaSegna(TEXT("MonetaOfferta")).SceltaNascostaSe(TEXT("MonetaOfferta"))  // (08/11) una volta sola
			.Scelta(TEXT("Va bene, me ne vado."), TEXT("congedo"));

		D.Nodo(TEXT("ritorno"), {
			TEXT("Ancora qui? Il passo non si è aperto, se è questo che speravi."),
			})
			.Scelta(TEXT("Da dove vieni?"), TEXT("chi"))
			.Scelta(TEXT("Perché è chiuso?"), TEXT("perche"))
			.Scelta(TEXT("(Offri una moneta) Per un amico una moneta si trova sempre, no?"), TEXT("moneta"), TEXT("Corruzione"))
			.SceltaSegna(TEXT("MonetaOfferta")).SceltaNascostaSe(TEXT("MonetaOfferta"))  // (08/11) una volta sola
			.Scelta(TEXT("Niente. Me ne vado."), TEXT("congedo"));

		D.Nodo(TEXT("altro"), {
			TEXT("Altro?"),
			})
			.Scelta(TEXT("Da dove vieni?"), TEXT("chi"))
			.Scelta(TEXT("Perché è chiuso?"), TEXT("perche"))
			.Scelta(TEXT("No. Me ne vado."), TEXT("congedo"));

		D.Nodo(TEXT("chi"), {
			TEXT("Dalla capitale. Mi hanno mandato su a primavera, con Tebaldo e con la capitana Livia."),
			TEXT("Tebaldo dice che al freddo ci si abitua. Tebaldo parla poco, ma di solito ha ragione."),
			})
			.Scelta(TEXT("Ti manca casa?"), TEXT("casa"))
			.Scelta(TEXT("Altro."), TEXT("altro"));

		D.Nodo(TEXT("casa"), {
			TEXT("(Fulvio esita.)"),
			TEXT("Non sta a me dirlo. Sto dove mi mette la Corona."),
			TEXT("Però d'estate, ad Aurelia, ci sono i fichi. Qui crescono solo le more."),
			})
			.Scelta(TEXT("Altro."), TEXT("altro"))
			.Scelta(TEXT("Buona guardia, Fulvio."), TEXT("congedo"));

		D.Nodo(TEXT("perche"), {
			TEXT("Ordine della Corona, firmato e sigillato. Il Balivo l'ha letto in piazza."),
			TEXT("Nessuno passa il sigillo. Nemmeno noi. Le provviste ce le porta su il villaggio, non arrivano da fuori."),
			})
			.Scelta(TEXT("E perché la Corona l'ha chiuso?"), TEXT("perchecorona"))
			.Scelta(TEXT("Altro."), TEXT("altro"));

		D.Nodo(TEXT("perchecorona"), {
			TEXT("Non lo so."),
			TEXT("(Una pausa.)"),
			TEXT("E se lo sapessi non te lo direi. Ma non lo so."),
			TEXT("C'è chi dice che il sigillo non serve a tenere dentro noi, ma a tenere fuori qualcos'altro. Io dico che sono chiacchiere da locanda."),
			})
			.Scelta(TEXT("Altro."), TEXT("altro"))
			.Scelta(TEXT("Me ne vado."), TEXT("congedo"));

		D.Nodo(TEXT("moneta"), {
			TEXT("(Fulvio guarda la moneta, poi te, poi Tebaldo. Tebaldo non dice niente. Non serve.)"),
			TEXT("Sei matto? Rimettila in tasca."),
			TEXT("Ho giurato alla Corona. Un giuramento non si vende, né per una moneta né per cento. E il sigillo non lo apro io: non lo apre nessuno, con l'oro."),
			})
			.Scelta(TEXT("Nessuno lo verrebbe a sapere."), TEXT("insiste"), TEXT("Corruzione"))
			.Scelta(TEXT("Hai ragione. Scusa."), TEXT("altro"));

		D.Nodo(TEXT("insiste"), {
			TEXT("Lo saprei io."),
			TEXT("E adesso lo sa anche la capitana: questa la riferisco. Vattene, prima che chiami Tebaldo."),
			}).Fine();

		D.Nodo(TEXT("congedo"), {
			TEXT("Torna alla valle. E non avvicinarti al velo: fa male."),
			}).Fine();

		return D;
	}

	/** (08/11) Nonna Edda, la vecchia della palude: lingua lunga, occhi svegli, e una porta che non si apre (il lago). */
	FValdorsoDialogo DialoghiEdda()
	{
		FValdorsoDialogo D;
		D.Id = TEXT("NonnaEdda");
		D.NomeBreve = TEXT("Nonna Edda");
		D.Titolo = TEXT("Nonna Edda, la vecchia della palude");
		D.Inizio = TEXT("primo");
		D.Ritorno = TEXT("ritorno");

		D.Nodo(TEXT("primo"), {
			TEXT("(Una vecchia piccola come una fascina mescola qualcosa nel calderone. Non alza gli occhi.)"),
			TEXT("Hai salutato il canneto, entrando? No. Si vede. Il canneto è permaloso. Io di più."),
			TEXT("Sulla bacheca della Gilda c'è scritto che chi viene da Nonna Edda porta il dovuto. L'ho fatto scrivere io. Allora: cosa mi hai portato?"),
			})
			.Scelta(TEXT("Niente. Mi dispiace."), TEXT("niente")).SceltaSegna(TEXT("DovutoNiente"))
			.Scelta(TEXT("Solo rispetto."), TEXT("rispetto")).SceltaSegna(TEXT("DovutoRispetto"))
			.Scelta(TEXT("Cosa sarebbe, il dovuto?"), TEXT("dovuto"))
			.Scelta(TEXT("Ho sbagliato strada. Me ne vado."), TEXT("congedo"));

		D.Nodo(TEXT("ritorno"), {
			TEXT("(Nonna Edda non si volta. Annusa l'aria, come fanno i cani vecchi.)"),
			TEXT("Odore di piazza e di fumo di locanda: sei di nuovo tu. Stavolta il canneto l'hai salutato?"),
			TEXT("Siediti, se vuoi. Se non vuoi, sta' in piedi, ma non sulle mie radici di mandragola."),
			})
			.Scelta(TEXT("Parlami delle erbe."), TEXT("erbe"))
			.Scelta(TEXT("Gaspare dice che anche tu credi all'Orso."), TEXT("orso"))
			.Scelta(TEXT("Ho delle domande."), TEXT("ancora"))
			.Scelta(TEXT("Passavo soltanto. Buona giornata, Nonna."), TEXT("congedo"));

		D.Nodo(TEXT("niente"), {
			TEXT("(Nonna Edda ride, e il calderone fa una bolla grossa, come se ridesse anche lui.)"),
			TEXT("Niente! Almeno è onesto. Quelli del villaggio mi portano un uovo marcio e pretendono l'unguento per la gotta."),
			TEXT("Il niente lo accetto, la prima volta. Siediti sulla panca, che le gambe giovani si stancano a stare dritte davanti ai vecchi."),
			})
			.Scelta(TEXT("Grazie, Nonna. Posso farti qualche domanda?"), TEXT("ancora"))
			.Scelta(TEXT("La prossima volta porterò qualcosa. Ciao, Nonna."), TEXT("congedo"));

		D.Nodo(TEXT("rispetto"), {
			TEXT("Rispetto. Mh."),
			TEXT("(Ti guarda per la prima volta: due occhi chiari e svegli in una faccia di corteccia.)"),
			TEXT("Il rispetto non si mangia e nel calderone non ci va. Però è la cosa che mi portano più di rado. Va bene. Per oggi basta quello."),
			TEXT("(Ti mette in mano un mazzetto d'erbe legato con un filo d'erba.) Menta d'acqua e achillea. Sulle ferite piccole, non sulle grandi: per quelle vieni da me."),
			})
			.Scelta(TEXT("Posso farti qualche domanda?"), TEXT("ancora"))
			.Scelta(TEXT("Allora ti lascio al tuo calderone."), TEXT("congedo"));

		D.Nodo(TEXT("dovuto"), {
			TEXT("Il dovuto è quello che è giusto. Un mazzo d'ortica, se vieni dagli orti. Un pesce, se vieni dal lago. Una storia, se non hai altro: le storie scaldano, d'inverno."),
			TEXT("Per adesso a te basta un saluto fatto bene e le orecchie aperte. Il resto lo saprai quando ti servirà qualcosa da me. E ti servirà, non temere."),
			})
			.Scelta(TEXT("Allora ti saluto come si deve, Nonna Edda."), TEXT("rispetto")).SceltaSegna(TEXT("DovutoRispetto"))
			.Scelta(TEXT("Per oggi non ho niente."), TEXT("niente")).SceltaSegna(TEXT("DovutoNiente"));

		D.Nodo(TEXT("ancora"), {
			TEXT("(Nonna Edda butta nel calderone una manciata di qualcosa che sa di stagno e di menta.)"),
			TEXT("Domanda, domanda. Finché il calderone bolle, ho tempo."),
			})
			.Scelta(TEXT("Che cosa fai bollire?"), TEXT("erbe"))
			.Scelta(TEXT("Credi all'Orso, come Gaspare?"), TEXT("orso"))
			.Scelta(TEXT("Conosci le rune degli Antichi?"), TEXT("rune"))
			.Scelta(TEXT("Altro: di te, della palude..."), TEXT("altro"));

		D.Nodo(TEXT("altro"), {
			TEXT("Altro ancora? Sei curioso come una gazza. Le gazze, qui, finiscono nel calderone."),
			TEXT("(Lo dice senza cattiveria. Quasi.)"),
			})
			.Scelta(TEXT("Chi sei, Nonna Edda?"), TEXT("chi"))
			.Scelta(TEXT("Perché vivi nella palude?"), TEXT("palude"))
			.Scelta(TEXT("Cosa c'è sul fondo del lago?"), TEXT("fondo"))
			.SceltaSegna(TEXT("FondoDelLago")).SceltaNascostaSe(TEXT("FondoDelLago"))  // si chiede una volta sola
			.Scelta(TEXT("Basta così. Ti lascio al calderone."), TEXT("congedo"));

		// --- Le erbe ----------------------------------------------------------------------------------------------
		D.Nodo(TEXT("erbe"), {
			TEXT("Ortica, per il sangue fiacco. Salice, per la testa che batte. Menta d'acqua, per chi ha mangiato da Bruno e se n'è pentito."),
			TEXT("E poi ci sono le erbe che non si dicono. Quelle crescono dove la palude è più scura, e si colgono con la luna giusta e la bocca chiusa."),
			TEXT("Il prete benedice, il Maestro scrive sui libri. Io faccio bollire. Alla fine la febbre scende con le mie, non con le loro."),
			})
			.Scelta(TEXT("Il Maestro? Chi è?"), TEXT("elric"))
			.Scelta(TEXT("Le erbe che non si dicono..."), TEXT("segrete"))
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"))
			.Scelta(TEXT("Ti lascio al calderone."), TEXT("congedo"));

		D.Nodo(TEXT("elric"), {
			TEXT("Maestro Elric. Quello che sa leggere tutto, tranne le facce."),
			TEXT("Davanti alla gente mi chiama strega, e dice che la palude fa ammalare chi ci passa vicino. Poi, di notte, manda il garzone a chiedermi la corteccia di salice per il suo mal di testa. E la paga, eh. Con le monete buone."),
			TEXT("(Ride sotto il naso.) Non dirglielo che te l'ho detto. O diglielo, che diventi rosso una volta tanto."),
			})
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"))
			.Scelta(TEXT("Me ne ricorderò. Ciao, Nonna."), TEXT("congedo"));

		D.Nodo(TEXT("segrete"), {
			TEXT("Ecco. Lo sapevo. Dici \"erbe\" e tutti sentono \"veleni\"."),
			TEXT("Non sono veleni. Sono erbe che fanno quello che vogliono loro, non quello che vuoi tu. Per usarle ci vogliono anni, e qualcuno che te le insegni."),
			TEXT("(Guarda verso il villaggio, oltre il canneto, come se aspettasse qualcuno.) Un giorno le insegnerò a qualcuno. Forse le sto già insegnando. Forse no. Fatti miei."),
			})
			.Scelta(TEXT("Le stai insegnando a qualcuno?"), TEXT("ragazza"))
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"));

		D.Nodo(TEXT("ragazza"), {
			TEXT("Ho detto forse."),
			TEXT("C'è una ragazzina del villaggio che viene a prendere le uova d'anatra. Le uova, dice sua madre. Se poi le uova le tiene in mano per un'ora, mentre io mescolo e parlo da sola, sono fatti delle uova."),
			TEXT("E tu non hai sentito niente. Né delle uova, né della ragazzina. Intesi?"),
			})
			.Scelta(TEXT("Non ho sentito niente."), TEXT("ancora"))
			.Scelta(TEXT("Intesi. Ti lascio, Nonna."), TEXT("congedo"));

		// --- L'Orso e il Cuore ------------------------------------------------------------------------------------
		D.Nodo(TEXT("orso"), {
			TEXT("(Nonna Edda smette di mescolare. Il cucchiaio di legno resta dritto nel calderone, da solo.)"),
			TEXT("Gaspare l'ha vista, l'ombra. Io l'ho sentita, che è diverso. D'inverno, quando la palude gela, sotto il ghiaccio passa un tepore. Come un fiato. Come se qualcuno, molto grande, dormisse lì sotto e si girasse nel sonno."),
			TEXT("Al villaggio ci chiamano i due matti, Gaspare e me. Va bene così: i matti dicono la verità e nessuno li sta a sentire. Così la verità resta tranquilla."),
			})
			.Scelta(TEXT("E il Cuore, nel tempio?"), TEXT("cuore"))
			.Scelta(TEXT("Tu e Gaspare vi conoscete da tanto?"), TEXT("gaspare"))
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"))
			.Scelta(TEXT("Ti lascio al calderone."), TEXT("congedo"));

		D.Nodo(TEXT("gaspare"), {
			TEXT("Gaspare. (Un sospiro, ma non è un sospiro triste.) Da ragazzi ballavamo alla festa del raccolto. Lui pestava i piedi a tutte e diceva che era colpa della musica."),
			TEXT("Adesso viene due volte l'anno, porta una lepre e se ne va senza sedersi. È il suo modo di dire grazie. Per cosa, lo sa lui. E lo so io."),
			TEXT("Se lo vedi, digli di fasciarsi stretto il ginocchio, quel testone."),
			})
			.Scelta(TEXT("Glielo dirò."), TEXT("ancora"))
			.Scelta(TEXT("Ti lascio al calderone."), TEXT("congedo"));

		D.Nodo(TEXT("cuore"), {
			TEXT("Il prete lo chiama il Cuore, e ci accende le candele davanti. Batte, dice. Due colpi e una pausa. E ha ragione: batte."),
			TEXT("Ma io sono vecchia, e mi faccio domande da vecchia. Per esempio: un cuore, prima di essere di pietra, di chi era?"),
			TEXT("(Torna a mescolare.) Non chiederlo ad Anselmo. Si spaventa, poveretto, e poi prega per me tutta la settimana."),
			})
			.Scelta(TEXT("Di chi era?"), TEXT("dichi"))
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"));

		D.Nodo(TEXT("dichi"), {
			TEXT("Ti ho detto che mi faccio domande. Non che ho le risposte."),
			TEXT("Le risposte stanno sotto. Sotto la collina, sotto le rovine, sotto l'acqua. Dove si arriva solo quando si è pronti, e quasi nessuno lo è."),
			TEXT("Torna quando avrai le mani sporche di palude e la testa un po' meno piena di fretta. Allora, forse, ti racconto una storia che il prete non racconta."),
			})
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"))
			.Scelta(TEXT("Tornerò, Nonna."), TEXT("congedo"));

		// --- Le rune ----------------------------------------------------------------------------------------------
		D.Nodo(TEXT("rune"), {
			TEXT("Le rune? Qualcuna. Non come il Maestro, che le copia sui fogli e poi non sa cosa farsene."),
			TEXT("Io ne so poche, ma le so usare. Questa, per esempio, ferma il sangue e chiude i tagli, se la dici bene e se l'acqua è pulita."),
			TEXT("rune: Nir-Ren | Nir-Ren, la Guarigione lieve"),
			TEXT("Nir, l'acqua. Ren, guarire. L'acqua che ripara. Per le ossa rotte non serve, quelle chiedile a Torvald. Ma per un taglio pulito, sì."),
			})
			.Scelta(TEXT("Insegnamela."), TEXT("insegna"))
			.Scelta(TEXT("Ne conosci altre?"), TEXT("ninna"))
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"))
			.Scelta(TEXT("Grazie, Nonna Edda."), TEXT("congedo"));

		D.Nodo(TEXT("insegna"), {
			TEXT("Insegnartela? Così, su due piedi, al primo che passa?"),
			TEXT("Le rune non sono parole da ripetere come un pappagallo. Prima si impara l'acqua: come scorre, dove ristagna, cosa porta via. Poi si impara la mano. Poi, se c'è tempo, la voce."),
			TEXT("Intanto scrivila sul tuo quaderno, e guardala la sera. Quando ti sembrerà di sentirla scorrere, torna da me."),
			})
			.Scelta(TEXT("Lo farò."), TEXT("ancora"))
			.Scelta(TEXT("Grazie, Nonna Edda."), TEXT("congedo"));

		D.Nodo(TEXT("ninna"), {
			TEXT("Una. Una sola che si può dire forte, perché non fa male a nessuno. Mia nonna me la cantava, e a lei la sua."),
			TEXT("(Nonna Edda chiude gli occhi e canta piano, con una voce che non ti aspettavi: bassa e ferma, come l'acqua di uno stagno.)"),
			TEXT("rune: Nir-Ren-Ka | Nir-Ren-Ka, Acqua che veglia"),
			TEXT("\"Nir-Ren-Ka, acqua che veglia, chiudi la porta e tieni la soglia. Dormi, piccino, che l'acqua è sveglia: finché lei canta, il buio non passa.\""),
			TEXT("(Riapre gli occhi.) Ka vuol dire proteggere. Le mamme del lago la cantavano quando le notti erano lunghe. Adesso la canto al calderone. Lui non si lamenta."),
			})
			.Scelta(TEXT("Proteggere da cosa?"), TEXT("buio"))
			.Scelta(TEXT("È bellissima. Ho altre domande."), TEXT("ancora"));

		D.Nodo(TEXT("buio"), {
			TEXT("Da quello che sta oltre la barriera. Da quello che si muove quando il Cuore salta un colpo."),
			TEXT("(Si stringe nello scialle, anche se accanto al fuoco fa caldo.)"),
			TEXT("Ma sono storie da notte lunga, e io ho la zuppa sul fuoco."),
			})
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"))
			.Scelta(TEXT("Ti lascio alla zuppa."), TEXT("congedo"));

		// --- Lei e la palude --------------------------------------------------------------------------------------
		D.Nodo(TEXT("chi"), {
			TEXT("Chi sono? Edda. Nonna per tutti, anche per quelli che hanno più rughe di me. Sono nata sul lago, quando il villaggio era dieci case e una campana."),
			TEXT("Ho fatto nascere metà della gente che adesso in piazza fa finta di non conoscermi. L'altra metà l'ho rimessa in piedi dopo una febbre. Mi salutano solo quando hanno il mal di pancia."),
			TEXT("Va bene così. Io sto qui, con le rane e il calderone. Le rane almeno ringraziano: cantano tutta la notte."),
			})
			.Scelta(TEXT("Perché vivi nella palude?"), TEXT("palude"))
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"))
			.Scelta(TEXT("Ti lascio alle tue rane."), TEXT("congedo"));

		D.Nodo(TEXT("palude"), {
			TEXT("Perché la palude è onesta. Ti dice subito dove affondi."),
			TEXT("E perché le erbe buone crescono qui, tra l'acqua ferma e la terra molle, dove il lago smette di essere lago. La capanna sta sui pali perché d'autunno l'acqua sale, e non chiede permesso."),
			TEXT("Quando vieni, cammina sulla passerella. Chi taglia dritto tra le pozze per fare prima, di solito arriva dopo. E con un piede solo dentro lo stivale."),
			})
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"))
			.Scelta(TEXT("Starò sulla passerella. Ciao, Nonna."), TEXT("congedo"));

		D.Nodo(TEXT("fondo"), {
			TEXT("(Nonna Edda smette di mescolare. Per un lungo momento si sente solo il calderone che borbotta.)"),
			TEXT("No."),
			TEXT("Questa non si chiede. Te l'hanno scritto sulla bacheca, e te l'avrà detto anche quel testone di Gaspare. Non si chiede, e basta."),
			TEXT("Adesso va', che mi hai fatto passare la voglia di chiacchierare."),
			}).Fine();

		D.Nodo(TEXT("congedo"), {
			TEXT("Va', va'. E sulla passerella guarda dove metti i piedi: le assi vecchie scricchiolano, quelle nuove mentono."),
			TEXT("(Nonna Edda torna al calderone, e comincia a canticchiare qualcosa che somiglia a una ninnananna.)"),
			}).Fine();

		return D;
	}

	/** (08/11) Bruno, il locandiere: vecchio sergente, tutte le voci della valle, e del Balivo parla piano. */
	FValdorsoDialogo DialoghiBruno()
	{
		FValdorsoDialogo D;
		D.Id = TEXT("Bruno");
		D.NomeBreve = TEXT("Bruno");
		D.Titolo = TEXT("Bruno, il locandiere");
		D.Inizio = TEXT("primo");
		D.Ritorno = TEXT("ritorno");

		D.Nodo(TEXT("primo"), {
			TEXT("(Un omone con un grembiule che fu bianco asciuga un boccale con uno straccio che non lo fu mai. Sull'avambraccio, una vecchia cicatrice da lama.)"),
			TEXT("Avanti, avanti, che il fuoco non lo pago per scaldare la porta! Faccia nuova! Questa è la Locanda di Bruno: l'unica della valle, quindi anche la migliore."),
			TEXT("Cosa ti porto? Zuppa, un letto o le chiacchiere? Le chiacchiere sono gratis. Ma costano care lo stesso."),
			})
			.Scelta(TEXT("Mi daresti una scodella di zuppa?"), TEXT("scodella"))
			.SceltaSegna(TEXT("PrimaScodella")).SceltaNascostaSe(TEXT("PrimaScodella"))  // la prima la offre la casa: una volta
			.Scelta(TEXT("Che si dice in giro?"), TEXT("voci"))
			.Scelta(TEXT("Parlami della locanda."), TEXT("locanda"))
			.Scelta(TEXT("Niente, grazie. Me ne vado."), TEXT("congedo"));

		D.Nodo(TEXT("ritorno"), {
			TEXT("(Bruno alza lo straccio come una bandiera.)"),
			TEXT("Ma guarda chi si rivede! Il solito? Non c'è, il solito, qui. C'è la zuppa, e se va bene è quella di ieri."),
			TEXT("Siediti dove vuoi, tranne lo sgabello all'angolo: quello è di Teodoro del mulino, e se lo trova occupato fa il muso per una settimana."),
			})
			.Scelta(TEXT("Novità dalla valle?"), TEXT("voci"))
			.Scelta(TEXT("Mi serve un letto per la notte."), TEXT("dormire"))
			.Scelta(TEXT("Ho qualche domanda."), TEXT("ancora"))
			.Scelta(TEXT("Passavo a salutare."), TEXT("congedo"));

		D.Nodo(TEXT("ancora"), {
			TEXT("(Bruno riempie un boccale, lo guarda controluce e lo vuota lui.)"),
			TEXT("Allora? Chiedi, chiedi. Un locandiere che non risponde è come un pozzo senz'acqua."),
			})
			.Scelta(TEXT("Che si dice in giro?"), TEXT("voci"))
			.Scelta(TEXT("Sei stato soldato, vero?"), TEXT("livia"))
			.Scelta(TEXT("Cosa pensi del Balivo?"), TEXT("balivo"))
			.Scelta(TEXT("Altro: la locanda, un letto..."), TEXT("altro"));

		D.Nodo(TEXT("altro"), {
			TEXT("Altro ancora? Hai più domande tu di un esattore della Corona."),
			})
			.Scelta(TEXT("Parlami della locanda."), TEXT("locanda"))
			.Scelta(TEXT("Mi serve un letto per la notte."), TEXT("dormire"))
			.Scelta(TEXT("Mi daresti una scodella di zuppa?"), TEXT("scodella"))
			.SceltaSegna(TEXT("PrimaScodella")).SceltaNascostaSe(TEXT("PrimaScodella"))
			.Scelta(TEXT("Basta così. Ti lascio lavorare."), TEXT("congedo"));

		// --- La locanda, la zuppa, il letto ------------------------------------------------------------------------
		D.Nodo(TEXT("scodella"), {
			TEXT("(Bruno sbatte sul bancone una scodella fumante di zuppa di cipolle e un pezzo di pane nero.)"),
			TEXT("Mangia. No, metti via quella mano, che non voglio vedere monete. La prima la offre la casa: è la regola di Bruno, ed è più vecchia della locanda."),
			TEXT("(Il pane nero te lo fa mettere nella bisaccia: per la strada, dice, che la valle è lunga.)"),
			TEXT("La seconda la paghi. Con gli Aureus della Corona, quando ne avrai da spendere: per adesso qui si paga in legna, in pesce e in storie belle."),
			})
			.Scelta(TEXT("Grazie, Bruno. È buonissima."), TEXT("ancora"))
			.Scelta(TEXT("Allora pago con una domanda: che si dice in giro?"), TEXT("voci"));

		D.Nodo(TEXT("locanda"), {
			TEXT("La locanda l'ho tirata su io, pietra per pietra. Be', le pietre le ha portate il carro di Teodoro. Ma l'idea era mia."),
			TEXT("Sotto si mangia e si beve, sopra si dorme. Le scale scricchiolano apposta: così so chi sale a quest'ora, e con chi."),
			TEXT("È il posto dove la valle viene a dire le cose che in piazza non dice. Io ascolto. Pulisco i boccali e ascolto. Sono bravissimo, a pulire i boccali."),
			})
			.Scelta(TEXT("Allora dimmi cosa hai sentito."), TEXT("voci"))
			.Scelta(TEXT("Un letto ce l'hai?"), TEXT("dormire"))
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"))
			.Scelta(TEXT("Ti lascio ai tuoi boccali."), TEXT("congedo"));

		D.Nodo(TEXT("dormire"), {
			TEXT("Un letto? Di sopra ce ne sono quattro. Le lenzuola le lava Agnese del mulino: sanno un po' di farina, ma sono pulite."),
			TEXT("Quanto costa? Niente, finché la Corona non decide quanto devono costare le cose da queste parti. Si dorme, si russa piano e si lascia il letto come lo si è trovato."),
			TEXT("Le scale sono in fondo. Il terzo gradino scricchiola: saltalo, se torni tardi."),
			})
			.Scelta(TEXT("Vado su, allora. Buonanotte, Bruno."), TEXT("buonanotte"))
			.Scelta(TEXT("Grazie. Prima però ho qualche domanda."), TEXT("ancora"));

		D.Nodo(TEXT("buonanotte"), {
			TEXT("Buonanotte. E se senti cantare per le scale, è Teodoro che torna a casa. Tu dormi lo stesso."),
			}).Fine();

		// --- Le voci dalla locanda (girano: dall'ultima si torna alla prima) ---------------------------------------
		D.Nodo(TEXT("voci"), {
			TEXT("(Bruno si sporge sul bancone e abbassa la voce, anche se nella sala non c'è nessuno.)"),
			TEXT("Ti dico quello che si dice, eh. Non quello che dico io. Io non dico mai niente."),
			TEXT("L'altra notte Padre Anselmo è uscito dal tempio bianco come un lenzuolo. Tobia, il ragazzino del prete, giura che il frammento del tempio ha saltato un battito. Due colpi e una pausa, da sempre. E quella notte un colpo solo, e poi niente, per il tempo di un respiro. Come la Pietra Cuore di Aurelia, nel 309: e quella volta lo seppe tutto il regno."),
			})
			.Scelta(TEXT("Un'altra voce?"), TEXT("sigillo"))
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"))
			.Scelta(TEXT("Me ne vado, Bruno."), TEXT("congedo"));

		D.Nodo(TEXT("sigillo"), {
			TEXT("Un'altra? Va bene. Questa me l'ha raccontata uno della milizia al terzo boccale, quindi vale per tre."),
			TEXT("Il sigillo del passo, quel velo di luce che la Corona ha messo da parete a parete: di notte trema. Le guardie dicono che non è niente. Ma Tebaldo, quello grosso del passo, che in vent'anni di guardia non ha mai guardato indietro: be', adesso il turno di notte lo fa con la schiena al muro e gli occhi al velo."),
			})
			.Scelta(TEXT("Un'altra voce?"), TEXT("ombra"))
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"));

		D.Nodo(TEXT("ombra"), {
			TEXT("Un'altra ancora? Sei peggio delle comari del lavatoio."),
			TEXT("Gaspare, il cacciatore. È sceso al villaggio per il sale, la settimana scorsa: lui, che scende due volte l'anno. Ha bevuto un sorso, uno, e ha detto: \"L'ombra si è avvicinata.\" Poi ha pagato con una lepre ed è sparito."),
			TEXT("Al villaggio ridono. Io no. Gaspare ha la testa dura, ma gli occhi buoni."),
			})
			.Scelta(TEXT("Un'altra voce?"), TEXT("luci"))
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"));

		D.Nodo(TEXT("luci"), {
			TEXT("L'ultima, poi basta, che le voci si consumano come le candele."),
			TEXT("I ragazzi della conceria giurano che di notte, alle rovine degli Antichi, ci sono delle luci. Non le rune, quelle si accendono da sole da sempre. Luci che si muovono. Lanterne. Come se laggiù qualcuno cercasse qualcosa, e non volesse farlo sapere al Balivo."),
			TEXT("Chi sia, non lo so. E se lo sapessi, lo saprei soltanto io."),
			})
			.Scelta(TEXT("Ricominciamo: cosa si dice del tempio?"), TEXT("voci"))
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"))
			.Scelta(TEXT("Basta voci per oggi."), TEXT("congedo"));

		// --- Livia e la guerra ------------------------------------------------------------------------------------
		D.Nodo(TEXT("livia"), {
			TEXT("(Bruno smette di asciugare il boccale. Si tira su la manica: la cicatrice arriva al gomito.)"),
			TEXT("Soldato? Sergente, prego. Dodici anni con la Corona, sulle terre di confine. E per otto di quegli anni la mia comandante è stata una ragazza smilza con la lingua più tagliente della spada: Livia Serra."),
			TEXT("Adesso è capitana e comanda il posto di guardia del passo. Io friggo cipolle. Ognuno ha fatto carriera a modo suo."),
			})
			.Scelta(TEXT("Com'era, la guerra?"), TEXT("guerra"))
			.Scelta(TEXT("Vi vedete ancora?"), TEXT("liviaora"))
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"));

		D.Nodo(TEXT("guerra"), {
			TEXT("Com'era? Fredda, quando non era bagnata. Lunga, quando non faceva paura. E piena di gente brava che adesso non c'è più."),
			TEXT("Sui confini abbiamo visto cose che al bancone non si raccontano. Ombre che non stavano ferme quando avrebbero dovuto. Livia le guardava in faccia. Io guardavo Livia, e così riuscivo a non scappare."),
			TEXT("(Riprende ad asciugare il boccale, più forte di prima.) Ecco. Visto? Si finisce sempre a parlare di cose tristi. Bevi qualcosa."),
			})
			.Scelta(TEXT("Ombre? Come quelle oltre la barriera?"), TEXT("ombre"))
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"));

		D.Nodo(TEXT("ombre"), {
			TEXT("Non lo so. E a quest'ora non voglio saperlo."),
			TEXT("Ti dico una cosa sola, e poi chiudo il discorso: quando Livia ha saputo che la Corona metteva un sigillo al passo, non ha chiesto perché. Ha chiesto quanto era alto. Una che chiede quanto è alto un muro sa già cosa c'è dall'altra parte."),
			})
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"))
			.Scelta(TEXT("Ci penserò. Ciao, Bruno."), TEXT("congedo"));

		D.Nodo(TEXT("liviaora"), {
			TEXT("Ogni tanto scende al villaggio per il rapporto al Balivo. Passa di qui dopo, mai prima: dice che prima del Balivo non si beve, e dopo il Balivo si deve."),
			TEXT("Ci sediamo a quel tavolo là, vicino alle botti. Parliamo poco. Una volta, a un tavolo così, eravamo in trenta. Adesso siamo in due, e bastiamo."),
			})
			.Scelta(TEXT("Cosa pensi del Balivo?"), TEXT("balivo"))
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"));

		// --- Il Balivo (con prudenza) -----------------------------------------------------------------------------
		D.Nodo(TEXT("balivo"), {
			TEXT("(Bruno guarda la porta. Poi la finestra. Poi te.)"),
			TEXT("Del Balivo? Il Balivo Corrado Valli è un uomo devoto alla Corona, attento alle tasse e generoso con le parole. Soprattutto con le parole."),
			TEXT("Ecco. Ho detto tutto quello che c'era da dire, e qualcosa in più. Tu non hai sentito niente, e io ho solo lodato le autorità."),
			})
			.Scelta(TEXT("Non ti piace, eh?"), TEXT("balivo2"))
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"));

		D.Nodo(TEXT("balivo2"), {
			TEXT("Piacermi? Mi ha raddoppiato la tassa sulla birra perché la birra \"rende allegri i coloni, e i coloni allegri non lavorano\". Parole sue, scritte e sigillate."),
			TEXT("E da quando c'è il sigillo al passo comanda come se la valle fosse casa sua e noi i mobili. Ma basta, basta: i muri hanno orecchie, e in questa valle anche le botti."),
			TEXT("(Più piano.) Certo, c'è chi va e viene senza passare sotto il naso del Balivo. Vera, per esempio: non la vedi mai entrare, e la trovi sempre dentro. Da dove passa, chissà. Sotto la torre, dicono i bambini. I bambini dicono tante cose."),
			})
			.Scelta(TEXT("Chi è Vera?"), TEXT("vera"))
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"));

		D.Nodo(TEXT("vera"), {
			TEXT("Chi? Ho detto Vera? Ho detto \"vero\". \"È vero\", ho detto. A quest'ora mi si impasta la lingua."),
			TEXT("(Bruno ti versa un dito di sidro che non hai chiesto.) Bevi, che fa bene alla memoria. A dimenticare, intendo."),
			})
			.Scelta(TEXT("Come non detto."), TEXT("ancora"))
			.Scelta(TEXT("Me ne vado, Bruno."), TEXT("congedo"));

		D.Nodo(TEXT("congedo"), {
			TEXT("Vai, vai. E torna, che le storie nuove le porta la gente nuova."),
			TEXT("(Bruno torna a strofinare lo stesso boccale. Brilla già da un pezzo.)"),
			}).Fine();

		return D;
	}

	/** (08/11) Padre Anselmo, il custode del frammento: mite, stanco, e più attento di quanto sembri. */
	FValdorsoDialogo DialoghiAnselmo()
	{
		FValdorsoDialogo D;
		D.Id = TEXT("PadreAnselmo");
		D.NomeBreve = TEXT("Padre Anselmo");
		D.Titolo = TEXT("Padre Anselmo, sacerdote del Tempio del Cuore");
		D.Inizio = TEXT("primo");
		D.Ritorno = TEXT("ritorno");

		D.Nodo(TEXT("primo"), {
			TEXT("(Un uomo anziano e magro, con la tonaca rammendata ai gomiti, spegne le candele consumate con due dita bagnate.)"),
			TEXT("Ah. Ti riconosco. Ti ho scritto io nel Registro di Val d'Orso, con questa mano che trema un po', la sera del tuo arrivo."),
			TEXT("Benedetto il Cuore che ti ha portato fin qui. Siediti, se vuoi: le panche sono dure, ma sono di tutti."),
			})
			.Scelta(TEXT("Cos'è il Cuore?"), TEXT("cuore"))
			.Scelta(TEXT("Parlami della pietra sull'altare."), TEXT("pietra"))
			.Scelta(TEXT("In quali dèi crede la valle?"), TEXT("fedi"))
			.Scelta(TEXT("Grazie, padre. Me ne vado."), TEXT("congedo"));

		D.Nodo(TEXT("ritorno"), {
			TEXT("(Padre Anselmo alza gli occhi dal libro dei canti e sorride.)"),
			TEXT("Ecco una faccia che conosco. Il Cuore ti ha riportato qui: è un buon segno."),
			TEXT("Cosa ti porta al tempio? Una domanda, una preghiera o soltanto la voglia di stare al fresco? Vanno bene tutte e tre."),
			})
			.Scelta(TEXT("Raccontami ancora del Cuore."), TEXT("cuore"))
			.Scelta(TEXT("Parlami delle fedi."), TEXT("fedi"))
			.Scelta(TEXT("Ho delle domande."), TEXT("ancora"))
			.Scelta(TEXT("Solo un saluto, padre."), TEXT("congedo"));

		D.Nodo(TEXT("ancora"), {
			TEXT("(Padre Anselmo raddrizza una candela storta. Poi un'altra.)"),
			TEXT("Dimmi pure. Il tempio ha tempo: è l'unica cosa che ha in abbondanza."),
			})
			.Scelta(TEXT("Il Cuore e il suo battito."), TEXT("cuore"))
			.Scelta(TEXT("La pietra e il territorio."), TEXT("pietra"))
			.Scelta(TEXT("Le fedi e i Vecchi Dei."), TEXT("fedi"))
			.Scelta(TEXT("Altro: le feste, la veglia, il ragazzino..."), TEXT("altro"));

		D.Nodo(TEXT("altro"), {
			TEXT("Altro? Chiedi. Se non so rispondere, almeno ti dico che non lo so: è già una risposta."),
			})
			.Scelta(TEXT("Ci sono feste, al tempio?"), TEXT("festa"))
			.Scelta(TEXT("Chi veglia il frammento, di notte?"), TEXT("veglia"))
			.Scelta(TEXT("Chi è il ragazzino che corre per il tempio?"), TEXT("tobia"))
			.Scelta(TEXT("Basta così. Grazie, padre."), TEXT("congedo"));

		// --- Il Cuore e il battito --------------------------------------------------------------------------------
		D.Nodo(TEXT("cuore"), {
			TEXT("Il Cuore. (Si volta verso l'altare, e abbassa la voce come davanti a qualcuno che dorme.)"),
			TEXT("Quella pietra sull'altare è un frammento del Cuore. Batte: due colpi e una pausa, come il cuore di un uomo quando è tranquillo. Batte da prima che ci fosse il villaggio, da prima che ci fosse la Corona."),
			TEXT("Finché batte, la barriera tiene, e le ombre restano dall'altra parte. Così mi hanno insegnato, e così insegno. Io ne sono il custode. Non il padrone: il custode."),
			})
			.Scelta(TEXT("E se smettesse di battere?"), TEXT("smettesse"))
			.Scelta(TEXT("Da dove viene il Cuore?"), TEXT("daviene"))
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"))
			.Scelta(TEXT("Grazie, padre."), TEXT("congedo"));

		D.Nodo(TEXT("smettesse"), {
			TEXT("(Padre Anselmo resta in silenzio un momento di troppo.)"),
			TEXT("Non smetterà. Ogni tanto il frammento si stanca, e va ricaricato con un rito che conosco io e pochi altri. Ci vuole pazienza, e qualche mano in più. Ma non smetterà."),
			TEXT("Ci sono state notti in cui... no. Chiacchiere da locanda: Bruno ne mette in giro più che zuppa. Il Cuore batte. Ascolta."),
			TEXT("(Ascolti. Dall'altare arriva un battito lento, due colpi e una pausa, che senti più nel petto che nelle orecchie.)"),
			})
			.Scelta(TEXT("Lo sento, padre."), TEXT("ancora"))
			.Scelta(TEXT("Me ne vado. Buona giornata."), TEXT("congedo"));

		D.Nodo(TEXT("daviene"), {
			TEXT("Le Cronache dicono che quando gli Antichi compirono il loro rito, sulla collina, qualcosa di grande cadde e qualcosa di grande restò. Il Cuore è quello che restò."),
			TEXT("Al villaggio i vecchi raccontano dell'Orso, dei Vecchi Dei, della collina. Io dico che l'Orso è caduto tanto tempo fa, e che adesso dorme nella terra e nei canti. È una bella storia. Le belle storie non fanno male a nessuno."),
			TEXT("Quello che so per certo è che il Cuore batte, e che qualcuno deve vegliarlo."),
			})
			.Scelta(TEXT("Vegliarlo?"), TEXT("veglia"))
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"));

		// --- La pietra, il territorio, il Registro ----------------------------------------------------------------
		D.Nodo(TEXT("pietra"), {
			TEXT("La pietra di Val d'Orso. Guardala pure da vicino: non morde. Sull'altare sono incise le parole del Registro, consumate dalle mani di chi le ha toccate per giurare."),
			TEXT("Val d'Orso è il primo territorio iscritto nel registro della Corona, da questa parte dei monti. Terra: Val d'Orso. Signore: la Corona di Aurelia. Governa il Balivo, per conto della Corona. E un giorno, dicono, sarà una baronia: la Baronia di Valdorso."),
			TEXT("La pietra è il segno di tutto questo. Dove c'è la pietra, c'è la valle. Chi firma il Registro riceve terra e protezione, e giura di restare."),
			})
			.Scelta(TEXT("Chi custodisce la pietra?"), TEXT("custode"))
			.Scelta(TEXT("Il Registro: perché tutte quelle domande?"), TEXT("registro"))
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"))
			.Scelta(TEXT("Grazie, padre."), TEXT("congedo"));

		D.Nodo(TEXT("custode"), {
			TEXT("Io. Padre Anselmo, sacerdote del villaggio, custode della pietra. Lo dice anche l'altare, se lo leggi."),
			TEXT("Il Balivo governa le tasse e le strade. La capitana Livia governa il passo. Io governo una pietra, dodici panche e un ragazzino che non sta mai fermo. Il lavoro più difficile è il ragazzino."),
			})
			.Scelta(TEXT("Il ragazzino?"), TEXT("tobia"))
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"));

		D.Nodo(TEXT("registro"), {
			TEXT("Perché il Registro non è un elenco di nomi. È la valle che ti chiede chi sei, prima di lasciarti restare."),
			TEXT("Da dove vieni, cosa porti, cosa temi, a chi affidi l'anima. Io scrivo, e il frammento ascolta. A volte, quando qualcuno risponde, le fiamme delle candele si piegano. Non chiedermi perché. Lo annoto, e basta."),
			})
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"))
			.Scelta(TEXT("Grazie, padre."), TEXT("congedo"));

		// --- Le fedi ----------------------------------------------------------------------------------------------
		D.Nodo(TEXT("fedi"), {
			TEXT("Le fedi. (Sorride, stanco.) Qui al tempio si pregano i Cinque, come in tutto il regno: Solara, la Luce, per la giustizia e la guarigione. Ignar, il Fuoco, per la forgia e la guerra. Nereia, l'Acqua, per il mare e i viaggi. Torvald, la Terra, per il raccolto e le montagne. Zefira, l'Aria, per la libertà e i messaggi."),
			TEXT("Ognuno sceglie il suo, e il Cuore batte per tutti. Anche per chi non ne sceglie nessuno: ce ne sono, e non li caccio."),
			})
			.Scelta(TEXT("E i Vecchi Dei della foresta?"), TEXT("vecchidei"))
			.Scelta(TEXT("Tu quale preghi?"), TEXT("tuo"))
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"))
			.Scelta(TEXT("Grazie, padre."), TEXT("congedo"));

		D.Nodo(TEXT("vecchidei"), {
			TEXT("(Padre Anselmo si guarda intorno, come per controllare che le candele non ascoltino.)"),
			TEXT("I Vecchi Dei... Sono le storie dei boscaioli e della gente di palude. Alberi che parlano, pietre che ricordano, un Orso più grande di una casa. La Corona dice che sono superstizioni."),
			TEXT("Io dico che quando qualcuno, nel Registro, affida l'anima ai Vecchi Dei, io scrivo e faccio finta di non sentire. È il modo più gentile che conosco per rispettare una fede che non è la mia."),
			TEXT("E poi, detto tra noi: quella vecchia della palude, Nonna Edda, ha guarito più febbri lei di tutte le mie preghiere messe insieme. Ma non dirlo in piazza."),
			})
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"))
			.Scelta(TEXT("Il tuo segreto è al sicuro. Ciao, padre."), TEXT("congedo"));

		D.Nodo(TEXT("tuo"), {
			TEXT("Io? Io prego Solara, la Luce. Da ragazzo volevo fare il medico, e Solara è quella che guarisce. Poi ho scoperto che le mie mani tremano troppo per cucire una ferita. Ma non per tenere un libro."),
			TEXT("E prego il Cuore. Non è un dio, il Cuore, e la Corona sarebbe molto seccata se dicessi il contrario. Ma ogni sera gli dico buonanotte, e lui mi risponde. Due colpi e una pausa."),
			})
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"))
			.Scelta(TEXT("Buonanotte anche a lui, allora. Ciao, padre."), TEXT("congedo"));

		// --- La festa, la veglia, Tobia e Nico --------------------------------------------------------------------
		D.Nodo(TEXT("festa"), {
			TEXT("Feste? Il raccolto, d'autunno, con i canti e il pane nuovo. La notte più lunga, d'inverno, quando si tengono accese tutte le candele fino all'alba."),
			TEXT("E poi c'è la cerimonia dell'anno e un giorno. Quando un colono resta nella valle un anno e un giorno da quando ha firmato il Registro, il suo nome si legge ad alta voce davanti al Cuore, e la valle... be'. La valle lo tiene."),
			TEXT("Per te è ancora presto. Ma tienilo a mente: un anno e un giorno. Non è tanto, e non è poco."),
			})
			.Scelta(TEXT("Cosa vuol dire, che la valle lo tiene?"), TEXT("tiene"))
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"));

		D.Nodo(TEXT("tiene"), {
			TEXT("Vuol dire quello che vuol dire. Che non sei più un ospite. Che il tuo nome è scritto dove le ombre non lo cancellano."),
			TEXT("(Sorride.) Lo vedrai. Ci sono cose che si spiegano meglio quando succedono."),
			})
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"))
			.Scelta(TEXT("Allora aspetterò. Ciao, padre."), TEXT("congedo"));

		D.Nodo(TEXT("veglia"), {
			TEXT("Di notte il frammento va vegliato. Qualcuno deve stare qui, accanto all'altare, e ascoltare il battito fino all'alba. Se cambia, se salta, se si fa debole, bisogna saperlo subito."),
			TEXT("Per ora lo veglio io. E quando i miei occhi non ce la fanno più, mi sveglia Tobia, che ha il sonno leggero dei bambini cresciuti senza mamma."),
			TEXT("Ma sono vecchio. Un giorno chiederò a qualcuno del villaggio di vegliare con me. Qualcuno di cui il Cuore si fidi. (Ti guarda.) Chissà."),
			})
			.Scelta(TEXT("Chi è Tobia?"), TEXT("tobia"))
			.Scelta(TEXT("Se un giorno vorrai, veglierò con te."), TEXT("ancora"))
			.Scelta(TEXT("Me ne vado, padre."), TEXT("congedo"));

		D.Nodo(TEXT("tobia"), {
			TEXT("Tobia. È arrivato al tempio in una cesta, una mattina di neve, sei anni fa. Nessuno l'ha mai reclamato. Adesso dorme in canonica, mangia per tre e conosce il tempio meglio di me: ogni crepa, ogni gradino."),
			TEXT("Se lo vedi correre in piazza, lascialo correre. Se lo vedi piangere, chiamami. Se qualcuno lo tratta male..."),
			TEXT("(La mano che trema si stringe sulla tonaca.) Diciamo che anche un vecchio prete ha qualche amico nella milizia."),
			})
			.Scelta(TEXT("E gli altri ragazzi del villaggio?"), TEXT("nico"))
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"))
			.Scelta(TEXT("Lo terrò d'occhio. Ciao, padre."), TEXT("congedo"));

		D.Nodo(TEXT("nico"), {
			TEXT("Gli altri... sono bravi ragazzi. Quasi tutti."),
			TEXT("C'è Nico. Non ha mai fatto niente di male. Niente che io sappia. Saluta, porta l'acqua al lavatoio, viene alla funzione. Eppure, quando entra nel tempio, io mi giro a guardare il frammento. Non so perché."),
			TEXT("Che il Cuore mi perdoni: è brutto diffidare di qualcuno senza un motivo. Dimentica quello che ho detto."),
			})
			.Scelta(TEXT("Lo dimentico, padre."), TEXT("ancora"))
			.Scelta(TEXT("Me ne vado. Buona giornata."), TEXT("congedo"));

		D.Nodo(TEXT("congedo"), {
			TEXT("Va' in pace. Che il Cuore batta forte per te, e le ombre restino dove sono."),
			TEXT("(Padre Anselmo torna alle sue candele, e per un attimo ti sembra che le fiamme si pieghino tutte insieme, verso l'altare.)"),
			}).Fine();

		return D;
	}

	TArray<FValdorsoDialogo> DialoghiScritti()
	{
		TArray<FValdorsoDialogo> Tutti;
		Tutti.Add(DialoghiGaspare());
		Tutti.Add(DialoghiCorrado());
		Tutti.Add(DialoghiOttone());
		// (08/11)
		Tutti.Add(DialoghiEdda());
		Tutti.Add(DialoghiBruno());
		Tutti.Add(DialoghiAnselmo());
		return Tutti;
	}
}
