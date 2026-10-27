/* SPDX-License-Identifier: GPL-3.0-only
 * Portions Copyright (C) 2026 Juan Ignacio Antonucci (Juan Jiant)
 * Portions Copyright (C) 2026 NorfolkHacker
 * End-to-end channel input and external-clock tests using the real queues,
 * UART parser, ownership model, sequencer, voice renderer and UI source. */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"

static void midi_test_reset(void)
{
    ui_power_on();
    memset(midi_ch, 0, sizeof midi_ch); memset(midi_sel_on, 0, sizeof midi_sel_on);
    memset(midi_owners, 0, sizeof midi_owners); memset(midi_bend_q8, 0, sizeof midi_bend_q8);
    memset(midi_bend_target, 0, sizeof midi_bend_target); memset(&midi_clock, 0, sizeof midi_clock);
    memset(&um, 0, sizeof um); memset(midi_in_source, 0, sizeof midi_in_source);
    mi_r = mi_w = 0; midi_in_overflow = 0; midi_beat_samples = 0;
    fm1_in.notes = kb_prev = 0; fm1_ms = 0; song.sel = 0;
    events_block(CTL);
}
static void queued(uint32_t st, uint32_t d1, uint32_t d2, uint32_t source)
{
    midi_enqueue((st >= 0xF8u ? 0xFu : st >> 4) | st << 8 | d1 << 16 | d2 << 24, source);
    events_block(CTL);
}
static int gate_note(const track_t *t, uint32_t note)
{
    for (uint32_t i = 0; i < NVOICE; i++) if (t->v[i].active && t->v[i].gate && t->v[i].note == note) return 1;
    return 0;
}
static int controls_test(void)
{
    int bad = 0; midi_test_reset(); track_t *t = &trk[0];
    queued(0xE0, 127, 127, 1);
    bad += check("USB maximum bend is exactly the default +2 semitones", midi_bend_target[0] == 512);
    queued(0xE0, 0, 0, 1);
    bad += check("USB minimum bend is exactly the default -2 semitones", midi_bend_target[0] == -512);
    queued(0xE0, 0, 64, 1);
    bad += check("bend centre is zero", midi_bend_target[0] == 0);
    queued(0xB0, 101, 0, 1); queued(0xB0, 100, 0, 1); queued(0xB0, 6, 12, 1); queued(0xB0, 38, 50, 1);
    queued(0xE0, 127, 127, 1);
    bad += check("RPN0 supports semitone and cent bend sensitivity", midi_ch[0].semis == 12 && midi_ch[0].cents == 50 && midi_bend_target[0] == 3200);
    queued(0xB0, 99, 0, 1); queued(0xB0, 6, 24, 1);
    bad += check("selecting NRPN cancels RPN data entry", midi_ch[0].semis == 12);
    queued(0x90, 60, 100, 1); int32_t out[CTL]; int32_t before = midi_bend_q8[0];
    track_render(t, out, CTL);
    bad += check("bend is smoothed toward the target in the real voice renderer", midi_bend_q8[0] > before && midi_bend_q8[0] < midi_bend_target[0]);
    for (uint32_t i = 0; i < 64; i++) track_render(t, out, CTL);
    bad += check("bend smoothing reaches the exact target without changing saved pitch", midi_bend_q8[0] == 3200 && t->p[P_TRANS] == 0 && t->v[0].note == 60);
    queued(0xB0, 121, 0, 1);
    bad += check("Reset Controllers centres bend and preserves RPN sensitivity", !midi_bend_target[0] && midi_ch[0].semis == 12 && gate_note(t, 60));
    queued(0xB0, 1, 87, 1); queued(0xB0, 11, 23, 1); queued(0xD0, 90, 0, 1);
    bad += check("existing modwheel/expression/aftertouch stay routed to the track", t->mw == 87 && t->ex_off == 104 && t->at == 90);
    queued(0xE3, 127, 127, 1);
    bad += check("drum track ignores pitch bend", !midi_bend_target[3]);
    return bad;
}
static int sustain_test(void)
{
    int bad = 0; midi_test_reset(); track_t *t = &trk[0];
    queued(0x90, 60, 100, 1); queued(0xB0, 64, 127, 1); queued(0x80, 60, 0, 1);
    bad += check("pedal holds a released synth note and its owner", gate_note(t, 60) && mn_get(0, 60) == (1u | MIDI_PEDAL_NOTE));
    queued(0xB0, 64, 0, 1);
    bad += check("pedal release releases its held note", !gate_note(t, 60) && !mn_get(0, 60) && !midi_owners[0]);
    queued(0x90, 62, 100, 1); queued(0xB0, 64, 127, 1); queued(0xB0, 123, 0, 1);
    bad += check("All Notes Off honours sustain rather than hard-killing", gate_note(t, 62) && (mn_get(0, 62) & MIDI_PEDAL_NOTE));
    queued(0xB0, 121, 0, 1);
    bad += check("Reset Controllers releases pedal-held notes", !gate_note(t, 62) && !mn_get(0, 62));
    queued(0x93, 36, 100, 1); queued(0xB3, 64, 127, 1); queued(0x83, 36, 0, 1);
    bad += check("drum note-offs do not accumulate pedal-held owners", !mn_get(3, 36) && !midi_owners[3]);
    queued(0x90, 67, 100, 1); queued(0xB0, 64, 127, 1); queued(0x80, 67, 0, 1); queued(0xB0, 120, 0, 1);
    bad += check("All Sound Off ignores pedal and discards all track ownership", !mn_get(0, 67) && !midi_owners[0] && !t->nheld && !gate_note(t, 67));
    return bad;
}
static int ownership_test(void)
{
    int bad = 0; midi_test_reset(); host_route_sel(1); track_t *t = &trk[0];
    queued(0x94, 60, 100, 1); queued(0x95, 60, 100, 2);
    bad += check("USB/TRS channels can share one sounding note with independent owners", midi_owners[0] == 2u && gate_note(t, 60));
    queued(0x84, 60, 0, 1);
    bad += check("one channel's note-off preserves another channel's hold", midi_owners[0] == 1u && gate_note(t, 60));
    queued(0x85, 60, 0, 2);
    bad += check("last channel owner releases the shared note", !midi_owners[0] && !gate_note(t, 60));
    fm1_in.notes = 1u; events_block(CTL); uint32_t local = kb_note[0];
    queued(0x94, local, 100, 1); queued(0x84, local, 0, 1);
    bad += check("MIDI release cannot cut a still-held local keyboard note", gate_note(t, local));
    fm1_in.notes = 0; events_block(CTL);
    bad += check("local release after MIDI release ends the shared note", !gate_note(t, local));
    queued(0x94, 65, 100, 1); song.sel = 1; queued(0x84, 65, 0, 1);
    bad += check("note-off follows its note-on across selected-track changes", !gate_note(&trk[0], 65) && !mn_get(4, 65));
    song.sel = 0; queued(0x94, 67, 100, 1); queued(0xB4, 64, 127, 1); queued(0x84, 67, 0, 1);
    panic_req |= 1u; events_block(CTL);
    queued(0x90, 67, 100, 1); queued(0xB4, 64, 0, 1);
    trk[0].xf = 0; events_block(CTL);                   /* (0.6.13: the old sound's fade done: the note asked for plays) */
    bad += check("preset panic forgets old pedal owners so later pedal-up preserves new notes", !mn_get(4, 67) && gate_note(&trk[0], 67));
    for (uint32_t i = 0; i <= MQ; i++) midi_enqueue(0x09u | 0x90u << 8 | 70u << 16 | 100u << 24, 1);
    events_block(CTL);
    bad += check("queue overflow recovers ownership and releases stuck notes", !midi_in_overflow && mi_r == mi_w && !midi_owners[0] && !gate_note(&trk[0], 67));
    return bad;
}
static void clock_setup(uint32_t mode)
{
    midi_test_reset(); song.g[G_CLOCK] = (int16_t)mode;
    trk[0].p[P_SLEN] = 16; trk[0].p[P_SDIV] = 2;
    for (uint32_t i = 0; i < 16; i++) trk[0].step[i] = (step_t){{(uint8_t)(60 + i)}, 1, ST_NOTE, 0, 100};
    events_block(CTL);
}
static void clock_packet(uint32_t source, uint32_t status, uint32_t ms)
{
    fm1_ms = ms;
    if (source == 2u) { um_byte(status); events_block(CTL); }
    else queued(status, 0, 0, source);
}
static void clock_to(uint32_t source, uint32_t start, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++) clock_packet(source, 0xF8, start + (i * 125u) / 6u);
}
static int clock_test(uint32_t source)
{
    int bad = 0; clock_setup(source);
    clock_packet(source == 1u ? 2u : 1u, 0xFA, 1);
    bad += check(source == 1 ? "USB mode ignores TRS Start" : "TRS mode ignores USB Start", !song.playing);
    clock_packet(source, 0xFA, 2);
    bad += check(source == 1 ? "USB Start begins at step zero" : "TRS Start begins at step zero", song.playing && trk[0].seq_idx == 0u);
    clock_to(source, 2, 7);
    bad += check(source == 1 ? "USB six pulses advance exactly one 16th step" : "TRS six pulses advance exactly one 16th step", trk[0].seq_idx == 1u && song.g[G_BPM] == 120);
    fm1_ms = 132; events_block(CTL); uint16_t idx = trk[0].seq_idx; uint32_t pos = trk[0].seq_pos;
    clock_packet(source, 0xFC, 133);
    bad += check("external Stop preserves step position and releases sequence notes", !song.playing && trk[0].seq_idx == idx && trk[0].seq_pos == pos && !trk[0].seq_n);
    clock_packet(source, 0xFB, 150);
    bad += check("external Continue resumes the existing step position", song.playing && trk[0].seq_idx == idx && trk[0].seq_pos == pos);
    clock_packet(source, 0xF8, 151); fm1_ms = 672; events_block(CTL);
    bad += check("lost clock times out and releases transport notes", !song.playing && !trk[0].seq_n);
    clock_packet(source, 0xFA, 700); clock_to(source, 700, 7);
    bad += check("Start after timeout restarts cleanly at the master's phase", song.playing && trk[0].seq_idx == 1u);
    /* Original base values remain after MIDI Stop even while motion sounds. */
    trk[0].p[P_REV] = 23; motion_clear(&trk[0]); motion_set_event(&trk[0], 0, P_REV, 100);
    clock_packet(source, 0xFA, 1000); clock_packet(source, 0xF8, 1000);
    bad += check("external-clock playback applies recorded motion", trk[0].p[P_REV] == 100);
    clock_packet(source, 0xFC, 1010);
    bad += check("external Stop restores the musical parameter base", trk[0].p[P_REV] == 23);
    return bad;
}
static int clock_arp_and_boundaries(void)
{
    int bad = 0; clock_setup(1); trk[0].p[P_AMODE] = 1; trk[0].p[P_ARATE] = 2;
    queued(0x90, 60, 100, 1); queued(0x90, 64, 100, 1);
    clock_packet(1, 0xFA, 10); clock_to(1, 10, 1); uint32_t before = trk[0].arp_idx;
    clock_to(2, 20, 7);
    bad += check("wrong-source clocks cannot advance the ARP", trk[0].arp_idx == before);
    clock_to(1, 10, 7);
    bad += check("selected-source MIDI clock advances the ARP", trk[0].arp_idx > before);
    clock_setup(2); fm1_ms = 100; um_byte(0x90); um_byte(60); um_byte(0xFA); um_byte(100); events_block(CTL);
    bad += check("TRS realtime interleaving preserves the incomplete note message", gate_note(&trk[0], 60) && song.playing);
    /* Millisecond counter wraps naturally during external-clock operation. */
    clock_setup(1); clock_packet(1, 0xFA, 0xFFFFFFF0u); clock_packet(1, 0xF8, 0xFFFFFFF0u);
    clock_packet(1, 0xF8, 5u); fm1_ms = 10; events_block(CTL);
    bad += check("external clock timestamps survive uint32 millisecond wrap", song.playing && midi_clock.last_ms == 5u);
    clock_setup(1); transport_req = 1; events_block(CTL); fm1_ms = 501; events_block(CTL);
    bad += check("local PLAY waiting for missing external clock also times out", !song.playing);
    return bad;
}
/* ARP HOLD latched on an external clock: a Stop, a lost clock or a tap while stopped must not leave the arp note
 * sounding; Continue plays the latched chord again; the internal clock's free-running arp is unchanged by STOP */
