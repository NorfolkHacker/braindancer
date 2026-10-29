# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 NorfolkHacker
"""ACID FM's own name for its origins (3c-1 task 10, spec 5): ACID FM is "built on JIANT FM and Felucca", never "a fork of
JIANT" or "JIANT FM under a new name". Checked: the device's ABOUT (firmware/src/ui_menu.c, its string literals) and the
visible text of the web pages (the installer, the 404, the editor, the emulator, the manual's page and scripts, the
installer's scripts' messages, the landing page: comments left out, they may keep JIANT's version notes; whitespace, line
breaks, &nbsp; and HTML tags between the words do not hide a match). (fix 1) The installer's scripts' messages and report
name no JIANT at all ("JIANT" in capitals in a string; the lower-case "jiant" is the firmware kind they detect), the report
says "Braindancer installer report". The licence record stays as it is: LICENSING.md and every page's copyright lines (checked
present).
(Task 10b) The manual is English only and ACID FM's own: the web manual (its page, scripts, generated nav.js, service
worker), docs/MANUAL.md, the navigation map and the cheat sheets (their generators' output and the generators' strings)
carry no Spanish text, no language switch, no link to MANUAL.es.md (deleted, as the Spanish map), and name JIANT only
on the one credits line "Built on JIANT FM (Juan Jiant) and Felucca (Hügelton Instruments)" (comments left out)."""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BAD = re.compile(r"fork\s+of\s+(JIANT|Felucca)|under\s+a\s+new\s+name|JIANT\s+FM\s+under", re.I)
WEB = ["web/index_pkg.html", "web/404.html", "web/editor.html", "web/emu/index.html", "web/manual/index.html",
       "web/manual/manual.js", "web/manual/data.js", "web/fm1backup.js", "web/fm1diag.js", "web/fm1ota.js", "web/fm1pkg.js",
       "web/landing/index.html"]
SCRIPTS = ["web/fm1diag.js", "web/fm1ota.js", "web/fm1backup.js", "web/fm1pkg.js"]   # (the installer's messages and report)
# the copyright lines that must stay where they are (spec 5: code comments and LICENSING.md unchanged)
KEEP = {
    "LICENSING.md": ["Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments",
                     "Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant) — JIANT FM",
                     "GPL-3.0-only"],
    "web/index_pkg.html": ["Portions Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)"],
    "web/editor.html": ["Portions Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)"],
    "web/emu/index.html": ["Portions Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)"],
    "web/fm1backup.js": ["Portions Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)"],
    "firmware/src/ui_menu.c": ["Copyright (C) 2026 Juan Ignacio Antonucci"],
}


def visible(src: str) -> str:
    """the text with HTML, C and JS comments taken out (a // after a colon is a URL's, kept)"""
    keep = lambda m: "\n" * m.group(0).count("\n")   # (the lines kept: a finding's line is the file's)
    src = re.sub(r"<!--.*?-->", keep, src, flags=re.S)
    src = re.sub(r"/\*.*?\*/", keep, src, flags=re.S)
    return re.sub(r"(^|[\s;{}(),])//[^\n]*", r"\1", src)


def plain(src: str, html: bool) -> str:
    """visible() with the entities for a space as spaces and (HTML) the tags taken out, the lines kept"""
    src = re.sub(r"&nbsp;|&#160;|&#x0*a0;", " ", visible(src), flags=re.I).replace("\u00a0", " ")
    if html:
        src = re.sub(r"</?[A-Za-z][^<>]*>", lambda m: "\n" * m.group(0).count("\n"), src)
    return src


def c_literals(src: str) -> str:
    return " ".join(re.findall(r'"((?:[^"\\\n]|\\.)*)"', visible(src)))


bad = []
about = c_literals((ROOT / "firmware/src/ui_menu.c").read_text(encoding="utf-8"))
if "Built on JIANT FM and Felucca" not in about:
    bad.append('ui_menu.c: ABOUT does not say "Built on JIANT FM and Felucca"')
for m in BAD.finditer(about):
    bad.append(f'ui_menu.c: ABOUT says "{m.group(0)}"')
for who in ("Juan Jiant", "Hugelton Instruments", "NorfolkHacker"):   # (the credits keep every name)
    if who not in about:
        bad.append(f"ui_menu.c: ABOUT no longer names {who}")
# (Braindancer 0.7, Task 1) the firmware's own name: no string literal of the device says "ACID FM" (comments keep their history);
# ABOUT names Braindancer
for c in sorted((ROOT / "firmware/src").glob("*.c")):
    for m in re.finditer(r'"((?:[^"\\\n]|\\.)*)"', visible(c.read_text(encoding="utf-8", errors="replace"))):
        if "ACID FM" in m.group(1):
            bad.append(f'{c.name}: a string literal says "ACID FM": {m.group(1)[:60]!r}')
