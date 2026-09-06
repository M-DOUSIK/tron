import os
import glob

prompts_dir = "C:/Users/Dousik/Workspace/TRON/MedSight_Docs/prompts"

# Delete existing 08C and 09-14
for f in glob.glob(os.path.join(prompts_dir, "session_*.md")):
    base = os.path.basename(f)
    if base >= "session_08C.md":
        os.remove(f)

s09 = """# Session 09 - Dispenser Hardware Interface (Servo & IR)

## Prompt for Antigravity (copy-paste as-is)
```
OBJECTIVE
Implement the hardware abstraction layer for the pill dispenser using Servo motors (instead of steppers) and IR LED break-beam sensors.

BACKGROUND AND CONTEXT
The user has the physical electronics (Servo motors, IR LEDs). We need a `dispenser.c` that can drive the servos to singulate and dispense pills, and use the IR LEDs via EXTI interrupts to count the drops. Action recognition is deferred to the end of the project. For now, we just rely on counting the drops.

EXPECTED OUTPUTS
- `FSBL/Src/dispenser.c/.h` exposing `dispense_dose(uint8_t hopper_id, uint8_t count)`
- Uses Servos (via PWM) instead of stepper motors.
- Uses IR LED EXTI interrupts with proper debounce to count falling pills.
- All OSAL-safe. No direct FreeRTOS calls.
```
"""

s10 = """# Session 10 - Registration Completion & UI Integration

## Prompt for Antigravity (copy-paste as-is)
```
OBJECTIVE
Complete the UI registration flow started in Session 08B, and integrate the dispenser hardware.

BACKGROUND AND CONTEXT
Session 08B already implemented face capture, QWERTY keyboard, and FatFS saving. We now need to expand `PatientRecord` to include Phone Number and Medicine selection (mapping to the hopper IDs). After dispensing, instead of AI Action Recognition, implement a simple "OK" confirmation button on the screen for the patient to acknowledge consumption.

EXPECTED OUTPUTS
- Extend `state_machine.c` and `ai_vision.c` to capture Phone Number and select Medicine Type/Time.
- Tie the dispensing state to `dispense_dose()` from Session 09.
- Add a "Pill Taken OK" touchscreen button post-dispense.
```
"""

s11 = """# Session 11 - uT-Kernel 3.0 Migration

## Prompt for Antigravity (copy-paste as-is)
```
OBJECTIVE
Migrate the OSAL backend from FreeRTOS to uT-Kernel 3.0.

BACKGROUND AND CONTEXT
The project strictly used `ms_osal.h` everywhere except `ms_osal.c`. Now we swap the backend.

EXPECTED OUTPUTS
- Add uT-Kernel 3.0 source files to the project.
- Rewrite `ms_osal.c` to use uT-Kernel APIs (tk_dly_tsk, tk_cre_tsk, tk_wai_flg, etc.).
- Ensure FatFS OS locks in `ffsystem.c` also use uT-Kernel mutexes.
- Build and verify the entire system runs identically on uT-Kernel.
```
"""

s12 = """# Session 12 - Action/Consumption Recognition (Leftovers)

## Prompt for Antigravity (copy-paste as-is)
```
OBJECTIVE
Replace the "OK" button with the NPU-based action recognition to confirm pill consumption.

BACKGROUND AND CONTEXT
This was deferred from Session 08C. We use a lightweight short-sequence classifier running on the NPU to detect the hand-to-mouth gesture.

EXPECTED OUTPUTS
- Buffer camera frames during/after dispense.
- Run action recognition inference.
- Auto-confirm consumption if the gesture is detected, removing the need for the manual "OK" button.
```
"""

s13 = """# Session 13 - Final Polish & Packaging

## Prompt for Antigravity (copy-paste as-is)
```
OBJECTIVE
Final UI polish, robustness testing, and Contest Packaging.

BACKGROUND AND CONTEXT
The project is complete. Clean up any debug UART prints containing personal data. Ensure SD card disconnects don't crash the system. Write the final README.
```
"""

with open(os.path.join(prompts_dir, "session_09.md"), "w") as f: f.write(s09)
with open(os.path.join(prompts_dir, "session_10.md"), "w") as f: f.write(s10)
with open(os.path.join(prompts_dir, "session_11.md"), "w") as f: f.write(s11)
with open(os.path.join(prompts_dir, "session_12.md"), "w") as f: f.write(s12)
with open(os.path.join(prompts_dir, "session_13.md"), "w") as f: f.write(s13)

print("Prompts updated successfully!")
