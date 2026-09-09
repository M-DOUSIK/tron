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

> **Accuracy note (Session 12).** Earlier revisions of this document described two
> things the firmware has never contained: collection of a patient **phone number**,
> and an on-device **delete-user-data** option. Neither was ever built. They were
> removed rather than implemented — see §5 for the decisions and the reasoning. A
> privacy posture that describes controls which do not exist is worse than one that
> is merely modest, so this file is now written strictly against what the code does.

## 2. Actual Technical Measures Taken

- **Zero cloud connectivity.** No Wi-Fi/network stack is initialized anywhere in the
  firmware, at any session. Ethernet hardware exists on the board but is never brought
  up. There is no code path capable of transmitting anything off-device, which is a
  stronger guarantee than a policy: it is not that the device chooses not to transmit,
  it is that it cannot. This holds even for the scheduled-dose alerts added in
  Session 15 — a due dose is announced on the device, to the person standing in
  front of it, and recorded locally. Nothing is pushed anywhere.
- **Local-only storage.** All logs, dispensing events and face-recognition data
  (enrollment embeddings) — plus, from Session 09's registration flow, the patient's
  name and dose size — are written exclusively to the on-board microSD card via FATFS
  (`sd_logger.c`, Session 06, extended in Sessions 08B/09/12).
- **Minimal collection.** The patient record is exactly four fields: a validity flag,
  a name, a 128-byte face embedding, and the number of pills in one dose. There is no
  phone number, no address, no date of birth, no medical history. This is not an
  accident of scope — it is the smallest set that makes the device work, and it is
  worth stating as a design position.
- **Embeddings, not photographs.** What is stored for face recognition is a
  128-dimensional quantised embedding, not the enrolment image. The camera frame used
  to produce it is never written to the card. An embedding is not reversible into a
  usable photograph.
- **Physical data control.** Because data lives only on a removable SD card, the user
  retains direct physical control over their own data — they can remove, inspect, or
  destroy the card at any time — rather than relying on a third party's data-handling
  practices. With no delete function in the firmware (§5), removing or wiping the card
  **is** the deletion mechanism, and it is a complete and verifiable one.

## 3. Why This Matters for the Contest Submission

Framing this honestly (design principle, not certification) is actually a *stronger*
position for a student RTOS/AI contest entry than an inflated regulatory claim would be
— judges evaluating a technical submission will see through overclaiming quickly, and an
honest "this is architected the way a real medical device's data layer would need to
work" is both true and impressive on its own merits.

The same reasoning is why Session 12 removed the two unbuilt claims in §5 rather than
leaving them in the document to look better. A posture that can be checked against the
source is worth more than one that cannot.

## 4. Biometric and Personal Data Handling Specifics

Registration collects two categories of personal data: a face enrolment embedding, and
a name. (A dose size is also stored; it is health-adjacent but not identifying on its
own.) Both get the same treatment:

- Stored only in the patient-record format defined in `SOFTWARE_ARCHITECTURE.md` §6,
  only on the SD card, in `patients.dat` — never in a separate, less-protected
  location.
- **Never logged to the UART debug channel.** Debug strings only — never image or
  biometric payloads — so nothing sensitive can leak through the USB virtual COM port
  during development. This rule is verified by an explicit grep of every `printf` and
  `SD_Log_Event*` call site in each session's self-review, and from Session 12 onward
  against a **real captured UART log**, not just by code review.
- **What may be logged:** names, gallery slot indices, dose counts, and match
  confidence scores. A confidence score is a single scalar derived from an embedding,
  not the embedding itself; Session 12 added one to make the match threshold tunable
  (see `milestones/session_12_notes.md` Addendum 3).
- Nothing else in the firmware reads `patients.dat`. `ai_vision.c` owns the gallery;
  every other module reaches it through that module's API.

## 5. Two Controls This Document Used to Claim, and Why They Were Removed

Recorded here rather than deleted silently, because a reader comparing an older
revision of this file needs to know these were deliberate decisions.

**Phone number collection — removed from the document; never existed in code.**
Early drafts of the data model included a phone number field for a future
notification feature, and this document discussed at length the unusual choice of
collecting a number that is never transmitted. The field was never implemented, and
with the zero-network stance being permanent there is no feature that would ever use
it. Collecting a piece of personal data "in case it is needed later" is the opposite
of data minimisation, so the right outcome is the one that happened by default: it
does not exist. `SOFTWARE_ARCHITECTURE.md` §6 and `AI_PIPELINE.md` §4 were corrected
earlier; this file was the last to still describe it.

**On-device delete-user-data option — claim dropped, feature not built.**
`MASTER_PROJECT_PLAN.md` §6 and this document both described an on-device delete
option with no authentication, framed as an acceptable bench-prototype
simplification. No such function was ever written. Rather than build an
unauthenticated delete of biometric data — which this document itself argued was
inappropriate — the claim was removed. Deletion remains available and complete
through physical control of the card (§2). If the project continues past the contest,
the right implementation is a delete behind real authentication — which is
exactly what Session 15's password-gated carer mode provides, and this section
should be revised when that lands.

## 6. Known Limitations, Stated Plainly

- **Enrolment is currently unauthenticated — the most significant hole in the
  build, and it is open as of Session 12.** Anyone can tap REGISTER PATIENT,
  enrol their own face and dose size, and then be dispensed medication by the
  normal DISPENSE flow. The audit log records the result as a legitimate,
  face-matched dispense to a registered patient, because from the device's
  point of view it is one. Face recognition works exactly as designed; the
  gallery it matches against is what accepts anyone who asks, and every other
  control rests on that gallery being trustworthy.

  This was raised by the project's own user while closing out Session 12 — not
  caught by any prior review, including this document's earlier passes, which
  were specifically looking for this class of problem. It is recorded here
  rather than quietly fixed later because a posture document that omits a known
  hole is worth less than one that names it.

  Session 15 §B2a closes it: REGISTER PATIENT prompts for the carer password
  before any face is captured, sharing one prompt and one validation routine
  with carer-mode entry. Revise this bullet when that lands.

- **No authentication on the patient-facing flow.** Anyone can tap Dispense; what
  gates a dose is face recognition, not a credential. The carer-only functions added
  in Session 15 are password-gated, but the password is a build-time constant on a
  device with no secure element — adequate for a prototype, not for deployment.
- **Face matching is a similarity threshold, not an identity guarantee.** It can
  produce both false rejections and, in principle, false acceptances. The threshold is
  documented and measurable rather than assumed (`session_12_notes.md` Addendum 3).
- **The SD card is unencrypted.** Physical possession of the card gives access to the
  names, embeddings and event log on it. Encryption at rest is future work.
- **No tamper detection, no audit-log integrity protection.** The event log is
  append-only by convention, not by enforcement.