static void arp_latch(uint32_t mode)
{
    midi_test_reset(); song.g[G_CLOCK] = (int16_t)mode; events_block(CTL);
    trk[0].p[P_AMODE] = 1; trk[0].p[P_AHOLD] = 1; trk[0].p[P_ARATE] = 2; trk[0].p[P_AGATE] = 120; trk[0].p[P_AOCT] = 2;
    events_block(CTL);
}
static uint32_t arp_ext_run(uint32_t ms)       /* Start, tap 60 (latched), clocks until the arp plays 72 */
{
    uint32_t i;
    clock_packet(1, 0xFA, ms); queued(0x90, 60, 100, 1); queued(0x80, 60, 0, 1);
    for (i = 0; i < 400 && trk[0].arp_note != 72; i++) clock_packet(1, 0xF8, ms + i * 21u);
    return ms + i * 21u;
}
static void idle_ms(uint32_t ms)               /* about ms of audio blocks, no MIDI */
{
    uint32_t i, ms0 = fm1_ms;
    for (i = 0; i < ms * 23u / 16u; i++) { fm1_ms = ms0 + i * 16u / 23u; events_block(CTL); }
}
static int arp_ext_stop_test(void)
{
    int bad = 0; uint32_t ms, i, off;
    arp_latch(1); ms = arp_ext_run(10);
    bad += check("ARP HOLD on the external clock plays the upper octave", trk[0].arp_note == 72 && gate_note(&trk[0], 72));
    clock_packet(1, 0xFC, ms + 5); idle_ms(2000);
    bad += check("external Stop ends the sounding ARP note", !trk[0].arp_note && !gate_note(&trk[0], 72) && !gate_note(&trk[0], 60));
    bad += check("external Stop keeps the latched HOLD chord", trk[0].nheld == 1u && trk[0].held[0] == 60u);
    ms = fm1_ms; clock_packet(1, 0xFB, ms);
    for (i = 1; i < 40 && !trk[0].arp_note; i++) clock_packet(1, 0xF8, ms + i * 21u);
    bad += check("Continue plays the latched chord again", trk[0].arp_note != 0);
    arp_latch(1); arp_ext_run(10); idle_ms(2000);
    bad += check("a lost external clock stops the transport and ends the ARP note", !song.playing && !trk[0].arp_note && !gate_note(&trk[0], 72));
    arp_latch(1); queued(0x90, 64, 100, 1); queued(0x80, 64, 0, 1);
    bad += check("a tap while the external transport is stopped sounds", trk[0].arp_note == 64);
    idle_ms(2000);
    bad += check("its ARP gate still ends in real time", !trk[0].arp_note && !gate_note(&trk[0], 64));
    arp_latch(0); transport_req = 1; events_block(CTL); queued(0x90, 60, 100, 1); queued(0x80, 60, 0, 1);
    for (i = 0; i < 4000 && trk[0].arp_note != 72; i++) events_block(CTL);
    off = trk[0].arp_off;
    transport_req = 2; events_block(CTL);
    bad += check("internal clock: STOP leaves the free-running ARP note to its gate", trk[0].arp_note == 72 && gate_note(&trk[0], 72) && trk[0].arp_off == off - CTL);
    for (i = 0; i < 4000 && (!trk[0].arp_note || trk[0].arp_note == 72); i++) events_block(CTL);
    bad += check("internal clock: the ARP keeps stepping after STOP", trk[0].arp_note && trk[0].arp_note != 72 && trk[0].nheld == 1u);
    return bad;
}
/* USB-MIDI back-pressure: full 64-byte bulk packets (16 events) faster than the audio ISR drains the ring are
 * left in the endpoint (NAK) instead of overflowing it, which would panic every sounding note */
