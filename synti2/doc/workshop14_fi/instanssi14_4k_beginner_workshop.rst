.. raw:: latex

  %make block paragraphs:
  \parindent0pt  \parskip10pt


#########################################################
Instanssi 2014 -- Aloittelijatyöpaja 4k-demokoodaukseen
#########################################################

Paavo Nieminen, ``paavo.j.nieminen@jyu.fi``, a.k.a qma (or his
Instanssi alter ego, "The Old Dude")


.. contents::

Työpajan tavoite
=================================

Kaikessa yksinkertaisuudessan tavoitteena on, että jokainen työpajan
osallistuja pystyy osallistumaan Instanssi 2014 -tapahtuman
4k-demokilpailuun omalla tuotoksellaan. Kyseessä on aika "hardcore"
ohjelmointitaidon näyttö, jos tekijä aloittaa kaiken puhtaalta
pöydältä. Oma matkani tyhjästä editori-ikkunasta viime vuoden
Assemblyn 9. sijalle yltäneeseen tuotokseen kesti kalenteriajassa
laskettuna kolme vuotta. Aloittelijoille suunnatussa työpajassa meidän
on otettava pohjaksi valmista, toimivaksi todettua, esimerkkikoodia,
jota muunnellaan. Samasta lähtökohdasta johtuen on todennäköistä, että
osallistujen tuotokset tämän viikonlopun kilpailussa muistuttavat
paljolti toisiaan. Mutta hyväksytään nyt se, että taideteos nimeltä
"punainen pallo" on eri tuotos kuin "vihreä pallo pomppii" tai "kaksi
sinistä palloa" :). Äänimaisemat tulevat luultavasti olemaan
siniaallon ja valkoisen kohinan risteytyksiä.

Lähtökohdaksi otetaan Windows-alustalla Iñigo Quilezin "4ksystem" ja
Linux-alustalla allekirjoittaneen tämän päiväinen versio
"synti2"-softasyntikkaprojektista.

Aikatauluarvio::

  21:00 Aloitetaan, tutustutaan, ja kartoitetaan toimintamallit.

        Pieni esittely 4k:n perusideasta ja työpajan sisällöstä.

  21:15 Tekeminen alkaa.

  23:00 Tauko; happea.

  23:15 Tekeminen jatkuu.  

  01:00 Työpaja päättyy (ei tarkoita, etteikö apua voisi kysyä aina
        kilpailun deadlineen saakka!)

Työpaja etenee jokaisen osallistujan kohdalta seuraavin
askelin. Meillä tulee varmasti olemaan runsaasti yllättäviä alustoihin
liittyviä haasteita ja uutta opittavaa kaikista vaiheista:

1. Työkalujen asentaminen:

    - Windows-käyttäjät tarvitsevat Visual C++ -ohjelmiston ja
      Crinkler-työkalun.

    - Linux-käyttäjät tarvitsevat erinäisiä kirjastoja ja apuohjelmia,
      joista suurin osa löytyy luultavasti oman distron
      pakettimanagerilla ja loput etsitään netistä yhdessä tuumin. En
      muista ulkoa, mitä kaikkea tarvitaan (ainakin dev-kirjastoineen
      GL, SDL, fltk, jack sekä apuohjelmapuolella sstrip ja zopfli)

2. Frameworkin haku omalle koneelle

    - Windows-käyttäjät ottavat Inigon 4ksystem-frameworkin auki VC++:aan

    - Linux-käyttäjät ottavat uusimman version synti2:n
      master-haarasta. (Varaudutaan debuggaamaan tätä tapahtuman
      mittaan)

3. Frameworkista löytyvän 4k-demoesimerkin kääntäminen ja
   linkittäminen alle 4096 tavun mittaiseksi exeksi.

4. Muutaman Framework-esimerkin testaaminen Instanssin kompokoneella
   (oletetaan, että jos esimerkkikoodi toimii kompokoneella, niin myös
   siitä pienin muutoksin tehdyt omat tuotokset toimivat. Ja puuta
   koputetaan tässä kohtaa nyt nii-in kovaa!!!)

5. Kopioidaan Framework-esimerkki pohjaksi omalle tuotokselle ja
   aloitetaan modaamaan. **HUOM: Muista tuotoksen yhteydessä
   rehellisesti ilmoittaa tekijän nimen ja nettiosoitteen kera, mistä
   esimerkistä olet lähtenyt liikkeelle! Tämä on sanomattakin selvää,
   mutta sanotaan se tässä kuitenkin.**

.. raw:: latex

    \pagebreak



Mikä on "4k intro"
=============================


