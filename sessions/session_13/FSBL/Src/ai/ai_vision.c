/* ai_vision.c — MedSight face recognition pipeline (Session 09B / clean do-over
 * of Session 08B).
 *
 * Replaces the Session 08A throwaway NPU test model with the real two-model
 * pipeline: CenterFace face detector -> FaceID embedder -> gallery match.
 * Model wrapper code (stai_fd.*, stai_faceid.*, fd.c, faceid.c, *_ecblobs.h)
 * is copied verbatim from the proven PeleAB reference project
 * (tools/PeleAB_repo/Model; that folder was named scratch/ at the time) per
 * documents/prompts/session_08B.md; only this file, ai_vision.h, and the
 * sd_logger.c file-I/O additions are original to MedSight.
 *
 * IMPORTANT MEMORY HAZARD (found during this session, not in the original
 * briefing): both generated networks' activation scratch pools are hardcoded
 * by STM32Cube.AI codegen to overlap BUFFER_ADDRESS (0x34200000) — see the
 * epoch-block cache-invalidate calls in faceid.c/fd.c and PeleAB's own
 * Doc/Application-Overview.md ("activations | 507 KB | 0x34200000 | NPURAMS").
 * That address is our live camera framebuffer. Running either network
 * therefore clobbers the camera frame currently sitting there. The pipeline
 * below copies the region it needs (a centered crop) into a static hold
 * buffer BEFORE calling stai_fd_run(), and does all further cropping (for the
 * embedder) from that hold buffer, never from BUFFER_ADDRESS again once
 * inference has started. Camera DMA must also be stopped by the caller
 * (state_machine.c) before calling ai_vision_run_pipeline() — see
 * ai_vision.h's contract note.
 */

#include "ai_vision.h"
#include "ms_osal.h"
#include "sd_logger.h"
#include "main.h"          /* SCB_CleanDCache_by_Addr / SCB_InvalidateDCache_by_Addr */
#include "stai_fd.h"
#include "stai_faceid.h"
#include "app/face_debug_image.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

/* ── Patient gallery (Session 08B placeholder; Session 09 wires real
 *    enrollment on top of this same storage) ──────────────────────────── */
PatientRecord patient_gallery[MAX_PATIENTS];

#define GALLERY_FILE "patients.dat"

/* ── patients.dat on-card format (Session 12) ─────────────────────────────
 *
 * Sessions 08B-12 wrote the raw PatientRecord[] array to the card with no
 * header at all, and gallery_init() accepted it if — and only if — the file
 * length happened to equal sizeof(patient_gallery). Any other length, for any
 * reason, produced the single line "starting with an empty gallery", which is
 * the same thing it prints on a genuine first boot. So "the file is from an
 * older firmware", "the file is truncated", "the file belongs to a different
 * device" and "there is no file" were all indistinguishable, and all silently
 * discarded every enrolled patient.
 *
 * That mattered the moment this session changed PatientRecord (dropping
 * pills_remaining shrinks a record from 163 to 162 bytes, and the gallery from
 * 1630 to 1620), because every existing card became silently unreadable.
 *
 * The file now starts with a small header carrying a magic number, a format
 * version, and the record geometry it was written with. A mismatch is
 * reported specifically, so the log says which of those four situations you
 * are actually in. */
#define GALLERY_MAGIC     0x4753444Du   /* 'M','D','S','G' little-endian */
#define GALLERY_VERSION   2u            /* v2 = Session 12, pills_remaining removed */

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t record_size;    /* sizeof(PatientRecord) when written */
    uint16_t record_count;   /* MAX_PATIENTS when written          */
    uint16_t reserved;       /* zero; keeps the header 4-byte aligned */
} GalleryFileHeader;

#define GALLERY_FILE_BYTES  (sizeof(GalleryFileHeader) + sizeof(patient_gallery))
/* Cosine-similarity accept threshold. Matches PeleAB's face_gallery.c
 * FACE_GALLERY_MATCH_SIMILARITY default (0.65f) — starting point only, tune
 * against real enrolled-vs-impostor measurements during hardware bring-up. */
#define GALLERY_MATCH_THRESHOLD 0.65f

/* ── CenterFace detector output head indices ────────────────────────────
 * Verified against PeleAB's Inc/svc/face_detect.h header comment (STEdgeAI
 * output order confirmed there by value-range analysis on the original
 * centerface_OE_3_3_1.onnx) rather than re-derived here. Not re-using that
 * header directly — it pulls in arm_math.h + od_pp_output_if.h, a CMSIS-DSP
 * postprocessing dependency this project doesn't otherwise need. */
#define FD_OUT_SCALE     0
#define FD_OUT_LANDMARKS 1
#define FD_OUT_HEATMAP   2
#define FD_OUT_OFFSET    3
#define FD_GRID          32   /* STAI_FD_OUT_3_HEIGHT == STAI_FD_OUT_3_WIDTH */
/* Detector confidence threshold (post-sigmoid heatmap value, 0..1). */
#define FD_CONF_THRESHOLD 0.5f

/* Centered square crop taken from the live 800x480 RGB565 frame before any
 * NPU model runs (see hazard note above). Uses the FULL frame height
 * (480px) — a 256x256 first attempt turned out too tight on real hardware
 * (first hardware run: detector consistently found no face, most likely
 * because the live face wasn't reliably falling inside a 256-wide/256-tall
 * window centered on an 800x480 frame). 480x480 RGB565 = 450 KB — still
 * comfortably inside the 1023 KB RAM budget. Re-tune further if real
 * framing still puts faces outside it. */
