// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)
// Portions Copyright (C) 2026 NorfolkHacker
//
// Checks of the interactive manual (web/manual). Run from the repo root:  node web/test_manual.mjs
// - data.js / nav.js / manual.js load without a browser; nav.js is what tools/gen_manual.py makes from the firmware
// - every control on the FM-1 drawing (8 knobs, 14 buttons, 27 keys) has an entry; every combo names real controls
// - every text in English, one string (3c-1 task 10b: no Spanish, no language switch); every screen the page names is in
//   shots/ (a real 240 x 240 PNG); what's new is ACID FM's own versions
// - the search: case and accents ignored, filtered by section, the right command first
// - the page in Chromium (Playwright, when installed): no error, the drawing drawn, a hover fills the panel, the search
//   filters, English, no horizontal scroll at 390 px; skipped without Playwright
import { execFileSync } from "node:child_process";
import { existsSync, readFileSync, readdirSync } from "node:fs";
import { createRequire } from "node:module";
import { join } from "node:path";
import vm from "node:vm";

let failed = 0;
const ok = (cond, what) => { console.log(`${what.padEnd(72)} ${cond ? "ok" : "FAIL"}`); if (!cond) failed++; };
const HERE = new URL(".", import.meta.url).pathname, DIR = join(HERE, "manual"), ROOT = join(HERE, "..");

/* the scripts as the page loads them, in one context without a document */
const ctx = { console };
ctx.window = ctx;
vm.createContext(ctx);
for (const f of ["data.js", "nav.js", "manual.js"]) vm.runInContext(readFileSync(join(DIR, f), "utf8"), ctx, { filename: f });
const M = ctx.JMAN, N = ctx.JNAV, A = ctx.JMANUAL;
ok(M && N && A, "manual: data.js, nav.js and manual.js load (no DOM)");

/* nav.js is the firmware's: tools/gen_manual.py --check */
try {
  execFileSync("python3", [join(ROOT, "tools/gen_manual.py"), "--check"], { stdio: "pipe" });
  ok(true, "manual: nav.js matches the firmware chains, every named screen in shots/");
} catch (e) { console.log(String(e.stdout || "")); ok(false, "manual: nav.js matches the firmware chains, every named screen in shots/"); }
ok(N.sections.join() === "HOME,EDIT,SEQ,FX,GLO,ENV,LFO,SCL,ARP,SAVE", "manual: the ten sections of the firmware");
const fwv = /FELUCCA_VERSION "(v[^"]+)"/.exec(readFileSync(join(ROOT, "firmware/src/felucca.c"), "utf8"))[1];
ok(N.version === fwv, `manual: the map is the firmware's version (${fwv})`);

/* every control on the drawing has an entry */
ok(A.CONTROL_IDS.length === 22 && A.KEYS.length === 27, "manual: 8 knobs + 14 buttons and 27 keys drawn");
ok(A.KEYS.filter((k) => k.black).length === 11 && A.KEYS[0].name === "F3" && A.KEYS[26].name === "G5", "manual: keys F3..G5, 11 black");
const missing = A.CONTROL_IDS.filter((id) => { const c = M.controls[id]; return !c || typeof c.alone !== "string" || !c.alone.trim(); });
ok(!missing.length, "manual: every knob and button has its text" + (missing.length ? " (" + missing + ")" : ""));
ok(Object.keys(M.controls).every((id) => A.CONTROL_IDS.includes(id)), "manual: no entry for a control the drawing lacks");
ok(A.KEYS.every((k) => A.keyLayers(k.name).length > 0) && typeof M.keyAlone === "string" && M.keyAlone, "manual: every key has its text and at least one layer");
const firmwareButtons = /B_NAME\[NB\] = \{([^}]*)\}/.exec(readFileSync(join(ROOT, "firmware/src/panel.c"), "utf8"))[1].match(/"([^"]+)"/g).map((s) => s.slice(1, -1));
ok(firmwareButtons.every((b) => A.CONTROL_IDS.includes(b)), "manual: the buttons are panel.c's (FX SCL ENV LFO EDIT GLO HOME SAVE ARP SEQ PLAY REC OCT±)");
const keyNames = new Set(A.KEYS.map((k) => k.name));
const tokOk = (t) => A.CONTROL_IDS.includes(t) || keyNames.has(t) || t === "STEP";
const combos = [...M.commands.map((c) => c.k), ...Object.values(M.controls).flatMap((c) => (c.combos || []).map((x) => x.k))];
const bad = combos.filter((k) => !A.tokens(k).every(tokOk));
ok(!bad.length, `manual: every combo (${combos.length}) names real controls` + (bad.length ? " (" + bad + ")" : ""));
for (const [id, ly] of Object.entries(M.layers))
  if (ly.keys) ok(Object.keys(ly.keys).every((k) => keyNames.has(k)), `manual: layer ${id}: its keys exist`);

