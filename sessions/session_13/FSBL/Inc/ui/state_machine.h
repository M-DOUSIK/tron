#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    STATE_IDLE,
    STATE_INTRO,
    STATE_HOME,
    STATE_INSTRUCT_REGISTER,
    STATE_CAMERA_REGISTER,
    STATE_KEYBOARD_REGISTER,    /* Session 09: on-screen name entry */
    STATE_PILLCOUNT_REGISTER,   /* Session 09: daily pill-count entry */
    STATE_CONFIRM_REGISTER,     /* Session 09: summary + confirm/retry */
    STATE_INSTRUCT_DISPENSE,
    STATE_CAMERA_DISPENSE,
    STATE_DISPENSING,           /* Session 10: simulated dispense animation/countdown */
    STATE_CONFIRM_TAKEN,        /* Session 10: "I Took It" / "Skip" confirmation screen */
    /* Session 12 hardening states. Both are error/alert paths for cases the
     * Session 09/10 flows previously ended in a dead end or handled only in a
     * timed dialog — not new functionality. */
    STATE_FACE_RETRY,           /* "Face not recognised" -> TRY AGAIN / CANCEL */
    STATE_ALERT                 /* one-button alert (gallery full, refill, SD) */
} AppState_t;

void state_machine_init(void);
void state_machine_update(void);

#ifdef __cplusplus
}
#endif

#endif // STATE_MACHINE_H
