====================
Freeciv Versie 1.13
====================

Welkom bij Freeciv!

Dit archief bevat Freeciv, een vrije Civilization kloon, primair voor
X onder Unix. Het heeft ondersteuning voor multi-speler spellen lokaal
of over een netwerk en een KI die de meeste mensen een de stuipen op het
lijf jaagt.

Freeciv probeert overwegend spelregel-uitwisselbaar te zijn met 
Civilization II [tm], gepubliceerd door Sid Meier en Microprose [tm].
Een aantal regels zijn anders waar we denken dat dat zinvol is en we
hebben heel veel instelbare parameters om het customizen van spelen
mogelijk te maken.

Freeciv is volledig onafhankelijk van Civilization gebouwd; u hebt
Civilization niet nodig om Freeciv te kunnen spelen.

Hoewel de computerspelers (KI's) nog niet kunnen onderhandelen, hebben
we inmiddels geluidsondersteuning, zeer volledige spelregels en is onze
multispeler netwerkcode uitstekend.


Web site:
=========

Freeciv's web site is hier:

  http://www.freeciv.org/

Wij nodigen u uit deze te bezoeken. U kunt hier het laatste Freeciv nieuws,
nieuwe versies en patches vinden, meer te weten komen over de mailinglijsten
van Freeciv en de Freeciv Metaserver zien, die spelen bijhoudt die over de
hele wereld gespeeld worden.


Licentie:
=========

Freeciv wordt uitgebracht onder de GNU General Public License. In het kort
mag u dit programma onbeperkt kopiëren (inclusief source), maar kijk naar
het bestand COPYING voor volledige details.


Compileren and installatie:
===========================

Lees alstublieft het bestand INSTALL zorgvuldig voor instructies hoe
u Freeciv gecompileerd en geïnstalleerd krijgt op uw machine.


Een nieuw spel beginnen:
========================

Freeciv is eigenlijk twee programma's, een server en een client. Wanneer
een spel loopt, dan zal er één serverprogramma draaien en zoveel clients
als er menselijke spelers zijn. De server heeft geen X nodig, maar de
clients wel.

  OPMERKING:
  De volgende voorbeelden gaan ervan uit dat Freeciv op uw systeem 
  geïnstalleerd is en dat de map die "civclient" en "civserver" bevat
  zich in uw PATH bevindt. Als Freeciv niet geïnstalleerd is, dan wilt
  u wellicht de "civ" en "ser" programma's gebruiken. Deze kunt u vinden
  in de top van de Freeciv map. Ze worden op exact dezelfde wijze
  gebruikt als "civclient" en "civserver".

Het draaien van Freeciv omvat het starten van de server, daana de client(s)
en KI(s), vervolgens het geven van de opdracht aan de server om het spel
te starten. Hier volgen de stappen:

Server:

  Om de server te starten:

  |  % civserver

  Of voor een lijst van opdrachtregel-opties:

  |  % civserver --help

  Zodra de server eenmaal gestart is verschijnt een prompt:

  |  Voor een introductie, type 'help'.
  |  >

  en u kunt de volgende informatie zien door de opdracht help te
  gebruiken:

  |  > help
  |  Welkom - dit is de introduktie helptekst voor de Freeciv server.
  |
  |  Twee belangrijke server concepten zijn Opdrachten en Opties.
  |  Opdrachten, zoals 'help', worden gebruikt om met de server te
  |  communiceren.  Sommige opdrachten verwachten één of meer argumenten,
  |  gescheiden door spaties.
  |  In veel gevallen kunnen opdrachten en argumenten worden ingekort.
  |  Opties zijn instellingen die de server sturen terwijl hij draait.
  |
  |  Om te leren hoe u meer informatie over opdrachten en opties kunt
  |  krijgen, gebruik 'help help'.
  |
  |  Voor de ongeduldigen zijn de belangrijkste opdrachten om aan de
  |  gang te kunnen gaan:
  |    show   - om de huidige instellingen te bekijken
  |    set    - om de instellingen te wijzigen
  |    start  - om het spel te starten zodra alle spelers verbinding hebben
  |    save   - om het huidige spel te bewaren
  |    quit   - om de server te verlaten
  |  >

  Als u wilt kunt u de opdracht 'set' gebruiken om elk van de diverse
  server-opties voor het spel te wijzigen. U kunt een lijst van de opties
  krijgen met de opdracht 'show' en gedetailleerde beschrijvingen van elk
  met de opdracht 'help <optienaam>'.

  Bijvoorbeeld:

  |  > help xsize
  |  Optie: xsize  -  Kaart-breedte in vlakken
  |  Status: veranderbaar
  |  Waarde: 80, Minimaal: 40, Standaard: 80, Maximaal: 200
  |  >

  And:

  |  > set xsize 100
  |  > set ysize 80

  Dit maakt de kaart bijna tweemaal zo groot als de standaard van 80x50.

Client:

  Nu moeten alle menselijke spelers meedoen door de Freeciv client
  op te starten:

  |  % civclient

  Dit gaat er vanuit dat de server draait op dezelfde machine. Indien
  niet, dan kunt de servernaam opgeven met hetzij met een optie '--server'
  op de opdrachtregel of door het intypen in het eerste dialoogvenster
  zodra de client gestart is.

  Bijvoorbeeld, aangenomen dat de server draait op een andere machine met
  de naam 'neptune'. Dan zouden spelers meedoen met een opdracht als:

  |  % civclient --server neptune

  Als u de enige menselijke speler bent, dan is slechts één client
  nodig. In standaard Unix-stijl kunt u de client "in de achtergrond"
  starten door toevoeging van een '&'-teken:

  |  % civclient &

  Een ander optie voor de client die u wellicht wilt proberen is de 
  '--tiles' optie, die gebruikt wordt voor het kiezen van een andere
  "vlakkenset" (dat is een verzameling andere afbeeldingen voor kaart,
  terrein, eenheden, enz.). De distributie bevat twee hoofdsets:
  - isotrident: Een isometrische vlakkenset gelijkend op die van civ 2.
    (nog niet voor de Xaw client)
  - trident: een civ1-stijl vlakkenset met 30x30 afbeeldingen.
    De trident vlakkenset heeft een variant met de naam "trident_shields".

  In deze uitgave is de isotrident vlakkenset de standaard voor de gtk-,
  amiga- en win32 clients, terwijl de xaw clients trident als standaard
  heeft. De "_shields" variant gebruikt een schild-vormige vlag, die 
  kleiner is en wellicht minder bedekt. Probeer ze beide en beslis voor
  uzelf. Om de trident vlakkenset te gebruiken start u de client met:

  |  % civclient --tiles trident

  Andere vlakkensets (tilesets in het Engels) zijn beschikbaar van de 
  ftp- en websites.

  Clients kunnen geautoriseerd worden om serveropdrachten te geven.
  Om alleen informatieve opdrachten te geven, type aan de serverprompt

  |  > cmdlevel info

  Clients kunnen nu '/help', '/list', '/show settlers', enz. gebruiken.

Computerspelers:

  Er zijn twee manieren om KI-spelers te maken. De eerste is om
  het aantal spelers (menselijk én KI) in te stellen met de 'aifill'
  server-opdracht. Bijvoorbeeld:

  |  > set aifill 7

  Na het gebruik van de server-opdracht 'start' zullen alle spelers die
  niet door gebruikers worden bestuurd KI-spelers zijn. Voor het boven-
  staande voorbeeld zouden in het geval van twee menselijke spelers vijf
  KI-spelers worden gemaakt.

  De tweede manier is om expliciet een KI te maken met de server-opdracht
  'create'. Bijvoorbeeld:

  |  > create MensenMoordenaar

  Dit zal een KI-bestuurde speler met de naam MensenMoordenaar maken.

  KI-spelers worden toegekend aan naties nadat alle menselijke spelers
  een natie gekozen hebben, maar u kunt een specifieke natie voor een
  KI-speler kiezen door het gebruik van de normale naam voor die natie's
  leider. Om bijvoorbeeld tegen KI-bestuurde Romeinen te spelen dient u
  de volgende opdracht te typen (op de serverprompt):

  |  > create Caesar

  Let op, dit is slechts een voorkeur: als geen andere menselijke speler
  de Romeinen als natie kiest, zal deze KI dat doen.

Server:

  Wanneer iedereen verbinding gemaakt heeft (gebruik de opdracht 'list'
  om te zien wie er binnen zit), start dan het spel met de opdracht
  'start':

  |  > start

En het spel is begonnen!


Aankondigen van het spel:
=========================

Als u uw opponenten niet wilt beperken tot lokale vrienden of KI-spelers,
bezoek dan de Freeciv metaserver:

  http://meta.freeciv.org/metaserver/

Het is een lijst van Freeciv servers. Om uw eigen server zich daar te laten
aankondigen, start civserver met de '--meta' optie, of gewoon '-m'.

Aandachtspunten:

 1) Als gevolg van nieuwe mogelijkheden zijn verschillende client- en
    server-versies vaak niet uitwisselbaar. De 1.13.0 versie is bijv.
    niet uitwisselbaar met 1.12.0 of eerdere versies.

 2) Als de Metaserver knop in de verbindingsdialoog niet werkt, controleer
    dan of uw ISP een verplichte WWW-proxy heeft en laat civclient deze
    gebruiken door de $http_proxy omgevingsvariabele. Als bijvoorbeeld de
    proxy proxy.myisp.com is, poort 8888, zet dan $http_proxy op
    http://proxy.myisp.com:8888/ voordat u de client start.

 3) Soms zijn er geen spelen op de metaserver. Dat gebeurt. Het aantal
    spelers daar varieert in de loop van een dag. Probeer zelf een spel
    te starten!