# the USB product strings (usb.c STR_PROD, the app's and the loader's): UTF-16LE, the length byte right
_usb = (ROOT / "firmware/src/usb.c").read_text(encoding="utf-8")
_prods = re.findall(r"STR_PROD\[\] = \{([^}]*)\};", _usb, re.S)   # (the loader's first, then the app's)
if len(_prods) != 2:
    bad.append("usb.c: STR_PROD is not defined twice (loader, app)")
for _want, _name, _body in zip(("BRAINDANCER UPD", "BRAINDANCER"), ("loader", "app"), _prods):
    _m = re.match(r"(.*)", _body, re.S)
    _b = [x.strip() for x in _m.group(1).split(",")]
    _txt = "".join(x.strip("'") for x in _b[2::2])
    if _txt != _want or int(_b[0]) != 2 + 2 * len(_want) or int(_b[1]) != 3 or any(x != "0" for x in _b[3::2]):
        bad.append(f"usb.c: the {_name} product string is {_txt!r} (length byte {_b[0]}), not {_want!r}")
if "Braindancer" not in about:
    bad.append('ui_menu.c: ABOUT does not name Braindancer')
for f in WEB:
    text = plain((ROOT / f).read_text(encoding="utf-8"), f.endswith(".html"))
    for m in BAD.finditer(text):
        line = text[:m.start()].count("\n") + 1
        bad.append(f'{f}: "{" ".join(m.group(0).split())}" in its visible text (line {line})')
for f in SCRIPTS:
    text = visible((ROOT / f).read_text(encoding="utf-8"))
    for m in re.finditer(r"([\"'`])((?:(?!\1)[^\\\n]|\\.)*)\1", text):
        if "JIANT" in m.group(2):
            line = text[:m.start()].count("\n") + 1
            bad.append(f"{f}: a message names JIANT (line {line}): {m.group(2)[:60]!r}")
if "Braindancer installer report" not in visible((ROOT / "web/fm1diag.js").read_text(encoding="utf-8")):
    bad.append('web/fm1diag.js: the report does not open with "Braindancer installer report"')
for f, lines in KEEP.items():
    src = (ROOT / f).read_text(encoding="utf-8")
    for s in lines:
        if s not in src:
            bad.append(f"{f}: lost its line {s!r}")

# ---- (Task 10b) the manual: English only, JIANT on the credits line alone
CREDITS = "Built on JIANT FM (Juan Jiant) and Felucca (Hügelton Instruments)"
MANUAL = ["web/manual/index.html", "web/manual/manual.js", "web/manual/data.js", "web/manual/nav.js", "web/manual/sw.js",
          "docs/MANUAL.md", "docs/navigation-map.svg", "docs/cheatsheet/controls.html", "docs/cheatsheet/sound.html"]
GENERATORS = ["tools/gen_cheatsheets.py", "tools/gen_nav_map.py", "tools/gen_manual.py"]
HERO = "tools/gen_readme_hero.py"   # (the inherited README images: "built on" wording, no "fork of")
CREDITED = ["web/manual/index.html", "docs/MANUAL.md", "docs/cheatsheet/controls.html", "docs/cheatsheet/sound.html"]
GONE = ["docs/MANUAL.es.md", "docs/navigation-map.es.svg", "docs/navigation-map.es.png"]
LINKERS = MANUAL + GENERATORS + [HERO, "CONTRIBUTING.md", ".github/workflows/emulator.yml", "README.md"]
SPANISH = re.compile(r"[ñ¿¡áéíóúÁÉÍÓÚ]|\b(el|los|las|una|del|para|pista|pistas|perilla|perillas|tecla|teclas|mantené|"
                     r"página|páginas|también|sonido|paso|pasos|cuando|desde|elegí|tocá|manual interactivo|novedades|"
                     r"capa|grabar|borrar|deshacer|recetas|mapa|navegación|español|idioma|de|en|con|la|que|por|pero|"
                     r"sin|muy|esta|este|está)\b")   # (lower case, whole words: EN, LA … as names stay upper case)


def py_strings(src: str) -> str:
    """a Python file's string literals (its comments and docstrings left out), one a line"""
    import ast
    out = []
    tree = ast.parse(src)
    docs = set()
    for node in ast.walk(tree):
        if isinstance(node, (ast.Module, ast.FunctionDef, ast.ClassDef)) and node.body and \
                isinstance(node.body[0], ast.Expr) and isinstance(node.body[0].value, ast.Constant):
            docs.add(id(node.body[0].value))
    for node in ast.walk(tree):
        if isinstance(node, ast.Constant) and isinstance(node.value, str) and id(node) not in docs:
            out.append(node.value)
    return "\n".join(out)


