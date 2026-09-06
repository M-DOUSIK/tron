#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    STATE_HOME,
    STATE_INSTRUCT_REGISTER,
    STATE_CAMERA_REGISTER,
    STATE_KEYBOARD_REGISTER,    /* Session 09: on-screen name entry */
    STATE_PILLCOUNT_REGISTER,   /* Session 09: daily pill-count entry */
    STATE_CONFIRM_REGISTER,     /* Session 09: summary + confirm/retry */
    STATE_INSTRUCT_DISPENSE,
    STATE_CAMERA_DISPENSE
} AppState_t;

void state_machine_init(void);
void state_machine_update(void);

#ifdef __cplusplus
}
#endif

#endif // STATE_MACHINE_H