static int usb_burst_test(void)
{
    int bad = 0; uint8_t pkt[64]; uint32_t e, k, taken = 0;
    midi_test_reset();
    queued(0x90, 48, 100, 1);
    for (e = 0; e < 16; e++) {                       /* CC7 = 100 on 16 channels */
        pkt[4 * e] = 0x0B; pkt[4 * e + 1] = (uint8_t)(0xB0 | e); pkt[4 * e + 2] = 7; pkt[4 * e + 3] = 100;
    }
    for (k = 0; k < 5; k++)                          /* 5 polls at 2 kHz before the next half drains */
        taken += (uint32_t)ep1_take(pkt, sizeof pkt);
    bad += check("a USB burst fills the MIDI ring without overflowing it", taken == 3u && !midi_in_overflow && mi_w - mi_r == 48u);
    bad += check("the packet that does not fit is refused (left for the next poll)", !ep1_take(pkt, sizeof pkt) && mi_w - mi_r == 48u);
    for (e = 0; e < 8; e++)                          /* TRS MIDI meanwhile: its 8 slots are free */
        midi_in_event(0x0Bu | 0xB1u << 8 | 1u << 16 | e << 24);
    bad += check("TRS MIDI into a ring full of USB still fits (no overflow)", !midi_in_overflow && mi_w - mi_r == 56u);
    events_block(CTL);
    bad += check("the ring drains with no panic: the held note keeps sounding", gate_note(&trk[0], 48) && mi_r == mi_w);
    bad += check("the refused packet is taken once the ring has room", ep1_take(pkt, sizeof pkt) && mi_w - mi_r == 16u);
    events_block(CTL);
    bad += check("the held note survives all 88 events", gate_note(&trk[0], 48) && !midi_in_overflow);
    for (k = 0; k < 3; k++) ep1_take(pkt, sizeof pkt);
    midi_in_event(0x09u | 0x90u << 8 | 50u << 16 | 100u << 24);   /* one more event: 49 used, 15 free */
    bad += check("free space below one packet refuses a whole packet", !ep1_take(pkt, sizeof pkt) && !midi_in_overflow);
    events_block(CTL);
    return bad;
}
/* GLO > SYSTEM ROUT (#68): CH1-4 listens to channels 1..4 only, channels 5..16 are free for other instruments
 * (notes, bend, CCs, aftertouch, panic and reset all ignored, from USB and TRS); SEL plays the selected track
 * from every channel; a switch to CH1-4 lets go of what channels 5..16 held */
