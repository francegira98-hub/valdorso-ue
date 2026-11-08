// Valdorso - I testi dei dialoghi (v0.1.3): Gaspare al margine del bosco e le due guardie della Corona al passo.
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
//   il segno). Per personaggio e per PNG, solo in memoria come "chi ha già parlato". Così la moneta si offre una volta.
//   Al massimo 4 risposte per nodo. Una riga "rune: Ur | didascalia" si mostra con le rune disegnate.
// I test (Valdorso.Dialoghi.*) controllano che ogni nodo esista, ci si arrivi e porti a una fine.
//
// Gaspare (dal Design): burbero, solitario, lo prendono per matto. Ha visto l'ombra dell'Orso più di chiunque altro.
// Il suo segreto (trent'anni fa l'Orso gli salvò la vita, e da allora caccia solo ciò che serve) NON si dice: si lascia
// solo intuire. Ama chi rispetta il bosco, detesta chi caccia per divertimento. Lui e Nonna Edda sono gli unici che
// credono all'Orso. Prepara la missione "Dove l'ho vista" e le lezioni sulle tracce ("torna quando avrai imparato a
// stare zitto nel bosco").
// Le guardie (Corrado, il veterano, e Ottone, il giovane): poche parole, leali alla Corona, non si comprano.

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

	/** Corrado, il veterano del posto di guardia: due parole quando bastano due, una quando basta una. */
	FValdorsoDialogo DialoghiCorrado()
	{
		FValdorsoDialogo D;
		D.Id = TEXT("GuardiaCorrado");
		D.NomeBreve = TEXT("Corrado");
		D.Titolo = TEXT("Corrado, guardia della Corona");
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
			TEXT("(Corrado guarda la moneta. Poi guarda te. La moneta resta dov'è.)"),
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
			TEXT("(Corrado fa un cenno con il mento verso il velo, senza voltarsi.)"),
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
			TEXT("(Per la prima volta, Corrado guarda il velo alle sue spalle. Un attimo solo.)"),
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

	/** Ottone, la guardia giovane: viene dalla capitale, è qui da primavera, ha giurato e ci crede. */
	FValdorsoDialogo DialoghiOttone()
	{
		FValdorsoDialogo D;
		D.Id = TEXT("GuardiaOttone");
		D.NomeBreve = TEXT("Ottone");
		D.Titolo = TEXT("Ottone, guardia della Corona");
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
			TEXT("Dalla capitale. Mi hanno mandato su a primavera, con Corrado e con la capitana Livia."),
			TEXT("Corrado dice che al freddo ci si abitua. Corrado parla poco, ma di solito ha ragione."),
			})
			.Scelta(TEXT("Ti manca casa?"), TEXT("casa"))
			.Scelta(TEXT("Altro."), TEXT("altro"));

		D.Nodo(TEXT("casa"), {
			TEXT("(Ottone esita.)"),
			TEXT("Non sta a me dirlo. Sto dove mi mette la Corona."),
			TEXT("Però d'estate, ad Aurelia, ci sono i fichi. Qui crescono solo le more."),
			})
			.Scelta(TEXT("Altro."), TEXT("altro"))
			.Scelta(TEXT("Buona guardia, Ottone."), TEXT("congedo"));

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
			TEXT("(Ottone guarda la moneta, poi te, poi Corrado. Corrado non dice niente. Non serve.)"),
			TEXT("Sei matto? Rimettila in tasca."),
			TEXT("Ho giurato alla Corona. Un giuramento non si vende, né per una moneta né per cento. E il sigillo non lo apro io: non lo apre nessuno, con l'oro."),
			})
			.Scelta(TEXT("Nessuno lo verrebbe a sapere."), TEXT("insiste"), TEXT("Corruzione"))
			.Scelta(TEXT("Hai ragione. Scusa."), TEXT("altro"));

		D.Nodo(TEXT("insiste"), {
			TEXT("Lo saprei io."),
			TEXT("E adesso lo sa anche la capitana: questa la riferisco. Vattene, prima che chiami Corrado."),
			}).Fine();

		D.Nodo(TEXT("congedo"), {
			TEXT("Torna alla valle. E non avvicinarti al velo: fa male."),
			}).Fine();

		return D;
	}

	TArray<FValdorsoDialogo> DialoghiScritti()
	{
		TArray<FValdorsoDialogo> Tutti;
		Tutti.Add(DialoghiGaspare());
		Tutti.Add(DialoghiCorrado());
		Tutti.Add(DialoghiOttone());
		return Tutti;
	}
}