for f in GONE:
    if (ROOT / f).exists():
        bad.append(f"{f}: still there (the manual is English only)")
for f in LINKERS:
    if "MANUAL.es" in (ROOT / f).read_text(encoding="utf-8") or "navigation-map.es" in (ROOT / f).read_text(encoding="utf-8"):
        bad.append(f"{f}: links the Spanish manual or map")
def js_strings(src: str) -> str:
    """a script's string literals (its code and comments left out), one a line"""
    return "\n".join(m.group(2) for m in re.finditer(r"([\"'`])((?:(?!\1)[^\\\n]|\\.)*)\1", visible(src)))


def manual_text(f: str) -> str:
    src = re.sub(r"base64,[A-Za-z0-9+/=]+", "base64,", (ROOT / f).read_text(encoding="utf-8"))   # (a font's data, not words)
    return js_strings(src) if f.endswith(".js") else plain(src, f.endswith((".html", ".svg")))


texts = {f: manual_text(f) for f in MANUAL}
texts.update({f: py_strings((ROOT / f).read_text(encoding="utf-8")) for f in GENERATORS})
for f, text in texts.items():
    for n, line in enumerate(text.splitlines(), 1):
        rest = line.replace(CREDITS, "")
        if re.search(r"jiant", rest, re.I):
            bad.append(f"{f}: names JIANT outside the credits line (line {n}): {line.strip()[:90]!r}")
        if f.endswith(".md"):
            rest = re.sub(r"(?<![*\w])\*[^*\n]+\*(?![*\w])", "", rest)   # (*la, ta, ri*: sung sounds quoted, not prose)
        rest = re.sub(r"\b[\w-]+=\"[^\"]*\"", "", rest)          # (an attribute's value: lang="en" is markup)
        m = SPANISH.search(re.sub(r"(?<![A-Za-z])ñ(?![A-Za-z])", "", rest.replace("Hügelton", "")))   # (the letter named alone:
                                                                                                       # VOICE's lyrics, ñ as NY)
        if m:
            bad.append(f"{f}: Spanish text (line {n}, {m.group(0)!r}): {line.strip()[:90]!r}")
for m in BAD.finditer(py_strings((ROOT / HERO).read_text(encoding="utf-8"))):
    bad.append(f'{HERO}: says "{m.group(0)}"')
for f in CREDITED:
    if CREDITS not in texts[f]:
        bad.append(f"{f}: no credits line {CREDITS!r}")
page = (ROOT / "web/manual/index.html").read_text(encoding="utf-8")
if '<html lang="en">' not in page or re.search(r'class="lang"|data-lang=', page):
    bad.append("web/manual/index.html: a language switch, or not lang=\"en\"")
js = (ROOT / "web/manual/manual.js").read_text(encoding="utf-8")
if re.search(r"setLang|\bes:\s*\{|lang\s*===\s*\"es\"|\"es\"", js):
    bad.append("web/manual/manual.js: still switches language")
if re.search(r'"es":|"label":\{', (ROOT / "web/manual/nav.js").read_text(encoding="utf-8")):
    bad.append("web/manual/nav.js: still carries a second language")

# ---- (Task 10b fix 1) no repo document links a local file that is not there (the deleted Spanish manual and map)
LINKBASE = {"README.md": ".", "docs/JIANT-README.md": ".",   # (the archived upstream README: its links as they were, from
             "docs/MANUAL.md": "docs", "CONTRIBUTING.md": "."}  # the repository's root)
for f, base in LINKBASE.items():
    src = (ROOT / f).read_text(encoding="utf-8")
    for n, line in enumerate(src.splitlines(), 1):
        for m in re.finditer(r"\]\(([^)\s]+)\)|\bsrc=\"([^\"]+)\"", line):
            target = (m.group(1) or m.group(2)).split("#")[0]
            if not target or re.match(r"[a-z]+:", target):
                continue
            if not (ROOT / base / target).exists():
                bad.append(f"{f}: links {target}, which is not in the repository (line {n})")

# ---- (Task 10b fix 1) the manual names pages and rows the firmware has: every "SECTION › PAGE (› ROW)" path in
# docs/MANUAL.md and the web manual, and every page in MANUAL.md's knob tables, read against the firmware's own names
# (the page chains of ui_lens.c through tools/gen_nav_map.py; the rows: every upper-case literal of firmware/src)
sys.path.insert(0, str(ROOT / "tools"))
import gen_nav_map  # noqa: E402
_, chains = gen_nav_map.parse()
PAGES = {}
for c in chains:
    PAGES.setdefault(c["sec"], set()).update(p["name"] for p in c["pages"])
