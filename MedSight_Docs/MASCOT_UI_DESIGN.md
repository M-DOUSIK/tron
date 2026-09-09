# MASCOT_UI_DESIGN.md — MedSight

## 1. Why a Mascot

Elderly users respond better to a warm, friendly on-screen presence than a clinical
status readout. An animated mascot gives clear emotional feedback (success / warning /
waiting) at a glance, which matters for a medical-adherence device where the user may
have reduced vision or attention.

## 2. Hard Rule: Original Character Only

**No Pokémon, no Nintendo IP, no existing copyrighted/trademarked character in any
form** — not the design, not a "reskin," not a similarly-shaped silhouette meant to
evoke a specific existing character, not in code comments, filenames, or documentation.
This applies to every session from 04 onward, including the touch-driven states added
in Session 05. If you or Antigravity is ever tempted to reference "Pokémon-style" or
"anime-style" as a design shortcut, stop — describe the actual visual qualities you want
instead (see below), not the IP you're borrowing them from.

**Note on source materials:** earlier project-brainstorming documents describe the UI
as "Pokémon-inspired." That phrasing is superseded by this rule — it was a scope
decision made explicitly during planning (an actual franchise character is a real legal
exposure for a contest submission, not just an internal style preference), and it stays
superseded even if it resurfaces in future drafts of the project brief. If you see that
phrase again anywhere, treat it as stale, not as new instruction.

## 3. Character Brief (original)

A simple, friendly, rounded mascot — think "a small pill-capsule-shaped or heart-shaped
character with a face" — clearly medicine/health themed rather than generically
"anime." Suggested working name: pick something original and unrelated to any existing
franchise (e.g. a simple invented name of your choosing).

Visual qualities to aim for (describe these, don't cite a source IP):
- Rounded, soft silhouette — non-threatening, easy to render at low resolution.
- 2–3 flat colors, high contrast against the camera-preview background for visibility.
- Simple facial expression set: neutral/waiting, happy/success, concerned/warning.

## 4. Animation States (Session 04 built idle; Session 13 wires the rest)

> **Status, stated accurately (corrected Session 12). The mascot is
> idle-only, by decision.**
>
> For eight sessions this table said Session 11 would drive
> `MASCOT_SUCCESS`/`MASCOT_ERROR` from real system events. It never happened:
> `anime_ui_set_state()` is not called from anywhere in the codebase, and
> `anime_ui_state_active()`, `_success()` and `_error()` all render the
> identical idle animation. The mascot has had exactly one state since
> Session 04.
>
> **That is now the intended design, not a gap.** The alternative — building
> the three animations and wiring them — was considered in Session 12 and
> deliberately declined: the UI already signals success and failure clearly
> through full-screen state changes (the dispensing screen, the "I Took It"
> confirmation, the FACE NOT RECOGNISED retry screen, the alert screens), so a
> second, parallel channel saying the same thing adds animation work without
> adding information. The idle mascot's job is presence and warmth, and it does
> that job in one state.
>
> **The table below therefore describes the enum, not device behaviour.** The
> four `mascot_state_t` values still exist in the code; only `MASCOT_IDLE` is
> ever selected. Anyone reviving this should read §6's rendering constraints
> first — every frame is CPU-drawn into a framebuffer the NPU also uses.

Maps directly onto the `mascot_state_t` enum defined in `SOFTWARE_ARCHITECTURE.md` §5:

| `mascot_state_t` value | Trigger | Visual |
|---|---|---|
| `MASCOT_IDLE` | No active event; also the reset target after any button tap or completed cycle | Gentle idle bounce/breathing loop |
| `MASCOT_ACTIVE` | Face capture running; dispensing in progress | A simple "in progress" animation cycle |
| `MASCOT_SUCCESS` | Patient matched; registration saved; dose confirmed | Happy animation, green accent |
| `MASCOT_ERROR` | No face found; no gallery match; gallery full; SD card unavailable; dispenser jam or short dispense (Session 14); missed dose window (Session 15) | Concerned animation, amber/red accent |

Session 04 implemented `MASCOT_IDLE` and stubbed the other three. Nothing since
has driven them, and nothing is planned to — see the status note above. The
enum and the stubs stay in place because they cost nothing and document the
intent, but the shipped device shows one animation.

## 5. Touch/Button Layer (Session 05)

Two main-screen buttons: **"Register"** and **"Dispense Medicine"** — these are the
final labels from the start (Session 05 wired them to debug output only; Session 09
gave Register its real enrollment flow, Session 10 gave Dispense Medicine its real
dispense flow, and Session 14 makes that dispense physical). Buttons should be large and high-contrast, consistent with the
elderly-user-friendly goal — this isn't a phone-sized touch target, use the full
available button area on the 5" panel.

## 6. Rendering Constraints

Sprite frames stored in flash, alpha-blended over the live camera feed via DMA2D
(NeoChrom GPU), non-blocking relative to the camera DMA pipeline (established in Session
04, must not regress in later sessions) and relative to touch polling (established in
Session 05). Target 30+ FPS for the animation layer.