static int any_gate(void)
{
    for (uint32_t i = 0; i < NTRK * NVOICE; i++) if (trk[i / NVOICE].v[i % NVOICE].active && trk[i / NVOICE].v[i % NVOICE].gate) return 1;
    return 0;
}
static int route_test(void)
{
    int bad = 0; uint32_t ch, src, owned = 0;
    midi_test_reset(); song.sel = 2;
    bad += check("ROUT defaults to CH1-6", !song.g[G_MRSEL]);
    for (src = 1; src <= 2u; src++)
        for (ch = 6; ch < 16u; ch++) queued(0x90 | ch, 60 + ch, 100, src);
    for (ch = 6; ch < 16u; ch++) owned |= mn_get(ch, 60 + ch);
    bad += check("CH1-6: note-ons on channels 7..16 (USB and TRS) play nothing", !any_gate() && !owned && !midi_owners[2] && !midi_hint);
    queued(0x91, 62, 100, 1); queued(0x93, 40, 100, 2);
    bad += check("CH1-6: channels 2 and 4 still play parts 2 and 4", gate_note(&trk[1], 62) && mn_get(1, 62) == 2u && mn_get(3, 40) == 4u);
    queued(0xE6, 127, 127, 1); queued(0xEF, 0, 0, 2); queued(0xE9, 127, 127, 1);
    bad += check("CH1-6: pitch bend on channels 7, 10, 16 bends no part", !midi_bend_target[0] && !midi_bend_target[1] && !midi_bend_target[2] && !midi_ch[6].bend && !midi_ch[15].bend);
    queued(0xB6, 1, 99, 1); queued(0xB9, 11, 10, 2); queued(0xDF, 77, 0, 1);
    bad += check("CH1-6: CC1 / CC11 and aftertouch on channels 7..16 reach no MOD source", !trk[2].mw && !trk[2].ex_off && !trk[2].at && !trk[0].mw && !trk[0].at);
    queued(0xB6, 101, 0, 1); queued(0xB6, 100, 0, 1); queued(0xB6, 6, 24, 1);
    bad += check("CH1-6: RPN on channel 7 changes nothing", midi_ch[6].semis != 24u);
    queued(0xB6, 64, 127, 1); queued(0x81, 62, 0, 1);
    bad += check("CH1-6: a channel 7 sustain pedal does not hold channel 2's note", !gate_note(&trk[1], 62) && !mn_get(1, 62));
    queued(0x91, 62, 100, 1); queued(0xB1, 1, 50, 1);
    queued(0xB6, 120, 0, 1); queued(0xB9, 123, 0, 2); queued(0xBF, 121, 0, 1);
    bad += check("CH1-6: CC120 / CC123 / CC121 on channels 7..16 leave parts 1..4 sounding", gate_note(&trk[1], 62) && gate_note(&trk[3], 40) && midi_owners[1] == 1u && trk[1].mw == 50);
    queued(0xB1, 123, 0, 1); queued(0x83, 40, 0, 2);
    bad += check("CH1-6: panic on channel 2 still works", !gate_note(&trk[1], 62) && !midi_owners[1] && !midi_owners[3]);

    midi_test_reset(); song.sel = 2; host_route_sel(1); trk[2].p[P_VOICE] = V_POLY; trk[2].p[P_SUS] = 127; events_block(CTL);
    queued(0x90, 60, 100, 1); queued(0x96, 62, 100, 2); queued(0x99, 64, 100, 1); queued(0x9F, 65, 100, 2);
    bad += check("SEL: channels 1, 7, 10, 16 all play the selected track", gate_note(&trk[2], 60) && gate_note(&trk[2], 62) && gate_note(&trk[2], 64) && gate_note(&trk[2], 65) && !gate_note(&trk[0], 60) && midi_owners[2] == 4u);
    queued(0xEF, 127, 127, 1); queued(0xB9, 1, 66, 2); queued(0xD6, 33, 0, 1);
    bad += check("SEL: bend, CC1 and aftertouch from channels 7..16 reach the selected track", midi_bend_target[2] == 512 && trk[2].mw == 66 && trk[2].at == 33);
    queued(0xB6, 64, 127, 1); queued(0x86, 62, 0, 2);
    bad += check("SEL: channel 7 pedal holds its note", gate_note(&trk[2], 62) && (mn_get(6, 62) & MIDI_PEDAL_NOTE));
    host_route_sel(0); events_block(CTL);
    bad += check("SEL -> CH1-6 releases channels 7..16's notes (pedal-held too), keeps channel 1's",
                 !gate_note(&trk[2], 62) && !gate_note(&trk[2], 64) && !gate_note(&trk[2], 65) && gate_note(&trk[2], 60) &&
                 !mn_get(6, 62) && !mn_get(9, 64) && !mn_get(15, 65) && midi_owners[2] == 1u && !midi_ch[6].pedal);
    bad += check(".. and resets their bend: each part follows its own channel 1..6", !midi_bend_target[2] && !midi_ch[15].bend);
    queued(0x80, 60, 0, 1);
    bad += check("channel 1's note-off still releases its note after the switch", !any_gate() && !midi_owners[2]);
    return bad;
}
/* #103: the standard CC map, on the track the channel plays, as a knob turn (and as AUTOMATION records one) */
static int cc_map_test(void)
{
    static const struct { uint8_t cc; uint8_t id; } COMMON[] = {
        {5, P_GLIDE}, {7, P_LEVEL}, {72, P_REL}, {73, P_ATK}, {75, P_DEC}, {91, P_REV}, {93, P_CHOR}, {94, P_DLY},
    };
    int bad = 0, ok = 1; uint32_t i, e; int16_t before[P_COUNT];
    midi_test_reset();
    bad += check("power-on parts: ANALOG, FM6, LOFI, DRUM", trk[0].eng_req == 0u && trk[1].eng_req == ENGI_FM6 &&
                 trk[2].eng_req == 3u && trk[3].eng_req == ENGI_DRUM);
    for (i = 0; i < NELEM(COMMON); i++) {
        queued(0xB0, COMMON[i].cc, 127, 1);
        ok &= trk[0].p[COMMON[i].id] == TP[COMMON[i].id].max;
        queued(0xB0, COMMON[i].cc, 0, 1);
        ok &= trk[0].p[COMMON[i].id] == TP[COMMON[i].id].min;
        queued(0xB0, COMMON[i].cc, 37, 1);
        ok &= trk[0].p[COMMON[i].id] == 37;
    }
    bad += check("CC5 7 72 73 75 91 93 94: GLIDE LEVEL REL ATK DEC REV CHO DLY, 0..127 over their range", ok);
    queued(0xB0, 10, 0, 1); ok = trk[0].p[P_PAN] == -64;
    queued(0xB0, 10, 127, 1); ok &= trk[0].p[P_PAN] == 63;
    queued(0xB0, 10, 64, 1); ok &= trk[0].p[P_PAN] == 0;
    bad += check("CC10 PAN: 0 left, 64 centre, 127 right", ok);
    queued(0xB0, 74, 127, 1); queued(0xB0, 71, 0, 1);
    bad += check("CC74 / CC71 on ANALOG: CUT / RES", trk[0].p[P_E4] == 127 && trk[0].p[P_E5] == 0);
    queued(0xB0, 16, 127, 1); queued(0xB0, 23, 0, 1); queued(0xB1, 17, 127, 1); queued(0xB0, 102, 100, 1); queued(0xB0, 105, 20, 1);
    bad += check("(0.6.96) CC16..23 the engine's E1..E8 on any engine (ANALOG E1 max, E8 min; FM6 E2 max), CC102 FCUT, CC105 SUS",
                 trk[0].p[P_E0] == param_desc_of(0, P_E0)->max && trk[0].p[P_E7] == param_desc_of(0, P_E7)->min && trk[1].p[P_E1] == param_desc_of(ENGI_FM6, P_E1)->max &&
                 trk[0].p[P_FCUT] == 100 && trk[0].p[P_SUS] == 20);
    queued(0xB1, 74, 64, 1); queued(0xB1, 74, 127, 1);
    bad += check("CC74 on FM6 (channel 2): MLVL, bipolar (127: +63)", trk[1].p[P_E2] == 63);
    memcpy(before, trk[1].p, sizeof before); queued(0xB1, 71, 99, 1);
    bad += check("CC71 on FM6 (no resonance): ignored", !memcmp(before, trk[1].p, sizeof before));
    queued(0xB3, 74, 10, 1);
    bad += check("CC74 on DRUM (channel 4): TONE", trk[3].p[P_E2] == 10);
    for (e = 0, ok = 1; e < NENGINES; e++) {   /* the table names a continuous EDIT value of every engine */
        uint32_t m = MIDI_CC_ENG[e], k;
        for (k = 0; k < 2u; k++) {
            uint32_t n = (m >> (k ? 0 : 4)) & 15u;
            const param_desc_t *d = n ? &ENGINES[e]->edit[n - 1u] : 0;
            ok &= n <= 8u && (!d || (d->fmt != F_ENUM && d->max - d->min >= 24));
        }
    }
    bad += check("every engine's CC74 / CC71 target is a continuous EDIT value (or none)", ok);
    trk[2].eng_req = trk[2].engine = 7u;               /* an engine of no CC74 / CC71 target (MIDI_CC_ENG 0): WHEEL's
                                                        * number (ACID FM 0.2: retired; no engine kept has none) */
    memcpy(before, trk[2].p, sizeof before); queued(0xB2, 74, 3, 1); queued(0xB2, 71, 3, 1);
    bad += check("CC74 / CC71 on an engine of no target (7, WHEEL's, retired): ignored", !memcmp(before, trk[2].p, sizeof before));
    memcpy(before, trk[0].p, sizeof before);
    queued(0xB0, 121, 0, 1);
    bad += check("CC121 resets no mapped parameter (RP-015)", !memcmp(before, trk[0].p, sizeof before));
    queued(0xB0, 30, 99, 1); queued(0xB0, 76, 99, 1); queued(0xB0, 95, 99, 1);   /* (0.6.96: 20 is E5 now) */
    bad += check("CCs outside the map change nothing", !memcmp(before, trk[0].p, sizeof before));
    memcpy(before, trk[2].p, sizeof before);
    queued(0xB6, 7, 5, 1); queued(0xB9, 74, 5, 2); queued(0xBF, 91, 5, 1);
    ok = !memcmp(before, trk[2].p, sizeof before) && trk[0].p[P_LEVEL] != 5 && trk[3].p[P_LEVEL] != 5;
    bad += check("ROUT CH1-6: mapped CCs on channels 7..16 reach no part", ok);
    host_route_sel(1); song.sel = 2; events_block(CTL);
    queued(0xB9, 7, 5, 2);
    bad += check("ROUT SEL: channel 10's CC7 sets the selected track's LEVEL", trk[2].p[P_LEVEL] == 5 && trk[0].p[P_LEVEL] != 5);
    host_route_sel(0); song.sel = 0; events_block(CTL);

    /* AUTOMATION: the selected track armed and playing records the CC as a knob move; else it is the new base */
    midi_test_reset();
    song.rec = 1; seq_start(); events_block(CTL);
    queued(0xB0, 74, 20, 1);
    bad += check("armed and playing: CC74 records a motion event of CUT on the selected track", motion.count == 1u &&
                 motion.event[0].param == P_E4 && motion.event[0].value == 20 && (motion.event[0].place >> 6) == 0u);
    queued(0xB1, 74, 20, 1);
    bad += check("  another track (channel 2, not selected): not recorded", motion.count == 1u && trk[1].p[P_E2] != 0);
    seq_stop(); events_block(CTL);
    bad += check("  stopped: the patch's own CUT comes back", trk[0].p[P_E4] == ANALOG_PRESETS[trk[0].preset].e[4]);
    song.rec = 0;
    queued(0xB0, 74, 77, 1);
    bad += check("not armed: the CC is the new value, no event", trk[0].p[P_E4] == 77 && motion.count == 1u);
    return bad;
}
/* (ACID FM 0.5, 3c-1 task 1) per-track MIDI: each track's IN, OUT and CHANNEL (G_MRIN1.., G_MROUT1.., G_MRCH1..) and
 * KEYBOARD PLAYS SELECTED (G_MRSEL); NOTES OUT / CLOCK OUT are midi_out_mode's (mo_lvl) */
