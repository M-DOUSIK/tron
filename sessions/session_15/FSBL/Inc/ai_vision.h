/* ai_vision.h — public API (do not change signatures; Session 09 depends on them)
 *
 * Session 09B (clean do-over of 08B): real one-shot face recognition + small-
 * gallery matching, replacing the Session 08A throwaway NPU test model.
 * See documents/AI_PIPELINE.md and documents/prompts/session_08B.md.
 */
#ifndef AI_VISION_H
#define AI_VISION_H

#include <stdbool.h>
#include <stdint.h>

#define EMBEDDING_SIZE    128
#define MAX_PATIENTS      10
#define PATIENT_NAME_MAX  32

/*
 * ── What `pill_count` means (corrected in Session 12) ─────────────────────
 *
 * `pill_count` is **the dose**: how many pills this patient takes in one
 * sitting. It is a fixed property of the patient's prescription, set once at
 * registration, and it NEVER changes as a result of dispensing. A patient
 * registered for 3 pills gets 3 pills today, 3 pills tomorrow, and 3 pills
 * every time thereafter.
 *
 * Sessions 10-12 carried a second field, `pills_remaining`, that was
 * initialised to `pill_count` and decremented **by one** on every confirmed
 * dose. That was wrong twice over: it treated the per-dose quantity as if it
 * were a stock level, and then drew down that "stock" one pill at a time
 * regardless of how many the dose actually was. The visible symptom was the
 * dispense screen announcing "3 pills" while the count fell 3 → 2 → 1, so the
 * same patient was offered a smaller dose each day until they were told to
 * refill. The field is gone.
 *
 * There is no stock counter in the data model at all now, deliberately. This
 * prototype's hopper holds one medication type, and the device has no way to
 * know how full it is — a software counter would only ever be a guess that
 * drifts from reality the moment anyone tops the hopper up by hand. Real
 * hopper-level sensing arrives with the IR break-beam counter in Session 14,
 * which counts pills as they physically drop; that is the right place for it,
 * because it measures the hopper instead of assuming it.
 */
/*
 * ── The dose schedule (Session 15) ────────────────────────────────────────
 *
 * The Program Plan submitted in March named "Schedule validation — compares
 * identified medicine against the patient's stored plan" as a CORE FUNCTION,
 * and nothing in this device has ever known the time of day. Session 15
 * closes that gap, and the schedule belongs in the patient record because it
 * is a property of the patient's prescription, exactly like the dose size.
 *
 * MAX_DOSE_TIMES is 4 because that is what real prescriptions use — once,
 * twice, three times or four times a day — and because every extra slot
 * costs two bytes in a record that is written to an SD card in full on every
 * enrolment. Four is a decision, not a limit that was reached for.
 *
 * Times are stored as MINUTE OF DAY (0..1439), the one axis the whole
 * scheduling path uses; see schedule_time_source.h for why. They are kept
 * sorted ascending, which is what lets "the next dose" be found by a single
 * forward scan rather than a sort at every alarm.
 *
 * A patient with dose_time_count == 0 has no schedule. That is a legitimate
 * state, not an error: they can still walk up and tap DISPENSE. The device
 * simply never reminds them and never records a missed dose for them.
 */
#define MAX_DOSE_TIMES    4

typedef struct {
    uint8_t   valid;
    char      name[PATIENT_NAME_MAX];
    int8_t    embedding[EMBEDDING_SIZE];
    uint8_t   pill_count;   /* pills per dose — fixed, never decremented */
    uint8_t   dose_time_count;                 /* Session 15: 0..MAX_DOSE_TIMES */
    uint16_t  dose_time[MAX_DOSE_TIMES];       /* Session 15: minute-of-day, ascending */
} PatientRecord;

extern PatientRecord patient_gallery[MAX_PATIENTS];

/* Init — call once after aiPreInitialize(), before any pipeline call.
 * Initializes the face detector + embedder NPU networks and loads the
 * on-SD-card patient gallery (patients.dat) via gallery_init(). */
void  ai_vision_init(void);

