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

## 4. Animation States (drives Session 04, made interactive in Session 05, wired to
## real events from Session 11 onward)

Maps directly onto the `mascot_state_t` enum defined in `SOFTWARE_ARCHITECTURE.md` §5:

| `mascot_state_t` value | Trigger | Visual |
|---|---|---|
| `MASCOT_IDLE` | No active event; also the reset target after any button tap or completed cycle | Gentle idle bounce/breathing loop |
| `MASCOT_ACTIVE` | User tapped a button (Session 05); registration in progress (Session 09), dispensing/verification in progress (Session 11) | A simple "in progress" animation cycle |
| `MASCOT_SUCCESS` | Correct patient matched + consumption confirmed (Session 11) | Happy animation, green accent, "Match!" text |
| `MASCOT_ERROR` | Wrong/unrecognized patient, missed dose, jam, or missed consumption confirmation (Session 11) | Concerned animation, amber/red accent |

Session 04 implements `MASCOT_IDLE` only, with the other three stubbed. Session 05
makes `MASCOT_ACTIVE` reachable via touch (button taps), still independent of any real
AI/dispenser logic. Session 11 is what finally drives `MASCOT_SUCCESS` and
`MASCOT_ERROR` from real system events rather than test buttons.

## 5. Touch/Button Layer (Session 05)

Two main-screen buttons: **"Register"** and **"Dispense Medicine"** — these are the
final labels from the start (Session 05 wires them to debug output only; Session 09
gives Register its real enrollment flow, Session 11 gives Dispense Medicine its real
dispense flow). Buttons should be large and high-contrast, consistent with the
elderly-user-friendly goal — this isn't a phone-sized touch target, use the full
available button area on the 5" panel.

## 6. Rendering Constraints

Sprite frames stored in flash, alpha-blended over the live camera feed via DMA2D
(NeoChrom GPU), non-blocking relative to the camera DMA pipeline (established in Session
04, must not regress in later sessions) and relative to touch polling (established in
Session 05). Target 30+ FPS for the animation layer.