Spelen van het spel:
====================

Het spel kan op ieder gewenst moment worden opgeslagen met de server-
opdracht 'save' aldus:

  |  > save mijnspel.sav

(Als uw server is gecompileerd met compressie-ondersteuning en de
'compress' serveroptie heeft een andere waarde dan 0 (nul), dan
wordt het bestand weggeschreven in een gecomprimeerd formaat met
de naam 'mijnspel.sav.gz'.)

De Freeciv client werkt behoorlijk zoals u zou mogen verwachten van
een multispeler civilization spel. Tenminste, de menselijke spelers
bewegen allemaal tegelijk, de KI-spelers bewegen zodra alle menselijke
spelers hun zet hebben voltooid. Er is een beurt-tijdslimiet die
standaard op 0 seconden staat (geen limiet). De serverbeheerder kan
deze waarde wijzigen op elk gewenst moment met de 'set' opdracht.

Kijk eens naar het online helpsysteem. Alle drie de muisknoppen worden
gebruikt en staan gedocumenteerd in de helpsysteem.

Spelers kunnen de 'Enter'-toets gebruiken om het einde van hun zet te
markeren of kunnen klikken op de 'Beurt klaar' knop.

Gebruik de 'Spelers'-dialoog om te zien wie al klaar is met zijn beurt
en op wie u zit te wachten. (Hé vriend, zit je te slapen of zo?? ;).

