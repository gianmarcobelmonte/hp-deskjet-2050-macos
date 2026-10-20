# Driver HP Deskjet 2050 J510 per macOS

*[Read in English](README.md)*

Un driver funzionante per macOS per la stampante **HP Deskjet 2050 J510 series** (USB), che HP non supporta più sulle versioni recenti di macOS come Sequoia.

È un port di `hpcups`, il filtro CUPS open source del driver Linux ufficiale di HP ([HPLIP](https://developers.hp.com/hp-linux-imaging-and-printing) 3.26.6). È ridotto al codice che serve a questa stampante e compilato per il CUPS incluso in macOS. Non ha dipendenze esterne, non richiede plugin proprietari HP e non installa estensioni del kernel.

> Questo progetto non è affiliato né approvato da HP. "HP" e "Deskjet" sono usati solo per identificare la stampante supportata.

## Compatibilità

| | |
|---|---|
| Stampante | HP Deskjet 2050 J510 series, collegata via **USB** |
| ID USB | `03f0:8711` |
| macOS | Provato su macOS 15 Sequoia (15.7). Dovrebbe funzionare anche su altre versioni recenti |
| Mac | Binario universale (Intel e Apple Silicon). Provato su un Mac Intel |

Le altre stampanti **non** sono supportate: lo script di installazione cerca proprio questo modello.

## Funzioni

- Stampa a colori e in scala di grigi (alta qualità, oppure solo nero)
- Qualità di stampa: Bozza veloce, Normale, Ottima, Foto ad alta risoluzione
- Carta comune e fotografica
- A4, Letter, A5, A6, B5, Legal, formati foto (10×15 cm, 13×18 cm…), buste (DL, C5, C6, #10) e formati personalizzati
- Opzioni del dialogo di stampa in italiano e in inglese, secondo la lingua di sistema

La scansione non è inclusa: è solo un driver di stampa.

## Installazione

### 1. Installa gli strumenti di compilazione

Servono i Command Line Tools di Apple. Sono gratuiti e non richiedono Xcode completo. Apri il **Terminale** ed esegui:

```sh
xcode-select --install
```

Se sono già installati comparirà un messaggio che lo dice. Passa al punto successivo.

### 2. Scarica il progetto

```sh
git clone https://github.com/gianmarcobelmonte/hp-deskjet-2050-macos.git
cd hp-deskjet-2050-macos
```

In alternativa, su GitHub usa **Code → Download ZIP**, estrai il file ed entra nella cartella con `cd`.

### 3. Compila

```sh
make
```

Vengono creati `build/hpcups` (il driver) e `build/usbprobe` (uno strumento diagnostico). Gli avvisi del compilatore su `Mode9.cpp` vengono dal codice originale HP e non sono un problema.

### 4. Collega la stampante

1. Accendi la stampante e **aspetta che sia completamente ferma e silenziosa**.
2. Solo dopo collega il cavo USB al Mac. Un adattatore USB‑C → USB‑A va benissimo.

Se colleghi il cavo mentre la stampante si sta ancora avviando, la sua interfaccia USB può smettere di rispondere (vedi [Risoluzione dei problemi](#risoluzione-dei-problemi)).

### 5. Installa

```sh
sudo ./install.sh
```

Inserisci la password del Mac quando richiesto. Lo script:

- copia il driver in `/Library/Printers/hpcups-mac/Filters/hpcups`;
- installa il file di descrizione della stampante (PPD) in `/Library/Printers/PPDs/Contents/Resources/`;
- **elimina l'eventuale coda di stampa `HP_Deskjet_2050_J510_series`** e ne crea una nuova che usa questo driver. macOS crea spesso quella coda da solo quando colleghi la stampante, di solito con un driver che non funziona.

Se avevi aggiunto la stampante a mano da **Impostazioni di Sistema → Stampanti e scanner** con un altro nome, rimuovila da lì per evitare confusione. Poi usa la coda **HP Deskjet 2050 J510 series**.

### 6. Imposta A4 come formato predefinito (facoltativo)

Il formato predefinito è Letter (americano). Per impostare A4 per il tuo utente:

```sh
lpoptions -p HP_Deskjet_2050_J510_series -o PageSize=A4
```

### 7. Stampa una pagina di prova

```sh
lp -d HP_Deskjet_2050_J510_series /usr/share/cups/data/testprint
```

Da qui in poi scegli **HP Deskjet 2050 J510 series** nel dialogo di stampa di qualsiasi app.

## Aggiornamento

Scarica la nuova versione, ricompila e rilancia l'installazione:

```sh
git pull
make
sudo ./install.sh
```

## Disinstallazione

```sh
sudo ./uninstall.sh
```

Vengono rimossi il driver, il PPD e la coda di stampa.

## Risoluzione dei problemi

### Non stampa e la coda si ferma con "Impossibile inviare i dati alla stampante"

CUPS non è riuscito a scrivere sulla stampante via USB e ha messo in pausa la coda. È un problema del collegamento USB, non del driver.

1. Stacca il cavo USB.
2. Controlla che la stampante sia accesa e ferma.
3. Ricollega il cavo.
4. Riattiva la coda:
   ```sh
   cupsenable HP_Deskjet_2050_J510_series
   ```

Se non basta, spegni la stampante, stacca l'alimentazione per 60 secondi, riaccendila, aspetta che sia ferma e solo allora collega il cavo USB.

Per verificare se la stampante risponde via USB:

```sh
./build/usbprobe
```

Una stampante che funziona restituisce una lunga stringa `GET_DEVICE_ID` (che inizia con `MFG:HP;MDL:Deskjet 2050 J510 series;CMD:...`) e `GET_PORT_STATUS ... no-error=1`. L'errore `0xe00002ed` significa che la stampante non risponde via USB: ricollegala come descritto sopra. `Printer interface 03f0:8711 not found` significa che macOS non vede un'interfaccia di stampa funzionante: ricollega il cavo.

### La stampante parte e si ferma; lampeggiano le spie di accensione e delle cartucce

Il carrello delle cartucce è bloccato (carriage stall): è un problema meccanico.

1. Spegni la stampante.
2. Apri lo sportello delle cartucce e togli eventuali pezzi di carta o oggetti dal percorso del carrello.
3. Sposta il carrello a mano per controllare che scorra libero, e verifica che entrambe le cartucce siano ben agganciate.
4. Rimetti la carta, chiudi lo sportello e riaccendi.

### Il contenuto vicino ai bordi esce tagliato

La stampante non può stampare fino al bordo del foglio. I margini non stampabili in A4 sono circa 3 mm a sinistra e a destra, 1,5 mm in alto e **14,5 mm in basso**. macOS taglia tutto quello che esce dall'area stampabile, ed è per questo che il taglio si vede già nell'anteprima di stampa.

Per evitarlo:
- Nel dialogo di stampa scegli **Adatta alla pagina**, oppure riduci la scala al 95% circa.
- Oppure imposta nel documento margini di almeno 15 mm in basso e 4 mm ai lati.

### Stampe sbiadite o colori mancanti

Probabilmente le cartucce sono quasi vuote. Le spie delle cartucce accese fisse indicano inchiostro in esaurimento.

### Leggere i log di CUPS

```sh
tail -50 /var/log/cups/error_log
```

Per avere più dettagli, attiva il log di debug, ristampa e poi disattivalo:

```sh
sudo cupsctl --debug-logging
sudo cupsctl --no-debug-logging
```

## Come funziona

```
App → sistema di stampa di macOS (CUPS) → PDF → immagine raster
    → hpcups (questo driver) → comandi PCL3GUI2 → USB → stampante
```

- `src/hpcups/`: il filtro `hpcups` di HPLIP 3.26.6, limitato alla famiglia di linguaggi PCL3 (questa stampante usa `pcl3gui2`).
- `src/common/`: codice di supporto di HPLIP.
- `ppd/hp-deskjet_2050_j510_series.ppd`: la descrizione ufficiale della stampante di HPLIP, con in più la traduzione italiana.
- `tools/usbprobe.c`: un piccolo strumento IOKit che interroga la stampante direttamente via USB.
- `install.sh` / `uninstall.sh`: installano e rimuovono il driver e la coda di stampa.

Le modifiche rispetto a HPLIP sono elencate nel [README in inglese](README.md#changes-from-hplip). Ogni file sorgente modificato riporta una nota che descrive la modifica.

## Licenza

Il progetto è distribuito sotto **GNU General Public License v2.0 o successiva** ([LICENSE](LICENSE)). È obbligatorio perché include codice HPLIP con licenza GPL v2 o successiva (`ErnieFilter.cpp`).

La maggior parte del codice HPLIP incluso è © HP Development Company, L.P. con licenza di tipo BSD, e alcuni file sono con licenza MIT. Ogni file mantiene la sua intestazione originale di copyright e licenza. Il testo completo delle licenze HPLIP è in [COPYING.HPLIP](COPYING.HPLIP).

Il software è fornito **senza alcuna garanzia**. Usalo a tuo rischio.
