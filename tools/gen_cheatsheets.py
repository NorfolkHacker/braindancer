# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)
# Portions Copyright (C) 2026 NorfolkHacker
# (JIANT 0.6.7) the two cheat sheets: docs/cheatsheet/controls.html and sound.html (black, Chakra Petch, the thermal
# palette), then tools/render_cheatsheets.mjs makes the PNGs and PDFs:
#   python3 tools/gen_cheatsheets.py && node tools/render_cheatsheets.mjs
# (3c-1 task 10b) English only, Braindancer's own pages (the ux_*.c pages, their knob labels and MORE rows; docs/MANUAL.md,
# web/manual/data.js); JIANT named only on the credits line. Update them together.
import html, os, re

R = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(R, 'docs', 'cheatsheet')
VERSION = re.search(r'FELUCCA_VERSION "(v[^"]+)"', open(os.path.join(R, 'firmware/src/felucca.c')).read()).group(1)
CREDITS = 'Built on JIANT FM (Juan Jiant) and Felucca (Hügelton Instruments)'

HEAT = ['#22d3ee', '#3b82f6', '#8b5cf6', '#d946ef', '#ef4444', '#ff7a00', '#facc15', '#f4f1ea']

CSS = """
@font-face { font-family: Chakra; src: url('../../assets/fonts/ChakraPetch-Medium.ttf'); font-weight: 500; }
@font-face { font-family: Chakra; src: url('../../assets/fonts/ChakraPetch-SemiBold.ttf'); font-weight: 600; }
@font-face { font-family: Chakra; src: url('../../assets/fonts/ChakraPetch-Bold.ttf'); font-weight: 700; }
* { box-sizing: border-box; margin: 0; padding: 0; }
html, body { background: #000; }
body { width: 1600px; color: #f4f1ea; font-family: Chakra, sans-serif; font-weight: 500; font-size: 17px;
       line-height: 1.32; padding: 36px 56px 40px; }
header { display: flex; align-items: center; gap: 36px; }
header h1 { font-size: 64px; font-weight: 700; letter-spacing: 1px; line-height: 1; }
header h2 { font-size: 26px; font-weight: 500; color: #b8b2c8; margin-top: 8px; }
header .ver { font-size: 18px; color: #7d7690; margin-top: 6px; }
.rule { height: 4px; margin: 22px 0 26px; border-radius: 2px;
        background: linear-gradient(90deg, #22d3ee, #3b82f6, #8b5cf6, #d946ef, #ef4444, #ff7a00, #facc15, #f4f1ea); }
.grid { display: grid; gap: 22px; }
.g2 { grid-template-columns: 1fr 1fr; } .g3 { grid-template-columns: 1fr 1fr 1fr; } .g4 { grid-template-columns: repeat(4, 1fr); }
.card { background: #100c16; border-radius: 12px; padding: 18px 20px 18px 24px; position: relative; }
.card::before { content: ''; position: absolute; left: 0; top: 14px; bottom: 14px; width: 5px; border-radius: 3px;
                background: var(--c); }
.card h3 { font-size: 23px; font-weight: 700; letter-spacing: .5px; color: var(--c); margin-bottom: 10px; }
.card h4 { font-size: 17px; font-weight: 600; color: #f4f1ea; margin: 12px 0 4px; letter-spacing: .4px; }
.sec { font-size: 30px; font-weight: 700; letter-spacing: 1px; margin: 30px 0 14px; display: flex; align-items: center; gap: 14px; }
.sec span { flex: 1; height: 2px; background: #2a2236; }
.k { display: inline-block; background: #2b2633; color: #f4f1ea; border-radius: 6px; padding: 0 7px; margin: 1px 1px;
     font-weight: 600; font-size: .92em; white-space: nowrap; }
.k.a { background: #ff7a00; color: #120a00; } .k.v { background: #b870ff; color: #12081c; }
.dim { color: #8f88a3; }
p + p { margin-top: 6px; }
table { width: 100%; border-collapse: collapse; }
td, th { text-align: left; vertical-align: top; padding: 5px 8px 5px 0; border-bottom: 1px solid #241d2e; }
th { color: #8f88a3; font-weight: 600; font-size: 15px; letter-spacing: .5px; }
tr:last-child td { border-bottom: 0; }
td.n { color: var(--c); font-weight: 700; white-space: nowrap; width: 1%; padding-right: 14px; }
.knobs { display: grid; grid-template-columns: repeat(4, 1fr); gap: 4px; margin-bottom: 4px; }
.col { display: flex; flex-direction: column; gap: 22px; }
.knobs span { background: #1c1624; border-radius: 6px; text-align: center; padding: 2px 0; font-weight: 600; font-size: 15px; }
.knobs span.e { color: #4a4258; }
.pages td.p { white-space: nowrap; width: 1%; padding-right: 12px; font-weight: 600; }
.kb { position: relative; height: 150px; margin: 8px 0 4px; }
.kb .w { position: absolute; bottom: 0; height: 92px; border-radius: 6px; background: #1f1a27; border: 1px solid #2f2839;
         font-size: 12.5px; font-weight: 600; text-align: center; padding: 4px 2px 0; line-height: 1.1; color: #d8d2e4; }
.kb .w b, .kb .b b { display: block; font-size: 11px; color: #6f6882; font-weight: 600; margin-bottom: 2px; }
.kb .b { position: absolute; top: 0; height: 54px; border-radius: 6px; background: #2b2633; font-size: 12px; font-weight: 600;
         text-align: center; padding: 3px 1px 0; line-height: 1.05; color: #f4f1ea; }
.kb .on { background: var(--c); color: #0b0710; border-color: transparent; }
.kb .on b { color: rgba(0, 0, 0, .55); }
footer { margin-top: 30px; color: #6f6882; font-size: 16px; display: flex; justify-content: space-between; }
ul { padding-left: 18px; } li { margin: 2px 0; }
"""

WHITE = ['F3', 'G3', 'A3', 'B3', 'C4', 'D4', 'E4', 'F4', 'G4', 'A4', 'B4', 'C5', 'D5', 'E5', 'F5', 'G5']
BLACK = [('F#3', 0), ('G#3', 1), ('A#3', 2), ('C#4', 4), ('D#4', 5), ('F#4', 7), ('G#4', 8), ('A#4', 9), ('C#5', 11),
         ('D#5', 12), ('F#5', 14)]   # (name, the white key it follows)


