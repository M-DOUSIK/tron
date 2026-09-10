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

> **Accuracy note (Session 12, updated Session 15).** Earlier revisions of this
> document described two things the firmware did not contain: collection of a
> patient **phone number**, and an on-device **delete-user-data** option. Neither
> had been built, and both claims were removed rather than implemented — see §5.
> The phone number still does not exist and never will. The delete now does, built
> in Session 15 behind the carer passcode, which is the condition §5 said it would
> need. A privacy posture that describes controls which do not exist is worse than
> one that is merely modest, so this file continues to be written strictly against
> what the code does — including §6's account of the enrolment hole that was open
> for three sessions.

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
- **Minimal collection.** The patient record is exactly five fields: a validity flag,
  a name, a 128-byte face embedding, the number of pills in one dose, and (Session 15)
  up to four times of day at which that dose is taken. There is no
  phone number, no address, no date of birth, no medical history. This is not an
  accident of scope — it is the smallest set that makes the device work, and it is
  worth stating as a design position. The schedule was added because scheduled dosing
  cannot work without it, and it was capped at four times because that is what real
  prescriptions use — not because four happened to fit.
- **Authenticated enrolment (Session 15).** Registering a patient — the act that
  puts a face embedding on the device — now requires the carer passcode, asked
  before the camera is used at all. See §6's first bullet for what this fixed and
  why it was open for so long.
- **Embeddings, not photographs.** What is stored for face recognition is a
  128-dimensional quantised embedding, not the enrolment image. The camera frame used
  to produce it is never written to the card. An embedding is not reversible into a
  usable photograph.
- **Physical data control.** Because data lives only on a removable SD card, the user
  retains direct physical control over their own data — they can remove, inspect, or
  destroy the card at any time — rather than relying on a third party's data-handling
  practices. Removing or wiping the card remains the **most complete** deletion
  mechanism, because it takes the audit log with it; the on-device delete added in
  Session 15 (§5) removes one patient's name and embedding and leaves the log
  intact, which is the right behaviour for a delete but is not the same thing.

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
a name. (A dose size is also stored, and from Session 15 a set of dose times; both are
health-adjacent but not identifying on their own.) All of it gets the same treatment:

- Stored only in the patient-record format defined in `SOFTWARE_ARCHITECTURE.md` §6,
  only on the SD card, in `patients.dat` — never in a separate, less-protected
  location.
- **Never logged to the UART debug channel.** Debug strings only — never image or
  biometric payloads — so nothing sensitive can leak through the USB virtual COM port
  during development. This rule is verified by an explicit grep of every `printf` and
  `SD_Log_Event*` call site in each session's self-review, and from Session 12 onward
  against a **real captured UART log**, not just by code review.
- **What may be logged:** names, gallery slot indices, dose counts, scheduled dose
  times, match confidence scores, and — from Session 15 — a timestamp on every
  line, but **only once a carer has set the clock**. Before that the line is
  written unstamped rather than stamped with the RTC's power-on default, because
  a log line reading `2000-01-01` looks like data and is worse than one that
  admits it does not know. Worth stating plainly: a timestamped adherence record
  is materially more revealing about a person's daily routine than an untimed
  one. That is a real increase in the sensitivity of `events.log`, and it is
  noted here rather than treated as a formatting change. A confidence score is a single scalar derived
  from an embedding, not the embedding itself; Session 12 added one to make the match
  threshold tunable (see `milestones/session_12_notes.md` Addendum 3).
- **What is never logged, added in Session 15:** the carer passcode, in any form —
  not the digits, not the hash, not the length, and not on a failed attempt. A
  lockout is recorded (`SECURITY: carer passcode locked out`) because the *event* is
  auditable and the *secret* is not. `gallery_delete_patient()` deliberately does not
  print the name it just removed either: the point of a delete is that the record
  stops existing, and echoing it into the log at the moment of deletion works against
  that.
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

**On-device delete-user-data — dropped as a claim in Session 12, BUILT in
Session 15, behind authentication.** `MASTER_PROJECT_PLAN.md` §6 and this document
both once described an on-device delete option with **no** authentication, framed as
an acceptable bench-prototype simplification. No such function was ever written, and
rather than build an unauthenticated delete of biometric data — which this document
itself argued was inappropriate — the claim was removed in Session 12.

