# BetaFPV SuperP 2.4GHz RX — riassunto per Roberto

> Versione italiana del riepilogo. La documentazione tecnica completa è in
> `README.md` (in inglese, per dev/upstream).

## In due righe

La porta Meshtastic per la SuperP **boota e funziona** (ESP32, BLE, WiFi, LED
verde, app dal cellulare). La **radio SX1280 NON funziona** sotto
Meshtastic/RadioLib. ExpressLRS sulla stessa scheda funziona perfettamente,
quindi non è un problema di pin, alimentazione o hardware: è il driver SX128x
generico di RadioLib che non sopporta come questa board si comporta sull'SPI.
Dopo molti tentativi (vedi sotto) abbiamo concluso che non si può chiudere a
livello di variant. **Ci fermiamo qui.**

## Cosa è stato consegnato (e funziona)

- Build clean, flash, BLE/WiFi/app: tutto OK
- LED RGB verde fisso come spia "acceso/vivo"
- Pin verificati 1:1 contro il layout ufficiale ExpressLRS
- Seconda SX1280 deselezionata correttamente al boot (hygiene)
- `default_envs` sistemato in forma a righe (quella con spazi dava errore)
- Macro `LORA_SPI_FREQUENCY` aggiunta al core (default 4 MHz, **nessun impatto
  su altre board**) per poter tarare il clock SPI LoRa per-target in futuro

## Cosa abbiamo escluso (con prove)

| Sospetto | Esito |
|---|---|
| Pin sbagliati | Combaciano al 100% col layout ELRS, ELRS funziona |
| Alimentazione/brownout | LiPo 2S, nessun brownout, `-2` lo stesso |
| Contesa bus dalla 2ª radio | NSS2 verificato a 1, errori sono `FF` (linea idle) non `00` (contesa) |
| Clock SPI troppo alto | Abbassato a 1 MHz, stessa garbage |
| Reset di RadioLib troppo corto | Allungato (delay 20+20 ms) — risolve la version-string ma non basta |
| Lasciar fare il reset a noi (`RADIOLIB_NC`) | Peggio: chip torna tutto `FF` (il reset di RadioLib serve) |
| Pin BUSY (GPIO37) non leggibile | Misurato: alto per ~1395 µs dopo reset, perfettamente normale |

## Cosa abbiamo capito alla fine

Patchando via via i check difensivi di RadioLib siamo arrivati a far passare la
`begin()` con `init result 0` (frequenza, bandwidth, potenza tutte impostate).
**Ma poi `setCRC` / `startReceive` fallisce lo stesso.** Il pattern è chiaro:

- Le **scritture** SPI sono accettate dal chip (risponde con status valido per
  quasi tutti i comandi).
- Le **letture** SPI (e lo status di certe scritture) tornano **`0xFF` in modo
  intermittente** — il chip smette di pilotare MISO a metà del burst.

ELRS funziona perché ha un driver SX1280 scritto a mano per *queste* board e
non fa o tollera quelle letture. Ogni protezione di RadioLib che disabilitiamo
fa emergere la successiva basata su un'altra read. **Stiamo solo costruendo un
firmware che mente a sé stesso**: anche forzando la `begin()` a tornare OK, la
RX (che dipende da letture di `IrqStatus`/`RxBufferStatus`/payload) resterebbe
inaffidabile.

Non è un problema risolvibile dal variant. È un problema di **integrazione
RadioLib ↔ board diversity SuperP** a livello di driver SPI.

## Cosa servirebbe per finirlo (in futuro)

Solo se davvero vuoi insistere — non è un pomeriggio di lavoro:

1. **Fork/patch upstream di RadioLib SX128x**: tolleranza ai `0xFF` spuri,
   ritry sulle letture, modalità "ELRS trust-and-go" per board diversity.
   Richiede iterazione con l'hardware (tu).
2. **Misure con oscilloscopio** su MISO durante letture consecutive: capire se
   è la seconda SX1280, le routing tracce, o gli AT2401C che alterano il
   timing. Senza questo si va a tentoni.
3. **Accettare il limite** (stato attuale): la radio non va, il resto sì.
   Documentato sia qui che in `README.md`.

## Lo stato della PR

Branch `claude/fix-betafpv-target-tlBdb` → PR #7 verso `develop`. Contiene
**solo** i fix solidi sopra (no diagnostici, no patch RadioLib sperimentali —
quelle restano nella storia git del branch se le vuoi rivedere).

Per fare il fast-forward su develop quando vuoi:

```bash
git fetch origin
git checkout develop
git merge --ff-only origin/claude/fix-betafpv-target-tlBdb
git push origin develop
```

Per riprovare le patch RadioLib sperimentali (a tuo rischio): ripristina dal
commit `fb35bd9` di questo branch lo script `radiolib_busy_patch.py` e
riabilita `extra_scripts` + i flag `RADIOLIB_DEBUG_*` nel `platformio.ini` del
target. Il `README.md` (inglese) ne spiega esattamente la motivazione.