#define CROP_SIZE 480

/* ── NPU network contexts (one context buffer per model, per Session 08A's
 *    established pattern of a byte-sized array cast to the runtime's
 *    internal context struct) ───────────────────────────────────────────── */
static stai_network fd_network[STAI_FD_CONTEXT_SIZE]
    __attribute__((aligned(STAI_FD_CONTEXT_ALIGNMENT)));
static stai_network faceid_network[STAI_FACEID_CONTEXT_SIZE]
    __attribute__((aligned(STAI_FACEID_CONTEXT_ALIGNMENT)));

static stai_ptr fd_outputs[STAI_FD_OUT_NUM];
static stai_ptr faceid_inputs[STAI_FACEID_IN_NUM];
static stai_ptr faceid_outputs[STAI_FACEID_OUT_NUM];

/* Detector input is NOT preallocated by the generated network (its per-buffer
 * flags lack STAI_FLAG_PREALLOCATED) — we own this buffer and bind it once
 * via stai_fd_set_inputs() in ai_vision_init(). FaceID's input IS
 * preallocated, so no equivalent buffer is declared for it; its pointer is
 * fetched via stai_faceid_get_inputs() instead. */
static uint8_t fd_input_buf[STAI_FD_IN_1_SIZE_BYTES] __attribute__((aligned(32)));

/* FaceID's OUTPUT (unlike its input) is also NOT preallocated —
 * STAI_FACEID_OUT_1_FLAGS is STAI_FLAG_OVERRIDE only, no
 * STAI_FLAG_PREALLOCATED. Confirmed against PeleAB's own working code
 * (app_pipeline.c calls stai_faceid_set_outputs() with its own buffer
 * before every run) — we do the same, once, in ai_vision_init(). Raw
 * float32[128] embedding output. */
static float faceid_output_buf[EMBEDDING_SIZE] __attribute__((aligned(32)));

/* Live-frame hold buffer — see the memory-hazard note at the top of this file. */
static uint16_t s_frame_hold[CROP_SIZE * CROP_SIZE] __attribute__((aligned(32)));

static bool s_ai_ready = false;

/* ══════════════════════════════════════════════════════════════════════════
 * Pixel format conversion helpers
 * ══════════════════════════════════════════════════════════════════════════ */

static inline void rgb565_to_rgb888(uint16_t px, uint8_t *r, uint8_t *g, uint8_t *b)
{
    *r = (uint8_t)(((px >> 11) & 0x1F) << 3);
    *g = (uint8_t)(((px >> 5) & 0x3F) << 2);
    *b = (uint8_t)((px & 0x1F) << 3);
}

/* Nearest-neighbor downscale of the CROP_SIZE x CROP_SIZE RGB565 hold buffer
 * into the detector's 128x128x3 U8 input.
 *
 * LAYOUT CORRECTION (found debugging on real hardware, not from the header
 * metadata): stai_fd.h reports STAI_FD_IN_1_FLAGS as STAI_FLAG_CHANNEL_LAST,
 * which reads as NHWC (interleaved RGB per pixel). That's what this function
 * originally did, and on hardware the detector consistently found nothing —
 * heatmap output was uniformly uncomparable (effectively NaN), meaning the
 * NPU never saw a sane image. Cross-checking against PeleAB's own actually-
 * working inference code (Src/app/app_pipeline.c's camera_bgr_to_rgb_chw(),
 * called on this exact fd_input_buffer before stai_fd_run()) shows the real
 * expected layout is CHW (planar): all R bytes, then all G, then all B —
 * i.e. the STAI_FLAG_CHANNEL_LAST metadata does not describe the actual
 * memory layout this compiled model expects. Trust the working reference
 * over the header flag.
 *
 * SCALE=1/OFFSET=0 (STAI_FD_IN_1_SCALES/OFFSETS) still means raw 0..255 RGB
 * bytes, no rescaling needed beyond RGB565 -> RGB888 expansion.
 * NOTE: STAI_FD_IN_1_SHAPE == {1,3,128,128} (batch,channel,height,width) is
 * ground truth for the real 128x128 spatial size; the individual
 * STAI_FD_IN_1_CHANNEL/HEIGHT/WIDTH macros in stai_fd.h are mislabeled by
 * codegen (CHANNEL reads 128, HEIGHT reads 3) — do not trust those macros
 * for dimensions, trust SHAPE. */
static void convert_crop_to_fd_input(const uint16_t *crop, int in_size, uint8_t *out_chw)
{
    const int out_size = 128;
    const int plane_size = out_size * out_size;
    uint8_t *r_plane = &out_chw[0 * plane_size];
    uint8_t *g_plane = &out_chw[1 * plane_size];
    uint8_t *b_plane = &out_chw[2 * plane_size];

    for (int y = 0; y < out_size; y++) {
        int sy = (y * in_size) / out_size;
        const uint16_t *src_row = &crop[sy * in_size];
        int row_off = y * out_size;
        for (int x = 0; x < out_size; x++) {
            int sx = (x * in_size) / out_size;
            uint8_t r, g, b;
            rgb565_to_rgb888(src_row[sx], &r, &g, &b);
            r_plane[row_off + x] = r;
            g_plane[row_off + x] = g;
            b_plane[row_off + x] = b;
        }
    }
}