def e(s):
    return html.escape(s)


def k(*names, cls=''):
    return ''.join(f'<span class="k {cls}">{e(n)}</span>' for n in names)


def keyboard(whites, blacks, width=692):
    """whites / blacks: dicts note -> label (only those set are lit)"""
    ww = width / 16.0
    out = ['<div class="kb">']
    for i, n in enumerate(WHITE):
        lab = whites.get(n, '')
        out.append(f'<div class="w{" on" if lab else ""}" style="left:{i * ww:.1f}px;width:{ww - 3:.1f}px">'
                   f'<b>{n}</b>{e(lab)}</div>')
    for n, after in BLACK:
        lab = blacks.get(n, '')
        x = (after + 1) * ww - ww * 0.36
        out.append(f'<div class="b{" on" if lab else ""}" style="left:{x:.1f}px;width:{ww * 0.72:.1f}px">'
                   f'<b>{n[:-1]}</b>{e(lab)}</div>')
    out.append('</div>')
    return ''.join(out)


def card(title, color, body, extra=''):
    return f'<div class="card" style="--c:{color}" {extra}><h3>{e(title)}</h3>{body}</div>'


def knobs(*labels):
    return '<div class="knobs">' + ''.join(
        f'<span class="{"e" if l == "-" else ""}">{e(l)}</span>' for l in labels) + '</div>'


def page(title, sub, body, name):
    doc = f"""<!doctype html><html lang="en"><head><meta charset="utf-8"><title>Braindancer · {e(title)}</title>
<style>{CSS}</style></head><body>
<header><div>
<h1>BRAINDANCER · {e(title)}</h1><h2>{e(sub)}</h2>
<div class="ver">firmware {VERSION} · M-VAVE FM-1 · norfolkhacker.github.io/braindancer</div></div></header>
<div class="rule"></div>
{body}
<footer><span>GPL-3.0 · {e(CREDITS)} · song mode from SLOOP</span>
<span>Braindancer {VERSION}</span></footer>
</body></html>"""
    with open(os.path.join(OUT, name), 'w') as f:
        f.write(doc)


def sec(t):
    return f'<div class="sec">{e(t)}<span></span></div>'