/* Run the full pipeline (detect -> crop -> embed) on the live camera frame.
 * Caller MUST have stopped the camera DMA (DCMIPP pipe) before calling this
 * and may resume it only after this returns — see state_machine.c's
 * g_isp_suspend / camera_stop()+camera_start() sequencing.
 * Writes a 128-byte quantized embedding into out_embedding.
 * Returns true if a face was detected and an embedding extracted. */
bool  ai_vision_run_pipeline(int8_t *out_embedding);

/* Debug-only: runs detect+embed on a known-good embedded test image instead
 * of the live camera. Isolates model/plumbing bugs from live-camera-capture
 * bugs. Prints PASS/FAIL over UART. Safe to call any time after
 * ai_vision_init(); does not touch the camera or BUFFER_ADDRESS. */
bool  ai_vision_self_test(void);

/* Cosine similarity between two 128-D int8 embeddings. Returns [-1.0, 1.0]. */
float ai_vision_match_face(const int8_t *emb1, const int8_t *emb2);

/* Gallery operations */
void  gallery_init(void);
bool  gallery_save(void);
int   gallery_add_patient(const char *name, const int8_t *embedding, int pill_count);
/* Whether the gallery_save() inside the last gallery_add_patient() reached
 * the SD card. A patient added while the card is missing is live in RAM and
 * will match faces immediately, but is lost on the next boot - the UI has to
 * be able to say so rather than reporting a flat "Registered!". */
bool  gallery_last_save_ok(void);
int   gallery_find_best_match(const int8_t *embedding, float *out_confidence);
/* Number of gallery slots currently occupied (0..MAX_PATIENTS). Session 12 —
 * lets the UI refuse a registration up front when the gallery is already
 * full, instead of capturing a face and only then discovering it. */
int   gallery_count(void);

/* ── Carer-mode editing (Session 15) ─────────────────────────────────────
 *
 * These three exist because carer mode is the first thing in this project
 * that MODIFIES an existing enrolment rather than adding one. Each writes
 * patients.dat and reports whether that write reached the card, for the same
 * reason gallery_last_save_ok() exists: a change that lives only in RAM
 * until the next power cycle must not be reported as saved.
 */

/* Replace a patient's dose schedule. `times` are minute-of-day values;
 * `count` may be 0 (no schedule). The array is sorted ascending on the way
 * in, so callers do not have to. */
bool  gallery_set_schedule(int slot, const uint16_t *times, uint8_t count);

/* Set the dose size (pills per sitting). Session 15 moved this out of the
 * patient-facing registration flow — see session_15_notes.md. */
bool  gallery_set_dose(int slot, uint8_t pill_count);

/* Remove a patient entirely: the name AND the embedding, together.
 * COMPLIANCE_PRIVACY_POSTURE.md §5 records that an unauthenticated delete of
 * biometric data was deliberately never built; behind carer mode's password
 * it becomes the right thing to have. A partial delete — clearing the name
 * and leaving the embedding, say — would defeat the entire point, so this
 * memsets the whole record. */
bool  gallery_delete_patient(int slot);

/* ══════════════════════════════════════════════════════════════════════════
 * NPU service task (Session 12)
 *
 * Up to and including Session 11, ai_vision_run_pipeline() was called
 * synchronously from the UI task: the NPU work — several hundred
 * milliseconds per attempt, up to three attempts — ran at the UI task's own
 * priority, on the UI task's stack, and the AI never touched an OS primitive
 * at all (the ST runtime is built LL_ATON_OSAL_BARE_METAL and polls). The AI
 * sat *alongside* the RTOS rather than being mediated by it.
 *
 * Session 12 moves inference into its own µT-Kernel task, one priority level
 * BELOW the UI task, and turns the call into a request/response exchange over
 * a µT-Kernel event flag. Two things follow from that, both real:
 *
 *  - The UI task can preempt inference. Touch polling and the physical USER1
 *    button keep working while the NPU runs, where before the whole UI task
 *    was stuck inside the pipeline call.
 *  - Frame-buffer ownership becomes explicit. Between _request() and _wait()
 *    the AI task owns BUFFER_ADDRESS, because both NPU networks' activation
 *    scratch overlaps it (see the memory-hazard note at the top of
 *    ai_vision.c). The caller must not draw into the framebuffer across that
 *    window, and must already have stopped the camera DMA — the same
 *    contract ai_vision_run_pipeline() always had, now enforced by a wait on
 *    an OS object instead of by "it happens to be the same task".
 *
 * The retry policy (3 attempts, 500 ms apart) moved into the task with the
 * inference, so the whole capture is one round trip for the caller.
 * ══════════════════════════════════════════════════════════════════════════ */