/* Crop [box_x,box_y,box_w,box_h] (in hold-buffer pixel space) out of the
 * hold buffer and resize to the embedder's 112x112x3 U8 NCHW input.
 * FaceID input is STAI_FLAG_CHANNEL_FIRST, SCALE=0.00784313771873713,
 * OFFSET=128 — i.e. dequantized value = (stored - 128) * (1/127.5), which is
 * exactly the standard "pixel/127.5 - 1" normalization computed FROM a raw
 * 0..255 byte. We again just write raw RGB bytes; the model's own
 * scale/offset already encodes the normalization. */
static void convert_crop_to_faceid_input(const uint16_t *hold, int hold_w, int hold_h,
                                          int box_x, int box_y, int box_w, int box_h,
                                          uint8_t *out_nchw)
{
    const int out_size = 112;
    uint8_t *r_plane = &out_nchw[0 * out_size * out_size];
    uint8_t *g_plane = &out_nchw[1 * out_size * out_size];
    uint8_t *b_plane = &out_nchw[2 * out_size * out_size];

    if (box_w < 1) box_w = 1;
    if (box_h < 1) box_h = 1;

    for (int y = 0; y < out_size; y++) {
        int sy = box_y + (y * box_h) / out_size;
        if (sy < 0) sy = 0;
        if (sy >= hold_h) sy = hold_h - 1;
        for (int x = 0; x < out_size; x++) {
            int sx = box_x + (x * box_w) / out_size;
            if (sx < 0) sx = 0;
            if (sx >= hold_w) sx = hold_w - 1;
            uint8_t r, g, b;
            rgb565_to_rgb888(hold[sy * hold_w + sx], &r, &g, &b);
            int idx = y * out_size + x;
            r_plane[idx] = r;
            g_plane[idx] = g;
            b_plane[idx] = b;
        }
    }
}

/* ══════════════════════════════════════════════════════════════════════════
 * CenterFace post-processing (single-best-box — this device only ever needs
 * to identify the one patient standing in front of it, so a full multi-box
 * decode + NMS pipeline would be pure overhead; we take the single grid cell
 * with the highest heatmap confidence).
 *
 * Standard CenterFace decode (verify against the original
 * centerface_OE_3_3_1.onnx training config on hardware — this is the
 * well-known public formula, not re-derived from this specific export):
 *   cx = (gx + offset_x) * stride
 *   cy = (gy + offset_y) * stride
 *   w  = exp(scale_w) * stride
 *   h  = exp(scale_h) * stride
 * stride = detector_input_size / grid = 128 / 32 = 4, expressed here in
 * the caller's hold-buffer space (hold_size — CROP_SIZE for the live
 * camera path, or the debug image's own native size for the self-test),
 * so stride_in_crop = 4 * (hold_size/128).
 * ══════════════════════════════════════════════════════════════════════════ */
/* Always returns the argmax grid cell and its confidence via *out_conf,
 * regardless of FD_CONF_THRESHOLD — the caller decides pass/fail so it can
 * log the true max confidence even on a miss (useful for tuning the
 * threshold/crop against real hardware instead of guessing blind). Returns
 * false only if the heatmap is completely empty/invalid (never happens in
 * practice — grid is always > 0), true otherwise; check *out_conf against
 * FD_CONF_THRESHOLD yourself. */
static bool decode_best_face_box(int hold_size, int *out_x, int *out_y, int *out_w, int *out_h, float *out_conf)
{
    const float *heatmap = (const float *)fd_outputs[FD_OUT_HEATMAP];
    const float *scale   = (const float *)fd_outputs[FD_OUT_SCALE];
    const float *offset  = (const float *)fd_outputs[FD_OUT_OFFSET];

    int best_idx = -1;
    float best_val = -1.0e30f;
    for (int i = 0; i < FD_GRID * FD_GRID; i++) {
        if (heatmap[i] > best_val) {
            best_val = heatmap[i];
            best_idx = i;
        }
    }
    if (best_idx < 0) {
        return false;
    }
    if (out_conf) *out_conf = best_val;

    int gy = best_idx / FD_GRID;
    int gx = best_idx % FD_GRID;

    /* offset/scale tensors are [1,32,32,2] channel-last: index*2+0 = y/h,
     * index*2+1 = x/w (per face_detect.h's documented head layout). */
    float off_y = offset[(gy * FD_GRID + gx) * 2 + 0];
    float off_x = offset[(gy * FD_GRID + gx) * 2 + 1];
    float sc_h  = scale[(gy * FD_GRID + gx) * 2 + 0];
    float sc_w  = scale[(gy * FD_GRID + gx) * 2 + 1];

    const float stride_in_crop = 4.0f * ((float)hold_size / 128.0f);

    float cx = (gx + off_x) * stride_in_crop;
    float cy = (gy + off_y) * stride_in_crop;
    float w  = expf(sc_w) * stride_in_crop;
    float h  = expf(sc_h) * stride_in_crop;

    *out_x = (int)(cx - w / 2.0f);
    *out_y = (int)(cy - h / 2.0f);
    *out_w = (int)w;
    *out_h = (int)h;
    return true;
}

/* ══════════════════════════════════════════════════════════════════════════
 * Public API
 * ══════════════════════════════════════════════════════════════════════════ */