/* one language: every text a plain English string (no [es, en] pair left), the news ACID FM's own */
const texts = [], pairs = [];
const walk = (v, key) => {
  if (typeof v === "string") { if (["t", "d", "alone", "held", "note"].includes(key) || key === "item") texts.push(v); }
  else if (Array.isArray(v)) { if (v.length === 2 && v.every((x) => typeof x === "string") && key !== "shots") pairs.push(v); v.forEach((x) => walk(x, key === "items" || key === "steps" ? "item" : key)); }
  else if (v && typeof v === "object") Object.entries(v).forEach(([k, x]) => walk(x, k));
};
walk({ c: M.commands, r: M.recipes, n: M.news, s: M.sections, k: M.controls });
ok(texts.length > 300 && texts.every((x) => x.trim()) && !pairs.length, `manual: ${texts.length} texts, each one English string` + (pairs.length ? ` (${pairs.length} pairs left)` : ""));
ok(M.news[0].v === fwv.slice(1) && M.news[M.news.length - 1].v === "0.1" && M.news.every((n) => /^0\.[1-9](\.[0-9])?$/.test(n.v)),
   "manual: what's new from the current version back to ACID FM 0.1, nothing older");
const ids = M.sections.map((s) => s.id);
ok([...M.commands, ...M.recipes].every((e) => ids.includes(e.s)), "manual: every command and recipe in a known section");
for (const s of ["home", "edit", "seq", "glo", "fx", "scl", "mod", "save", "menu", "drum", "gen", "mutate", "punch", "song"])
  if (!M.commands.some((c) => c.s === s)) ok(false, `manual: section ${s} has commands`);
ok(["live", "unq", "locks", "clear", "auto", "song", "gen", "mutate", "punch", "liveP", "learn", "usb", "backup", "cpu", "trouble"]
   .every((id) => M.recipes.some((r) => r.id === id)), "manual: the task recipes asked for are all there");

/* the screens: every one named is a 240 x 240 PNG in shots/, none left over */
const shots = A.allShots();
const pngOk = (n) => { const f = join(DIR, "shots", n + ".png"); if (!existsSync(f)) return false; const b = readFileSync(f);
  return b.readUInt32BE(0) === 0x89504e47 && b.readUInt32BE(16) === 240 && b.readUInt32BE(20) === 240; };
const noShot = shots.filter((n) => !pngOk(n));
ok(!noShot.length, `manual: ${shots.length} screens linked, each a 240 x 240 PNG in shots/` + (noShot.length ? " (" + noShot + ")" : ""));
const extra = readdirSync(join(DIR, "shots")).filter((f) => !shots.includes(f.replace(/\.png$/, "")));
ok(!extra.length, "manual: no screen in shots/ the page does not use" + (extra.length ? " (" + extra + ")" : ""));
{ /* (ACID FM 3b task 4: a synth's EDIT chain is 5 pages, was 10: a share, not a count) */
  const ps = N.chains.flatMap((c) => c.pages);
  ok(ps.filter((p) => p.shot).length >= 0.9 * ps.length, "manual: the map's pages open their screen (90 % or more)");
}

/* the page: its scripts and anchors */
const html = readFileSync(join(DIR, "index.html"), "utf8");
for (const id of ["fm1", "info", "q", "secs", "cmds", "recipes", "map", "news", "emubox", "ctlsel", "layers"])
  ok(html.includes(`id="${id}"`), `manual: index.html has #${id}`);
ok(["data.js", "nav.js", "manual.js"].every((f) => html.includes(`src="${f}"`)), "manual: index.html loads its three scripts");
ok(!/<script[^>]+src="https?:/.test(html) && (html.match(/https:\/\/fonts\.googleapis\.com/g) || []).length <= 2,
   "manual: no external script, only the landing's Google Fonts");
ok(!/JIANT Stems|Probe|probe/.test(readFileSync(join(DIR, "data.js"), "utf8") + html), "manual: no word of the Stems app beyond 'separate product'");
ok(readFileSync(join(HERE, "landing/index.html"), "utf8").includes('href="manual/"'), "manual: the landing page links it");
ok(readFileSync(join(ROOT, "README.md"), "utf8").includes("norfolkhacker.github.io/braindancer/manual/"), "manual: the README links it");
ok(/make_site\.py manual build\/pages/.test(readFileSync(join(ROOT, ".github/workflows/emulator.yml"), "utf8")), "manual: the workflow publishes it");