static uint32_t mo_pk[256], mo_pn;
static void mo_take(void)                          /* what MIDI OUT sent since the last mo_take (as usb.c drains it) */
{
    mo_pn = 0;
    while (mo_r != mo_w) {
        uint32_t p = midi_out_q[mo_r++ % MQ];
        if (mo_pn < NELEM(mo_pk)) mo_pk[mo_pn++] = p;
    }
}
static uint32_t mo_count(uint32_t status)          /* packets of this status byte (0x90 | ch, 0xF8 ..) taken */
{
    uint32_t i, n = 0;
    for (i = 0; i < mo_pn; i++) n += ((mo_pk[i] >> 8) & 0xFFu) == status;
    return n;
}
static uint32_t mo_notes(void)                     /* note-ons and note-offs taken, any channel */
{
    uint32_t i, n = 0;
    for (i = 0; i < mo_pn; i++) n += ((mo_pk[i] >> 12) & 0xEu) == 0x8u;
    return n;
}
static int route_track_test(void)
{
    int bad = 0, ok = 1; uint32_t ch, i;
    midi_test_reset();
    for (ch = 0; ch < 16u; ch++) ok &= midi_in_mask(ch) == (ch < NTRK ? 1u << ch : 0u);
    for (i = 0; i < NTRK; i++) ok &= trk_midi_ch(i) == i && song.g[G_MRIN1 + i] == 1 && song.g[G_MROUT1 + i] == 1;
    bad += check("routing defaults: T1..T6 receive and send on channels 1..6, nothing else receives", ok && !song.g[G_MRSEL]);
    song.g[G_MRCH1 + 1] = 99; song.g[G_MRSEL] = 7; trk_midi_defaults();
    bad += check("trk_midi_defaults: channels 1..6, IN / OUT on, KEYBOARD PLAYS SELECTED off",
                 song.g[G_MRCH1 + 1] == 2 && !song.g[G_MRSEL] && midi_in_mask(1) == 2u);

    midi_test_reset(); song.g[G_MRCH1 + 1] = 10; events_block(CTL);   /* T2 on channel 10 */
    queued(0x99, 60, 100, 1);
    bad += check("T2 on channel 10: a channel 10 note plays T2 only", gate_note(&trk[1], 60) && !gate_note(&trk[0], 60) &&
                 !gate_note(&trk[2], 60) && midi_owners[1] == 1u && mn_get(9, 60) == 2u);
    queued(0x91, 62, 100, 1);
    bad += check("  a channel 2 note then plays nothing (IN by channel)", !mn_get(1, 62) && !gate_note(&trk[1], 62) && !midi_owners[0]);
    queued(0x89, 60, 0, 1);
    bad += check("  its note-off releases it", !gate_note(&trk[1], 60) && !midi_owners[1] && !mn_get(9, 60));
    song.g[G_MRCH1 + 2] = 10; events_block(CTL);       /* T3 on channel 10 too */
    queued(0x99, 64, 100, 1);
    bad += check("two tracks on one channel both play its note", gate_note(&trk[1], 64) && gate_note(&trk[2], 64) &&
                 midi_owners[1] == 1u && midi_owners[2] == 1u);
    queued(0xB9, 7, 5, 1);
    bad += check("  .. and both take its CCs", trk[1].p[P_LEVEL] == 5 && trk[2].p[P_LEVEL] == 5 && trk[0].p[P_LEVEL] != 5);
    queued(0x89, 64, 0, 1);
    bad += check("  one note-off releases it on both", !gate_note(&trk[1], 64) && !gate_note(&trk[2], 64) && !midi_owners[1] &&
                 !midi_owners[2] && !mn_get(9, 64));
    queued(0x99, 65, 100, 1); song.g[G_MRCH1 + 2] = 3; events_block(CTL);
    bad += check("a routing change lets go of the notes its channels held (none hangs)", !gate_note(&trk[1], 65) &&
                 !gate_note(&trk[2], 65) && !midi_owners[1] && !midi_owners[2] && !mn_get(9, 65));

    midi_test_reset(); song.g[G_MRIN1 + 2] = 0; events_block(CTL);   /* T3's IN off */
    queued(0x92, 60, 100, 1); queued(0xB2, 7, 9, 1);
    bad += check("IN off on T3: channel 3's notes and CCs are ignored", !gate_note(&trk[2], 60) && !mn_get(2, 60) &&
                 !midi_owners[2] && trk[2].p[P_LEVEL] != 9 && !midi_in_mask(2));

    midi_test_reset(); song.g[G_MRCH1] = 16; events_block(CTL);       /* T1 on channel 16 */
    queued(0x9F, 60, 100, 1);
    bad += check("a track on channel 16 plays channel 16's notes", gate_note(&trk[0], 60) && mn_get(15, 60) == 1u);
    queued(0x8F, 60, 0, 1);
    midi_song_req = 0; memset((void *)midi_pc_req, 0, sizeof midi_pc_req);
    queued(0xCF, 2, 0, 1);
    bad += check("Program Change on channel 16 still selects a section / project, never T1's sound",
                 midi_song_req == 3u && !midi_pc_req[0]);
    midi_song_req = 0;
    song.g[G_MRCH1 + 4] = 3; events_block(CTL);         /* T3 and T5 on channel 3 */
    queued(0xC2, 4, 0, 1);
    bad += check("Program Change on channel 3 loads a sound on every track receiving 3",
                 midi_pc_req[2] == 5u && midi_pc_req[4] == 5u && !midi_pc_req[0] && !midi_pc_req[1] && !midi_pc_req[3] && !midi_song_req);
    memset((void *)midi_pc_req, 0, sizeof midi_pc_req);

    midi_test_reset(); song.g[G_MRSEL] = 1; song.sel = 2; trk[2].p[P_VOICE] = V_POLY; trk[2].p[P_SUS] = 127;
    events_block(CTL);                                  /* KEYBOARD PLAYS SELECTED, T3 selected */
    queued(0x9B, 60, 100, 1);
    bad += check("KEYBOARD PLAYS SELECTED: a note on an unclaimed channel (12) plays the selected track",
                 gate_note(&trk[2], 60) && mn_get(11, 60) == 3u && midi_owners[2] == 1u && midi_in_mask(11) == 1u << 2);
    queued(0x90, 62, 100, 1);
    bad += check("  on a claimed channel (1) it plays the claiming track only", gate_note(&trk[0], 62) && !gate_note(&trk[2], 62) &&
                 midi_in_mask(0) == 1u && mn_get(0, 62) == 1u);
    song.sel = 1; queued(0x8B, 60, 0, 1);
    bad += check("  its note-off follows the note across a selection change", !gate_note(&trk[2], 60) && !mn_get(11, 60) &&
                 !midi_owners[2]);
    song.g[G_MRSEL] = 0; queued(0x9B, 60, 100, 1);
    bad += check("  off: an unclaimed channel plays nothing", !mn_get(11, 60) && !gate_note(&trk[1], 60) && !midi_in_mask(11));
    queued(0x80, 62, 0, 1);

    /* a sound change (preset, PROGRAM CHANGE) on tracks sharing a channel while its note is held: no note hangs after */
    midi_test_reset(); song.g[G_MRCH1 + 1] = 10; song.g[G_MRCH1 + 2] = 10; events_block(CTL);
    queued(0x99, 60, 100, 1);
    midi_silence_track(1); midi_silence_track(2);
    queued(0x99, 62, 100, 1); queued(0x89, 60, 0, 1); queued(0x89, 62, 0, 1);
    for (i = 0; i < 4000u; i++) events_block(CTL);
    bad += check("two tracks on one channel, both silenced while a note is held: the next note ends on both",
                 !gate_note(&trk[1], 62) && !gate_note(&trk[2], 62) && !mn_get(9, 62) && !mn_get(9, 60) && !midi_owners[1] &&
                 !midi_owners[2]);
    midi_test_reset(); song.g[G_MRCH1 + 1] = 10; song.g[G_MRCH1 + 2] = 10; trk[2].p[P_VOICE] = V_POLY; trk[2].p[P_SUS] = 127;
    events_block(CTL);
    queued(0x99, 60, 100, 1);
    midi_silence_track(1);
    ok = mn_get(9, 60) == 3u && gate_note(&trk[2], 60) && midi_owners[2] == 1u;
    queued(0x99, 62, 100, 1); queued(0x89, 60, 0, 1);
    ok &= !gate_note(&trk[2], 60) && gate_note(&trk[2], 62);
    queued(0x89, 62, 0, 1);
    for (i = 0; i < 4000u; i++) events_block(CTL);
    bad += check("  one silenced: the note stays the other's, and ends there with its note-off", ok && !gate_note(&trk[1], 62) &&
                 !gate_note(&trk[2], 62) && !midi_owners[1] && !midi_owners[2]);

    midi_test_reset(); for (i = 1; i < 4u; i++) song.g[G_MRCH1 + i] = 10;
    events_block(CTL);
    queued(0x99, 60, 100, 1); midi_silence_track(1);
    ok = !mn_get(9, 60) && !midi_owners[2] && !midi_owners[3];
    queued(0x99, 62, 100, 1); queued(0x89, 60, 0, 1); queued(0x89, 62, 0, 1);
    for (i = 0; i < 4000u; i++) events_block(CTL);
    for (i = 1; i < 4u; i++) ok &= !gate_note(&trk[i], 62) && !gate_note(&trk[i], 60) && !midi_owners[i];
    bad += check("  three on one channel, one silenced: the note ends on the other two; the next one ends on all", ok);
    song.g[G_MRCH1 + 3] = 4; events_block(CTL);

    /* KEYBOARD PLAYS SELECTED: a routing change keeps the bend of the selected track playing through it */
    midi_test_reset(); host_route_sel(1); song.sel = 2; events_block(CTL);
    queued(0xEB, 127, 127, 1);
    ok = midi_bend_target[2] == 512;
    song.g[G_MRIN1 + 4] = 1; song.g[G_MRCH1 + 4] = 7; events_block(CTL);   /* (T5 now takes channel 7) */
    bad += check("a routing change elsewhere keeps the bend of the selected track (KEYBOARD PLAYS SELECTED)",
                 ok && midi_bend_target[2] == 512);
    host_route_sel(0); events_block(CTL);

    /* MIDI OUT: a track's notes on its CHANNEL when its OUT and NOTES OUT are on */
    midi_test_reset(); usb.config = 1; mo_lvl = MO_ALL; events_block(CTL); mo_take();
    song.g[G_MRCH1 + 1] = 10; events_block(CTL); mo_take();
    seq_note_on(&trk[1], 60, 100); seq_note_off(&trk[1], 60); mo_take();
    bad += check("MIDI OUT: T2 on channel 10 sends its sequenced notes on channel 10", mo_count(0x99) == 1u && mo_count(0x89) == 1u &&
                 mo_notes() == 2u && trk_midi_ch(1) == 9u);
    song.sel = 3; fm1_in.notes = 1u; events_block(CTL); fm1_in.notes = 0; events_block(CTL); mo_take();
    seq_note_on(&trk[3], 38, 100); seq_note_off(&trk[3], 38); mo_take();
    ok = mo_count(0x93) == 1u && mo_count(0x83) == 1u;
    fm1_in.notes = 1u; events_block(CTL); fm1_in.notes = 0; events_block(CTL); mo_take();
    ok &= mo_count(0x93) == 1u && mo_count(0x83) == 1u;
    bad += check("  T4 OUT on: its played and sequenced notes are sent on channel 4", ok && trk_midi_out(3));
    song.g[G_MROUT1 + 3] = 0; events_block(CTL); mo_take();
    fm1_in.notes = 1u; events_block(CTL); fm1_in.notes = 0; events_block(CTL);
    seq_note_on(&trk[3], 38, 100); seq_note_off(&trk[3], 38); mo_take();
    bad += check("  T4 OUT off: its played and sequenced notes send nothing", !mo_notes() && !trk_midi_out(3) && trk_midi_out(2));
    song.g[G_MROUT1 + 3] = 1; mo_lvl = MO_OFF; events_block(CTL); mo_take();
    for (i = 0, ok = 1; i < NTRK; i++) ok &= !trk_midi_out(i);
    seq_note_on(&trk[1], 60, 100); seq_note_off(&trk[1], 60);
    fm1_in.notes = 1u; events_block(CTL); fm1_in.notes = 0; events_block(CTL); mo_take();
    bad += check("  NOTES OUT off: no track sends", ok && !mo_notes());
    mo_lvl = MO_ALL; events_block(CTL); seq_note_on(&trk[1], 61, 100); song.g[G_MRCH1 + 1] = 11; events_block(CTL); mo_take();
    bad += check("  a CHANNEL change ends the notes sent on the old channel", mo_count(0x99) == 1u && mo_count(0x89) == 1u);
    seq_note_off(&trk[1], 61); mo_take();
    bad += check("  .. and its later note-off sends nothing more", !mo_notes());
    song.g[G_MRCH1 + 1] = 2; song.g[G_MRCH1] = 1; song.sel = 0; mo_lvl = MO_KEYS; events_block(CTL); mo_take();
    fm1_in.notes = 1u; events_block(CTL); mo_take(); ok = mo_count(0x90) == 1u;
    song.g[G_MRCH1] = 2; events_block(CTL); fm1_in.notes = 0; events_block(CTL); mo_take();
    bad += check("a key held while its track's CHANNEL changes 1 -> 2: its note-off on channel 1", ok && mo_count(0x80) == 1u &&
                 !mo_count(0x81));
    song.g[G_MRCH1] = 1; events_block(CTL);
    fm1_in.notes = 1u; events_block(CTL); mo_take(); ok = mo_count(0x90) == 1u;
    song.g[G_MROUT1] = 0; events_block(CTL); fm1_in.notes = 0; events_block(CTL); mo_take();
    bad += check("  .. while its OUT goes off: its note-off still sent", ok && mo_count(0x80) == 1u);
    song.g[G_MROUT1] = 1; events_block(CTL);
    fm1_in.notes = 1u; song.g[G_MROUT1] = 0; events_block(CTL); fm1_in.notes = 0; events_block(CTL); mo_take();
    bad += check("  a key struck with OUT off sends nothing, nor its note-off", !mo_notes());
    song.g[G_MROUT1] = 1; events_block(CTL);
    mo_lvl = MO_CLOCK; song.g[G_CLOCK] = 0; events_block(CTL); mo_take();
    for (i = 0; i < 400u; i++) events_block(CTL);
    mo_take(); ok = mo_count(0xF8) > 4u;
    song.g[G_CLOCK] = 1; events_block(CTL); mo_take();
    for (i = 0; i < 400u; i++) events_block(CTL);
    mo_take();
    bad += check("CLOCK OUT sends the clock (24 a beat) only with CLOCK INT", ok && !mo_count(0xF8));
    song.g[G_CLOCK] = 0; mo_lvl = MO_KEYS; usb.config = 0; events_block(CTL);
    return bad;
}
/* the routing is saved with the project, appended ('M' 'R' after the blocks before): a project of before (no block)
 * loads to the defaults, its ROUTE SEL as KEYBOARD PLAYS SELECTED; NEW PROJECT keeps the routing */