Gebruik de invoerregel aan de onderkant van het venster voor het sturen van
berichten naar andere spelers.

U kunt een persoonlijk bericht sturen (aan bijv. 'peter') als volgt:

  |  peter: Haal die tank *NU* weg!

De server is slim genoeg om 'name completering' toe te passen, dus als
u 'pet:' had gebruikt dan had hij de spelersnaam gezocht met hetzelfde
begin als wat u gebruikt had.

U kunt ook serveropdrachten versturen vanaf de invoerregel:

  |  /list
  |  /set settlers 4
  |  /save mijnspel.sav

Waarschijnlijk beperkt de serveroperator uw opdrachten tot alleen 
informatieve opdrachten. Dit is voornamelijk omdat er nogal wat
beveiligingsaspecten van toepassing zijn indien alle clients alle
serveropdrachten zouden kunnen geven; bedenk als een speler probeerde:

  |  /save /etc/passwd

Uiteraard zou de server niet met root-privileges mogen draaien in welk
geval dan ook, juist om dit soort risico's te beperken.

Als u net begint en u wilt een idee krijgen van de strategie, kijk dan
in de Freeciv HOWTO, in het bestand HOWTOPLAY.

Voor heel veel meer informatie over de client, de server en de concepten
en regels van het spel, bekijk de Freeciv handleiding, beschikbaar op de
webpagina's op:

  http://www.freeciv.org/manual/


