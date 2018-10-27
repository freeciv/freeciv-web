===================
Freeciv Version 2.2
===================

Välkommen till Freeciv!

Detta arkiv innehåller Freeciv, ett fritt Civilizationsliknande spel,
huvudsakligen för X under Unix. Den har stöd för flerspelarspel lokalt
eller över nätverk, samt utmanande datorstyrda spelare.

Freeciv siktar på att ha regler som huvudsakligen stämmer överens med
Civilisation II [tm] utgivet av Sid Meier och Microprose [tm]. Vissa
regler är annorlunda för att vi tycker att det är bättre så. Det finns
många inställbara parametrar för att anpassa sina spel.

Freeciv har skapats helt oberoende av Civilization; man behöver inte
äga Civilization för att spela Freeciv.

Detta är den svenska översättningen av filen "../README". Eftersom
denna översättning har färre läsare än originalet är det större risk
att den innehåller felaktigheter. Det kan finnas oupptäckta
felöversättningar, rester av gammal information som tagits bort i
originalet men ej i översättningen, samt tillägg i originalfilen
som ännu inte kommit med i översättningen. Jämför därför med
originalet om tveksamhet uppstår. Vid felaktigheter, säg till på
diskussionslistan <freeciv.se@freelists.org>.


Webbplats:
==========

Freecivs webbplats är:

  http://www.freeciv.org/

Här kan man få de senaste nyheterna, utgåvorna och uppdateringarna,
hitta information om Freecivs diskussionslistor samt se metaservern
som visar information om spel som spelas runt om i världen.

Licens:
=======

Freeciv ges ut under GNU General Public License. Det betyder i korthet
att man får kopiera detta program (även källkoden) fritt, men se filen
"COPYING" för fullständiga villkor.

Kompilera och installera:
=========================

Var god läs filen INSTALL.sv noga för anvisningar kompilering och
installering av Freeciv.


Påbörja ett nytt spel:
======================

Freeciv är 2 program, en server och en klient. När ett spel är i gång
körs ett serverprogram och så många klientprogram som det finns
mänskliga spelare. Serverprogrammet behöver inte X, men det gör
klientprogrammen.

  ANMÄRKNING:
  Följande exempel antar att Freeciv har installerats på systemet och
  att katalogen som innehåller programmen "civclient" och "civserver"
  finns i variabeln PATH. Om Freeciv inte är installerat kan man
  använda programmen "civ" och "ser" som finns i freecivkatalogen. De
  används på samma sätt som "civclient" och "civserver".

För att kunna spela Freeciv behöver man starta servern, klienterna och
datorspelarna, samt ge servern startkommandot. Här är stegen:

Server:

  För att starta servern:

  |  % civserver

  Eller för en lista över kommandoradsargument:

  |  % civserver --help

  När servern har startats visas en prompt:

  |  För inledande hjälp, skriv 'help'.
  |  >

  och man kan se denna information genom att använda hjälpkommandot:

  | > help
  |  Välkommen - detta är den inledande hjälptexten för
  |  freecivservern.
  |
  |  2 Viktiga serverbegrepp är kommandon och valmöjligheter.
  |  Kommandon, såsom "help", används för att växelverka med servern.
  |  Vissa kommandon tar ett eller flera argument, åtskilda av
  |  blanksteg.  I många fall kan kommandon och kommandoargument
  |  förkortas. Valmöjligheter är inställningar som styr servern medan
  |  den är i gång. 
  |
  |  För att ta reda på hur man får mer information om kommandon och
  |  valmöjligheter, använd "help help".
  |
  |  För den otåliga är kommandona för att komma i gång:
  |    show   -  se nuvarande valmöjligheter
  |    set    -  sätt valmöjligheter
  |    start  -  sätt i gång spelet när spelare har anslutit sig
  |    save   -  spara nuvarande spel
  |    quit   -  avsluta
  |  >

  Man kan använda kommandot "set" för att ändra någon av
  servervalmöjligheterna. Man kan få en lista med alla
  servervalmöjligheter med kommandot "show" och utförliga
  beskrivningar av varje servervalmöjlighet med kommandot "help
  <servervalmöjlighetsnamn>".

  Till exempel:

  |  > help size
  |  Valmöjlighet: size  -  kartstorlek i 1000 rutor
  |  Beskrivning:
  |    Detta värde bestämmer kartans storlek.
  |      size = 4 är en normal karta med 4000 rutor (standard)
  |      size = 20 är en jättelik karta med 20000 rutor
  |  Status: ändringsbar
  |  Värde: 4, Minsta möjliga: 1, Standard: 4, Högsta möjliga: 29

  Och:

  |  > set size 8

  Detta gör kartan dubbelt så stor som standardstorleken.