static void route_odd(void)                        /* a routing not the defaults */
{
    song.g[G_MRCH1 + 1] = 10; song.g[G_MRIN1 + 2] = 0; song.g[G_MROUT1 + 3] = 0; song.g[G_MRCH1 + 5] = 16;
    song.g[G_MRCH1 + 4] = 1; song.g[G_MRSEL] = 1;
}
static int route_is_odd(void)
{
    return song.g[G_MRCH1 + 1] == 10 && !song.g[G_MRIN1 + 2] && !song.g[G_MROUT1 + 3] && song.g[G_MRCH1 + 5] == 16 &&
           song.g[G_MRCH1 + 4] == 1 && song.g[G_MRSEL] == 1 && song.g[G_MRIN1] == 1 && song.g[G_MROUT1 + 1] == 1 &&
           song.g[G_MRCH1] == 1 && song.g[G_MRCH1 + 2] == 3;
}
static int route_is_def(void)
{
    int ok = 1; uint32_t i;
    for (i = 0; i < NTRK; i++)
        ok &= song.g[G_MRIN1 + i] == 1 && song.g[G_MROUT1 + i] == 1 && song.g[G_MRCH1 + i] == (int16_t)(i + 1u);
    return ok;
}
static void store_rehash(project_store_t *s)
{
    uint32_t sum = proj_hash(s->raw, PROJ_STORE_SIZE - 4u);
    memcpy(s->raw + PROJ_STORE_SIZE - 4u, &sum, 4);
}
static int store_load(const project_store_t *s)
{
    return proj_import(&proj_scratch, s, sizeof *s) && !project_restore_runtime(&proj_scratch);
}
static int route_store_test(void)
{
    static project_store_t s0, s1, ssel, s2, s3;
    static int16_t g0[G_COUNT];
    int bad = 0, ok; uint32_t i, d;
    int16_t one = 1;
    midi_test_reset();
    project_capture(&proj_scratch); ok = proj_pack(&s0, &proj_scratch) && store_load(&s0);   /* (once through a load: the
                                                         * power-on FM6 patches' names then as a load leaves them) */
    project_capture(&proj_scratch); memcpy(g0, proj_scratch.g, sizeof g0);
    ok &= proj_pack(&s0, &proj_scratch);                /* the routing at its defaults: as 0.4 wrote it */
    memcpy(&ssel, &s0, sizeof ssel);                    /* .. and with ROUTE SEL in its header */
    memcpy(ssel.raw + 8u + 2u * G_ROUTE, &one, 2); store_rehash(&ssel);
    route_odd(); events_block(CTL);
    ok &= store_load(&s0);
    bad += check("a project of before (no 'M' 'R', ROUTE CH1-6) loads to the defaults", ok && route_is_def() && !song.g[G_MRSEL]);
    for (i = 0, ok = 1; i < G_COUNT; i++) ok &= i == G_SLOT || i == G_LOAD || i == G_SAVE || song.g[i] == g0[i];
    project_capture(&proj_scratch);
    ok &= proj_pack(&s1, &proj_scratch) && !memcmp(&s0, &s1, sizeof s0);
    bad += check("  .. every other global as stored, and it saves again byte for byte the same", ok);
    route_odd(); song.g[G_MRSEL] = 0; events_block(CTL);
    ok = store_load(&ssel);
    for (i = 0; i < NTRK; i++)
        ok &= !song.g[G_MRIN1 + i] && song.g[G_MROUT1 + i] == 1 && song.g[G_MRCH1 + i] == (int16_t)(i + 1u);
    bad += check("a project of before with ROUTE SEL loads as before: no track's IN, KEYBOARD PLAYS SELECTED on",
                 ok && song.g[G_MRSEL] == 1 && midi_in_mask(0) == 1u << song.sel && midi_in_mask(9) == 1u << song.sel);
    project_capture(&proj_scratch); ok = proj_pack(&s1, &proj_scratch);
    for (d = 0; d < PROJ_STORE_SIZE && s1.raw[d] == ssel.raw[d]; d++)
        ;
    ok &= d < PROJC_PFX_OFF && s1.raw[d] == 'M' && s1.raw[d + 1u] == 'R' && s1.raw[d + 2u + NTRK] == 1u &&
          !memcmp(s1.raw + d + 9u, ssel.raw + d, PROJC_PFX_OFF - d - 9u) &&
          !memcmp(s1.raw + PROJC_PFX_OFF, ssel.raw + PROJC_PFX_OFF, PROJ_STORE_SIZE - 4u - PROJC_PFX_OFF);
    bad += check("  .. and gains an 'M' 'R' block on save (the rest as it was)", ok);
    trk_midi_defaults(); song.g[G_MRSEL] = 1; events_block(CTL);
    project_capture(&proj_scratch); ok = proj_pack(&s1, &proj_scratch);
    song.g[G_MRSEL] = 0; events_block(CTL);
    ok &= store_load(&s1) && route_is_def() && song.g[G_MRSEL] == 1;
    bad += check("a 0.5 project of default tracks with KEYBOARD PLAYS SELECTED reloads as saved (its 'M' 'R' written)", ok);

    trk_midi_defaults(); route_odd(); events_block(CTL);
    project_capture(&proj_scratch); ok = proj_pack(&s2, &proj_scratch);
    trk_midi_defaults(); events_block(CTL);
    ok &= store_load(&s2) && route_is_odd();
    project_capture(&proj_scratch);
    ok &= proj_pack(&s3, &proj_scratch) && !memcmp(&s2, &s3, sizeof s2);
    bad += check("a project with its routing changed round-trips (and saves again byte for byte the same)", ok);
    for (d = 0; d < PROJ_STORE_SIZE && s2.raw[d] == ssel.raw[d]; d++)
        ;
    ok = d < PROJC_PFX_OFF && s2.raw[d] == 'M' && s2.raw[d + 1u] == 'R' &&
         !memcmp(s2.raw + d + 9u, ssel.raw + d, PROJC_PFX_OFF - d - 9u) &&
         !memcmp(s2.raw + PROJC_PFX_OFF, ssel.raw + PROJC_PFX_OFF, PROJ_STORE_SIZE - 4u - PROJC_PFX_OFF);
    bad += check("  its store is the one of before with 'M' 'R' (9 bytes) appended after the last block", ok);
    route_odd(); song.g[G_BPM] = 99; events_block(CTL);
    system_init(); events_block(CTL);
    bad += check("NEW PROJECT keeps the MIDI routing", route_is_odd() && song.g[G_BPM] == GP[G_BPM].def);
    trk_midi_defaults(); events_block(CTL);
    return bad;
}
/* (ACID FM 0.5, device) a hardware sequencer's clock over TRS at a steady 120 wavers a few ms a pulse: the tempo followed
 * stays 120 (it read 117..123, a new tempo every quarter beat); a real change to 140 is followed within a few beats */
