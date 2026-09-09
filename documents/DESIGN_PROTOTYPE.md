# DESIGN_PROTOTYPE.md — MedSight Physical Design

Written for Session 13's contest documentation packaging. This is the
physical-design story: what is actually built as of this checkout, versus
what is documented intent for a version with more time and hoppers. It
deliberately does not blur the two — `MECHANICAL_DESIGN.md`'s own banner
has already had to be corrected three times as the scope changed session to
session, and a document written to look impressive is worse than one a judge
can trust.

## What is real today

The electronics are real and hardware-verified: an STM32N6570-DK, its 5"
touch LCD, the IMX335 camera, and a microSD card, running the full
registration → face-recognition → dose-confirmation flow described in
`UI_SCREEN_INVENTORY.md`. As of Session 12 this whole flow works end-to-end
on a physical board, confirmed from a genuine cold boot — see
`milestones/session_12_notes.md` Addendum 10 for exactly what is and isn't
hardware-evidenced.

**Dispensing itself:** check whether `sessions/session_14/` exists in this
checkout.
- **If it does not exist**, "dispensing" in this build is the software
  simulation described in `SOFTWARE_ARCHITECTURE.md` §7 — a progress bar and
  a confirmation screen, no pills actually move. This is exactly what
  Sessions 09–13 built and tested; it is a deliberate, documented
  simplification, not an oversight.
- **If it does exist**, Session 14 has built one real hopper: a 28BYJ-48
  unipolar stepper turning a turntable, singulating loose pills past an IR
  break-beam sensor that counts each one as it drops — the motor stops the
  instant the prescribed count is verified, not assumed. This is a direct,
  single-hopper build of the mechanism `MECHANICAL_DESIGN.md` specifies; see
  that session's own notes for what was measured on the physical mechanism.

## What is documented design intent, not built

`MECHANICAL_DESIGN.md`'s **6–8 independently-addressable hopper**
architecture — one hopper per medication, each hopper a physical duplicate
of the same turntable/break-beam mechanism, all dropping into one shared
collection chute — is the target design for a patient on real
polypharmacy (the average nursing-home resident is on roughly seven
medications). It is not built in this checkout, and the document says so in
its own status banner. The reasoning for *why* this architecture (stacked
independent modules, not one big carousel) is in that document's §2 table;
the short version is that a jam or an empty hopper in one module doesn't
take down dispensing for every other medication a patient is on.

The single hopper Session 14 builds (if it has run) is deliberately the
*same* mechanism at one-hopper scale, specifically so that scaling up later
is "print and wire one more identical module," not a redesign.

## Enclosure — not physically built, one text prompt exists

No enclosure has been printed or fitted. What exists is
[`RAGNAR_CAD_PROMPT.md`](RAGNAR_CAD_PROMPT.md) — a written prompt for a
text-to-3D tool (Ragnar.AI), describing a chassis with an angled LCD bezel,
an articulating camera boom, and two side-by-side hopper module slots sized
to extend in fixed-width increments for future hoppers. It has not yet been
run through the tool, so **no rendered image or STEP file exists in this
repository** — the prompt document is the design artifact, not a picture.
Every dimension in it is explicitly flagged as a placeholder pending real
caliper measurements of the board, panel, and camera module.

If renders are generated before submission, they belong alongside this
document (e.g. `documents/renders/`) with a short caption naming which
prompt revision produced them; none exist as of this writing.

## Honest summary for a judge

| Component | Status |
|---|---|
| Face recognition, registration, dose-confirmation UI flow | **Built, hardware-verified** |
| Physical dispensing (single hopper) | **Built in Session 14, if that session has run in this checkout — otherwise software-simulated** |
| Multi-hopper (2–8 medications) architecture | **Design intent** — `MECHANICAL_DESIGN.md`, not built |
| Enclosure | **Not built** — one unrun CAD-generation prompt (`RAGNAR_CAD_PROMPT.md`) |