Klient:

  Nu ska alla mänskliga spelare ansluta genom att köra
  freecivklienten:

  |  % civclient

  Detta antar att servern kör på samma maskin. Om inte kan man
  antingen ange det på kommandoraden med parametern "--server" eller
  skriva in det i den första dialogrutan som visas i klientprogrammet.

  Antag till exempel att servern körs på en annan maskin kallad
  "baldur". Då ansluter spelare med kommandot:

  |  % civclient --server baldur

  Om man är den enda mänskliga spelaren behöver endast en klient
  användas. På vanligt Unixvis kan man köra klienten i bakgrunden
  genom att lägga till ett och-tecken:

  |  % civclient &

  En annan valmöjlighet är "--tiles" som används för att köra klienten
  med en annan uppsättning rutbilder för landskap, enheter med mera.
  Utgåvan innehåller 2 uppsättningar rutbilder:
  - amplio: Isometrisk, större och mer detaljerade rutor.
  - isotrident: Isometrisk, liknar Civilization 2.
  - trident: Liknar Civilization 1, rutstorlek 30x30 bildpunkter.
  - isophex: Isometrisk och hexagonal.
  - hex2t: Hexagonal.

  I denna utgåva är amplio stadardrutbildsuppsättning.
  Kör klienten med följande kommando för att använda en annan
  uppsättning, t.ex. trident:

  |  % civclient --tiles trident

  Andra uppsättningar kan hämtas från:

       http://www.freeciv.org/wiki/Tilesets

  Klienter kan ges tillåtelse att utföra serverkommandon. Skriv
  följande vid serverprompten för att endast ge dem tillåtelse att
  endast ge informationskommandon:

  |  > cmdlevel info

  Klienter kan nu använda "/help", "/list", "/show settlers" med mera.

Datorstyrda spelare:

  Det finns 2 sätt att skapa datorstyrda spelare. Det först är att
  ange antalet spelare med servervalmöjligheten "aifill":

  |  > set aifill 7

  Efter att ha använt serverkommandot "start" för att sätta i gång
  spelet, kommer de spelare som inte är mänskliga att bli datorstyrda.
  I exempelt ovan skulle 5 datorstyrda spelare ha skapats om det hade
  funnits 2 mänskliga spelare.

  Det andra sättet är att skapa en datorspelare med serverkommandot
  "create":

  |  > create Widukind

  Detta skapar den datorstyrda spelaren Widukind.

  Datorstyrda spelare tilldelas folkstammar efter att alla mänskliga
  spelare har valt folkstammar, men man kan välja en särskild folkstam
  för en datorstyrd spelare genom att använda ett namn som är namnet
  på en ledare för den folkstammen. Man kan till exempel spela mot
  ryssarna med följande kommando:

  |  > create Stalin

  Om ingen av de mänskliga spelarna väljer att spela med ryssarna
  kommer denna datorstyrda spelare att göra det.

Server:

  När alla har anslutit (använd kommandot "list" för att se vilka som
  är anslutna), sätt i gång spelet med kommandot "start":

  |  > start

  Sedan är spelet i gång!

Lägg märke till att i denna version av Freeciv har GTK- samt SDL-
klienterna förmågan att automatiskt starta en civserver-session i
bakgrunden när spelaren väljer att starta ett nytt spel från
huvudmenyn. Tack vare detta har det blivit mycket enklare att
komma igång med att spela Freeciv. Å andra sidan innebär det att
i det fall klienten krashar, drar den med sig servern i fallet och
det pågående spelet går förlorat. P.g.a. detta är det fortfarande
rekommenderat att starta civserver separat från civclient.


Tillkännage spelet:
===================

Om man vill ha andra motståndare än lokala vänner och datorstyrda
spelare kan man besöka Freecivs metaserver:

  http://meta.freeciv.org/

Det är en lista över freecivservrar. För att få sin server att anmäla
sig där kör man civserver med kommandoradsargumentet "--meta" eller
"-m".

