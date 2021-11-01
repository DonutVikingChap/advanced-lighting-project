# Projektspecifikation - Avancerad ljussättning till spel med höga prestandakrav

Denna projektspecifikation beskriver ett projekt till kursen TSBK03 _Teknik för avancerade datorspel_ vid Linköpings universitet.

## Mål

Syftet med detta projekt är att skriva en avancerad 3D-renderare med hög kvalité och tillräckligt bra prestanda för att den ska kunna fungera som bas till ett modernt datorspel med höga prestandakrav, t.ex. ett tävlingsinriktat FPS-spel eller VR. Renderaren ska använda PBR-shading, lightmapping och cascaded shadow mapping med stöd för soft shadows. Målet är att uppfylla dessa krav med så hög bildrutefrekvens som möjligt.

Projektet skrivs i C++ och GLSL med OpenGL och SDL2 och skall resultera i en körbar applikation som renderar en enkel scen med teknikerna som nämnts ovan.

## Avgränsningar

Renderaren kommer inte att ha någon funktionalitet relaterad till partikelrendering eller post-processingeffekter som oskärpa eller bloom.

För att kunna fokusera på att uppnå en god basnivå av kvalité och prestanda måste renderaren inte ta culling-relaterade optimeringar i åtanke och kan istället anta att alla objekt i en scen är synliga på samma gång. Detta innebär att om renderaren ska användas till en slutprodukt så kan den behöva anpassas med applikationsspecifika optimeringar som t.ex. avståndsbaserad detaljnivå samt occlusion culling baserat på t.ex. areaportals.

För att få mer tid att fokusera på shadow mapping kommer lightmapgenerering inte att ingå i projektet, utan detta kommer att göras med ett externt bibliotek.

## Krav

1.  Applikationen **skall** visa den renderade scenen genom en dynamisk "kamera" vars synfält kan förflyttas i realtid.
2.  Renderaren **skall** kunna rendera en scen bestående av ett godtyckligt antal 3D-meshes med olika transformationer.
3.  Renderaren **skall** ha stöd för MSAA (multisamplad kantutjämning).
4.  Renderaren **skall** använda en shadermodell med stöd för normal maps och fysiska materialattributer representerade i texturer.
5.  Renderaren **skall**, i den resulterande applikationen, kunna uppnå en bildrutefrekvens på minst 1000 Hz på en dator med följande hårdvara:
    -   CPU: AMD Ryzen 7 5800X
    -   RAM: 32 GB, 3600 MHz, CL 16
    -   GPU: Nvidia GeForce GTX 1080
6.  Renderaren **bör** applicera en förbakad lightmap på den ljussatta 3D-scenen.
7.  Renderaren **bör** rendera en cube map "skybox" som bakgrund till 3D-scenen.
8.  Renderaren **bör** kunna visa reflektioner i material baserat på närmsta tillgängliga förrenderade cube map i omgivningen.
9.  Renderaren **bör** använda cascaded shadow mapping för att ge alla objekt i scenen en dynamisk skugga som kastas från en riktad "sol"-ljuskälla.
10. Renderaren **bör** filtrera shadow mappen med en teknik som PCF eller PCSS för att få mjuka kanter på skuggorna.
11. Filtreringsmetoden som används **bör** göra skuggorna mjukare beroende på avståndet till det skuggande objektet och/eller ljuskällan.

## Milstolpe

Vid mitten av projektet bör vi vara klara med PBR-shadingen och lightmap-appliceringen, d.v.s. krav 1 till 6. Vi kan då fokusera resten av tiden på att implementera och förbättra CSM.

## Svårighetsgrad

Vi anser att detta är ett avancerat projekt eftersom tekniken som krävs för högpresterande soft shadow mapping på stora scener är avancerad och har stor potential för finslipning och kreativa lösningar.
