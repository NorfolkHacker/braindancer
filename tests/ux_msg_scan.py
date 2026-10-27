#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 NorfolkHacker
"""ACID FM 3a: the texts HOME draws in its own faces, gathered from firmware/src at test-build time (so a message added later
is checked too, no list to keep by hand):

  ux_msg_scan.py OUT.h

UX_MSG_HDR (drawn in UF_HDR by HOME's status line, ux_home.c ux_home_status): every string literal among the arguments of
ui_message( / ui_say( / ux_status( (ternaries and concatenations too: the call's parentheses are matched) and of
str_cpy(ui.msg / ui.msg2, ...) (msg2 becomes the message after msg), the ux_*.c #define NAME "..." texts (UXV_INTRO,
UX_STOP) HOME's hints (ux_nav.c UXH_PAGES) and the ten GEN genre names (gen_tables.c GEN_SHOWN_NAME, said by gen_go's "GEN: name seed"). UX_MSG_LBL (UF_LBL, the footer's face): the literals of ux_footer( calls; (3b task 9) the held layers' key names (ui_layer.c
ly_keyname's str_cpy(nm, .., PF_NAME: ux_layer.c's MAP) and STATE's chips (lst_chip(: UF_HDR)
and UXH_PAGES (the footer's HOME . page); (3b) both: the literals of the ux_*.c tables named UX*_* (ux_section.c UXS_NAME and UXS_LABEL's
plain words, ux_more.c UXM_HEAD / UXM_ACT: knob labels, MORE rows; the values a MORE row can take: tests/ui_test.c; 3c-1 task 6: one named
UX*_*_LBL UF_LBL's alone, a picture's words); (3b task 4)
both: ui_lens.c SEC_NAME and the chains' CE_* / CG_* names (the footer's SECTION . PAGE, the hint "EDIT AGAIN: .."), the settings'
labels (params.c PD( / PE(, the engines' {"LABEL", F_..} descriptors: a page's knob row and readout; UXS_LABEL's renamed
ones left out, never drawn). tests/ux_font_test.c checks every byte of them has a glyph. Comments are blanked first; the script fails if it finds fewer
calls than a plain grep for 'ui_message("' would (a pattern gone stale) or under 150 texts in all.
"""
import glob
import re
import sys
from pathlib import Path

SRC = Path(__file__).resolve().parents[1] / "firmware" / "src"
CALL = re.compile(r'\b(ui_message|ui_say|ux_status|ux_footer|lst_chip)\s*\(|\bstr_cpy\s*\(\s*ui\.msg2?\s*,')
LIT = re.compile(r'"((?:[^"\\\n]|\\.)*)"')


def call_args(s, i):                       # the text from i to the ')' closing the call opened just before i
    d, j = 1, i
    while d and j < len(s):
        c = s[j]
        if c == '"':
            j += 1
            while s[j] != '"':
                j += 2 if s[j] == '\\' else 1
        elif c == "'":
            j += 1
            while s[j] != "'":
                j += 2 if s[j] == '\\' else 1
        elif c == '(':
            d += 1
        elif c == ')':
            d -= 1
        j += 1
    return s[i:j - 1]


