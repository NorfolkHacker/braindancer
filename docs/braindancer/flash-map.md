# Braindancer flash map (M-VAVE FM-1, 1 MiB, JEDEC 0x856014)

Measured on 2026-10-24 against this tree (branch `piece2-trim`) and the piece 1 full-flash backup
`~/fm1-backups/2026-10-24-jiant/fm1-recovery-20261024-130125/full-flash-before.bin` (1 MiB, sha256 `8531fc3a...c5349`,
taken while the device ran JIANT 0.9.9-beta4a, which had replaced the official V15). The backup was read as data only
(`python3 -I`, plain file reads); nothing in it was executed. Sectors are 4 KiB; "used" means not all `0xFF`.

Sources in the tree: `firmware/hal/fm1_flash.h` (`FL_*`, `FL_RANGE_OK`), `firmware/hal/fm1_flash_map.h`,
`firmware/src/storage.c` (flash-map comment, `st_sector`), `firmware/src/ota.c`, `firmware/loader/{loader.c,ldr_core.c}`,
`tools/fm1pkg_make.py` (JieLi region table, line 143 ff.). A grep of `firmware tools web` for every address constant in
the three gaps finds only the places cited below; no other source names them.

## The whole flash

| Start | End (incl.) | Size | Owner | Written by | Backup: used / sectors |
|---|---|---|---|---|---|
| 0x00000 | 0x03FFF | 16 KiB | JieLi head: SPL (uboot.boot), isd_config.ini (chip key), JLFS directory | factory; the update loader never writes it (`ldr_core.c`) | 4 / 4 |
| 0x04000 | 0x92FFF | 588 KiB | app region (encrypted): app slot 0x8EA90, cfg_tool.bin, cfg dir | update loader (`LDR_APP_LO..HI`) | 143 / 143 |
| **0x93000** | **0x96FFF** | 16 KiB | **gap 1** (nothing in this tree) | nothing; the update loader may erase it (below) | 0 / 4 |
| 0x97000 | 0x9EFFF | 32 KiB | projects / song 1, A+B of 4 sections (`OBJ_PROJECT0`) | `storage.c` | 0 / 8 |
| 0x9F000 | 0x9FFFF | 4 KiB | user-preset FM6 patches, copy A (`OBJ_UPFM6`) | `storage.c` | 0 / 1 |
| 0xA0000 | 0xB7FFF | 96 KiB | songs 2..4 (`OBJ_SONG1..`) | `storage.c` | 0 / 24 |
| 0xB8000 | 0xD7FFF | 128 KiB | sample area, `FL_SMP_LO..HI` (songs 5..8 of JIANT) | nothing yet (piece 4) | 0 / 32 |
| 0xD8000 | 0xD9FFF | 8 KiB | song index A/B (`OBJ_SONGIDX`) | `storage.c` | 0 / 2 |
| **0xDA000** | **0xDBFFF** | 8 KiB | **gap 2** (documented "free" in `storage.c`) | nothing | 0 / 2 |
| 0xDC000 | 0xDFFFF | 16 KiB | user preset banks (`OBJ_UPRESET0`, upreset.c) | `storage.c` | 0 / 4 |
| 0xE0000 | 0xE4FFF | 20 KiB | M-UPGRADE loader staging; update record slot 0xE4F00 (`OTA_AREA`, `OTA_RES`) | `ota.c` | 0 / 5 |
| 0xE5000 | 0xE6FFF | 8 KiB | autosave A/B (`OBJ_AUTOSAVE`) | `storage.c` | 1 / 2 (0xE6000: see below) |
| 0xE7000 | 0xE7FFF | 4 KiB | FM6 voice bank (`FL_VBANK`) | `fm6_vbank.c` | 0 / 1 |
| **0xE8000** | **0xE8FFF** | 4 KiB | **gap 3a**: stock VM header; stock update record slot 0xE8F00 | stock V15 | 1 / 1 |
| **0xE9000** | **0xE9FFF** | 4 KiB | **gap 3b**: JieLi `BTIF` region (10 bytes used) | stock V15 | 1 / 1 |
| **0xEA000** | **0xF1FFF** | 32 KiB | **gap 3c**: stock tone bank, two identical 16 KiB copies | factory / stock V15 | 8 / 8 |
| **0xF2000** | **0xF9FFF** | 32 KiB | **gap 3d**: inside the stock `USR` region, never written in the backup | unknown | 0 / 8 |
| **0xFA000** | **0xFBFFF** | 8 KiB | **gap 3e**: stock `USR` data | stock V15 | 2 / 2 |
| 0xFC000 | 0xFDFFF | 8 KiB | Felucca settings A/B (`FELU` magic) | `storage.c` | 2 / 2 |
| 0xFE000 | 0xFEFFF | 4 KiB | user-preset FM6 patches, copy B (`FELU`) | `storage.c` | 1 / 1 |
| 0xFF000 | 0xFFFFF | 4 KiB | `key_mac` (JLFS head entry, offset 0xFF000, 0x1000) | factory | 0 / 1 |