Beëindigen van het spel:
========================

Er zijn drie manieren waarop een spel kan eindigen:

1) Slechts één natie is overgebleven
2) Het eindjaar is bereikt
3) Een speler bouwt en lanceert een ruimteschip dat vervolgens 
   Alfa Centauri bereikt.

Een score-tabel zal in elk van de gevallen getoond worden. De server-
beheerder kan het eindjaar instellen terwijl het spel al aan de gang
is door de 'end-year' optie te wijzigen. Dit is leuk als de winnaar
duidelijk is, maar u wilt vast niet door de saaie 'opruimingsfase'
spelen.


Herstellen van spellen:
======================

U kunt een opgeslagen spel herstellen c.q. laden met de -f serveroptie:

  |  % civserver -f onsbewaard2002.sav

of, als het spel is gemaakt door een server met compressie:

  |  % civserver -f onsbewaard2002.sav.gz

Nu kunnen de spelers opnieuw verbinding maken met de server:

  |  % civclient -n Alexander

Merk op hoe de spelersnaam is opgegeven met de -n optie. Het is van
groot belang dat de speler dezelfde naam gebruikt die hij/zij had
toen het spel draaide, willen ze toestemming krijgen om mee te doen.

Het spel kan dan voortgezet worden door gebruik te maken van de 'start'-
opdracht als gebruikelijk.


Nationale Taal Ondersteuning:
=============================

Freeciv ondersteunt diverse nationale talen.

U kunt kiezen welke lokale taal gebruikt wordt door het opgeven van
een 'locale'. Elke locale heeft een standaard naam (bijv. 'de' voor
Duits). Als u Freeciv geïnstalleerd hebt kunt u een locale kiezen door
de omgevingsvariabele 'LANG' in te stellen op die standaard naam
vóórdat u civserver en civclient opstart.

Bijvoorbeeld, aangenomen dat u de Nederlandse locale wilt gebruiken
doet u:

  export LANG; LANG=nl   (in the Bourne shell (sh)),
of
  setenv LANG nl         (in the C shell (csh)).

(U kunt dit in uw .profile of .login bestand zetten.)


Logberichten:
=====================

Zowel de client ald de server geven berichten die bekend staan als
"logberichten". Er zijn vijf categoriëen van logberichten: "fataal",
"fout", "normaal", "uitgebreid" and "debug".

Standaard worden fataal, fout en normaal-berichten afgedrukt op standaard
uitvoer van de server-start. U kunt logberichten naar een bestand omleiden
met de '--log bestandsnaam' of '-l bestandsnaam' opdrachtregel-opties te
gebruiken.

U kunt het niveau van de logberichten die getoond wordt met
'--debug niveau' of '-d niveau' (of '-de niveau' voor de xaw client,
aangezien '-d' zowel '-debug' als '-display' zou kunnen zijn), waar
niveau de waarde 0, 1, 2 of 3 is. 0 betekent alleen fatale berichten,
1 fatale- en foutberichten, 2 fatale-, fout- en normale berichten (de
standaardwaarde), 3 betekent toon alle fatale-, fout-, normale- en 
uitgebreide berichten.