"4k intro" on maksimissaan 4096 tavun mittainen ohjelmatiedosto, joka
voidaan käynnistää ja suorittaa kohdealustalla ja joka generoi
tyypillisesti sekä liikkuvaa kuvaa että kuvaan jollain tavalla
liittyvää ääntä. Ohjelmien toteuttaminen pieneen tilaan on tunnetusti
haastavaa, ja päämääränä onkin kilpailla siitä, kuka saa tehtyä
hienoimman tuotoksen rajoitteena olevaan tavumäärään.

Koon suhteen rajoitettujen ohjelmien historia juontaa juurensa
demoskenen alkuaikoihin saakka. Aiemmin on harrastettu erilaisia
rajoituksia, mm. 64 kilotavua on ollut tyypillinen. Erityisesti
näytönohjainteknologian kehityksen ja tulkkaavien alustojen kautta 64
ei ole enää riittävän haastava rajoite, vaan on sanottu mm. että "4k
on uusi 64k". Aiemmin "kuninkuussarjana" olleesta 4k:sta on siis
tullut keskisarjan haaste, ja nykyään kuninkaat kisaavat 1k:ssa eli
1024 tavun tuotoksissa :). Emme mene historiaan tässä tämän enempää.

Miksi 4k on "vaikeaa":

- Normaaleihin tarkoituksiin tehdylle tietokoneohjelmalle kokorajoite
  on nykyaikana hyvin teennäinen, mistä syystä kääntäjätyökalut
  tuottavat 4096 tavuun nähden massiivisen kokoisia
  ohjelmatiedostoja. Neljän kilotavun saavuttaminen edellyttää
  erityismenettelyjä ja -työkaluja, joita normaalisti ei tarvita.

- Algoritmien suunnittelussa saatetaan joutua käyttämään
  epätyypillisiä tai epäloogisia ratkaisuja, jotta kompromissi saadaan
  painotettua pienen ohjelmakoodin suuntaan (vs. nopein tai
  yksinkertaisin ratkaisu)

- Ei voida käyttää valmiita platformikirjastoja, koska kilpailujen
  "herrasmiessopimukseen" kuuluu, että tuon 4096-tavuisen exen täytyy
  toimia ilman lisäkirjastojen asentamista koneelle.

Miksi ihmeessä tällaista hulluutta oikein tehdään?

- Kyseessä on kilpailu siinä missä vaikkapa biljardi tai
  jalkapallo. Kyseessä on laji, jossa voi kehittyä jatkuvasti
  paremmaksi ja kilpailla itseään ja muita vastaan. Tästä lajista
  sitten joko kiinnostuu tai ei, niinkuin biljardista ja
  jalkapallostakin.

- Ohjelmoijalle 4k tarjoaa muutakin kuin vain kilpailuasetelman. Se
  tarjoaa sovelluksen, jossa on aivan pakko keskittyä muutamaan
  yksityiskohtaan, esimerkiksi yhteen grafiikkaefektiin,
  äänisynteesimenetelmään tai generatiiviseen
  sisällöntuottomenetelmään, kerrallaan. 4k tarjoaa siis erinomaisen
  selväpiirteisen ympäristön opetella jokin uusi asia.

- Ohjelmakoodin runttaaminen aina pienempään tilaan vaatii temppuja,
  joiden tekemiseksi on pakko oppia ohjelmoinnista, kääntäjistä,
  käyttöjärjestelmästä ja varmasti muistakin asioista sellaisia
  asioita, jotka muissa olosuhteissa tulisivat harvemmin vastaan. Oppi
  ei tunnetusti ojaan kaada.

- Vaikka koodin koon optimointi ei ole tyypillinen tarve ohjelmien
  tekemisessä, on sillä kuitenkin sovellusalueensa esimerkiksi
  halpojen mikrokontrollereiden ohjelmoinnissa, joihin ei vaan mahdu
  määräänsä enempää koodia sisään. Lisäksi joskus (ei aina) lyhyt
  ohjelmakoodi on myös nopea, ja sehän ei koskaan haitaksi ole.

- 4k-tuotoksiin kykenevä koodari saa kollegoiltaan, jotka ymmärtävät
  haasteet, mutta eivät ole itse koskaan syventyneet asiaan,
  yllättävänkin suuren määrän respektiä!


Tietolähteitä ja foorumeita
================================


Alustoja
================================

- IQ

- qma

- etsivä löytää varmasti muitakin


Muuta mitä mieleen tulee
================================

TODO: Täydennetään työpajan mittaan, riippuen siitä mitä tapahtuu.