void ai_vision_init(void)
{
    /* Session 12: say which build this is, before touching the NPU.
     *
     * The epoch-controller blobs live in external OSPI NOR, which the linker
     * script marks (NOLOAD) — so they are flashed by hand, once, and a
     * mismatch between what is physically in flash and what this binary
     * expects shows up as an assert deep inside the ST runtime with no clue
     * as to why (see __assert_func() in main.c).
     *
     * `__OPTIMIZE__` is defined by GCC at -O1 and above, so it distinguishes
     * the Release build (-Os) from Debug (-O0) without relying on the
     * project's own DEBUG/NDEBUG defines — which are, as it happens,
     * inverted in this project's .cproject.
     *
     * Both configurations now produce the SAME OSPI layout: Session 12 added
     * -fno-toplevel-reorder to the Release compiler settings after finding
     * that -Os emitted the blobs in reverse order, which made a Debug-flashed
     * weight image unusable from Release. One flashed image serves both. */
#ifdef __OPTIMIZE__
    printf("ai_vision_init: RELEASE build (-Os).\r\n");
#else
    printf("ai_vision_init: DEBUG build (-O0).\r\n");
#endif

    extern void aiPreInitialize(void);
    aiPreInitialize();

    stai_return_code err = stai_runtime_init();
    if (err != STAI_SUCCESS) {
        printf("ai_vision_init: stai_runtime_init failed: %d\r\n", err);
        return;
    }

    err = stai_fd_init(fd_network);
    if (err != STAI_SUCCESS) {
        printf("ai_vision_init: stai_fd_init failed: %d\r\n", err);
        return;
    }

    err = stai_faceid_init(faceid_network);
    if (err != STAI_SUCCESS) {
        printf("ai_vision_init: stai_faceid_init failed: %d\r\n", err);
        return;
    }

    /* Detector input is not preallocated: bind our own buffer once. */
    stai_ptr fd_in_ptr = (stai_ptr)fd_input_buf;
    stai_size fd_in_n = STAI_FD_IN_NUM;
    err = stai_fd_set_inputs(fd_network, &fd_in_ptr, fd_in_n);
    if (err != STAI_SUCCESS) {
        printf("ai_vision_init: stai_fd_set_inputs failed: %d\r\n", err);
        return;
    }

    stai_size fd_out_n = STAI_FD_OUT_NUM;
    err = stai_fd_get_outputs(fd_network, fd_outputs, &fd_out_n);
    if (err != STAI_SUCCESS) {
        printf("ai_vision_init: stai_fd_get_outputs failed: %d\r\n", err);
        return;
    }

    /* FaceID input IS preallocated: just fetch its fixed pointer. */
    stai_size faceid_in_n = STAI_FACEID_IN_NUM;
    err = stai_faceid_get_inputs(faceid_network, faceid_inputs, &faceid_in_n);
    if (err != STAI_SUCCESS) {
        printf("ai_vision_init: stai_faceid_get_inputs failed: %d\r\n", err);
        return;
    }

    /* FaceID output is NOT preallocated: bind our own buffer once (mirrors
     * the fd input handling above, and matches PeleAB's proven pattern). */
    stai_ptr faceid_out_ptr = (stai_ptr)faceid_output_buf;
    stai_size faceid_out_n = STAI_FACEID_OUT_NUM;
    err = stai_faceid_set_outputs(faceid_network, &faceid_out_ptr, faceid_out_n);
    if (err != STAI_SUCCESS) {
        printf("ai_vision_init: stai_faceid_set_outputs failed: %d\r\n", err);
        return;
    }
    faceid_outputs[0] = faceid_out_ptr;

    gallery_init();

    s_ai_ready = true;
    printf("ai_vision_init: face detector + embedder ready.\r\n");

    /* Session 09B: ai_vision_self_test() was called here unconditionally
     * while debugging the NPU/weight-loading bugs documented in
     * session_08B_notes.md (Addendums 1-4) — it's what proved detection and
     * embedding both work correctly. Now that live detection is confirmed
     * working end-to-end (real dispense-flow test: correct "intruder" result
     * against an empty gallery), it's no longer called automatically: running
     * it at every boot corrupted the freshly-drawn home screen (same NPU-
     * activation-overlaps-display-buffer issue as the live dispense flow's
     * transient glitch — see the memory-hazard note atop this file) for no
     * ongoing benefit. Call ai_vision_self_test() manually if this pipeline
     * ever needs re-diagnosing. */
}

/* Runs detect+embed on the given hold_size x hold_size RGB565 buffer.
 * Shared by the live-camera path (ai_vision_run_pipeline, hold_size ==
 * CROP_SIZE) and the static-image self-test (ai_vision_self_test, hold_size
 * == the test image's native 128) so both exercise identical model-facing
 * code without diluting a small test image inside a mostly-black crop. */
