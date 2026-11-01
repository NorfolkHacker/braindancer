# Task 4b report: English only (spec item 15)

Status: DONE. Pages: web/index_pkg.html, web/editor.html, web/404.html. Tests: web/test_web.mjs.
Removed: JA/ES/EN-fallback tables, the language picker (installer) and toggle (editor), CSS .lang rules,
navigator.language / ?lang= sniffing, applyLang click handlers, dataset.lang re-render keys. `<html lang="en">` on all three.
Renamed applyLang -> applyText (it only fills the English text now). English text unchanged.
Kept on purpose: the VOICE engine's LANG (ES/EN/JA romaji lyrics, a device feature); 404's redirect regex word "actualiz" (matches old Spanish-named paths).
Tests: test_web.mjs translation checks became English-only; new englishOnly() (no JA chars 3040-30FF/4E00-9FFF, lang="en", no switch/tables/sniffing, no Spanish words) for the three pages. Red against HEAD pages, green now.
test_manual.mjs already checks English/no switch: unchanged, passes.
Results: node web/test_web.mjs passed; full suite ALL HOST TESTS PASSED. Skipped: Playwright/Chromium manual page check, emcc emulator.
Other translated pages: none found in web/ (no other es/ja attributes, lang= other than en, or toggles).