# ------------------------------------------------------------------------------------------------ 1. CONTROLS ---
def controls():
    b = []
    b.append(sec('NAVIGATION · SIX RULES'))
    rules = [
        ('1', 'SELECT WALKS THE PAGES', 'Every page of the section in one chain, a page a detent, stopping at both ends. '
         'No levels, no zoom.', 'FX: SENDS → SPACE → MASTER → MORE'),
        ('2', 'THE BUTTON: IN, OR ON', 'From another section: the page you were last on there. Already inside: the '
         'next page (round).', 'SEQ › STEPS: SEQ → STEP EXTRAS · GLO › LIVE: GLO → MIX'),
        ('3', 'PRESETS LOADS PRESETS', 'Always: the track\'s sounds; SEQ the patterns (PHRASES, OCT+ loads); GEN the '
         'genre. Never a page, a track or a module.', 'FX › SPACE: PRESETS ▶ the next sound'),
        ('4', 'KNOB 1 PICKS', 'The step, the slot, the stage, the operator, the kit\'s sound, the row. The track: '
         'always ALGORITHM.', 'SEQ › STEPS: K1 = all 64 steps · MORE: K1 = the row, K2 its value'),
        ('5', 'HOLD = THE LAYER', 'Held + SELECT: HOME GEN · EDIT SEQ LFO FX their dice · GLO the tempo · ENV SCL ARP '
         'SAVE their chain.', 'SEQ + G5 CLR STEP · SAVE + C5 NAME'),
        ('6', 'THE SCREEN SAYS WHERE', 'The header names the section and the track; the status line what the knobs '
         'and the next tap do. Footer: the page\'s place, n / N.', 'SEQ · STEPS   1 / 4'),
    ]
    b.append('<div class="grid g3">' + ''.join(
        card(f'{n} · {t}', HEAT[int(n) + 1], f'<p>{e(d)}</p><p class="dim">{e(x)}</p>') for n, t, d, x in rules) + '</div>')
    b.append(sec('THE PANEL'))
    rows = [
        ('MASTER', 'Master volume.'),
        ('SELECT', '<b>Walks the section\'s pages</b>, a page a detent, stopping at the ends (the MENU: its pages). Held with a button: '
                   'HOME <b>GEN</b>; EDIT · SEQ · LFO · FX their <b>dice</b> (OCT+ keeps, OCT− back); GLO the tempo; ENV · SCL · ARP · SAVE their chain.'),
        ('PRESETS', '<b>Always presets</b>: the sounds (factory and user) on every page; SEQ: the patterns (PHRASES); GEN: the genre. '
                    'Nothing in the MENU or with a button held.'),
        ('ALGORITHM', 'Selects the track from any page: T1–T6 (T1–T4 with TRACKS 4). MENU: nothing. A layer held: MAP · KNOBS · STATE.'),
        ('KNOB 1–4', 'The four knobs on screen, in their colours; KNOB 1 picks (step, slot, row). Fast sweeps the whole range, slow fine-tunes. '
                     'With a layer held: the layer\'s. A step held: a lock on that step.'),
        ('OCT− · OCT+', 'The keyboard\'s octave (both together: back to 0); on DRUM the kit stays put: +1/+2/+3 REPEAT 1/2, REPEAT, RATCHET X2; −1 SHORT, −2 GHOST, −3 FLAM. On action rows and dialogs: back / confirm.'),
        ('PLAY', 'Start / stop. GLO + PLAY: restart from the top.'),
        ('REC', 'Arms live recording on the selected track (notes and knob moves, overdub). On SEQ › STEPS stopped: step recording.'),
        ('HOME', 'Tap: HOME (on HOME the next page: TRACKS · VISUAL · GEN). <b>Hold: the MENU.</b>'),
        ('KEYS', '27 keys play the selected track; with a layer held they become that layer\'s; on GLO › LIVE they are the tricks.'),
    ]
    t = '<table>' + ''.join(f'<tr><td class="n">{e(n)}</td><td>{d}</td></tr>' for n, d in rows) + '</table>'
    pb = [
        ('HOME', 'TRACKS · VISUAL · GEN 1 · GEN 2 · GEN 3', 'MENU'),
        ('EDIT', 'SOUND · TONE · VOICE or FILTER · MORE (FM6: SOUND · OPERATOR · OP ENV · MORE; DRUM-X: KIT · SOUND · SOUND 2 · MORE)', 'INIT · RECALL · sound mutes'),
        ('SEQ', 'STEPS · STEP EXTRAS · PATTERN · MORE (TRACK · STEP · MUTATE · SHAPE · PLAYHEADS · AUTOMATION)', 'SEQ TOOLS'),
        ('FX', 'SENDS · SPACE · MASTER · MORE (DISTORT · AUTO PAN · DELAY · REVERB · CHORUS · SLICER · SLICE PITCH · PUNCH LANE · MASTER · TAPE)',
         'PUNCH-IN FX'),
        ('GLO', 'LIVE · MIX · MIDI · MORE (LIVE: the rails · SYSTEM · TEMPO · MIDI · UNDO); at power-on LIVE', 'MUTE · SOLO · LEVEL · TAP · UNDO'),
        ('ENV', 'AMP · ENV TO', '—'),
        ('LFO', 'LFO · DEST · MOD MATRIX · MORE (PHASE · STEPS)', 'MACROS M1–M4 (stay latched)'),
        ('SCL', 'KEY · CHORD · MORE', 'ROOT by key'),
        ('ARP', 'ARP · ARP 2', '—'),
        ('SAVE', 'SOUND · SONG · PROJECT · MORE (from outside, SOUND comes with SAVE picked)', 'SONG layer · C5 NAME'),
        ('REC', '(transport)', 'CLEAR · CLICK'),
    ]
    t2 = ('<table class="pages"><tr><th>BUTTON</th><th>PAGES (A TAP: THE NEXT · SELECT WALKS THEM)</th><th>HOLD</th></tr>' +
          ''.join(f'<tr><td class="n">{e(n)}</td><td>{e(p)}</td><td class="dim">{e(h)}</td></tr>' for n, p, h in pb) +
          '</table>')
    b.append('<div class="grid g2">' + card('KNOBS AND TRANSPORT', HEAT[0], t) +
             card('SECTIONS', HEAT[2], t2 + '<p style="margin-top:10px">' + k('BUTTON', cls='v') +
                  ' from elsewhere: the page last shown; inside: the next page. ' + k('SELECT', cls='v') +
                  ' walks every page of the section. <b>MORE</b>: KNOB 1 picks a row, KNOB 2 sets it.</p>') + '</div>')

    b.append(sec('LAYERS · HOLD A BUTTON'))
    fx_w = {'F3': 'REP 1/8', 'G3': 'REP 1/16', 'A3': 'REP 1/32', 'B3': 'GLITCH', 'C4': 'TAPE', 'D4': 'OCT−', 'E4': 'OCT+',
            'F4': '½ TEMPO', 'G4': 'DEC−', 'A4': 'DEC+', 'B4': 'STUT 1/8', 'C5': 'STUT 1/16', 'D5': 'ATK+', 'E5': 'ARP',
            'F5': 'RAND', 'G5': 'ERASE'}
    fx_b = {'F#3': 'M1', 'G#3': 'M2', 'A#3': 'M3', 'C#4': 'M4', 'D#4': 'M5', 'F#4': 'M6', 'G#4': 'FRZ', 'A#4': 'TGT',
            'C#5': 'SHUF', 'D#5': 'CRSH', 'F#5': 'THRW'}
    glo_w = {'F3': 'SOLO 1', 'G3': 'SOLO 2', 'A3': 'SOLO 3', 'B3': 'SOLO 4', 'C4': 'SOLO 5', 'D4': 'SOLO 6',
             'E4': 'ALL', 'F4': 'TAP', 'G4': 'UNDO', 'A4': 'FRZ', 'B4': 'RVRT ALL'}
    glo_b = {'F#3': 'M1', 'G#3': 'M2', 'A#3': 'M3', 'C#4': 'M4', 'D#4': 'M5', 'F#4': 'M6', 'G#4': 'KICK',
             'A#4': 'SNR', 'C#5': 'HAT', 'D#5': 'PERC'}
    live_w = {'F3': '/2', 'G3': 'X2', 'A3': '3/2', 'B3': '2/3', 'C4': 'LOOP 8', 'D4': 'LOOP 4', 'E4': 'LOOP 2', 'F4': 'LOOP 1',
              'G4': 'REV', 'A4': 'PING', 'B4': 'RATCH X2', 'C5': 'RATCH X3', 'D5': 'THIN', 'E5': 'OCT+', 'F5': 'OCT−', 'G5': 'RETURN'}
    live_b = {'F#3': 'T1', 'G#3': 'T2', 'A#3': 'T3', 'C#4': 'T4', 'D#4': 'T5', 'F#4': 'T6', 'G#4': 'LATCH', 'A#4': 'ALL',
              'C#5': 'TURN <', 'D#5': 'TURN >', 'F#5': 'RAND'}
    sv_w = {'F3': 'PLAY A', 'G3': 'PLAY B', 'A3': 'PLAY C', 'B3': 'PLAY D', 'C4': 'STORE A', 'D4': 'STORE B',
            'E4': 'STORE C', 'F4': 'STORE D', 'C5': 'NAME', 'D5': 'LOOP/ SONG', 'E5': 'SONG REC', 'G5': 'SONG'}
    ed_w = {'F3': 'INIT', 'G3': 'RECALL'}
    ed_b = {'F#3': 'BD', 'G#3': 'SD', 'A#3': 'CP', 'C#4': 'CH', 'D#4': 'OH', 'F#4': 'TM', 'G#4': 'RS', 'A#4': 'CB'}
    b.append('<div class="grid g2">')
    b.append(card('FX · PUNCH-IN EFFECTS', HEAT[5], keyboard(fx_w, fx_b) +
                  '<p>A key plays its effect while held (MENU FX LATCH ON: toggles); hold FX and let go untouched: <b>LOCKED</b> '
                  'open, FX again unlocks; OCT− all off. In on the next 1/16. Black 1–6 mute T1–T6, G#4 FREEZE, A#4 picks which tracks '
                  'the note effects touch (all · synths · drums), C#5 SHUFFLE; D#5 F#5 sweep CRUSH, THROW dry to wet over 2 bars (back in a beat). '
                  'With REC armed and playing they record into the section\'s lane; G5 erases it.</p>'
                  '<h4>KNOBS · PERFORMANCE MACROS</h4>' + knobs('STEP LOOP', 'INTERVAL', 'MUTATE', 'DJ FILTER') +
                  '<p>Up to the right; turn any back: all four off at once. GLITCH, TAPE STOP, FREEZE, SHUFFLE held: their own four.</p>'))
    b.append(card('GLO · MIX', HEAT[1], keyboard(glo_w, glo_b) +
                  '<p>Black 1–6 mute T1–T6 (latched), 7–10 the DRUM-X groups. White: solo while held, E4 unmutes all, '
                  'F4 tap tempo, G4 <b>UNDO</b> the last big change (again: redo), A4 <b>FREEZE</b> MUTATE, B4 <b>REVERT ALL</b> (every track back to MUTATE\'s anchor). SELECT: the tempo. PLAY: restart.</p><h4>KNOBS</h4>' +
                  knobs('LEVEL T1', 'LEVEL T2', 'LEVEL T3', 'LEVEL T4') +
                  '<p class="dim" style="margin-top:4px">T5 or T6 selected: the knobs are T3–T6.</p>'))
    b.append(card('GLO › LIVE · TRICKS (ON THE PAGE)', HEAT[4], keyboard(live_w, live_b) +
                  '<p>Black 1–6 pick the tracks while held (none or A#4: all); white keys play the tricks while held. G#4 LATCH, '
                  'C#5 / D#5 TURN, F#5 RANDOM. OCT− tapped RETURN, held BYPASS; OCT+ twice KEEP (into the sequence; GLO + G4 undoes). '
                  'GLO › MORE › LIVE: the rails (TRACKS, RATE, CUT, DENSITY), latched.</p>'
                  '<h4>KNOBS</h4>' + knobs('RATE', 'LOOP', 'THIN', 'DJ FILTER')))
    b.append(card('SAVE · SONG', HEAT[6], keyboard(sv_w, {}) +
                  '<p>PLAY A–D on the next bar (several in one hold: a quick chain), STORE the section into A–D. '
                  'OCT+ stores the playing section, OCT− recalls it. C5 renames (SOUND, SONG, PROJECT). On SONG KNOB 4 stores a scene.</p>'
                  '<h4>KNOBS</h4>' + knobs('PROJECT', '-', '-', '-')))
    b.append(card('EDIT · SOUND', HEAT[3], keyboard(ed_w, ed_b) +
                  '<p>INIT: the engine\'s own sound, dry (DRUM-X: the factory kit too). RECALL: the sound stored in the section. On a DRUM '
                  'track black 1–8 mute each DRUM-X sound.</p><h4>KNOBS</h4>' + knobs('SOUND 1', 'SOUND 2', 'SOUND 3', 'SOUND 4')))
    b.append('</div>')
    small = [
        ('LFO · MACROS', HEAT[2], 'Knobs M1–M4; they stay latched when let go (LFO again: off). F3 rolls new routes, '
                                  'G3 clears them (only tracks that sound). LFO + SELECT: the track\'s LFO diced.', ('M1', 'M2', 'M3', 'M4')),
        ('SCL · ROOT', HEAT[0], 'Any key sets the ROOT. ROOT and SCALE are every melodic track\'s.',
         ('ROOT', 'SCALE', 'ROTATE', 'PITCH')),
        ('SEQ · TOOLS', HEAT[4], 'On the SEQ pages: F3 CLEAR · G3 REVERSE · A3 B3 SHIFT · C4 RANDOM · D4 COOK · E4 BEAT · F4 CLR AUTO · G4–C5 the lane\'s tools (DRUM) · D5 REVERT · G5 CLR STEP · REC: CLEAR ALL. No undo.',
         ('LENGTH', 'DIVISION', 'SWING', 'GATE')),
        ('REC · RECORD', '#ef4444', 'F3 CLEAR the track (at once; GLO + G4 undoes it), G3 the metronome CLICK.', ('CLICK', '-', '-', '-')),
    ]
    b.append('<div class="grid g4" style="margin-top:22px">' + ''.join(
        card(t, c, f'<p>{e(d)}</p><h4>KNOBS</h4>' + knobs(*kn)) for t, c, d, kn in small) + '</div>')

    b.append(sec('COMBOS'))
    combos = [
        (k('HOME', cls='v') + ' + ' + k('SELECT', cls='v'), 'GEN, the song generator (HOME\'s last pages).'),
        (k('EDIT · SEQ · LFO · FX', cls='v') + ' + ' + k('SELECT', cls='v'), 'Their dice: sound · sequence · LFO · effects (BEFORE / NOW; OCT+ keeps, OCT− back).'),
        (k('ENV · SCL · ARP · SAVE', cls='v') + ' + ' + k('SELECT', cls='v'), 'Into that section and along its chain.'),
        (k('SEQ') + ' + ' + k('G5'), 'CLR STEP: the picked step and its locks (the AUTOMATION list: deletes the record).'),
        (k('SAVE') + ' + ' + k('C5'), 'NAME: rename the slot, the section, the project.'),
        (k('REC') + ' + ' + k('FX') + k('EDIT') + k('ENV') + k('LFO') + k('SCL') + k('ARP') + k('GLO'),
         'Clear that part\'s automation on the selected track (FX: the punch-in lane too).'),
        (k('SEQ') + ' + ' + k('REC'), 'CLEAR ALL SEQUENCES? (OCT+ confirms).'),
        (k('SAVE') + ' + ' + k('OCT+') + ' / ' + k('OCT−'), 'Store / recall the playing section.'),
        (k('GLO') + ' + ' + k('PLAY'), 'Restart from the top.'),
        (k('GLO') + ' + ' + k('SELECT'), 'Tempo (from any page; under CLOCK USB or TRS the clock\'s).'),
        (k('STEP HELD', cls='a') + ' + ' + k('KNOB'), 'A lock on that step (LOCK · SETTING · VALUE); + ' + k('SELECT', cls='v') + ' the EDIT page they lock; + ' + k('EDIT') + ' clears its locks.'),
        (k('HOME', cls='a') + ' held', 'MENU: AUDIO · CLICK · CONTROLS · MORE (speaker EQ, tracks, load, BPM lock; click and count-in; hold time, knob speedup, FX latch, calibration; MORE: screen, LEDs, USB audio, CPU cores and clock, ABOUT).'),
    ]
    t3 = '<table>' + ''.join(f'<tr><td style="white-space:nowrap;width:1%;padding-right:18px">{a}</td><td>{d}</td></tr>'
                             for a, d in combos) + '</table>'
    b.append(card('BUTTONS TOGETHER', HEAT[7], t3))

    b.append(sec('EVERY PAGE · KNOB 1 – 4'))
    fams = [
        ('ENV', HEAT[0], [('AMP', 'ATTACK DECAY SUSTAIN RELEASE'), ('ENV TO', 'FILTER PITCH SHAPE LOOP')]),
        ('LFO', HEAT[1], [('LFO', 'RATE WAVE MODE FADE'), ('DEST', 'PITCH FILTER SHAPE AMP'), ('MOD MATRIX', 'SLOT SOURCE DEST AMOUNT'),
                          ('MORE', 'PHASE · STEPS (LENGTH, STEP RATE, SMOOTH, STEP 1–16): K1 picks, K2 sets')]),
        ('FX', HEAT[5], [('SENDS', 'DISTORT CHORUS DELAY REVERB'), ('SPACE', 'DLY TIME DLY FDBK REV TYPE REV SIZE'),
                         ('MASTER', 'CLIP PUNCH DUCK AGE'),
                         ('MORE', 'DISTORT · AUTO PAN · DELAY · REVERB · CHORUS · SLICER · SLICE PITCH · PUNCH LANE · MASTER · TAPE: K1 picks, K2 sets')]),
        ('EDIT', HEAT[3], [('SOUND', 'E1 E2 E3 E4'), ('TONE', 'E5 E6 E7 E8'), ('VOICE', 'VOICE GLIDE PRIORITY SPREAD'),
                           ('FILTER', 'FILTER CUTOFF RESO DISTORT'), ('FM6 SOUND', 'ALGORITHM FEEDBACK MOD LEVEL MOD RATIO'),
                           ('OPERATOR', 'OP RATIO FINE LEVEL'), ('OP ENV', 'OP STAGE RATE LEVEL'), ('KIT', 'MORPH TUNE TONE DECAY'),
                           ('DRUM SOUND', 'DRUM LEVEL PITCH DECAY'), ('SOUND 2', 'DRUM NOISE COLOR WAVE'),
                           ('MORE', 'the engine\'s other settings in groups: K1 picks, K2 sets')]),
        ('SCL · ARP', HEAT[2], [('KEY', 'ROOT SCALE SNAP ALL TRACKS'), ('CHORD', 'CHORD VOICING STRUM -'),
                                ('MORE', 'TRANSPOSE: K1 picks, K2 sets'), ('ARP', 'ARP RATE OCTAVES GATE'),
                                ('ARP 2', 'SWING CHANCE HOLD ORDER')]),
        ('SEQ', HEAT[4], [('STEPS', 'STEP NOTE ACC/SLD GATE'), ('STEPS · DRUM', 'STEP LANE HIT ACC'),
                          ('STEP EXTRAS', 'RATCHET CHANCE CONDITION MICRO'), ('PATTERN', 'LENGTH DIVISION SWING GATE'),
                          ('MORE', 'TRACK (SEQ PITCH, CHANCE, HUMAN, REC SNAP, PLAYS) · STEP (TYPE, TYPE 2, EVERY, FOCUS, PITCH, MOVE) · MUTATE '
                                   '(SOUND, PATTERN, AMOUNT, MODE) · SHAPE (STRETCH, REPEAT, NUDGE, ROTATE) · PLAYHEADS (HEAD, RATE, '
                                   'DIRECTION, INTERVAL) · AUTOMATION (PLAY, EVENTS, LOCKS, CLEAR, the records): K1 picks, K2 sets')]),
        ('GLO · SAVE · MENU', HEAT[6], [('LIVE', 'RATE LOOP THIN DJ FILTER'), ('MIX', 'LEVEL PAN SWING TUNE'),
                                 ('MIDI', 'IN OUT CHANNEL CLOCK'),
                                 ('MORE', 'LIVE (TRACKS, RATE, CUT, DENSITY) · SYSTEM (TRACKS, LOAD, CPU) · TEMPO (BPM) · MIDI (KEYBOARD PLAYS '
                                          'SELECTED, CLOCK OUT, NOTES OUT, LEARN, LEARN CLEAR) · UNDO: K1 picks, K2 sets'),
                                 ('SAVE · SOUND', 'SLOT PRESET NAME SAVE'), ('SAVE · SONG', 'SECTION ROW BARS SCENE'),
                                 ('SAVE · PROJECT', 'PROJECT LOAD SAVE NAME'),
                                 ('SAVE · MORE', 'SONG (END, DELETE ROW, CLEAR SONG) · TOOLS (CLEAR SEQ, INIT SOUND, ERASE USER SLOT, NEW PROJECT, CPU TEST) · SOUND (LOAD SLOT, ENGINE, FAVORITE, LIST): K1 picks, K2 sets'),
                                 ('MENU', 'AUDIO (SPEAKER EQ TRACKS LOAD BPM LOCK) · CLICK · CONTROLS (HOLD TIME KNOB SPEEDUP FX LATCH CALIBRATION) · MORE')]),
    ]

    def ptable(rows):
        out = '<table class="pages">'
        for p, ks in rows:
            parts = ks.split(' ')
            if len(parts) == 4:
                cells = knobs(*parts)
            else:
                cells = f'<span class="dim">{e(ks)}</span>'
            out += f'<tr><td class="p">{e(p)}</td><td>{cells}</td></tr>'
        return out + '</table>'
    cards = {n: card(n, c, ptable(r)) for n, c, r in fams}
    b.append('<div class="grid g3"><div class="col">' + cards['ENV'] + cards['LFO'] + cards['SCL · ARP'] + '</div>' +
             cards['FX'] + cards['EDIT'] + '</div>')
    b.append('<div class="grid g2" style="margin-top:22px">' + cards['SEQ'] + cards['GLO · SAVE · MENU'] + '</div>')
    b.append(sec('GLO › MIDI'))
    b.append(card('MIDI PER TRACK · THE CLOCK', HEAT[1],
                  '<p>KNOB 1 <b>IN</b>, KNOB 2 <b>OUT</b>, KNOB 3 <b>CHANNEL</b> (1–16) of the selected track (ALGORITHM picks it; its row lit '
                  'in the table), KNOB 4 <b>CLOCK</b> INT / USB / TRS. A track plays the notes and CCs of its channel while IN is on, '
                  'and sends its notes on its channel over USB while OUT and NOTES OUT are on. Channel 16\'s Program Change always picks a '
                  'section or project. GLO › MORE › MIDI: <b>KEYBOARD PLAYS SELECTED</b> (notes on a channel no track listens to play the '
                  'selected track), <b>CLOCK OUT</b> (USB only: the TRS socket is an input), <b>NOTES OUT</b> OFF / KEYS / KEYS+SEQ, '
                  '<b>LEARN</b>, LEARN CLEAR. Under CLOCK USB or TRS the tempo is the clock\'s.</p>'))
    page('CONTROLS', 'every button, knob, layer, combo and page', ''.join(b), 'controls.html')