static uint32_t jit_seed;
static int32_t jitter_ms(void)
{
    jit_seed = jit_seed * 1103515245u + 12345u;
    return (int32_t)((jit_seed >> 16) % 7u) - 3;       /* -3 .. +3 ms */
}
static int clock_jitter_test(uint32_t source)
{
    int bad = 0, steady = 1, followed = 0;
    uint32_t i, changes = 0, beat = 0, t140 = 0, n140 = 0;
    clock_setup(source); jit_seed = 7;
    clock_packet(source, 0xFA, 1000);
    for (i = 0; i < 24u * 16u; i++) {                  /* 16 beats at 120: 500 ms a beat, 125/6 ms a clock */
        clock_packet(source, 0xF8, (uint32_t)(1000 + (int32_t)((i * 125u) / 6u) + jitter_ms()));
        if (i >= 24u * 3u) {                           /* settled after three beats */
            if (song.g[G_BPM] != 120) steady = 0;
            if (midi_beat_samples != beat) changes++;
        }
        beat = midi_beat_samples;
    }
    bad += check(source == 1 ? "USB clock jittered +-3 ms at 120 reads a steady 120" :
                               "TRS clock jittered +-3 ms at 120 reads a steady 120", steady);
    bad += check("a jittered steady clock does not keep re-tempoing the sequencer", changes <= 2u);
    t140 = 1000 + (24u * 16u * 125u) / 6u;
    for (i = 0; i < 24u * 8u; i++) {                   /* then 140: 60000 / 140 / 24 ms a clock */
        clock_packet(source, 0xF8, (uint32_t)((int32_t)(t140 + (i * 3000u) / 168u) + jitter_ms()));
        if (song.g[G_BPM] == 140 && !followed) { followed = 1; n140 = i; }
    }
    bad += check("a real tempo change to 140 is followed within four beats", followed && n140 <= 24u * 4u);
    bad += check("and holds 140", song.g[G_BPM] == 140);
    return bad;
}
int main(void)
{
    int bad = controls_test() + sustain_test() + ownership_test() + clock_test(1) + clock_test(2) + clock_arp_and_boundaries() +
              arp_ext_stop_test() + usb_burst_test() + route_test() + cc_map_test() + route_track_test() + route_store_test() +
              clock_jitter_test(1) + clock_jitter_test(2);
    printf("%s\n", bad ? "MIDI CONTROL/CLOCK TEST FAILED" : "MIDI control/clock integration tests passed"); return bad != 0;
}
