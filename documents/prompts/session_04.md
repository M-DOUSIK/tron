# Session 04 — Mascot Animation Overlay

## Prompt for Antigravity (copy-paste as-is)

```
OBJECTIVE
Implement an original, friendly mascot animation layer, alpha-blended over the live
camera feed from Session 03.

BACKGROUND AND CONTEXT
Building on tron/session_03/. MedSight targets elderly users, so the UI needs a warm,
approachable visual presence rather than a bare status readout. See
MASCOT_UI_DESIGN.md for the full character brief.

CRITICAL CONSTRAINT — READ BEFORE STARTING
The mascot must be a wholly original character design. Do NOT reference, reproduce, or
stylistically imitate any existing copyrighted/trademarked character (including but not
limited to Pokémon or any other franchise character) in the artwork, sprite naming, code
comments, or documentation. Follow the visual brief in MASCOT_UI_DESIGN.md directly
(rounded silhouette, 2-3 flat colors, simple expression set) rather than describing it
by reference to any existing IP.

RELEVANT PROJECT FILES AND FOLDERS
- FSBL/Src/ui/ (new)
- MASCOT_UI_DESIGN.md (character brief and animation state table)
- tron/session_03/ as the base

REQUIRED INPUTS
- None beyond the design brief; if sprite art assets need to be generated, generate
  simple original geometric/flat-color sprites consistent with the brief.

EXPECTED OUTPUTS
- An idle-state animated mascot loop (per MASCOT_UI_DESIGN.md's "Idle/waiting" state)
  rendered via DMA2D alpha-blending over the live camera feed, at 30+ FPS, without
  blocking the camera DMA pipeline.

CONSTRAINTS
- Do not block or stall the camera DMA while rendering the animation — this is a hard
  performance constraint carried into every later session.
- Ensure the DMA2D blending approach accounts for the framebuffer residing in internal AXI SRAM (`0x34200000`).
- Implement only the Idle/waiting state fully this session; stub the other states
  (Success, Warning-wrong-patient, Warning-missed-dose, Dispensing) as empty functions
  to be filled in during Sessions 07-09.

CODING STANDARDS
- Sprite frame data stored in flash (not SRAM/PSRAM at rest).
- Keep animation logic isolated in anime_ui.c — do not mix into other components, integrate directly into the `FSBL/Src/main.c` orchestrator.

FOLDER STRUCTURE TO FOLLOW
- FSBL/Src/ui/anime_ui.c
- FSBL/Inc/ui/anime_ui.h

FILES TO CREATE
- FSBL/Src/ui/anime_ui.c
- FSBL/Inc/ui/anime_ui.h

FILES TO MODIFY
- FSBL/Src/main.c (call anime_ui_init/update in the main rendering loop)

DOCUMENTATION TO UPDATE
- docs/milestones/session_04_notes.md: confirm original-character compliance, describe
  sprite storage layout and DMA2D blending approach used.

VALIDATION AND TESTING REQUIREMENTS
- Build cleanly.
- (Manual) Idle animation visibly loops smoothly over the live camera feed at 30+ FPS,
  camera preview remains tear-free and responsive.

COMPLETION CHECKLIST
- [ ] Sprite arrays stored in flash
- [ ] DMA2D alpha-blending configured and rendering over the camera background
- [ ] Idle/waiting animation state fully implemented
- [ ] Other 4 states stubbed (function signatures present, empty bodies, TODO comments
      referencing the session that will implement them)
- [ ] session_04_notes.md written, explicitly confirming original-character design

COMMON PITFALLS
- Animation logic accidentally running on the same DMA channel/priority as the camera
  pipeline, causing preview stutter — verify with the live feed running.
- Reaching for "anime-style" as a shorthand and drifting toward an existing character's
  actual design — re-read the brief in MASCOT_UI_DESIGN.md if unsure.

DEFINITION OF DONE
Idle mascot animation loops smoothly over a tear-free live camera feed at 30+ FPS. All
five UI states have at least a stub function. Original-character compliance explicitly
confirmed in the session notes.

SELF-REVIEW BEFORE DECLARING COMPLETE
Look at the actual sprite design description/asset one more time and confirm it does
not resemble any existing copyrighted character.
```

## Expected Deliverables
`anime_ui.c/.h` with idle animation implemented and 4 stub states, session notes with
explicit IP-compliance confirmation.

## Manual Verification Steps
1. Flash and observe the LCD.
2. Confirm the mascot idle animation loops smoothly.
3. Confirm camera preview is still tear-free and responsive with the overlay active.
4. Visually sanity-check the mascot design yourself against the "no existing IP" rule.

## Acceptance Criteria
Smooth 30+ FPS animation, no camera pipeline regression, original design confirmed.

## Next Prompt
Copy to `tron/session_04/`, proceed to `session_05.md`.