static bool run_pipeline_on_hold_buffer(const uint16_t *hold, int hold_size, int8_t *out_embedding)
{
    /* 2. Face detector */
    convert_crop_to_fd_input(hold, hold_size, fd_input_buf);
    SCB_CleanDCache_by_Addr((uint32_t *)fd_input_buf, sizeof(fd_input_buf));

    stai_return_code err = stai_fd_run(fd_network, STAI_MODE_SYNC);
    printf("det_run done rc=%d\r\n", err);
    if (err != STAI_SUCCESS) {
        return false;
    }

    SCB_InvalidateDCache_by_Addr((uint32_t *)fd_outputs[FD_OUT_HEATMAP], STAI_FD_OUT_3_SIZE_BYTES);
    SCB_InvalidateDCache_by_Addr((uint32_t *)fd_outputs[FD_OUT_SCALE], STAI_FD_OUT_1_SIZE_BYTES);
    SCB_InvalidateDCache_by_Addr((uint32_t *)fd_outputs[FD_OUT_OFFSET], STAI_FD_OUT_4_SIZE_BYTES);

    int box_x, box_y, box_w, box_h;
    float box_conf = -1.0f;
    bool  got_box = decode_best_face_box(hold_size, &box_x, &box_y, &box_w, &box_h, &box_conf);

    /* Always log the raw best confidence found, pass or fail — this is the
     * key diagnostic for tuning FD_CONF_THRESHOLD / CROP_SIZE against real
     * hardware instead of guessing blind. %d.%02d because this toolchain's
     * nano.specs libc printf doesn't support %f. */
    {
        int conf_i = (int)box_conf;
        int conf_frac = (int)((box_conf - (float)conf_i) * 100.0f);
        if (conf_frac < 0) conf_frac = -conf_frac;
        printf("Detector: best heatmap conf=%d.%02d (threshold=%d.%02d) at grid cell\r\n",
               conf_i, conf_frac, (int)FD_CONF_THRESHOLD, (int)(FD_CONF_THRESHOLD * 100.0f) % 100);
    }

    if (!got_box || box_conf < FD_CONF_THRESHOLD) {
        printf("Detector: No face found.\r\n");
        return false;
    }
    printf("Detector: Face detected!\r\n");

    /* Clamp the box into the hold buffer's bounds. */
    if (box_x < 0) box_x = 0;
    if (box_y < 0) box_y = 0;
    if (box_x + box_w > hold_size) box_w = hold_size - box_x;
    if (box_y + box_h > hold_size) box_h = hold_size - box_y;

    /* 3. Face embedder — crop from the hold buffer, NOT BUFFER_ADDRESS
     *    (which the detector run above has already clobbered). */
    convert_crop_to_faceid_input(hold, hold_size, hold_size,
                                  box_x, box_y, box_w, box_h,
                                  (uint8_t *)faceid_inputs[0]);
    SCB_CleanDCache_by_Addr((uint32_t *)faceid_inputs[0], STAI_FACEID_IN_1_SIZE_BYTES);

    err = stai_faceid_run(faceid_network, STAI_MODE_SYNC);
    printf("emb_run done rc=%d\r\n", err);
    if (err != STAI_SUCCESS) {
        return false;
    }

    SCB_InvalidateDCache_by_Addr((uint32_t *)faceid_outputs[0], STAI_FACEID_OUT_1_SIZE_BYTES);

    /* 4. Normalize the raw float32 embedding to unit L2 norm, then quantize
     *    to int8 (ai_vision.h's mandated storage format) — normalizing
     *    first maximizes use of the int8 dynamic range regardless of the
     *    model's raw output scale. Never printf the embedding itself — it
     *    is biometric data (COMPLIANCE_PRIVACY_POSTURE.md). */
    const float *emb_f32 = (const float *)faceid_outputs[0];
    float sumsq = 0.0f;
    for (int i = 0; i < EMBEDDING_SIZE; i++) {
        sumsq += emb_f32[i] * emb_f32[i];
    }
    float inv_norm = (sumsq > 1.0e-12f) ? (1.0f / sqrtf(sumsq)) : 0.0f;
    for (int i = 0; i < EMBEDDING_SIZE; i++) {
        float v = emb_f32[i] * inv_norm * 127.0f;
        if (v > 127.0f) v = 127.0f;
        if (v < -127.0f) v = -127.0f;
        out_embedding[i] = (int8_t)lrintf(v);
    }

    return true;
}

bool ai_vision_run_pipeline(int8_t *out_embedding)
{
    if (!out_embedding || !s_ai_ready) {
        return false;
    }

    /* 1. Snapshot a centered crop of the live frame BEFORE any NPU run —
     *    see the memory-hazard note at the top of this file. Caller must
     *    already have stopped the camera DMA pipe. */
    const uint16_t *frame = (const uint16_t *)BUFFER_ADDRESS;
    const int x0 = (FRAME_WIDTH - CROP_SIZE) / 2;
    const int y0 = (FRAME_HEIGHT - CROP_SIZE) / 2;

    SCB_InvalidateDCache_by_Addr((uint32_t *)BUFFER_ADDRESS, FRAME_BUFFER_SIZE);
    for (int row = 0; row < CROP_SIZE; row++) {
        memcpy(&s_frame_hold[row * CROP_SIZE],
               &frame[(y0 + row) * FRAME_WIDTH + x0],
               CROP_SIZE * sizeof(uint16_t));
    }

    return run_pipeline_on_hold_buffer(s_frame_hold, CROP_SIZE, out_embedding);
}

bool ai_vision_self_test(void)
{
    if (!s_ai_ready) {
        printf("ai_vision_self_test: not ready.\r\n");
        return false;
    }

    /* Known-good 128x128 RGB565 test face (skimage 'astronaut' fixture,
     * copied from PeleAB's own self-test mechanism — Src/app/
     * face_debug_image.c / FACE_INPUT_STATIC in their app_pipeline.c).
     * Bypasses the live camera entirely and runs the exact same detect+
     * embed code path as the live pipeline, at the image's OWN native
     * 128x128 resolution (matching the detector's native input size
     * exactly, no padding/dilution) — an earlier version of this self-test
     * centered the image inside a CROP_SIZE-sized black-padded buffer,
     * which shrank the actual face to a tiny fraction of the detector's
     * field of view after downscaling and produced a false-negative
     * (conf=0.17, correctly below threshold for how little face content
     * was actually visible) even after the underlying weight-loading bug
     * was fixed. Feeding it at native size avoids that dilution entirely.
     * If this finds a face, the model/init/output-read plumbing is proven
     * correct; if it still fails, the bug is upstream of the camera
     * (model wiring, weights, or output handling). */
    int8_t dummy_embedding[EMBEDDING_SIZE];
    printf("ai_vision_self_test: running detect+embed on known-good test image...\r\n");
    bool ok = run_pipeline_on_hold_buffer(g_face_debug_image_rgb565,
                                           FACE_DEBUG_IMAGE_WIDTH, dummy_embedding);
    printf("ai_vision_self_test: %s\r\n", ok ? "PASS (face found)" : "FAIL (no face found)");
    return ok;
}

