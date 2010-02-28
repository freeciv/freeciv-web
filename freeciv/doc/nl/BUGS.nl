================
BUGS c.q. FOUTEN
================

Freeciv 1.13.0 is een "stabiele" versie en wordt bug/foutvrij genoeg geacht
voor dagelijks gebruik. Indien u echter een bug ontdekt dan willen we dat echt
graag weten, zodat we dat kunnen verhelpen. Dit document is een opsomming van
ons bekende fouten in deze versie en geeft informatie over hoe u een nieuwe
bug kunt rapporteren.

De opsomming betreft uitsluitend de meest zichtbare fouten. Voor een complete
opsomming zie:

    http://www.freeciv.org/cgi-bin/bugs

BEKENDE FOUTEN:
===============

 - Sommige regels met speciale tekens erin verschijnen blanco indien uw locale
   op "C" is ingesteld. Als een noodoplossing kunt u uw locale op iets anders
   zetten, zoals "en_US" of uiteraard "dutch" ;-)

 - Wijzigingen in de Burgemeester-instellingen worden pas naar de server
   gezonden als u op de 'Beurt klaar' knop klikt (of Enter geeft). Wijzigingen
   die u maakt in dezelfde beurt als die waarin u het spel opslaat komen als
   gevolg hiervan dus niet in het opgeslagen spel terecht.

 - Als u de burgemeester gebruikt is het resulterende opgeslagen spel niet
   'endian' en 64bit veilig. U kunt deze spelen dus niet overdragen naar een
   computer met een andere architectuur.

 - De 'easy' KI is niet gemakkelijk genoeg voor beginnende spelers. Als de KI
   u al in het beginstadium van het spel op uw donder geeft, probeer dan de
   'generator' serveroptie op 2 of 3 te zetten. Uiteraard voordat u het spel
   begonnen bent. Aan de serverprompt type:
       set generator 2
   of:
       set generator 3

 - De 'hard' KI is niet sterk genoeg voor zeer ervaren spelers en doet nog
   steeds enkele domme dingen. Hij verkiest het bijv. om steden in opstand te
   laten boven het laten uithongeren/krimpen.

 - Soms zijn er zoveel vooruitgangen in het 'doel' menu van het
   wetenschappelijk rapport, dat het menu voorbij de onderkant van het scherm
   komt en u niet alles meer kunt selecteren.

 - U kunt some het bericht krijgen
     {ss} speler voor fragment <01> niet gevonden
     {ss} speler voor fragment <01> niet gevonden
   wanneer u de esound driver gebruikt. Dat is niets om u zorgen over te
   maken.

 - Sommige gevolgen van wonderen en onderzoek hebben pas de beurt erop
   effect. Bijv. wanneer u de vuurtoren gebouwd hebt dan zullen de triremen
   pas de beurt erop een bewegingspunt extra krijgen.

 - De XAW client kan slechts 25 burgers tonen in de stadsdialoog.

 - De auto-aanval werkt in het algemeen niet erg goed.

 - Wanneer u een goto/ga naar in de server plant, bijv. een auto-kolonist of
   een vliegtuig, dan kan de server informatie gebruiken waarover de speler
   nog niet kan beschikken.

 - De wetenschapsdialoog wordt niet bijgewerkt wanneer u een vooruitgang
   boekt. U dient hem te sluiten en opnieuw te openen.

 - In de Gtk client is er soms rommel in het gebied naast de minikaart.

 - Automatische routines zoals auto-onderzoek kunnen niet zo goed overweg
   met triremen.

 - LOG_DEBUG werkt niet met niet-GCC compilers.

 - De kleurmarkering in het Gtk-berichtenvenster gaat verloren indien de
   dialoog gesloten en heropend wordt.

 - Bij het instellen van servervariabelen controleert de server de waarden
   niet altijd zo goed als zou moeten.

 - Erge dingen gebeuren wanneer u meerdere algemene werklijsten tegelijk
   bijwerkt.

 - Zelfs in enkelspel zal de KI elke beurt zowel voor als na de menselijke
   speler een kans om te bewegen krijgen. Dit kan soms de indruk wekken dat
   een KI tweemaal verplaatst (dat is dus echt niet zo).

 - De xaw client werkt niet erg goed in combinatie met de KDE window manager.
   Probeer de GTK client te gebruiken of gebruik een andere window manager.

 - Versie 1.13.0 werkt niet met versie 1.12.0 en eerder. Merk op dat de client
   nu wat harder stopt door een boodschap af te drukken op stdout en daarna te
   stoppen.