Varningar:

 1) På grund av nya funktioner är olika versioner av server och klient
    ofta oförenliga. Versionen 2.0.0 är till exempel oförenlig med
    1.14.2 och tidigare versioner.

 2) Om metaserverknappen i anslutningsdialogen inte fungerar, undersök
    om internetanslutningen kräver en WWW-proxy, och se till att
    Freeciv använder den genom att ställa in variabeln $http_proxy. Om
    proxyn till exempel är proxy.minanslutning.se port 8888, sätt
    $http_proxy till http://proxy.minanslutning.se:8888/ innan
    klienten startas.

 3) Ibland finns det inga spel på metaservern. Antalet spelare där
    växlar under dygnets tider. Försök att skapa ett spel där själv!


Under spelets gång:
===================

Spelet kan sparas med serverkommandot "save":

  |  > save mittspel.sav

(Om servern är kompilerad med packningsstöd och servervalmöjligheten
"compress" är satt till någnting annat än 0 packas filen och kallas
mittspel.sav.gz.)

Freecivklienten fungerar i stort sett så som man kan förvänta sig av
ett civilizationspel med flerspelarstöd. De mänskliga spelarna gör
sina drag samtidigt. De datorstyrda spelarna gör sina drag när de
mänskliga spelarna har avslutat sina omgångar. Det finns en tidsgräns
som är satt till 0 sekunder (ingen tidsgräns) som standard. Detta
värde kan ändras med serverkommandot "set".

Titta på hjälpen i klientprogrammet. Alla 3 musknapparna används och
är dokumenterade i hjälpen.

Spelare kan trycka på returnknappen eller klicka på "Avsluta
omgång"-knappen för att avsluta sin omgång.

Använd spelardialogen för att se vilka som har avslutat sin omgång och
vilka man väntar på.

Använd inmatningsraden vid fönstrets underkant för att skicka
meddelanden till andra spelare.

Man kan skicka ett meddelande till en enskild spelare (till exempel
"einar"):

  |  einar: flytta på pansarvagnen NU!

Servern kan gissa sig till namn om man skriver dem ofullständigt. Om
man till exempel skriver "ein:" hittar den spelaren med namn som
stämmer delvis med namnet man skrev.

På nyare servrar (version 1.8.1 eller vissa utvecklingsversioner av
1.8.0) eller nyare kan man ge serverkommandon på klientens
inmatningsrad:

  |  /list
  |  /set settlers 4
  |  /save mittspel.sav

Serverhandhavaren tillåter kanske bara informationskommandon eftersom
det är en säkerhetsrisk att låta spelare använda alla serverkommandon,
till exempel:

  |  /save /etc/passwd

Naturligtvis ska freecivservern inte köras med fullständiga
rättigheter på grund av denna risk.

Om man just har börjat spela Freeicv och vill ha hjälp med strategin
kan man titta i filen "HOWTOPLAY.sv".

Se freecivhandboken på följande adress för mycket mer information om
klienten, servern och spelet:

  http://www.freeciv.org/wiki/Manual


Avsluta spelet:
===============

Det finns 3 sätt att avsluta spelet:

1) Vara den enda återstående spelaren.
2) Nå slutåret.
3) Bygga ett rymdskepp och sända i väg det så att det når Alfa
   Kentauri.

En utvärderingstabell visas i samtliga fall. Anmärkning:
Serverhandhavaren kan sätta slutåret när spelet är i gång genom att
ändra servervalmöjligheten "end-year". Detta är användbart när det är
uppenbart vem som kommer att segra men man inte vill spela sig igenom
uppstädningen.


Öppna spel:
===========

Man kan öppna ett sparat spel genom att köra servern med
kommandoradsargumentet "-f":

  |  % civserver -f mittspel2001.sav

eller om filen är pacakd:

  |  % civserver -f mittspel2001.sav.gz

Sedan kan spelarna återansluta:

  |  % civclient -n Bismarck

Lägg märke till att spelarnamnet anges med kommandoradsargumentet
"-n". Det är viktigt att spelaren använder sama namn som den använde
förrut, annars släpps de inte in.

Spelet kan sättas i gång igen med serverkommandot "start".


Lokalt språkstöd:
=================

Freeciv stöder flera språk.

