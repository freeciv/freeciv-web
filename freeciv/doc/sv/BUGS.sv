======
BUGGAR
======

Freeciv 2.1 är en "driftssäker" utgåva och anses vara tillräckligt
fri från programfel för dagligt bruk. Om man trots allt hittar
buggar skulle vi vilja få reda på det så att vi kan rätta
dem.


ATT ANMÄLA BUGGAR:
==================

Så här gör man:

- Ser efter att det inte är en känd bugg! Se listan vid:

        http://www.freeciv.org/wiki/Known_Bugs

- Tittar på <http://www.freeciv.org> och försäkrar sig om att man har
  den nyaste versionen. (Vi kanske redan har rättat felet.)

- Tittar på Freecivs system för spårning av buggar vid:

        http://bugs.freeciv.org/

  för att se om programfelet redan har anmälts.

- Anmäler buggen via buggsökarsystemet ovan!

   Om du får några GDK/GTK-meddelanden, som t.ex.:

     Gtk-CRITICAL **: file gtkobject.c: line 1163 (gtk_object_ref): 
     assertion oject->ref_count > 0' failed.

   v.g. starta om din klient och lägg till "-- --g-fatal-warnings" vid
   kommandoraden. På detta sätt får du en "core dump". V.g. bifoga den
   del av denna core dump som kallas "stack trace" med din buggrapport.

   Vad man ska nämna i sin buggrapport:

   - Beskrivning av problemet och i förekommande fall det felmeddelande
     man får.

   - Vilken klient man använder (GTK+, SDL, Win32 eller Xaw).

   - Namn och versionsnummer av:

       - Det operativsystem som man använder. Kommandot "uname -a" kan
         vara användbart.

       - Versionsnumret för Freeciv.

       - Om man använder Gtk+-klienten, versionsnumren (om man känner
         till dem) för sina Gtk+-, glib- och imlib-bibliotek.

       - Om man använder SDL-klienten, versionsnumren (om man känner
         till dem) för sina SDL-, SDL_image-, PNG- och
         freetype-biblioteken.

       - Om man använder Xaw-klienten, versionsnumren (om man känner
         till dem) för X-, PNG-, Z- och Xaw-biblioteken, samt i
         synnerhet om det är en variant såsom Xaw3d, Xaw95 eller Nextaw.

       - Om man kompilerar från källkod: namnet och versionsnumret för
         kompilatorn.

       - Om man installerar från ett färdigkompilerat paket, dess
         namn, vilken distribution det är för och varifrån man hämtat
         det.

   - Om Freeciv gör en "core dump", kan vi efterfråga en "stack trace"
     som du kan skaffa fram m.h.a. en avlusare. För detta behövs
     "core"-filen samt binären, så var god behåll de ett tag.

   - Om det är ett fel i en översättning ska det anmälas till
     översättaren för språket i fråga. För deras namn och addresser,
     se:


YTTERLIGARE INFORMATION:
========================

Freecivs webbplats tillhandahåller mycket mer material:

        http://www.freeciv.org/