float ai_vision_match_face(const int8_t *emb1, const int8_t *emb2)
{
    if (!emb1 || !emb2) {
        return -1.0f;
    }

    float f1[EMBEDDING_SIZE], f2[EMBEDDING_SIZE];
    float sumsq1 = 0.0f, sumsq2 = 0.0f;
    for (int i = 0; i < EMBEDDING_SIZE; i++) {
        f1[i] = (float)emb1[i];
        f2[i] = (float)emb2[i];
        sumsq1 += f1[i] * f1[i];
        sumsq2 += f2[i] * f2[i];
    }
    if (sumsq1 < 1.0e-9f || sumsq2 < 1.0e-9f) {
        return -1.0f;
    }
    float inv1 = 1.0f / sqrtf(sumsq1);
    float inv2 = 1.0f / sqrtf(sumsq2);

    float dot = 0.0f;
    for (int i = 0; i < EMBEDDING_SIZE; i++) {
        dot += (f1[i] * inv1) * (f2[i] * inv2);
    }
    if (dot > 1.0f) dot = 1.0f;
    if (dot < -1.0f) dot = -1.0f;
    return dot;
}

/* ══════════════════════════════════════════════════════════════════════════
 * Gallery — Session 08B placeholder storage. Persists to the SD card
 * (patients.dat) via sd_logger.c's generic file API, per
 * SOFTWARE_ARCHITECTURE.md's rule that only sd_logger.c touches FATFS
 * directly. Session 09 replaces this placeholder with the canonical
 * patient_profile_t-based storage from the real registration flow.
 * ══════════════════════════════════════════════════════════════════════════ */

/* Staging buffer for the whole file (header + records). Static rather than a
 * local because the AI task's stack is 8 KB and this is ~1.6 KB. */
static uint8_t s_gallery_io[GALLERY_FILE_BYTES];

void gallery_init(void)
{
    memset(patient_gallery, 0, sizeof(patient_gallery));

    uint32_t bytes_read = 0;
    bool ok = SD_Read_File(GALLERY_FILE, s_gallery_io,
                            (uint32_t)sizeof(s_gallery_io), &bytes_read);
    if (!ok) {
        /* SD_Read_File already said whether this was "no such file" (normal on
         * a first boot) or a real media error. */
        printf("gallery_init: no gallery loaded - starting empty.\r\n");
        return;
    }

    if (bytes_read != (uint32_t)GALLERY_FILE_BYTES) {
        printf("gallery_init: %s is %lu bytes, expected %lu - IGNORED, "
               "starting empty.\r\n",
               GALLERY_FILE, (unsigned long)bytes_read,
               (unsigned long)GALLERY_FILE_BYTES);
        return;
    }

    GalleryFileHeader hdr;
    memcpy(&hdr, s_gallery_io, sizeof(hdr));

    if (hdr.magic != GALLERY_MAGIC) {
        printf("gallery_init: %s has no MedSight header (magic %08lX) - "
               "IGNORED, starting empty.\r\n",
               GALLERY_FILE, (unsigned long)hdr.magic);
        return;
    }
    if (hdr.version != GALLERY_VERSION ||
        hdr.record_size != (uint16_t)sizeof(PatientRecord) ||
        hdr.record_count != (uint16_t)MAX_PATIENTS) {
        printf("gallery_init: %s is format v%u (%ux%u bytes), this firmware "
               "wants v%u (%ux%u) - IGNORED, please re-register.\r\n",
               GALLERY_FILE, (unsigned)hdr.version,
               (unsigned)hdr.record_count, (unsigned)hdr.record_size,
               (unsigned)GALLERY_VERSION,
               (unsigned)MAX_PATIENTS, (unsigned)sizeof(PatientRecord));
        return;
    }

    memcpy(patient_gallery, s_gallery_io + sizeof(hdr), sizeof(patient_gallery));

    int count = 0;
    for (int i = 0; i < MAX_PATIENTS; i++) {
        if (patient_gallery[i].valid) {
            count++;
            /* Names and dose counts are loggable; embeddings never are. */
            printf("gallery_init:   slot %d = '%s', %u pill(s) per dose\r\n",
                   i, patient_gallery[i].name,
                   (unsigned)patient_gallery[i].pill_count);
        }
    }
    printf("gallery_init: loaded %d patient(s) from %s (format v%u).\r\n",
           count, GALLERY_FILE, (unsigned)GALLERY_VERSION);
}

bool gallery_save(void)
{
    GalleryFileHeader hdr = {
        .magic        = GALLERY_MAGIC,
        .version      = GALLERY_VERSION,
        .record_size  = (uint16_t)sizeof(PatientRecord),
        .record_count = (uint16_t)MAX_PATIENTS,
        .reserved     = 0u,
    };
    memcpy(s_gallery_io, &hdr, sizeof(hdr));
    memcpy(s_gallery_io + sizeof(hdr), patient_gallery, sizeof(patient_gallery));

    bool ok = SD_Write_File(GALLERY_FILE, s_gallery_io,
                            (uint32_t)sizeof(s_gallery_io));
    if (!ok) {
        printf("gallery_save: FAILED to write %s.\r\n", GALLERY_FILE);
    }
    return ok;
}

/* Set by every gallery_add_patient(); read by the registration UI. */
static bool s_last_save_ok = false;