Man kan välja vilket lokalt språk man vill använda genom att ange en
"locale". Varje locale har ett standardnamn (till exempel "de" för
tyska). Om man har installerat Freeciv kan man välja locale genom att
sätta variablen LANG till denna locales standardnamn innan man kör
civserver och civclient. För att till exempel köra Freeciv på tyska
gör man så här:

  export LANG; LANG=de    (i Bourneskalet (sh))

eller

  setenv LANG de          (i C-skalet (csh))

(Man kan göra detta i sin "~/.profile" eller "~/.login".)

Loggmeddelanden:
================

Både klienten och servern skriver loggmeddelanden. Dessa är av 5 olika
slag: "fatal", "error", "normal", "verbose", samt "debug".

Som standard skrivs fatal-, error- samt normal-meddelanden till
standard output. Man man skicka loggmeddelanden till en fil i stället
med kommandoradsargumentet "--log <filnamn>" eller "-l filnamn".

Man kan ändra loggläget med kommandoradsargumentet "--debug <läge>"
eller "-d <läge>" (eller "-de <läge>" för Xawklienten eftersom "-d" är
flertydigt mellan "-debug" och "-display"), där <läge> är 0, 1, 2
eller 3. 0 betyder att endast dödligameddelanden visas, 1 betyder att
dödliga och felmeddelanden visas, 2 betyder att dödliga, fel- och
normala meddelanden visas (standard). 3 betyder att dödliga, fel-,
normala och mångordiga meddellanden visas.

Om man kompilerar med DEBUG definierad (ett enkelt sätt att göra detta
är att konfigurera med "--enable-debug") , kan man få
avlusningsmeddelanden genom att sätta loggläget till 4. Det är
dessutom möjligt att styra avlusningsmeddelanden (men inte andra
meddelanden) med avseende på fil. Använd då "--debug 4:str1:str2" (så
många strängar man vill) och alla filnamn som överensstämmer med dessa
strängar som understräng har avlusningsloggning påslaget. Alla andra
avlusningsmeddelanden stängs av. Använd "--debug 4:str1,undre,övre"
för att styra rader. Endast meddelanden mellan undre raden och övre
raden kommer att visas. Endast 1 uppsättning gränser kan anges för en
fil.

Exempel:

  |  % civserver -l mitt.log -d 3

Detta skickar alla loggmeddelanden, innefattande verbose-
meddelanden, från servern till filen "mitt.log".

Exempel:

  |  % civclient --debug 0

Detta döljer alla loggmeddelanden utom dödliga meddelanden.

Exempel:

  | % civserver -d 4:log:civserver,120,500:autoattack

Detta visar alla fatal-, error-, normal- samt verbose-meddelanden för
servern, samt avlusningsmeddelanden för vissa angivna delar. Lägg
märke till att "log" stämmer överens med både "gamelog.c" och "log.c".
För "civserver.c" visas endast avlusningsmeddelanden mellan raderna
120 och 500. Detta exempel fungerar endast om servern har kompilerats
med DEBUG.


Buggar:
=======

Vi vill gärna bli underrättade om buggar så att vi kan åtgärda dem.
Se filen BUGS.sv för instruktioner om hur man rapporterar buggar.


Diskussionslistor:
==================

Vi har fyra diskussionslistor:

  freeciv-announce Kungörelser av allmänt intresse.
                   Denna lista kan endast läsas och har låg aktivitet.
                   Man kan alltså inte skicka mail till
                   listan utan bara ta emot.
  freeciv-i18n     Översättning av Freeciv.
                   Samtal om översättning av Freecivkoden,
                   dokumentation och websida till andra språk än 
                   engelska.
  freeciv-dev      Programmering och annan utveckling.
  freeciv-commits  Kungörelser om ändringar i SVN-trädet.
                   Denna lista kan endast läsas och sprider
                   automatiska meddelanden. Man kan alltså inte skicka
                   brev till listan utan endast ta emot.

Alla listor är öppna för allmänheten och alla är välkomna att prenumerera.

Listorna tillhandahålls av gna.org. För mer information hur du kan
läsa och prenumerera, se http://gna.org/mail/?group=freeciv


Internet Relay Chat (IRC)
=========================

Flera spelare och utvecklare håller till på #freeciv och #freeciv-dev
på Freenode. Försök ansluta till servern:

	irc.freenode.net


Slutligen:
==========

Ha det kul och lycka till!

                                   --  Freecivgänget