It exists now, and the difference is the passcode in front of it. Carer mode's
DELETE removes the name **and** the embedding together, in one `memset` of the whole
record, followed by a rewrite of `patients.dat`. A partial delete — clearing the name
and leaving the embedding — would defeat the entire point: a record with a live
embedding and no name still matches a face, it just matches it to nobody.

The delete is confirmed on a two-choice screen naming the patient, timing out to
KEEP rather than to DELETE, and the action is recorded in the event log by name.
Deletion by physical control of the card (§2) remains available and is still the
more complete option, because it removes the audit log as well.

## 6. Known Limitations, Stated Plainly

- **Enrolment was unauthenticated until Session 15. It is the most significant
  hole this build ever had, and it is now closed.** The old behaviour, stated
  plainly because a posture document that hides a fixed hole is worth less than
  one that shows the fix: anyone could tap REGISTER PATIENT, enrol their own face
  and their own dose size, and then be dispensed medication by the normal DISPENSE
  flow — and the audit log would record the result as a legitimate, face-matched
  dispense to a registered patient, because from the device's point of view it was
  one. Face recognition worked exactly as designed. The gallery it matched against
  accepted anyone who asked, and every other control in the device rests on that
  gallery being trustworthy. With enrolment open, identification is theatre.

  This was raised by the project's own user while closing out Session 12 — not
  caught by any prior review, **including this document's earlier passes, which
  were specifically looking for this class of problem.** The person who uses the
  device found it and the documents did not; that is worth remembering about
  documents, this one included.

  Session 15 §B2a closed it. REGISTER PATIENT prompts for the carer passcode
  before any face is captured, sharing one prompt screen and one validation
  routine with carer-mode entry. The gate is at the START of the flow, not the
  end, deliberately: asking afterwards would mean a face and a name had already
  been taken from someone who was never authorised to give them, which is
  precisely the thing this section would otherwise have to apologise for.

- **What the carer passcode actually protects against: a curious patient or a
  visitor. Not an attacker.** Be clear-eyed about it, because the feature is only
  worth what the honesty about it is worth:
  - The stored value is a salted 32-bit FNV-1a hash in `carer.cfg` on an
    unencrypted card. Anyone holding the card and knowing the algorithm can
    brute-force a four-digit code in seconds. It is a hash rather than plaintext
    for one narrow reason — a carer's chosen code is likely a code they use
    elsewhere, and writing it in the clear on a removable card would leak that
    reuse for no benefit at all.
  - There is no secure element on this board and nowhere to put a key.
  - Rate limiting (five attempts, then 30 seconds) lives in RAM, so a power cycle
    clears it. Deliberate: persisting it would turn a wrong tap into an SD write,
    and would give anyone a way to wear the card out, or to lock a device out
    permanently by pulling power at the right moment. Against this threat model
    the RAM counter is the better trade — and it is a trade, recorded rather
    than overlooked. The README lists persisting it as future work.
  - A build-time default passcode ships so a fresh device is usable at all, and
    the device says on every boot when it is still on that default.

- **No authentication on the patient-facing flow.** Anyone can tap Dispense; what
  gates a dose is face recognition, not a credential. That is intentional — the
  patient is the person the device exists to serve, and a code between them and
  their own medication would be the wrong trade.
- **Face matching is a similarity threshold, not an identity guarantee.** It can
  produce both false rejections and, in principle, false acceptances. The threshold is
  documented and measurable rather than assumed (`session_12_notes.md` Addendum 3).
- **The SD card is unencrypted.** Physical possession of the card gives access to the
  names, embeddings and event log on it. Encryption at rest is future work.
- **No tamper detection, no audit-log integrity protection.** The event log is
  append-only by convention, not by enforcement. Session 15 made it readable on
  the device, in carer mode, which raises the value of the log without changing
  this: a carer reading it on the panel is trusting the same unprotected file.
- **The clock is trusted, and anyone with the carer code can set it.** Every
  scheduled-dose and missed-dose record is stamped against the device's own RTC.
  A wrong clock produces a schedule that fires at the wrong time and an audit
  trail that says it was right — which is why the clock is shown on every carer
  screen, and why the home screen states plainly when it has never been set. On
  LSI (used only if the LSE crystal will not start) the drift is on the order of
  a percent, i.e. minutes per day: immaterial for a dose reminder, and not
  something to build anything time-critical on.