bool gallery_last_save_ok(void) { return s_last_save_ok; }

int gallery_add_patient(const char *name, const int8_t *embedding, int pill_count)
{
    s_last_save_ok = false;
    if (!name || !embedding) {
        return -1;
    }

    int slot = -1;
    for (int i = 0; i < MAX_PATIENTS; i++) {
        if (patient_gallery[i].valid &&
            strncmp(patient_gallery[i].name, name, PATIENT_NAME_MAX) == 0) {
            slot = i; /* re-enroll into the same slot */
            break;
        }
        if (slot < 0 && !patient_gallery[i].valid) {
            slot = i; /* first free slot, kept searching for a name match */
        }
    }
    if (slot < 0) {
        printf("gallery_add_patient: gallery full (max %d).\r\n", MAX_PATIENTS);
        return -1;
    }

    PatientRecord *rec = &patient_gallery[slot];
    memset(rec, 0, sizeof(*rec));
    rec->valid = 1;
    strncpy(rec->name, name, PATIENT_NAME_MAX - 1);
    memcpy(rec->embedding, embedding, sizeof(rec->embedding));
    /* The dose. Fixed for the life of the enrolment — see the comment on
     * PatientRecord in ai_vision.h for why there is no stock counter beside
     * it any more. */
    rec->pill_count = (uint8_t)(pill_count < 0 ? 0 : (pill_count > 255 ? 255 : pill_count));

    s_last_save_ok = gallery_save();
    if (!s_last_save_ok) {
        printf("gallery_add_patient: WARNING save to SD failed for slot %d.\r\n", slot);
    }

    return slot;
}

int gallery_find_best_match(const int8_t *embedding, float *out_confidence)
{
    if (out_confidence) *out_confidence = -1.0f;
    if (!embedding) return -1;

    int best_slot = -1;
    float best_sim = -1.0f;
    for (int i = 0; i < MAX_PATIENTS; i++) {
        if (!patient_gallery[i].valid) continue;
        float sim = ai_vision_match_face(embedding, patient_gallery[i].embedding);
        if (sim > best_sim) {
            best_sim = sim;
            best_slot = i;
        }
    }

    if (out_confidence) *out_confidence = best_sim;

    /* Session 12: report the similarity that was actually measured, and the
     * bar it had to clear.
     *
     * Session 08B set GALLERY_MATCH_THRESHOLD to 0.65 by copying the reference
     * project's constant and explicitly flagged it as "a starting point only,
     * tune against real enrolled-vs-impostor measurements" — but nothing ever
     * printed the number you would tune it against. A rejection said only
     * "intruder", which is indistinguishable between "a stranger, correctly
     * refused" (similarity nowhere near the bar) and "an enrolled patient the
     * threshold is too strict for" (similarity just under it). Those need
     * opposite fixes, and the first real hardware run of the finished flow
     * produced exactly that ambiguity: the same enrolled person matched on one
     * attempt and was rejected on the next.
     *
     * Printed as hundredths because this toolchain links nano.specs, whose
     * printf has no %f. Cosine similarity is [-1.0, 1.0], so this reads
     * -100..100. Confidence scores are explicitly loggable under
     * COMPLIANCE_PRIVACY_POSTURE.md — it is the embedding itself that must
     * never be printed, and this is a single scalar derived from it, not the
     * biometric data.
     *
     * One line per dispense attempt, never on a hot path. */
    printf("Gallery: best similarity %d/100, threshold %d/100 -> %s\r\n",
           (int)(best_sim * 100.0f),
           (int)(GALLERY_MATCH_THRESHOLD * 100.0f),
           (best_slot >= 0 && best_sim >= GALLERY_MATCH_THRESHOLD) ? "MATCH" : "no match");

    if (best_slot >= 0 && best_sim >= GALLERY_MATCH_THRESHOLD) {
        return best_slot;
    }
    return -1;
}

int gallery_count(void)
{
    int count = 0;
    for (int i = 0; i < MAX_PATIENTS; i++) {
        if (patient_gallery[i].valid) count++;
    }
    return count;
}

/* ══════════════════════════════════════════════════════════════════════════
 * NPU service task (Session 12)
 *
 * See ai_vision.h for why this exists. The short version: until Session 11
 * the NPU pipeline ran synchronously inside the UI task and never touched an
 * OS primitive, so the AI sat alongside µT-Kernel rather than being mediated
 * by it. It now runs in its own task, one priority level below the UI, and
 * the UI asks for a capture through a µT-Kernel event flag.
 *
 * Why an event flag and not a queue: the UI's wait has THREE outcomes it must
 * distinguish in a single blocking call — face found, no face, and "the AI
 * task never answered". An OR-wait on (DONE | FAIL) with a timeout expresses
 * exactly that in one tk_wai_flg(), where a queue would need either a second
 * object or a polling loop. The flag pattern that satisfied the wait tells
 * the caller which of the two arms fired; the timeout covers the third.
 *
 * There is no lock around the result buffers below and none is needed: the
 * flag handshake makes access strictly alternating. The UI writes nothing;
 * the AI task writes s_capture_* only between receiving REQUEST and setting
 * DONE/FAIL, and the UI reads them only after that flag arrives.
 * ══════════════════════════════════════════════════════════════════════════ */