def main(out):
    hdr, lbl, plain, got_msg, tables, chains, labels = [], [], 0, 0, 0, 0, []
    for f in sorted(glob.glob(str(SRC / "*.c"))) + sorted(glob.glob(str(SRC / "ux_*.h"))):   # (3b: ux_more.h UXM_K34 ..)
        s = open(f, encoding="latin-1").read()
        s = re.sub(r'/\*.*?\*/', lambda m: re.sub(r'[^\n]', ' ', m.group()), s, flags=re.S)
        s = re.sub(r'//[^\n]*', '', s)
        if f.endswith(".h"):                          # (a ux header: its #define UX* texts only, ux_more.h UXM_K34)
            hdr.extend(LIT.findall("\n".join(re.findall(r'^#define\s+UX\w*\s+("(?:[^"\\\n]|\\.)*")', s, flags=re.M))))
            continue
        plain += len(re.findall(r'\bui_message\s*\(\s*"', s))
        for m in CALL.finditer(s):
            a = call_args(s, m.end())
            if m.group(0).startswith("str_cpy"):
                a = a.rsplit(",", 1)[0]               # (not the size argument)
            lits = LIT.findall(a)
            if m.group(1) == "ui_message" and a.lstrip().startswith('"'):
                got_msg += 1
            (lbl if m.group(1) == "ux_footer" else hdr).extend(lits)
        t = re.search(r'GEN_SHOWN_NAME\[[^]]*\]\s*=\s*\{(.*?)\};', s, flags=re.S)
        if t:                                         # (3b: GEN's genre names, said in the status line: "GEN: " + name + seed)
            hdr.extend(LIT.findall(t.group(1)))
        if Path(f).name.startswith("ux_"):
            hdr.extend(LIT.findall("\n".join(re.findall(r'^#define\s+UX\w*\s+("(?:[^"\\\n]|\\.)*")', s, flags=re.M))))
            t = re.search(r'UXH_PAGES\[[^]]*\]\s*=\s*\{(.*?)\};', s, flags=re.S)
            if t:                                     # (the names in the footer, the hints in the status line: both)
                hdr.extend(LIT.findall(t.group(1)))
                lbl.extend(LIT.findall(t.group(1)))
            for t in re.finditer(r'\b(UX[A-Z]*_[A-Z_]+)\s*\[[^]]*\]\s*=\s*\{(.*?)\};', s, flags=re.S):
                if t.group(1).endswith('_LBL'):       # (3c-1 task 6: a picture's own words, drawn in UF_LBL alone: ux_glo.c
                    tables += 1                       # UXG_ARROW_LBL, LIVE's REVERSE / PING-PONG arrows)
                    lbl.extend(LIT.findall(t.group(2)))
                elif t.group(1) != 'UXH_PAGES':       # (3b: ux_section.c UXS_LABEL, ux_more.c UXM_HEAD / UXM_ACT: a knob's
                    tables += 1                       # label, a MORE row, the status line: both faces)
                    hdr.extend(LIT.findall(t.group(2)))
                    lbl.extend(LIT.findall(t.group(2)))
        if Path(f).name == "ui_layer.c":              # (3b task 9: the held layers' MAP, ux_layer.c's list in UF_LBL: the keys'
            t = re.search(r'PF_NAME\[[^]]*\]\s*=\s*\{(.*?)\};', s, flags=re.S)   # names, ly_keyname's str_cpy(nm, ..) and
            if t:                                     # the effects' PF_NAME; STATE's chips, lst_chip(, in UF_HDR: above)
                lbl.extend(LIT.findall(t.group(1)))
            for m in re.finditer(r'\bstr_cpy\s*\(\s*nm\s*,', s):
                lbl.extend(LIT.findall(call_args(s, m.end()).rsplit(",", 1)[0]))
        if Path(f).name == "ui_lens.c":               # (3b task 4: the footer's SECTION . PAGE, the status line's hint
            for t in re.finditer(r'\b(SEC_NAME|C[EG]_[A-Z0-9_]+)\s*\[[^]]*\]\s*=\s*\{(.*?)\};', s, flags=re.S):
                chains += 1                           # "EDIT AGAIN: NEXT PAGE": the sections' and the chains' names)
                hdr.extend(LIT.findall(t.group(2)))
                lbl.extend(LIT.findall(t.group(2)))
        if Path(f).name == "params.c" or Path(f).name.startswith("eng_"):
            # (3b task 4: a page's knob labels and the status line's readout: the settings' labels, TP / GP's PD( PE( and
            # the engines' .edit[] / mode relabels {"LABEL", F_..}; those ux_section.c UXS_LABEL renames are never drawn)
            for t in re.finditer(r'\bP[DE]\(\s*"((?:[^"\\\n]|\\.)*)"|\{\s*"((?:[^"\\\n]|\\.)*)"\s*,\s*F_[A-Z]+\b', s):
                labels.append(t.group(1) if t.group(1) is not None else t.group(2))
    sx = open(SRC / "ux_section.c", encoding="latin-1").read()
    t = re.search(r'UXS_LABEL\[\]\s*=\s*\{(.*?)\};', sx, flags=re.S)
    renamed = set(re.findall(r'\{\s*"((?:[^"\\\n]|\\.)*)"\s*,', t.group(1))) if t else set()
    for x in labels:
        if x and x != "-" and x not in renamed:
            hdr.append(x)
            lbl.append(x)
    if got_msg < plain or len(hdr) < 150 or tables < 3 or chains < 10 or len(labels) < 150:
        sys.exit(f"ux_msg_scan: {got_msg} of {plain} ui_message(\"...\") calls, {len(hdr)} texts, {chains} chain tables, "
                 f"{len(labels)} labels: the pattern is stale")
    with open(out, "w", encoding="latin-1") as o:
        o.write("/* generated by tests/ux_msg_scan.py from firmware/src: do not edit */\n")
        for name, xs in (("UX_MSG_HDR", hdr), ("UX_MSG_LBL", lbl)):
            o.write(f"static const char *const {name}[] = {{\n")
            for x in dict.fromkeys(xs):
                o.write(f'    "{x}",\n')
            o.write('    0,\n};\n')
    print(f"ux_msg_scan: {len(set(hdr))} status-line texts, {len(set(lbl))} footer / label texts ({tables} ux tables, "
          f"{chains} chain tables, {len(labels)} setting labels) -> {out}")


if __name__ == "__main__":
    main(sys.argv[1])