# ------------------------------------------------------------------------------------------- 2. SOUND & FX ---
def sound():
    b = []
    b.append(sec('ENGINES · EDIT › SOUND / TONE'))
    eng = [
        ('DRUM-X', '#ff7a00', 'MORPH TUNE TONE DECAY', 'DRUM LEVEL PITCH DECAY',
         '8 sounds BD SD CP CH OH TM RS CB, sides A / B: <b>MORPH</b> moves the whole kit between them. KIT is the whole kit; '
         'SOUND (shown here) and SOUND 2 (DRUM NOISE COLOR WAVE) one drum, KNOB 1 picks it (a key struck too). MORE: SIDE, PITCH MOD, '
         'MOD TYPE, NOISE TYPE, CRACK, DRIVE, CLANG, FEEDBACK, SEND; the kit\'s NOISE, CLICK, FOLD, METAL, HANDS, STEREO, MORPH LFO.'),
        ('VOICE', HEAT[6], 'VOWEL VOWEL 2 TALK SHIFT', 'BUZZ BREATH RESO RANDOM',
         'A formant voice: vowels, talk, breath. <b>WORDS</b> (MORE) ES / EN / JA sings syllables, a note each: SOUND shows POSITION, '
         'THEME, CRISP; LYRIC is the text or the dice\'s phrases (TYPE LYRIC opens NAME; the web editor sends it too).'),
        ('PHASE', HEAT[2], 'WAVE WAVE 2 BRIGHT SWEEP', 'DETUNE LINE 2 SUB FEEDBACK',
         'Phase distortion: BRIGHT how far the phase is bent, SWEEP how far the envelope moves it; FEEDBACK from hard edge to growl.'),
        ('LOFI', HEAT[0], 'CHIP WAVE DUTY CRUSH', 'CUTOFF VIBRATO BEND MASK',
         'A chip: PLS TRI SAW NOIS WRAM. <b>BYTE</b>: 32 bytebeat FORMULAS, VARIANT · MOTION GRIT BEND LOOP (LOOP high: a tuned note). '
         '<b>FLOAT</b>: 32 floatbeat formulas in the key played; CHIP sets the resolution. RETRIG in MORE.'),
        ('ANALOG', HEAT[1], 'WAVE DETUNE MIX NOISE', 'CUTOFF RESO DRIVE INTERVAL',
         'SAW SQR TRI SIN PWM <b>SYNC RING SAW3</b>; INTERVAL: osc 2 ±24 st. Its filter is per voice; page 3 is VOICE (VOICE GLIDE PRIORITY SPREAD).'),
        ('FM6', HEAT[3], 'ALGORITHM FEEDBACK MOD LEVEL MOD RATIO', 'OP RATIO FINE LEVEL',
         'Six operators, 32 algorithms drawn (P 5 = the patch\'s own). TONE here is OPERATOR (ratio or FIXED freq, fine, level); OP ENV: '
         'OP STAGE RATE LEVEL. MORE: PATCH F1–F8, OWN, B1–B32 (a 32-voice .syx sent from the web editor), MOD TIME, MOD VEL, DETUNE.'),
        ('NOISE', HEAT[4], 'SOURCE COLOR FREQ PEAK', 'KEY TRACK DENSITY DRIFT CRUSH',
         'Coloured and metallic noise (SOURCE ANLG DUST LFSR META), tracked to the key or free.'),
    ]
    t = '<table><tr><th>ENGINE</th><th>SOUND (KIT)</th><th>TONE (SOUND)</th><th>WHAT TO KNOW</th></tr>'
    for n, c, e1, e2, d in eng:
        t += (f'<tr><td class="n" style="--c:{c}">{e(n)}</td><td style="width:22%">{knobs(*e1.split(" "))}</td>'
              f'<td style="width:22%">{knobs(*e2.split(" "))}</td><td>{d}</td></tr>')
    t += '</table>'
    b.append(card('SEVEN ENGINES, ALL SYNTHESIS', HEAT[7], t))
    b.append('<div class="grid g3" style="margin-top:22px">')
    b.append(card('FILTER · EVERY ENGINE', HEAT[3], knobs('FILTER', 'CUTOFF', 'RESO', 'DISTORT') +
                  '<p style="margin-top:8px"><b>LP · HP · BP · COMB</b>. COMB rings tuned to the note: CUTOFF moves it, RESO how much '
                  'it rings. Page 3 on PHASE, LOFI, VOICE, NOISE; on ANALOG, CUTOFF RESO DRIVE are TONE\'s and the type is in MORE; on FM6 '
                  'and DRUM-X the filter heads MORE.</p>'))
    b.append(card('VOICE · PLAYING', HEAT[2], knobs('VOICE', 'GLIDE', 'PRIORITY', 'SPREAD') +
                  '<p style="margin-top:8px">POLY · MONO · LEGATO · UNISON; GLIDE and GLIDE MODE; LAST / LOW / HIGH priority. '
                  '<b>12 voices</b> with both cores (8 with one), at most 8 a track.</p>'))
    b.append(card('DICE · GEN', HEAT[5], '<p><b>EDIT + SELECT</b>: a random factory preset of the engine, every '
                  'setting moved up to a quarter of its range (DRUM-X: both sides of every sound around the factory kit). Heard at once: '
                  'OCT+ keeps it, OCT− brings the sound back. SEQ / LFO / FX + SELECT: the sequence, the LFO, the effects, the same way. '
                  '<b>HOME + SELECT</b>: GEN, HOME\'s last three pages: GENRE (EXPLORE, RANDOM, or IDM, GLITCH HOP, BREAKS, '
                  'DRILL\'N\'BASS, ACID, ELECTRO, MICRO, RETROBOC, AMBIENT, AMB TECHNO) · SEED · ENERGY · FORM; DRUMS · BASS · CHORDS · ARP; '
                  'LEAD · COMPLEX · SWING · HUMAN. OCT+ makes the piece, OCT− back to TRACKS. GLO + G4 undoes.</p>'))
    b.append('</div>')

    b.append(sec('MODULATION'))
    b.append('<div class="grid g3">')
    b.append(card('ENV · LFO', HEAT[0],
                  '<h4>ENV › AMP</h4>' + knobs('ATTACK', 'DECAY', 'SUSTAIN', 'RELEASE') + '<h4>ENV › ENV TO</h4>' +
                  knobs('FILTER', 'PITCH', 'SHAPE', 'LOOP') +
                  '<p style="margin-top:6px">LOOP: held, the envelope cycles from the attack: an ADSR-shaped LFO.</p>'
                  '<h4>LFO</h4>' + knobs('RATE', 'WAVE', 'MODE', 'FADE') + '<p class="dim">SINE TRI SAW SQR RAND · TRIG / FREE</p>'
                  '<h4>LFO › DEST</h4>' + knobs('PITCH', 'FILTER', 'SHAPE', 'AMP')))
    b.append(card('MOD MATRIX · 4 SLOTS A TRACK', HEAT[2],
                  knobs('SLOT', 'SOURCE', 'DEST', 'AMOUNT') +
                  '<h4>SOURCES</h4><p>' + k('LFO', 'ENV', 'VELOCITY', 'KEY', 'NOTE RAND', 'WHEEL', 'PRESSURE', 'EXPR', 'MACRO 1–4',
                                             'STEPS', 'NOISE') + '</p>'
                  '<h4>DESTINATIONS</h4><p>' + k('PITCH', 'FILTER', 'SHAPE', 'AMP', 'PAN', 'DISTORT', 'CHORUS', 'DELAY', 'REVERB',
                                                  'LFO RATE', 'VIBRATO', 'E1…E8', 'CLIP', 'PUNCH', 'DLY TIME', 'DLY FDBK', 'DLY TONE',
                                                  'DLY MIX', 'REV SIZE', 'REV DAMP', 'CHO RATE', 'CHO DEPTH') + '</p>'
                  '<p style="margin-top:6px">E1…E8: the engine\'s own EDIT knobs, by their names (CUTOFF …). CLIP / PUNCH: the master. '
                  'DLY REV CHO: the buses.</p>'))
    b.append(card('STEPS · MACROS', HEAT[1],
                  '<h4>LFO › MORE › STEPS · A 16-LEVEL SEQUENCER</h4>' +
                  '<p>LENGTH, STEP RATE, SMOOTH, STEP 1–16, in time with the transport. Use it from MOD MATRIX, SOURCE STEPS.</p>'
                  '<h4>MACROS M1–M4</h4><p>Hold LFO: the knobs are M1–M4, latched until LFO again (also HOME › VISUAL). F3 rolls new '
                  'routes, G3 clears them. By hand: MOD MATRIX, SOURCE MACRO 1–4. Saved with the project.</p>'))
    b.append('</div>')
    b.append('<div class="grid g3" style="margin-top:22px">')
    b.append(card('SEQUENCER', HEAT[4],
                  '<p>64 steps a track, up to 4 notes a step. Four pages: <b>STEPS</b> (the roll; on DRUM the lane grid; KNOB 1 all 64 '
                  'steps, SELECT on the last cell the next 16), <b>STEP EXTRAS</b> (the picked step\'s RATCHET, CHANCE, CONDITION '
                  'EVERY 2ND / 1ST OF 4 …, MICRO in 64ths), <b>PATTERN</b>, <b>MORE</b>. <b>Hold a step + turn a knob</b>: a lock '
                  '(LOCK · SETTING · VALUE). Step recording: stopped, REC on STEPS; a chord held + KNOB 1 its length, + ARP arpeggiated. '
                  'MORE › STEP: two components a step (CHANCE, RATCHET, ARP, SEQ, WALK, PULSE, OCTAVE, RANDOM, BEND, TONAL, JUMP, '
                  'REVERSE, CONDITION, FILL, KILL) and EVERY. TRACK: SEQ PITCH, CHANCE thins the whole track, HUMAN, REC SNAP. '
                  'SHAPE: STRETCH, REPEAT, NUDGE, ROTATE as you turn. REC + knobs while playing: automation (64 records a section; '
                  'MORE › AUTOMATION plays, clears and lists them).</p>'
                  '<h4>STEPS · STEP EXTRAS · PATTERN</h4>' + knobs('STEP', 'NOTE', 'ACC/SLD', 'GATE') +
                  knobs('RATCHET', 'CHANCE', 'CONDITION', 'MICRO') + knobs('LENGTH', 'DIVISION', 'SWING', 'GATE')))
    b.append(card('SCALE · ARP', HEAT[0],
                  '<p><b>ROOT and SCALE</b> are every melodic track\'s: CHROMATIC MAJOR MINOR DORIAN MIXOLYDIAN PENTATONIC MINOR PENT '
                  'HARMONIC MINOR PHRYGIAN LYDIAN LOCRIAN MELODIC MINOR BLUES WHOLE TONE and two DIMINISHED. SNAP puts the keys (or the '
                  'sequence) on the scale, <b>ALL TRACKS</b> moves every sequence along it. <b>CHORD</b>: a key plays a chord (TRIAD 7TH '
                  'ADD9 SUS2 SUS4 in key, MAJ MIN 7 MAJ7 MIN7 POWER), VOICING LEAD leads each to the next, STRUM as a harp.</p>'
                  '<h4>ARP MODES</h4><p class="dim">OFF UP DOWN UP-DOWN RANDOM REPEAT DOWN-UP UP+OCTAVE CONVERGE DIVERGE PINKY THUMB '
                  'WALK CHORD <b style="color:#f4f1ea">TRNS</b> (the keys move the sequence)</p>'))
    b.append(card('SONGS', HEAT[6],
                  '<p><b>4 projects × 4 sections (A–D)</b>, a song of up to 16 rows by bars (SAVE › SONG, OCT+ adds a row, PLAY '
                  'plays; END: STOP, x2–x8, LOOP), SLOOP style. Each row can hold a <b>scene</b>: mutes, DRUM-X group mutes, macros '
                  'and punch-ins. SAVE held plays sections live on the next bar, stores them, records the song (SONG REC). Each '
                  'section keeps its sounds, steps, automation, kit and its SLICER.</p>'))
    b.append('</div>')

    b.append(sec('EFFECTS'))
    b.append('<div class="grid g3">')
    b.append(card('PER TRACK', HEAT[5], '<h4>FX › SENDS</h4>' + knobs('DISTORT', 'CHORUS', 'DELAY', 'REVERB') +
                  '<h4>MORE › DISTORT</h4>' + knobs('DIST TYPE', 'DIST TONE', '-', '-') +
                  '<p style="margin-top:6px">SOFT · HARD · FOLD · CRUSH · RECTIFY · AMP.</p>'
                  '<h4>MORE › AUTO PAN</h4>' + knobs('PAN DEPTH', 'PAN RATE', 'PAN SYNC', 'PAN WAVE') +
                  '<p style="margin-top:6px">The track\'s pan swung L–R around its PAN. SYNC <b>BEAT</b>: in time with the bar; '
                  '<b>FREE</b>: in Hz.</p>'))
    b.append(card('SLICER · ONE ON A BUS', HEAT[3], knobs('SLICE MODE', 'SLICE PAT', 'SLICE RATE', 'SLICE DEPTH') +
                  '<p style="margin-top:6px">OFF · GATE · STUTTER, 16 patterns. <b>SLICE T1–T6</b> and <b>SLICE FX</b> (the delay / '
                  'reverb / chorus returns) ON / OFF: everything on = the whole mix chopped after the effects. <b>SLICE PITCH</b>: '
                  'the repeats\' 16-step pitch (PITCH 1–16, PITCH LOOP, PITCH SNAP to the scale). Not with LOAD ECO.</p>'))
    b.append(card('DELAY · CHORUS', HEAT[1], '<h4>FX › SPACE</h4>' + knobs('DLY TIME', 'DLY FDBK', 'REV TYPE', 'REV SIZE') +
                  '<h4>MORE › DELAY</h4><p>DLY TONE, DLY MIX, DLY WIDE, DLY PITCH, DLY SPRAY, DLY MOD, DLY TYPE, DLY PING. Synced '
                  'or free (up to ~3 s); PITCH / SPRAY: a grain delay.</p>'
                  '<h4>MORE › CHORUS</h4>' + knobs('CHO RATE', 'CHO DEPTH', 'CHO WIDE', '-') +
                  '<p style="margin-top:6px">Not with LOAD ECO.</p>'))
    b.append('</div>')
    rv = [
        ('ROOM', 'Classic room', 'decay', 'darkens', 'chorus on the tail', 'its speed', 'stereo'),
        ('SPRING', 'Amp spring tank', 'length · decay', 'darkens', 'wobble', 'wobble speed', 'stereo'),
        ('SHIMMER', 'Up an octave on every pass; <b>REV AIR</b> (±24 st: down darker and lower, up airier and higher)', 'decay', 'darkens', 'how much shimmer', '—', '—'),
        ('RESO', '4 strings tuned to the scale; <b>REV PITCH</b> (0–14 steps up), <b>FOLLOW</b> in DAMP\'s place (OFF / T1–T6: the '
                 'chord follows that track\'s notes)', 'how long they ring', 'FOLLOW', 'past half: the chord opens', '—', '—'),
        ('CLOUD', 'Granular wash; SIZE max <b>freezes</b> it; <b>REV AIR</b> (±24 st: the cloud down / up)', 'decay · freeze', 'darkens', 'grains pitched + chorus',
         'grain density', 'grain spread'),
    ]
    t = ('<table><tr><th>TYPE</th><th>WHAT IT IS</th><th>SIZE</th><th>DAMP</th><th>MOD</th><th>RATE</th><th>WIDE</th></tr>' +
         ''.join(f'<tr><td class="n">{e(a)}</td><td>{b_}</td><td>{e(c)}</td><td>{e(d)}</td><td>{e(f)}</td><td>{e(g)}</td>'
                 f'<td>{e(h)}</td></tr>' for a, b_, c, d, f, g, h in rv) + '</table>')
    b.append('<div class="grid g2" style="margin-top:22px;grid-template-columns:2fr 1fr">')
    b.append(card('REVERB · FIVE MODELS', HEAT[2], knobs('REV TYPE', 'REV SIZE', 'REV DAMP', 'REV PRE') + knobs('REV MOD', 'REV RATE', 'REV TONE', 'REV WIDE') +
                  '<div style="margin-top:10px">' + t + '</div>'))
    b.append(card('MASTER', HEAT[6], knobs('CLIP', 'PUNCH', 'DUCK', 'AGE') +
                  '<p style="margin-top:8px"><b>CLIP</b> saturates without getting louder. <b>PUNCH</b> drives the drums: '
                  'denser body. <b>DUCK</b>: the kick pulls the rest down (DUCK TIME, its release, is in FX › MORE › MASTER). '
                  '<b>AGE</b>: old tape in one knob, setting <b>TAPE</b>\'s WOW, FLUTTER, TAPE SAT, HISS and TAPE TONE together (off at 0; the details are in FX › MORE › TAPE). An always-on '
                  '<b>leveler</b> before the limiter keeps patches at a steady volume. <b>DJ FILTER</b> (GLO › LIVE KNOB 4): '
                  'left a low-pass, right a high-pass.</p>'
                  '<h4>PUNCH-IN FX (FX HELD)</h4><p>REPEAT 1/8 · 1/16 · 1/32, GLITCH, TAPE STOP, FREEZE, SHUFFLE; on the notes: OCT−/+, '
                  '½ TEMPO, DECAY −/+, STUTTER, ATTACK +, ARP, RANDOM; sweeps CRUSH THROW. Knobs: STEP LOOP, INTERVAL, MUTATE, '
                  'DJ FILTER. Recordable per section (FX › MORE › PUNCH LANE).</p>'))
    b.append('</div>')
    page('SOUND · FX · MOD', 'engines, effects, modulation, sequencer and songs', ''.join(b), 'sound.html')


if __name__ == '__main__':
    os.makedirs(OUT, exist_ok=True)
    controls()
    sound()
    print('docs/cheatsheet/controls.html, sound.html')