Census check: gap 1 0/4, gap 2 0/2, 0xE8000..0xFBFFF 12/20 used (E8, E9, EA..F1, FA, FB), 0xE5000..0xE7FFF 1/3,
0xFC000..0xFFFFF 3/4 all match the quick census.

## What the JieLi region table says

`tools/fm1pkg_make.py:143` writes these descriptors into the app area head (decoded):

| Name | Offset | Size | Covers |
|---|---|---|---|
| `VM` | 0x93000 | 352256 (0x56000) | 0x93000..0xE8FFF |
| `PRCT`/`CODE` | 0 | whole flash | everything |
| `BTIF`/`AUTO` | 0xE9000 | 0x1000 | 0xE9000..0xE9FFF |
| `USR` | 0xEA000 | 0x12000 | 0xEA000..0xFBFFF |

So the three "gaps" are not unowned in the vendor layout. Gaps 1 and 2 and the whole of Felucca's store lie inside the
SDK's `VM` area (the SDK's non-volatile config store); gap 3 is its tail (`E8000`), `BTIF` and `USR`. JIANT already
uses most of `VM` (0x97000..0xE7FFF) as its own store, so using more of it adds no new kind of exposure. The table lives
in the encrypted app area; Braindancer is freestanding (`tools/build.py` takes only the SPL, cfg_tool.bin and eq_cfg_hw.bin
from the SDK) and nothing in `firmware/` reads or acts on the table. Whether the boot ROM / SPL parses it is not
documented in this tree. kagaimiq/jl-uboot-tool (https://github.com/kagaimiq/jl-uboot-tool) does not document the VM
area or update records on its front page; its docs/ folder was not read.

## What writes where: the update loader

- `firmware/loader/loader.c` limits the RAM driver to `FL_RANGE_OK = [0x4000,0x97000) + [0xE0000,0xFC000)`.
- `ldr_core.c` after a successful update scans, for `s` from `LDR_REC_HI 0xFC000` down to (excluding) `LDR_REC_LO 0x93000`
  in 0x1000 steps, the 80 bytes at `s - 0x100`: the last 256 bytes of the sector below `s`, so the slot offset inside
  each sector is 0xF00. That is 105 candidate slots (`0x93F00`, `0x94F00`, ... `0xFBF00`), 73 of them skipped
  (sectors inside the store window below), 32 read. The scan `break`s (does not `continue`) on a flash read error. It erases the whole sector below any 80-byte record that looks like an
  update record (bytes 6..7 = `0x5441`, first CRC16 matches bytes 2..79). It skips only `[LDR_DATA_LO 0x97000,
  LDR_DATA_HI 0xE0000)`, "Felucca's store".
- Consequences: gap 2 and the sample area are inside the skipped window, so the loader never erases them. Gap 1
  (0x93000..0x96FFF) and everything from 0xE0000 up are not skipped, so the loader would erase a gap-1 sector whose
  `+0xF00` bytes happen to look like a record. The same record format is what the SPL looks for at power-on, which is
  why `fm1pkg_make.py` refuses an image with a look-alike. In the backup no such look-alike exists anywhere in
  0x93000..0xFBFFF (checked at every candidate slot).
- `ota.c` writes only `0xE0000..0xE4FFF` (staging; record at `0xE4F00`) and erases it again at boot.
- `storage.c` writes only the sectors in the table above.

## Gap 1: 0x93000..0x96FFF (16 KiB) - verdict: unknown

Evidence: erased (`0xFF`) in all 4 sectors of the backup. No address constant, write or read in `firmware/`, `web/` or
`tools/` other than the loader/record-scan above and the `VM` descriptor (start of the SDK `VM` area, not read by us).
The stock V15 left it empty in the backup. That is weak evidence: it is one lightly used device, and the backup was
taken after V15 had been replaced by JIANT. JIANT's install does not erase this range, so V15 probably never wrote it
on this unit, but a heavily used V15 might.

Why not "free": the shipped update loader can erase it (record scan above, window starts at 0x97000, not 0x93000), and
the SPL's own record scan covers it. That is code we ship touching it, so the strict criterion fails today. Also it is
a 16 KiB island between the app region and the projects, not contiguous with the sample area, so its value is small.

What would settle it: lower `LDR_DATA_LO` to 0x93000 in `ldr_core.c` (so the loader skips it like the rest of the
store) and, in piece 4, have the sample writer refuse a sector whose `+0xF00` 80 bytes look like a record (CRC16/`AT`
magic: probability about 2^-32 per sector for random data: negligible but cheap to guard, so guard it). Then it is free on the same footing as
the rest of `VM`. Official V15, if the user goes back to stock, may use `VM` (it is the SDK config store); in the
backup it did not use this part.

## Gap 2: 0xDA000..0xDBFFF (8 KiB) - verdict: free, with the SPL-record guard every sample sector needs

Evidence: erased in both sectors of the backup. `storage.c` documents it free ("0xDA000..0xDBFFF free"), the only
tree references are that comment and the `_Static_assert` family that keeps the song objects below the sample area. It is
inside `[LDR_DATA_LO, LDR_DATA_HI)` so the update loader skips it and `FL_RANGE_OK` of the loader cannot reach it, and
`ota.c` does not touch it. Between the song index (0xD8000..0xD9FFF) and the user preset banks (0xDC000), so no
neighbouring object spills over: `st_sector()` computes its addresses from object numbers and none lands here
(`OBJ_SONGIDX` 0xD8000/0xD9000, `OBJ_UPRESET0` from 0xDC000).
Stock V15: lies in the SDK `VM` area, like the 0x97000..0xE7FFF Felucca store that JIANT already overwrites; the backup
shows no sign of V15 using it on this lightly used unit (not proof for others). If a user goes back to stock, V15 would find sample bytes where it expects erased VM
sectors; the same is already true for every JIANT project, so this adds no new class of harm, and it does not matter for
what we run.

SPL caveat: the loader's skip window protects against the loader only. The SPL's own power-on scan for an update
record (the same 80-byte record at every 4K-256 slot, see "The update-record guard") is not shown to ignore
0x97000..0xE0000; that Felucca's store lives there and works is circumstantial. Gap 2 has two slots, 0xDAF00 and
0xDBF00. It is judged free only with that guard.

JIANT/Felucca: JIANT (and Felucca) use 0xDA000..0xDBFFF as free by comment only; nothing of theirs writes it, so a
device that goes back to JIANT loses nothing there, but JIANT would not protect samples stored there either (see
"JIANT and Felucca" below).

Caveat for piece 4: it is a separate 8 KiB extent (the song index separates it from 0xB8000..0xD7FFF), not part of the
128 KiB contiguous run. The controller must show it to the user before piece 4 uses it (spec 6.1).

## Gap 3: 0xE8000..0xFBFFF (80 KiB) - verdict: not free (owner: the official V15 firmware), one part unknown

Per the backup, 12 of 20 sectors are in use, by data nothing we ship reads:

- **0xE8000** (1 sector): `55 AA AA 55 91 6A 20 00 ...`, 150 non-`FF` bytes, all within the first 173 bytes (offsets 0..172), the rest `FF`. A header with magic
  `0x55AAAA55` followed by a short record; looks like a JieLi VM/system-config header (last sector of `VM`). Its
  `+0xF00` slot (0xE8F00) is the stock firmware's update-record location, as `ldr_core.c` notes ("the stock firmware's:
  0xE8F00"); it is erased now. The loader erases this sector if a record appears there. Owner: official V15 / SDK VM.
- **0xE9000** (1 sector): 10 bytes `95 66 60 00 38 EB 65 69 14 31` then `FF`; the region table names this `BTIF`.
  Owner: stock Bluetooth/identity data. Unknown content; could be a device address.
- **0xEA000..0xF1FFF** (8 sectors): two byte-identical 16 KiB copies (sector hashes EA..ED equal EE..F1). Content is
  FM patch records of 128 B with ASCII names ("PIANO 1", "ORGAN 1", "SYN LEAD 1", ... "BASS 8", "BRASS 1", "WOODWIND 1",
  "STRING 1", "VOICE 1", "SAW EM UP", "KALIMBA", "TIMPANI", ..., "WHAT IF"): the official firmware's factory tone
  bank, duplicated for safety. Nothing in Braindancer reads it (Braindancer has its own presets and FM6 banks).
  Not free: erasing it destroys stock V15's factory sounds, which a user going back to stock needs; the user's backup
  is the only copy.
- **0xF2000..0xF9FFF** (8 sectors, 32 KiB): erased. Inside the stock `USR` region (0xEA000, 0x12000). Whether V15 uses it
  (e.g. when saving user tones or sequences) is not visible in this one backup.
- **0xFA000..0xFBFFF** (2 sectors): repeated 3-byte records `50 03 00 1E 1E ...` and `00 02 00 00 03 00 ... 08 00`,
  repeating with a period of 0x3B bytes: stock user/performance data (`USR`); no ASCII, no magic.

Other things seen: 0xE6000 (autosave B, 1 used sector) holds a JLFS head `LOADER.BIN` / `usb_hid_ota.bin` and
the stock OTA's staged loader header: a leftover of the stock firmware's own update, which stages into the autosave
margin (the `storage.c` comment). It is not in the gap and ACID's autosave treats it as no valid copy.

Verdicts per part:

| Part | Verdict | Why |
|---|---|---|
| 0xE8000..0xE9FFF | not free (official V15; loader erases a record there) | system header and `BTIF` |
| 0xEA000..0xF1FFF | not free (official V15 factory tone bank) | irreplaceable but for the backup |
| 0xFA000..0xFBFFF | not free (official V15 `USR` data) | in use |
| 0xF2000..0xF9FFF | unknown | erased, but inside the V15 `USR` region; settle with a second backup after exercising V15 (save user tones, sequences, settings) and diffing, or a disassembly of V15's `USR` writer |

If the user accepts losing the stock firmware's factory tones (restorable from the backup via the unbrick tool), the
0xEA000..0xF9FFF run becomes 80 KiB contiguous; that is the user's decision, not a fact about the flash.

## The update-record guard (applies to every sample sector)

The SPL looks for an update record at power-on in slots at `sector + 0xF00` (4K - 256). `fm1pkg_make.py:159` and
`ldr_core.c` both test the same 80-byte record `r` at that slot; it is taken for a record when all of:
- `r[0] | r[1] != 0` (the CRC16, little endian, is nonzero),
- `r[6] == 0x41 and r[7] == 0x54` (little-endian 16-bit `0x5441`, "AT"),
- `crc16(r[2:80]) == LE16(r[0:2])` (CRC-CCITT, poly 0x1021, init 0, MSB first, as `fm1pkg_make.crc16` / `ota_crc16(p, 78, 0)`).

For random data the chance is about 2^-32 per sector (2^-16 for the 2-byte magic times 2^-16 for the 2-byte CRC; the nonzero test barely changes it), negligible but cheap to guard, so guard it. The
SPL then runs the loader on a sample. This applies to every sector of 0xB8000..0xD7FFF (32 slots) and gap 2 (2 slots).
Requirement for piece 4, plainly: the sample writer must ensure no sample sector's `+0xF00` slot can be taken for an
update record. Either reject and alter any 256-byte tail block that matches the test above (flip a bit of the sample
data, which is harmless for audio; test on the data about to be written), or reserve the slot (do not store sample
data in the last 256 bytes of each sector). Which is better is not decided by evidence here; testing the slot is the
cheaper one. Whether the SPL ignores 0x97000..0xE0000 is unknown and should not be relied on.

## JIANT and Felucca

JIANT 0.9.x and Felucca store songs 5..8 at 0xB8000..0xD7FFF (`storage.c` flash-map comment, "songs 2..4 ... at
0xA0000..0xB7FFF", the other four in the sectors above) and Braindancer now reserves that range as the sample area.
- A device that has samples from Braindancer and goes back to JIANT: JIANT's songs 5..8 read those sectors as song objects
  (commit-record magic and CRCs fail, so they read as empty); the first time JIANT saves song 5..8 it erases and
  overwrites the sample sectors, so the breaks there are lost. Gap 2 is not used by JIANT, so samples stored only there
  survive a JIANT visit.
- A Braindancer installed over a JIANT that had songs 5..8: those songs' sectors are the sample area and contain valid
  JIANT song objects (`FELU` commit records). Piece 4's sample directory must treat foreign data there as empty/invalid
  (validate its own magic and CRC, never trust or crash on anything else), and the user should be told songs 5..8 are
  gone/ignored. The installer does not erase the range (the loader writes only the app area).
- The backup here had 0/32 sectors used in this range (JIANT songs not saved), so this is the code's behaviour, not
  something seen on the device.

## The sample area

Guaranteed: **128 KiB**, 0xB8000..0xD7FFF (32 sectors, contiguous, `FL_SMP_LO..HI`).
Judged free in addition: **8 KiB**, gap 2 at 0xDA000..0xDBFFF (separate extent). Gap 1 (16 KiB) and the gap-3 sectors
are not added. Total: 136 KiB, of which 128 KiB contiguous. Both extents are subject to the update-record guard above;
and 0xB8000..0xD7FFF is JIANT's song 5..8 store, so foreign data in it must read as empty.
