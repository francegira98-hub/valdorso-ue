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
			// (11/11, v0.1.8) "Una lettera per casa" (ValdorsoStorie.h): solo a chi porta la lettera della capitana
			.Scelta(TEXT("Ho una lettera che deve passare il velo."), TEXT("corvo"))
			.SceltaVisibileSe(TEXT("Livia:LetteraData")).SceltaNascostaSe(TEXT("CorvoPartito"))
			.Scelta(TEXT("Gaspare dice che anche tu credi all'Orso."), TEXT("orso"))
			.Scelta(TEXT("Ho delle domande."), TEXT("ancora"))
			.Scelta(TEXT("Passavo soltanto. Buona giornata, Nonna."), TEXT("congedo"));

		D.Nodo(TEXT("corvo"), {
			TEXT("(Nonna Edda prende la lettera con due dita, la annusa, la gira. Il sigillo e' di cera semplice, senza stemma.)"),
			TEXT("Ah. La capitana. Una donna che mette le mani avanti anche quando chiede un favore: si vede dalla piega."),
			TEXT("Il velo ferma gli uomini, ragazzo mio. Ferma i carri, ferma le spade, ferma perfino le preghiere, dicono. Ma non ferma i corvi. I corvi sono piu' vecchi dei sigilli."),
			})
			.Scelta(TEXT("Allora mandala con un corvo."), TEXT("corvo_via"))
			.Scelta(TEXT("Ci devo pensare."), TEXT("congedo"));

		D.Nodo(TEXT("corvo_via"), {
			TEXT("(Fischia, basso. Dal tetto della capanna scende un corvo grosso come un gatto, con un occhio bianco. Nonna Edda gli lega la lettera alla zampa con un filo di giunco e gli parla all'orecchio, in una lingua che non e' la tua.)"),
			TEXT("(Il corvo sale, gira due volte sopra il lago e punta a sud. Quando arriva al velo, il velo trema appena, come l'acqua quando ci passa sopra il vento. Poi il corvo non c'e' piu'.)"),
			TEXT("Fatto. Di' alla capitana che il corvo non chiede ordini a nessuno. E che la prossima volta venga lei: ho un unguento per la sua schiena."),
			}).Segno(TEXT("CorvoPartito"))
			.Scelta(TEXT("Grazie, Nonna."), TEXT("congedo"));

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
			// (11/11, v0.2) l'acqua torbida (ValdorsoStorie.h): il server guarda se hai il secchio pieno
			.Scelta(TEXT("Ti ho portato un secchio d'acqua torbida."), TEXT("acqua_torbida"), FName(ValdorsoStorie::EventoAcquaEdda))
			.Scelta(TEXT("Ti lascio al calderone."), TEXT("congedo"));

		D.Nodo(TEXT("acqua_torbida"), {
			TEXT("(Nonna Edda annusa il secchio e fa una smorfia contenta, come davanti a una torta.)"),
			TEXT("Ah, l'acqua delle gallerie dei nani. Ferro, muschio e qualcosa che non ti dico. Per gli unguenti e' meglio di quella del lago: la ruggine tiene lontane le febbri."),
			TEXT("(Versa l'acqua nel calderone e ti rende il secchio. Poi ti mette in mano tre mazzetti d'erbe.) Il dovuto, al contrario. Non abituarti."),
			})
			.Scelta(TEXT("Grazie, Nonna."), TEXT("congedo"))
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"));

		D.Nodo(TEXT("acqua_niente"), {
			TEXT("(Nonna Edda ti guarda le mani.) Mi serve acqua torbida, ragazzo mio. Non quella del lago, che e' buona solo per le rane: quella ferrosa."),
			TEXT("Quella che serve a me sta nelle gallerie dei nani, sotto il nord, dove l'acqua ha allagato le miniere. Riempi un secchio laggiu' e portamelo: ti pago in erbe."),
			})
			.Scelta(TEXT("Tornero' con il secchio."), TEXT("congedo"))
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"));

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
			.Scelta(TEXT("Ti serve qualcosa, dalla valle?"), TEXT("acqua_niente"))
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
			.Scelta(TEXT("E di questi giorni, che si dice?"), TEXT("fresche"))
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
			.Scelta(TEXT("E di questi giorni, che si dice?"), TEXT("fresche"))
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"))
			.Scelta(TEXT("Basta voci per oggi."), TEXT("congedo"));

		// (11/11, v0.1.6) Le voci vere: quello che e' successo davvero nella valle, dal registro eventi del server
		// (ValdorsoVoci.h: la riga "{voci}" diventa da una a tre voci di adesso)
		D.Nodo(TEXT("fresche"), {
			TEXT("(Bruno si guarda intorno, posa lo straccio e si china sul bancone.)"),
			TEXT("{voci}"),
			TEXT("Questo è quello che gira adesso. Domani chissà: le voci sono come il pane, buone finché sono fresche."),
			})
			.Scelta(TEXT("E le voci di sempre?"), TEXT("voci"))
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"))
			.Scelta(TEXT("Grazie, Bruno."), TEXT("congedo"));

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
			TEXT("{senza:atto1}(Padre Anselmo alza gli occhi dal libro dei canti e sorride.)"),
			TEXT("{se:atto1}(Padre Anselmo alza gli occhi dal libro dei canti.)"),
			TEXT("{senza:atto1}Ecco una faccia che conosco. Il Cuore ti ha riportato qui: è un buon segno."),
			// (11/11, v0.1.6) l'Atto I "Il battito cambiato" (ValdorsoVoci.h): dopo, Anselmo non sorride piu'
			TEXT("{se:atto1}(Ha gli occhi cerchiati di chi non dorme, e non sorride.) Ecco una faccia che conosco. L'hai sentito anche tu, vero? Il battito. Si è fermato, per un respiro. E da allora, ogni tanto, si ferma ancora."),
			TEXT("Cosa ti porta al tempio? Una domanda, una preghiera o soltanto la voglia di stare al fresco? Vanno bene tutte e tre."),
			})
			// (11/11) la cerimonia dell'anno e un giorno (ValdorsoRiti.h): solo a chi e' arrivato il suo giorno
			.Scelta(TEXT("Padre, e' passato un anno e un giorno."), TEXT("cerimonia"), FName(ValdorsoRiti::EventoCerimonia))
			.SceltaVisibileSe(FName(ValdorsoRiti::SegnoAnno)).SceltaNascostaSe(FName(ValdorsoRiti::SegnoCittadino))
			.Scelta(TEXT("Raccontami ancora del Cuore."), TEXT("cuore"))
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
			.Scelta(TEXT("E per me, quanto manca?"), TEXT("presto"))
			.Scelta(TEXT("Ho altre domande."), TEXT("ancora"));

		// (11/11) la cerimonia (ValdorsoRiti.h): il server ci manda solo chi ha il segno dell'anno; gli altri a "presto"
		D.Nodo(TEXT("presto"), {
			TEXT("(Padre Anselmo apre il Registro, cerca la tua pagina e fa scorrere il dito sulla data della firma.)"),
			TEXT("Non e' ancora il tuo giorno, figliolo. Un anno e un giorno da quando hai firmato: quando sara' il momento, lo saprai. Il Cuore se ne accorge prima di noi."),
			})
			.Scelta(TEXT("Aspettero'."), TEXT("ancora"))
			.Scelta(TEXT("Grazie, padre."), TEXT("congedo"));

		D.Nodo(TEXT("cerimonia"), {
			TEXT("(Padre Anselmo chiude il libro dei canti. Va all'altare, prende il Registro e lo apre alla tua pagina. Poi suona la campanella, tre volte.)"),
			TEXT("Davanti al Cuore di Val d'Orso, che batte due colpi e una pausa da prima di tutti noi: questo nome e' stato scritto un anno e un giorno fa. Ha lavorato, ha camminato, ha dormito sotto questo cielo. La valle lo conosce."),
			TEXT("(Legge il tuo nome ad alta voce. Nel tempio vuoto la voce torna indietro due volte, come il battito.)"),
			TEXT("Da oggi non sei piu' un ospite. Sei di Valdorso. E Valdorso e' un po' tua."),
			}).Segno(FName(ValdorsoRiti::SegnoCittadino))
			.Scelta(TEXT("Grazie, padre."), TEXT("cittadino"))
			.Scelta(TEXT("(Resta in silenzio, e ascolta il battito.)"), TEXT("cittadino"));

		D.Nodo(TEXT("cittadino"), {
			TEXT("(Padre Anselmo sorride, e per un momento non sembra stanco.) Vai, cittadino. E torna a trovare il vecchio prete, ogni tanto."),
			}).Fine();

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
			.Scelta(TEXT("Quando si fa, la veglia?"), TEXT("di_giorno"))
			// (11/11, v0.1.8) la veglia del frammento (ValdorsoStorie.h): di notte si comincia davvero
			.Scelta(TEXT("Lasciami vegliare con te, stanotte."), TEXT("veglia_inizio"), FName(ValdorsoStorie::EventoVeglia))
			.Scelta(TEXT("Me ne vado, padre."), TEXT("congedo"));

		D.Nodo(TEXT("veglia_inizio"), {
			TEXT("(Padre Anselmo ti guarda a lungo. Poi prende due candele dall'altare, ne accende una con l'altra e te la mette in mano.)"),
			TEXT("Allora siediti qui, vicino alla pietra. Non serve pregare, se non vuoi. Serve ascoltare, e non andarsene."),
			TEXT("Quando la candela finisce, finisce la veglia. Se ti allontani, il Cuore resta solo: non succede niente di male, ma lui se ne accorge."),
			})
			.Scelta(TEXT("(Ti siedi accanto all'altare, e ascolti.)"), NAME_None);

		D.Nodo(TEXT("di_giorno"), {
			TEXT("(Padre Anselmo sorride e indica la finestra.) Di notte, figliolo: dalle nove fino all'alba, quando il villaggio dorme e il battito si sente meglio. Di giorno la pietra parla troppo piano."),
			TEXT("Se vuoi vegliare, torna a notte fonda e chiedimelo. Io sono qui."),
			})
			.Scelta(TEXT("Tornerò stanotte."), TEXT("congedo"));

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

	/** (10/11) Lucia, della Gilda: tiene il registro e il bancone. Assegna i fogli della bacheca, li timbra, e le monete
	 *  non le tocca mai (paga la cassa). Ha la vecchia mappa delle gallerie. "Ho un foglio della Gilda" apre il bancone
	 *  (l'evento BanconeGilda, ValdorsoGilda.h). */
	FValdorsoDialogo DialoghiLucia()
	{
		FValdorsoDialogo D;
		D.Id = TEXT("Lucia");
		D.NomeBreve = TEXT("Lucia");
		D.Titolo = TEXT("Lucia, della Gilda");
		D.Inizio = TEXT("primo");
		D.Ritorno = TEXT("ritorno");
		const FName Bancone(TEXT("BanconeGilda"));

		D.Nodo(TEXT("primo"), {
			TEXT("(Una donna magra, i capelli raccolti con una matita, scrive in un registro grande come una porta. Non alza gli occhi.)"),
			TEXT("Nome, mestiere e cosa sai fare. No, aspetta: prima dimmi se hai un foglio della bacheca. Se non ce l'hai, la bacheca e' quella, sulla parete."),
			TEXT("Sono Lucia. Tengo il registro della Gilda di Valdorso, il bancone e la pazienza. Delle tre, la pazienza finisce per prima."),
			})
			.Scelta(TEXT("Ho un foglio della Gilda."), NAME_None, Bancone)
			.Scelta(TEXT("Che cos'e' la Gilda?"), TEXT("gilda"))
			.Scelta(TEXT("Come si lavora per la Gilda?"), TEXT("come"))
			.Scelta(TEXT("Niente, grazie. Torno un'altra volta."), TEXT("congedo"));

		D.Nodo(TEXT("ritorno"), {
			TEXT("(Lucia intinge la penna, la scuote due volte e finalmente ti guarda.)"),
			TEXT("Ah, sei tu. Hai un foglio, o sei venuto a farmi perdere il segno?"),
			})
			.Scelta(TEXT("Ho un foglio della Gilda."), NAME_None, Bancone)
			.Scelta(TEXT("Ricordami come funziona."), TEXT("come"))
			.Scelta(TEXT("Dicono che tu abbia una mappa delle gallerie."), TEXT("mappa"))
			.Scelta(TEXT("Passavo a salutare."), TEXT("congedo"));

		D.Nodo(TEXT("gilda"), {
			TEXT("La Gilda e' la valle che si da' una mano, con un registro in mezzo perche' nessuno se lo dimentichi."),
			TEXT("Bruno vuole la legna, Teodoro vuole i cinghiali fuori dal grano, Gaspare vuole i lupi lontani dalle stalle. Scrivono a me, io appendo il foglio, qualcuno lo stacca. La cassa del villaggio paga. Io scrivo."),
			TEXT("Anche la Corona, ogni tanto, appende qualcosa. Quelli li riconosci: il sigillo e' piu' grosso e la paga piu' piccola."),
			})
			.Scelta(TEXT("E come si lavora?"), TEXT("come"))
			.Scelta(TEXT("E i gradi?"), TEXT("gradi"))
			.Scelta(TEXT("Ho gia' un foglio, in effetti."), NAME_None, Bancone)
			.Scelta(TEXT("Ho capito. Grazie, Lucia."), TEXT("congedo"));

		D.Nodo(TEXT("come"), {
			TEXT("Guardi la bacheca. Scegli un foglio. Lo stacchi: da quel momento e' tuo per mezz'ora, e nessun altro lo puo' prendere."),
			TEXT("Me lo porti. Io guardo il tuo grado e, se va bene, lo timbro: l'incarico e' assegnato, e da li' conta quello che fai. Una bestia uccisa prima del timbro, per la Gilda, e' solo una bestia morta."),
			TEXT("Fatto il lavoro torni qui: la legna sul bancone, o il racconto se ti avevo mandato da qualche parte. Timbro \"compiuto\", e la cassa ti paga. Le monete io non le tocco: sporcano il registro."),
			})
			.Scelta(TEXT("E i gradi?"), TEXT("gradi"))
			.Scelta(TEXT("Ho un foglio da farti timbrare."), NAME_None, Bancone)
			.Scelta(TEXT("Chiaro. Vado alla bacheca."), TEXT("congedo"));

		D.Nodo(TEXT("gradi"), {
			TEXT("Ogni foglio ha un sigillo di ceralacca. Rosso: Comune, li puo' fare chiunque abbia due mani. Bronzo, Argento, Oro: serve il grado, e il grado non si compra."),
			TEXT("I punti della Gilda si fanno lavorando: dieci per essere pronti al Bronzo, quaranta per l'Argento, centoventi per l'Oro. Ma il grado lo da' Mastro Ettore, il capo della filiale, quando ti ha visto lavorare. Lui, o il capo gilda, se la valle ne ha uno."),
			TEXT("Per l'Argento e per l'Oro tengo una cauzione, un decimo della paga: te la ridò alla fine. Se rinunci, resta alla Gilda."),
			TEXT("E rinunciare a un incarico che ti ho assegnato costa due punti. Non per cattiveria: per il registro. Il registro non dimentica."),
			})
			.Scelta(TEXT("E come si lavora, di preciso?"), TEXT("come"))
			.Scelta(TEXT("Ho un foglio da farti timbrare."), NAME_None, Bancone)
			.Scelta(TEXT("Va bene. Torno con un foglio."), TEXT("congedo"));

		// (11/11, v0.1.7) la storia "Quelli che non tornarono" (ValdorsoGrottaNord.h; Proposta nella Lore, da confermare):
		// la mappa delle gallerie del nord, la compagnia del 309, le tre cose da riportare
		const FName Mappa(ValdorsoGrottaNord::SegnoMappa);
		const FName Finite(ValdorsoGrottaNord::SegnoFinite);
		D.Nodo(TEXT("mappa"), {
			TEXT("(La penna si ferma a meta' di una parola.)"),
			TEXT("Chi te l'ha detto? Bruno. E' sempre Bruno."),
			TEXT("Si', la Gilda ha una mappa delle gallerie dei nani, quelle sotto la montagna del nord, dietro la frana. Vecchia, disegnata da gente che non e' tornata a correggerla. Da qualche anno, di notte, da laggiu' si sentono dei colpi. Nessuno della valle scava, la' sotto."),
			})
			.Scelta(TEXT("Me la mostreresti?"), TEXT("mappa2")).SceltaNascostaSe(Mappa)
			.Scelta(TEXT("Delle gallerie: ho qualcosa da dirti."), TEXT("gallerie_tue")).SceltaVisibileSe(Mappa).SceltaNascostaSe(Finite)
			.Scelta(TEXT("Parliamo d'altro: ho un foglio."), NAME_None, Bancone)
			.Scelta(TEXT("Lasciamo stare. A presto, Lucia."), TEXT("congedo"));

		D.Nodo(TEXT("mappa2"), {
			TEXT("A te? Con il sigillo rosso ancora fresco sulle dita? No."),
			TEXT("Fatti un nome alla Gilda. O dimostrami che sai guardare: quando qualcuno mi dira' che cosa c'e' davvero in fondo alla grotta a est, e tornera' per raccontarlo, allora ne riparliamo."),
			})
			.Scelta(TEXT("Sono stato in fondo alla grotta a est. Ho visto la porta."), TEXT("provalo"))
			.Scelta(TEXT("Ho capito. Ho un foglio, intanto."), NAME_None, Bancone)
			.Scelta(TEXT("Me ne ricordero'."), TEXT("congedo"));

		D.Nodo(TEXT("provalo"), {
			TEXT("(Lucia alza gli occhi dal registro, e questa volta li tiene su di te.)"),
			TEXT("Allora dimmi: che cosa c'e' inciso, sulla porta?"),
			})
			.Scelta(TEXT("Un sole con sette raggi."), TEXT("bugia"))
			.Scelta(TEXT("Cinque posti in cerchio. E una mano aperta, nel mezzo."), TEXT("storia"), FName(ValdorsoGrottaNord::EventoMappa))
			.Scelta(TEXT("Non ricordo bene. Era buio."), TEXT("bugia"));

		D.Nodo(TEXT("bugia"), {
			TEXT("Allora non ci sei stato. O non hai guardato. Laggiu' le due cose sono la stessa."),
			TEXT("Torna quando l'avrai vista davvero."),
			})
			.Scelta(TEXT("Tornero'."), TEXT("congedo"));

		D.Nodo(TEXT("storia"), {
			TEXT("(Lucia posa la penna. Apre un cassetto che ha la serratura, e ne tira fuori due fogli piegati in quattro.)"),
			TEXT("Autunno del 309. Da sotto la montagna del nord cominciarono i colpi, di notte. La filiale mando' quattro dei suoi: Ranieri, il capo della compagnia. Gisella, che disegnava le mappe. Cecco, minatore della valle, che leggeva i segni dei nani. E io, con la corda."),
			TEXT("Allargammo la fessura sopra la frana. Loro entrarono, io restai fuori. Il primo giorno la corda tiro' tre volte: tutto bene. Il secondo Gisella ci lego' queste due pagine. La terza notte la frana scese ancora, e la fessura si chiuse. Da allora ci passa solo l'aria."),
			TEXT("Prendile. Se un giorno la frana si apre, e tu scendi, riportami quello che resta di loro. Non per la Gilda."),
			}).Segno(Mappa)
			.Scelta(TEXT("Li cerchero'."), TEXT("promessa"))
			.Scelta(TEXT("Perche' non sei tornata a scavare?"), TEXT("perche"));

		D.Nodo(TEXT("perche"), {
			TEXT("Ci ho provato. Un inverno intero, con le mani e poi con il piccone di Marta. La frana e' roccia: si sposta solo quando la terra decide di muoversi."),
			TEXT("Dicono che una notte lo fara'. Quella notte, se senti tremare la montagna, vieni a dirmelo."),
			})
			.Scelta(TEXT("Li cerchero'."), TEXT("promessa"));

		D.Nodo(TEXT("promessa"), {
			TEXT("Il taccuino di Gisella. La lanterna di Cecco. L'anello di Ranieri: d'argento, con la lanterna della Gilda e una R."),
			TEXT("(Riprende la penna.) E adesso vai. Ho un registro da tenere, e non mi va che tu mi veda scrivere storto."),
			}).Fine();

		D.Nodo(TEXT("gallerie_tue"), {
			TEXT("(Lucia posa la penna e non dice niente. Aspetta.)"),
			})
			.Scelta(TEXT("Ho trovato le loro cose."), TEXT("consegna"), FName(ValdorsoGrottaNord::EventoConsegna))
			.Scelta(TEXT("Ne ho trovato solo una parte."), TEXT("mancano"))
			.Scelta(TEXT("Non ancora. Torno laggiu'."), TEXT("congedo"));

		D.Nodo(TEXT("mancano"), {
			TEXT("Allora non e' finita. Il taccuino, la lanterna, l'anello: tutte e tre, o niente."),
			TEXT("Le gallerie sono lunghe. Guarda dove i nani hanno messo le loro sale."),
			})
			.Scelta(TEXT("Tornero' con tutte e tre."), TEXT("congedo"));

		D.Nodo(TEXT("consegna"), {
			TEXT("(Lucia apre il taccuino di Gisella e lo chiude subito. Gira la lanterna di Cecco tra le mani. Poi prende l'anello, e lo tiene stretto.)"),
			TEXT("Ranieri non c'era, vero? Tra le ossa. Solo l'anello."),
			TEXT("(Chiude il registro, cosa che non fa mai.) Allora c'e' un'altra uscita, e lui l'ha trovata. Oltre la porta che i nani hanno chiuso. Un giorno qualcuno la aprira'."),
			TEXT("La cassa ti paga, il registro ti ricorda. E io ti devo qualcosa che non si scrive."),
			}).Segno(Finite)
			.Scelta(TEXT("Com'erano?"), TEXT("ricordo"))
			.Scelta(TEXT("Lo troveremo."), TEXT("congedo"));

		D.Nodo(TEXT("ricordo"), {
			TEXT("Gisella disegnava anche le cose che non c'erano: diceva che una mappa senza draghi ai bordi e' una bugia. Cecco cantava male e scavava bene. Ranieri..."),
			TEXT("Ranieri faceva sembrare facili le cose difficili. E' cosi' che si finisce sotto una montagna."),
			})
			.Scelta(TEXT("Grazie, Lucia."), TEXT("congedo"));

		D.Nodo(TEXT("congedo"), {
			TEXT("(Lucia e' gia' tornata al registro.) Chiudi la porta, che le carte volano."),
			}).Fine();

		return D;
	}

	/** (10/11) Mastro Ettore, capo della filiale della Gilda: i gradi li da' lui (cioe' lo staff, a suo nome). "A che punto
	 *  sono?" ha l'evento GradoGilda: il server risponde con i punti veri (UValdorsoGildaComponent::DimmiGrado). */
	FValdorsoDialogo DialoghiCapoGilda()
	{
		FValdorsoDialogo D;
		D.Id = TEXT("CapoGilda");
		D.NomeBreve = TEXT("Mastro Ettore");
		D.Titolo = TEXT("Mastro Ettore, capo della filiale");
		D.Inizio = TEXT("primo");
		D.Ritorno = TEXT("ritorno");
		const FName Grado(TEXT("GradoGilda"));

		D.Nodo(TEXT("primo"), {
			TEXT("(Un uomo largo di spalle, la barba grigia tagliata corta, legge un foglio tenendolo lontano dagli occhi. Ha le mani di chi ha lavorato prima di comandare.)"),
			TEXT("Mastro Ettore Bracci, capo della filiale di Valdorso. Lucia tiene il registro, io tengo la Gilda. Se cerchi lavoro, la bacheca e' quella. Se cerchi me, e' per i gradi."),
			})
			.Scelta(TEXT("Come si sale di grado?"), TEXT("gradi"))
			.Scelta(TEXT("A che punto sono con la Gilda?"), TEXT("punto"), Grado)
			.Scelta(TEXT("Chi e' il capo gilda dei coloni?"), TEXT("capo"))
			.Scelta(TEXT("Niente, Mastro. Vi lascio al vostro foglio."), TEXT("congedo"));

		D.Nodo(TEXT("ritorno"), {
			TEXT("(Mastro Ettore abbassa il foglio di un dito.)"),
			TEXT("Ancora tu. Bene: chi torna di solito ha lavorato. Dimmi."),
			})
			.Scelta(TEXT("A che punto sono con la Gilda?"), TEXT("punto"), Grado)
			.Scelta(TEXT("Ricordatemi come si sale di grado."), TEXT("gradi"))
			.Scelta(TEXT("Chi e' il capo gilda dei coloni?"), TEXT("capo"))
			.Scelta(TEXT("Passavo soltanto."), TEXT("congedo"));

		D.Nodo(TEXT("gradi"), {
			TEXT("Con il lavoro fai i punti: ogni foglio timbrato da Lucia ne vale qualcuno. Dieci e sei pronto per il Bronzo, quaranta per l'Argento, centoventi per l'Oro."),
			TEXT("Pronto non vuol dire promosso. Il grado lo do io, quando ho visto come lavori, e se hai rinunciato a meta' strada me ne ricordo. Il registro me lo ricorda, se mi dimentico."),
			})
			.Scelta(TEXT("E io, a che punto sono?"), TEXT("punto"), Grado)
			.Scelta(TEXT("Chi altro puo' dare i gradi?"), TEXT("capo"))
			.Scelta(TEXT("Ho capito. Torno al lavoro."), TEXT("congedo"));

		D.Nodo(TEXT("punto"), {
			TEXT("(Fa scorrere il dito sul registro di Lucia, riga per riga, finche' non trova il tuo nome.)"),
			TEXT("Ecco qui. Il resto te lo dico in faccia, che sui fogli le cose si leggono male."),
			})
			.Scelta(TEXT("Come si sale, di preciso?"), TEXT("gradi"))
			.Scelta(TEXT("Grazie, Mastro."), TEXT("congedo"));

		D.Nodo(TEXT("capo"), {
			TEXT("Quando la valle avra' qualcuno di fidato tra i coloni, lo faro' capo gilda. Potra' dare i gradi a chi li merita, fino all'Argento. L'Oro resta mio."),
			TEXT("E non pensare che il capo gilda faccia quello che vuole: ogni grado che da' finisce nel registro, e il registro lo leggo io."),
			})
			.Scelta(TEXT("E a che punto sono io?"), TEXT("punto"), Grado)
			.Scelta(TEXT("Capito. Vi lascio lavorare."), TEXT("congedo"));

		D.Nodo(TEXT("congedo"), {
			TEXT("(Mastro Ettore torna al suo foglio.) Vai, e fai in modo che Lucia abbia da scrivere."),
			}).Fine();

		return D;
	}

	// (11/11, v0.1.8 "Le storie della valle") le quest segrete dei volti nuovi sono dentro i loro dialoghi: la lettera di
	// Livia (con il corvo di Nonna Edda), il sogno di Tobia, l'incudine che canta di Marta, la veglia con Padre Anselmo.
	// Vedi ValdorsoStorie.h; i segni di un altro PNG si scrivono "Altro:Segno" (ValdorsoDialoghi::HaSegnoDi).
	// --- (11/11, v0.1.7 "Volti e giornate") Sei volti nuovi, scritti da Claude l'11/11/2026, dal Design MMO -----------
	// - Marta, la fabbra (la fucina): burbera, orgogliosa. Il segreto (l'incudine e' una pietra degli Antichi, con rune che
	//   si scaldano) NON si dice: si vede solo che l'incudine e' strana e che lei cambia discorso. Compra i cuori di
	//   pietra dei guardiani (la bottega). La spada spezzata del padre: un accenno.
	// - La capitana Livia Serra (al passo, con le guardie): dura, incorruttibile, parla poco; vecchia comandante di
	//   Bruno. Le lettere da casa e le pietre del sigillo: solo accenni (le quest verranno).
	// - Tobia, l'orfano (dieci anni, protetto da Padre Anselmo): curioso, sogna l'Orso e ha paura di dirlo. I suoi sogni
	//   anticipano i fatti (ValdorsoSogni: si sogna anche lui). Sempre trattato con delicatezza: e' un bambino.
	// - Teodoro e Agnese, i mugnai: la radio del villaggio. Due PNG, ognuno con le sue voci ({voci}) e le frecciate
	//   all'altro. Il grano nascosto: niente, per ora (v0.2).
	// - Maestro Elric, il mago di corte: arriva con l'Atto I (AValdorsoPNG::AttoMinimo = 1) e studia le rovine.
	//   Arrogante e brillante; disprezza Nonna Edda; ha paura di fallire davanti al collegio (si intuisce soltanto).
	FValdorsoDialogo DialoghiMarta()
	{
		FValdorsoDialogo D;
		D.Id = TEXT("Marta");
		D.NomeBreve = TEXT("Marta");
		D.Titolo = TEXT("Marta, la fabbra");
		D.Inizio = TEXT("primo");
		D.Ritorno = TEXT("ritorno");

		D.Nodo(TEXT("primo"), {
			TEXT("(Una donna con le braccia nude fino alle spalle e un grembiule di cuoio bruciacchiato tiene un ferro rosso sull'incudine. Batte tre colpi prima di guardarti.)"),
			TEXT("Se vuoi comprare, il banco e' quello. Se vuoi guardare, guarda da li': le scintille non chiedono permesso."),
			TEXT("Marta. La fucina e' mia, e prima era di mio padre. Il ferro che gira per la valle, chi l'ha fatto lo sai."),
			})
			.Scelta(TEXT("Che cosa sai fare?"), TEXT("lavoro"))
			.Scelta(TEXT("Che strana incudine. Di che pietra e'?"), TEXT("incudine"))
			.Scelta(TEXT("Cosa compri, alla fucina?"), TEXT("compri"))
			.Scelta(TEXT("Non ti disturbo. Buon lavoro."), TEXT("congedo"));

		D.Nodo(TEXT("ritorno"), {
			TEXT("(Marta immerge il ferro nel secchio. Il vapore sale fino alle travi.)"),
			TEXT("Tu. Hai rovinato la lama che ti ho venduto? Fammela vedere. No? Bene. Allora dimmi."),
			})
			.Scelta(TEXT("Raccontami del tuo lavoro."), TEXT("lavoro"))
			.Scelta(TEXT("Cosa compri, alla fucina?"), TEXT("compri"))
			.Scelta(TEXT("Ho sentito parlare delle gallerie dei nani."), TEXT("nani"))
			.Scelta(TEXT("Passavo a salutare."), TEXT("congedo"));

		D.Nodo(TEXT("lavoro"), {
			TEXT("Lame, punte, chiodi, cerchioni, serrature. Una zappa ben fatta sfama una famiglia per vent'anni; una spada ben fatta ne salva una in una notte. Faccio tutte e due, e non mi chiedere quale preferisco."),
			TEXT("Il metallo buono si sente sotto il martello: canta. Quello cattivo tossisce. Al passo arriva poco ferro, da quando c'e' il sigillo, e quel poco tossisce parecchio."),
			})
			.Scelta(TEXT("E cosa vorresti forgiare, un giorno?"), TEXT("sogno"))
			.Scelta(TEXT("Cosa compri, alla fucina?"), TEXT("compri"))
			.Scelta(TEXT("Grazie, Marta."), TEXT("congedo"));

		D.Nodo(TEXT("sogno"), {
			TEXT("(Per un momento il martello resta sospeso.)"),
			TEXT("Un'arma che si ricordino. Non per chi l'ha usata: per com'era fatta. Che tra cent'anni qualcuno la prenda in mano e dica: questa l'ha battuta una donna di Valdorso."),
			TEXT("Mio padre ne ha fatta una, una volta. Si e' spezzata. Non te lo racconto, non oggi."),
			})
			.Scelta(TEXT("E l'incudine? Sembra antica."), TEXT("incudine"))
			.Scelta(TEXT("Un giorno me lo racconterai."), TEXT("congedo"));

		D.Nodo(TEXT("incudine"), {
			TEXT("(Marta mette una mano piatta sull'incudine, come si fa con un cavallo. La pietra e' grigia, con delle linee sottili che sembrano graffi. Non e' ferro.)"),
			TEXT("E' pietra. La piu' dura della valle. L'ha trovata mio nonno, mezza sepolta vicino alle rovine, e ci sono voluti sei buoi per portarla qui. Da allora non si e' scheggiata una volta."),
			TEXT("Ma che domande sono? Sei venuto a comprare o a fare il curioso? Il banco e' quello."),
			})
			.Scelta(TEXT("Quei segni sembrano rune."), TEXT("rune"))
			.Scelta(TEXT("Va bene, va bene. Cosa compri, tu?"), TEXT("compri"))
			.Scelta(TEXT("Scusa. Me ne vado."), TEXT("congedo"));

		D.Nodo(TEXT("rune"), {
			TEXT("Graffi. Sono graffi. Sessant'anni di martello lasciano il segno anche sulla pietra."),
			TEXT("(Lo dice troppo in fretta. E per un momento, mentre ti volta le spalle, ti sembra che le linee sotto la sua mano siano piu' chiare, come braci sotto la cenere. Poi niente.)"),
			})
			.Segno(TEXT("IncudineVista"))
			.Scelta(TEXT("Come vuoi tu."), TEXT("congedo"));

		D.Nodo(TEXT("compri"), {
			TEXT("Ferro e rame buoni, quando ne trovi. E i cuori di pietra, quelle cose grigie che hanno nel petto i guardiani dei nani, la' sotto il nord: pesano come sassi e si lavorano come burro caldo. Te li pago bene, quarantacinque l'uno."),
			TEXT("Se trovi un martello dei nani, invece, tienitelo. Non c'e' niente che io sappia fare meglio di loro, e me ne vergogno un po'."),
			})
			.Scelta(TEXT("Le gallerie dei nani? Raccontami."), TEXT("nani"))
			// (11/11, v0.1.8) "L'incudine che canta" (ValdorsoStorie.h): solo a chi ha visto le linee dell'incudine
			.Scelta(TEXT("Ho un cuore di pietra. Battilo sulla tua incudine."), TEXT("canta"), FName(ValdorsoStorie::EventoIncudine))
			.SceltaVisibileSe(TEXT("IncudineVista")).SceltaNascostaSe(TEXT("Canta"))
			.Scelta(TEXT("Me ne ricordero'."), TEXT("congedo"));

		D.Nodo(TEXT("canta"), {
			TEXT("(Marta prende il cuore di pietra, lo soppesa, lo posa sull'incudine. Alza il martello, esita, e batte una volta sola.)"),
			TEXT("(Le linee dell'incudine si accendono. Non come braci: come acqua al sole. E dalla pietra sale una nota bassa, lunga, che fa tremare i ferri appesi alle pareti. Il cuore di pietra si scalda, si apre, diventa polvere luminosa e sparisce nelle linee.)"),
			TEXT("(Marta non dice niente per un pezzo. Poi prende un chiodo dal secchio, lo mette nel fuoco e lo batte sull'incudine che ancora canta. Quando lo raffredda, sulla testa c'e' un segno che lei non ha fatto.)"),
			TEXT("Va bene. Non sono graffi. Mio nonno lo sapeva, e mio padre anche, e io ho fatto finta per vent'anni. Prendi questo: e' il primo. Ce ne saranno altri, e non saranno chiodi. Ma di questo, alla Corona, non si parla."),
			}).Segno(TEXT("Canta"))
			.Scelta(TEXT("Non ne parlero'."), TEXT("congedo"));

		D.Nodo(TEXT("senza_cuore"), {
			TEXT("I cuori di pietra li hanno i guardiani dei nani, sotto il nord: nel petto, dove noi abbiamo il cuore vero. Grossi come armadi, lenti, e picchiano come una frana."),
			TEXT("(Ti guarda le mani.) E finche' non ne hai uno in tasca, e' inutile che vieni a chiedermi di batterlo. Prima trovalo."),
			})
			.Scelta(TEXT("Tornero' con uno."), TEXT("congedo"));

		D.Nodo(TEXT("nani"), {
			TEXT("Ne so quello che sa Bartolo, e Bartolo lo sa da una bottiglia. Che i nani lavoravano la pietra come noi lavoriamo il legno. Che sotto la montagna hanno lasciato forge, sale, e cose che non dovevano essere trovate."),
			TEXT("E trappole. Se ci scendi, guarda per terra: una piastra piu' scura del resto e' una piastra che ti vuole morto. E conta, quando senti le lame. I nani contavano tutto."),
			})
			.Scelta(TEXT("E i cuori di pietra, dove si trovano?"), TEXT("senza_cuore"))
			.Scelta(TEXT("Grazie del consiglio."), TEXT("congedo"));

		D.Nodo(TEXT("congedo"), {
			TEXT("(Marta e' gia' tornata al ferro.) Chiudi la tenda, che entra il freddo e il ferro si offende."),
			}).Fine();

		return D;
	}

	FValdorsoDialogo DialoghiLivia()
	{
		FValdorsoDialogo D;
		D.Id = TEXT("Livia");
		D.NomeBreve = TEXT("la capitana Livia");
		D.Titolo = TEXT("Livia Serra, capitana della Guardia");
		D.Inizio = TEXT("primo");
		D.Ritorno = TEXT("ritorno");

		D.Nodo(TEXT("primo"), {
			TEXT("(Una donna asciutta, i capelli corti grigi alle tempie, legge un rapporto in piedi. Ha il mantello della Corona e una cicatrice sottile sotto lo zigomo.)"),
			TEXT("Capitana Livia Serra. Comando il passo e chi lo guarda. Se sei qui per il sigillo, la risposta e' no. Se sei qui per un'altra cosa, sii breve."),
			})
			.Scelta(TEXT("Perche' il passo e' chiuso?"), TEXT("sigillo"))
			.Scelta(TEXT("Bruno dice che eri la sua comandante."), TEXT("bruno"))
			.Scelta(TEXT("(Offri una moneta) Forse possiamo intenderci."), TEXT("moneta"), TEXT("Corruzione"))
			.SceltaSegna(TEXT("MonetaOfferta")).SceltaNascostaSe(TEXT("MonetaOfferta"))
			.Scelta(TEXT("Niente, capitana."), TEXT("congedo"));

		D.Nodo(TEXT("ritorno"), {
			TEXT("(Livia non alza gli occhi dal rapporto.)"),
			TEXT("Ancora tu. Breve."),
			})
			.Scelta(TEXT("Parlami del sigillo."), TEXT("sigillo"))
			.Scelta(TEXT("Parlami di Bruno."), TEXT("bruno"))
			.Scelta(TEXT("Ti manca casa?"), TEXT("casa"))
			.Scelta(TEXT("Niente. Buona guardia."), TEXT("congedo"));

		D.Nodo(TEXT("sigillo"), {
			TEXT("Perche' la Corona l'ha ordinato, con un sigillo di ceralacca grosso come una mano. Io gli ordini li eseguo. Non li commento con i coloni."),
			TEXT("(Guarda il velo di luce, oltre la sbarra.) Ti dico solo questo: non toccarlo. E se di notte vedi qualcuno vicino alle pietre che lo tengono su, vieni a dirmelo. A me. Non al Balivo."),
			})
			.Segno(TEXT("PietreSigillo"))
			.Scelta(TEXT("Perche' non al Balivo?"), TEXT("balivo"))
			.Scelta(TEXT("Lo faro'."), TEXT("congedo"));

		D.Nodo(TEXT("balivo"), {
			TEXT("Perche' il Balivo scrive. Scrive tutto, e le cose scritte viaggiano. Io preferisco che certe cose restino tra chi le ha viste e chi deve saperle."),
			TEXT("E con questo ti ho detto anche troppo."),
			})
			.Scelta(TEXT("Me ne ricordero'."), TEXT("congedo"));

		D.Nodo(TEXT("bruno"), {
			TEXT("(Per la prima volta, l'angolo della bocca si muove.)"),
			TEXT("Bruno. Otto anni sui confini. Il miglior sergente che abbia avuto e il peggiore a giocare a dadi. Ha lasciato due dita in una palude, e ancora mi deve quattro monete."),
			TEXT("Quando passo dalla locanda beviamo, e non parliamo di quello che abbiamo visto. E' cosi' che si resta amici, dopo una guerra."),
			})
			.Scelta(TEXT("E tu, da dove vieni?"), TEXT("casa"))
			.Scelta(TEXT("Grazie, capitana."), TEXT("congedo"));

		D.Nodo(TEXT("casa"), {
			TEXT("Aurelia. Una casa con un fico nel cortile e una madre che mi scrive una volta al mese, anche se le lettere ci mettono due."),
			TEXT("(Piega il rapporto in quattro, piu' piano del necessario.) Da quando c'e' il sigillo non passano piu' nemmeno quelle. Ordini. Li ho firmati io, gli ordini."),
			})
			.Scelta(TEXT("Mi dispiace."), TEXT("dispiace"))
			.Scelta(TEXT("Se un giorno potro' portarne una, lo faro'."), TEXT("lettera")).SceltaSegna(TEXT("LetteraPromessa"))
			.SceltaNascostaSe(TEXT("LetteraPromessa"))
			// (11/11, v0.1.8) "Una lettera per casa" (ValdorsoStorie.h)
			.Scelta(TEXT("Scrivi la lettera, capitana. Il modo lo trovo io."), TEXT("scrivi"))
			.SceltaVisibileSe(TEXT("LetteraPromessa")).SceltaNascostaSe(TEXT("LetteraData"))
			.Scelta(TEXT("La tua lettera e' partita. Con un corvo."), TEXT("partita"))
			.SceltaVisibileSe(TEXT("NonnaEdda:CorvoPartito")).SceltaNascostaSe(TEXT("LetteraArrivata"));

		D.Nodo(TEXT("scrivi"), {
			TEXT("(La capitana ti guarda come si guarda un ponte che non si sa se regge. Poi tira fuori un foglio dalla giubba: e' gia' scritto, piegato e ripiegato tante volte che gli angoli sono morbidi.)"),
			TEXT("L'ho scritta la prima settimana. Non l'ho mai data a nessuno, perche' gli ordini li ho firmati io."),
			TEXT("(Te la mette in mano.) Il passo no. Il sigillo no. Qualunque altra strada trovi, io non voglio saperla. E se ti prendono, questa lettera non l'hai mai vista."),
			}).Segno(TEXT("LetteraData"))
			.Scelta(TEXT("Arrivera'."), TEXT("congedo"));

		D.Nodo(TEXT("partita"), {
			TEXT("Con un... (Si ferma. Guarda verso sud-ovest, dove c'e' la palude.) La vecchia. Naturalmente."),
			TEXT("(Per un momento la capitana Livia Serra non sembra una capitana. Si passa una mano sulla faccia, e quando la toglie e' di nuovo lei.)"),
			TEXT("Tieni. (Si sfila dal collo un fischietto d'ottone.) Otto anni sui confini. Se fischi tre volte, chi ha servito con me si volta. Bruno compreso, anche se fa finta di no."),
			}).Segno(TEXT("LetteraArrivata"))
			.Scelta(TEXT("Grazie, capitana."), TEXT("congedo"));

		D.Nodo(TEXT("dispiace"), {
			TEXT("Non ti dispiacere per me. Dispiaciti per chi sta dall'altra parte del velo e non sa se siamo ancora vivi."),
			})
			.Scelta(TEXT("Buona guardia, capitana."), TEXT("congedo"));

		D.Nodo(TEXT("lettera"), {
			TEXT("(Ti guarda a lungo, come si guarda una recluta che ha detto una cosa stupida e coraggiosa.)"),
			TEXT("Non l'hai detto. E io non l'ho sentito. Ma me lo ricordo."),
			})
			.Scelta(TEXT("Buona guardia, capitana."), TEXT("congedo"));

		D.Nodo(TEXT("moneta"), {
			TEXT("(Livia guarda la moneta. Poi te. Non dice niente per il tempo di tre respiri.)"),
			TEXT("Rimettila via, prima che la veda qualcuno che non sono io. Ho fatto impiccare uomini migliori di te per meno. No, non e' vero. Ma ci ho pensato."),
			TEXT("Questa volta faccio finta che tu sia solo stupido. La prossima no."),
			})
			.Scelta(TEXT("Chiedo scusa."), TEXT("congedo"));

		D.Nodo(TEXT("congedo"), {
			TEXT("(La capitana e' gia' tornata al rapporto.) Vai."),
			}).Fine();

		return D;
	}

	FValdorsoDialogo DialoghiTobia()
	{
		FValdorsoDialogo D;
		D.Id = TEXT("Tobia");
		D.NomeBreve = TEXT("Tobia");
		D.Titolo = TEXT("Tobia, il ragazzino del tempio");
		D.Inizio = TEXT("primo");
		D.Ritorno = TEXT("ritorno");

		D.Nodo(TEXT("primo"), {
			TEXT("(Un ragazzino con le ginocchia sbucciate e una fionda infilata nella cintura ti gira intorno due volte prima di fermarsi.)"),
			TEXT("Tu sei un colono nuovo! Si vede. Hai la faccia di uno che non sa ancora dove sta il forno. Il forno sta la'. Io sono Tobia. Abito al tempio, con Padre Anselmo."),
			TEXT("Hai mai visto un lupo? Da vicino? Quanto era grosso? Piu' di un cane? Molto di piu'?"),
			})
			.Scelta(TEXT("Si', ne ho visto uno. Grosso cosi'."), TEXT("lupo"))
			.Scelta(TEXT("Cosa fai tutto il giorno, Tobia?"), TEXT("giorno"))
			.Scelta(TEXT("Hai una faccia stanca. Dormi bene?"), TEXT("sogni"))
			.Scelta(TEXT("Devo andare, Tobia."), TEXT("congedo"));

		D.Nodo(TEXT("ritorno"), {
			TEXT("(Tobia ti corre incontro.)"),
			TEXT("Sei tornato! Hai una storia? Una storia vera, non come quelle di Teodoro che cambiano ogni volta."),
			})
			.Scelta(TEXT("Ti racconto di un lupo."), TEXT("lupo"))
			.Scelta(TEXT("Cosa hai fatto oggi?"), TEXT("giorno"))
			.Scelta(TEXT("Hai fatto ancora quel sogno?"), TEXT("sogni"))
			.Scelta(TEXT("Oggi niente storie, scusa."), TEXT("congedo"));

		D.Nodo(TEXT("lupo"), {
			TEXT("(Tobia spalanca gli occhi e sta zitto per tutto il racconto, cosa che non gli succede mai.)"),
			TEXT("Io da grande faccio l'avventuriero. Come te. Padre Anselmo dice che prima devo imparare a leggere. Ma gli avventurieri non leggono, combattono! ...Vero?"),
			})
			.Scelta(TEXT("Leggono anche. Le mappe, soprattutto."), TEXT("leggere"))
			.Scelta(TEXT("Combattono, ma solo quando serve."), TEXT("leggere"))
			.Scelta(TEXT("Devo andare, Tobia."), TEXT("congedo"));

		D.Nodo(TEXT("leggere"), {
			TEXT("Uffa. Va bene. Allora imparo. Ma solo le mappe."),
			TEXT("(Abbassa la voce.) Sai che Agnese del mulino fa le frittelle col miele, la domenica? Se le porti la legna te ne da' due. Una la puoi dare a me."),
			})
			.Scelta(TEXT("Affare fatto."), TEXT("congedo"));

		D.Nodo(TEXT("giorno"), {
			TEXT("La mattina aiuto Padre Anselmo a spazzare il tempio e a pulire la pietra. Poi vado al mulino, che Teodoro mi fa girare la ruota piccola. Poi vado alla fucina, ma Marta mi caccia. Poi torno."),
			TEXT("E la sera la capitana Livia passa dalla locanda e io la guardo dalla finestra. Ha una spada vera. Una volta mi ha fatto un cenno. Cosi'. (Fa un cenno serissimo con il mento.)"),
			})
			.Scelta(TEXT("E di notte, dormi bene?"), TEXT("sogni"))
			.Scelta(TEXT("Una vita piena. Ciao, Tobia."), TEXT("congedo"));

		D.Nodo(TEXT("sogni"), {
			TEXT("(Tobia smette di saltellare. Si guarda intorno, per vedere se c'e' qualcuno.)"),
			TEXT("Se te lo dico non lo dici a Padre Anselmo? Si preoccupa. Gli vengono le rughe qui."),
			})
			.Scelta(TEXT("Promesso. Non lo dico a nessuno."), TEXT("orso"))
			.Scelta(TEXT("Non devi dirmelo per forza."), TEXT("forza"))
			// (11/11, v0.1.8) "Il sogno di Tobia" (ValdorsoStorie.h): il server guarda se hai letto il sasso
			.Scelta(TEXT("Il sasso del tuo sogno: l'ho trovato."), TEXT("sasso_trovato"), FName(ValdorsoStorie::EventoSasso))
			.SceltaVisibileSe(TEXT("SassoSognato")).SceltaNascostaSe(TEXT("SassoTrovato"));

		D.Nodo(TEXT("sasso_sogno"), {
			TEXT("(Tobia si avvicina e parla piano, con le mani a coppa intorno alla bocca.)"),
			TEXT("Stanotte non era l'orso grande. Era un sasso. Un sasso grande, sulla collina dell'Orso, in cima, dove l'erba e' piegata. Ha la forma di un orso che dorme, con la testa sulle zampe."),
			TEXT("E sopra c'e' un segno. Nel sogno ci mettevo la mano e il segno era caldo. Io non ci posso andare, e' troppo lontano e Padre Anselmo si arrabbia. Ci vai tu? Poi mi dici se c'e' davvero?"),
			}).Segno(TEXT("SassoSognato"))
			.Scelta(TEXT("Ci vado. Te lo prometto."), TEXT("congedo"))
			.Scelta(TEXT("E cosa c'e' scritto, sul segno?"), TEXT("non_ancora"));

		D.Nodo(TEXT("sasso_trovato"), {
			TEXT("(Tobia spalanca la bocca. Poi la chiude. Poi la spalanca di nuovo.)"),
			TEXT("C'e' davvero? Con il segno? Allora... allora i miei sogni non sono matti. Allora l'orso me lo dice apposta."),
			TEXT("(Fruga nella tasca e tira fuori un sasso liscio, piccolo, a forma di orsetto.) Questo l'ho trovato al fiume, due anni fa. Lo tenevo sotto il cuscino. Adesso e' tuo, perche' tu ci sei andato. Non perderlo, eh."),
			}).Segno(TEXT("SassoTrovato"))
			.Scelta(TEXT("Lo terro' con cura."), TEXT("congedo"));

		D.Nodo(TEXT("non_ancora"), {
			TEXT("(Tobia ti guarda storto.) Non te lo dico. Io me lo ricordo, dal sogno, ma se te lo dico poi non vale: devi leggerlo tu, con i tuoi occhi."),
			TEXT("Vai sulla collina dell'Orso, in cima. Leggi il segno. Poi torni e me lo dici."),
			})
			.Scelta(TEXT("Hai ragione. Ci vado."), TEXT("congedo"));

		D.Nodo(TEXT("forza"), {
			TEXT("No, te lo dico. Tanto tu sei nuovo, e i nuovi non ridono."),
			})
			.Scelta(TEXT("Ti ascolto."), TEXT("orso"));

		D.Nodo(TEXT("orso"), {
			TEXT("Sogno un orso. Ogni notte. E' grande come il tempio, ma non fa paura. Sta seduto sulla collina e guarda giu', verso il villaggio, come fa Padre Anselmo quando dormo e lui crede che non lo veda."),
			TEXT("{senza:atto1}E certe volte, nel sogno, succede una cosa, e poi il giorno dopo succede davvero. Piccole cose. Che piove. Che Teodoro cade dallo sgabello. E' una cosa brutta?"),
			TEXT("{se:atto1}L'altra notte, nel sogno, l'orso ha messo la zampa sulla pietra del tempio e la pietra si e' fermata. E il giorno dopo... lo sai anche tu. Tutti dicono che e' il battito. Io l'avevo sognato prima. E' colpa mia?"),
			})
			.Segno(TEXT("SognoTobia"))
			.Scelta(TEXT("No, Tobia. Non e' colpa tua."), TEXT("colpa"))
			.Scelta(TEXT("Forse l'orso ti vuole bene."), TEXT("bene"))
			.Scelta(TEXT("Anch'io faccio sogni strani, qui."), TEXT("anche"))
			.Scelta(TEXT("E stanotte, cos'hai sognato?"), TEXT("sasso_sogno")).SceltaNascostaSe(TEXT("SassoSognato"));

		D.Nodo(TEXT("colpa"), {
			TEXT("(Tira su col naso, piano.) Va bene. Se lo dici tu, che sei grande."),
			TEXT("Se lo sogno ancora, te lo dico. Solo a te."),
			})
			.Scelta(TEXT("Solo a me. Ciao, Tobia."), TEXT("congedo"));

		D.Nodo(TEXT("bene"), {
			TEXT("Dici? (Ci pensa.) Si'. Secondo me si'. Non mi guarda mai come guarda il bosco. Mi guarda come si guarda una cosa da tenere."),
			TEXT("Ora vado, che se Padre Anselmo non mi trova per il vespro mi fa pulire tutte le panche."),
			})
			.Scelta(TEXT("Corri, allora."), TEXT("congedo"));

		D.Nodo(TEXT("anche"), {
			TEXT("Davvero? Anche tu? (Gli si illumina la faccia.) Allora non sono matto. Allora siamo in due."),
			TEXT("Quando sogni l'orso, fagli ciao con la mano. Io faccio cosi', e lui una volta ha girato la testa."),
			})
			.Scelta(TEXT("Lo faro'. Ciao, Tobia."), TEXT("congedo"));

		D.Nodo(TEXT("congedo"), {
			TEXT("(Tobia e' gia' corso via, a meta' strada, quando si gira e ti saluta con tutte e due le braccia.)"),
			}).Fine();

		return D;
	}

	FValdorsoDialogo DialoghiTeodoro()
	{
		FValdorsoDialogo D;
		D.Id = TEXT("Teodoro");
		D.NomeBreve = TEXT("Teodoro");
		D.Titolo = TEXT("Teodoro, il mugnaio");
		D.Inizio = TEXT("primo");
		D.Ritorno = TEXT("ritorno");

		D.Nodo(TEXT("primo"), {
			TEXT("(Un omone infarinato fino alle sopracciglia ti stringe la mano e te la lascia bianca.)"),
			TEXT("Teodoro, del mulino! Il mulino e' mio, la ruota e' mia, il grano e' della valle e la farina... be', la farina e' un po' di tutti, specie quella che resta sui vestiti."),
			TEXT("Sei nuovo? Allora non sai ancora niente. Siediti. No, non li': quello e' il mio sgabello. Ecco. Chiedimi pure, io so tutto. E quello che non so, lo sa Agnese, e me lo dice anche se non voglio."),
			})
			.Scelta(TEXT("Che si dice in giro?"), TEXT("voci"))
			.Scelta(TEXT("Chi e' Agnese?"), TEXT("agnese"))
			.Scelta(TEXT("Come va il mulino?"), TEXT("mulino"))
			.Scelta(TEXT("Ti lascio lavorare, Teodoro."), TEXT("congedo"));

		D.Nodo(TEXT("ritorno"), {
			TEXT("(Teodoro allarga le braccia, e una nuvola di farina si alza intorno a lui.)"),
			TEXT("Ecco il mio colono preferito! Be', uno dei sei. Che mi racconti? No, aspetta: prima racconto io."),
			})
			.Scelta(TEXT("Che si dice in giro?"), TEXT("voci"))
			.Scelta(TEXT("Come sta Agnese?"), TEXT("agnese"))
			.Scelta(TEXT("Come va il mulino?"), TEXT("mulino"))
			.Scelta(TEXT("Passavo soltanto."), TEXT("congedo"));

		D.Nodo(TEXT("voci"), {
			TEXT("(Teodoro si china verso di te, e la voce scende di un tono solo, cioe' la sentono comunque fino alla piazza.)"),
			TEXT("{voci}"),
			TEXT("Ma non dire che te l'ho detto io. Di' che te l'ha detto Agnese: tanto ci crederanno lo stesso."),
			})
			.Scelta(TEXT("E Agnese, che ne pensa?"), TEXT("agnese"))
			.Scelta(TEXT("Grazie delle notizie."), TEXT("congedo"));

		D.Nodo(TEXT("agnese"), {
			TEXT("Agnese e' mia moglie da ventidue anni, e da ventidue anni ha ragione lei. Anche quando ho ragione io. Specie allora."),
			TEXT("Lei dice che io parlo troppo. Io dico che lei ascolta troppo. Insieme sappiamo tutto quello che succede tra il passo e la palude. Il Balivo ha i suoi rapporti; noi abbiamo il pozzo, il forno e la locanda."),
			})
			.Scelta(TEXT("E il grano? Basta per l'inverno?"), TEXT("grano"))
			.Scelta(TEXT("Salutamela."), TEXT("congedo"));

		D.Nodo(TEXT("mulino"), {
			TEXT("Gira! Gira sempre. Finche' c'e' acqua nel torrente e grano nei campi, Teodoro macina. Il pane di Valdorso e' fatto con la mia farina, e il pane di Valdorso e' il migliore del regno. Chiedi a chiunque. No, non al fornaio: e' di parte."),
			TEXT("La sera, pero', il mulino si ferma e Teodoro va alla locanda. Il mio sgabello mi aspetta. Se lo trovo occupato, mi offendo."),
			})
			.Scelta(TEXT("E il grano? Basta per l'inverno?"), TEXT("grano"))
			.Scelta(TEXT("Lo terro' a mente."), TEXT("congedo"));

		D.Nodo(TEXT("grano"), {
			TEXT("(Per la prima volta, Teodoro esita. Si spolvera le mani sui calzoni, a lungo.)"),
			TEXT("Basta, basta. Quest'anno basta. E' il prossimo che... be', chi lo sa il prossimo. Con il passo chiuso non sale niente da Aurelia."),
			TEXT("Ma non pensarci tu, che sei nuovo. Ci pensa Agnese. Agnese pensa a tutto."),
			})
			// (11/11, v0.2.0) il grano nascosto (ValdorsoStorie.h, ValdorsoScorte.h): il server guarda il granaio
			.Scelta(TEXT("Il granaio e' quasi vuoto, Teodoro. Lo so che sai qualcosa."), TEXT("grano_nascosto"), FName(ValdorsoStorie::EventoGrano))
			.Scelta(TEXT("E se il grano finisse davvero?"), TEXT("grano_ok"))
			.Scelta(TEXT("Va bene. Ciao, Teodoro."), TEXT("congedo"));

		D.Nodo(TEXT("grano_nascosto"), {
			TEXT("(Teodoro si guarda intorno. Per una volta, non parla. Poi ti prende per un braccio e ti porta dietro il mulino, dove c'e' una botola sotto la paglia.)"),
			TEXT("Quaranta sacchi. Li abbiamo messi via due inverni fa, quando la Corona ha chiuso il passo e Agnese ha detto: verra' il giorno. E' venuto."),
			TEXT("Non e' rubare, eh. E' tenere da parte. Li porto al granaio stanotte, cosi' nessuno fa domande. Tu non l'hai visto, e io non te l'ho detto. E Agnese... Agnese lo sapra' comunque."),
			}).Segno(TEXT("GranoNascosto"))
			.Scelta(TEXT("Hai fatto bene, Teodoro."), TEXT("congedo"));

		D.Nodo(TEXT("grano_ok"), {
			TEXT("Finire? Il grano? (Ride, ma guarda verso il magazzino.) Il granaio e' pieno abbastanza, per ora. Quando sara' mezzo vuoto, ne riparliamo."),
			TEXT("Intanto porta legna al magazzino, che d'inverno la valle ne brucia il doppio."),
			})
			.Scelta(TEXT("E tu, non hai niente da parte?"), TEXT("grano_gia"))
			.Scelta(TEXT("Va bene."), TEXT("congedo"));

		D.Nodo(TEXT("grano_gia"), {
			TEXT("(Teodoro allarga le braccia.) Da parte? Quello che c'era sotto il mulino, se c'era, e' gia' andato al granaio quando serviva, o ci andra' quando servira'. Non ce n'e' altro. Adesso tocca alla valle: legna, carovane, raccolti."),
			})
			.Scelta(TEXT("Capisco."), TEXT("congedo"));

		D.Nodo(TEXT("congedo"), {
			TEXT("(Teodoro ti da' una pacca sulla spalla, e la tua spalla diventa bianca.) Torna! Domani ne so altre."),
			}).Fine();

		return D;
	}

	FValdorsoDialogo DialoghiAgnese()
	{
		FValdorsoDialogo D;
		D.Id = TEXT("Agnese");
		D.NomeBreve = TEXT("Agnese");
		D.Titolo = TEXT("Agnese, la mugnaia");
		D.Inizio = TEXT("primo");
		D.Ritorno = TEXT("ritorno");

		D.Nodo(TEXT("primo"), {
			TEXT("(Una donna minuta, le maniche rimboccate e un fazzoletto in testa, setaccia la farina con colpi secchi. Ti guarda dalla testa ai piedi, e poi di nuovo, piu' lentamente.)"),
			TEXT("Agnese. La moglie di Teodoro, che ti avra' gia' raccontato che ho sempre ragione io. E' vero."),
			TEXT("Tu sei quello arrivato col carro dell'altra settimana. O quella. Al pozzo non si sono messi d'accordo. Sistemiamo subito: come ti chiami, da dove vieni, e sei sposato?"),
			})
			.Scelta(TEXT("Che si dice al pozzo?"), TEXT("voci"))
			.Scelta(TEXT("Teodoro parla molto di te."), TEXT("teodoro"))
			.Scelta(TEXT("Preferisco non dirlo."), TEXT("segreto"))
			.Scelta(TEXT("Scusate, devo andare."), TEXT("congedo"));

		D.Nodo(TEXT("ritorno"), {
			TEXT("(Agnese batte il setaccio sul bordo della madia.)"),
			TEXT("Ah, sei tu. Ho saputo tutto di te, sai? Be', quasi tutto. Il resto me lo dici adesso."),
			})
			.Scelta(TEXT("Che si dice al pozzo?"), TEXT("voci"))
			.Scelta(TEXT("Dov'e' Teodoro?"), TEXT("teodoro"))
			.Scelta(TEXT("Hai qualcosa per me?"), TEXT("frittelle"))
			.Scelta(TEXT("Passavo a salutare."), TEXT("congedo"));

		D.Nodo(TEXT("voci"), {
			TEXT("(Agnese posa il setaccio. Questo, a quanto pare, merita entrambe le mani.)"),
			TEXT("{voci}"),
			TEXT("E non ascoltare Teodoro, che le cose le sente a meta' e le racconta doppie."),
			})
			.Scelta(TEXT("E Teodoro, dov'e' adesso?"), TEXT("teodoro"))
			.Scelta(TEXT("Grazie, Agnese."), TEXT("congedo"));

		D.Nodo(TEXT("teodoro"), {
			TEXT("Di giorno al mulino, a parlare con la ruota. La sera alla locanda, sul suo sgabello, a parlare con chiunque altro. Di notte russa. Ventidue anni."),
			TEXT("(Ma lo dice sorridendo, e il setaccio, per un momento, va piu' piano.) E' un brav'uomo. Non dirglielo, che poi se ne approfitta."),
			})
			.Scelta(TEXT("Il vostro segreto e' al sicuro."), TEXT("congedo"));

		D.Nodo(TEXT("segreto"), {
			TEXT("Ah. Uno di quelli. Bene, bene."),
			TEXT("(Ti guarda con un'attenzione nuova, come si guarda un pacco ben legato.) Tanto lo scopro lo stesso. Lo scopro sempre."),
			})
			.Scelta(TEXT("Che si dice al pozzo, allora?"), TEXT("voci"))
			.Scelta(TEXT("Vedremo. Buona giornata."), TEXT("congedo"));

		D.Nodo(TEXT("frittelle"), {
			TEXT("La domenica faccio le frittelle col miele. Chi porta legna al mulino ne ha due. Chi non porta niente ne ha il profumo."),
			TEXT("E se vedi Tobia, digli che le frittelle non spariscono da sole dalla finestra: lo so che e' lui."),
			})
			.Scelta(TEXT("Glielo diro'."), TEXT("congedo"));

		D.Nodo(TEXT("congedo"), {
			TEXT("(Agnese ha gia' ripreso a setacciare.) Vai, vai. E se senti qualcosa, sai dove trovarmi."),
			}).Fine();

		return D;
	}

	FValdorsoDialogo DialoghiElric()
	{
		FValdorsoDialogo D;
		D.Id = TEXT("Elric");
		D.NomeBreve = TEXT("Maestro Elric");
		D.Titolo = TEXT("Maestro Elric, mago di corte");
		D.Inizio = TEXT("primo");
		D.Ritorno = TEXT("ritorno");

		D.Nodo(TEXT("primo"), {
			TEXT("(Un uomo alto, sulla quarantina, la veste blu del collegio sporca di terra fino al ginocchio, copia dei segni da una pietra delle rovine su un quaderno rilegato in pelle. Non si volta.)"),
			TEXT("Se sei del villaggio, non ho bisogno di uova, di guide, ne' di storie sulla vecchia della palude. Se sei un colono con il richiamo, aspetta che finisca la riga."),
			TEXT("(Finisce la riga.) Maestro Elric, del collegio dei maghi di corte, mandato dall'Arcimago in persona. Il battito del vostro frammento si e' fermato, e qualcuno a corte vuole sapere perche'. Quel qualcuno sono io."),
			})
			.Scelta(TEXT("Cosa stai studiando?"), TEXT("rune"))
			.Scelta(TEXT("Perche' il battito si e' fermato?"), TEXT("battito"))
			.Scelta(TEXT("Nonna Edda dice che-"), TEXT("edda"))
			.Scelta(TEXT("Non ti disturbo, Maestro."), TEXT("congedo"));

		D.Nodo(TEXT("ritorno"), {
			TEXT("(Elric alza un dito, senza staccare gli occhi dal quaderno. Aspetti. Poi lo abbassa.)"),
			TEXT("Tu. Hai qualcosa di intelligente da dirmi, o devo tornare alle mie pietre?"),
			})
			.Scelta(TEXT("Come vanno le ricerche?"), TEXT("rune"))
			.Scelta(TEXT("Ho visto iscrizioni dei nani, sotto il nord."), TEXT("nani"))
			.Scelta(TEXT("Hai paura di sbagliare, vero?"), TEXT("paura"))
			.Scelta(TEXT("Niente, Maestro."), TEXT("congedo"));

		D.Nodo(TEXT("rune"), {
			TEXT("Rune degli Antichi. Non quelle dei nani, che sono conti e confini: queste sono parole. Alcune le leggo, altre le indovino, la maggior parte le ignoro, come tutti. La differenza e' che io lo so."),
			TEXT("Vedi questa? (Una linea spezzata, con un punto nel mezzo.) Si e' accesa, l'altra notte. Una luce fioca, per il tempo di tre battiti. Nessuno l'ha toccata. Da trecento anni nessuna runa si accendeva da sola."),
			})
			.Scelta(TEXT("Cosa vuol dire?"), TEXT("significa"))
			.Scelta(TEXT("Ne ho viste anche sotto la montagna del nord."), TEXT("nani"))
			.Scelta(TEXT("Affascinante. Ti lascio lavorare."), TEXT("congedo"));

		D.Nodo(TEXT("significa"), {
			TEXT("Vuol dire che qualcosa sotto questa valle si e' svegliato, o sta per farlo. Oppure che mi sono sbagliato, e la pietra rifletteva una lanterna."),
			TEXT("(Chiude il quaderno di scatto.) Non mi sono sbagliato. Al collegio diranno di si'. Al collegio dicono sempre di si', quando non sono loro a camminare nel fango."),
			})
			.Scelta(TEXT("E il battito del frammento?"), TEXT("battito"))
			.Scelta(TEXT("Buona fortuna, Maestro."), TEXT("congedo"));

		D.Nodo(TEXT("battito"), {
			TEXT("Il vostro sacerdote crede che sia un segno degli dei. La vecchia della palude crede che sia l'Orso. I coloni credono che porti sfortuna. Io credo che sia un fenomeno, e i fenomeni si misurano."),
			TEXT("Ho portato da Aurelia due pendoli e una clessidra d'argento. Quando il battito si ferma ancora, lo saprò al granello. E allora qualcuno al collegio dovra' ascoltarmi."),
			})
			.Scelta(TEXT("E se non fosse solo qui?"), TEXT("ovunque"))
			.Scelta(TEXT("Spero che tu abbia ragione."), TEXT("congedo"));

		D.Nodo(TEXT("ovunque"), {
			TEXT("(Elric ti guarda, e per un istante non sembra arrogante. Sembra stanco.)"),
			TEXT("Allora non sarebbe un problema di Valdorso. Sarebbe un problema del regno. E nessuno vuole essere quello che porta al re una notizia cosi', con un quaderno di appunti presi nel fango."),
			TEXT("(Riapre il quaderno.) Domanda intelligente. Rara. Torna, se ne hai altre."),
			})
			.Segno(TEXT("DomandaIntelligente"))
			.Scelta(TEXT("Tornero'."), TEXT("congedo"));

		D.Nodo(TEXT("edda"), {
			TEXT("No. Ti fermo qui. Qualunque cosa dica la vecchia della palude, e' un'erba bollita con una filastrocca sopra."),
			TEXT("(Pausa.) Anche se, devo ammetterlo, le sue erbe per la febbre funzionano. Questo non lo scrivo nel rapporto. E nemmeno tu lo racconterai."),
			})
			.Scelta(TEXT("Cosa stai studiando, allora?"), TEXT("rune"))
			.Scelta(TEXT("Il tuo segreto e' al sicuro."), TEXT("congedo"));

		D.Nodo(TEXT("nani"), {
			TEXT("(Il quaderno si chiude a meta' di una parola.)"),
			TEXT("Sotto il nord? Dietro la frana? Non me l'aveva detto nessuno. Naturalmente: qui nessuno dice niente a chi viene dalla capitale."),
			TEXT("Se trovi delle iscrizioni, copiale. Esatte, segno per segno: non come le ricordi, come sono. Quello che i nani hanno scritto sui muri, di solito, e' quello che volevano che non si dimenticasse."),
			})
			.Scelta(TEXT("Lo faro'."), TEXT("congedo"));

		D.Nodo(TEXT("paura"), {
			TEXT("(Elric posa la penna. Lentamente.)"),
			TEXT("Paura? Io ho studiato vent'anni al collegio, sono il piu' giovane maestro della mia generazione, e l'Arcimago in persona mi ha scelto per questo incarico."),
			TEXT("...Il che vuol dire che se sbaglio, sbaglio davanti a tutti. Si'. Contento? Adesso lasciami lavorare."),
			})
			.Scelta(TEXT("Non sbaglierai."), TEXT("congedo"));

		D.Nodo(TEXT("congedo"), {
			TEXT("(Elric e' gia' chino sulla pietra, a contare i segni con la punta della penna.)"),
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
		// (10/11)
		Tutti.Add(DialoghiLucia());
		Tutti.Add(DialoghiCapoGilda());
		// (11/11) volti e giornate
		Tutti.Add(DialoghiMarta());
		Tutti.Add(DialoghiLivia());
		Tutti.Add(DialoghiTobia());
		Tutti.Add(DialoghiTeodoro());
		Tutti.Add(DialoghiAgnese());
		Tutti.Add(DialoghiElric());
		return Tutti;
	}

	// --- (10/11) I saluti a chi passa vicino (una riga nella chat, ogni tanto): di giorno, la sera, di notte ---------
	struct FSalutoScritto
	{
		const TCHAR* Id;
		/** 0 giorno (6-17), 1 sera (17-21), 2 notte (21-6). */
		int32 Momento;
		const TCHAR* Testo;
	};

	const FSalutoScritto SalutiScritti[] = {
		{ TEXT("Gaspare"), 0, TEXT("Cammina piano, che il bosco ti sente da un miglio.") },
		{ TEXT("Gaspare"), 0, TEXT("Tracce fresche, verso est. Cinghiali. Lasciali stare, se non hai fame.") },
		{ TEXT("Gaspare"), 1, TEXT("Al tramonto i lupi scendono. Torna al villaggio, colono.") },
		{ TEXT("Gaspare"), 2, TEXT("Tu, a quest'ora, nel bosco? O sei matto o sei come me.") },
		{ TEXT("GuardiaCorrado"), 0, TEXT("Circolare. Il passo e' chiuso, come ieri e come domani.") },
		{ TEXT("GuardiaCorrado"), 1, TEXT("Fra poco si chiude la porta. Chi e' dentro e' dentro.") },
		{ TEXT("GuardiaCorrado"), 2, TEXT("Alla larga dal sigillo, di notte. Ordini della Corona.") },
		{ TEXT("GuardiaOttone"), 0, TEXT("Buongiorno! Cioe'... in nome della Corona, buongiorno.") },
		{ TEXT("GuardiaOttone"), 1, TEXT("Hai visto anche tu il velo tremare? No? Meglio cosi'.") },
		{ TEXT("GuardiaOttone"), 2, TEXT("(sbadiglia) Chi va la'? Ah, un colono. Vai, vai.") },
		{ TEXT("NonnaEdda"), 0, TEXT("Le erbe si colgono col sole alto, e si ringraziano.") },
		{ TEXT("NonnaEdda"), 1, TEXT("L'acqua del lago e' scura, stasera. Non farci il bagno.") },
		{ TEXT("NonnaEdda"), 2, TEXT("Nir... Ren... Ka... (canticchia piano, senza guardarti)") },
		{ TEXT("Bruno"), 0, TEXT("Ehi, colono! La zuppa e' calda e le chiacchiere sono gratis.") },
		{ TEXT("Bruno"), 0, TEXT("Hai una faccia da stufato, oggi. Entra, entra.") },
		{ TEXT("Bruno"), 1, TEXT("Sera! I boccali sono puliti, piu' o meno.") },
		{ TEXT("Bruno"), 2, TEXT("A quest'ora la locanda e' piena di storie. Vere, alcune.") },
		{ TEXT("PadreAnselmo"), 0, TEXT("Che il Cuore batta per te, figliolo.") },
		{ TEXT("PadreAnselmo"), 1, TEXT("Al vespro la pietra si sente meglio. Ascolta.") },
		{ TEXT("PadreAnselmo"), 2, TEXT("Due colpi e una pausa. Ancora. Sia lodato.") },
		{ TEXT("Lucia"), 0, TEXT("Ci sono fogli nuovi sulla bacheca. Non stanno li' per bellezza.") },
		{ TEXT("Lucia"), 1, TEXT("Chiudo il registro fra poco. Se hai un foglio, adesso.") },
		{ TEXT("Lucia"), 2, TEXT("(scrive ancora, a lume di candela) Torna domattina.") },
		{ TEXT("CapoGilda"), 0, TEXT("Lavora bene e Lucia avra' da scrivere. E io da firmare.") },
		{ TEXT("CapoGilda"), 1, TEXT("Una giornata onesta? Bene. Domani un'altra.") },
		{ TEXT("CapoGilda"), 2, TEXT("Anche la Gilda dorme, colono. Quasi.") },
		{ TEXT("Marta"), 0, TEXT("Le scintille non chiedono permesso. Stai indietro.") },
		{ TEXT("Marta"), 1, TEXT("La forgia si spegne al tramonto. Se hai fretta, avevi fretta stamattina.") },
		{ TEXT("Marta"), 2, TEXT("(non risponde: dorme, o fa finta)") },
		{ TEXT("Livia"), 0, TEXT("Circolare. E tieni le mani dove le vedo.") },
		{ TEXT("Livia"), 1, TEXT("Una birra da Bruno e poi di nuovo al passo. Non ditelo al Balivo.") },
		{ TEXT("Livia"), 2, TEXT("Di notte al passo non si viene. Lo dico una volta.") },
		{ TEXT("Tobia"), 0, TEXT("Ciao! Hai visto un lupo oggi? No? Peccato.") },
		{ TEXT("Tobia"), 1, TEXT("Corro, che se non arrivo per il vespro Padre Anselmo mi fa pulire le panche!") },
		{ TEXT("Tobia"), 2, TEXT("(sbadiglia) Stanotte sogno l'orso, me lo sento.") },
		{ TEXT("Teodoro"), 0, TEXT("Ehi, colono! Ne ho una nuova: vieni che te la racconto!") },
		{ TEXT("Teodoro"), 1, TEXT("Il mio sgabello mi aspetta. Chi tocca lo sgabello tocca Teodoro.") },
		{ TEXT("Teodoro"), 2, TEXT("(canta, stonato) ...e la mugnaia ride, ride, e il mulino gira...") },
		{ TEXT("Agnese"), 0, TEXT("Hai una faccia da notizia. Avanti, dimmi.") },
		{ TEXT("Agnese"), 1, TEXT("Hai visto Teodoro? Si', alla locanda. Lo sapevo.") },
		{ TEXT("Agnese"), 2, TEXT("A quest'ora? Brutte cose, o belle storie. Domattina mi racconti.") },
		{ TEXT("Elric"), 0, TEXT("Non adesso. Sto contando.") },
		{ TEXT("Elric"), 1, TEXT("La luce cala. Le rune si leggono meglio al tramonto, sai? No, non lo sai.") },
		{ TEXT("Elric"), 2, TEXT("Hai visto una luce sulle pietre, stanotte? No? Allora non disturbarmi.") },
	};

}