/** Outcome of one capture request. */
typedef enum {
    AI_CAPTURE_OK = 0,    /* face found; embedding written                   */
    AI_CAPTURE_NO_FACE,   /* pipeline ran, no face after all retries         */
    AI_CAPTURE_TIMEOUT,   /* AI task did not answer in time (see notes)      */
    AI_CAPTURE_NOT_READY  /* ai_vision_init() has not completed / NPU failed */
} ai_capture_result_t;

/** Create the AI service's event flag. Call once from main(), alongside the
 *  other osal_*_create() calls, BEFORE osal_scheduler_start(). */
void ai_vision_service_init(void);

/** Task body for the NPU task. Never returns — create it via
 *  osal_task_create() (main.c). Runs ai_vision_init() itself, then serves
 *  capture requests forever. */
void task_ai_fn(void *arg);

/** True once the AI task has finished ai_vision_init() successfully. */
/* Block until task_ai has finished ai_vision_init(), or until timeout_ms.
 * Returns true if initialisation completed, false on timeout.
 *
 * WHY THIS EXISTS — the cold-boot display fault.
 * ---------------------------------------------
 * Both networks' activation arena is placed by the Cube.AI codegen at
 * 0x34200000, which is BUFFER_ADDRESS: the LCD framebuffer. That overlap is
 * deliberate and safe during a capture, because the UI hands the buffer over
 * and does not draw until the result comes back.
 *
 * It was NOT safe at start-up. Up to Session 11, ai_vision_init() ran on the
 * UI task itself, so it necessarily completed before the home screen was
 * ever drawn. Session 12 moved it onto task_ai (priority 3, below the UI's
 * 4) to keep several hundred ms of blocking init off the UI's startup path —
 * and in doing so turned a guaranteed ordering into a race. task_ui draws
 * the home screen, then sleeps 10 ms in its poll loop; task_ai runs during
 * those sleeps and finishes bringing up the NPU on top of the pixels.
 *
 * On the bench that looked like the display coming up, glitching, and then
 * greying out entirely while UART, touch, SD and every task stayed perfectly
 * healthy — and like a *timing* fault, because it went away whenever the
 * relative ordering shifted (a warm re-flash, or pausing in the debugger).
 * It was chased as a power-domain problem, then a warm-reset problem, then a
 * missing-boot-delay problem, and it was none of those; the tell was in the
 * boot log all along, where "BSP_TS_Init successful." had moved from just
 * after "task_ui: started." to after every other task's banner.
 *
 * The fix is to restore the ordering explicitly instead of by accident:
 * state_machine_init() waits here before it touches the framebuffer. Nothing
 * about the concurrency is given up — this costs the UI task only the
 * start-up window, and the AI task keeps its own priority and stack for
 * every capture afterwards.
 *
 * ai_vision_init()'s own comment already recorded that running NPU work over
 * a freshly drawn home screen corrupts it. It was written about
 * ai_vision_self_test(); it applies just as much to the init itself. */
bool ai_vision_wait_init(uint32_t timeout_ms);

bool ai_vision_is_ready(void);

/** Ask the AI task to capture a face from the frozen camera frame.
 *  Caller MUST have stopped the camera DMA first and MUST NOT touch
 *  BUFFER_ADDRESS until the matching ai_vision_capture_wait() returns. */
void ai_vision_capture_request(void);

/** Wait for the capture posted by ai_vision_capture_request().
 *  On AI_CAPTURE_OK the 128-byte embedding is copied into out_embedding. */
ai_capture_result_t ai_vision_capture_wait(int8_t *out_embedding,
                                            uint32_t timeout_ms);

#endif /* AI_VISION_H */
