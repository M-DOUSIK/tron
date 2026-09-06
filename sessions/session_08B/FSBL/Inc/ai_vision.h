/* ai_vision.h — public API (do not change signatures; Session 09 depends on them)
 *
 * Session 08B: real one-shot face recognition + small-gallery matching,
 * replacing the Session 08A throwaway NPU test model.
 * See MedSight_Docs/AI_PIPELINE.md and MedSight_Docs/prompts/session_08B.md.
 */
#ifndef AI_VISION_H
#define AI_VISION_H

#include <stdbool.h>
#include <stdint.h>

#define EMBEDDING_SIZE    128
#define MAX_PATIENTS      10
#define PATIENT_NAME_MAX  32

typedef struct {
    uint8_t   valid;
    char      name[PATIENT_NAME_MAX];
    int8_t    embedding[EMBEDDING_SIZE];
    uint8_t   pill_count;
    uint8_t   pills_remaining;
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
int   gallery_find_best_match(const int8_t *embedding, float *out_confidence);

#endif /* AI_VISION_H */