Als u gecompileerd heeft met DEBUG gedefinieerd (een makkelijke manier
om dit te doen is door te configureren met de optie --enable-debug),
dan kunt u debug-berichten krijgen door het niveau op 4 te zetten.
Ook is het mogelijk om debug-berichten (maar niet andere-) te beheersen 
op een per-bestand en per-regel basis. Om dit te doen gebruik 
"--debug 4:str1:str2" (zoveel strings als u wilt, gescheiden door :'en)
en elke bestandsnaam die overeenkomt met zal debug-berichten gaan
genereren terwijl alle andere debug-berichten onderdrukt worden. Om 
regels te beheersen gebruik "--debug 4:str1,min,max" en voor bestanden
die in naam overeenkomen met str1 zullen van regel min tot regel max
debug-berichten genereren, terwijl alle andere debug-berichten onderdrukt
worden. Slechts één set van (min,max) kan per bestand worden toegepast.

Voorbeeld:

  |  % civserver -l mijn.log -d 3

Dit stuurt alle server logberichten naar bestand "mijn.log", inclusief
berichten van de 'uitgebreid' categorie.

Voorbeeld:

  |  % civclient --debug 0

Dit onderdrukt alle niet-fatale logberichten.

Voorbeeld:

  |  % civserver -d 4:log:civserver,120,500:autoattack

Dit zet alle fatale-, fout-, normale- en uitgebreide berichten aan
voor de server en debug-klasse berichten voor enkele specifieke modules.
Let op dat 'log' zal overeenkomen met zowel 'gamelog.c' als 'log.c'.
Voor 'civserver.c' zullen debugberichten komen voor akties tussen regel
120 en 500. Dit voorbeeld werkt alleen indien de server is gecompileerd
met DEBUG.


Fouten:
=======

Fout gevonden? We willen dit echt graag van u horen zodat we dat herstellen
kunnen. Bekijk het bestand BUGS voor een lijst met bekende fouten in deze
versie en informatie over het rapporteren van fouten.


Mailing lijsten:
================

Wij onderhouden 7 mailing lijsten:

  freeciv          Algemene discussie
  freeciv-announce Aankondigingen van algemeen belang
                   Dit is een 'Alleen lezen' lijst met infrequente berichten.
                   M.a.w. u kunt niet naar deze lijst sturen, alleen maar
                   ontvangen. (Aankondigingen die hier naar toe gestuurd
                   worden komen ook op freeciv).
  freeciv-dev      Freeciv ontwikkeling.
  freeciv-data     Freeciv data-map ontwikkeling.
                   (Discussie over spelregels, vlakkensets, enz.)
  freeciv-java     Freeciv Java client ontwikkeling.
  freeciv-ai       Freeciv KI ontwikkeling.
  freeciv-cvs      Kennisgevingen van veranderingen in de CVS repository.
                   Dit is een 'Alleen lezen' lijst met uitsluitend
                   automatische berichten. M.a.w. u kunt niets naar deze
                   lijst sturen, u kunt alleen maar ontvangen.

Alle lijsten staan open voor het grote publiek en iedereen is welkom om mee te
doen.

Om mee te doen (of niet meer mee te doen) met deze lijsten volg de volgende
stappen:

  1. Email naar <listar@freeciv.org>.
  2. Laat het onderwerp leeg
  3. In het bericht van de email zet één van:
      Om mee te doen:
        subscribe freeciv
        subscribe freeciv-announce
        subscribe freeciv-dev
        subscribe freeciv-data
        subscribe freeciv-java
        subscribe freeciv-ai
        subscribe freeciv-cvs
      To niet meer mee te doen:
        unsubscribe freeciv
        unsubscribe freeciv-announce
        unsubscribe freeciv-dev
        unsubscribe freeciv-data
        unsubscribe freeciv-java
        unsubscribe freeciv-ai
        unsubscribe freeciv-cvs

Om email naar de lijst te sturen, stuur het naar:

  Voor freeciv, mail moet gestuurd worden naar <freeciv@freeciv.org>.
  Voor freeciv-dev, mail moet gestuurd worden naar <freeciv-dev@freeciv.org>.
  Voor freeciv-data, mail moet gestuurd worden naar <freeciv-data@freeciv.org>.
  Voor freeciv-java, mail moet gestuurd worden naar <freeciv-java@freeciv.org>.
  Voor freeciv-ai, mail moet gestuurd worden naar <freeciv-ai@freeciv.org>.


Internet Relay Chat (IRC)
=========================

Diverse spelers en ontwikkelaars hangen rond op #freeciv op het Open Projects
netwerk. Probeer verbinding te maken met de server

	irc.openprojects.net


Nieuwe versies:
===============

We hopen elke vier maanden een nieuwe release van Freeciv te maken.
Controleer het Freeciv website van tijd tot tijd om te zien of er een nieuwere
versie is!!


Tenslotte:
==========

Heb plezier en geef ze van katoen!!

                                   -- Het Freeciv team.