/* Flag bits. Kept sparse and named so a UART-visible pattern is readable. */
#define AI_FLAG_REQUEST   (1u << 0)   /* UI -> AI: run a capture             */
#define AI_FLAG_DONE      (1u << 1)   /* AI -> UI: face found                */
#define AI_FLAG_FAIL      (1u << 2)   /* AI -> UI: ran, but found no face    */
/* AI -> everyone, latched, never cleared: ai_vision_init() has returned, so
 * the NPU runtime is done touching the activation arena — which overlaps the
 * LCD framebuffer at BUFFER_ADDRESS. See ai_vision_wait_init(). */
#define AI_FLAG_INIT_DONE (1u << 3)

/* Capture retry policy — moved here from state_machine.c's two copies of the
 * same loop (STATE_CAMERA_REGISTER and STATE_CAMERA_DISPENSE), unchanged in
 * behaviour: 3 attempts, 500 ms apart. Keeping it beside the pipeline means
 * one round trip per capture for the caller instead of three. */
#define AI_CAPTURE_ATTEMPTS   3
#define AI_CAPTURE_RETRY_MS   500u

static osal_flag_handle_t s_ai_flag = NULL;
static int8_t             s_capture_embedding[EMBEDDING_SIZE];

void ai_vision_service_init(void)
{
    s_ai_flag = osal_flag_create();
}

bool ai_vision_wait_init(uint32_t timeout_ms)
{
    if (s_ai_flag == NULL) {
        return false;
    }
    /* AND, and deliberately WITHOUT OSAL_FLAG_WAIT_CLEAR: this bit is a
     * latch that any number of callers may test, now or later, not a
     * one-shot handshake. */
    return osal_flag_wait(s_ai_flag, AI_FLAG_INIT_DONE,
                          OSAL_FLAG_WAIT_AND, NULL, timeout_ms);
}

bool ai_vision_is_ready(void)
{
    return s_ai_ready;
}

void ai_vision_capture_request(void)
{
    if (s_ai_flag == NULL) {
        return;
    }
    /* Drop any stale completion left by an abandoned request (e.g. a caller
     * that timed out and gave up) so this wait cannot be satisfied by the
     * previous capture's result. */
    osal_flag_clear(s_ai_flag, AI_FLAG_DONE | AI_FLAG_FAIL);
    osal_flag_set(s_ai_flag, AI_FLAG_REQUEST);
}

ai_capture_result_t ai_vision_capture_wait(int8_t *out_embedding,
                                            uint32_t timeout_ms)
{
    if ((s_ai_flag == NULL) || (out_embedding == NULL)) {
        return AI_CAPTURE_NOT_READY;
    }

    uint32_t got = 0u;
    bool ok = osal_flag_wait(s_ai_flag,
                             AI_FLAG_DONE | AI_FLAG_FAIL,
                             OSAL_FLAG_WAIT_OR | OSAL_FLAG_WAIT_CLEAR,
                             &got,
                             timeout_ms);
    if (!ok) {
        /* The AI task is still working and still owns BUFFER_ADDRESS. The
         * caller must NOT redraw yet — state_machine.c handles this by
         * waiting again rather than by pressing on. */
        return AI_CAPTURE_TIMEOUT;
    }

    if ((got & AI_FLAG_DONE) != 0u) {
        memcpy(out_embedding, s_capture_embedding, sizeof(s_capture_embedding));
        return AI_CAPTURE_OK;
    }
    return s_ai_ready ? AI_CAPTURE_NO_FACE : AI_CAPTURE_NOT_READY;
}

void task_ai_fn(void *arg)
{
    (void)arg;
    printf("task_ai: started.\r\n");

    /* One-time NPU bring-up (OSPI memory-mapped mode, PSRAM, both networks)
     * plus the SD-card gallery load. Moved here from task_ui_fn: the AI task
     * owns the NPU, so it should be the thing that brings it up, and doing it
     * here keeps several hundred milliseconds of blocking init off the UI
     * task's startup path. gallery_init()'s SD read is safe at this point
     * regardless of whether the logger task has mounted the card yet —
     * sd_logger.c mounts lazily on first use as of Session 12. */
    ai_vision_init();

    /* Latch "the NPU arena is yours again". task_ui blocks on this in
     * state_machine_init() before it draws anything — see ai_vision_wait_init().
     * Set unconditionally, including on the failure paths inside
     * ai_vision_init(): a UI that cannot do face recognition is still a UI,
     * and must not be held off the display forever by a dead NPU. */
    osal_flag_set(s_ai_flag, AI_FLAG_INIT_DONE);

    for (;;) {
        /* Block until the UI asks for a capture. TWF_ORW with BITCLR so the
         * request bit is consumed atomically and a second request posted
         * while we work is not silently merged into this one. */
        if (!osal_flag_wait(s_ai_flag, AI_FLAG_REQUEST,
                            OSAL_FLAG_WAIT_OR | OSAL_FLAG_WAIT_CLEAR,
                            NULL, OSAL_WAIT_FOREVER)) {
            /* Should not happen with an infinite timeout; back off rather
             * than spin if the object ever goes away. */
            osal_delay_ms(100u);
            continue;
        }

        if (!s_ai_ready) {
            osal_flag_set(s_ai_flag, AI_FLAG_FAIL);
            continue;
        }

        bool got_face = false;
        for (int attempt = 0; attempt < AI_CAPTURE_ATTEMPTS && !got_face; attempt++) {
            got_face = ai_vision_run_pipeline(s_capture_embedding);
            if (!got_face && (attempt + 1) < AI_CAPTURE_ATTEMPTS) {
                osal_delay_ms(AI_CAPTURE_RETRY_MS);
            }
        }

        osal_flag_set(s_ai_flag, got_face ? AI_FLAG_DONE : AI_FLAG_FAIL);
    }
}