ALLPAGES = set().union(*PAGES.values())
LITERALS = set()
for c in sorted((ROOT / "firmware/src").glob("*.c")):
    for lit in re.findall(r'"([A-Z0-9][A-Z0-9 +/.\-]*)"', c.read_text(encoding="latin-1")):
        LITERALS.add(lit.strip())


def known(words, names):
    """the longest start of words that is one of names (a phrase may run on into the sentence), or None"""
    for k in range(len(words), 0, -1):
        if " ".join(words[:k]) in names:
            return k
    return None


SECS = "|".join(PAGES)
PATH = re.compile(r"\b(" + SECS + r") ?[›>] ?([A-Z0-9][A-Z0-9 +/\-]*?)((?: ?[›>] ?[A-Z0-9][A-Z0-9 +/\-]*?)*)(?=$|[^A-Z0-9 +/\-›>])")
for f, text in (("docs/MANUAL.md", (ROOT / "docs/MANUAL.md").read_text(encoding="utf-8")),
                ("web/manual/data.js", js_strings((ROOT / "web/manual/data.js").read_text(encoding="utf-8")))):
    for n, line in enumerate(text.replace("**", "").splitlines(), 1):
        for m in PATH.finditer(line):
            sec, rest = m.group(1), [m.group(2)] + [x for x in re.split(r" ?[›>] ?", m.group(3)) if x]
            words = rest[0].split()
            if not known(words, PAGES[sec]):
                bad.append(f"{f}: {sec} › {rest[0]}: no such page in {sec} (line {n})")
                continue
            for row in rest[1:]:
                if not known(row.split(), LITERALS | ALLPAGES):
                    bad.append(f"{f}: {sec} › … › {row}: no such page or row in the firmware (line {n})")
    if f.endswith(".md"):
        intable = False
        for n, line in enumerate(text.splitlines(), 1):
            if line.startswith("| Page | KNOB 1"):
                intable = True
                continue
            if not line.startswith("|"):
                intable = False
            elif intable and not re.match(r"\|\s*-", line):
                cell = re.sub(r"\(.*?\)", "", line.split("|")[1]).replace("**", "").strip()
                if cell and not known(cell.split(), ALLPAGES):
                    bad.append(f"{f}: knob table page {cell!r} is no firmware page (line {n})")

# ---- (ACID FM 3c-2 task 9, spec 5.4) no device string or example name is Spanish: the string literals of firmware/src/*.c
# and of the render scenes behind the manual's screens (tests/ui_render.c) name none of the inherited Spanish words (whole
# words, any case). The ES lyrics are a feature and stay: the generated bank (felucca_lyrics.h, no .c), sing.c's ES tables
# and the WORDS scene's ES lyric (where the ES bank is shown and tested) are left out by name
ES_WORDS = re.compile(r"\b(LENTE|NOCHE|NUBE|BOSQUE|MANO|RIELES|PISTAS|CORTE|CARRERA)\b", re.I)
ES_TABLES = {"firmware/src/sing.c": ["SCAT_ES", "TPL_ES", "VOCABLE"], "tests/ui_render.c": ["W"]}   # (the WORDS scene's: ES,
                                                                                                   # EN, JA and an edit)


def es_free(f: str, src: str) -> str:
    """src with the named ES tables' initialisers taken out (the lines kept)"""
    for name in ES_TABLES.get(f, []):
        if not re.search(r"\bstatic const char \*const " + name + r"\[", src):
            bad.append(f"{f}: no table {name} (ES_TABLES names one that is gone)")
        src = re.sub(r"\bstatic const char \*const " + name + r"\[[^=;]*=\s*\{.*?\};",
                     lambda m: "\n" * m.group(0).count("\n"), src, flags=re.S)
    return src


for f in [str(c.relative_to(ROOT)) for c in sorted((ROOT / "firmware/src").glob("*.c"))] + ["tests/ui_render.c"]:
    text = es_free(f, visible((ROOT / f).read_text(encoding="latin-1")))
    for m in re.finditer(r'"((?:[^"\\\n]|\\.)*)"', text):
        w = ES_WORDS.search(m.group(1))
        if w:
            bad.append(f"{f}: a Spanish word {w.group(0)!r} in a string (line {text[:m.start()].count(chr(10)) + 1}): "
                       f"{m.group(1)[:60]!r}")

if bad:
    print("\n".join("FAIL: " + b for b in bad))
    sys.exit(1)
print(f"ok   built on JIANT FM and Felucca: ABOUT and {len(WEB)} web files; the copyright lines kept; the manual English "
      f"only, JIANT on its credits line alone ({len(MANUAL) + len(GENERATORS)} files)")
