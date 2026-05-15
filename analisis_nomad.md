# Analisi Tecnica Supporto ExpressLRS - RadioMaster Nomad

Questa relazione analizza approfonditamente l'hardware e il firmware ExpressLRS relativi al modulo **RadioMaster Nomad 2.4/900 TX**, con focus sulla mappatura dei GPIO, la gestione dei due chip RF LR1121, dell'amplificatore di potenza (PA) esterno tramite il pin DAC `power_apc2`, della ventola e le implicazioni per un eventuale porting verso l'ecosistema Meshtastic e la libreria RadioLib.

---

## 1. Executive Summary

L'analisi del firmware ExpressLRS e dei file di target permette di stabilire con certezza l'architettura hardware del modulo RadioMaster Nomad:

### Cosa è Certo (Evidenza nel Codice e Target JSON)
* **Architettura Dual Radio:** Il modulo monta **due chip transceiver Semtech LR1121** collegati a un bus SPI condiviso (SCK: 25, MOSI: 32, MISO: 33). Ciascun chip ha pin dedicati per NSS, BUSY, RESET e DIO1 (IRQ).
* **Controllo della Potenza tramite DAC (`power_apc2`):** Il controllo del guadagno del Power Amplifier (PA) esterno avviene tramite il convertitore analogico-digitale (DAC) integrato dell'ESP32 sul **GPIO 26**.
* **Gestione degli RF Switch Integrata nel Chip:** Non esistono pin GPIO dell'MCU dedicati all'abilitazione di TX/RX (quali `TX_EN` o `RX_EN`) o al controllo di switch d'antenna esterni. Il percorso RF (TX/RX switch e LNA) è pilotato interamente e automaticamente dai pin **DIO interni del chip LR1121** (DIO5-DIO8) tramite comandi dedicati inviati via SPI.
* **Controllo Ventola Digitale:** La ventola è pilotata in modalità On/Off (non PWM) tramite un transistor collegato al **GPIO 2**.
* **Pulsanti e LED:** Sono presenti due pulsanti fisici (GPIO 14 e 12) e una linea dati per LED RGB indirizzabili WS2812 (GPIO 22).

### Cosa è Probabile (Inferenza Logica Altamente Affidabile)
* Il chip LR1121 invia i segnali RF a un front-end esterno che include un PA (Power Amplifier) e un LNA (Low Noise Amplifier). La tensione analogica fornita dal DAC (`power_apc2` su GPIO 26) regola il bias o la tensione di alimentazione del PA, permettendo di controllare la potenza effettiva trasmessa.

### Cosa NON è Deducibile Solamente dalla Repository
* La disposizione geometrica e la mutua interazione delle antenne (se ottimizzate per bande diverse come 2.4GHz e 868/915MHz, oppure se entrambe multibanda).
* La piedinatura esatta dei moduli RF fisici o i dettagli del circuito di alimentazione del PA.

---

## 2. File Analizzati

I file chiave esaminati all'interno della codebase ExpressLRS per questa analisi sono:

1. **Target di Configurazione:**
   * **[`targets.json`](file:///Users/roberto/Developer/git/ExpressLRS-Targets/targets.json)** (Righe 3571-3579): Definisce il prodotto Nomad, la piattaforma (ESP32) e il firmware associato (`Unified_ESP32_LR1121_TX`).
   * **[`Radiomaster Nomad.json`](file:///Users/roberto/Developer/git/ExpressLRS-Targets/TX/Radiomaster%20Nomad.json)** (50 righe): Contiene la mappatura completa dei pin GPIO hardware e i parametri di potenza.

2. **Firmware Core ExpressLRS:**
   * **[`Unified_ESP32_TX.h`](file:///Users/roberto/Developer/git/ExpressLRS/src/include/target/Unified_ESP32_TX.h)** (Righe 1-143): Mappa le definizioni del file JSON di layout del target alle macro del firmware.
   * **[`POWERMGNT.cpp`](file:///Users/roberto/Developer/git/ExpressLRS/src/lib/POWERMGNT/POWERMGNT.cpp)** (Righe 257-286): Gestisce la logica del DAC analogico (`power_apc2`) per impostare i livelli di potenza.
   * **[`LR1121_hal.cpp`](file:///Users/roberto/Developer/git/ExpressLRS/src/lib/LR1121Driver/LR1121_hal.cpp)** (Righe 1-206): Implementa il livello di astrazione hardware SPI, Reset e Busy per i due chip LR1121.
   * **[`LR1121.cpp`](file:///Users/roberto/Developer/git/ExpressLRS/src/lib/LR1121Driver/LR1121.cpp)** (Righe 1-800): Implementa il driver Semtech per l'LR1121, inclusa la configurazione degli RF switch interni e del PA.
   * **[`RFAMP_hal.cpp`](file:///Users/roberto/Developer/git/ExpressLRS/src/lib/RFAMP/RFAMP_hal.cpp)** (Righe 1-228): Dimostra che per il Nomad non vengono configurati pin GPIO dell'MCU per abilitare TX/RX, poiché non presenti nel JSON.
   * **[`devThermal.cpp`](file:///Users/roberto/Developer/git/ExpressLRS/src/lib/THERMAL/devThermal.cpp)** (Righe 1-257): Gestisce l'abilitazione e isteresi della ventola.
   * **[`common.cpp`](file:///Users/roberto/Developer/git/ExpressLRS/src/src/common.cpp)** (Righe 236-239): Definisce che il modulo è rilevato come dual radio a runtime se `NSS_2` è definito.

---

## 3. Tabella Pin RadioMaster Nomad

| Funzione Logica | GPIO Fisico | Radio / Comune | Direzione I/O | Stato al Boot | Quando / Come cambia | Fonte File & Riga |
| :--- | :---: | :---: | :---: | :---: | :--- | :--- |
| **`serial_rx`** | **4** | Comune | Input | Floating | Porta seriale CRSF (Half-Duplex) | [`Nomad.json:2`](file:///Users/roberto/Developer/git/ExpressLRS-Targets/TX/Radiomaster%20Nomad.json#L2) |
| **`serial_tx`** | **4** | Comune | Output | - | Porta seriale CRSF (Half-Duplex) | [`Nomad.json:3`](file:///Users/roberto/Developer/git/ExpressLRS-Targets/TX/Radiomaster%20Nomad.json#L3) |
| **`radio_sck`** | **25** | Comune | Output | LOW | Clock del bus SPI condiviso | [`Nomad.json:7`](file:///Users/roberto/Developer/git/ExpressLRS-Targets/TX/Radiomaster%20Nomad.json#L7) |
| **`radio_mosi`**| **32** | Comune | Output | LOW | MOSI del bus SPI condiviso | [`Nomad.json:6`](file:///Users/roberto/Developer/git/ExpressLRS-Targets/TX/Radiomaster%20Nomad.json#L6) |
| **`radio_miso`**| **33** | Comune | Input | - | MISO del bus SPI condiviso (con pullup) | [`Nomad.json:5`](file:///Users/roberto/Developer/git/ExpressLRS-Targets/TX/Radiomaster%20Nomad.json#L5) |
| **`radio_nss`** | **27** | Radio 1 | Output | HIGH | Selezione SPI attiva `LOW` | [`Nomad.json:11`](file:///Users/roberto/Developer/git/ExpressLRS-Targets/TX/Radiomaster%20Nomad.json#L11) |
| **`radio_rst`** | **15** | Radio 1 | Output | HIGH | Portato `LOW` per 1ms durante reset | [`Nomad.json:12`](file:///Users/roberto/Developer/git/ExpressLRS-Targets/TX/Radiomaster%20Nomad.json#L12) |
| **`radio_busy`**| **36** | Radio 1 | Input | - | Monitorato dall'MCU, alto durante elaborazioni | [`Nomad.json:9`](file:///Users/roberto/Developer/git/ExpressLRS-Targets/TX/Radiomaster%20Nomad.json#L9) |
| **`radio_dio1`**| **37** | Radio 1 | Input | - | Interrupt su fronte di salita (`RISING`) | [`Nomad.json:10`](file:///Users/roberto/Developer/git/ExpressLRS-Targets/TX/Radiomaster%20Nomad.json#L10) |
| **`radio_nss_2`**| **13** | Radio 2 | Output | HIGH | Selezione SPI della Radio 2, attiva `LOW` | [`Nomad.json:16`](file:///Users/roberto/Developer/git/ExpressLRS-Targets/TX/Radiomaster%20Nomad.json#L16) |
| **`radio_rst_2`**| **21** | Radio 2 | Output | HIGH | Reset Radio 2, portato `LOW` per 1ms | [`Nomad.json:17`](file:///Users/roberto/Developer/git/ExpressLRS-Targets/TX/Radiomaster%20Nomad.json#L17) |
| **`radio_busy_2`**| **39**| Radio 2 | Input | - | Monitorato dall'MCU per Radio 2 | [`Nomad.json:14`](file:///Users/roberto/Developer/git/ExpressLRS-Targets/TX/Radiomaster%20Nomad.json#L14) |
| **`radio_dio1_2`**| **34**| Radio 2 | Input | - | Interrupt della Radio 2 (`RISING`) | [`Nomad.json:15`](file:///Users/roberto/Developer/git/ExpressLRS-Targets/TX/Radiomaster%20Nomad.json#L15) |
| **`power_apc2`**| **26** | Comune | DAC Out | 0V | Scritto con `dacWrite()` in base alla potenza | [`Nomad.json:22`](file:///Users/roberto/Developer/git/ExpressLRS-Targets/TX/Radiomaster%20Nomad.json#L22) |
| **`misc_fan_en`**| **2**  | Comune | Output | LOW | Portato `HIGH` per avviare la ventola | [`Nomad.json:49`](file:///Users/roberto/Developer/git/ExpressLRS-Targets/TX/Radiomaster%20Nomad.json#L49) |
| **`led_rgb`**   | **22** | Comune | Output | - | Segnale dati per striscia di LED WS2812 | [`Nomad.json:33`](file:///Users/roberto/Developer/git/ExpressLRS-Targets/TX/Radiomaster%20Nomad.json#L33) |
| **`button`**    | **14** | Comune | Input | Pullup | Pulsante 1 di boot/configurazione | [`Nomad.json:38`](file:///Users/roberto/Developer/git/ExpressLRS-Targets/TX/Radiomaster%20Nomad.json#L38) |
| **`button2`**   | **12** | Comune | Input | Pullup | Pulsante 2 di boot/configurazione | [`Nomad.json:39`](file:///Users/roberto/Developer/git/ExpressLRS-Targets/TX/Radiomaster%20Nomad.json#L39) |

---

## 4. Gestione LR1121

L'inizializzazione e il controllo dei due chip LR1121 nel modulo Nomad avvengono tramite un driver unificato in `LR1121.cpp`.

### Inizializzazione (Init Sequence)
In `LR1121Driver::Begin` (righe 108-160):
1. Inizializzazione dell'HAL (`hal.init()`), che imposta le modalità dei pin GPIO (NSS, BUSY, DIO1, SPI) e configura gli interrupt su DIO1 (`dioISR_1` e `dioISR_2`).
2. Esecuzione del reset hardware di entrambi i chip radio (`hal.reset()`).
3. Verifica della presenza e versione firmware di ciascun chip tramite `CheckVersion(SX12XX_Radio_1)` e, se `NSS_2` è definito, `CheckVersion(SX12XX_Radio_2)`.
4. Invio dei comandi di configurazione di sistema Semtech a entrambi i chip (via `SX12XX_Radio_All`):
   * `SetRxTxFallbackMode` impostato su **Frequency Synthesizer (FS) mode** (`LR11XX_RADIO_FALLBACK_FS`).
   * `SetRxBoosted` abilitato (`1`) per massimizzare la sensibilità LNA in ricezione.
   * `SetDioAsRfSwitch` per attivare i pin DIO interni del chip come switch automatici di antenna.
   * `SetRegMode` abilitato per attivare il convertitore switching DC-DC interno (`radio_dcdc: true`) al posto del regolatore LDO, ottimizzando i consumi.
   * `CalibrateImage` in base alla banda di frequenza impostata.

### Reset Hardware
Nel file `LR1121_hal.cpp` (righe 79-121):
* I pin di reset `GPIO_PIN_RST` (15) e `GPIO_PIN_RST_2` (21) vengono portati `LOW` per 1 millisecondo.
* Se si forza il bootloader, i pin `BUSY` (36) e `BUSY_2` (39) vengono impostati temporaneamente come `OUTPUT` e tenuti `LOW` durante il reset.
* I pin di reset vengono riportati `HIGH`.
* Segue un **ritardo di 300 millisecondi** (`delay(300)`). Questo ritardo è cruciale poiché il pin BUSY del chip LR1121 rimane alto per circa 230ms subito dopo il reset per caricare il firmware interno prima di poter accettare nuovi comandi SPI.

### Gestione dei Pin di Segnale
* **Busy Pin Polling:** In `WaitOnBusy` (righe 159-191), l'MCU controlla `GPIO_PIN_BUSY` (e `GPIO_PIN_BUSY_2`) attendendo che tornino `LOW`. Il timeout è fissato a 2000 microsecondi (2ms).
* **DIO1 / IRQ:** I pin di interrupt `GPIO_PIN_DIO1` (37) e `GPIO_PIN_DIO1_2` (34) sono configurati come `INPUT`. Gli interrupt sono associati a `RISING` ed attivati da eventi come la fine della trasmissione o ricezione di un pacchetto.
* **NSS/SPI:** SPI hardware condiviso su SCK (25), MOSI (32) e MISO (33) (configurato con pullup). L'ESP32 gestisce il CS in hardware tramite `spiAttachSS()` assegnando l'indice 0 a NSS (27) e l'indice 1 a NSS_2 (13).

---

## 5. Gestione PA / power_apc / power_apc2

Il pin `power_apc2` è un pin critico per il corretto funzionamento dello stadio trasmittente ad alta potenza.

### Funzionamento del Pin DAC
In `POWERMGNT.cpp` (righe 257-286), la potenza di trasmissione viene impostata in base all'indice di potenza selezionato:
* Se `POWER_OUTPUT_DACWRITE` è attivo, il firmware chiama:
  ```cpp
  dacWrite(GPIO_PIN_RFamp_APC2, SAFE_GET_POWER_VALUE(POWER_OUTPUT_VALUES, powerIdx));
  ```
* Nello specifico, per il Nomad:
  * Il pin `power_apc2` corrisponde al **GPIO 26** dell'ESP32.
  * Il target definisce `"power_values": [120, 120, 120, 120, 120, 120, 95]`.
  * Questo significa che per i primi 6 livelli di potenza (fino a 500mW), il DAC emette un valore analogico pari a **120** (circa $1.55\text{V}$ su una scala di $3.3\text{V}$ e 255 livelli).
  * Al massimo livello di potenza (1000mW), il valore DAC viene abbassato a **95** (circa $1.23\text{V}$).
  * Contestualmente alla variazione del DAC, la potenza interna del chip LR1121 viene variata tramite il comando SPI `SetTxParams` utilizzando i valori di `"power_values2"` o `"power_values_dual"`, che vanno da **-18dBm a +5dBm**.

### Ruolo Elettronico del Pin
La tensione analogica generata sul GPIO 26 controlla il bias di guadagno (APC, *Automatic Power Control*) dell'amplificatore di potenza RF esterno.
> [!IMPORTANT]
> Se il pin GPIO 26 viene mantenuto a 0V, il PA esterno risulterà non polarizzato (o spento), bloccando di fatto la propagazione ad alta potenza del segnale RF (o introducendo una fortissima attenuazione e distorsione del segnale nativo dell'LR1121).

### Sicurezza nei Test
* Per effettuare test a bassissima potenza senza rischiare di danneggiare il trasmettitore o le antenne, **è sicuro lasciare spento o scollegato il DAC** (tensione 0V su GPIO 26).
* Tuttavia, per testare la trasmissione effettiva del segnale anche a livelli minimi (ad es. 10mW), il DAC **deve** essere impostato sul valore `120` per polarizzare correttamente lo stadio amplificatore.

---

## 6. Gestione RF Switch / DIO

A differenza di altri moduli trasmettitori ExpressLRS, il Nomad **non ha GPIO dell'MCU adibiti al controllo del percorso RF**. 

### Configurazione dei DIO interni dell'LR1121
Nel file `LR1121.cpp` (righe 284-311), la funzione `SetDioAsRfSwitch` invia al chip il comando SPI `LR11XX_SYSTEM_SET_DIO_AS_RF_SWITCH_OC`:
* Poiché nel file JSON del Nomad **non** è definito l'array `"radio_rfsw_ctrl"`, il firmware utilizza i **valori di default** interni:
  * **`switchbuf[0] = 0b00001111;`** (RfswEnable): Abilita i pin DIO5, DIO6, DIO7, DIO8 del chip LR1121 per funzionare in modalità RF switch automatico controllato dallo stato operativo interno del transceiver.
  * **`switchbuf[1] = 0b00000000;`** (RfSwStbyCfg): Nessun pin attivo in modalità Standby.
  * **`switchbuf[2] = 0b00000100;`** (RfSwRxCfg): **DIO7** è portato `HIGH` quando il chip è in ricezione (RX).
  * **`switchbuf[3] = 0b00001000;`** (RfSwTxCfg): **DIO8** è portato `HIGH` quando il chip è in trasmissione (TX) a bassa potenza.
  * **`switchbuf[4] = 0b00001000;`** (RfSwTxHPCfg): **DIO8** è portato `HIGH` quando il chip è in trasmissione (TX) ad alta potenza.
  * **`switchbuf[5] = 0b00000010;`** (RfSwTxHfCfg): **DIO6** è portato `HIGH` quando il chip trasmette ad alta frequenza (2.4GHz).
  * **`switchbuf[7] = 0b00000001;`** (RfSwWifiCfg): **DIO5** è portato `HIGH` in ricezione ad alta frequenza (2.4GHz).

### Logica del Percorso RF
Grazie a questa configurazione Semtech nativa, il chip LR1121 commuta in modo autonomo e a livello hardware i segnali di controllo degli switch d'antenna e di abilitazione LNA/PA esterni a seconda dello stato in cui si trova (TX o RX) e della banda di frequenza utilizzata (900MHz o 2.4GHz), eliminando la necessità di qualsiasi intervento dell'MCU ESP32 a livello di GPIO digitali in fase di commutazione TX/RX.

---

## 7. Implicazioni per Porting Meshtastic / RadioLib

Per poter far funzionare il modulo RadioMaster Nomad all'interno del firmware Meshtastic utilizzando la libreria RadioLib, occorre configurare correttamente il file `variant.h` (o file di configurazione equivalente) e implementare alcune inizializzazioni software specifiche.

### 1. Configurazione dei Pin in `variant.h`
```cpp
// Bus SPI condiviso
#define SCK_PIN        25
#define MOSI_PIN       32
#define MISO_PIN       33

// Chip 1 (Banda Primaria o Singolo Uso)
#define NSS_PIN        27
#define RST_PIN        15
#define BUSY_PIN       36
#define DIO1_PIN       37

// Chip 2 (secondo LR1121 - da tenere spento se non supportato)
#define NSS_PIN_2      13
#define RST_PIN_2      21
#define BUSY_PIN_2     39
#define DIO1_PIN_2     34

// Controllo ventola
#define FAN_EN_PIN     2

// Controllo Potenza PA (DAC)
#define PA_APC_DAC_PIN 26

// Pulsanti e LED
#define BUTTON_1_PIN   14
#define BUTTON_2_PIN   12
#define WS2812_LED_PIN 22
```

### 2. Parti da NON Inserire
* Non definire alcun pin di abilitazione PA o TX/RX dell'MCU (come `PA_EN`, `TX_EN`, `RX_EN`, `LNA_EN`) in quanto non esistono fisicamente collegati all'ESP32.
* Evitare di configurare il secondo reset (`RST_2` su GPIO 21) come input o floating, altrimenti la seconda radio potrebbe oscillare o avviarsi in modo imprevisto disturbando il bus SPI.

### 3. Come Tenere Spento il Secondo LR1121
Dato che Meshtastic non supporta nativamente la modalità Gemini dual-band a due chip contemporanei, è consigliabile utilizzare **soltanto il primo chip LR1121** (NSS: 27) e disattivare completamente il secondo per risparmiare energia e scongiurare conflitti sul bus SPI condiviso.
All'avvio (`setup()`):
```cpp
// Spegne la seconda radio LR1121
pinMode(RST_PIN_2, OUTPUT);
digitalWrite(RST_PIN_2, LOW); // Tiene la Radio 2 in stato di reset permanente

pinMode(NSS_PIN_2, OUTPUT);
digitalWrite(NSS_PIN_2, HIGH); // Disattiva l'interfaccia SPI della Radio 2
```

### 4. Come Configurare `power_apc2` (DAC)
Senza la corretta polarizzazione del PA sul GPIO 26, la portata del Nomad sarà minima o nulla.
Nel firmware di porting, subito dopo l'avvio, configurare l'uscita DAC a un valore fisso per polarizzare l'amplificatore per potenze di trasmissione sicure (es. 100mW o 500mW):
```cpp
// Inizializza il bias del PA sul DAC
#include <driver/dac.h>

void initPA() {
    // Abilita il DAC1 sul canale 2 (GPIO 26)
    dac_output_enable(DAC_CHANNEL_2); 
    
    // Imposta la tensione di bias a ~1.55V (valore 120 su 255)
    dac_output_write(DAC_CHANNEL_2, 120); 
}
```
*Se si desidera spegnere completamente lo stadio di potenza nei momenti di idle/sleep, si può chiamare `dac_output_write(DAC_CHANNEL_2, 0);` o disabilitare il DAC.*

### 5. Configurazione RF Switch Table (RadioLib)
Poiché il percorso RF è guidato dai pin DIO dell'LR1121, **è tassativo** istruire RadioLib a abilitare la commutazione automatica interna all'avvio dell'LR1121. In RadioLib, questo si ottiene invocando la funzione di configurazione dell'RF switch sul chip:
```cpp
// Codice di inizializzazione per RadioLib (SX128x / LR11xx)
// Abilita il controllo automatico degli switch d'antenna sui pin DIO5-DIO8
int state = radio.setDioAsRfSwitch(true);
if (state != RADIOLIB_ERR_NONE) {
    // Gestione errore
}
```
Questo comando invierà i byte corretti (`0b00001111` per l'abilitazione e le relative maschere RX/TX sui pin da DIO5 a DIO8) configurando lo switch RF in modo identico a ExpressLRS.

### 6. Cosa Verificare con la Strumentazione (Oscilloscopio / Logic Analyzer)
In caso di problemi durante il porting, è consigliabile monitorare i seguenti punti fisici sulla scheda:
1. **Segnale sul GPIO 26 (`power_apc2`):** Verificare con un multimetro o oscilloscopio che all'avvio la tensione salga stabilmente a circa $1.5\text{V}$ (bias del PA).
2. **Bus SPI condiviso:** Monitorare che `NSS_2` (GPIO 13) rimanga costantemente `HIGH` (3.3V) e non si abbassi mai, per escludere che il secondo chip (se disattivato) risponda sul bus.
3. **Pin Busy e Reset della Radio 1 (GPIO 36 e 15):** Controllare che la linea di reset scenda per 1ms e che il pin BUSY rimanga alto per circa 230ms prima di tornare stabilmente basso, confermando il corretto caricamento del firmware interno all'avvio.

---

## 8. Comandi Usati per la Ricerca

Per condurre questa indagine approfondita all'interno dei repository `ExpressLRS` e `ExpressLRS-Targets`, sono stati utilizzati i seguenti comandi shell e filtri `ripgrep` (`rg`):

```bash
# 1. Ricerca del target Nomad nei file JSON dei target
rg -i "nomad" targets.json
cat TX/Radiomaster\ Nomad.json

# 2. Ricerca di occorrenze di Nomad e LR1121 nel firmware
rg -i "nomad" src/
rg -n "Unified_ESP32_LR1121_TX" src/

# 3. Ricerca del pin power_apc2 e della logica del DAC
rg -n "power_apc2" src/
rg -n "GPIO_PIN_RFamp_APC2" src/
rg -n "POWER_OUTPUT_DACWRITE" src/

# 4. Ricerca dei driver dell'LR1121 e configurazione RF switch
rg -n "void LR1121" src/
rg -n "SetDioAsRfSwitch" src/
rg -n "SetPaConfig" src/
rg -n "SetTxParams" src/

# 5. Analisi del controllo dual-radio e del secondo reset
rg -n "isDualRadio" src/
rg -n "GPIO_PIN_NSS_2" src/
rg -n "GPIO_PIN_RST_2" src/

# 6. Ricerca del pin e della gestione termica della ventola
rg -n "GPIO_PIN_FAN_EN" src/
rg -n "timeoutFan" src/
```