/* the search */
const first = (q, s) => { const r = A.search(q, s); return r[0] && r[0].k; };
ok(first("clear step") === "SEQ+G5", "search: 'clear step' -> SEQ + G5 first");
ok(first("undo") === "GLO+G4", "search: undo -> GLO + G4");
ok(A.search("UNQUANTIZED").some((c) => /REC SNAP/.test(c.t)), "search: case ignored (UNQUANTIZED -> REC SNAP)");
ok(A.search("micro").some((c) => /MICRO/.test(c.t)), "search: MICRO found");
ok(A.search("midi learn").length >= 1 && A.search("midi learn", "menu").every((c) => c.s === "menu"), "search: MIDI LEARN, filtered to MENU");
ok(A.search("", "seq").length === M.commands.filter((c) => c.s === "seq").length, "search: a section alone lists all its commands");
ok(A.search("zzzqqq").length === 0, "search: nonsense finds nothing");
ok(A.search("stems", "all", M.recipes).some((r) => r.id === "usb"), "search: the recipes too (stems -> USB modes)");
ok(A.emuHint("SEQ+G5") === "Emulator: hold «0» + «O»" && A.emuHint("GLO+G4") === "Emulator: hold «6» + «.»",
   "manual: the emulator's keys for a combo (SEQ = 0, G5 = O, GLO = 6, G4 = .)");

/* the page in Chromium, when Playwright is there */
let chromium = null;
try { chromium = createRequire(import.meta.url)("playwright").chromium; } catch (e) {
  try { chromium = createRequire("/opt/node-tools/node_modules/")("playwright").chromium; } catch (e2) { /* none */ }
}
if (!chromium) console.log("manual: page in Chromium (no Playwright)".padEnd(72) + " skip");
else {
  const exe = process.env.CHROMIUM || "/opt/pw-browsers/chromium-1194/chrome-linux/chrome";
  const b = await chromium.launch(existsSync(exe) ? { executablePath: exe } : {});
  for (const [w, h] of [[1280, 860], [390, 844]]) {
    const p = await b.newPage({ viewport: { width: w, height: h } });
    const errs = [];
    p.on("pageerror", (e) => errs.push(e.message));
    p.on("requestfailed", (r) => { if (!/fonts\.g/.test(r.url())) errs.push("failed " + r.url()); });
    await p.route(/fonts\.(googleapis|gstatic)\.com/, (r) => r.abort());      /* offline: the page must not need them */
    await p.goto("file://" + join(DIR, "index.html"));
    const n = await p.locator("#fm1 .ctl").count();
    ok(n === 49, `page ${w}px: the drawing has its 49 controls (${n})`);
    await p.locator('#fm1 .ctl[data-ctl="SEQ"]').click();
    ok(/SEQ TOOLS/.test(await p.locator("#info").innerText()), `page ${w}px: SEQ touched -> its entry in the panel`);
    await p.locator('#layers button[data-layer="FX"]').click();
    ok((await p.locator('#fm1 .ctl[data-ctl="F3"] .klay').textContent()) === "RPT 1/8", `page ${w}px: the FX layer on the keys (F3 RPT 1/8)`);
    await p.fill("#q", "clear step");
    ok(/CLR STEP/.test(await p.locator("#cmds .cmd").first().innerText()), `page ${w}px: the search filters (clear step -> CLR STEP)`);
    await p.locator('.pg[data-ce="CE_SEQ"]').nth(2).click();   /* (ACID FM 3c-1 task 4: STEPS STEP EXTRAS PATTERN MORE) */
    ok(/SEQ · PATTERN · PATTERN/.test(await p.locator("#pagebox").innerText()), `page ${w}px: a page of the map opens its screen and knobs`);
    const imgsOk = await p.evaluate(() => Promise.all([...document.images].filter((i) => i.src.includes("/shots/")).map((i) =>
      i.complete ? i.naturalWidth === 240 : new Promise((r) => { i.onload = () => r(i.naturalWidth === 240); i.onerror = () => r(false); }))).then((a) => a.every(Boolean)));
    ok(imgsOk, `page ${w}px: the screens shown load`);
    ok((await p.locator("h2").nth(1).innerText()) === "How do I…?" && !(await p.locator(".lang").count()), `page ${w}px: English, no language switch`);
    ok(await p.evaluate(() => document.documentElement.scrollWidth <= innerWidth), `page ${w}px: no horizontal scroll`);
    ok(!errs.length, `page ${w}px: no error` + (errs.length ? " (" + errs.join("; ") + ")" : ""));
    await p.close();
  }
  await b.close();
}

console.log(failed ? `\n${failed} FAILED` : "\nall ok");
process.exit(failed ? 1 : 0);
