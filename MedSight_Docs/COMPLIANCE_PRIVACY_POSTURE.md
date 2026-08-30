# COMPLIANCE_PRIVACY_POSTURE.md — MedSight

## 1. What This Document Is (and isn't)

This describes design principles MedSight follows because they're *inspired by* the
kinds of data-handling requirements a real medical-adjacent device would eventually need
to satisfy under Japanese law (the Act on the Protection of Personal Information, and
PMDA oversight of Software as a Medical Device). **This is not a regulatory submission,
and MedSight has not undergone and is not claiming any PMDA approval, SaMD
classification, or clinical validation.** Do not describe it that way in the contest
submission, README, or any public-facing material — describe it as "designed with
data-locality principles in mind," not "PMDA-compliant" or "approved."

## 2. Actual Technical Measures Taken

- **Zero cloud connectivity.** No Wi-Fi/network stack is initialized anywhere in the
  firmware, at any session. Ethernet hardware exists on the board but is never brought
  up. This includes the "notify the patient's phone" feature in the dispense flow —
  the prototype implements that as a local LCD+buzzer alert, not an actual phone push,
  specifically to preserve this zero-network stance (see `MASTER_PROJECT_PLAN.md` §7).
- **Local-only storage.** All logs, dispensing events, face-recognition data
  (enrollment images/embeddings), and — as of Session 09's registration flow — patient
  names and phone numbers, are written exclusively to the on-board microSD card via
  FATFS (`sd_logger.c`, Session 06, extended in Session 09). Nothing is transmitted
  off-device by design — there is no code path capable of transmitting it, since no
  network stack exists. Collecting a phone number but never transmitting it anywhere
  is a deliberate, slightly unusual choice worth stating plainly in any documentation
  or demo: the field exists for a future notification feature that isn't implemented
  yet, not because the device currently does anything with it.
- **Physical data control.** Because data lives only on a removable SD card, the user
  retains direct physical control over their own data (they can remove, inspect, or wipe
  the card at any time) rather than relying on a third party's data-handling practices.
  This includes the prototype's on-device delete-user-data option (Session 11) — while
  that option itself has no authentication in the prototype (a deliberate, explicitly
  documented simplification, see `MASTER_PROJECT_PLAN.md` §6), the underlying
  local-only storage model is what makes any deletion complete and verifiable in the
  first place.

## 3. Why This Matters for the Contest Submission

Framing this honestly (design principle, not certification) is actually a *stronger*
position for a student RTOS/AI contest entry than an inflated regulatory claim would be
— judges evaluating a technical submission will see through overclaiming quickly, and an
honest "this is architected the way a real medical device's data layer would need to
work" is both true and impressive on its own merits.

## 4. Biometric and Personal Data Handling Specifics

Registration (Session 09) collects three categories of personal data: a face
enrollment photo/embedding, a name, and a phone number. All three get the same
treatment:
- Stored only in the patient-profile format defined in Session 09, only on the SD
  card — never in a separate, less-protected location.
- Never logged to the UART debug channel (Session 02's debug log) — debug strings
  only, never image/biometric/name/phone payloads, to avoid accidentally exposing them
  via the USB virtual COM port during development. This rule is verified by an
  explicit grep-for-DEBUG_LOG-near-personal-data step in both Session 09's and Session
  08C's self-review.
- The phone number specifically exists only as a data field for a *future*
  notification feature — the prototype never transmits it anywhere, since no
  transmission path exists (see §2). Don't let a demo or writeup imply the phone
  number is currently used for anything; it's captured now so the data model doesn't
  need to change later if phone notifications become a real, scoped feature.
- Document in `AI_PIPELINE.md` exactly what's stored for face data (embeddings vs. raw
  images) once Session 08C implementation decisions are made; document the
  name/phone-number storage format in Session 09's own notes.
- The prototype's delete-user-data option (Session 11) must remove all three
  categories together for a given patient — a partial deletion (e.g. removing the
  name but leaving the face embedding) would defeat the point of offering deletion at
  all.