RAPPORTEREN VAN FOUTEN:
=======================

(Als het vertalingsfouten betreft in de Nederlandse versie, neem dan contact
op met Pieter J. Kersten <kersten@dia.eur.nl>. Voor vertalingsfouten in andere
talen, neem contact op met de primaire contactpersoon voor die taal. Kijk op 
<http://www.freeciv.org/i10n.phtml> voor de namen en mailadressen van deze
mensen.)

Dit is wat u moet doen:

 - Controleer of het niet één van de bovenstaande fouten is! :-)

 - Controleer het Freeciv website om er zeker van te zijn dat u de laatste
   versie van Freeciv hebt. (We zouden het al opgelost kunnen hebben.)

   In het bijzonder zou u een ontwikkelaars-versie kunnen proberen uit onze
   CVS database. Deze kunt u FTP'en van:

        http://www.freeciv.org/latest.html

 - Controleer de Freeciv FAQ (Frequent Asked Questions) op het Freeciv website
   om te kijken of er een noodoplossing (Engels: workaround) is voor uw fout.

 - Controleer het Freeciv foutenvolgsysteem op:

        http://www.freeciv.org/cgi-bin/bugs

   om te kijken of de fout al eerder gerapporteerd is.

 - Verstuur een foutrapport!

   U kunt een foutrapport per email aan <bugs@freeciv.freeciv.org> versturen
   of via het website op <http://www.freeciv.org/cgi-bin/bugs> een formulier
   invullen. Let op: de voertaal is Engels!

   Of, als u de Freeciv ontwikkelaars enig commentaar wilt sturen maar niet
   direct een foutrapport in wilt vullen, kunt u email sturen naar
   <freeciv-dev@freeciv.org>, de Freeciv ontwikkelaars mailinglijst.

   Wat moet er in uw rapport staan (Let op: in het Engels!):

   - Beschrijf het probleem, inclusief eventuele berichten die getoond werden.

   - Geef aan welke client u gebruikte (Gtk+ of Xaw, ...)

   - Vertel ons de naam en de versie van:

       - Het besturingssysteem dat u gebruikt. U kunt in dit geval de 
         "uname -a" opdracht bruikbaar vinden.

       - Het versienummer van Freeciv

       - Als u de Gtk+ client gebruikt, de versienummers (indien bekend) van
         uw Gtk+, glib en imlib bibliotheken.

       - Als u de Xaw client gebruikt, de versienummers (indien bekend) van de
         X, Xpm en Xaw-bibliotheken en in het bijzonder of het standaard Xaw
         is of een variant als Xaw3d, Xaw95 of Nextaw.

       - Als u van sourcecode hebt gecompileerd, de naam en het versienummer
         van de compiler.

       - Als u installeerde van een binair pakket, de naam van het pakket, de
         distrubutie waarvoor het bestemd is en waar u het vandaan hebt.

   - Als Freeciv "core dumpt", dan kunnen wij u vragen om een debugger te
     gebruiken om ons van een "stack trace" te voorzien. U hebt daar dan het
     "core" bestand voor nodig, dus bewaar deze alstublieft nog even.


VERDERE INFORMATIE:
====================

Voor meer informatie, als altijd, kom naar het Freeciv website:

        http://www.freeciv.org/
