/**
  ******************************************************************************
  * @file    network.c
  * @author  AST Embedded Analytics Research Platform
  * @date    2026-08-31T13:18:09+0530
  * @brief   AI Tool Automatic Code Generator for Embedded NN computing
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  ******************************************************************************
  */

#include "ai_lite_inspect.h"
#include "ai_platform_interface.h"
#include "layers.h"
#include "core_convert.h"
#include "network.h"
#include "network_details.h"
#include "network_data.h"
#include "stai_events.h"

#include "lite_operators.h"

#include "ai_lite_inspect.h"
/*****************************************************************************/
#define STAI_INTERNAL_API_MAJOR               (1)
#define STAI_INTERNAL_API_MINOR               (0)
#define STAI_INTERNAL_API_MICRO               (0)

#define STAI_MAGIC                            (0xB1C00100)

/*****************************************************************************/
#define _STAI_CONCAT_ARG(a, b)     a ## b
#define STAI_CONCAT(a, b)         _STAI_CONCAT_ARG(a, b)

/*!  STAI_CAST SECTION                       *********************************/
#define STAI_CAST(type, expr) \
  ((type)(expr))


/*****************************************************************************/
#define STAI_SIZE(_size) \
  ((stai_size)(_size))

/*****************************************************************************/
#define STAI_INIT_BUFFER(_flags, _size, _address) \
  { \
    .size = (_size), \
    .address = (uintptr_t)(_address), \
    .flags = (_flags), \
  }

#define STAI_INIT_TENSOR(_name, _flags, _fmt, _size_bytes, _shape, _scale, _zeropoint) \
  { \
    .size_bytes = (_size_bytes), \
    .flags = (_flags), \
    .format = (stai_format)(_fmt), \
    .shape = STAI_PACK(_shape), \
    .scale = STAI_PACK(_scale), \
    .zeropoint = STAI_PACK(_zeropoint), \
    .name = (_name) \
  }

#define STAI_INIT_ARRAY(_size, _ptr) \
  { .size = STAI_SIZE(_size), .data = STAI_PACK(_ptr) }


#define STAI_CAST_ARRAY(_type, _size, _ptr) \
  { .size = STAI_SIZE(_size), .data = (_type)STAI_PACK(_ptr) }


#define STAI_DECLARE_ARRAY(_type, _size, ...) \
  { .size = STAI_SIZE(_size), .data = (_type[_size]) { STAI_PACK(__VA_ARGS__) } }


#define STAI_EMPTY_ARRAY() \
  { .size = 0, .data = NULL }


#define STAI_INIT_VERSION(_major, _minor, _micro) \
  { .major = (_major), .minor = (_minor), .micro = (_micro), .reserved = 0x0 }

/*****************************************************************************/
/**  Getters and setters  **/

#define STAI_GET_ARRAY_SIZE(nd_array) \
  (nd_array.size)


#define STAI_GET_ARRAY_ELEM(nd_array, pos) \
  (nd_array.data[(pos)])

#define _STAI_SET_ERROR(net_ctx, cond, value, exit) { \
  if (!(net_ctx)) { return STAI_ERROR_NETWORK_INVALID_CONTEXT_HANDLE; } \
  if (((uintptr_t)net_ctx) & (_STAI_CONTEXT_ALIGNMENT-1)) { return STAI_ERROR_NETWORK_INVALID_CONTEXT_ALIGNMENT; } \
  if (((value) >= STAI_ERROR_GENERIC) && (cond)) { \
    if ((net_ctx)->_return_code == STAI_SUCCESS) { \
      (net_ctx)->_return_code = (value); \
    } \
    return (exit); \
  } \
}

/*****************************************************************************/
/* TODO REMOVE THESE TWO MACROS */
#define STAI_EVENT_NODE_START_CB
#define STAI_EVENT_NODE_STOP_CB

#ifdef STAI_EVENT_NODE_START_CB
#ifndef _STAI_NETWORK_EVENT_NODE_START_CB
  #define _STAI_NETWORK_EVENT_NODE_START_CB(_node_id, _buffers_size, ...) \
  if (net_ctx->_callback) { \
    const stai_event_node_start_stop _start_event = { \
      .node_id=(_node_id), \
      .buffers={ \
        .size=(_buffers_size), \
        .data=(stai_ptr const*)(const stai_ptr[_buffers_size])STAI_PACK(__VA_ARGS__) \
      } \
    }; \
    net_ctx->_callback(net_ctx->_callback_cookie, STAI_EVENT_NODE_START, (const void*)&_start_event); \
  }
#endif
#else
  #define _STAI_NETWORK_EVENT_NODE_START_CB(_node_id, _buffers_size, ...) \
    do { /* _STAI_NETWORK_EVENT_NODE_START_CB() */ } while(0);
#endif      /* STAI_EVENT_NODE_START_CB */

#ifdef STAI_EVENT_NODE_STOP_CB
#ifndef _STAI_NETWORK_EVENT_NODE_STOP_CB
  #define _STAI_NETWORK_EVENT_NODE_STOP_CB(_node_id, _buffers_size, ...) \
  if (net_ctx->_callback) { \
    const stai_event_node_start_stop _stop_event = { \
      .node_id=(_node_id), \
      .buffers={ \
        .size=(_buffers_size), \
        .data=(stai_ptr const*)(stai_ptr[_buffers_size])STAI_PACK(__VA_ARGS__) \
      } \
    }; \
    net_ctx->_callback(net_ctx->_callback_cookie, STAI_EVENT_NODE_STOP, (const void*)&_stop_event); \
  }
#endif
#else
  #define _STAI_NETWORK_EVENT_NODE_STOP_CB(_node_id, _buffers_size, ...) \
    do { /* _STAI_NETWORK_EVENT_NODE_STOP_CB() */ } while(0);
#endif      /* STAI_EVENT_NODE_STOP_CB */


/*****************************************************************************/
#define _STAI_NETWORK_MODEL_SIGNATURE     "0xa04f89b9f721e246ea99ea60bccf606d"
#define _STAI_NETWORK_DATETIME            "2026-08-31T13:18:09+0530"
#define _STAI_NETWORK_COMPILE_DATETIME    __DATE__ " " __TIME__

#define _STAI_CONTEXT_ALIGNMENT        STAI_NETWORK_CONTEXT_ALIGNMENT

/*****************************************************************************/
#define g_network_activations_1     (NULL)




#if defined(HAVE_NETWORK_INFO)
/*****************************************************************************/
static const stai_network_info g_network_info = {
  .model_signature = _STAI_NETWORK_MODEL_SIGNATURE,
  .c_compile_datetime = _STAI_NETWORK_COMPILE_DATETIME,
  .c_model_name = STAI_NETWORK_MODEL_NAME,
  .c_model_datetime = _STAI_NETWORK_DATETIME,
  .c_model_signature = 0x0,
  .runtime_version = STAI_INIT_VERSION(12, 0, 1),
  .tool_version = STAI_INIT_VERSION(4, 0, 1),
  .api_version = STAI_INIT_VERSION(1, 0, 0),
  .n_macc = STAI_NETWORK_MACC_NUM,
  .n_nodes = STAI_NETWORK_NODES_NUM,
  .flags = STAI_NETWORK_FLAGS,
  .n_inputs = STAI_NETWORK_IN_NUM,
  .n_outputs = STAI_NETWORK_OUT_NUM,
  .n_activations = STAI_NETWORK_ACTIVATIONS_NUM,
  .n_weights = STAI_NETWORK_WEIGHTS_NUM,
  .n_states = STAI_NETWORK_STATES_NUM,
  .inputs = (stai_tensor[STAI_NETWORK_IN_NUM]) {
    STAI_INIT_TENSOR(
      STAI_NETWORK_IN_1_NAME,
      STAI_NETWORK_IN_1_FLAGS,
      STAI_NETWORK_IN_1_FORMAT,
      STAI_NETWORK_IN_1_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 4, 1, 160, 160, 3),
      STAI_DECLARE_ARRAY(float, 1, 0.003921568859368563f),
      STAI_DECLARE_ARRAY(int16_t, 1, -128)),
    },
    .outputs = (stai_tensor[STAI_NETWORK_OUT_NUM]) {
    STAI_INIT_TENSOR(
      STAI_NETWORK_OUT_1_NAME,
      STAI_NETWORK_OUT_1_FLAGS,
      STAI_NETWORK_OUT_1_FORMAT,
      STAI_NETWORK_OUT_1_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 4, 1, 10, 10, 2),
      STAI_DECLARE_ARRAY(float, 1, 0.00390625f),
      STAI_DECLARE_ARRAY(int16_t, 1, -128)),
    STAI_INIT_TENSOR(
      STAI_NETWORK_OUT_2_NAME,
      STAI_NETWORK_OUT_2_FLAGS,
      STAI_NETWORK_OUT_2_FORMAT,
      STAI_NETWORK_OUT_2_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 4, 1, 20, 20, 20),
      STAI_DECLARE_ARRAY(float, 1, 0.02451060339808464f),
      STAI_DECLARE_ARRAY(int16_t, 1, -5)),
    STAI_INIT_TENSOR(
      STAI_NETWORK_OUT_3_NAME,
      STAI_NETWORK_OUT_3_FLAGS,
      STAI_NETWORK_OUT_3_FORMAT,
      STAI_NETWORK_OUT_3_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 4, 1, 5, 5, 2),
      STAI_DECLARE_ARRAY(float, 1, 0.00390625f),
      STAI_DECLARE_ARRAY(int16_t, 1, -128)),
    STAI_INIT_TENSOR(
      STAI_NETWORK_OUT_4_NAME,
      STAI_NETWORK_OUT_4_FLAGS,
      STAI_NETWORK_OUT_4_FORMAT,
      STAI_NETWORK_OUT_4_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 4, 1, 10, 10, 20),
      STAI_DECLARE_ARRAY(float, 1, 0.023661937564611435f),
      STAI_DECLARE_ARRAY(int16_t, 1, -18)),
    STAI_INIT_TENSOR(
      STAI_NETWORK_OUT_5_NAME,
      STAI_NETWORK_OUT_5_FLAGS,
      STAI_NETWORK_OUT_5_FORMAT,
      STAI_NETWORK_OUT_5_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 4, 1, 10, 10, 8),
      STAI_DECLARE_ARRAY(float, 1, 0.01881924830377102f),
      STAI_DECLARE_ARRAY(int16_t, 1, -128)),
    STAI_INIT_TENSOR(
      STAI_NETWORK_OUT_6_NAME,
      STAI_NETWORK_OUT_6_FLAGS,
      STAI_NETWORK_OUT_6_FORMAT,
      STAI_NETWORK_OUT_6_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 4, 1, 5, 5, 8),
      STAI_DECLARE_ARRAY(float, 1, 0.015103490091860294f),
      STAI_DECLARE_ARRAY(int16_t, 1, -128)),
    STAI_INIT_TENSOR(
      STAI_NETWORK_OUT_7_NAME,
      STAI_NETWORK_OUT_7_FLAGS,
      STAI_NETWORK_OUT_7_FORMAT,
      STAI_NETWORK_OUT_7_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 4, 1, 5, 5, 20),
      STAI_DECLARE_ARRAY(float, 1, 0.01342423353344202f),
      STAI_DECLARE_ARRAY(int16_t, 1, -20)),
    STAI_INIT_TENSOR(
      STAI_NETWORK_OUT_8_NAME,
      STAI_NETWORK_OUT_8_FLAGS,
      STAI_NETWORK_OUT_8_FORMAT,
      STAI_NETWORK_OUT_8_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 4, 1, 20, 20, 8),
      STAI_DECLARE_ARRAY(float, 1, 0.019490458071231842f),
      STAI_DECLARE_ARRAY(int16_t, 1, -128)),
    STAI_INIT_TENSOR(
      STAI_NETWORK_OUT_9_NAME,
      STAI_NETWORK_OUT_9_FLAGS,
      STAI_NETWORK_OUT_9_FORMAT,
      STAI_NETWORK_OUT_9_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 4, 1, 20, 20, 2),
      STAI_DECLARE_ARRAY(float, 1, 0.00390625f),
      STAI_DECLARE_ARRAY(int16_t, 1, -128)),
    },
  .activations = (stai_tensor[STAI_NETWORK_ACTIVATIONS_NUM]) {
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_ACTIVATION_1_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_ACTIVATION_1_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 120560),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    },
  .weights = (stai_tensor[STAI_NETWORK_WEIGHTS_NUM]) {
    STAI_INIT_TENSOR(
      (NULL),
      STAI_NETWORK_WEIGHT_1_FLAGS,
      STAI_FORMAT_U8,
      STAI_NETWORK_WEIGHT_1_SIZE_BYTES,
      STAI_DECLARE_ARRAY(int32_t, 1, 642272),
      STAI_EMPTY_ARRAY(),
      STAI_EMPTY_ARRAY()),
    },

  .states = NULL
};
#endif

#define _STAI_CONTEXT_ACQUIRE(_net_ctx, _net_handle) \
  _stai_network_context* _net_ctx = (_stai_network_context*)(_net_handle); \
  STAI_ASSERT(_net_ctx != NULL) \
  _STAI_SET_ERROR(_net_ctx, _net_ctx->_magic != STAI_MAGIC, \
                  STAI_ERROR_NETWORK_INVALID_CONTEXT_HANDLE, _net_ctx->_return_code)


/*****************************************************************************/
static
void _stai_network_check(_stai_network_context* net_ctx)
{
  stai_size idx;

// Check activations status
  for (idx=0; idx<STAI_NETWORK_ACTIVATIONS_NUM; idx++) {
    if (net_ctx->_activations[idx] == NULL) break;
  }
  net_ctx->_flags |= (idx == STAI_NETWORK_ACTIVATIONS_NUM) ? STAI_FLAG_ACTIVATIONS : STAI_FLAG_NONE;
// Check inputs status
  for (idx=0; idx<STAI_NETWORK_IN_NUM; idx++) {
    if (net_ctx->_inputs[idx] == NULL) break;
  }
  net_ctx->_flags |= (idx == STAI_NETWORK_IN_NUM) ? STAI_FLAG_INPUTS : STAI_FLAG_NONE;

  // Check outputs status
  for (idx=0; idx<STAI_NETWORK_OUT_NUM; idx++) {
    if (net_ctx->_outputs[idx] == NULL) break;
  }
  net_ctx->_flags |= (idx == STAI_NETWORK_OUT_NUM) ? STAI_FLAG_OUTPUTS : STAI_FLAG_NONE;

// Check weights status
  for (idx=0; idx<STAI_NETWORK_WEIGHTS_NUM; idx++) {
    if (net_ctx->_weights[idx] == NULL) break;
  }
  net_ctx->_flags |= (idx == STAI_NETWORK_WEIGHTS_NUM) ? STAI_FLAG_WEIGHTS : STAI_FLAG_NONE;
STAI_PRINT("  [_stai_network_check] flags: 0x%08x\n", net_ctx->_flags)
}


/*****************************************************************************/
STAI_API_ENTRY
stai_return_code stai_network_init(
  stai_network* network)
{
  /* Memory where to store internal context is provided by applications as a raw byte buffer */
  _stai_network_context* net_ctx = (_stai_network_context*)(network);
  net_ctx->_return_code = STAI_SUCCESS;
  STAI_PRINT("[Entering Network Init] network(%p) context_size(%d)\n", net_ctx, (int32_t)sizeof(_stai_network_context))

  _STAI_SET_ERROR(net_ctx, STAI_NETWORK_CONTEXT_SIZE != sizeof(_stai_network_context),
                 STAI_ERROR_NETWORK_INVALID_CONTEXT_SIZE, net_ctx->_return_code)

  {
    const _stai_network_context _network_context = {
      ._magic = STAI_MAGIC,
      ._signature = STAI_NETWORK_MODEL_SIGNATURE,
      ._flags = STAI_NETWORK_FLAGS,
      ._return_code = STAI_SUCCESS,
      ._callback = NULL,
      ._callback_cookie = NULL,
      ._activations = {
      (stai_ptr)g_network_activations_1
      },
      ._weights = {
      (stai_ptr)g_network_weights_array
      },
      ._inputs = {
    NULL},
      ._outputs = {
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL},
    };

    // Deep copy of internal context to opaque buffer provided by app
    *net_ctx = _network_context;

    _stai_network_check(net_ctx);
  }

  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_deinit(
  stai_network* network)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)

  /*  Reset flags to initial state  */
  net_ctx->_flags = STAI_NETWORK_FLAGS;
  return net_ctx->_return_code;
}

/*****************************************************************************/



/* Int quant #0 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_34_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.0007190321339294314f),
    AI_PACK_INTQ_ZP(20)))

/* Int quant #1 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(resize_35_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.0007190321339294314f),
    AI_PACK_INTQ_ZP(20)))

/* Int quant #2 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_37_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.0016279983101412654f),
    AI_PACK_INTQ_ZP(5)))

/* Int quant #3 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(eltwise_38_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.0016279983101412654f),
    AI_PACK_INTQ_ZP(5)))

/* Int quant #4 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(resize_39_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.0016279983101412654f),
    AI_PACK_INTQ_ZP(5)))

/* Int quant #5 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_41_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.0027904396411031485f),
    AI_PACK_INTQ_ZP(-19)))

/* Int quant #6 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(eltwise_42_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.0027904396411031485f),
    AI_PACK_INTQ_ZP(-19)))

/* Int quant #7 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_48_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.0390465185046196f),
    AI_PACK_INTQ_ZP(92)))

/* Int quant #8 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(nl_49_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.00390625f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #9 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_40_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.0009802478598430753f),
    AI_PACK_INTQ_ZP(1)))

/* Int quant #10 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_56_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.0006985461805015802f),
    AI_PACK_INTQ_ZP(-17)))

/* Int quant #11 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(eltwise_57_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.0009802478598430753f),
    AI_PACK_INTQ_ZP(1)))

/* Int quant #12 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_78_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.036569274961948395f),
    AI_PACK_INTQ_ZP(61)))

/* Int quant #13 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(nl_79_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.00390625f),
    AI_PACK_INTQ_ZP(-128)))

/* Int quant #14 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_36_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.00038219871930778027f),
    AI_PACK_INTQ_ZP(18)))

/* Int quant #15 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_59_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.0002102396683767438f),
    AI_PACK_INTQ_ZP(25)))

/* Int quant #16 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(eltwise_60_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.00038219871930778027f),
    AI_PACK_INTQ_ZP(18)))

/* Int quant #17 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(conv2d_66_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.02307526022195816f),
    AI_PACK_INTQ_ZP(127)))

/* Int quant #18 */
AI_INTQ_INFO_LIST_OBJ_DECLARE(nl_67_output_array_intq, AI_STATIC,
  AI_BUFFER_META_FLAG_SCALE_FLOAT|AI_BUFFER_META_FLAG_ZEROPOINT_S8, 1,
  AI_PACK_INTQ_INFO(
    AI_PACK_INTQ_SCALE(0.00390625f),
    AI_PACK_INTQ_ZP(-128)))



/* Array#0 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_34_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 400, AI_STATIC)

/* Array#1 */
AI_ARRAY_OBJ_DECLARE(
  resize_35_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 1600, AI_STATIC)

/* Array#2 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_37_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 1600, AI_STATIC)

/* Array#3 */
AI_ARRAY_OBJ_DECLARE(
  eltwise_38_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 1600, AI_STATIC)

/* Array#4 */
AI_ARRAY_OBJ_DECLARE(
  resize_39_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 6400, AI_STATIC)

/* Array#5 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_41_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 6400, AI_STATIC)

/* Array#6 */
AI_ARRAY_OBJ_DECLARE(
  eltwise_42_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 6400, AI_STATIC)

/* Array#7 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_48_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 800, AI_STATIC)

/* Array#8 */
AI_ARRAY_OBJ_DECLARE(
  nl_49_output_array, AI_ARRAY_FORMAT_S8|AI_FMT_FLAG_IS_IO,
  NULL, NULL, 800, AI_STATIC)

/* Array#9 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_40_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 1600, AI_STATIC)

/* Array#10 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_56_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 1600, AI_STATIC)

/* Array#11 */
AI_ARRAY_OBJ_DECLARE(
  eltwise_57_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 1600, AI_STATIC)

/* Array#12 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_78_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 200, AI_STATIC)

/* Array#13 */
AI_ARRAY_OBJ_DECLARE(
  nl_79_output_array, AI_ARRAY_FORMAT_S8|AI_FMT_FLAG_IS_IO,
  NULL, NULL, 200, AI_STATIC)

/* Array#14 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_36_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 400, AI_STATIC)

/* Array#15 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_59_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 400, AI_STATIC)

/* Array#16 */
AI_ARRAY_OBJ_DECLARE(
  eltwise_60_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 400, AI_STATIC)

/* Array#17 */
AI_ARRAY_OBJ_DECLARE(
  conv2d_66_output_array, AI_ARRAY_FORMAT_S8,
  NULL, NULL, 50, AI_STATIC)

/* Array#18 */
AI_ARRAY_OBJ_DECLARE(
  nl_67_output_array, AI_ARRAY_FORMAT_S8|AI_FMT_FLAG_IS_IO,
  NULL, NULL, 50, AI_STATIC)



/* Tensor #0 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_34_output, AI_STATIC,
  109, 0x1,
  AI_SHAPE_INIT(4, 1, 16, 5, 5), AI_STRIDE_INIT(4, 1, 1, 16, 80),
  1, &conv2d_34_output_array, &conv2d_34_output_array_intq)

/* Tensor #1 */
AI_TENSOR_OBJ_DECLARE(
  resize_35_output, AI_STATIC,
  283, 0x1,
  AI_SHAPE_INIT(4, 1, 16, 10, 10), AI_STRIDE_INIT(4, 1, 1, 16, 160),
  1, &resize_35_output_array, &resize_35_output_array_intq)

/* Tensor #2 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_37_output, AI_STATIC,
  118, 0x1,
  AI_SHAPE_INIT(4, 1, 16, 10, 10), AI_STRIDE_INIT(4, 1, 1, 16, 160),
  1, &conv2d_37_output_array, &conv2d_37_output_array_intq)

/* Tensor #3 */
AI_TENSOR_OBJ_DECLARE(
  eltwise_38_output, AI_STATIC,
  274, 0x1,
  AI_SHAPE_INIT(4, 1, 16, 10, 10), AI_STRIDE_INIT(4, 1, 1, 16, 160),
  1, &eltwise_38_output_array, &eltwise_38_output_array_intq)

/* Tensor #4 */
AI_TENSOR_OBJ_DECLARE(
  resize_39_output, AI_STATIC,
  284, 0x1,
  AI_SHAPE_INIT(4, 1, 16, 20, 20), AI_STRIDE_INIT(4, 1, 1, 16, 320),
  1, &resize_39_output_array, &resize_39_output_array_intq)

/* Tensor #5 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_41_output, AI_STATIC,
  131, 0x1,
  AI_SHAPE_INIT(4, 1, 16, 20, 20), AI_STRIDE_INIT(4, 1, 1, 16, 320),
  1, &conv2d_41_output_array, &conv2d_41_output_array_intq)

/* Tensor #6 */
AI_TENSOR_OBJ_DECLARE(
  eltwise_42_output, AI_STATIC,
  275, 0x1,
  AI_SHAPE_INIT(4, 1, 16, 20, 20), AI_STRIDE_INIT(4, 1, 1, 16, 320),
  1, &eltwise_42_output_array, &eltwise_42_output_array_intq)

/* Tensor #7 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_48_output, AI_STATIC,
  158, 0x1,
  AI_SHAPE_INIT(4, 1, 2, 20, 20), AI_STRIDE_INIT(4, 1, 1, 2, 40),
  1, &conv2d_48_output_array, &conv2d_48_output_array_intq)

/* Tensor #8 */
AI_TENSOR_OBJ_DECLARE(
  nl_49_output, AI_STATIC,
  278, 0x1,
  AI_SHAPE_INIT(4, 1, 2, 20, 20), AI_STRIDE_INIT(4, 1, 1, 2, 40),
  1, &nl_49_output_array, &nl_49_output_array_intq)

/* Tensor #9 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_40_output, AI_STATIC,
  126, 0x1,
  AI_SHAPE_INIT(4, 1, 16, 10, 10), AI_STRIDE_INIT(4, 1, 1, 16, 160),
  1, &conv2d_40_output_array, &conv2d_40_output_array_intq)

/* Tensor #10 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_56_output, AI_STATIC,
  173, 0x1,
  AI_SHAPE_INIT(4, 1, 16, 10, 10), AI_STRIDE_INIT(4, 1, 1, 16, 160),
  1, &conv2d_56_output_array, &conv2d_56_output_array_intq)

/* Tensor #11 */
AI_TENSOR_OBJ_DECLARE(
  eltwise_57_output, AI_STATIC,
  276, 0x1,
  AI_SHAPE_INIT(4, 1, 16, 10, 10), AI_STRIDE_INIT(4, 1, 1, 16, 160),
  1, &eltwise_57_output_array, &eltwise_57_output_array_intq)

/* Tensor #12 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_78_output, AI_STATIC,
  251, 0x1,
  AI_SHAPE_INIT(4, 1, 2, 10, 10), AI_STRIDE_INIT(4, 1, 1, 2, 20),
  1, &conv2d_78_output_array, &conv2d_78_output_array_intq)

/* Tensor #13 */
AI_TENSOR_OBJ_DECLARE(
  nl_79_output, AI_STATIC,
  280, 0x1,
  AI_SHAPE_INIT(4, 1, 2, 10, 10), AI_STRIDE_INIT(4, 1, 1, 2, 20),
  1, &nl_79_output_array, &nl_79_output_array_intq)

/* Tensor #14 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_36_output, AI_STATIC,
  113, 0x1,
  AI_SHAPE_INIT(4, 1, 16, 5, 5), AI_STRIDE_INIT(4, 1, 1, 16, 80),
  1, &conv2d_36_output_array, &conv2d_36_output_array_intq)

/* Tensor #15 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_59_output, AI_STATIC,
  177, 0x1,
  AI_SHAPE_INIT(4, 1, 16, 5, 5), AI_STRIDE_INIT(4, 1, 1, 16, 80),
  1, &conv2d_59_output_array, &conv2d_59_output_array_intq)

/* Tensor #16 */
AI_TENSOR_OBJ_DECLARE(
  eltwise_60_output, AI_STATIC,
  277, 0x1,
  AI_SHAPE_INIT(4, 1, 16, 5, 5), AI_STRIDE_INIT(4, 1, 1, 16, 80),
  1, &eltwise_60_output_array, &eltwise_60_output_array_intq)

/* Tensor #17 */
AI_TENSOR_OBJ_DECLARE(
  conv2d_66_output, AI_STATIC,
  209, 0x1,
  AI_SHAPE_INIT(4, 1, 2, 5, 5), AI_STRIDE_INIT(4, 1, 1, 2, 10),
  1, &conv2d_66_output_array, &conv2d_66_output_array_intq)

/* Tensor #18 */
AI_TENSOR_OBJ_DECLARE(
  nl_67_output, AI_STATIC,
  279, 0x1,
  AI_SHAPE_INIT(4, 1, 2, 5, 5), AI_STRIDE_INIT(4, 1, 1, 2, 10),
  1, &nl_67_output_array, &nl_67_output_array_intq)



AI_STATIC_CONST ai_float resize_35_scales_data[] = { 2.0, 2.0, 1.0, 1.0 };
AI_ARRAY_OBJ_DECLARE(
    resize_35_scales, AI_ARRAY_FORMAT_FLOAT,
    resize_35_scales_data, resize_35_scales_data, 4, AI_STATIC_CONST)
AI_TENSOR_CHAIN_OBJ_DECLARE(
  resize_35_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &conv2d_34_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &resize_35_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  resize_35_layer, 35,
  UPSAMPLE_TYPE, 0x0, NULL,
  upsample, forward_upsample_nearest,
  &resize_35_chain,
  NULL, &resize_35_layer, AI_STATIC, 
  .scales = &resize_35_scales, 
  .center = false, 
  .mode = AI_UPSAMPLE_NEAREST, 
  .nearest_mode = AI_ROUND_FLOOR, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  eltwise_38_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &conv2d_37_output, &resize_35_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &eltwise_38_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  eltwise_38_layer, 38,
  ELTWISE_INTEGER_TYPE, 0x0, NULL,
  eltwise_integer, forward_eltwise_integer_INT8,
  &eltwise_38_chain,
  NULL, &eltwise_38_layer, AI_STATIC, 
  .operation = ai_sum_f32, 
  .buffer_operation = ai_sum_buffer_INT8, 
)


AI_STATIC_CONST ai_float resize_39_scales_data[] = { 2.0, 2.0, 1.0, 1.0 };
AI_ARRAY_OBJ_DECLARE(
    resize_39_scales, AI_ARRAY_FORMAT_FLOAT,
    resize_39_scales_data, resize_39_scales_data, 4, AI_STATIC_CONST)
AI_TENSOR_CHAIN_OBJ_DECLARE(
  resize_39_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &eltwise_38_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &resize_39_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  resize_39_layer, 39,
  UPSAMPLE_TYPE, 0x0, NULL,
  upsample, forward_upsample_nearest,
  &resize_39_chain,
  NULL, &resize_39_layer, AI_STATIC, 
  .scales = &resize_39_scales, 
  .center = false, 
  .mode = AI_UPSAMPLE_NEAREST, 
  .nearest_mode = AI_ROUND_FLOOR, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  eltwise_42_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &conv2d_41_output, &resize_39_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &eltwise_42_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  eltwise_42_layer, 42,
  ELTWISE_INTEGER_TYPE, 0x0, NULL,
  eltwise_integer, forward_eltwise_integer_INT8,
  &eltwise_42_chain,
  NULL, &eltwise_42_layer, AI_STATIC, 
  .operation = ai_sum_f32, 
  .buffer_operation = ai_sum_buffer_INT8, 
)


AI_STATIC_CONST ai_i8 nl_49_nl_params_data[] = { -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -125, -125, -125, -125, -125, -125, -125, -125, -125, -124, -124, -124, -124, -124, -124, -123, -123, -123, -123, -123, -123, -122, -122, -122, -122, -121, -121, -121, -121, -120, -120, -120, -119, -119, -119, -118, -118, -118, -117, -117, -116, -116, -115, -115, -115, -114, -113, -113, -112, -112, -111, -111, -110, -109, -109, -108, -107, -106, -106, -105, -104, -103, -102, -101, -100, -99, -98, -97, -96, -95, -94, -93, -92, -90, -89, -88, -86, -85, -84, -82, -81, -79, -78, -76, -74, -73, -71, -69, -67, -66, -64, -62, -60, -58, -56, -54, -52, -50, -48, -45, -43, -41, -39, -36, -34, -32, -29, -27, -25, -22, -20, -17, -15, -12, -10, -7, -5, -2, 0, 2, 5, 7, 10, 12, 15, 17, 20, 22, 25, 27, 29, 32, 34, 36, 39, 41, 43, 45, 48, 50, 52, 54, 56, 58, 60, 62, 64, 66, 67, 69, 71, 73, 74, 76 };
AI_ARRAY_OBJ_DECLARE(
    nl_49_nl_params, AI_ARRAY_FORMAT_S8,
    nl_49_nl_params_data, nl_49_nl_params_data, 256, AI_STATIC_CONST)
AI_TENSOR_CHAIN_OBJ_DECLARE(
  nl_49_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &conv2d_48_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &nl_49_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  nl_49_layer, 49,
  NL_TYPE, 0x0, NULL,
  nl, forward_nl_integer,
  &nl_49_chain,
  NULL, &nl_49_layer, AI_STATIC, 
  .nl_params = &nl_49_nl_params, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  eltwise_57_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &conv2d_40_output, &conv2d_56_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &eltwise_57_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  eltwise_57_layer, 57,
  ELTWISE_INTEGER_TYPE, 0x0, NULL,
  eltwise_integer, forward_eltwise_integer_INT8,
  &eltwise_57_chain,
  NULL, &eltwise_57_layer, AI_STATIC, 
  .operation = ai_sum_f32, 
  .buffer_operation = ai_sum_buffer_INT8, 
)


AI_STATIC_CONST ai_i8 nl_79_nl_params_data[] = { -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -128, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -125, -125, -125, -125, -125, -125, -125, -125, -125, -124, -124, -124, -124, -124, -124, -124, -123, -123, -123, -123, -123, -123, -122, -122, -122, -122, -122, -121, -121, -121, -121, -120, -120, -120, -119, -119, -119, -118, -118, -118, -117, -117, -117, -116, -116, -115, -115, -115, -114, -114, -113, -113, -112, -111, -111, -110, -110, -109, -108, -108, -107, -106, -106, -105, -104, -103, -102, -101, -101, -100, -99, -98, -97, -96, -95, -94, -93, -91, -90, -89, -88, -87, -85, -84, -83, -81, -80, -78, -77, -75, -74, -72, -71, -69, -67, -66, -64, -62, -60, -59, -57, -55, -53, -51, -49, -47, -45, -43, -41, -39, -36, -34, -32, -30, -28, -25, -23, -21, -19, -16, -14, -12, -9, -7, -5, -2, 0, 2, 5, 7, 9, 12, 14, 16, 19, 21, 23, 25, 28, 30, 32, 34, 36, 39, 41, 43, 45, 47, 49, 51, 53, 55, 57, 59, 60, 62, 64, 66, 67, 69, 71, 72, 74, 75, 77, 78, 80, 81, 83, 84, 85, 87, 88, 89, 90, 91, 93, 94, 95, 96, 97, 98, 99, 100, 101, 101, 102, 103, 104, 105, 106, 106, 107 };
AI_ARRAY_OBJ_DECLARE(
    nl_79_nl_params, AI_ARRAY_FORMAT_S8,
    nl_79_nl_params_data, nl_79_nl_params_data, 256, AI_STATIC_CONST)
AI_TENSOR_CHAIN_OBJ_DECLARE(
  nl_79_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &conv2d_78_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &nl_79_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  nl_79_layer, 79,
  NL_TYPE, 0x0, NULL,
  nl, forward_nl_integer,
  &nl_79_chain,
  NULL, &nl_79_layer, AI_STATIC, 
  .nl_params = &nl_79_nl_params, 
)

AI_TENSOR_CHAIN_OBJ_DECLARE(
  eltwise_60_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 2, &conv2d_36_output, &conv2d_59_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &eltwise_60_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  eltwise_60_layer, 60,
  ELTWISE_INTEGER_TYPE, 0x0, NULL,
  eltwise_integer, forward_eltwise_integer_INT8,
  &eltwise_60_chain,
  NULL, &eltwise_60_layer, AI_STATIC, 
  .operation = ai_sum_f32, 
  .buffer_operation = ai_sum_buffer_INT8, 
)


AI_STATIC_CONST ai_i8 nl_67_nl_params_data[] = { -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -127, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -126, -125, -125, -125, -125, -125, -125, -125, -125, -125, -125, -125, -125, -125, -125, -125, -124, -124, -124, -124, -124, -124, -124, -124, -124, -124, -124, -123, -123, -123, -123, -123, -123, -123, -123, -123, -122, -122, -122, -122, -122, -122, -122, -121, -121, -121, -121, -121, -121, -121, -120, -120, -120, -120, -120, -119, -119, -119, -119, -119, -118, -118, -118, -118, -118, -117, -117, -117, -117, -116, -116, -116, -116, -115, -115, -115, -114, -114, -114, -114, -113, -113, -113, -112, -112, -112, -111, -111, -110, -110, -110, -109, -109, -108, -108, -108, -107, -107, -106, -106, -105, -105, -104, -104, -103, -103, -102, -102, -101, -101, -100, -99, -99, -98, -98, -97, -96, -96, -95, -94, -94, -93, -92, -92, -91, -90, -89, -89, -88, -87, -86, -86, -85, -84, -83, -82, -81, -80, -80, -79, -78, -77, -76, -75, -74, -73, -72, -71, -70, -69, -68, -67, -66, -64, -63, -62, -61, -60, -59, -58, -56, -55, -54, -53, -52, -50, -49, -48, -47, -45, -44, -43, -41, -40, -39, -37, -36, -35, -33, -32, -30, -29, -28, -26, -25, -23, -22, -20, -19, -18, -16, -15, -13, -12, -10, -9, -7, -6, -4, -3, -1, 0 };
AI_ARRAY_OBJ_DECLARE(
    nl_67_nl_params, AI_ARRAY_FORMAT_S8,
    nl_67_nl_params_data, nl_67_nl_params_data, 256, AI_STATIC_CONST)
AI_TENSOR_CHAIN_OBJ_DECLARE(
  nl_67_chain, AI_STATIC_CONST, 4,
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &conv2d_66_output),
  AI_TENSOR_LIST_OBJ_INIT(AI_FLAG_NONE, 1, &nl_67_output),
  AI_TENSOR_LIST_OBJ_EMPTY,
  AI_TENSOR_LIST_OBJ_EMPTY
)

AI_LAYER_OBJ_DECLARE(
  nl_67_layer, 67,
  NL_TYPE, 0x0, NULL,
  nl, forward_nl_integer,
  &nl_67_chain,
  NULL, &nl_67_layer, AI_STATIC, 
  .nl_params = &nl_67_nl_params, 
)
/**  Hybrid layers declarations section  *************************************/
void forward_lite_upsample_nearest_resize_35(_stai_network_context* net_ctx)
{
  conv2d_34_output_array.data = AI_PTR(net_ctx->_activations[0] + 128);
  conv2d_34_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 128);
  resize_35_output_array.data = AI_PTR(net_ctx->_activations[0] + 45792);
  resize_35_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 45792);
  _STAI_NETWORK_EVENT_NODE_START_CB(35, 1, { conv2d_34_output.data->data});
  forward_upsample_nearest(&resize_35_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(35, 1, { resize_35_output.data->data});
}
void forward_lite_eltwise_integer_INT8_eltwise_38(_stai_network_context* net_ctx)
{
  conv2d_37_output_array.data = AI_PTR(net_ctx->_activations[0] + 47392);
  conv2d_37_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 47392);
  resize_35_output_array.data = AI_PTR(net_ctx->_activations[0] + 45792);
  resize_35_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 45792);
  eltwise_38_output_array.data = AI_PTR(net_ctx->_activations[0] + 30560);
  eltwise_38_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 30560);
  _STAI_NETWORK_EVENT_NODE_START_CB(38, 2, { conv2d_37_output.data->data,resize_35_output.data->data});
  forward_eltwise_integer_INT8(&eltwise_38_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(38, 1, { eltwise_38_output.data->data});
}
void forward_lite_upsample_nearest_resize_39(_stai_network_context* net_ctx)
{
  eltwise_38_output_array.data = AI_PTR(net_ctx->_activations[0] + 30560);
  eltwise_38_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 30560);
  resize_39_output_array.data = AI_PTR(net_ctx->_activations[0] + 36064);
  resize_39_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 36064);
  _STAI_NETWORK_EVENT_NODE_START_CB(39, 1, { eltwise_38_output.data->data});
  forward_upsample_nearest(&resize_39_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(39, 1, { resize_39_output.data->data});
}
void forward_lite_eltwise_integer_INT8_eltwise_42(_stai_network_context* net_ctx)
{
  conv2d_41_output_array.data = AI_PTR(net_ctx->_activations[0] + 42464);
  conv2d_41_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 42464);
  resize_39_output_array.data = AI_PTR(net_ctx->_activations[0] + 36064);
  resize_39_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 36064);
  eltwise_42_output_array.data = AI_PTR(net_ctx->_activations[0] + 0);
  eltwise_42_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 0);
  _STAI_NETWORK_EVENT_NODE_START_CB(42, 2, { conv2d_41_output.data->data,resize_39_output.data->data});
  forward_eltwise_integer_INT8(&eltwise_42_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(42, 1, { eltwise_42_output.data->data});
}
void forward_lite_nl_integer_nl_49(_stai_network_context* net_ctx)
{
  conv2d_48_output_array.data = AI_PTR(net_ctx->_activations[0] + 2320);
  conv2d_48_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 2320);
  nl_49_output_array.data = AI_PTR(net_ctx->_outputs[8] + 0);
  nl_49_output_array.data_start = AI_PTR(net_ctx->_outputs[8] + 0);
  _STAI_NETWORK_EVENT_NODE_START_CB(49, 1, { conv2d_48_output.data->data});
  forward_nl_integer(&nl_49_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(49, 1, { nl_49_output.data->data});
}
void forward_lite_eltwise_integer_INT8_eltwise_57(_stai_network_context* net_ctx)
{
  conv2d_40_output_array.data = AI_PTR(net_ctx->_activations[0] + 34464);
  conv2d_40_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 34464);
  conv2d_56_output_array.data = AI_PTR(net_ctx->_activations[0] + 1504);
  conv2d_56_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 1504);
  eltwise_57_output_array.data = AI_PTR(net_ctx->_activations[0] + 6368);
  eltwise_57_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 6368);
  _STAI_NETWORK_EVENT_NODE_START_CB(57, 2, { conv2d_40_output.data->data,conv2d_56_output.data->data});
  forward_eltwise_integer_INT8(&eltwise_57_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(57, 1, { eltwise_57_output.data->data});
}
void forward_lite_nl_integer_nl_79(_stai_network_context* net_ctx)
{
  conv2d_78_output_array.data = AI_PTR(net_ctx->_activations[0] + 14368);
  conv2d_78_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 14368);
  nl_79_output_array.data = AI_PTR(net_ctx->_outputs[0] + 0);
  nl_79_output_array.data_start = AI_PTR(net_ctx->_outputs[0] + 0);
  _STAI_NETWORK_EVENT_NODE_START_CB(79, 1, { conv2d_78_output.data->data});
  forward_nl_integer(&nl_79_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(79, 1, { nl_79_output.data->data});
}
void forward_lite_eltwise_integer_INT8_eltwise_60(_stai_network_context* net_ctx)
{
  conv2d_36_output_array.data = AI_PTR(net_ctx->_activations[0] + 30160);
  conv2d_36_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 30160);
  conv2d_59_output_array.data = AI_PTR(net_ctx->_activations[0] + 2504);
  conv2d_59_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 2504);
  eltwise_60_output_array.data = AI_PTR(net_ctx->_activations[0] + 1800);
  eltwise_60_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 1800);
  _STAI_NETWORK_EVENT_NODE_START_CB(60, 2, { conv2d_36_output.data->data,conv2d_59_output.data->data});
  forward_eltwise_integer_INT8(&eltwise_60_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(60, 1, { eltwise_60_output.data->data});
}
void forward_lite_nl_integer_nl_67(_stai_network_context* net_ctx)
{
  conv2d_66_output_array.data = AI_PTR(net_ctx->_activations[0] + 1800);
  conv2d_66_output_array.data_start = AI_PTR(net_ctx->_activations[0] + 1800);
  nl_67_output_array.data = AI_PTR(net_ctx->_outputs[2] + 0);
  nl_67_output_array.data_start = AI_PTR(net_ctx->_outputs[2] + 0);
  _STAI_NETWORK_EVENT_NODE_START_CB(67, 1, { conv2d_66_output.data->data});
  forward_nl_integer(&nl_67_layer);
  _STAI_NETWORK_EVENT_NODE_STOP_CB(67, 1, { nl_67_output.data->data});
}

/*****************************************************************************/


static const ai_u16 conv2d_1_t_in_0_shape_w_const_u16 = 160;
static const ai_u16 conv2d_1_t_out_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_1_t_weight_0_shape_w_const_u16 = 3;
static const ai_i32 conv2d_1_l_pad_W_0_const_s32 = 1;
static const ai_u16 conv2d_1_l_stride_0_const_u16 = 2;
static const ai_i8 conv2d_1_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_1_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_1_t_in_0_fmt_scale_const_f32 = 0.003921568859368563f;
static const ai_float conv2d_1_t_out_0_fmt_scale_const_f32 = 0.004335746634751558f;
static const ai_float conv2d_1_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.00032090963213704526f, 0.0010118549689650536f, 1.0446107978623331e-08f, 0.0016259559197351336f, 0.0012074647238478065f, 0.00024298575590364635f, 0.0015029531205073f, 0.00022353691747412086f, 0.0013117155758664012f, 0.0011690674582496285f, 0.001566592836752534f, 0.0004916933830827475f, 0.0006598795298486948f, 0.0013138118665665388f, 0.0007194540812633932f, 1.2276839100877623e-08f);
static const ai_layer_format_type conv2d_1_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_1_t_out_0_shape_w_const_u16 = 80;

static const ai_i8 conv2d_2_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_2_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_2_pad_before_t_in_0_shape_h_const_u32 = 80;

static const ai_u16 conv2d_2_t_in_0_shape_w_const_u16 = 82;
static const ai_u16 conv2d_2_t_in_0_shape_h_const_u16 = 82;
static const ai_u16 conv2d_2_t_in_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_2_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_2_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_2_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_2_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_2_t_in_0_fmt_scale_const_f32 = 0.004335746634751558f;
static const ai_float conv2d_2_t_out_0_fmt_scale_const_f32 = 0.013141044415533543f;
static const ai_float conv2d_2_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.05896664038300514f, 0.048926450312137604f, 3.937008052901092e-09f, 0.01933022029697895f, 0.002790115773677826f, 0.029011836275458336f, 0.015307598747313023f, 0.08290638774633408f, 0.04099096730351448f, 0.03898725286126137f, 0.022466305643320084f, 0.014599230140447617f, 0.059187233448028564f, 0.009069109335541725f, 0.008982636034488678f, 4.019468757832101e-09f);
static const ai_u16 conv2d_2_t_out_0_shape_w_const_u16 = 80;
static const ai_u16 conv2d_2_t_out_0_shape_h_const_u16 = 80;

static const ai_u16 conv2d_3_t_in_0_shape_w_const_u16 = 80;
static const ai_u16 conv2d_3_t_in_0_shape_h_const_u16 = 80;
static const ai_u16 conv2d_3_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_3_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_3_t_in_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_3_t_out_0_shape_ch_const_u16 = 16;
static const ai_i8 conv2d_3_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_3_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_3_t_in_0_fmt_scale_const_f32 = 0.013141044415533543f;
static const ai_float conv2d_3_t_out_0_fmt_scale_const_f32 = 0.00846110749989748f;
static const ai_float conv2d_3_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.015887092798948288f, 0.0062223379500210285f, 0.004514268599450588f, 0.005081856157630682f, 0.0013951611472293735f, 0.006284276954829693f, 0.0009253659518435597f, 0.0032089066226035357f, 0.006772555410861969f, 0.004293143283575773f, 0.006696009077131748f, 0.005918031558394432f, 0.006636421196162701f, 0.0010908411350101233f, 0.0031200391240417957f, 0.004225643817335367f);
static const ai_layer_format_type conv2d_3_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_5_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_5_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_5_pad_before_t_in_0_shape_h_const_u32 = 80;

static const ai_u16 conv2d_5_t_in_0_shape_w_const_u16 = 82;
static const ai_u16 conv2d_5_t_in_0_shape_h_const_u16 = 82;
static const ai_u16 conv2d_5_t_in_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_5_l_stride_1_const_u16 = 2;
static const ai_u16 conv2d_5_l_stride_0_const_u16 = 2;
static const ai_i8 conv2d_5_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_5_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_5_t_in_0_fmt_scale_const_f32 = 0.00846110749989748f;
static const ai_float conv2d_5_t_out_0_fmt_scale_const_f32 = 0.005124824121594429f;
static const ai_float conv2d_5_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.002985820872709155f, 0.003367518074810505f, 0.00615567434579134f, 0.00311306887306273f, 0.010586417280137539f, 0.002150422427803278f, 0.022628311067819595f, 0.0036224101204425097f, 0.003962444607168436f, 0.005086956545710564f, 0.002086834516376257f, 0.0027549099177122116f, 0.002798893256112933f, 0.011348981410264969f, 0.0073872460052371025f, 0.003488442162051797f);
static const ai_u16 conv2d_5_t_out_0_shape_w_const_u16 = 40;
static const ai_u16 conv2d_5_t_out_0_shape_h_const_u16 = 40;

static const ai_u16 conv2d_6_t_in_0_shape_w_const_u16 = 40;
static const ai_u16 conv2d_6_t_in_0_shape_h_const_u16 = 40;
static const ai_u16 conv2d_6_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_6_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_6_t_in_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_6_t_out_0_shape_ch_const_u16 = 40;
static const ai_i8 conv2d_6_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_6_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_6_t_in_0_fmt_scale_const_f32 = 0.005124824121594429f;
static const ai_float conv2d_6_t_out_0_fmt_scale_const_f32 = 0.005847045686095953f;
static const ai_float conv2d_6_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.002921642968431115f, 0.005371857900172472f, 0.0013168106088414788f, 0.002925789449363947f, 0.0015147507656365633f, 0.005688185337930918f, 0.002013328019529581f, 0.0065336874686181545f, 0.002812094520777464f, 0.0009213887387886643f, 0.004158057272434235f, 0.010315261781215668f, 0.0012426921166479588f, 0.0016756955301389098f, 0.003614368150010705f, 0.0017921315738931298f, 0.0007080482901073992f, 0.002254464663565159f, 0.002716850023716688f, 0.004453702364116907f, 0.004019367974251509f, 0.0022291517816483974f, 0.0013232605997473001f, 0.00294568482786417f, 0.0019113278249278665f, 0.0015800130786374211f, 0.0007931032450869679f, 0.0012024358147755265f, 0.002106189029291272f, 0.004032560624182224f, 0.004281397443264723f, 0.003369710873812437f, 0.001206221990287304f, 0.0020620361901819706f, 0.0012103733606636524f, 0.003353320062160492f, 0.0013604576233774424f, 0.002412281231954694f, 0.0024282787926495075f, 0.0023006403353065252f);
static const ai_layer_format_type conv2d_6_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_7_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_7_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_7_pad_before_t_in_0_shape_h_const_u32 = 40;

static const ai_u16 conv2d_7_t_in_0_shape_w_const_u16 = 42;
static const ai_u16 conv2d_7_t_in_0_shape_h_const_u16 = 42;
static const ai_u16 conv2d_7_t_in_0_shape_ch_const_u16 = 40;
static const ai_u16 conv2d_7_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_7_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_7_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_7_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_7_t_in_0_fmt_scale_const_f32 = 0.005847045686095953f;
static const ai_float conv2d_7_t_out_0_fmt_scale_const_f32 = 0.005930582992732525f;
static const ai_float conv2d_7_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.023629693314433098f, 0.004756956826895475f, 0.024385150521993637f, 0.018998825922608376f, 0.040177248418331146f, 0.008165547624230385f, 0.027391718700528145f, 0.009397627785801888f, 0.029555831104516983f, 0.031806595623493195f, 0.0100698946043849f, 0.005261218175292015f, 0.02988705039024353f, 0.010106015019118786f, 0.013859584927558899f, 0.024039333686232567f, 0.047588519752025604f, 0.023938925936818123f, 0.014039339497685432f, 0.016346383839845657f, 0.011101611889898777f, 0.018673323094844818f, 0.012652083300054073f, 0.011776073835790157f, 0.013657758943736553f, 0.030056575313210487f, 0.019980981945991516f, 0.027675263583660126f, 0.038364097476005554f, 0.011813181452453136f, 0.00877333153039217f, 0.01760895363986492f, 0.01219323929399252f, 0.04051398113369942f, 0.023175163194537163f, 0.01371899712830782f, 0.02844415418803692f, 0.02437637187540531f, 0.03938131406903267f, 0.017242563888430595f);
static const ai_u16 conv2d_7_t_out_0_shape_w_const_u16 = 40;
static const ai_u16 conv2d_7_t_out_0_shape_h_const_u16 = 40;

static const ai_u16 conv2d_8_t_in_0_shape_w_const_u16 = 40;
static const ai_u16 conv2d_8_t_in_0_shape_h_const_u16 = 40;
static const ai_u16 conv2d_8_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_8_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_8_t_in_0_shape_ch_const_u16 = 40;
static const ai_u16 conv2d_8_t_out_0_shape_ch_const_u16 = 40;
static const ai_i8 conv2d_8_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_8_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_8_t_in_0_fmt_scale_const_f32 = 0.005930582992732525f;
static const ai_float conv2d_8_t_out_0_fmt_scale_const_f32 = 0.0034911902621388435f;
static const ai_float conv2d_8_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.003202931024134159f, 0.004138390999287367f, 0.0053244465962052345f, 0.0011507770977914333f, 0.0010085389949381351f, 0.00497528025880456f, 0.004945333115756512f, 0.0007317453273572028f, 0.004229584708809853f, 0.0020665163174271584f, 0.0035504500847309828f, 0.0019940603524446487f, 0.002885544439777732f, 0.002766064368188381f, 0.0008805685210973024f, 0.0026634768582880497f, 0.0027474050875753164f, 0.0011012546019628644f, 0.004484795965254307f, 0.003665386000648141f, 0.0028059305623173714f, 0.0016202027909457684f, 0.0031131920404732227f, 0.0022741404827684164f, 0.0028999403584748507f, 0.001882666489109397f, 0.004637422971427441f, 0.0038688310887664557f, 0.00473589263856411f, 0.004142149351537228f, 0.0026330447290092707f, 0.0016819320153445005f, 0.0008881015819497406f, 0.006079333834350109f, 0.005980069283396006f, 0.0030937884002923965f, 0.004580645821988583f, 0.0037788732443004847f, 0.0008250067476183176f, 0.002952362410724163f);
static const ai_layer_format_type conv2d_8_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_10_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_10_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_10_pad_before_t_in_0_shape_h_const_u32 = 40;

static const ai_u16 conv2d_10_t_in_0_shape_w_const_u16 = 42;
static const ai_u16 conv2d_10_t_in_0_shape_h_const_u16 = 42;
static const ai_u16 conv2d_10_t_in_0_shape_ch_const_u16 = 40;
static const ai_u16 conv2d_10_l_stride_1_const_u16 = 2;
static const ai_u16 conv2d_10_l_stride_0_const_u16 = 2;
static const ai_i8 conv2d_10_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_10_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_10_t_in_0_fmt_scale_const_f32 = 0.0034911902621388435f;
static const ai_float conv2d_10_t_out_0_fmt_scale_const_f32 = 0.004611345939338207f;
static const ai_float conv2d_10_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.00520420353859663f, 0.014319561421871185f, 0.0033126655034720898f, 0.007343521807342768f, 0.016279859468340874f, 0.005083177238702774f, 0.0038003730587661266f, 0.014138642698526382f, 0.004693392664194107f, 0.004348174203187227f, 0.0036523593589663506f, 0.004726162645965815f, 0.015309544280171394f, 0.006081718020141125f, 0.015231567434966564f, 0.016773857176303864f, 0.005319957621395588f, 0.01192290149629116f, 0.004504564218223095f, 0.004186434205621481f, 0.005069564562290907f, 0.007284537889063358f, 0.005638682283461094f, 0.0040654088370501995f, 0.0040604062378406525f, 0.012303279712796211f, 0.0041411276906728745f, 0.004279139451682568f, 0.003705402370542288f, 0.004453746136277914f, 0.007387540303170681f, 0.005851908586919308f, 0.010753201320767403f, 0.006085359491407871f, 0.003172806929796934f, 0.003802692983299494f, 0.004507491365075111f, 0.007195469457656145f, 0.011581224389374256f, 0.004822977352887392f);
static const ai_u16 conv2d_10_t_out_0_shape_w_const_u16 = 20;
static const ai_u16 conv2d_10_t_out_0_shape_h_const_u16 = 20;

static const ai_u16 conv2d_11_t_in_0_shape_w_const_u16 = 20;
static const ai_u16 conv2d_11_t_in_0_shape_h_const_u16 = 20;
static const ai_u16 conv2d_11_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_11_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_11_t_in_0_shape_ch_const_u16 = 40;
static const ai_u16 conv2d_11_t_out_0_shape_ch_const_u16 = 72;
static const ai_i8 conv2d_11_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_11_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_11_t_in_0_fmt_scale_const_f32 = 0.004611345939338207f;
static const ai_float conv2d_11_t_out_0_fmt_scale_const_f32 = 0.0022572644520550966f;
static const ai_float conv2d_11_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.003669136203825474f, 0.0013281665742397308f, 0.0023101679980754852f, 0.0009334696806035936f, 0.001049905433319509f, 0.001218432909809053f, 0.002460744231939316f, 0.0012645565439015627f, 0.00273712701164186f, 0.002450427506119013f, 0.0010036317398771644f, 0.0011012997711077332f, 0.0012161184567958117f, 0.001328474492765963f, 0.002441074000671506f, 0.001422567991539836f, 0.0012513240799307823f, 0.001260993885807693f, 0.0006890281802043319f, 0.001083023613318801f, 0.0021157937590032816f, 0.0041383132338523865f, 0.001689621713012457f, 0.0015465962933376431f, 0.0024765655398368835f, 0.001349275466054678f, 0.0019338655984029174f, 0.002596154110506177f, 0.0008866848074831069f, 0.00192562909796834f, 0.0009246067493222654f, 0.002860535169020295f, 0.0022373045794665813f, 0.003600724972784519f, 0.0008472480694763362f, 0.002010551979765296f, 0.0022727297618985176f, 0.0017976411618292332f, 0.0010683898581191897f, 0.002109277993440628f, 0.0010033233556896448f, 0.0007775070844218135f, 0.0026960913091897964f, 0.0015547283692285419f, 0.0029264355544000864f, 0.0015348917804658413f, 0.0012972045224159956f, 0.004023575223982334f, 0.002689251909032464f, 0.0009559222962707281f, 0.001771074254065752f, 0.001139852567575872f, 0.0010767283383756876f, 0.003331926651299f, 0.0018973314436152577f, 0.001150219002738595f, 0.0011675427667796612f, 0.0025824944023042917f, 0.002462285105139017f, 0.00138755957596004f, 0.0011821024818345904f, 0.0007412926061078906f, 0.002131153829395771f, 0.0011767642572522163f, 0.0009869160130620003f, 0.0007588041480630636f, 0.0019160809461027384f, 0.0025688710156828165f, 0.002964471001178026f, 0.0013188804732635617f, 0.0016802524914965034f, 0.0011329645058140159f);
static const ai_layer_format_type conv2d_11_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_12_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_12_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_12_pad_before_t_in_0_shape_h_const_u32 = 20;

static const ai_u16 conv2d_12_t_in_0_shape_w_const_u16 = 22;
static const ai_u16 conv2d_12_t_in_0_shape_h_const_u16 = 22;
static const ai_u16 conv2d_12_t_in_0_shape_ch_const_u16 = 72;
static const ai_u16 conv2d_12_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_12_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_12_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_12_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_12_t_in_0_fmt_scale_const_f32 = 0.0022572644520550966f;
static const ai_float conv2d_12_t_out_0_fmt_scale_const_f32 = 0.00447167968377471f;
static const ai_float conv2d_12_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.01729077845811844f, 0.028287101536989212f, 0.012531779706478119f, 0.03513753414154053f, 0.027922572568058968f, 0.022750280797481537f, 0.006831400562077761f, 0.014670168980956078f, 0.012189795263111591f, 0.008472573012113571f, 0.01346380915492773f, 0.01583881862461567f, 0.02194761112332344f, 0.016421275213360786f, 0.0108365248888731f, 0.021156547591090202f, 0.027409488335251808f, 0.02777671441435814f, 0.028262076899409294f, 0.024281034246087074f, 0.007900912314653397f, 0.012776358053088188f, 0.0201092679053545f, 0.008554820902645588f, 0.012194523587822914f, 0.019880620762705803f, 0.013615185394883156f, 0.011053184047341347f, 0.01770937070250511f, 0.01612122170627117f, 0.02534264139831066f, 0.015263467095792294f, 0.013782012276351452f, 0.01655285805463791f, 0.02568015828728676f, 0.014096381142735481f, 0.0065721264109015465f, 0.01588362269103527f, 0.022738942876458168f, 0.01401476375758648f, 0.02503376640379429f, 0.03154803812503815f, 0.0102835139259696f, 0.02171480841934681f, 0.00933101586997509f, 0.01669261045753956f, 0.021069850772619247f, 0.010971988551318645f, 0.00971557293087244f, 0.019140033051371574f, 0.012471894733607769f, 0.013725897297263145f, 0.02661486715078354f, 0.005492587108165026f, 0.009082205593585968f, 0.033580049872398376f, 0.02184351347386837f, 0.01605289801955223f, 0.01270361989736557f, 0.013683629222214222f, 0.016525976359844208f, 0.01689649000763893f, 0.014029224403202534f, 0.020156564190983772f, 0.022914769127964973f, 0.020994894206523895f, 0.014668081887066364f, 0.015218360349535942f, 0.017507918179035187f, 0.013730291277170181f, 0.015206752344965935f, 0.017540575936436653f);
static const ai_u16 conv2d_12_t_out_0_shape_w_const_u16 = 20;
static const ai_u16 conv2d_12_t_out_0_shape_h_const_u16 = 20;

static const ai_u16 conv2d_13_t_in_0_shape_w_const_u16 = 20;
static const ai_u16 conv2d_13_t_in_0_shape_h_const_u16 = 20;
static const ai_u16 conv2d_13_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_13_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_13_t_in_0_shape_ch_const_u16 = 72;
static const ai_u16 conv2d_13_t_out_0_shape_ch_const_u16 = 72;
static const ai_i8 conv2d_13_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_13_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_13_t_in_0_fmt_scale_const_f32 = 0.00447167968377471f;
static const ai_float conv2d_13_t_out_0_fmt_scale_const_f32 = 0.003036969341337681f;
static const ai_float conv2d_13_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0011017124634236097f, 0.0025259077083319426f, 0.0028219493106007576f, 0.00281588826328516f, 0.00212468090467155f, 0.000925813103094697f, 0.0031973279546946287f, 0.0035001980140805244f, 0.0010615445207804441f, 0.0018320214003324509f, 0.001874226494692266f, 0.0011414961190894246f, 0.002206208184361458f, 0.0010025817900896072f, 0.0022474112920463085f, 0.002732660388574004f, 0.0029781516641378403f, 0.0008254662971012294f, 0.0023795664310455322f, 0.002293291501700878f, 0.0029511877801269293f, 0.0016972485464066267f, 0.0016586740966886282f, 0.0015123754274100065f, 0.003007052466273308f, 0.0028655834030359983f, 0.001263914629817009f, 0.0015786258736625314f, 0.0013284488813951612f, 0.0019204959971830249f, 0.0022243643179535866f, 0.0010061607463285327f, 0.0021282525267452f, 0.0023543338757008314f, 0.0010213084751740098f, 0.00201199552975595f, 0.003141873050481081f, 0.0009721298702061176f, 0.0016390831442549825f, 0.0016220177058130503f, 0.0020558065734803677f, 0.0016335916006937623f, 0.0008321384084410965f, 0.0014380767242982984f, 0.0020957947708666325f, 0.0008958284743130207f, 0.0011813570745289326f, 0.0020891092717647552f, 0.0019593399483710527f, 0.0014775673625990748f, 0.0010404259664937854f, 0.002285099122673273f, 0.0031753391958773136f, 0.0021201055496931076f, 0.004766941536217928f, 0.002522739116102457f, 0.0014675200218334794f, 0.001000876072794199f, 0.0018407604657113552f, 0.001900639501400292f, 0.0011436602799221873f, 0.0011717090383172035f, 0.0024549358058720827f, 0.002419991884380579f, 0.0018025063909590244f, 0.0041940780356526375f, 0.000836525927297771f, 0.002034660428762436f, 0.0016135259065777063f, 0.0016065513482317328f, 0.002636715304106474f, 0.0009204396628774703f);
static const ai_layer_format_type conv2d_13_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_14_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_14_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_14_pad_before_t_in_0_shape_h_const_u32 = 20;

static const ai_u16 conv2d_14_t_in_0_shape_w_const_u16 = 22;
static const ai_u16 conv2d_14_t_in_0_shape_h_const_u16 = 22;
static const ai_u16 conv2d_14_t_in_0_shape_ch_const_u16 = 72;
static const ai_u16 conv2d_14_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_14_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_14_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_14_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_14_t_in_0_fmt_scale_const_f32 = 0.003036969341337681f;
static const ai_float conv2d_14_t_out_0_fmt_scale_const_f32 = 0.0049627092666924f;
static const ai_float conv2d_14_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.013975136913359165f, 0.012295694090425968f, 0.015008382499217987f, 0.005216883961111307f, 0.01090878713876009f, 0.02156950905919075f, 0.013662776909768581f, 0.014377478510141373f, 0.009161693975329399f, 0.011525358073413372f, 0.007270658854395151f, 0.01654965616762638f, 0.011053282767534256f, 0.022528046742081642f, 0.008442739024758339f, 0.005689517129212618f, 0.017085790634155273f, 0.01904185675084591f, 0.0074923839420080185f, 0.008521822281181812f, 0.01937531679868698f, 0.008121475577354431f, 0.011715780943632126f, 0.01190645806491375f, 0.010345918126404285f, 0.009762473404407501f, 0.013203260488808155f, 0.010349947027862072f, 0.016379477456212044f, 0.013280966319143772f, 0.01150763314217329f, 0.018161704763770103f, 0.008223776705563068f, 0.020233983173966408f, 0.032451093196868896f, 0.010180860757827759f, 0.012600519694387913f, 0.013209420256316662f, 0.01900842972099781f, 0.013075679540634155f, 0.009881438687443733f, 0.012739949859678745f, 0.0239535104483366f, 0.013354495167732239f, 0.013557498343288898f, 0.01206190511584282f, 0.008197932504117489f, 0.007884987629950047f, 0.006446665618568659f, 0.009688634425401688f, 0.008534321561455727f, 0.019845474511384964f, 0.004552354570478201f, 0.006006672512739897f, 0.016080064699053764f, 0.008725416846573353f, 0.009942249394953251f, 0.016564715653657913f, 0.011773902922868729f, 0.009550909511744976f, 0.012167206034064293f, 0.0234779492020607f, 0.016287703067064285f, 0.009410995058715343f, 0.012241377495229244f, 0.007545806467533112f, 0.015590599738061428f, 0.017995622009038925f, 0.007895431481301785f, 0.01667107455432415f, 0.012598385103046894f, 0.011642899364233017f);
static const ai_u16 conv2d_14_t_out_0_shape_w_const_u16 = 20;
static const ai_u16 conv2d_14_t_out_0_shape_h_const_u16 = 20;

static const ai_u16 conv2d_15_t_in_0_shape_w_const_u16 = 20;
static const ai_u16 conv2d_15_t_in_0_shape_h_const_u16 = 20;
static const ai_u16 conv2d_15_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_15_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_15_t_in_0_shape_ch_const_u16 = 72;
static const ai_u16 conv2d_15_t_out_0_shape_ch_const_u16 = 72;
static const ai_i8 conv2d_15_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_15_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_15_t_in_0_fmt_scale_const_f32 = 0.0049627092666924f;
static const ai_float conv2d_15_t_out_0_fmt_scale_const_f32 = 0.0046248785220086575f;
static const ai_float conv2d_15_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0012988471426069736f, 0.003568116342648864f, 0.002321176929399371f, 0.003360136179253459f, 0.002812261926010251f, 0.004126167390495539f, 0.003325560363009572f, 0.0015138868475332856f, 0.003388291224837303f, 0.0029569268226623535f, 0.002075670752674341f, 0.0026178820990025997f, 0.003856700612232089f, 0.0031759890262037516f, 0.0016493572620674968f, 0.0035599968396127224f, 0.004439482931047678f, 0.004382862243801355f, 0.002219122601673007f, 0.003881569253280759f, 0.0026219855062663555f, 0.004175451584160328f, 0.0024668199475854635f, 0.002480192808434367f, 0.004112082999199629f, 0.001702966750599444f, 0.0024751080200076103f, 0.003356723114848137f, 0.0020800891797989607f, 0.0016543372767046094f, 0.0020065270364284515f, 0.002157228998839855f, 0.003842363366857171f, 0.0030368391890078783f, 0.0012887113261967897f, 0.002627386013045907f, 0.00299296947196126f, 0.0038603851571679115f, 0.0024321407545357943f, 0.0030505280010402203f, 0.0012852259678766131f, 0.0031210831366479397f, 0.00491315359249711f, 0.0029835228342562914f, 0.0033407118171453476f, 0.002121771452948451f, 0.0036673368886113167f, 0.0030392701737582684f, 0.0023546419106423855f, 0.004600555635988712f, 0.0022826367057859898f, 0.0031205476261675358f, 0.004756611306220293f, 0.0012999363243579865f, 0.003126257797703147f, 0.0015641470672562718f, 0.0030408911406993866f, 0.0035579833202064037f, 0.0018236038740724325f, 0.0034716615919023752f, 0.002208725782111287f, 0.0030155950225889683f, 0.002205105032771826f, 0.0020481818355619907f, 0.0037037802394479513f, 0.0028013025876134634f, 0.003271227702498436f, 0.0034039118327200413f, 0.003240877529606223f, 0.0022532835137099028f, 0.003237171098589897f, 0.005018953233957291f);
static const ai_layer_format_type conv2d_15_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_17_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_17_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_17_pad_before_t_in_0_shape_h_const_u32 = 20;

static const ai_u16 conv2d_17_t_in_0_shape_w_const_u16 = 22;
static const ai_u16 conv2d_17_t_in_0_shape_h_const_u16 = 22;
static const ai_u16 conv2d_17_t_in_0_shape_ch_const_u16 = 72;
static const ai_u16 conv2d_17_l_stride_1_const_u16 = 2;
static const ai_u16 conv2d_17_l_stride_0_const_u16 = 2;
static const ai_i8 conv2d_17_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_17_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_17_t_in_0_fmt_scale_const_f32 = 0.0046248785220086575f;
static const ai_float conv2d_17_t_out_0_fmt_scale_const_f32 = 0.0088109839707613f;
static const ai_float conv2d_17_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.004191384185105562f, 0.003914225846529007f, 0.0050780330784618855f, 0.0022954309824854136f, 0.011267170310020447f, 0.005414100829511881f, 0.007174581754952669f, 0.003747066715732217f, 0.004035492427647114f, 0.0038356375880539417f, 0.003513868898153305f, 0.005356510169804096f, 0.010551388375461102f, 0.003303655656054616f, 0.004517706576734781f, 0.007100601214915514f, 0.00725932139903307f, 0.007693634368479252f, 0.009486585855484009f, 0.004126574844121933f, 0.003961346577852964f, 0.00517520634457469f, 0.010112820193171501f, 0.004392598289996386f, 0.006665202789008617f, 0.003901016665622592f, 0.005911632906645536f, 0.0027569446247071028f, 0.007403570227324963f, 0.0057536098174750805f, 0.004132422618567944f, 0.0036822145339101553f, 0.0049962978810071945f, 0.003756207413971424f, 0.005171614233404398f, 0.005979533772915602f, 0.003989801742136478f, 0.003718402236700058f, 0.003951352089643478f, 0.009320640936493874f, 0.00504583865404129f, 0.005552144255489111f, 0.002402228070423007f, 0.004166464786976576f, 0.004438559990376234f, 0.002752511063590646f, 0.0033033215440809727f, 0.006111203692853451f, 0.003670679870992899f, 0.006715341471135616f, 0.004801878239959478f, 0.0025309778284281492f, 0.010524275712668896f, 0.003855870570987463f, 0.0039283521473407745f, 0.0038448215927928686f, 0.007673716172575951f, 0.006379340309649706f, 0.004774427507072687f, 0.0022376077249646187f, 0.004315129481256008f, 0.003498662728816271f, 0.0064645446836948395f, 0.004279743880033493f, 0.003067617304623127f, 0.0049122837372124195f, 0.007668178994208574f, 0.00944389309734106f, 0.005271440837532282f, 0.004092865623533726f, 0.0037038244772702456f, 0.004332319833338261f);
static const ai_u16 conv2d_17_t_out_0_shape_w_const_u16 = 10;
static const ai_u16 conv2d_17_t_out_0_shape_h_const_u16 = 10;

static const ai_u16 conv2d_18_t_in_0_shape_w_const_u16 = 10;
static const ai_u16 conv2d_18_t_in_0_shape_h_const_u16 = 10;
static const ai_u16 conv2d_18_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_18_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_18_t_in_0_shape_ch_const_u16 = 72;
static const ai_u16 conv2d_18_t_out_0_shape_ch_const_u16 = 152;
static const ai_i8 conv2d_18_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_18_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_18_t_in_0_fmt_scale_const_f32 = 0.0088109839707613f;
static const ai_float conv2d_18_t_out_0_fmt_scale_const_f32 = 0.003233096329495311f;
static const ai_float conv2d_18_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0010982430540025234f, 0.002163079334422946f, 0.0032211937941610813f, 0.001323363627307117f, 0.002683516126126051f, 0.0018447686452418566f, 0.0016970073338598013f, 0.0021327051799744368f, 0.0017447832506150007f, 0.0014713775599375367f, 0.0015283713582903147f, 0.0023527625016868114f, 0.0010281028226017952f, 0.0008419663063250482f, 0.0020182945299893618f, 0.0019611164461821318f, 0.002489241538569331f, 0.00153800705447793f, 0.0027853515930473804f, 0.0014126502210274339f, 0.0019752231892198324f, 0.003135297680273652f, 0.0016599168302491307f, 0.0014589178608730435f, 0.002334326272830367f, 0.0009830128401517868f, 0.0019951947033405304f, 0.00243400945328176f, 0.0017375819152221084f, 0.0017817626940086484f, 0.001304641948081553f, 0.001491934061050415f, 0.0017867301357910037f, 0.0029327920638024807f, 0.0038357125595211983f, 0.001803082530386746f, 0.0027994969859719276f, 0.0019258775282651186f, 0.0009455326362513006f, 0.0016013039276003838f, 0.001935686101205647f, 0.0011491754557937384f, 0.0013954943278804421f, 0.0017137934919446707f, 0.00531987939029932f, 0.004477732814848423f, 0.0015583073254674673f, 0.0024069463834166527f, 0.001883580582216382f, 0.0018946289783343673f, 0.002807651413604617f, 0.0013065328821539879f, 0.0021411466877907515f, 0.0018953619292005897f, 0.0009121160255745053f, 0.0020594936795532703f, 0.0016956723993644118f, 0.0014789315173402429f, 0.0012508188374340534f, 0.0020393438171595335f, 0.0027770795859396458f, 0.0019152637105435133f, 0.0012023837771266699f, 0.000871746800839901f, 0.0013737764675170183f, 0.0015690313884988427f, 0.0032105157151818275f, 0.00144367350731045f, 0.001892931293696165f, 0.002702693222090602f, 0.0009918131399899721f, 0.001985992072150111f, 0.002926273737102747f, 0.0011881828540936112f, 0.0015991134569048882f, 0.0015740061644464731f, 0.00223534251563251f, 0.00204489310272038f, 0.0021425883751362562f, 0.0013056847965344787f, 0.002163202501833439f, 0.0016708007315173745f, 0.0017390440916642547f, 0.0014840392395853996f, 0.0021059741266071796f, 0.0023719549644738436f, 0.0015149712562561035f, 0.0011890014866366982f, 0.0014371091965585947f, 0.0025783060118556023f, 0.0014655219856649637f, 0.0010111709125339985f, 0.0011689916718751192f, 0.000646508124191314f, 0.0017773750005289912f, 0.0022809752263128757f, 0.0016363393515348434f, 0.0015079538570716977f, 0.001793631468899548f, 0.0011122884461656213f, 0.0019923041108995676f, 0.0028476030565798283f, 0.002520548179745674f, 0.0012543249176815152f, 0.0011663234326988459f, 0.0022177889477461576f, 0.0013461990747600794f, 0.0032823411747813225f, 0.000918711768463254f, 0.0018797052325680852f, 0.0014429223956540227f, 0.003140039509162307f, 0.0011356724426150322f, 0.0011635791743174195f, 0.0017492492916062474f, 0.0022348666097968817f, 0.0027484591118991375f, 0.0017723629716783762f, 0.0037652566097676754f, 0.001508680870756507f, 0.001245971885509789f, 0.0016434312565252185f, 0.0021991741377860308f, 0.0015859223203733563f, 0.0017281521577388048f, 0.0024612657725811005f, 0.001657607383094728f, 0.0027214365545660257f, 0.001352804247289896f, 0.0017475608037784696f, 0.002158411778509617f, 0.0010602321708574891f, 0.00123036268632859f, 0.003428203286603093f, 0.002445232355967164f, 0.0014771983260288835f, 0.0018015154637396336f, 0.0018191090784966946f, 0.0015253527089953423f, 0.0019149048021063209f, 0.002219324465841055f, 0.0012761539546772838f, 0.0020336511079221964f, 0.002748222555965185f, 0.0017323167994618416f, 0.0019248020835220814f, 0.0011097132228314877f, 0.0010674688965082169f, 0.0010861833579838276f, 0.0017680812161415815f, 0.0013479130575433373f, 0.0014992491342127323f);
static const ai_layer_format_type conv2d_18_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_19_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_19_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_19_pad_before_t_in_0_shape_h_const_u32 = 10;

static const ai_u16 conv2d_19_t_in_0_shape_w_const_u16 = 12;
static const ai_u16 conv2d_19_t_in_0_shape_h_const_u16 = 12;
static const ai_u16 conv2d_19_t_in_0_shape_ch_const_u16 = 152;
static const ai_u16 conv2d_19_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_19_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_19_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_19_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_19_t_in_0_fmt_scale_const_f32 = 0.003233096329495311f;
static const ai_float conv2d_19_t_out_0_fmt_scale_const_f32 = 0.007384846452623606f;
static const ai_float conv2d_19_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.007830248214304447f, 0.005743850953876972f, 0.004206044599413872f, 0.01208124216645956f, 0.014796849340200424f, 0.011249654926359653f, 0.008921376429498196f, 0.013156449422240257f, 0.0073485225439071655f, 0.013589256443083286f, 0.007270210422575474f, 0.006685193162411451f, 0.012818374671041965f, 0.008413557894527912f, 0.005919212009757757f, 0.009397968649864197f, 0.009631824679672718f, 0.006282986607402563f, 0.0034436299465596676f, 0.010717874392867088f, 0.01092672161757946f, 0.006677643395960331f, 0.008152955211699009f, 0.008602467365562916f, 0.005408288910984993f, 0.010191613808274269f, 0.005701141897588968f, 0.008923416025936604f, 0.01115909032523632f, 0.007657767739146948f, 0.008411480113863945f, 0.012392839416861534f, 0.004291375633329153f, 0.007097826339304447f, 0.007861731573939323f, 0.01038507092744112f, 0.005204715766012669f, 0.011901983991265297f, 0.0067955506965518f, 0.006245020776987076f, 0.009250842966139317f, 0.010174569673836231f, 0.007567095570266247f, 0.009435923770070076f, 0.010557616129517555f, 0.003184547880664468f, 0.014295199885964394f, 0.008729616180062294f, 0.007224017754197121f, 0.012487106956541538f, 0.008016875013709068f, 0.007131517864763737f, 0.009959814138710499f, 0.007604216691106558f, 0.019907895475625992f, 0.009892291389405727f, 0.010609962977468967f, 0.008943693712353706f, 0.008079258725047112f, 0.008556073531508446f, 0.009112605825066566f, 0.014188731089234352f, 0.009755522944033146f, 0.010356496088206768f, 0.012380590662360191f, 0.009629135951399803f, 0.007775312755256891f, 0.009708302095532417f, 0.013295958749949932f, 0.013191865757107735f, 0.008216308429837227f, 0.008217830210924149f, 0.009247994050383568f, 0.009484312497079372f, 0.006365483161062002f, 0.015586091205477715f, 0.009565887041389942f, 0.0120485108345747f, 0.007157766725867987f, 0.012283745221793652f, 0.008392544463276863f, 0.009067256934940815f, 0.00836173165589571f, 0.012245677411556244f, 0.00661081587895751f, 0.004975270479917526f, 0.004776841029524803f, 0.008676610887050629f, 0.010300450958311558f, 0.011261023581027985f, 0.007378963753581047f, 0.011543058790266514f, 0.01799832284450531f, 0.013954396359622478f, 0.009827415458858013f, 0.0058621251955628395f, 0.012701169587671757f, 0.007747661788016558f, 0.013044934719800949f, 0.007819351740181446f, 0.006811482831835747f, 0.010384510271251202f, 0.006679625250399113f, 0.012530489824712276f, 0.014819847419857979f, 0.007544876076281071f, 0.005951268598437309f, 0.010156821459531784f, 0.010501619428396225f, 0.007946668192744255f, 0.008932298049330711f, 0.004358398262411356f, 0.009672602638602257f, 0.012449093163013458f, 0.00884963572025299f, 0.01118102390319109f, 0.00737137021496892f, 0.0076079582795500755f, 0.016192063689231873f, 0.009283345192670822f, 0.014630256220698357f, 0.008095629513263702f, 0.009405357763171196f, 0.008623928762972355f, 0.00771283358335495f, 0.009384386241436005f, 0.009072048589587212f, 0.0046484642662107944f, 0.013011621311306953f, 0.008421732112765312f, 0.008324968628585339f, 0.008264558389782906f, 0.0067922621965408325f, 0.003679963992908597f, 0.00778165552765131f, 0.006685388274490833f, 0.005675023887306452f, 0.00602942518889904f, 0.005129910074174404f, 0.0061551970429718494f, 0.0069162845611572266f, 0.009266344830393791f, 0.010759101249277592f, 0.004894961137324572f, 0.007607718463987112f, 0.012217294424772263f, 0.013309053145349026f, 0.0061669787392020226f, 0.01456387247890234f, 0.004474777728319168f, 0.014106504619121552f, 0.00915549322962761f);
static const ai_u16 conv2d_19_t_out_0_shape_w_const_u16 = 10;
static const ai_u16 conv2d_19_t_out_0_shape_h_const_u16 = 10;

static const ai_u16 conv2d_20_t_in_0_shape_w_const_u16 = 10;
static const ai_u16 conv2d_20_t_in_0_shape_h_const_u16 = 10;
static const ai_u16 conv2d_20_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_20_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_20_t_in_0_shape_ch_const_u16 = 152;
static const ai_u16 conv2d_20_t_out_0_shape_ch_const_u16 = 152;
static const ai_i8 conv2d_20_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_20_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_20_t_in_0_fmt_scale_const_f32 = 0.007384846452623606f;
static const ai_float conv2d_20_t_out_0_fmt_scale_const_f32 = 0.0035589300096035004f;
static const ai_float conv2d_20_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.002620646497234702f, 0.0016211821930482984f, 0.0019827233627438545f, 0.0027708851266652346f, 0.002674754010513425f, 0.002283357549458742f, 0.0018406329909339547f, 0.002320081926882267f, 0.0016227267915382981f, 0.0016311592189595103f, 0.0018614798318594694f, 0.001489768153987825f, 0.0018849207554012537f, 0.001218969002366066f, 0.0027784754056483507f, 0.0027960364241153f, 0.0036147141363471746f, 0.0028342141304165125f, 0.002338122110813856f, 0.0025493719149380922f, 0.002209231024608016f, 0.001204298809170723f, 0.0020561960991472006f, 0.0027437452226877213f, 0.0018025435274466872f, 0.001874356297776103f, 0.002494476269930601f, 0.002484342083334923f, 0.0024576301220804453f, 0.002056339755654335f, 0.0029882199596613646f, 0.0021954237017780542f, 0.0032440414652228355f, 0.0051232799887657166f, 0.0014037707587704062f, 0.0032546245492994785f, 0.0029633587691932917f, 0.0020924920681864023f, 0.0021383820567280054f, 0.0024824494030326605f, 0.002102799015119672f, 0.003253869479522109f, 0.0011991456849500537f, 0.0018064698670059443f, 0.0020040639210492373f, 0.0018779406091198325f, 0.003050131257623434f, 0.0014125353191047907f, 0.0014746857341378927f, 0.0016916723689064384f, 0.0020100523252040148f, 0.0015749044250696898f, 0.0027739510405808687f, 0.0020263863261789083f, 0.002875550650060177f, 0.0014542017597705126f, 0.0019306923495605588f, 0.00237496686168015f, 0.003710002638399601f, 0.001806145068258047f, 0.001599381910637021f, 0.001195750548504293f, 0.002468631137162447f, 0.0016367018688470125f, 0.0017615625401958823f, 0.0013570813462138176f, 0.002036508871242404f, 0.0017675901763141155f, 0.0029489321168512106f, 0.002335320459678769f, 0.0022935958113521338f, 0.002225296339020133f, 0.00201492034830153f, 0.001578761963173747f, 0.0034424560144543648f, 0.0027048487681895494f, 0.00264631025493145f, 0.00277917948551476f, 0.002229248872026801f, 0.0026362526696175337f, 0.001732444390654564f, 0.0022106885444372892f, 0.003555419621989131f, 0.0015501700108870864f, 0.0015681469812989235f, 0.003346743993461132f, 0.0019958368502557278f, 0.0023159561678767204f, 0.00243544764816761f, 0.0017312744166702032f, 0.0019079677294939756f, 0.0026240507140755653f, 0.0013899803161621094f, 0.000838720821775496f, 0.001999164931476116f, 0.0028685687575489283f, 0.0023792937863618135f, 0.002086271531879902f, 0.0021960497833788395f, 0.0016404481139034033f, 0.0020979782566428185f, 0.0022937527392059565f, 0.0018985762726515532f, 0.0022319238632917404f, 0.0026860288344323635f, 0.0012278237845748663f, 0.0011408652644604445f, 0.0024712395388633013f, 0.002670202637091279f, 0.0035072017926722765f, 0.0017858716892078519f, 0.0022828313522040844f, 0.0020199113059788942f, 0.0022295790258795023f, 0.0017418625066056848f, 0.0015401961281895638f, 0.0021494957618415356f, 0.0033635185100138187f, 0.0018018311820924282f, 0.0016226317966356874f, 0.0028839402366429567f, 0.002601945074275136f, 0.0019793694373220205f, 0.0021198359318077564f, 0.00245106965303421f, 0.0020315698347985744f, 0.0015939585864543915f, 0.0018464941531419754f, 0.0034068257082253695f, 0.0024090204387903214f, 0.0015157533343881369f, 0.003952380269765854f, 0.0022853550035506487f, 0.0014095057267695665f, 0.0017637393902987242f, 0.001790875568985939f, 0.0019137144554406404f, 0.0024094167165458202f, 0.00260242260992527f, 0.0026187945622950792f, 0.0014488986926153302f, 0.0030922505538910627f, 0.002279269741848111f, 0.0017667794600129128f, 0.0015629869885742664f, 0.0014284984208643436f, 0.0010008824756368995f, 0.0023235438857227564f, 0.0028517560567706823f, 0.0025699101388454437f, 0.0021784149575978518f, 0.0017596312100067735f);
static const ai_layer_format_type conv2d_20_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_22_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_22_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_22_pad_before_t_in_0_shape_h_const_u32 = 10;

static const ai_u16 conv2d_22_t_in_0_shape_w_const_u16 = 12;
static const ai_u16 conv2d_22_t_in_0_shape_h_const_u16 = 12;
static const ai_u16 conv2d_22_t_in_0_shape_ch_const_u16 = 152;
static const ai_u16 conv2d_22_l_stride_1_const_u16 = 2;
static const ai_u16 conv2d_22_l_stride_0_const_u16 = 2;
static const ai_i8 conv2d_22_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_22_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_22_t_in_0_fmt_scale_const_f32 = 0.0035589300096035004f;
static const ai_float conv2d_22_t_out_0_fmt_scale_const_f32 = 0.003517482429742813f;
static const ai_float conv2d_22_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0063691334798932076f, 0.003244267776608467f, 0.007678014691919088f, 0.0024814673233777285f, 0.003645117161795497f, 0.0032872857991605997f, 0.0037961259949952364f, 0.00601125368848443f, 0.0065773543901741505f, 0.004771499894559383f, 0.003269980661571026f, 0.005887957289814949f, 0.005151527002453804f, 0.005871899425983429f, 0.007467540912330151f, 0.009166168980300426f, 0.002667089691385627f, 0.004767259582877159f, 0.004909043665975332f, 0.005519082769751549f, 0.0021242094226181507f, 0.005169977899640799f, 0.007601565215736628f, 0.0034903320483863354f, 0.003678607288748026f, 0.006425634026527405f, 0.007761016953736544f, 0.006149278022348881f, 0.0029992086347192526f, 0.0036918872501701117f, 0.008849089033901691f, 0.0033096729312092066f, 0.0031896070577204227f, 0.0018570891115814447f, 0.007808836176991463f, 0.004247803241014481f, 0.006762448698282242f, 0.003654319327324629f, 0.0029608409386128187f, 0.005035999231040478f, 0.005959834437817335f, 0.005815758369863033f, 0.006175029091536999f, 0.0070466878823935986f, 0.003568618092685938f, 0.006520794704556465f, 0.006528744474053383f, 0.008035941980779171f, 0.003428113181143999f, 0.004958314821124077f, 0.008096600882709026f, 0.006585625000298023f, 0.007844002917408943f, 0.004911681637167931f, 0.00575731135904789f, 0.0051980772987008095f, 0.0062957825139164925f, 0.00566277327015996f, 0.003444389905780554f, 0.005209648050367832f, 0.005180066451430321f, 0.005023965612053871f, 0.005559633951634169f, 0.004124390427023172f, 0.005625630728900433f, 0.005278073251247406f, 0.00711033632978797f, 0.004901662934571505f, 0.008241688832640648f, 0.003873503068462014f, 0.0038884184323251247f, 0.004928416106849909f, 0.006866393610835075f, 0.0045244693756103516f, 0.003414957784116268f, 0.0060641528107225895f, 0.004189351107925177f, 0.007919578813016415f, 0.004213269799947739f, 0.005580027587711811f, 0.005196880083531141f, 0.004698482342064381f, 0.0034367747139185667f, 0.004894740879535675f, 0.003884701756760478f, 0.0023338969331234694f, 0.007259990554302931f, 0.0034823992755264044f, 0.009133797138929367f, 0.009100028313696384f, 0.004443516489118338f, 0.006044115871191025f, 0.002935622353106737f, 0.005571696907281876f, 0.005221164785325527f, 0.004704510793089867f, 0.005261923652142286f, 0.006702335085719824f, 0.003574982052668929f, 0.006296129431575537f, 0.0029833067674189806f, 0.0034891441464424133f, 0.004457981325685978f, 0.00630560889840126f, 0.00652412511408329f, 0.00394049845635891f, 0.004166950471699238f, 0.0025279999244958162f, 0.00792863592505455f, 0.0031883525662124157f, 0.0069796983152627945f, 0.004373886622488499f, 0.0033709187991917133f, 0.004075021483004093f, 0.007412791717797518f, 0.005456274375319481f, 0.005707248579710722f, 0.0038793738931417465f, 0.004367486108094454f, 0.006383819971233606f, 0.006813329644501209f, 0.003192783799022436f, 0.006913569290190935f, 0.006022625137120485f, 0.0035246743354946375f, 0.006774428300559521f, 0.003489037975668907f, 0.003843209007754922f, 0.0075591192580759525f, 0.005838504992425442f, 0.005205895751714706f, 0.002640866208821535f, 0.003777411999180913f, 0.004659361205995083f, 0.0027767799329012632f, 0.004441418685019016f, 0.0038377968594431877f, 0.0025909363757818937f, 0.0030960827134549618f, 0.008759699761867523f, 0.0031083435751497746f, 0.004070460330694914f, 0.005019934382289648f, 0.003108720760792494f, 0.004468120168894529f, 0.005010226741433144f, 0.0051179067231714725f, 0.003121420042589307f, 0.003837169148027897f, 0.008660821244120598f, 0.005876537878066301f, 0.008576744236052036f);
static const ai_u16 conv2d_22_t_out_0_shape_w_const_u16 = 5;
static const ai_u16 conv2d_22_t_out_0_shape_h_const_u16 = 5;

static const ai_u16 conv2d_23_t_in_0_shape_w_const_u16 = 5;
static const ai_u16 conv2d_23_t_in_0_shape_h_const_u16 = 5;
static const ai_u16 conv2d_23_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_23_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_23_t_in_0_shape_ch_const_u16 = 152;
static const ai_u16 conv2d_23_t_out_0_shape_ch_const_u16 = 288;
static const ai_i8 conv2d_23_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_23_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_23_t_in_0_fmt_scale_const_f32 = 0.003517482429742813f;
static const ai_float conv2d_23_t_out_0_fmt_scale_const_f32 = 0.005404532887041569f;
static const ai_float conv2d_23_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.002019676612690091f, 0.001843270962126553f, 0.0020382432267069817f, 0.00232253922149539f, 0.000988353043794632f, 0.002186213620007038f, 0.0013225260190665722f, 0.0012735107447952032f, 0.001953321974724531f, 0.0014306844677776098f, 0.002510734833776951f, 0.0019099503988400102f, 0.0010263865115121007f, 0.001720563624985516f, 0.0024496777914464474f, 0.001717355102300644f, 0.0016129814321175218f, 0.001740850624628365f, 0.002021062420681119f, 0.0013581009116023779f, 0.0019200782990083098f, 0.0019198870286345482f, 0.002072061412036419f, 0.0017823210218921304f, 0.0010083853267133236f, 0.0014000131050124764f, 0.0021883780136704445f, 0.0021875607781112194f, 0.0013712006621062756f, 0.001591721549630165f, 0.0022539463825523853f, 0.002037317957729101f, 0.0012378491228446364f, 0.0020682928152382374f, 0.0032702889293432236f, 0.0013513686135411263f, 0.0022930551785975695f, 0.0025969587732106447f, 0.0014245340134948492f, 0.002009909600019455f, 0.001496966928243637f, 0.0024336723145097494f, 0.0013509130803868175f, 0.0008766148239374161f, 0.0032008816488087177f, 0.002415976021438837f, 0.0019865441136062145f, 0.0018472857773303986f, 0.001624307013116777f, 0.0021428782492876053f, 0.00155932258348912f, 0.002409536624327302f, 0.0012786148581653833f, 0.002488515805453062f, 0.003620656905695796f, 0.0028348888736218214f, 0.0011199007276445627f, 0.004767767619341612f, 0.0030405165161937475f, 0.0027656317688524723f, 0.002009756863117218f, 0.001743090571835637f, 0.0014301553601399064f, 0.002318310784175992f, 0.0024657254107296467f, 0.0011482641566544771f, 0.003351448103785515f, 0.0022904020734131336f, 0.0021363284904509783f, 0.0027560738380998373f, 0.0023904668632894754f, 0.0013497129548341036f, 0.001700106542557478f, 0.0011881544487550855f, 0.0013405554927885532f, 0.0022480536717921495f, 0.0019050759728997946f, 0.0017055829521268606f, 0.0029052423778921366f, 0.0020073081832379103f, 0.001722965156659484f, 0.0011735386215150356f, 0.001768189249560237f, 0.002175727393478155f, 0.0020343950018286705f, 0.0016658880049362779f, 0.003289512125775218f, 0.0027597416192293167f, 0.0023501794785261154f, 0.0015043704770505428f, 0.003037923714146018f, 0.0015818655956536531f, 0.0017334714066237211f, 0.0015613288851454854f, 0.001746575115248561f, 0.0011188926873728633f, 0.0016924041556194425f, 0.001475611119531095f, 0.0014453285839408636f, 0.001786250970326364f, 0.002784550888463855f, 0.0019324831664562225f, 0.0011192432139068842f, 0.0015553522389382124f, 0.001467689173296094f, 0.0034372922964394093f, 0.0015623192302882671f, 0.0013210392789915204f, 0.0031627588905394077f, 0.0021861630957573652f, 0.004427412990480661f, 0.0016628208104521036f, 0.0020751524716615677f, 0.0023654282558709383f, 0.001843473524786532f, 0.0015130130341276526f, 0.0020633849781006575f, 0.0024701545480638742f, 0.0015211927238851786f, 0.001122975256294012f, 0.0014636648120358586f, 0.003804141189903021f, 0.003536244621500373f, 0.0022854458075016737f, 0.0034466921351850033f, 0.0013636909425258636f, 0.0025291002821177244f, 0.001121743000112474f, 0.0014251504326239228f, 0.0012357907835394144f, 0.0014800371136516333f, 0.002149261301383376f, 0.0022535815369337797f, 0.0030583052430301905f, 0.0015633468283340335f, 0.002273228717967868f, 0.0015460068825632334f, 0.002306142821907997f, 0.0015342480037361383f, 0.002082038903608918f, 0.002403134247288108f, 0.0012133080745115876f, 0.001902171759866178f, 0.00199499586597085f, 0.0019450730178505182f, 0.0011198563734069467f, 0.002635964658111334f, 0.0013102424563840032f, 0.00283241574652493f, 0.001311708241701126f, 0.0016414327546954155f, 0.0017085005529224873f, 0.0012459354475140572f, 0.001209034351631999f, 0.0010512361768633127f, 0.0012786106672137976f, 0.002450093859806657f, 0.0015851029893383384f, 0.001536153955385089f, 0.0014445885317400098f, 0.0024477015249431133f, 0.0022176550701260567f, 0.001996125327423215f, 0.0017331158742308617f, 0.0019067078828811646f, 0.0013328554341569543f, 0.00644606864079833f, 0.001419421867467463f, 0.0019950158894062042f, 0.0020517201628535986f, 0.0017353497678413987f, 0.0013105557300150394f, 0.001457287697121501f, 0.002087096683681011f, 0.0017537083476781845f, 0.0013812006218358874f, 0.0025062996428459883f, 0.0014015593333169818f, 0.001845580874942243f, 0.0015798909589648247f, 0.001262473058886826f, 0.0024836480151861906f, 0.0012417543912306428f, 0.0015414775116369128f, 0.0020714045967906713f, 0.001606736215762794f, 0.001473482814617455f, 0.002857720945030451f, 0.0018139693420380354f, 0.001762055093422532f, 0.002935248427093029f, 0.0014878170331940055f, 0.0024673098232597113f, 0.002491563791409135f, 0.0017506348667666316f, 0.0025861733593046665f, 0.0025502329226583242f, 0.001915769069455564f, 0.0026483451947569847f, 0.0018889589700847864f, 0.002904467051848769f, 0.0018247879343107343f, 0.0015076057752594352f, 0.0010898939799517393f, 0.001246377476491034f, 0.001442417735233903f, 0.0012234689202159643f, 0.0016989477444440126f, 0.0010688066249713302f, 0.0016554821049794555f, 0.0021421739365905523f, 0.0019214162603020668f, 0.0015043066814541817f, 0.001399679691530764f, 0.001551394583657384f, 0.0027611369732767344f, 0.0018708741990849376f, 0.002660792088136077f, 0.0017600069986656308f, 0.00127565604634583f, 0.002182922326028347f, 0.0016826590290293097f, 0.0018563297344371676f, 0.002717003459110856f, 0.001321108196862042f, 0.0026042263489216566f, 0.00209615146741271f, 0.0018466322217136621f, 0.0009730273741297424f, 0.001666339347139001f, 0.0011212917743250728f, 0.001971819903701544f, 0.0011590858921408653f, 0.0014181877486407757f, 0.0012094627600163221f, 0.0015855432720854878f, 0.001598248491063714f, 0.0020770044066011906f, 0.0014860510127618909f, 0.0021593605633825064f, 0.0024042418226599693f, 0.001242329366505146f, 0.0019281364511698484f, 0.0025824010372161865f, 0.0016202115220949054f, 0.0015723566757515073f, 0.0010756244882941246f, 0.0022210765164345503f, 0.0018005069578066468f, 0.0016362497117370367f, 0.0027359826490283012f, 0.0018058288842439651f, 0.001444243942387402f, 0.0013878903118893504f, 0.0019263525027781725f, 0.008359279483556747f, 0.0011809651041403413f, 0.001695653423666954f, 0.0024841008707880974f, 0.0013867536326870322f, 0.0015215077437460423f, 0.00234416825696826f, 0.0028589058201760054f, 0.001659133704379201f, 0.0013051884016022086f, 0.0022429360542446375f, 0.0019499296322464943f, 0.002192431828007102f, 0.001355163985863328f, 0.0013190762838348746f, 0.0014576631365343928f, 0.0015841544372960925f, 0.0011681339237838984f, 0.0015158834867179394f, 0.0023231992963701487f, 0.001127398107200861f, 0.0030401612166315317f, 0.0018027886981144547f, 0.0014824002282693982f, 0.0015079797012731433f, 0.0017717775190249085f, 0.0010968938004225492f, 0.0014966285089030862f, 0.0019854274578392506f, 0.0018432664219290018f, 0.0013879392063245177f, 0.002184430370107293f, 0.0009293034672737122f);
static const ai_layer_format_type conv2d_23_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_24_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_24_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_24_pad_before_t_in_0_shape_h_const_u32 = 5;

static const ai_u16 conv2d_24_t_in_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_24_t_in_0_shape_h_const_u16 = 7;
static const ai_u16 conv2d_24_t_in_0_shape_ch_const_u16 = 288;
static const ai_u16 conv2d_24_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_24_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_24_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_24_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_24_t_in_0_fmt_scale_const_f32 = 0.005404532887041569f;
static const ai_float conv2d_24_t_out_0_fmt_scale_const_f32 = 0.003411282319575548f;
static const ai_float conv2d_24_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.006264713127166033f, 0.006700744386762381f, 0.005821641068905592f, 0.0056006889790296555f, 0.006185013335198164f, 0.007349810563027859f, 0.006193578243255615f, 0.0090987803414464f, 0.005882985889911652f, 0.008890886791050434f, 0.008929330855607986f, 0.003823476145043969f, 0.007197344209998846f, 0.005747704301029444f, 0.003571184119209647f, 0.005606948398053646f, 0.006235605105757713f, 0.007311945781111717f, 0.008693791925907135f, 0.005789687857031822f, 0.007043077144771814f, 0.0056897797621786594f, 0.005456867162138224f, 0.008569778874516487f, 0.006298028863966465f, 0.009301265701651573f, 0.0056706699542701244f, 0.006203155033290386f, 0.002975743031129241f, 0.008354755118489265f, 0.0025870599783957005f, 0.007626691833138466f, 0.005782332271337509f, 0.006251745857298374f, 0.006521336734294891f, 0.0070131151005625725f, 0.004868416581302881f, 0.006564334500581026f, 0.004883802030235529f, 0.006032384932041168f, 0.00590038625523448f, 0.0060520535334944725f, 0.004977299366146326f, 0.00994224101305008f, 0.007183089852333069f, 0.00658319890499115f, 0.003261169418692589f, 0.009999261237680912f, 0.005931399762630463f, 0.0063008577562868595f, 0.007786950562149286f, 0.0035969812888652086f, 0.0030145631171762943f, 0.0067088971845805645f, 0.0062635717913508415f, 0.003600307973101735f, 0.006262191105633974f, 0.002553576370701194f, 0.003922137897461653f, 0.0059799375012516975f, 0.004223651718348265f, 0.007941493764519691f, 0.005708782002329826f, 0.007129520643502474f, 0.006315631791949272f, 0.007658213842660189f, 0.008630666881799698f, 0.008377613499760628f, 0.006804925389587879f, 0.006290199235081673f, 0.0058990782126784325f, 0.008381200954318047f, 0.005955750588327646f, 0.005637787748128176f, 0.007675011642277241f, 0.0063910093158483505f, 0.007494804449379444f, 0.009008588269352913f, 0.008058298379182816f, 0.005839996039867401f, 0.005548490677028894f, 0.007970128208398819f, 0.007973100058734417f, 0.006476712413132191f, 0.004192527383565903f, 0.008866358548402786f, 0.003279599593952298f, 0.008120439946651459f, 0.003711269935593009f, 0.004627508111298084f, 0.006799432449042797f, 0.005382186267524958f, 0.004242370370775461f, 0.010167925618588924f, 0.006863812450319529f, 0.008423839695751667f, 0.0051544904708862305f, 0.006317618768662214f, 0.0073988051153719425f, 0.004123280756175518f, 0.004983746446669102f, 0.0030137374997138977f, 0.004848245996981859f, 0.00902202632278204f, 0.006446779239922762f, 0.009818827733397484f, 0.004501964896917343f, 0.005964255426079035f, 0.006654833909124136f, 0.006013327743858099f, 0.00446822727099061f, 0.010399525985121727f, 0.010149243287742138f, 0.00622537499293685f, 0.010381203144788742f, 0.008674334734678268f, 0.007658018264919519f, 0.0075930021703243256f, 0.004693275783210993f, 0.010086800903081894f, 0.005763709545135498f, 0.006675769109278917f, 0.008088838309049606f, 0.008711780421435833f, 0.008097134530544281f, 0.006738867145031691f, 0.0068429699167609215f, 0.005850448273122311f, 0.008397904224693775f, 0.008966157212853432f, 0.005904016550630331f, 0.009299175813794136f, 0.006730359513312578f, 0.0058320071548223495f, 0.007730638142675161f, 0.007426643744111061f, 0.009294424206018448f, 0.006812071893364191f, 0.005287994630634785f, 0.009646122343838215f, 0.009590559639036655f, 0.006647094618529081f, 0.005220940802246332f, 0.005825316999107599f, 0.005060861352831125f, 0.00828113779425621f, 0.00324457255192101f, 0.005282227415591478f, 0.007938665337860584f, 0.009178238920867443f, 0.006127862259745598f, 0.007060521747916937f, 0.008799879811704159f, 0.009593009017407894f, 0.007097363471984863f, 0.006311060860753059f, 0.005312641151249409f, 0.0032224298920482397f, 0.008993814699351788f, 0.009787493385374546f, 0.007933025248348713f, 0.006585114635527134f, 0.007195116486400366f, 0.005692771170288324f, 0.006214743014425039f, 0.00845420453697443f, 0.0044013517908751965f, 0.007061255630105734f, 0.005357448942959309f, 0.007342138327658176f, 0.00655131321400404f, 0.008831887505948544f, 0.007655390538275242f, 0.005412185564637184f, 0.010384575463831425f, 0.0028495800215750933f, 0.009836123324930668f, 0.008860514499247074f, 0.007499839644879103f, 0.007400678005069494f, 0.006578424014151096f, 0.006385656073689461f, 0.00632688170298934f, 0.007009128574281931f, 0.00633583776652813f, 0.010826555080711842f, 0.0027138548903167248f, 0.00682493019849062f, 0.007756498642265797f, 0.007447842974215746f, 0.007045833393931389f, 0.0045601557940244675f, 0.0042105731554329395f, 0.0024768300354480743f, 0.0025791290681809187f, 0.007678653113543987f, 0.003880333388224244f, 0.007288278546184301f, 0.008272569626569748f, 0.00646434398368001f, 0.006189798004925251f, 0.006956459488719702f, 0.006695459596812725f, 0.0036979687865823507f, 0.005880078300833702f, 0.007533069234341383f, 0.004866236820816994f, 0.005565401632338762f, 0.0067380317486822605f, 0.00869657751172781f, 0.0068701826967298985f, 0.005498948507010937f, 0.006706612650305033f, 0.0049402411095798016f, 0.0040022265166044235f, 0.007713253144174814f, 0.006050481926649809f, 0.007716143038123846f, 0.00757636921480298f, 0.00555173447355628f, 0.006610057782381773f, 0.010237040929496288f, 0.004711703862994909f, 0.002677115611732006f, 0.005229542963206768f, 0.010486084967851639f, 0.008724754676222801f, 0.0055922409519553185f, 0.008675910532474518f, 0.005266066174954176f, 0.008100002072751522f, 0.003988157492130995f, 0.008154788054525852f, 0.0054985289461910725f, 0.005381093826144934f, 0.006251340266317129f, 0.004949876572936773f, 0.009546073153614998f, 0.005512618459761143f, 0.009544389322400093f, 0.005742775741964579f, 0.0051100593991577625f, 0.007923630997538567f, 0.0067533040419220924f, 0.008465788327157497f, 0.005469797644764185f, 0.004613635130226612f, 0.004981108009815216f, 0.009125187993049622f, 0.005022467579692602f, 0.005296185612678528f, 0.005887690931558609f, 0.008492588065564632f, 0.0098344124853611f, 0.007081140764057636f, 0.005239712540060282f, 0.004105428699404001f, 0.0076101371087133884f, 0.005054940935224295f, 0.0046576266176998615f, 0.005989530123770237f, 0.008944237604737282f, 0.0038853497244417667f, 0.004822691902518272f, 0.01018149871379137f, 0.010302528738975525f, 0.008495152927935123f, 0.007725473493337631f, 0.006699931342154741f, 0.005966124590486288f, 0.004888067953288555f, 0.005038164090365171f, 0.004683127626776695f, 0.005634311120957136f, 0.007716976571828127f, 0.0042509702034294605f, 0.007139334920793772f, 0.011699221096932888f, 0.0059698279947042465f, 0.008592119440436363f, 0.005396345164626837f, 0.00433904305100441f, 0.009677965193986893f, 0.005230553448200226f, 0.008317610248923302f, 0.005408119410276413f, 0.008643969893455505f, 0.00851821806281805f);
static const ai_u16 conv2d_24_t_out_0_shape_w_const_u16 = 5;
static const ai_u16 conv2d_24_t_out_0_shape_h_const_u16 = 5;

static const ai_u16 conv2d_25_t_in_0_shape_w_const_u16 = 5;
static const ai_u16 conv2d_25_t_in_0_shape_h_const_u16 = 5;
static const ai_u16 conv2d_25_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_25_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_25_t_in_0_shape_ch_const_u16 = 288;
static const ai_u16 conv2d_25_t_out_0_shape_ch_const_u16 = 288;
static const ai_i8 conv2d_25_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_25_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_25_t_in_0_fmt_scale_const_f32 = 0.003411282319575548f;
static const ai_float conv2d_25_t_out_0_fmt_scale_const_f32 = 0.0020968448370695114f;
static const ai_float conv2d_25_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0027581907343119383f, 0.002580654341727495f, 0.0015516160055994987f, 0.0012578604510053992f, 0.0029583987779915333f, 0.0018308206927031279f, 0.00245169410482049f, 0.0019487093668431044f, 0.0022907620295882225f, 0.002779609989374876f, 0.0016693664947524667f, 0.0017826072871685028f, 0.0029966444708406925f, 0.0023392559960484505f, 0.0018084273906424642f, 0.0028303824365139008f, 0.0015571641270071268f, 0.0019075751770287752f, 0.00271406932733953f, 0.0013089304557070136f, 0.0025305040180683136f, 0.0020211513619869947f, 0.0018883025040850043f, 0.00210942211560905f, 0.0015250927535817027f, 0.002292369492352009f, 0.002021856838837266f, 0.002629039343446493f, 0.002638170961290598f, 0.0019700846169143915f, 0.0022113053128123283f, 0.002424938604235649f, 0.0025964914821088314f, 0.0014627769123762846f, 0.0027139366138726473f, 0.001050120103172958f, 0.0012244719546288252f, 0.0017005185363814235f, 0.001763362786732614f, 0.0019088414264842868f, 0.001963916467502713f, 0.0037512797862291336f, 0.002563315676525235f, 0.0016623790143057704f, 0.0022264306899160147f, 0.0023176108952611685f, 0.0024771629832684994f, 0.0025745166931301355f, 0.0011132418876513839f, 0.0024777876678854227f, 0.0015790489269420505f, 0.001969400327652693f, 0.0013617968652397394f, 0.001928643323481083f, 0.001376140397042036f, 0.0027760358061641455f, 0.0023072201292961836f, 0.00151804368942976f, 0.0015727789141237736f, 0.0018869831692427397f, 0.0017340648919343948f, 0.0013142944080755115f, 0.0014046506257727742f, 0.0019459519535303116f, 0.002935538301244378f, 0.0016932690050452948f, 0.0019915958400815725f, 0.002072496572509408f, 0.0014973822981119156f, 0.0021670209243893623f, 0.0015856040408834815f, 0.0018898998387157917f, 0.00263445102609694f, 0.0020817930344492197f, 0.0016319395508617163f, 0.0021066865883767605f, 0.001842128811404109f, 0.0026225128676742315f, 0.0017152804648503661f, 0.002111180918291211f, 0.00285855564288795f, 0.0016578332288190722f, 0.0022491510026156902f, 0.0020485573913902044f, 0.002932674717158079f, 0.002066791756078601f, 0.00204897066578269f, 0.002195952693000436f, 0.0019705856684595346f, 0.001732202828861773f, 0.0017983586294576526f, 0.0016921324422582984f, 0.0021325065754354f, 0.0016007982194423676f, 0.001928093028254807f, 0.001966180745512247f, 0.0022635788191109896f, 0.0018917240668088198f, 0.0018831809284165502f, 0.00226711668074131f, 0.0029591794591397047f, 0.002691191853955388f, 0.001966031501069665f, 0.002047671005129814f, 0.0015719664515927434f, 0.001705027767457068f, 0.0028789963107556105f, 0.0016988404095172882f, 0.0018361990805715322f, 0.0018496733391657472f, 0.002561308443546295f, 0.0026349136605858803f, 0.0015110253589227796f, 0.0023819219786673784f, 0.003113283310085535f, 0.007587182801216841f, 0.002375611336901784f, 0.0014932058984413743f, 0.0017334625590592623f, 0.0016536739422008395f, 0.0018194741569459438f, 0.0021842143032699823f, 0.0030462571885436773f, 0.0032712214160710573f, 0.001732989796437323f, 0.0018225940875709057f, 0.0021627279929816723f, 0.0017256977735087276f, 0.0022764892783015966f, 0.0018410891061648726f, 0.0028911030385643244f, 0.0019548200070858f, 0.0017653494141995907f, 0.002022250322625041f, 0.0017916428623721004f, 0.0034758790861815214f, 0.002068464644253254f, 0.0023803783114999533f, 0.0025041187182068825f, 0.0018761659739539027f, 0.002209013095125556f, 0.0030384454876184464f, 0.0018268870189785957f, 0.0019154894398525357f, 0.0016407481161877513f, 0.001488403300754726f, 0.0016196054639294744f, 0.0018691997975111008f, 0.0021055194083601236f, 0.0017305089859291911f, 0.001554106012918055f, 0.0012979588937014341f, 0.0024585381615906954f, 0.0012656463077291846f, 0.0020695924758911133f, 0.00213414803147316f, 0.0016402109758928418f, 0.001594078028574586f, 0.001106166746467352f, 0.001235505798831582f, 0.001530861365608871f, 0.001417172490619123f, 0.0019966396503150463f, 0.0015615852316841483f, 0.0020006108097732067f, 0.0015136920846998692f, 0.0025988765992224216f, 0.0012536488939076662f, 0.001992916688323021f, 0.0017212176462635398f, 0.0020391056314110756f, 0.0016415020218119025f, 0.001881518168374896f, 0.0016218861564993858f, 0.0012563426280394197f, 0.0019769903738051653f, 0.001674053492024541f, 0.0022213461343199015f, 0.0024727587588131428f, 0.0017356799216941f, 0.0016379598528146744f, 0.0020439443178474903f, 0.0017478378722444177f, 0.0015628047985956073f, 0.0027387146838009357f, 0.0028411683160811663f, 0.0013252942590042949f, 0.0017458520596846938f, 0.0019865769427269697f, 0.001975562423467636f, 0.0015746736899018288f, 0.001454031909815967f, 0.0027384816203266382f, 0.001687315059825778f, 0.002404302591457963f, 0.002642347477376461f, 0.0024263898376375437f, 0.0017170681385323405f, 0.0024443399161100388f, 0.0018389745382592082f, 0.002057654783129692f, 0.0037113300058990717f, 0.001829566084779799f, 0.002619005274027586f, 0.0018593712011352181f, 0.0017591931391507387f, 0.001484125037677586f, 0.001826962921768427f, 0.003319781506434083f, 0.002289667958393693f, 0.001219790312461555f, 0.002336971228942275f, 0.0020739680621773005f, 0.002424246398732066f, 0.0020636289846152067f, 0.0020184798631817102f, 0.0020906811114400625f, 0.0020842580124735832f, 0.0021185490768402815f, 0.002225659554824233f, 0.001596819725818932f, 0.0010755750117823482f, 0.0023628580383956432f, 0.0024667405523359776f, 0.00395566038787365f, 0.0015268020797520876f, 0.0012912852689623833f, 0.00441736401990056f, 0.002094307215884328f, 0.001668176962994039f, 0.0018399805994704366f, 0.0019928168039768934f, 0.001321627525612712f, 0.0026411928702145815f, 0.0020179555285722017f, 0.001406772993505001f, 0.0016375925624743104f, 0.0019588861614465714f, 0.001609624712727964f, 0.0018225080566480756f, 0.0013736682012677193f, 0.0022816513665020466f, 0.001905542565509677f, 0.002228051656857133f, 0.0022746529430150986f, 0.002675799885764718f, 0.002261193236336112f, 0.002102088648825884f, 0.0015568649396300316f, 0.001226188731379807f, 0.002382985781878233f, 0.0019235977670177817f, 0.002062443410977721f, 0.001823068130761385f, 0.001506297616288066f, 0.002059070859104395f, 0.0014104964211583138f, 0.0019459897885099053f, 0.002427560044452548f, 0.0015629252884536982f, 0.0021812773775309324f, 0.002528298180550337f, 0.0013615700881928205f, 0.0019530726131051779f, 0.0021227968391031027f, 0.0034297655802220106f, 0.0012332671321928501f, 0.002866141963750124f, 0.00134831084869802f, 0.001811179448850453f, 0.0018092714017257094f, 0.0016989819705486298f, 0.0016958448104560375f, 0.0016456585144624114f, 0.0018734807381406426f, 0.0018355288775637746f, 0.004021867644041777f, 0.001323001109994948f, 0.002173843327909708f, 0.0030252470169216394f, 0.0015279604122042656f, 0.0028022045735269785f, 0.00182839366607368f, 0.002013920806348324f, 0.001137438928708434f, 0.002084847539663315f, 0.0014335010200738907f, 0.0015787557931616902f);
static const ai_layer_format_type conv2d_25_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_26_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_26_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_26_pad_before_t_in_0_shape_h_const_u32 = 5;

static const ai_u16 conv2d_26_t_in_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_26_t_in_0_shape_h_const_u16 = 7;
static const ai_u16 conv2d_26_t_in_0_shape_ch_const_u16 = 288;
static const ai_u16 conv2d_26_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_26_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_26_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_26_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_26_t_in_0_fmt_scale_const_f32 = 0.0020968448370695114f;
static const ai_float conv2d_26_t_out_0_fmt_scale_const_f32 = 0.002392140682786703f;
static const ai_float conv2d_26_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0033998717553913593f, 0.007904672995209694f, 0.003610107349231839f, 0.003398618893697858f, 0.0066346460953354836f, 0.0033615909051150084f, 0.005070327315479517f, 0.005329531151801348f, 0.007195565849542618f, 0.005912612192332745f, 0.0034595513716340065f, 0.0049417829141020775f, 0.009743883274495602f, 0.006408303510397673f, 0.007993699051439762f, 0.009059333242475986f, 0.004245628137141466f, 0.0035647531040012836f, 0.01008542999625206f, 0.0030917455442249775f, 0.010714671574532986f, 0.00712900934740901f, 0.0045484574511647224f, 0.009098435752093792f, 0.0054296995513141155f, 0.009250293485820293f, 0.005966086406260729f, 0.003708845004439354f, 0.005908084101974964f, 0.006344986148178577f, 0.00686423247680068f, 0.007908042520284653f, 0.008038804866373539f, 0.004280161578208208f, 0.00937596894800663f, 0.004395976662635803f, 0.011586790904402733f, 0.004863463807851076f, 0.0064564114436507225f, 0.006793598178774118f, 0.004365968517959118f, 0.011751838959753513f, 0.005009536165744066f, 0.0029410107526928186f, 0.0061362385749816895f, 0.00951020885258913f, 0.009104541502892971f, 0.006924161221832037f, 0.003742374014109373f, 0.010561266914010048f, 0.008014192804694176f, 0.007408980280160904f, 0.005796300247311592f, 0.007157888729125261f, 0.007200498599559069f, 0.007809617556631565f, 0.006002852227538824f, 0.004211912397295237f, 0.004123130813241005f, 0.007797806523740292f, 0.009466576389968395f, 0.004225931596010923f, 0.00682345125824213f, 0.005440376698970795f, 0.012102282606065273f, 0.005452124401926994f, 0.004117895849049091f, 0.005917520262300968f, 0.00519322557374835f, 0.005132253281772137f, 0.006700269877910614f, 0.007873878814280033f, 0.006509940139949322f, 0.004045405890792608f, 0.006059183739125729f, 0.005306389648467302f, 0.004936802666634321f, 0.007462493609637022f, 0.006833879742771387f, 0.006751071196049452f, 0.0062429411336779594f, 0.005300928372889757f, 0.005148247815668583f, 0.004780258983373642f, 0.008780509233474731f, 0.00981571152806282f, 0.006940475199371576f, 0.009570387192070484f, 0.004054648336023092f, 0.004107499960809946f, 0.003693103091791272f, 0.00879939366132021f, 0.004841323476284742f, 0.007033695466816425f, 0.004381299018859863f, 0.008542263880372047f, 0.007928626611828804f, 0.004025454632937908f, 0.00779394805431366f, 0.0042235651053488255f, 0.006517986301332712f, 0.009601601399481297f, 0.004627765156328678f, 0.007757748011499643f, 0.0038339521270245314f, 0.007135160733014345f, 0.008272863924503326f, 0.004739995114505291f, 0.004594366066157818f, 0.007130342070013285f, 0.006344780325889587f, 0.008790300227701664f, 0.008359530940651894f, 0.007897439412772655f, 0.006301208399236202f, 0.003271350171416998f, 0.004980050027370453f, 0.005910590756684542f, 0.0060094911605119705f, 0.009763051755726337f, 0.004815336782485247f, 0.005401251371949911f, 0.006464678794145584f, 0.006335926707834005f, 0.003478909144178033f, 0.008222585543990135f, 0.007882327772676945f, 0.0037123814690858126f, 0.005224835593253374f, 0.0070863040164113045f, 0.007097019348293543f, 0.006045519839972258f, 0.004444832447916269f, 0.007283568382263184f, 0.007635250221937895f, 0.004987113643437624f, 0.0057175057008862495f, 0.010674318298697472f, 0.006078444886952639f, 0.006673175375908613f, 0.006897417828440666f, 0.00555478036403656f, 0.004182680044323206f, 0.00678999675437808f, 0.008069412782788277f, 0.005062696989625692f, 0.007694755680859089f, 0.003437083214521408f, 0.003652045503258705f, 0.0029160657431930304f, 0.005442721303552389f, 0.0070210471749305725f, 0.004760678391903639f, 0.004412579350173473f, 0.00691973278298974f, 0.008395805954933167f, 0.007858405821025372f, 0.005112692713737488f, 0.009713830426335335f, 0.007293859031051397f, 0.00663022231310606f, 0.005073494743555784f, 0.007152748294174671f, 0.004882628098130226f, 0.003753627883270383f, 0.006473027169704437f, 0.0066012050956487656f, 0.00672987662255764f, 0.005492419935762882f, 0.004463579040020704f, 0.004853003658354282f, 0.006756845861673355f, 0.004683256149291992f, 0.004243318922817707f, 0.0040294574573636055f, 0.004339942242950201f, 0.006141787860542536f, 0.005566366016864777f, 0.005931518506258726f, 0.005232952069491148f, 0.006201510317623615f, 0.006028350442647934f, 0.0044586691074073315f, 0.0073647755198180676f, 0.005022361408919096f, 0.007940716110169888f, 0.004366053733974695f, 0.004229705780744553f, 0.00276862527243793f, 0.007723103277385235f, 0.0031324748415499926f, 0.004140668548643589f, 0.0058493418619036674f, 0.004866430535912514f, 0.005942023824900389f, 0.006593095138669014f, 0.009886040352284908f, 0.007884780876338482f, 0.008731795474886894f, 0.007529459893703461f, 0.006431313697248697f, 0.0034628913272172213f, 0.006726991385221481f, 0.007753775455057621f, 0.0054716323502361774f, 0.005446871742606163f, 0.005916922818869352f, 0.0033483311999589205f, 0.00850663147866726f, 0.005944399628788233f, 0.004323004744946957f, 0.007404681295156479f, 0.003697353648021817f, 0.003740346059203148f, 0.0027727282140403986f, 0.005747064016759396f, 0.0033410792239010334f, 0.007390165701508522f, 0.008862191811203957f, 0.007880992256104946f, 0.004081853199750185f, 0.0096212113276124f, 0.01332743652164936f, 0.0074257515370845795f, 0.006404539570212364f, 0.00588417612016201f, 0.0060594575479626656f, 0.008974890224635601f, 0.005348083563148975f, 0.006227235309779644f, 0.0033634162973612547f, 0.008428165689110756f, 0.004360818304121494f, 0.008845780976116657f, 0.007984105497598648f, 0.0068881879560649395f, 0.00906866043806076f, 0.006317431107163429f, 0.004200613126158714f, 0.007907296530902386f, 0.004772514570504427f, 0.007060958072543144f, 0.005220702849328518f, 0.009934022091329098f, 0.00574033847078681f, 0.006027744151651859f, 0.003099149791523814f, 0.00575816398486495f, 0.0070233214646577835f, 0.007223739288747311f, 0.006475428584963083f, 0.007697350811213255f, 0.006743241101503372f, 0.004124328959733248f, 0.0068508037365973f, 0.009341729804873466f, 0.007577107287943363f, 0.006297576241195202f, 0.004721116274595261f, 0.008176467381417751f, 0.0067924861796200275f, 0.0054511805064976215f, 0.003546361345797777f, 0.0039914753288030624f, 0.006723137106746435f, 0.00983336940407753f, 0.00418555224314332f, 0.004910612944513559f, 0.008356407284736633f, 0.007037010043859482f, 0.006397737190127373f, 0.008300605230033398f, 0.004095924086868763f, 0.006976970937103033f, 0.007850561290979385f, 0.00741222919896245f, 0.004003535490483046f, 0.005868417676538229f, 0.006872337311506271f, 0.00955645740032196f, 0.005240269470959902f, 0.008604123257100582f, 0.004784936551004648f, 0.006176781374961138f, 0.0051972391083836555f, 0.00419890321791172f, 0.005522547289729118f, 0.005931779742240906f);
static const ai_u16 conv2d_26_t_out_0_shape_w_const_u16 = 5;
static const ai_u16 conv2d_26_t_out_0_shape_h_const_u16 = 5;

static const ai_u16 conv2d_27_t_in_0_shape_w_const_u16 = 5;
static const ai_u16 conv2d_27_t_in_0_shape_h_const_u16 = 5;
static const ai_u16 conv2d_27_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_27_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_27_t_in_0_shape_ch_const_u16 = 288;
static const ai_u16 conv2d_27_t_out_0_shape_ch_const_u16 = 288;
static const ai_i8 conv2d_27_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_27_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_27_t_in_0_fmt_scale_const_f32 = 0.002392140682786703f;
static const ai_float conv2d_27_t_out_0_fmt_scale_const_f32 = 0.002094843192026019f;
static const ai_float conv2d_27_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.001973807578906417f, 0.0025584185495972633f, 0.0020307765807956457f, 0.0016852904809638858f, 0.0017535811057314277f, 0.0018477087141945958f, 0.0025762871373444796f, 0.002486057812348008f, 0.001925827469676733f, 0.00397692434489727f, 0.002563306363299489f, 0.0016211727634072304f, 0.0017997821560129523f, 0.0015583537751808763f, 0.0017706499202176929f, 0.0030946629121899605f, 0.002021073130890727f, 0.0018821557750925422f, 0.0037297026719897985f, 0.002700192155316472f, 0.002495585475116968f, 0.0018394759390503168f, 0.0018267256673425436f, 0.002099528443068266f, 0.0018336210632696748f, 0.0018544156337156892f, 0.0022423171903938055f, 0.0024917179252952337f, 0.0019367748172953725f, 0.002878190018236637f, 0.0019487221725285053f, 0.0017070829635486007f, 0.00202633673325181f, 0.0019173796754330397f, 0.0017189764184877276f, 0.0025914907455444336f, 0.001665332354605198f, 0.001739822793751955f, 0.0020948792807757854f, 0.0031900787726044655f, 0.0020954590290784836f, 0.0024709340650588274f, 0.0022750988136976957f, 0.0027337667997926474f, 0.0030151193495839834f, 0.0026200138963758945f, 0.0017743386561051011f, 0.0016392623074352741f, 0.0026752406265586615f, 0.0029397474136203527f, 0.0020888117142021656f, 0.0018959813751280308f, 0.0016072156140580773f, 0.001981253968551755f, 0.00229035341180861f, 0.0015936334384605289f, 0.0022389290388673544f, 0.0019660203251987696f, 0.0018465156899765134f, 0.0019875704310834408f, 0.002055580262094736f, 0.0019877352751791477f, 0.0014720445033162832f, 0.0024202577769756317f, 0.0023756008595228195f, 0.0021885947789996862f, 0.002179470844566822f, 0.00337823829613626f, 0.00228599039837718f, 0.002120453165844083f, 0.0012549936072900891f, 0.0020172896329313517f, 0.0031883097253739834f, 0.002036056946963072f, 0.002085210056975484f, 0.0016251534689217806f, 0.002430568914860487f, 0.001455481629818678f, 0.0016908298712223768f, 0.002865859540179372f, 0.0011892784386873245f, 0.003793079638853669f, 0.0017336970195174217f, 0.002553757047280669f, 0.0019085683161392808f, 0.0025608984287828207f, 0.0034285159781575203f, 0.002088697161525488f, 0.0021702898666262627f, 0.0023065863642841578f, 0.002623795298859477f, 0.0022010132670402527f, 0.001455932855606079f, 0.0021396384108811617f, 0.0019364628242328763f, 0.002239573048427701f, 0.0018443617736920714f, 0.001379743218421936f, 0.0023847492411732674f, 0.0018251624424010515f, 0.0011246531503275037f, 0.002411964815109968f, 0.0017719253664836287f, 0.0027038217522203922f, 0.001854485017247498f, 0.003442274872213602f, 0.002029543509706855f, 0.0025830348022282124f, 0.0021655713208019733f, 0.001933765597641468f, 0.0017907846486195922f, 0.0023527650628238916f, 0.0034311418421566486f, 0.00296718324534595f, 0.0022340109571814537f, 0.004522961098700762f, 0.001864661811850965f, 0.0037311953492462635f, 0.0020424213726073503f, 0.001804581144824624f, 0.0022688922472298145f, 0.0021900038700550795f, 0.0023479051887989044f, 0.0026410433929413557f, 0.0019237708766013384f, 0.0017441364470869303f, 0.002287793904542923f, 0.0021504515316337347f, 0.0030399495735764503f, 0.0030099269933998585f, 0.0025536336470395327f, 0.002006287220865488f, 0.0024016224779188633f, 0.00281071406789124f, 0.0023286505602300167f, 0.0028279602993279696f, 0.0025971722789108753f, 0.001591519801877439f, 0.0027074047829955816f, 0.00212314841337502f, 0.001506111235357821f, 0.0022122308146208525f, 0.002638715784996748f, 0.002233208157122135f, 0.0015550145180895925f, 0.0023633665405213833f, 0.0016487769316881895f, 0.002652910305187106f, 0.002912587020546198f, 0.0028145224787294865f, 0.002230534330010414f, 0.0016009457176551223f, 0.0029594041407108307f, 0.0027820104733109474f, 0.0033863524440675974f, 0.0025152016896754503f, 0.0021735045593231916f, 0.001711433520540595f, 0.0018703308887779713f, 0.002552290679886937f, 0.0015317248180508614f, 0.0014657479478046298f, 0.0021513672545552254f, 0.001750410650856793f, 0.0015235270839184523f, 0.0028944960795342922f, 0.002721039578318596f, 0.002031811513006687f, 0.0020254955161362886f, 0.0017430150182917714f, 0.0026335713919252157f, 0.002612127922475338f, 0.002161122625693679f, 0.00205207453109324f, 0.0029370032716542482f, 0.001970314420759678f, 0.0013689788756892085f, 0.0021686796098947525f, 0.0019939893390983343f, 0.002179471543058753f, 0.00172421894967556f, 0.002002197317779064f, 0.00212876801379025f, 0.002646715845912695f, 0.0015546218492090702f, 0.002475661225616932f, 0.0020077393855899572f, 0.0020930978935211897f, 0.0013939070049673319f, 0.002686917781829834f, 0.0014330805279314518f, 0.0024162756744772196f, 0.0012117794249206781f, 0.0021167860832065344f, 0.0028350430075079203f, 0.0028608697466552258f, 0.0022147954441607f, 0.0023721398320049047f, 0.0016159048536792397f, 0.002179844072088599f, 0.00241510639898479f, 0.003021582728251815f, 0.0025868494994938374f, 0.0024105263873934746f, 0.0024282049853354692f, 0.0020177634432911873f, 0.002591713098809123f, 0.0020384604576975107f, 0.002302185632288456f, 0.0035555115900933743f, 0.0013005428481847048f, 0.002996183233335614f, 0.004661386366933584f, 0.002477192087098956f, 0.0027565008495002985f, 0.002796800108626485f, 0.0018255526665598154f, 0.003340667113661766f, 0.0016759625868871808f, 0.001932094106450677f, 0.002801238326355815f, 0.0024448770564049482f, 0.0017976490780711174f, 0.003729448653757572f, 0.0024527187924832106f, 0.0013361491728574038f, 0.001665968680754304f, 0.00195036840159446f, 0.0023798916954547167f, 0.0016917886678129435f, 0.0015701217344030738f, 0.0016987195704132318f, 0.002394650364294648f, 0.002174591878429055f, 0.002415713854134083f, 0.0021615929435938597f, 0.00331684248521924f, 0.0019740343559533358f, 0.0014875439228489995f, 0.0029645489994436502f, 0.001523282378911972f, 0.002237889217212796f, 0.0023376045282930136f, 0.0029780957847833633f, 0.001895614666864276f, 0.002089990070089698f, 0.001947136246599257f, 0.004107585176825523f, 0.0015404579462483525f, 0.0020010066218674183f, 0.001866536564193666f, 0.0017484449781477451f, 0.0037155128084123135f, 0.0017106698360294104f, 0.0019317338010296226f, 0.0013590131420642138f, 0.00249800574965775f, 0.0022235140204429626f, 0.0018610681872814894f, 0.001870875246822834f, 0.0027320722583681345f, 0.001837241230532527f, 0.00252602924592793f, 0.0024286748375743628f, 0.0016445111250504851f, 0.0015207072719931602f, 0.0025938155595213175f, 0.0017491590697318316f, 0.0038062934763729572f, 0.0012004947056993842f, 0.0017791130812838674f, 0.0017334012081846595f, 0.001876516966149211f, 0.0015533752739429474f, 0.002824833383783698f, 0.0026148443575948477f, 0.002168149221688509f, 0.002086399821564555f, 0.0015376840019598603f, 0.0022113467566668987f, 0.001807561842724681f, 0.0027036061510443687f, 0.0031136441975831985f, 0.001963106682524085f, 0.001813792739994824f, 0.0019499638583511114f, 0.0017770378617569804f, 0.002732164692133665f);
static const ai_layer_format_type conv2d_27_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_28_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_28_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_28_pad_before_t_in_0_shape_h_const_u32 = 5;

static const ai_u16 conv2d_28_t_in_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_28_t_in_0_shape_h_const_u16 = 7;
static const ai_u16 conv2d_28_t_in_0_shape_ch_const_u16 = 288;
static const ai_u16 conv2d_28_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_28_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_28_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_28_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_28_t_in_0_fmt_scale_const_f32 = 0.002094843192026019f;
static const ai_float conv2d_28_t_out_0_fmt_scale_const_f32 = 0.0020765908993780613f;
static const ai_float conv2d_28_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.005779944825917482f, 0.0033483461011201143f, 0.0040567233227193356f, 0.007233654614537954f, 0.008112980984151363f, 0.006082765758037567f, 0.003731254255399108f, 0.007894732058048248f, 0.007236524019390345f, 0.007848306559026241f, 0.005420936271548271f, 0.0042881746776402f, 0.005123208276927471f, 0.005513826385140419f, 0.003597823204472661f, 0.009530487470328808f, 0.0029660698492079973f, 0.00405362993478775f, 0.006282321177423f, 0.006623656488955021f, 0.008112026378512383f, 0.0031600832007825375f, 0.003428695723414421f, 0.004485015757381916f, 0.006912957411259413f, 0.008572683669626713f, 0.004841128829866648f, 0.009943503886461258f, 0.007073716726154089f, 0.010408868081867695f, 0.003582743462175131f, 0.0049636163748800755f, 0.005472173448652029f, 0.003054759930819273f, 0.007278257980942726f, 0.006376157980412245f, 0.00497391214594245f, 0.00455431779846549f, 0.006974635645747185f, 0.006903862580657005f, 0.0052823335863649845f, 0.003536948235705495f, 0.008610866963863373f, 0.0062801227904856205f, 0.010551328770816326f, 0.0072581153362989426f, 0.005417870823293924f, 0.003300891723483801f, 0.008011305704712868f, 0.007506506517529488f, 0.004384651780128479f, 0.00881904549896717f, 0.0031189622823148966f, 0.007322735618799925f, 0.0037280411925166845f, 0.003157178172841668f, 0.007076377049088478f, 0.009520984254777431f, 0.0038194500375539064f, 0.007031625602394342f, 0.005080737639218569f, 0.005959593690931797f, 0.002991378540173173f, 0.007440036628395319f, 0.00891097728163004f, 0.0061917901039123535f, 0.00281329033896327f, 0.006659972481429577f, 0.006144816521555185f, 0.006115802098065615f, 0.005604166071861982f, 0.007169800344854593f, 0.007238849997520447f, 0.004765285644680262f, 0.007023148238658905f, 0.0042230915278196335f, 0.004808501340448856f, 0.004983828868716955f, 0.009646497666835785f, 0.003264394123107195f, 0.003358751069754362f, 0.004671263974159956f, 0.005170939955860376f, 0.004921048413962126f, 0.003275978611782193f, 0.004721182864159346f, 0.007914590649306774f, 0.004276163410395384f, 0.008166605606675148f, 0.006191161461174488f, 0.007864346727728844f, 0.007478287909179926f, 0.004463591612875462f, 0.0038127105217427015f, 0.004792109131813049f, 0.004416946787387133f, 0.004188348539173603f, 0.009093528613448143f, 0.004000172484666109f, 0.005312817171216011f, 0.004329599905759096f, 0.006587835494428873f, 0.006235704757273197f, 0.007165730930864811f, 0.005866562016308308f, 0.00396428257226944f, 0.006026489194482565f, 0.004767870530486107f, 0.008105886168777943f, 0.004484574776142836f, 0.007218069396913052f, 0.00693539110943675f, 0.006911745760589838f, 0.004243362229317427f, 0.006962477695196867f, 0.003343078540638089f, 0.006581039633601904f, 0.010848037898540497f, 0.006313750986009836f, 0.005241537932306528f, 0.0076192705892026424f, 0.008012779988348484f, 0.006907738279551268f, 0.008984372019767761f, 0.008239085786044598f, 0.006251271348446608f, 0.00483791995793581f, 0.005253235809504986f, 0.004936108365654945f, 0.006194076966494322f, 0.00837953295558691f, 0.004488147329539061f, 0.007297798991203308f, 0.00866742618381977f, 0.00662139430642128f, 0.005239580292254686f, 0.007397875189781189f, 0.005655511748045683f, 0.005478209350258112f, 0.004764611832797527f, 0.005078636575490236f, 0.002939915284514427f, 0.005965698044747114f, 0.0040013473480939865f, 0.007598720490932465f, 0.005561996251344681f, 0.004610842559486628f, 0.00796677265316248f, 0.004053038079291582f, 0.006682665087282658f, 0.004647640511393547f, 0.0066691297106444836f, 0.007207273505628109f, 0.0052899811416864395f, 0.0065338220447301865f, 0.0046822489239275455f, 0.005107787437736988f, 0.006248112767934799f, 0.006062981206923723f, 0.007363562937825918f, 0.0036280800122767687f, 0.006910555064678192f, 0.007608584128320217f, 0.003156757913529873f, 0.002514335559681058f, 0.007393620442599058f, 0.007374896202236414f, 0.004321020096540451f, 0.00441776541993022f, 0.0033937704283744097f, 0.00731823081150651f, 0.007798740640282631f, 0.00673669995740056f, 0.006558737717568874f, 0.00696926936507225f, 0.006157238502055407f, 0.004543546121567488f, 0.0025468782987445593f, 0.007495695259422064f, 0.004802186042070389f, 0.006569733377546072f, 0.003491726005449891f, 0.0093957195058465f, 0.007255876436829567f, 0.005668595898896456f, 0.0042135450057685375f, 0.004207391757518053f, 0.007847229018807411f, 0.0095105841755867f, 0.009782835841178894f, 0.006780954077839851f, 0.008238437585532665f, 0.004207239951938391f, 0.004957708530128002f, 0.006358577404171228f, 0.010349245741963387f, 0.003467631759122014f, 0.005584443919360638f, 0.008590986020863056f, 0.006415196694433689f, 0.004353335592895746f, 0.00715861888602376f, 0.0058195688761770725f, 0.006481508258730173f, 0.006918520200997591f, 0.0056544640101492405f, 0.009425347670912743f, 0.010812003165483475f, 0.004541134927421808f, 0.007645661476999521f, 0.005203011445701122f, 0.005601982586085796f, 0.0067130341194570065f, 0.005758041515946388f, 0.009569477289915085f, 0.0064485808834433556f, 0.0034450553357601166f, 0.010156572796404362f, 0.007933462969958782f, 0.00534763652831316f, 0.00677492143586278f, 0.009203913621604443f, 0.007415656466037035f, 0.006926939357072115f, 0.00639727246016264f, 0.004651749040931463f, 0.004599892068654299f, 0.006406559143215418f, 0.0030663886573165655f, 0.006612516473978758f, 0.006760101765394211f, 0.007496855687350035f, 0.0059773875400424f, 0.003991615492850542f, 0.007888153195381165f, 0.007088254671543837f, 0.005509654525667429f, 0.0072993622161448f, 0.0035159368999302387f, 0.008508509956300259f, 0.008710529655218124f, 0.0065251681953668594f, 0.008811966516077518f, 0.007355178706347942f, 0.004639959894120693f, 0.007475379388779402f, 0.004166493657976389f, 0.008575433865189552f, 0.003663986921310425f, 0.006771925836801529f, 0.004744493868201971f, 0.004330884665250778f, 0.008017778396606445f, 0.003040923271328211f, 0.00501353619620204f, 0.003565863473340869f, 0.009117496199905872f, 0.007644311524927616f, 0.006329794880002737f, 0.0050322930328547955f, 0.007939052768051624f, 0.00377110717818141f, 0.00501553388312459f, 0.006532604340463877f, 0.0049143387004733086f, 0.0057046483270823956f, 0.004062597174197435f, 0.003990091383457184f, 0.006761804688721895f, 0.005093690473586321f, 0.006350691895931959f, 0.0070649986155331135f, 0.0031681545078754425f, 0.005465628579258919f, 0.00785091146826744f, 0.006309131626039743f, 0.010001852177083492f, 0.005079294554889202f, 0.004336945246905088f, 0.007259317208081484f, 0.0032992642372846603f, 0.006861818954348564f, 0.00459845969453454f, 0.009245466440916061f, 0.006779402960091829f, 0.004866832867264748f, 0.003268000204116106f, 0.007978850975632668f);
static const ai_u16 conv2d_28_t_out_0_shape_w_const_u16 = 5;
static const ai_u16 conv2d_28_t_out_0_shape_h_const_u16 = 5;

static const ai_u16 conv2d_29_t_in_0_shape_w_const_u16 = 5;
static const ai_u16 conv2d_29_t_in_0_shape_h_const_u16 = 5;
static const ai_u16 conv2d_29_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_29_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_29_t_in_0_shape_ch_const_u16 = 288;
static const ai_u16 conv2d_29_t_out_0_shape_ch_const_u16 = 288;
static const ai_i8 conv2d_29_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_29_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_29_t_in_0_fmt_scale_const_f32 = 0.0020765908993780613f;
static const ai_float conv2d_29_t_out_0_fmt_scale_const_f32 = 0.0015556998550891876f;
static const ai_float conv2d_29_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0015342636033892632f, 0.0017712761182338f, 0.0018030302599072456f, 0.0019500335911288857f, 0.00142268359195441f, 0.0025399343576282263f, 0.0014684164198115468f, 0.001605275203473866f, 0.003446209477260709f, 0.002073138952255249f, 0.0018040889408439398f, 0.004345915745943785f, 0.0018080343725159764f, 0.0021649093832820654f, 0.0017741521587595344f, 0.002681775949895382f, 0.0016728932969272137f, 0.002402623649686575f, 0.0015448536723852158f, 0.0020278929732739925f, 0.0018525875639170408f, 0.0020896384958177805f, 0.0034607904963195324f, 0.002074948977679014f, 0.0019007590599358082f, 0.00248503964394331f, 0.0016023587668314576f, 0.001659074448980391f, 0.0026605750899761915f, 0.0024744756519794464f, 0.0016337891574949026f, 0.0018692216835916042f, 0.001181779196485877f, 0.00219279364682734f, 0.001984623959288001f, 0.0020791657734662294f, 0.0015007145702838898f, 0.0014589548809453845f, 0.002183028729632497f, 0.0014843731187283993f, 0.002232055878266692f, 0.0026051229797303677f, 0.003350871382281184f, 0.0015701393131166697f, 0.0017090740147978067f, 0.0025064926594495773f, 0.0018981985049322248f, 0.0026060380041599274f, 0.0025971687864512205f, 0.0017461305251345038f, 0.001342597184702754f, 0.0023467126302421093f, 0.0018282749224454165f, 0.0023145899176597595f, 0.0029442966915667057f, 0.0013989838771522045f, 0.0019231614423915744f, 0.001990827964618802f, 0.0016438281163573265f, 0.0022861093748360872f, 0.0027021171990782022f, 0.002818060340359807f, 0.002162361517548561f, 0.0017633629031479359f, 0.002612857846543193f, 0.0015514737460762262f, 0.0022945620585232973f, 0.001708142808638513f, 0.0021354493219405413f, 0.0018434966914355755f, 0.004239257890731096f, 0.0015677633928135037f, 0.003097819397225976f, 0.002445017686113715f, 0.0025511784479022026f, 0.001670786296017468f, 0.002435339381918311f, 0.0028312434442341328f, 0.0016539511270821095f, 0.0021714873146265745f, 0.0019026428926736116f, 0.0016497810138389468f, 0.0029509561136364937f, 0.0022088417317718267f, 0.0015258410712704062f, 0.0018239556811749935f, 0.0024484179448336363f, 0.0030252134893089533f, 0.0024171373806893826f, 0.0020092958584427834f, 0.0024360190145671368f, 0.0019745773170143366f, 0.0016623010160401464f, 0.0029039441142231226f, 0.002713172696530819f, 0.001921714749187231f, 0.002166942460462451f, 0.002982574747875333f, 0.001991723431274295f, 0.002582677872851491f, 0.003233244875445962f, 0.0025703441351652145f, 0.002977167721837759f, 0.001634220709092915f, 0.002250069286674261f, 0.002308553783223033f, 0.00195628241635859f, 0.0029158839024603367f, 0.0016222464619204402f, 0.001919020083732903f, 0.0032883831299841404f, 0.002240539761260152f, 0.002608795650303364f, 0.0026452834717929363f, 0.0019249585457146168f, 0.002241929527372122f, 0.0023017132189124823f, 0.001181399798952043f, 0.0018275517504662275f, 0.0018155802972614765f, 0.002761752577498555f, 0.001697668805718422f, 0.0024955510161817074f, 0.0023801401257514954f, 0.0021430430933833122f, 0.0017748866230249405f, 0.0025766987819224596f, 0.0037020340096205473f, 0.0025460959877818823f, 0.0026018787175416946f, 0.0016762245213612914f, 0.0018633019644767046f, 0.002024958608672023f, 0.002200404182076454f, 0.0027958557475358248f, 0.002921266946941614f, 0.001271899207495153f, 0.0018893530359491706f, 0.001427129958756268f, 0.0021621931809931993f, 0.0017701521283015609f, 0.0021755979396402836f, 0.0024432449135929346f, 0.002926234155893326f, 0.0026032975874841213f, 0.0025939212646335363f, 0.0021328330039978027f, 0.0018381146946921945f, 0.0015520175220444798f, 0.005164304748177528f, 0.0023418832570314407f, 0.002010271418839693f, 0.002912632655352354f, 0.004089072812348604f, 0.002955388743430376f, 0.0023352589923888445f, 0.0016283390577882528f, 0.0011417401256039739f, 0.001996151637285948f, 0.0017211685189977288f, 0.0014451418537646532f, 0.001808158471249044f, 0.0025338290724903345f, 0.0019572076853364706f, 0.0015717329224571586f, 0.0023424469400197268f, 0.002054787240922451f, 0.0031535353045910597f, 0.0015027618501335382f, 0.0021005889866501093f, 0.0015208633849397302f, 0.0017888823058456182f, 0.0027988397050648928f, 0.002327467780560255f, 0.001449253992177546f, 0.0014585047028958797f, 0.002061028964817524f, 0.002148808678612113f, 0.0022213305346667767f, 0.0018988195806741714f, 0.0016506098909303546f, 0.003664689604192972f, 0.0022247324232012033f, 0.0026299874298274517f, 0.0019007083028554916f, 0.0021057985723018646f, 0.001767768175341189f, 0.002695194212719798f, 0.002697761170566082f, 0.0017769942060112953f, 0.002072347095236182f, 0.0023212633095681667f, 0.004559789318591356f, 0.0020633956883102655f, 0.002775141503661871f, 0.0024666693061590195f, 0.002383999992161989f, 0.0019243170972913504f, 0.002024193527176976f, 0.002282075583934784f, 0.001733636250719428f, 0.00412101810798049f, 0.0021671848371624947f, 0.002754820045083761f, 0.0028324967715889215f, 0.002240965608507395f, 0.0017543236026540399f, 0.0014527853345498443f, 0.002065372420474887f, 0.002023151842877269f, 0.0015347192529588938f, 0.002479770453646779f, 0.0033302106894552708f, 0.0029036744963377714f, 0.001984385075047612f, 0.0022365848999470472f, 0.0023444402031600475f, 0.002287400420755148f, 0.0022178313229233027f, 0.0015906685730442405f, 0.0014186919433996081f, 0.002076626755297184f, 0.0013071441790089011f, 0.001591420266777277f, 0.0020345430821180344f, 0.0024636450689285994f, 0.001944507472217083f, 0.003092628438025713f, 0.0028430523816496134f, 0.0019356297561898828f, 0.002435422269627452f, 0.002563801594078541f, 0.0017077444354072213f, 0.0017641430022194982f, 0.0027816498186439276f, 0.0025757625699043274f, 0.0027478239499032497f, 0.0022920090705156326f, 0.002562047215178609f, 0.0016642501577734947f, 0.002469061641022563f, 0.0028341731522232294f, 0.0023206148762255907f, 0.0017111279303207994f, 0.001598866074346006f, 0.003299992298707366f, 0.0021079869475215673f, 0.002512571634724736f, 0.00206311815418303f, 0.0012403016444295645f, 0.0015272562159225345f, 0.0016323827439919114f, 0.0025353925302624702f, 0.0015790092293173075f, 0.0030501908622682095f, 0.0024002683348953724f, 0.002086494816467166f, 0.0018901161383837461f, 0.0027352108154445887f, 0.0022267764434218407f, 0.002655471209436655f, 0.0020806463435292244f, 0.0010510087013244629f, 0.0019158725626766682f, 0.0015818077372387052f, 0.0021663163788616657f, 0.0026947956066578627f, 0.0031125363893806934f, 0.0020549732726067305f, 0.0025389594957232475f, 0.001953310100361705f, 0.0017367815598845482f, 0.0031440157908946276f, 0.0022670840844511986f, 0.003132881596684456f, 0.002218238776549697f, 0.0013109610881656408f, 0.0021884723100811243f, 0.002583226654678583f, 0.0021775010973215103f, 0.00250973436050117f, 0.0019248585449531674f, 0.003914056811481714f, 0.0019279555417597294f, 0.0030957309063524008f, 0.001706254086457193f, 0.0012791535118594766f, 0.002590482123196125f);
static const ai_layer_format_type conv2d_29_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_30_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_30_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_30_pad_before_t_in_0_shape_h_const_u32 = 5;

static const ai_u16 conv2d_30_t_in_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_30_t_in_0_shape_h_const_u16 = 7;
static const ai_u16 conv2d_30_t_in_0_shape_ch_const_u16 = 288;
static const ai_u16 conv2d_30_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_30_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_30_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_30_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_30_t_in_0_fmt_scale_const_f32 = 0.0015556998550891876f;
static const ai_float conv2d_30_t_out_0_fmt_scale_const_f32 = 0.001783824060112238f;
static const ai_float conv2d_30_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0028359629213809967f, 0.005119334906339645f, 0.00522179389372468f, 0.0030831668991595507f, 0.0032966709695756435f, 0.005614178720861673f, 0.004978413227945566f, 0.0021504743490368128f, 0.007719267159700394f, 0.005758899729698896f, 0.007341394200921059f, 0.007324310950934887f, 0.003804186126217246f, 0.003914214204996824f, 0.005024989601224661f, 0.0068471599370241165f, 0.005107569508254528f, 0.006043059751391411f, 0.0033182112965732813f, 0.00395951559767127f, 0.004754994995892048f, 0.006261772941797972f, 0.0055093965493142605f, 0.005695018917322159f, 0.0046547781676054f, 0.00464183883741498f, 0.005210799165070057f, 0.005095639731734991f, 0.006826979108154774f, 0.006866501644253731f, 0.005106133408844471f, 0.00538150779902935f, 0.0036346931010484695f, 0.0035653344821184874f, 0.004031669814139605f, 0.004751154221594334f, 0.004388611763715744f, 0.003963336814194918f, 0.005690267309546471f, 0.004385888110846281f, 0.007898376323282719f, 0.007499204017221928f, 0.009380181320011616f, 0.004066677298396826f, 0.0040703085251152515f, 0.0037286432925611734f, 0.0052869594655931f, 0.005344085860997438f, 0.005814952775835991f, 0.0046017286367714405f, 0.00371379591524601f, 0.007031071465462446f, 0.005975537002086639f, 0.005324927624315023f, 0.007331005297601223f, 0.00537990452721715f, 0.004907199647277594f, 0.0050481585785746574f, 0.0034091356210410595f, 0.005221241619437933f, 0.004500543233007193f, 0.004666908644139767f, 0.004395170137286186f, 0.00658915750682354f, 0.006101693492382765f, 0.0053038871847093105f, 0.006103631108999252f, 0.005388621706515551f, 0.004405537620186806f, 0.005964435636997223f, 0.005242096725851297f, 0.004210872109979391f, 0.007508497219532728f, 0.008179975673556328f, 0.0051206075586378574f, 0.0033620025496929884f, 0.006606229115277529f, 0.004751272965222597f, 0.002881881780922413f, 0.006798120215535164f, 0.006692442577332258f, 0.003832374233752489f, 0.008687681518495083f, 0.005292018875479698f, 0.003452611155807972f, 0.005610168911516666f, 0.008881707675755024f, 0.009022288955748081f, 0.008918067440390587f, 0.007709587924182415f, 0.007232739590108395f, 0.0088269067928195f, 0.0021689373534172773f, 0.006575313396751881f, 0.006495762150734663f, 0.0043725986033678055f, 0.006400765385478735f, 0.0038596498779952526f, 0.006137909833341837f, 0.007373821921646595f, 0.008657771162688732f, 0.0041504534892737865f, 0.009215664118528366f, 0.004273834638297558f, 0.006622806191444397f, 0.007319086696952581f, 0.005733173806220293f, 0.007754472084343433f, 0.005398091860115528f, 0.004791141487658024f, 0.007316050585359335f, 0.006207875907421112f, 0.007320651318877935f, 0.006293823476880789f, 0.00791305024176836f, 0.005667544435709715f, 0.003279257332906127f, 0.005217991769313812f, 0.006208679638803005f, 0.0025143714156001806f, 0.009653955698013306f, 0.003913922235369682f, 0.005964464042335749f, 0.007572399452328682f, 0.0055684614926576614f, 0.010175403207540512f, 0.011181691661477089f, 0.008810678496956825f, 0.006517970934510231f, 0.004215739201754332f, 0.0048543489538133144f, 0.004388722125440836f, 0.006950763054192066f, 0.007517213001847267f, 0.00594784040004015f, 0.004891124088317156f, 0.004096996039152145f, 0.00284135271795094f, 0.006057754158973694f, 0.0057923938147723675f, 0.003758394857868552f, 0.0061331684701144695f, 0.006321077235043049f, 0.003383152186870575f, 0.00634906766936183f, 0.004646657034754753f, 0.005841757636517286f, 0.005029964726418257f, 0.005279436707496643f, 0.008273010142147541f, 0.003698943881317973f, 0.006376610603183508f, 0.008136519230902195f, 0.007000519894063473f, 0.00544097600504756f, 0.0037332531064748764f, 0.005325619596987963f, 0.0031270598992705345f, 0.003931071609258652f, 0.004506027325987816f, 0.004950210452079773f, 0.004358567297458649f, 0.0070286765694618225f, 0.006426634732633829f, 0.0034537548199295998f, 0.007333939895033836f, 0.004433550406247377f, 0.0049832286313176155f, 0.005529331974685192f, 0.003534079296514392f, 0.00357699953019619f, 0.004787466488778591f, 0.006805162876844406f, 0.010251482017338276f, 0.004080597311258316f, 0.0033281573560088873f, 0.006587804760783911f, 0.006920140702277422f, 0.005308462772518396f, 0.004271453712135553f, 0.005038973409682512f, 0.007569034118205309f, 0.007269990164786577f, 0.009264080785214901f, 0.006208845414221287f, 0.005936425179243088f, 0.005276021547615528f, 0.0065763951279222965f, 0.006773488596081734f, 0.00465394975617528f, 0.004110897891223431f, 0.0043729208409786224f, 0.006004169583320618f, 0.0051521556451916695f, 0.005660815164446831f, 0.005586619488894939f, 0.00994131900370121f, 0.004301510285586119f, 0.005028583575040102f, 0.005271521396934986f, 0.005553934257477522f, 0.008405961096286774f, 0.005406097508966923f, 0.00699281133711338f, 0.0034025441855192184f, 0.0061738514341413975f, 0.0071664778515696526f, 0.004240476526319981f, 0.004927770234644413f, 0.005511168856173754f, 0.0030272998847067356f, 0.005038413684815168f, 0.007638947106897831f, 0.006862930953502655f, 0.003293686779215932f, 0.00789584033191204f, 0.0049091000109910965f, 0.004035378340631723f, 0.004263443406671286f, 0.003164007794111967f, 0.0038347758818417788f, 0.0049196272157132626f, 0.006020736880600452f, 0.006437249481678009f, 0.00525902584195137f, 0.0026026233099400997f, 0.005756829399615526f, 0.007933741435408592f, 0.006130981259047985f, 0.004257456865161657f, 0.008218702860176563f, 0.004973341710865498f, 0.00482981139793992f, 0.004683192353695631f, 0.006926769856363535f, 0.008455437608063221f, 0.007207796908915043f, 0.004236725624650717f, 0.004331785719841719f, 0.0026079686358571053f, 0.005081402137875557f, 0.005648975260555744f, 0.004368189722299576f, 0.0019022939959540963f, 0.004993425216525793f, 0.007602215278893709f, 0.005970390047878027f, 0.008151900954544544f, 0.005464125890284777f, 0.004148311447352171f, 0.006230417639017105f, 0.0026182434521615505f, 0.0033897580578923225f, 0.0039680348709225655f, 0.007233800832182169f, 0.005367294419556856f, 0.003279975149780512f, 0.004682507831603289f, 0.006215429399162531f, 0.006514369044452906f, 0.008725046180188656f, 0.00331738474778831f, 0.005196032114326954f, 0.0026482376269996166f, 0.004541152156889439f, 0.008441060781478882f, 0.004862007685005665f, 0.007172556594014168f, 0.005444440990686417f, 0.0037722871638834476f, 0.007027891464531422f, 0.004740335047245026f, 0.008627770468592644f, 0.0031233683694154024f, 0.005791061092168093f, 0.005175752565264702f, 0.004372843075543642f, 0.006399092264473438f, 0.00652289018034935f, 0.005033357068896294f, 0.005666584707796574f, 0.005372477695345879f, 0.005272591486573219f, 0.00521005317568779f, 0.006028336472809315f, 0.0033450527116656303f, 0.004081400111317635f, 0.005725421942770481f);
static const ai_u16 conv2d_30_t_out_0_shape_w_const_u16 = 5;
static const ai_u16 conv2d_30_t_out_0_shape_h_const_u16 = 5;

static const ai_u16 conv2d_31_t_in_0_shape_w_const_u16 = 5;
static const ai_u16 conv2d_31_t_in_0_shape_h_const_u16 = 5;
static const ai_u16 conv2d_31_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_31_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_31_t_in_0_shape_ch_const_u16 = 288;
static const ai_u16 conv2d_31_t_out_0_shape_ch_const_u16 = 288;
static const ai_i8 conv2d_31_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_31_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_31_t_in_0_fmt_scale_const_f32 = 0.001783824060112238f;
static const ai_float conv2d_31_t_out_0_fmt_scale_const_f32 = 0.0013555892510339618f;
static const ai_float conv2d_31_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.001561221550218761f, 0.0020109231118112803f, 0.0016169085865840316f, 0.002611157251521945f, 0.002120132325217128f, 0.002106851665303111f, 0.0033719870261847973f, 0.001551086432300508f, 0.002270115539431572f, 0.0019678333774209023f, 0.0023923590779304504f, 0.001435181125998497f, 0.002030769595876336f, 0.0022761456202715635f, 0.002656338969245553f, 0.003088949481025338f, 0.002392431255429983f, 0.00213049678131938f, 0.0023491953033953905f, 0.0025610870216041803f, 0.002056479686871171f, 0.0018326339777559042f, 0.0018312082393094897f, 0.002289613476023078f, 0.0026506634894758463f, 0.00197001826018095f, 0.0026081702671945095f, 0.002281643683090806f, 0.002494615502655506f, 0.0020053349435329437f, 0.001887873513624072f, 0.002023879671469331f, 0.001843196339905262f, 0.0034629483707249165f, 0.004189635161310434f, 0.0029167074244469404f, 0.0020159052219241858f, 0.004239161964505911f, 0.001564769889228046f, 0.0025823931209743023f, 0.002205065218731761f, 0.002952652284875512f, 0.0030238605104386806f, 0.0016907028621062636f, 0.0018728323047980666f, 0.0029560911934822798f, 0.0018363158451393247f, 0.0016808987129479647f, 0.0021402405109256506f, 0.002342397579923272f, 0.002499536145478487f, 0.00298884604126215f, 0.0024760495871305466f, 0.002291948301717639f, 0.0027506244368851185f, 0.001811595051549375f, 0.002705996623262763f, 0.004137901589274406f, 0.002957419026643038f, 0.0019212528131902218f, 0.0020377985201776028f, 0.0019706320017576218f, 0.0021157870069146156f, 0.005044536665081978f, 0.004142732825130224f, 0.0023871948942542076f, 0.0015086591010913253f, 0.00244337972253561f, 0.0023284920025616884f, 0.0029469537548720837f, 0.0027789785526692867f, 0.0019614254124462605f, 0.0022446915972977877f, 0.0025126219261437654f, 0.0017698714509606361f, 0.0022540483623743057f, 0.002131862798705697f, 0.0020530151668936014f, 0.0020664779003709555f, 0.001985382055863738f, 0.002064801985397935f, 0.001505786320194602f, 0.0019535415340214968f, 0.0028678267262876034f, 0.00293780118227005f, 0.002149739069864154f, 0.0020794118754565716f, 0.0015119413146749139f, 0.002590321470052004f, 0.002584957517683506f, 0.001954431878402829f, 0.004486873280256987f, 0.0019768388010561466f, 0.0025755194947123528f, 0.0027567720972001553f, 0.001381188747473061f, 0.002064656699076295f, 0.001997114159166813f, 0.0024070951621979475f, 0.0017439756775274873f, 0.0024311738088726997f, 0.0016785373445600271f, 0.0022211240138858557f, 0.002817681059241295f, 0.0026356279850006104f, 0.002214528387412429f, 0.002732956549152732f, 0.0026330819819122553f, 0.002653545467182994f, 0.001534607377834618f, 0.002479478484019637f, 0.0029492147732526064f, 0.002491119783371687f, 0.0025570804718881845f, 0.0019742869772017f, 0.0023947127629071474f, 0.0019859932363033295f, 0.002386835403740406f, 0.0022339588031172752f, 0.002455709967762232f, 0.002323735738173127f, 0.0033193305134773254f, 0.0017491542967036366f, 0.0034031453542411327f, 0.0022973965387791395f, 0.002543373266234994f, 0.004419970791786909f, 0.0024929794017225504f, 0.0017272124532610178f, 0.0018962094327434897f, 0.0023130173794925213f, 0.003453664481639862f, 0.0018721366068348289f, 0.003052455373108387f, 0.002450374886393547f, 0.0016570170409977436f, 0.002517256187275052f, 0.0024247877299785614f, 0.0026585652958601713f, 0.0017769559053704143f, 0.0032998649403452873f, 0.0019558966159820557f, 0.002426240360364318f, 0.0020677149295806885f, 0.0028485090006142855f, 0.0022999821230769157f, 0.0017165829194709659f, 0.0024003777652978897f, 0.0018473954405635595f, 0.002509335521608591f, 0.001992717618122697f, 0.0025850271340459585f, 0.002229871693998575f, 0.001951729878783226f, 0.002012808807194233f, 0.001879530493170023f, 0.0029112331103533506f, 0.0018411398632451892f, 0.005398529581725597f, 0.0027783680707216263f, 0.0018147743539884686f, 0.002352168085053563f, 0.0026063325349241495f, 0.0028864808846265078f, 0.00422102864831686f, 0.0030556260608136654f, 0.002043302170932293f, 0.0029270274098962545f, 0.0018123898189514875f, 0.001967450836673379f, 0.0022106063552200794f, 0.0016745280008763075f, 0.0023642026353627443f, 0.0022130522411316633f, 0.0014982938300818205f, 0.0021584865171462297f, 0.002969790482893586f, 0.0026671886444091797f, 0.0016193907940760255f, 0.0037994557060301304f, 0.0020281216129660606f, 0.0025807744823396206f, 0.002306607086211443f, 0.0020823876839131117f, 0.0021754212211817503f, 0.0018676933832466602f, 0.001744049834087491f, 0.002333922078832984f, 0.0024217518512159586f, 0.0015807992313057184f, 0.0017877820646390319f, 0.001901041716337204f, 0.002621869556605816f, 0.00226895441301167f, 0.0019348249770700932f, 0.0012694353936240077f, 0.002608687151223421f, 0.0023767862003296614f, 0.0020472805481404066f, 0.004260561428964138f, 0.00210169842466712f, 0.0019359285943210125f, 0.0019391831010580063f, 0.001941134687513113f, 0.0015343368286266923f, 0.0026217878330498934f, 0.0033324805554002523f, 0.0021938730496913195f, 0.0017166049219667912f, 0.0027480199933052063f, 0.0026299869641661644f, 0.0019729940686374903f, 0.002069066045805812f, 0.003310989588499069f, 0.0021063878666609526f, 0.0018489835783839226f, 0.001906692748889327f, 0.0019184848060831428f, 0.0023063188418745995f, 0.002184836193919182f, 0.002137143397703767f, 0.0023218696005642414f, 0.0021656739991158247f, 0.0030517177656292915f, 0.0026277475990355015f, 0.0026467428542673588f, 0.0038866628892719746f, 0.003421306610107422f, 0.002624466549605131f, 0.001582280034199357f, 0.0014328339602798223f, 0.0026490644086152315f, 0.002880175830796361f, 0.003309804480522871f, 0.001901191659271717f, 0.0018356204964220524f, 0.0021803348790854216f, 0.002276801969856024f, 0.002708310727030039f, 0.0019279586849734187f, 0.0029247631318867207f, 0.002171408850699663f, 0.00425738375633955f, 0.003573043504729867f, 0.0027819680981338024f, 0.002306390320882201f, 0.002334240823984146f, 0.002056318800896406f, 0.0018515426199883223f, 0.0016312963562086225f, 0.002679214347153902f, 0.0025470550172030926f, 0.0015832126373425126f, 0.002575740683823824f, 0.002944770734757185f, 0.002456449903547764f, 0.0025100628845393658f, 0.0025107660330832005f, 0.002417699433863163f, 0.0024765681009739637f, 0.002269440796226263f, 0.003357431385666132f, 0.0029441099613904953f, 0.001979716820642352f, 0.0019611292518675327f, 0.0021820080000907183f, 0.0027836239896714687f, 0.0034374725073575974f, 0.002659605350345373f, 0.002099736128002405f, 0.0012253577588126063f, 0.0016296672401949763f, 0.0025111502036452293f, 0.002790067344903946f, 0.004814367741346359f, 0.0023862377274781466f, 0.002665831707417965f, 0.0023467745631933212f, 0.0021639487240463495f, 0.0014412540476769209f, 0.002412684028968215f, 0.0017151611391454935f, 0.002355364151299f, 0.0031932638958096504f, 0.002349028829485178f, 0.002364078303799033f, 0.0021434249356389046f, 0.0019220731919631362f);
static const ai_layer_format_type conv2d_31_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_32_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_32_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_32_pad_before_t_in_0_shape_h_const_u32 = 5;

static const ai_u16 conv2d_32_t_in_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_32_t_in_0_shape_h_const_u16 = 7;
static const ai_u16 conv2d_32_t_in_0_shape_ch_const_u16 = 288;
static const ai_u16 conv2d_32_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_32_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_32_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_32_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_32_t_in_0_fmt_scale_const_f32 = 0.0013555892510339618f;
static const ai_float conv2d_32_t_out_0_fmt_scale_const_f32 = 0.001734352670609951f;
static const ai_float conv2d_32_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.004720325116068125f, 0.00276087480597198f, 0.0035031645093113184f, 0.007196012418717146f, 0.005452618468552828f, 0.008587238378822803f, 0.006497704423964024f, 0.005283428356051445f, 0.004951775074005127f, 0.0066300807520747185f, 0.005997858475893736f, 0.004359775222837925f, 0.005748308729380369f, 0.004347160458564758f, 0.006461812183260918f, 0.003660560352727771f, 0.003570485394448042f, 0.005915763787925243f, 0.007233777083456516f, 0.006275269668549299f, 0.003929282072931528f, 0.0034134008456021547f, 0.007947690784931183f, 0.004910459276288748f, 0.005677528213709593f, 0.006400763057172298f, 0.005477560218423605f, 0.00813342072069645f, 0.007802680600434542f, 0.002619289793074131f, 0.003407206851989031f, 0.00571722537279129f, 0.004572946112602949f, 0.006364978849887848f, 0.0050725131295621395f, 0.006331156939268112f, 0.004245080519467592f, 0.007928770035505295f, 0.004787778481841087f, 0.005361458752304316f, 0.004480067174881697f, 0.004976142197847366f, 0.006070636212825775f, 0.006852676160633564f, 0.0048620253801345825f, 0.0036694020964205265f, 0.0022917233873158693f, 0.003636301727965474f, 0.0051832678727805614f, 0.004867137875407934f, 0.003941320814192295f, 0.00726076727733016f, 0.005234786309301853f, 0.005708508659154177f, 0.004732082132250071f, 0.0032129595056176186f, 0.007804816123098135f, 0.007536155637353659f, 0.006286080460995436f, 0.005103712901473045f, 0.00521833123639226f, 0.0041408115066587925f, 0.006286298856139183f, 0.00734928622841835f, 0.005221380852162838f, 0.004592097830027342f, 0.003537966636940837f, 0.005899909883737564f, 0.0048389676958322525f, 0.006225701421499252f, 0.009033964946866035f, 0.003874412504956126f, 0.005197280086576939f, 0.0054673729464411736f, 0.0033362992107868195f, 0.0020146945025771856f, 0.005607534199953079f, 0.005773325450718403f, 0.004617559257894754f, 0.003404089482501149f, 0.004817187786102295f, 0.007814263924956322f, 0.00504776556044817f, 0.006132333539426327f, 0.006225391756743193f, 0.0044693779200315475f, 0.003477812744677067f, 0.004188083577901125f, 0.007900933735072613f, 0.005092525389045477f, 0.0032728235237300396f, 0.004518892150372267f, 0.007150361314415932f, 0.0073348344303667545f, 0.005383727606385946f, 0.0036638446617871523f, 0.0050137960352003574f, 0.0028534226585179567f, 0.005693953484296799f, 0.004499328788369894f, 0.0064807552844285965f, 0.0038232074584811926f, 0.002758981892839074f, 0.004448825027793646f, 0.0076777259819209576f, 0.007052636705338955f, 0.005569838918745518f, 0.0064110527746379375f, 0.003726192517206073f, 0.003808993846178055f, 0.006533991079777479f, 0.006050542928278446f, 0.006035865284502506f, 0.0036972560919821262f, 0.005509151611477137f, 0.006433811970055103f, 0.0071122776716947556f, 0.005118163768202066f, 0.005183516535907984f, 0.004643223714083433f, 0.006255919113755226f, 0.008832000195980072f, 0.0046759070828557014f, 0.006383705884218216f, 0.0035805669613182545f, 0.00729506416246295f, 0.0053574820049107075f, 0.005335476715117693f, 0.004430023021996021f, 0.007361348252743483f, 0.007184854242950678f, 0.005299725569784641f, 0.003854083828628063f, 0.008780729956924915f, 0.00799342431128025f, 0.0037418026477098465f, 0.006730800494551659f, 0.0037209924776107073f, 0.0076949880458414555f, 0.005531185306608677f, 0.005295873153954744f, 0.006515825167298317f, 0.005125688388943672f, 0.00774414511397481f, 0.0046981098130345345f, 0.005413568112999201f, 0.004162211436778307f, 0.006435181945562363f, 0.0061135743744671345f, 0.005158610176295042f, 0.005414534360170364f, 0.005189328454434872f, 0.007196852006018162f, 0.004193707834929228f, 0.003489759285002947f, 0.005055244080722332f, 0.005629915278404951f, 0.0033414310310035944f, 0.008851425722241402f, 0.007708101067692041f, 0.00396258570253849f, 0.004000959452241659f, 0.007690038997679949f, 0.005880272947251797f, 0.002930969465523958f, 0.00732395937666297f, 0.00406047934666276f, 0.006403505802154541f, 0.0028131913859397173f, 0.004653500393033028f, 0.0051581403240561485f, 0.0053268009796738625f, 0.007410361897200346f, 0.005993986967951059f, 0.005957403220236301f, 0.004276093561202288f, 0.004060832317918539f, 0.004790265113115311f, 0.0041956244967877865f, 0.004981399513781071f, 0.007966696284711361f, 0.006105779204517603f, 0.005182270426303148f, 0.004632180090993643f, 0.0037866737693548203f, 0.007812442723661661f, 0.005086949095129967f, 0.004152160137891769f, 0.006433736067265272f, 0.006088297814130783f, 0.005857253912836313f, 0.0054289475083351135f, 0.004801888484507799f, 0.005017949268221855f, 0.003114510327577591f, 0.0037150189746171236f, 0.003411607351154089f, 0.00614207936450839f, 0.003296677488833666f, 0.0064302776008844376f, 0.003563257399946451f, 0.003723487723618746f, 0.004448284860700369f, 0.004855085629969835f, 0.002597800688818097f, 0.0056172143667936325f, 0.0049883704632520676f, 0.00604624068364501f, 0.0061431466601789f, 0.006678713485598564f, 0.0030349576845765114f, 0.003077098401263356f, 0.005667917896062136f, 0.004973255097866058f, 0.003039454808458686f, 0.003389048855751753f, 0.005350029096007347f, 0.0033482990693300962f, 0.00770478043705225f, 0.0034327704925090075f, 0.008211052045226097f, 0.00644104927778244f, 0.005869605112820864f, 0.004414303228259087f, 0.007690100464969873f, 0.004655655473470688f, 0.005783292464911938f, 0.0049815247766673565f, 0.00507918419316411f, 0.0043345242738723755f, 0.004178439732640982f, 0.0028556555043905973f, 0.006761441007256508f, 0.004276755265891552f, 0.002759294118732214f, 0.00620706332847476f, 0.004848164040595293f, 0.004941509570926428f, 0.004266005475074053f, 0.007399209309369326f, 0.004232073202729225f, 0.004290016833692789f, 0.004495508968830109f, 0.0031054632272571325f, 0.006438858341425657f, 0.007644483353942633f, 0.00492477510124445f, 0.0039059624541550875f, 0.0031094409059733152f, 0.00416476558893919f, 0.010299092158675194f, 0.005446872673928738f, 0.0032586753368377686f, 0.005527624860405922f, 0.005486825946718454f, 0.005939151626080275f, 0.005576781928539276f, 0.005391145125031471f, 0.006019753869622946f, 0.006664938293397427f, 0.007723074406385422f, 0.010758518241345882f, 0.00911550410091877f, 0.005553048104047775f, 0.0067210630513727665f, 0.006616232451051474f, 0.00459137512370944f, 0.00691695511341095f, 0.0025454885326325893f, 0.006287802010774612f, 0.003531288355588913f, 0.004617696162313223f, 0.006327172741293907f, 0.005913183558732271f, 0.005193477962166071f, 0.0048681297339499f, 0.0060143801383674145f, 0.004120024386793375f, 0.006043302360922098f, 0.004024790599942207f, 0.004423246253281832f, 0.0034787848126143217f, 0.002836306346580386f, 0.0049242242239415646f, 0.007488648407161236f, 0.004982504062354565f, 0.007669847924262285f, 0.004025911912322044f);
static const ai_u16 conv2d_32_t_out_0_shape_w_const_u16 = 5;
static const ai_u16 conv2d_32_t_out_0_shape_h_const_u16 = 5;

static const ai_u16 conv2d_33_t_in_0_shape_w_const_u16 = 5;
static const ai_u16 conv2d_33_t_in_0_shape_h_const_u16 = 5;
static const ai_u16 conv2d_33_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_33_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_33_t_in_0_shape_ch_const_u16 = 288;
static const ai_u16 conv2d_33_t_out_0_shape_ch_const_u16 = 288;
static const ai_i8 conv2d_33_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_33_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_33_t_in_0_fmt_scale_const_f32 = 0.001734352670609951f;
static const ai_float conv2d_33_t_out_0_fmt_scale_const_f32 = 0.0012167595559731126f;
static const ai_float conv2d_33_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.001398925669491291f, 0.00253589591011405f, 0.0010685845045372844f, 0.004436730407178402f, 0.002537437714636326f, 0.0024949435610324144f, 0.0030354943592101336f, 0.002036381745710969f, 0.0023859450593590736f, 0.005987147334963083f, 0.0016075868625193834f, 0.0028123140800744295f, 0.0027226905804127455f, 0.002555086277425289f, 0.003226113272830844f, 0.002988541731610894f, 0.0020865476690232754f, 0.0027645647060126066f, 0.0018985791830345988f, 0.005139581859111786f, 0.0014029588783159852f, 0.0020680257584899664f, 0.002516119508072734f, 0.0017838268540799618f, 0.0032188312616199255f, 0.0027061323635280132f, 0.001376924104988575f, 0.0036211421247571707f, 0.0021320267114788294f, 0.002324315719306469f, 0.0025235656648874283f, 0.003562909783795476f, 0.00292190327309072f, 0.0034923707135021687f, 0.0023643006570637226f, 0.0036544979084283113f, 0.002512666629627347f, 0.0035792633425444365f, 0.003081441158428788f, 0.0017174489330500364f, 0.0025070353876799345f, 0.002367665758356452f, 0.0019318145932629704f, 0.0015444377204403281f, 0.0023823461961001158f, 0.004108300432562828f, 0.00187777413520962f, 0.004616540390998125f, 0.0027769741136580706f, 0.0013483078218996525f, 0.003605497535318136f, 0.0013787188800051808f, 0.0024425084702670574f, 0.0016673762584105134f, 0.0035219453275203705f, 0.0029000691138207912f, 0.003010069252923131f, 0.002277351450175047f, 0.0015551784308627248f, 0.002268281066790223f, 0.00257961917668581f, 0.0026112317573279142f, 0.002639404032379389f, 0.002691102446988225f, 0.0012339192908257246f, 0.0009561071055941284f, 0.002311448100954294f, 0.0041181049309670925f, 0.0023980115074664354f, 0.0015715820482000709f, 0.003221197985112667f, 0.0019985055550932884f, 0.002272033365443349f, 0.0018441739957779646f, 0.0017032651230692863f, 0.0018031148938462138f, 0.0016046082600951195f, 0.002368135843425989f, 0.00298359920270741f, 0.0012690752046182752f, 0.0034805256873369217f, 0.0019051291747018695f, 0.0022933741565793753f, 0.0039518787525594234f, 0.0015220233472064137f, 0.0028757681138813496f, 0.0018861093558371067f, 0.0026404722593724728f, 0.001091635087504983f, 0.0013204739661887288f, 0.0033110477961599827f, 0.0017580074490979314f, 0.0026918749790638685f, 0.002482205629348755f, 0.0021885971073061228f, 0.0019103724043816328f, 0.002431126544252038f, 0.004648477304726839f, 0.0028885744977742434f, 0.00220950273796916f, 0.0027138979639858007f, 0.0023850889410823584f, 0.0029389197006821632f, 0.0035348229575902224f, 0.0027314857579767704f, 0.0014636754058301449f, 0.0025085946545004845f, 0.0021271591540426016f, 0.0021995583083480597f, 0.0034136527683585882f, 0.003841004567220807f, 0.001919895294122398f, 0.0028135227039456367f, 0.0024660134222358465f, 0.002416180679574609f, 0.0016227043233811855f, 0.0023949414025992155f, 0.0011471553007140756f, 0.0019311801297590137f, 0.0021264769602566957f, 0.0025913913268595934f, 0.0030368510633707047f, 0.0017563168657943606f, 0.002898856531828642f, 0.0020908277947455645f, 0.0017368732951581478f, 0.0019185520941391587f, 0.002438458614051342f, 0.002021459396928549f, 0.0015618997858837247f, 0.001928343321196735f, 0.002860370557755232f, 0.002380459103733301f, 0.0014471106696873903f, 0.0016338772838935256f, 0.00280886166729033f, 0.0017473198240622878f, 0.002184184966608882f, 0.0036208454985171556f, 0.003373619168996811f, 0.006147282663732767f, 0.0010826897341758013f, 0.002292538760229945f, 0.003185786074027419f, 0.002039378508925438f, 0.0024143285118043423f, 0.003744166111573577f, 0.0016796075506135821f, 0.002268679440021515f, 0.0020191234070807695f, 0.003070676000788808f, 0.002559882355853915f, 0.0010500153293833137f, 0.0024621121119707823f, 0.003353786189109087f, 0.0017963050631806254f, 0.002899846062064171f, 0.002066061832010746f, 0.001840108074247837f, 0.004450320731848478f, 0.0017094158101826906f, 0.0030981271993368864f, 0.002184274373576045f, 0.0023462148383259773f, 0.003944579046219587f, 0.0014470691094174981f, 0.0015489647630602121f, 0.001383648021146655f, 0.0006821550778113306f, 0.0018262868979945779f, 0.003298953641206026f, 0.002351324539631605f, 0.0013542285887524486f, 0.0017191917868331075f, 0.00486884405836463f, 0.0024308147840201855f, 0.003600485622882843f, 0.0008395565673708916f, 0.00275505636818707f, 0.0024295838084071875f, 0.002526936586946249f, 0.0017928661545738578f, 0.0019983600359410048f, 0.0025487670209258795f, 0.0015261608641594648f, 0.0012843151343986392f, 0.0025414812844246626f, 0.001834387076087296f, 0.002833615057170391f, 0.003744665067642927f, 0.0028470929246395826f, 0.002171906642615795f, 0.0013491580029949546f, 0.0024960017763078213f, 0.002866060473024845f, 0.0016704609151929617f, 0.0021001927088946104f, 0.001503274543210864f, 0.002079242141917348f, 0.0013932333094999194f, 0.0031786563340574503f, 0.0018034590175375342f, 0.0030937271658331156f, 0.002980266697704792f, 0.0031054834835231304f, 0.004631143994629383f, 0.0043313889764249325f, 0.0012008860940113664f, 0.003731546690687537f, 0.002366862492635846f, 0.004004701040685177f, 0.0015126639045774937f, 0.0032081962563097477f, 0.006545466370880604f, 0.003303442383185029f, 0.0016579884104430676f, 0.0014784326776862144f, 0.0023066981229931116f, 0.003510388545691967f, 0.001196552999317646f, 0.0028880005702376366f, 0.0018581341719254851f, 0.0019732334185391665f, 0.0034260370302945375f, 0.0023105894215404987f, 0.0024061643052846193f, 0.002596287988126278f, 0.0031536801252514124f, 0.0011658400762826204f, 0.0020832240115851164f, 0.0020306003279983997f, 0.005371165461838245f, 0.0015950837405398488f, 0.0025105189997702837f, 0.0031285302247852087f, 0.0025421902537345886f, 0.002110677072778344f, 0.0064566838555037975f, 0.0020289195235818624f, 0.002071171533316374f, 0.001995848724618554f, 0.002242136048153043f, 0.002086323220282793f, 0.002403832972049713f, 0.0017600676510483027f, 0.0009352755732834339f, 0.004910530522465706f, 0.0014273548731580377f, 0.0032823060173541307f, 0.0023744869977235794f, 0.002472137799486518f, 0.002158669289201498f, 0.0025684786960482597f, 0.0021524520125240088f, 0.002076588338240981f, 0.0015000698622316122f, 0.0016707137692719698f, 0.0022001652978360653f, 0.004138365853577852f, 0.0030158632434904575f, 0.002170751802623272f, 0.001659807632677257f, 0.002250065328553319f, 0.0013971042353659868f, 0.0040478152222931385f, 0.002105341525748372f, 0.002022158820182085f, 0.002909015631303191f, 0.0031053926795721054f, 0.002482395386323333f, 0.0020856093615293503f, 0.0027144604828208685f, 0.0032573342323303223f, 0.0016997650964185596f, 0.00100387679412961f, 0.0025418144650757313f, 0.0017911444883793592f, 0.0016503806691616774f, 0.002693925518542528f, 0.0042087906040251255f, 0.0020233923569321632f, 0.0014481251128017902f, 0.0016017709858715534f, 0.0030014864169061184f, 0.002360308077186346f, 0.0037351560313254595f, 0.001729360199533403f, 0.0031953160651028156f);
static const ai_layer_format_type conv2d_33_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_u16 conv2d_34_t_in_0_shape_w_const_u16 = 5;
static const ai_u16 conv2d_34_t_in_0_shape_h_const_u16 = 5;
static const ai_u16 conv2d_34_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_34_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_34_t_in_0_shape_ch_const_u16 = 288;
static const ai_u16 conv2d_34_t_out_0_shape_ch_const_u16 = 16;
static const ai_i8 conv2d_34_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_34_t_out_0_fmt_zero_const_s8 = 20;
static const ai_float conv2d_34_t_in_0_fmt_scale_const_f32 = 0.0012167595559731126f;
static const ai_float conv2d_34_t_out_0_fmt_scale_const_f32 = 0.0007190321339294314f;
static const ai_float conv2d_34_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0006588561227545142f, 0.0005514587974175811f, 0.0004997408832423389f, 0.0004953508032485843f, 0.0005489723407663405f, 0.0004905653768219054f, 0.0005898771923966706f, 0.0007163826958276331f, 0.0006024699541740119f, 0.0005581033183261752f, 0.0006070986273698509f, 0.0006612234865315259f, 0.0005060039693489671f, 0.0005857438663952053f, 0.0005382698145695031f, 0.00048020348185673356f);
static const ai_layer_format_type conv2d_34_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_36_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(20);
static const ai_i16 conv2d_36_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_36_pad_before_t_in_0_shape_h_const_u32 = 5;

static const ai_u16 conv2d_36_t_in_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_36_t_in_0_shape_h_const_u16 = 7;
static const ai_u16 conv2d_36_t_in_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_36_t_out_0_shape_ch_const_u16 = 16;
static const ai_i8 conv2d_36_t_in_0_fmt_zero_const_s8 = 20;
static const ai_i8 conv2d_36_t_out_0_fmt_zero_const_s8 = 18;
static const ai_float conv2d_36_t_in_0_fmt_scale_const_f32 = 0.0007190321339294314f;
static const ai_float conv2d_36_t_out_0_fmt_scale_const_f32 = 0.00038219871930778027f;
static const ai_float conv2d_36_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.00030615192372351885f, 0.0003906601632479578f, 0.0004486503603402525f, 0.0003232570888940245f, 0.0003592756693251431f, 0.0003296570503152907f, 0.0002316192985745147f, 0.0002919179096352309f, 0.0003580742923077196f, 0.0003155063313897699f, 0.00042846545693464577f, 0.00028788100462406874f, 0.00030718775815330446f, 0.0003120785695500672f, 0.0003584610531106591f, 0.0005464626010507345f);
static const ai_layer_format_type conv2d_36_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_36_t_out_0_shape_w_const_u16 = 5;
static const ai_u16 conv2d_36_t_out_0_shape_h_const_u16 = 5;


static const ai_u16 conv2d_37_t_in_0_shape_w_const_u16 = 10;
static const ai_u16 conv2d_37_t_in_0_shape_h_const_u16 = 10;
static const ai_u16 conv2d_37_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_37_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_37_t_in_0_shape_ch_const_u16 = 152;
static const ai_u16 conv2d_37_t_out_0_shape_ch_const_u16 = 16;
static const ai_i8 conv2d_37_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_37_t_out_0_fmt_zero_const_s8 = 5;
static const ai_float conv2d_37_t_in_0_fmt_scale_const_f32 = 0.0035589300096035004f;
static const ai_float conv2d_37_t_out_0_fmt_scale_const_f32 = 0.0016279983101412654f;
static const ai_float conv2d_37_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0005299033364281058f, 0.000755088753066957f, 0.0006601532222703099f, 0.0006824946613050997f, 0.000710410880856216f, 0.0005233926931396127f, 0.0007519868086092174f, 0.0008129508933052421f, 0.0004661219718400389f, 0.0007461546338163316f, 0.0005540573038160801f, 0.0006860301364213228f, 0.0007688648765906692f, 0.0008143484010361135f, 0.0006254467298276722f, 0.0006039498839527369f);
static const ai_layer_format_type conv2d_37_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;


static const ai_i8 conv2d_40_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(5);
static const ai_i16 conv2d_40_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_40_pad_before_t_in_0_shape_h_const_u32 = 10;

static const ai_u16 conv2d_40_t_in_0_shape_w_const_u16 = 12;
static const ai_u16 conv2d_40_t_in_0_shape_h_const_u16 = 12;
static const ai_u16 conv2d_40_t_in_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_40_t_out_0_shape_ch_const_u16 = 16;
static const ai_i8 conv2d_40_t_in_0_fmt_zero_const_s8 = 5;
static const ai_i8 conv2d_40_t_out_0_fmt_zero_const_s8 = 1;
static const ai_float conv2d_40_t_in_0_fmt_scale_const_f32 = 0.0016279983101412654f;
static const ai_float conv2d_40_t_out_0_fmt_scale_const_f32 = 0.0009802478598430753f;
static const ai_float conv2d_40_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0004668093752115965f, 0.0006805977900512516f, 0.000543700298294425f, 0.0004800534516107291f, 0.00047955449554137886f, 0.0005526022869162261f, 0.0007583347032777965f, 0.0006072812248021364f, 0.0005408652941696346f, 0.00063224759651348f, 0.000696812232490629f, 0.0007255096570588648f, 0.0005455870414152741f, 0.0005368897691369057f, 0.0005518867983482778f, 0.000610160524956882f);
static const ai_layer_format_type conv2d_40_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_40_t_out_0_shape_w_const_u16 = 10;
static const ai_u16 conv2d_40_t_out_0_shape_h_const_u16 = 10;


static const ai_u16 conv2d_41_t_in_0_shape_w_const_u16 = 20;
static const ai_u16 conv2d_41_t_in_0_shape_h_const_u16 = 20;
static const ai_u16 conv2d_41_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_41_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_41_t_in_0_shape_ch_const_u16 = 72;
static const ai_u16 conv2d_41_t_out_0_shape_ch_const_u16 = 16;
static const ai_i8 conv2d_41_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_41_t_out_0_fmt_zero_const_s8 = -19;
static const ai_float conv2d_41_t_in_0_fmt_scale_const_f32 = 0.0046248785220086575f;
static const ai_float conv2d_41_t_out_0_fmt_scale_const_f32 = 0.0027904396411031485f;
static const ai_float conv2d_41_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0010633118217810988f, 0.0008588451310060918f, 0.0011512775672599673f, 0.0011365776881575584f, 0.00106084777507931f, 0.0011621816083788872f, 0.0008705674554221332f, 0.0011789248092100024f, 0.0008558022673241794f, 0.0009159314795397222f, 0.0010100057115778327f, 0.0008907351875677705f, 0.0008282797643914819f, 0.0008719167090021074f, 0.0009948243387043476f, 0.0010257144458591938f);
static const ai_layer_format_type conv2d_41_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;


static const ai_i8 conv2d_43_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-19);
static const ai_i16 conv2d_43_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_43_pad_before_t_in_0_shape_h_const_u32 = 20;

static const ai_u16 conv2d_43_t_in_0_shape_w_const_u16 = 22;
static const ai_u16 conv2d_43_t_in_0_shape_h_const_u16 = 22;
static const ai_u16 conv2d_43_t_in_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_43_t_out_0_shape_ch_const_u16 = 16;
static const ai_i8 conv2d_43_t_in_0_fmt_zero_const_s8 = -19;
static const ai_i8 conv2d_43_t_out_0_fmt_zero_const_s8 = -6;
static const ai_float conv2d_43_t_in_0_fmt_scale_const_f32 = 0.0027904396411031485f;
static const ai_float conv2d_43_t_out_0_fmt_scale_const_f32 = 0.001759668462909758f;
static const ai_float conv2d_43_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0011293303687125444f, 0.0011760680936276913f, 0.0009753815247677267f, 0.0009881576988846064f, 0.0010322262533009052f, 0.001450524665415287f, 0.0014363398076966405f, 0.0011368084233254194f, 0.0009962490294128656f, 0.0016596573404967785f, 0.001311174826696515f, 0.001750895637087524f, 0.0009177675819955766f, 0.0010226115118712187f, 0.0009975916473194957f, 0.0015550096286460757f);
static const ai_layer_format_type conv2d_43_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_43_t_out_0_shape_w_const_u16 = 20;
static const ai_u16 conv2d_43_t_out_0_shape_h_const_u16 = 20;

static const ai_i8 conv2d_44_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-6);
static const ai_i16 conv2d_44_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_44_pad_before_t_in_0_shape_h_const_u32 = 20;

static const ai_u16 conv2d_44_t_in_0_shape_w_const_u16 = 22;
static const ai_u16 conv2d_44_t_in_0_shape_h_const_u16 = 22;
static const ai_u16 conv2d_44_t_in_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_44_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_44_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_44_t_in_0_fmt_zero_const_s8 = -6;
static const ai_i8 conv2d_44_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_44_t_in_0_fmt_scale_const_f32 = 0.001759668462909758f;
static const ai_float conv2d_44_t_out_0_fmt_scale_const_f32 = 0.006812004838138819f;
static const ai_float conv2d_44_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0538061298429966f, 0.04909934848546982f, 0.03201599046587944f, 0.05576976016163826f, 0.04157031700015068f, 0.02616916410624981f, 0.04099302366375923f, 0.03787241876125336f, 0.035231661051511765f, 0.050521332770586014f, 0.03665308281779289f, 0.04445100575685501f, 0.025133611634373665f, 0.0330488421022892f, 0.03983333706855774f, 0.02559344843029976f);
static const ai_u16 conv2d_44_t_out_0_shape_w_const_u16 = 20;
static const ai_u16 conv2d_44_t_out_0_shape_h_const_u16 = 20;

static const ai_u16 conv2d_45_t_in_0_shape_w_const_u16 = 20;
static const ai_u16 conv2d_45_t_in_0_shape_h_const_u16 = 20;
static const ai_u16 conv2d_45_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_45_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_45_t_in_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_45_t_out_0_shape_ch_const_u16 = 64;
static const ai_i8 conv2d_45_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_45_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_45_t_in_0_fmt_scale_const_f32 = 0.006812004838138819f;
static const ai_float conv2d_45_t_out_0_fmt_scale_const_f32 = 0.004793986212462187f;
static const ai_float conv2d_45_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0029359515756368637f, 0.001419215346686542f, 0.0018997595179826021f, 0.004309053532779217f, 0.002311783842742443f, 0.0011167438933625817f, 0.0013676427770406008f, 0.002390391193330288f, 0.0012073538964614272f, 0.0028784992173314095f, 0.0018997930455952883f, 0.0013222513953223825f, 0.0037769032642245293f, 0.0017444778932258487f, 0.0030631248373538256f, 0.002242246875539422f, 0.0017242080066353083f, 0.0019791664090007544f, 0.00258418801240623f, 0.003356708213686943f, 0.0009446113836020231f, 0.00176821683999151f, 0.0022392948158085346f, 0.0011782681103795767f, 0.0035380334593355656f, 0.0009724116534925997f, 0.001669074408710003f, 0.001770581235177815f, 0.001625980599783361f, 0.002639355603605509f, 0.004667629487812519f, 0.002833048813045025f, 0.001823085593059659f, 0.002092539332807064f, 0.0030402736738324165f, 0.001695160986855626f, 0.001953114289790392f, 0.0008337076869793236f, 1.3214298633101862e-06f, 0.002660573460161686f, 0.0016769325593486428f, 0.002286485629156232f, 0.001797738135792315f, 0.0014901059912517667f, 0.003190905787050724f, 0.0009183314978145063f, 0.002074390184134245f, 0.001769543974660337f, 0.0017240701708942652f, 0.0029935799539089203f, 0.0035787601955235004f, 0.0014200470177456737f, 0.002587884431704879f, 0.0014037943910807371f, 0.002144313883036375f, 0.001821099198423326f, 0.00256026117131114f, 0.0017555104568600655f, 0.0017160108545795083f, 0.0013565417611971498f, 0.0013991603627800941f, 0.002145143924281001f, 0.00663819070905447f, 0.005090457387268543f);
static const ai_layer_format_type conv2d_45_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_46_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_46_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_46_pad_before_t_in_0_shape_h_const_u32 = 20;

static const ai_u16 conv2d_46_t_in_0_shape_w_const_u16 = 22;
static const ai_u16 conv2d_46_t_in_0_shape_h_const_u16 = 22;
static const ai_u16 conv2d_46_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_46_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_46_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_46_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_46_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_46_t_in_0_fmt_scale_const_f32 = 0.004793986212462187f;
static const ai_float conv2d_46_t_out_0_fmt_scale_const_f32 = 0.0055921501480042934f;
static const ai_float conv2d_46_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.010494563728570938f, 0.010605278424918652f, 0.025137759745121002f, 0.00888914242386818f, 0.006516470108181238f, 0.013067897409200668f, 0.018864808604121208f, 0.005491694901138544f, 0.022315431386232376f, 0.0059118010103702545f, 0.006760654971003532f, 0.010161712765693665f, 0.005419215653091669f, 0.00825266819447279f, 0.008719474077224731f, 0.009480536915361881f, 0.015575602650642395f, 0.010825276374816895f, 0.004957766272127628f, 0.005477870348840952f, 0.0074367462657392025f, 0.010984059423208237f, 0.031109949573874474f, 0.021912192925810814f, 0.004561237059533596f, 0.008689875714480877f, 0.00596254738047719f, 0.007226709276437759f, 0.011290998198091984f, 0.015372865833342075f, 0.0036624004133045673f, 0.012593777850270271f, 0.01239060889929533f, 0.007355237379670143f, 0.006382199004292488f, 0.0076677254401147366f, 0.00959028396755457f, 0.018033599480986595f, 8.642256830171391e-07f, 0.010810413397848606f, 0.010721485130488873f, 0.0070863692089915276f, 0.009483495727181435f, 0.012243605218827724f, 0.007504014763981104f, 0.018011871725320816f, 0.01775909960269928f, 0.0077346875332295895f, 0.013001637533307076f, 0.015960561111569405f, 0.013297432102262974f, 0.011943869292736053f, 0.007884364575147629f, 0.016937704756855965f, 0.010604322887957096f, 0.010072811506688595f, 0.010173548012971878f, 0.013361671008169651f, 0.010990921407938004f, 0.01280953548848629f, 0.010341846384108067f, 0.012870799750089645f, 0.002783763688057661f, 0.007520175073295832f);
static const ai_u16 conv2d_46_t_out_0_shape_w_const_u16 = 20;
static const ai_u16 conv2d_46_t_out_0_shape_h_const_u16 = 20;

static const ai_u16 conv2d_47_t_in_0_shape_w_const_u16 = 20;
static const ai_u16 conv2d_47_t_in_0_shape_h_const_u16 = 20;
static const ai_u16 conv2d_47_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_47_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_47_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_47_t_out_0_shape_ch_const_u16 = 64;
static const ai_i8 conv2d_47_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_47_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_47_t_in_0_fmt_scale_const_f32 = 0.0055921501480042934f;
static const ai_float conv2d_47_t_out_0_fmt_scale_const_f32 = 0.011582382023334503f;
static const ai_float conv2d_47_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.007178599946200848f, 0.013711689971387386f, 0.008730718865990639f, 0.008559228852391243f, 0.007215472403913736f, 0.012119756080210209f, 0.006732477340847254f, 0.01154287252575159f, 0.00347338174469769f, 0.00807168148458004f, 0.030364874750375748f, 0.007044109981507063f, 0.01783040352165699f, 0.0003869631909765303f, 0.014575662091374397f, 0.011250026524066925f, 0.012401307001709938f, 0.007123367860913277f, 0.013019192032516003f, 0.014115927740931511f, 0.012298285029828548f, 0.013813909143209457f, 0.012204308062791824f, 0.005378184840083122f, 0.004118338692933321f, 0.019795510917901993f, 0.014469259418547153f, 0.01356459315866232f, 0.021380670368671417f, 0.007443297188729048f, 0.010018914006650448f, 0.0038716807030141354f, 0.0071553117595613f, 0.015238115563988686f, 0.020673556253314018f, 0.009590374305844307f, 0.009576660580933094f, 0.021192334592342377f, 0.014865157194435596f, 0.007759467698633671f, 0.026286443695425987f, 0.014861281029880047f, 0.006469743326306343f, 0.016968470066785812f, 0.008864186704158783f, 0.007529925089329481f, 0.008635765872895718f, 0.018653593957424164f, 0.011582382023334503f, 0.0033111271914094687f, 0.01600705087184906f, 0.009791918098926544f, 0.012024523690342903f, 0.01983579248189926f, 0.015480396337807178f, 0.019283900037407875f, 0.010500476695597172f, 0.008827056735754013f, 0.012781881727278233f, 0.016441447660326958f, 0.006399929989129305f, 0.01039449218660593f, 0.013482608832418919f, 0.007879111915826797f);
static const ai_layer_format_type conv2d_47_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_48_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_48_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_48_pad_before_t_in_0_shape_h_const_u32 = 20;

static const ai_u16 conv2d_48_t_in_0_shape_w_const_u16 = 22;
static const ai_u16 conv2d_48_t_in_0_shape_h_const_u16 = 22;
static const ai_u16 conv2d_48_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_48_t_out_0_shape_ch_const_u16 = 2;
static const ai_i8 conv2d_48_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_48_t_out_0_fmt_zero_const_s8 = 92;
static const ai_float conv2d_48_t_in_0_fmt_scale_const_f32 = 0.011582382023334503f;
static const ai_float conv2d_48_t_out_0_fmt_scale_const_f32 = 0.0390465185046196f;
static const ai_float conv2d_48_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.002944828709587455f, 0.002317798789590597f);
static const ai_layer_format_type conv2d_48_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_48_t_out_0_shape_w_const_u16 = 20;
static const ai_u16 conv2d_48_t_out_0_shape_h_const_u16 = 20;


static const ai_i8 conv2d_51_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_51_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_51_pad_before_t_in_0_shape_h_const_u32 = 20;

static const ai_u16 conv2d_51_t_in_0_shape_w_const_u16 = 22;
static const ai_u16 conv2d_51_t_in_0_shape_h_const_u16 = 22;
static const ai_u16 conv2d_51_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_51_t_out_0_shape_ch_const_u16 = 8;
static const ai_i8 conv2d_51_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_51_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_51_t_in_0_fmt_scale_const_f32 = 0.011582382023334503f;
static const ai_float conv2d_51_t_out_0_fmt_scale_const_f32 = 0.019490458071231842f;
static const ai_float conv2d_51_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0015113323461264372f, 0.0013797841966152191f, 0.0018689797725528479f, 0.001861316035501659f, 0.0017348702531307936f, 0.001744130626320839f, 0.0014385957038030028f, 0.0015630864072591066f);
static const ai_layer_format_type conv2d_51_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_51_t_out_0_shape_w_const_u16 = 20;
static const ai_u16 conv2d_51_t_out_0_shape_h_const_u16 = 20;

static const ai_i8 conv2d_53_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_53_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_53_pad_before_t_in_0_shape_h_const_u32 = 20;

static const ai_u16 conv2d_53_t_in_0_shape_w_const_u16 = 22;
static const ai_u16 conv2d_53_t_in_0_shape_h_const_u16 = 22;
static const ai_u16 conv2d_53_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_53_t_out_0_shape_ch_const_u16 = 20;
static const ai_i8 conv2d_53_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_53_t_out_0_fmt_zero_const_s8 = -5;
static const ai_float conv2d_53_t_in_0_fmt_scale_const_f32 = 0.011582382023334503f;
static const ai_float conv2d_53_t_out_0_fmt_scale_const_f32 = 0.02451060339808464f;
static const ai_float conv2d_53_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0010434421710669994f, 0.0013363852631300688f, 0.0011691527906805277f, 0.001355471438728273f, 0.0012057666899636388f, 0.0014932575868442655f, 0.0009830291382968426f, 0.0014296253211796284f, 0.001066021854057908f, 0.001441705971956253f, 0.00134635204449296f, 0.0012301403330639005f, 0.0012215799652040005f, 0.0012813430512323976f, 0.001440109102986753f, 0.0013213447527959943f, 0.001093618804588914f, 0.0011255594436079264f, 0.001230191090144217f, 0.001160263316705823f);
static const ai_layer_format_type conv2d_53_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_53_t_out_0_shape_w_const_u16 = 20;
static const ai_u16 conv2d_53_t_out_0_shape_h_const_u16 = 20;

static const ai_i8 pad_55_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-6);
static const ai_i16 pad_55_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 pad_55_t_in_0_shape_h_const_u32 = 20;

static const ai_u16 conv2d_56_t_in_0_shape_w_const_u16 = 22;
static const ai_u16 conv2d_56_t_in_0_shape_h_const_u16 = 22;
static const ai_u16 conv2d_56_t_in_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_56_t_out_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_56_t_weight_0_shape_w_const_u16 = 3;
static const ai_u16 conv2d_56_t_weight_0_shape_h_const_u16 = 3;
static const ai_u16 conv2d_56_l_stride_1_const_u16 = 2;
static const ai_u16 conv2d_56_l_stride_0_const_u16 = 2;
static const ai_i32 conv2d_56_l_pad_W_0_const_s32 = 0;
static const ai_i32 conv2d_56_l_pad_H_0_const_s32 = 0;
static const ai_i8 conv2d_56_t_in_0_fmt_zero_const_s8 = -6;
static const ai_i8 conv2d_56_t_out_0_fmt_zero_const_s8 = -17;
static const ai_float conv2d_56_t_in_0_fmt_scale_const_f32 = 0.001759668462909758f;
static const ai_float conv2d_56_t_out_0_fmt_scale_const_f32 = 0.0006985461805015802f;
static const ai_float conv2d_56_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0005398452049121261f, 0.0006289698649197817f, 0.0004214309446979314f, 0.0005510823684744537f, 0.0003545499057509005f, 0.0004210655752103776f, 0.0005230073584243655f, 0.0003864141763187945f, 0.0004243951116222888f, 0.00048378962674178183f, 0.00042984148603864014f, 0.00039625391946174204f, 0.00044710811926051974f, 0.0007295834948308766f, 0.00042767723789438605f, 0.0004864720976911485f);
static const ai_layer_format_type conv2d_56_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_56_t_out_0_shape_w_const_u16 = 10;
static const ai_u16 conv2d_56_t_out_0_shape_h_const_u16 = 10;


static const ai_i8 conv2d_73_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(1);
static const ai_i16 conv2d_73_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_73_pad_before_t_in_0_shape_h_const_u32 = 10;

static const ai_u16 conv2d_73_t_in_0_shape_w_const_u16 = 12;
static const ai_u16 conv2d_73_t_in_0_shape_h_const_u16 = 12;
static const ai_u16 conv2d_73_t_in_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_73_t_out_0_shape_ch_const_u16 = 16;
static const ai_i8 conv2d_73_t_in_0_fmt_zero_const_s8 = 1;
static const ai_i8 conv2d_73_t_out_0_fmt_zero_const_s8 = 16;
static const ai_float conv2d_73_t_in_0_fmt_scale_const_f32 = 0.0009802478598430753f;
static const ai_float conv2d_73_t_out_0_fmt_scale_const_f32 = 0.0006639527273364365f;
static const ai_float conv2d_73_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0008198376162908971f, 0.000807957025244832f, 0.001013173721730709f, 0.0007809032103978097f, 0.0008640320738777518f, 0.0011820601066574454f, 0.0007694935193285346f, 0.0007342479657381773f, 0.0011625643819570541f, 0.0005791242583654821f, 0.00088417500955984f, 0.0010054927552118897f, 0.0008714107098057866f, 0.0009189904085360467f, 0.0007560861995443702f, 0.0006768339080736041f);
static const ai_layer_format_type conv2d_73_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_73_t_out_0_shape_w_const_u16 = 10;
static const ai_u16 conv2d_73_t_out_0_shape_h_const_u16 = 10;

static const ai_i8 conv2d_74_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(16);
static const ai_i16 conv2d_74_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_74_pad_before_t_in_0_shape_h_const_u32 = 10;

static const ai_u16 conv2d_74_t_in_0_shape_w_const_u16 = 12;
static const ai_u16 conv2d_74_t_in_0_shape_h_const_u16 = 12;
static const ai_u16 conv2d_74_t_in_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_74_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_74_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_74_t_in_0_fmt_zero_const_s8 = 16;
static const ai_i8 conv2d_74_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_74_t_in_0_fmt_scale_const_f32 = 0.0006639527273364365f;
static const ai_float conv2d_74_t_out_0_fmt_scale_const_f32 = 0.002446890575811267f;
static const ai_float conv2d_74_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.07649308443069458f, 0.08330236375331879f, 0.08336204290390015f, 0.05160045996308327f, 0.06379500776529312f, 0.04182526096701622f, 0.047440752387046814f, 0.028814729303121567f, 0.08183059841394424f, 0.062498945742845535f, 0.05910142511129379f, 0.0841669961810112f, 0.046268489211797714f, 0.0604732520878315f, 0.0395137257874012f, 0.06027912348508835f);
static const ai_u16 conv2d_74_t_out_0_shape_w_const_u16 = 10;
static const ai_u16 conv2d_74_t_out_0_shape_h_const_u16 = 10;

static const ai_u16 conv2d_75_t_in_0_shape_w_const_u16 = 10;
static const ai_u16 conv2d_75_t_in_0_shape_h_const_u16 = 10;
static const ai_u16 conv2d_75_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_75_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_75_t_in_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_75_t_out_0_shape_ch_const_u16 = 64;
static const ai_i8 conv2d_75_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_75_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_75_t_in_0_fmt_scale_const_f32 = 0.002446890575811267f;
static const ai_float conv2d_75_t_out_0_fmt_scale_const_f32 = 0.0035019763745367527f;
static const ai_float conv2d_75_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.00752221280708909f, 0.004885881207883358f, 0.0033804585691541433f, 0.006849487312138081f, 0.0029488166328519583f, 0.004441870376467705f, 0.0019178451038897038f, 0.0020083540584892035f, 0.0054315002635121346f, 0.007245756220072508f, 0.0027773575857281685f, 0.00412715645506978f, 0.003164731664583087f, 0.003036211710423231f, 0.003238922916352749f, 0.004095171112567186f, 0.002144651720300317f, 0.0024264338426291943f, 0.0035330727696418762f, 0.005169901065528393f, 0.0033230120316147804f, 0.0035917069762945175f, 0.004273476079106331f, 0.0021680013742297888f, 0.010677283629775047f, 0.003007396822795272f, 0.006892566103488207f, 0.010487326420843601f, 0.003982368391007185f, 0.0034875248093158007f, 0.0037580630742013454f, 0.005040638614445925f, 0.008620464242994785f, 0.002661177422851324f, 0.0037139225751161575f, 0.0021208347752690315f, 0.003718302585184574f, 0.0041496045887470245f, 0.0016854183049872518f, 0.005983730778098106f, 0.0028805227484554052f, 0.0022410955280065536f, 0.005864986218512058f, 0.0024966965429484844f, 0.002444094978272915f, 0.004308254458010197f, 0.002252511912956834f, 0.0031197122298181057f, 0.002388477325439453f, 0.002861460205167532f, 0.0043130177073180676f, 0.013751897029578686f, 0.004879657179117203f, 0.0028510920237749815f, 0.005683120805770159f, 0.003244265215471387f, 0.011992069892585278f, 0.004812481347471476f, 0.0045089698396623135f, 0.008389394730329514f, 0.0044339424930512905f, 0.005559967830777168f, 0.0033271813299506903f, 0.003988384269177914f);
static const ai_layer_format_type conv2d_75_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_76_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_76_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_76_pad_before_t_in_0_shape_h_const_u32 = 10;

static const ai_u16 conv2d_76_t_in_0_shape_w_const_u16 = 12;
static const ai_u16 conv2d_76_t_in_0_shape_h_const_u16 = 12;
static const ai_u16 conv2d_76_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_76_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_76_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_76_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_76_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_76_t_in_0_fmt_scale_const_f32 = 0.0035019763745367527f;
static const ai_float conv2d_76_t_out_0_fmt_scale_const_f32 = 0.004130890127271414f;
static const ai_float conv2d_76_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.009577402845025063f, 0.013123457320034504f, 0.0102116409689188f, 0.00473289517685771f, 0.015967926010489464f, 0.011213692836463451f, 0.009832213632762432f, 0.004685616586357355f, 0.0037391704972833395f, 0.0029079956002533436f, 0.01053850818425417f, 0.004279468208551407f, 0.010525738820433617f, 0.010103121399879456f, 0.011396009474992752f, 0.011414767242968082f, 0.00920164491981268f, 0.023299183696508408f, 0.008572297170758247f, 0.008060768246650696f, 0.005404376890510321f, 0.005258414428681135f, 0.0062547847628593445f, 0.006699664052575827f, 0.004435788374394178f, 0.011938597075641155f, 0.003501506522297859f, 0.002853194484487176f, 0.011782677844166756f, 0.007588318083435297f, 0.005849395878612995f, 0.009989404119551182f, 0.008025247603654861f, 0.0062560755759477615f, 0.013891935348510742f, 0.007222307380288839f, 0.007339596748352051f, 0.009529538452625275f, 0.008133999072015285f, 0.006282138172537088f, 0.011452971026301384f, 0.0182130616158247f, 0.009282710030674934f, 0.01335986703634262f, 0.014598915353417397f, 0.007358937989920378f, 0.007561901584267616f, 0.006795017514377832f, 0.011119935661554337f, 0.00460032606497407f, 0.009105361998081207f, 0.007422712165862322f, 0.010638275183737278f, 0.0066315489821136f, 0.006455772556364536f, 0.005595686845481396f, 0.004155527800321579f, 0.010470512323081493f, 0.008246684446930885f, 0.0085986889898777f, 0.007822246290743351f, 0.00942886434495449f, 0.010627953335642815f, 0.013431739993393421f);
static const ai_u16 conv2d_76_t_out_0_shape_w_const_u16 = 10;
static const ai_u16 conv2d_76_t_out_0_shape_h_const_u16 = 10;

static const ai_u16 conv2d_77_t_in_0_shape_w_const_u16 = 10;
static const ai_u16 conv2d_77_t_in_0_shape_h_const_u16 = 10;
static const ai_u16 conv2d_77_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_77_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_77_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_77_t_out_0_shape_ch_const_u16 = 64;
static const ai_i8 conv2d_77_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_77_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_77_t_in_0_fmt_scale_const_f32 = 0.004130890127271414f;
static const ai_float conv2d_77_t_out_0_fmt_scale_const_f32 = 0.010677007026970387f;
static const ai_float conv2d_77_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.010333627462387085f, 0.014884773641824722f, 0.011042214930057526f, 0.019576426595449448f, 0.02008722722530365f, 0.02582165040075779f, 0.027492817491292953f, 0.010362918488681316f, 0.02046356163918972f, 0.016864294186234474f, 0.012205559760332108f, 0.022206757217645645f, 0.012046240270137787f, 0.020132293924689293f, 0.015294783748686314f, 0.02840513549745083f, 0.016330866143107414f, 0.012914635241031647f, 0.01727301999926567f, 0.011980080977082253f, 0.024553291499614716f, 0.016938818618655205f, 0.014256841503083706f, 0.00868278369307518f, 0.012441585771739483f, 0.009438317269086838f, 0.022672103717923164f, 0.019689621403813362f, 0.014189834706485271f, 0.01589272916316986f, 0.0178451556712389f, 0.021338550373911858f, 0.02330993115901947f, 0.020844588056206703f, 0.023829594254493713f, 0.017930228263139725f, 1.9448198145255446e-05f, 0.012459988705813885f, 0.013228284195065498f, 0.020857572555541992f, 0.009722667746245861f, 0.015713581815361977f, 0.026467256247997284f, 7.532092422479764e-05f, 0.022365685552358627f, 0.012908363714814186f, 0.013787860982120037f, 0.01967690885066986f, 0.009194777347147465f, 0.00436065811663866f, 0.02145860530436039f, 0.02024766243994236f, 0.012604246847331524f, 0.009253191761672497f, 0.008837657980620861f, 0.014384903013706207f, 0.02353166602551937f, 0.021517951041460037f, 0.023068875074386597f, 0.017543016001582146f, 0.015499565750360489f, 0.01897832192480564f, 0.016753071919083595f, 0.02632272057235241f);
static const ai_layer_format_type conv2d_77_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_78_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_78_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_78_pad_before_t_in_0_shape_h_const_u32 = 10;

static const ai_u16 conv2d_78_t_in_0_shape_w_const_u16 = 12;
static const ai_u16 conv2d_78_t_in_0_shape_h_const_u16 = 12;
static const ai_u16 conv2d_78_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_78_t_out_0_shape_ch_const_u16 = 2;
static const ai_i8 conv2d_78_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_78_t_out_0_fmt_zero_const_s8 = 61;
static const ai_float conv2d_78_t_in_0_fmt_scale_const_f32 = 0.010677007026970387f;
static const ai_float conv2d_78_t_out_0_fmt_scale_const_f32 = 0.036569274961948395f;
static const ai_float conv2d_78_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0026410946156829596f, 0.002065754495561123f);
static const ai_layer_format_type conv2d_78_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_78_t_out_0_shape_w_const_u16 = 10;
static const ai_u16 conv2d_78_t_out_0_shape_h_const_u16 = 10;


static const ai_i8 conv2d_81_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_81_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_81_pad_before_t_in_0_shape_h_const_u32 = 10;

static const ai_u16 conv2d_81_t_in_0_shape_w_const_u16 = 12;
static const ai_u16 conv2d_81_t_in_0_shape_h_const_u16 = 12;
static const ai_u16 conv2d_81_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_81_t_out_0_shape_ch_const_u16 = 8;
static const ai_i8 conv2d_81_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_81_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_81_t_in_0_fmt_scale_const_f32 = 0.010677007026970387f;
static const ai_float conv2d_81_t_out_0_fmt_scale_const_f32 = 0.01881924830377102f;
static const ai_float conv2d_81_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0013322110753506422f, 0.0014809599379077554f, 0.001054398133419454f, 0.0012130015529692173f, 0.0009512786054983735f, 0.0009876033291220665f, 0.0012071996461600065f, 0.001142806140705943f);
static const ai_layer_format_type conv2d_81_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_81_t_out_0_shape_w_const_u16 = 10;
static const ai_u16 conv2d_81_t_out_0_shape_h_const_u16 = 10;

static const ai_i8 conv2d_83_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_83_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_83_pad_before_t_in_0_shape_h_const_u32 = 10;

static const ai_u16 conv2d_83_t_in_0_shape_w_const_u16 = 12;
static const ai_u16 conv2d_83_t_in_0_shape_h_const_u16 = 12;
static const ai_u16 conv2d_83_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_83_t_out_0_shape_ch_const_u16 = 20;
static const ai_i8 conv2d_83_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_83_t_out_0_fmt_zero_const_s8 = -18;
static const ai_float conv2d_83_t_in_0_fmt_scale_const_f32 = 0.010677007026970387f;
static const ai_float conv2d_83_t_out_0_fmt_scale_const_f32 = 0.023661937564611435f;
static const ai_float conv2d_83_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0009109099046327174f, 0.0009357962990179658f, 0.0007544374675489962f, 0.00082877412205562f, 0.000987847219221294f, 0.0006771260523237288f, 0.0008753295405767858f, 0.0006077648722566664f, 0.0008640820742584765f, 0.00070779281668365f, 0.0007391735562123358f, 0.0009382826392538846f, 0.0006853863596916199f, 0.000962286489084363f, 0.0008615648839622736f, 0.0006113447598181665f, 0.0007220425177365541f, 0.000559472304303199f, 0.0010998883517459035f, 0.0008852346800267696f);
static const ai_layer_format_type conv2d_83_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_83_t_out_0_shape_w_const_u16 = 10;
static const ai_u16 conv2d_83_t_out_0_shape_h_const_u16 = 10;

static const ai_i8 pad_58_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(1);
static const ai_i16 pad_58_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 pad_58_t_in_0_shape_h_const_u32 = 10;

static const ai_u16 conv2d_59_t_in_0_shape_w_const_u16 = 12;
static const ai_u16 conv2d_59_t_in_0_shape_h_const_u16 = 12;
static const ai_u16 conv2d_59_t_in_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_59_t_out_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_59_t_weight_0_shape_w_const_u16 = 3;
static const ai_u16 conv2d_59_t_weight_0_shape_h_const_u16 = 3;
static const ai_u16 conv2d_59_l_stride_1_const_u16 = 2;
static const ai_u16 conv2d_59_l_stride_0_const_u16 = 2;
static const ai_i32 conv2d_59_l_pad_W_0_const_s32 = 0;
static const ai_i32 conv2d_59_l_pad_H_0_const_s32 = 0;
static const ai_i8 conv2d_59_t_in_0_fmt_zero_const_s8 = 1;
static const ai_i8 conv2d_59_t_out_0_fmt_zero_const_s8 = 25;
static const ai_float conv2d_59_t_in_0_fmt_scale_const_f32 = 0.0009802478598430753f;
static const ai_float conv2d_59_t_out_0_fmt_scale_const_f32 = 0.0002102396683767438f;
static const ai_float conv2d_59_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.00019337474077474326f, 0.00023558303655590862f, 0.00016626089927740395f, 0.00013003031199332327f, 0.00023444421822205186f, 0.0002342451480217278f, 0.00016036670422181487f, 0.00019997070194222033f, 0.00022534671006724238f, 0.00022042774071451277f, 0.00018918259593192488f, 0.000177176232682541f, 0.00021475032554008067f, 0.00022071869170758873f, 0.0001492311421316117f, 0.00018595227447804064f);
static const ai_layer_format_type conv2d_59_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_59_t_out_0_shape_w_const_u16 = 5;
static const ai_u16 conv2d_59_t_out_0_shape_h_const_u16 = 5;


static const ai_i8 conv2d_61_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(18);
static const ai_i16 conv2d_61_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_61_pad_before_t_in_0_shape_h_const_u32 = 5;

static const ai_u16 conv2d_61_t_in_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_61_t_in_0_shape_h_const_u16 = 7;
static const ai_u16 conv2d_61_t_in_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_61_t_out_0_shape_ch_const_u16 = 16;
static const ai_i8 conv2d_61_t_in_0_fmt_zero_const_s8 = 18;
static const ai_i8 conv2d_61_t_out_0_fmt_zero_const_s8 = 0;
static const ai_float conv2d_61_t_in_0_fmt_scale_const_f32 = 0.00038219871930778027f;
static const ai_float conv2d_61_t_out_0_fmt_scale_const_f32 = 0.00044369601528160274f;
static const ai_float conv2d_61_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.00031145065440796316f, 0.0005149797652848065f, 0.00043502365588210523f, 0.0005935768713243306f, 0.0004313491517677903f, 0.000432314962381497f, 0.0004248709883540869f, 0.0007554746698588133f, 0.00041761744068935513f, 0.00036213448038324714f, 0.0005389009020291269f, 0.00040376133983954787f, 0.00046172080328688025f, 0.0004511683655437082f, 0.00047562431427650154f, 0.0004218352260068059f);
static const ai_layer_format_type conv2d_61_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_61_t_out_0_shape_w_const_u16 = 5;
static const ai_u16 conv2d_61_t_out_0_shape_h_const_u16 = 5;

static const ai_i8 conv2d_62_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(0);
static const ai_i16 conv2d_62_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_62_pad_before_t_in_0_shape_h_const_u32 = 5;

static const ai_u16 conv2d_62_t_in_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_62_t_in_0_shape_h_const_u16 = 7;
static const ai_u16 conv2d_62_t_in_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_62_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_62_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_62_t_in_0_fmt_zero_const_s8 = 0;
static const ai_i8 conv2d_62_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_62_t_in_0_fmt_scale_const_f32 = 0.00044369601528160274f;
static const ai_float conv2d_62_t_out_0_fmt_scale_const_f32 = 0.0011108251055702567f;
static const ai_float conv2d_62_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.022939912974834442f, 0.026425618678331375f, 0.02356921322643757f, 0.01634649559855461f, 0.02081017941236496f, 0.014211279340088367f, 0.009175100363790989f, 0.013587810099124908f, 0.010054429061710835f, 0.025954751297831535f, 0.025815151631832123f, 0.030296165496110916f, 0.03804542496800423f, 0.02628416195511818f, 0.02408832497894764f, 0.016814418137073517f);
static const ai_u16 conv2d_62_t_out_0_shape_w_const_u16 = 5;
static const ai_u16 conv2d_62_t_out_0_shape_h_const_u16 = 5;

static const ai_u16 conv2d_63_t_in_0_shape_w_const_u16 = 5;
static const ai_u16 conv2d_63_t_in_0_shape_h_const_u16 = 5;
static const ai_u16 conv2d_63_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_63_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_63_t_in_0_shape_ch_const_u16 = 16;
static const ai_u16 conv2d_63_t_out_0_shape_ch_const_u16 = 64;
static const ai_i8 conv2d_63_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_63_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_63_t_in_0_fmt_scale_const_f32 = 0.0011108251055702567f;
static const ai_float conv2d_63_t_out_0_fmt_scale_const_f32 = 0.0011557439574971795f;
static const ai_float conv2d_63_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.008528424426913261f, 0.008978179655969143f, 0.004647127352654934f, 0.00782978069037199f, 0.015086609870195389f, 0.003578969743102789f, 0.010128571651875973f, 0.011661486700177193f, 0.006695986725389957f, 0.009216244332492352f, 0.005840979516506195f, 0.005604551639407873f, 0.0077231768518686295f, 0.006516618188470602f, 0.0073168920353055f, 0.012331916019320488f, 0.00835549645125866f, 0.010857971385121346f, 0.007290467619895935f, 0.0067638009786605835f, 0.009408343583345413f, 0.010805954225361347f, 0.005111913196742535f, 0.007872460409998894f, 0.00765158049762249f, 0.016232673078775406f, 0.00531695457175374f, 0.006034027319401503f, 0.010326380841434002f, 0.0058626881800591946f, 0.004886502865701914f, 0.005103439558297396f, 0.007324635982513428f, 0.008253260515630245f, 0.00780922407284379f, 0.00947487261146307f, 0.011156854219734669f, 0.005244070198386908f, 0.003989572636783123f, 0.008879865519702435f, 0.010218802839517593f, 0.005664169788360596f, 0.004273584578186274f, 0.005517566576600075f, 0.0103714969009161f, 0.006834110245108604f, 0.012003842741250992f, 0.00858275592327118f, 0.006080422550439835f, 0.009673196822404861f, 0.009222890250384808f, 0.006750756874680519f, 0.011032468639314175f, 0.007123224437236786f, 0.007323727943003178f, 0.014567652717232704f, 0.006458402145653963f, 0.009894909337162971f, 0.007033111993223429f, 0.011770406737923622f, 0.00785877462476492f, 0.00657047051936388f, 0.005912614054977894f, 0.014270004816353321f);
static const ai_layer_format_type conv2d_63_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_64_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_64_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_64_pad_before_t_in_0_shape_h_const_u32 = 5;

static const ai_u16 conv2d_64_t_in_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_64_t_in_0_shape_h_const_u16 = 7;
static const ai_u16 conv2d_64_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_64_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_64_l_stride_0_const_u16 = 1;
static const ai_i8 conv2d_64_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_64_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_64_t_in_0_fmt_scale_const_f32 = 0.0011557439574971795f;
static const ai_float conv2d_64_t_out_0_fmt_scale_const_f32 = 0.0012280665105208755f;
static const ai_float conv2d_64_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.009250561706721783f, 0.00846878346055746f, 0.00835095439106226f, 0.007240598555654287f, 0.004091943148523569f, 0.011069541797041893f, 0.01187953818589449f, 0.005463505629450083f, 0.01169815007597208f, 0.0108694052323699f, 0.009341757744550705f, 0.006615754216909409f, 0.011060935445129871f, 0.011707767844200134f, 0.005889652296900749f, 0.005501951090991497f, 0.016803184524178505f, 0.00306888110935688f, 0.008265078999102116f, 0.007301318924874067f, 0.008037393912672997f, 0.013505784794688225f, 0.004884256049990654f, 0.008910068310797215f, 0.01022039633244276f, 0.006122393533587456f, 0.007909695617854595f, 0.00679551437497139f, 0.0050844247452914715f, 0.003913138993084431f, 0.007319064810872078f, 0.0059590572491288185f, 0.006617757957428694f, 0.0077791959047317505f, 0.007059056311845779f, 0.009250044822692871f, 0.00520192738622427f, 0.008836908265948296f, 0.004546152427792549f, 0.005319509189575911f, 0.007004113867878914f, 0.007328670937567949f, 0.0093917865306139f, 0.013805189169943333f, 0.004925891757011414f, 0.006463878322392702f, 0.007947449572384357f, 0.009937649592757225f, 0.012930390425026417f, 0.005833140108734369f, 0.007594786584377289f, 0.007602091412991285f, 0.01334236841648817f, 0.007293212693184614f, 0.009274285286664963f, 0.0038674252573400736f, 0.009221034124493599f, 0.00808036606758833f, 0.009418020024895668f, 0.005408075172454119f, 0.005675629246979952f, 0.00450756074860692f, 0.014303185977041721f, 0.0057080453261733055f);
static const ai_u16 conv2d_64_t_out_0_shape_w_const_u16 = 5;
static const ai_u16 conv2d_64_t_out_0_shape_h_const_u16 = 5;

static const ai_u16 conv2d_65_t_in_0_shape_w_const_u16 = 5;
static const ai_u16 conv2d_65_t_in_0_shape_h_const_u16 = 5;
static const ai_u16 conv2d_65_l_stride_1_const_u16 = 1;
static const ai_u16 conv2d_65_l_stride_0_const_u16 = 1;
static const ai_u16 conv2d_65_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_65_t_out_0_shape_ch_const_u16 = 64;
static const ai_i8 conv2d_65_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_65_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_65_t_in_0_fmt_scale_const_f32 = 0.0012280665105208755f;
static const ai_float conv2d_65_t_out_0_fmt_scale_const_f32 = 0.005131934769451618f;
static const ai_float conv2d_65_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0058990116231143475f, 0.023133285343647003f, 0.023018615320324898f, 0.017107082530856133f, 0.005613075103610754f, 0.023050835356116295f, 0.02119685523211956f, 0.01733982004225254f, 0.04842013120651245f, 0.019836843013763428f, 0.012578470632433891f, 0.019440019503235817f, 0.024757130071520805f, 0.018484044820070267f, 0.007149084471166134f, 0.017017735168337822f, 0.02911391295492649f, 0.02815280295908451f, 0.030077584087848663f, 0.009789453819394112f, 0.025654150173068047f, 0.021463608369231224f, 1.408105617883848e-06f, 0.04037635028362274f, 0.031166670843958855f, 0.011716780252754688f, 0.020067133009433746f, 0.01805400289595127f, 0.030265256762504578f, 0.028853487223386765f, 0.05366593971848488f, 0.025181228294968605f, 0.022661132737994194f, 0.013806157745420933f, 0.010522392578423023f, 0.013933664187788963f, 0.038330286741256714f, 0.01062841061502695f, 0.03299403935670853f, 0.02131531946361065f, 0.02251364476978779f, 0.01489485427737236f, 0.06262286752462387f, 0.020979246124625206f, 0.013025753200054169f, 0.013411971740424633f, 0.023756882175803185f, 0.02318969927728176f, 0.020408853888511658f, 0.024162983521819115f, 0.005653952714055777f, 3.3426003938075155e-06f, 0.003521818434819579f, 0.025593433529138565f, 0.00965343788266182f, 0.005375697743147612f, 0.01571662724018097f, 0.022660359740257263f, 0.019229432567954063f, 0.018778832629323006f, 0.030893167480826378f, 0.030623380094766617f, 0.008224425837397575f, 0.022407038137316704f);
static const ai_layer_format_type conv2d_65_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;

static const ai_i8 conv2d_66_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_66_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_66_pad_before_t_in_0_shape_h_const_u32 = 5;

static const ai_u16 conv2d_66_t_in_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_66_t_in_0_shape_h_const_u16 = 7;
static const ai_u16 conv2d_66_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_66_t_out_0_shape_ch_const_u16 = 2;
static const ai_i8 conv2d_66_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_66_t_out_0_fmt_zero_const_s8 = 127;
static const ai_float conv2d_66_t_in_0_fmt_scale_const_f32 = 0.005131934769451618f;
static const ai_float conv2d_66_t_out_0_fmt_scale_const_f32 = 0.02307526022195816f;
static const ai_float conv2d_66_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.001683653797954321f, 0.0015520922606810927f);
static const ai_layer_format_type conv2d_66_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_66_t_out_0_shape_w_const_u16 = 5;
static const ai_u16 conv2d_66_t_out_0_shape_h_const_u16 = 5;


static const ai_i8 conv2d_69_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_69_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_69_pad_before_t_in_0_shape_h_const_u32 = 5;

static const ai_u16 conv2d_69_t_in_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_69_t_in_0_shape_h_const_u16 = 7;
static const ai_u16 conv2d_69_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_69_t_out_0_shape_ch_const_u16 = 8;
static const ai_i8 conv2d_69_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_69_t_out_0_fmt_zero_const_s8 = -128;
static const ai_float conv2d_69_t_in_0_fmt_scale_const_f32 = 0.005131934769451618f;
static const ai_float conv2d_69_t_out_0_fmt_scale_const_f32 = 0.015103490091860294f;
static const ai_float conv2d_69_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0009285494452342391f, 0.0010349098592996597f, 0.0009503779583610594f, 0.001190097420476377f, 5.673971941178024e-07f, 5.299032750372135e-07f, 5.542021312976431e-07f, 5.311709969646472e-07f);
static const ai_layer_format_type conv2d_69_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_69_t_out_0_shape_w_const_u16 = 5;
static const ai_u16 conv2d_69_t_out_0_shape_h_const_u16 = 5;

static const ai_i8 conv2d_71_pad_before_v_pad_constant_value_const_s8[] = LITE_ARRAY_VALUES(-128);
static const ai_i16 conv2d_71_pad_before_t_in_0_fmt_bitsize_const_s16 = 8;
static const ai_u32 conv2d_71_pad_before_t_in_0_shape_h_const_u32 = 5;

static const ai_u16 conv2d_71_t_in_0_shape_w_const_u16 = 7;
static const ai_u16 conv2d_71_t_in_0_shape_h_const_u16 = 7;
static const ai_u16 conv2d_71_t_in_0_shape_ch_const_u16 = 64;
static const ai_u16 conv2d_71_t_out_0_shape_ch_const_u16 = 20;
static const ai_i8 conv2d_71_t_in_0_fmt_zero_const_s8 = -128;
static const ai_i8 conv2d_71_t_out_0_fmt_zero_const_s8 = -20;
static const ai_float conv2d_71_t_in_0_fmt_scale_const_f32 = 0.005131934769451618f;
static const ai_float conv2d_71_t_out_0_fmt_scale_const_f32 = 0.01342423353344202f;
static const ai_float conv2d_71_t_weight_0_fmt_scale_const_f32[] = LITE_ARRAY_VALUES(0.0006965579814277589f, 0.0008373488672077656f, 0.0008125400054268539f, 0.0007657739333808422f, 0.0008770599961280823f, 0.0007047575199976563f, 0.0007075313478708267f, 0.0006773094646632671f, 0.0007203716668300331f, 0.0006868905038572848f, 5.642226824420504e-05f, 4.3109088437631726e-05f, 5.4042175179347396e-05f, 4.3206309783272445e-05f, 3.1572268198942766e-05f, 4.139283555559814e-05f, 5.027929728385061e-05f, 7.613204070366919e-05f, 4.973559043719433e-05f, 7.586360152345151e-05f);
static const ai_layer_format_type conv2d_71_l_out_ch_format_const_layer_format_type = AI_LAYER_FORMAT_CHANNEL_LAST_VALID;
static const ai_u16 conv2d_71_t_out_0_shape_w_const_u16 = 5;
static const ai_u16 conv2d_71_t_out_0_shape_h_const_u16 = 5;
STAI_API_ENTRY
stai_return_code stai_network_run(
  stai_network* network,
  const stai_run_mode mode)
{
   STAI_UNUSED(mode)
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)

  _STAI_SET_ERROR(net_ctx, (net_ctx->_flags & STAI_FLAG_ACTIVATIONS) != STAI_FLAG_ACTIVATIONS,
        STAI_ERROR_NETWORK_INVALID_ACTIVATIONS_PTR, net_ctx->_return_code)

  _STAI_SET_ERROR(net_ctx, (net_ctx->_flags & STAI_FLAG_INPUTS) != STAI_FLAG_INPUTS,
                  STAI_ERROR_NETWORK_INVALID_IN_PTR, net_ctx->_return_code)
  _STAI_SET_ERROR(net_ctx, (net_ctx->_flags & STAI_FLAG_OUTPUTS) != STAI_FLAG_OUTPUTS,
                  STAI_ERROR_NETWORK_INVALID_OUT_PTR, net_ctx->_return_code)

  _STAI_SET_ERROR(net_ctx, (net_ctx->_flags & STAI_FLAG_WEIGHTS) != STAI_FLAG_WEIGHTS,
                  STAI_ERROR_NETWORK_INVALID_WEIGHTS_PTR, net_ctx->_return_code)


  /* LITE_KERNEL_SECTION BEGIN conv2d_1 */
  {
      const ai_i8* conv2d_1_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_inputs[0] + 0);
    const ai_i8* conv2d_1_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 0);
    const ai_i32* conv2d_1_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 432);
    ai_i8* conv2d_1_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 13568);
    ai_i16* conv2d_1_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 117268);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(0, 1, {(stai_ptr) conv2d_1_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_rgb_sssa8_ch(conv2d_1_t_in_0_ptr_const_s8, conv2d_1_t_in_0_shape_w_const_u16, conv2d_1_t_weight_0_ptr_const_s8, conv2d_1_t_out_0_shape_ch_const_u16, conv2d_1_t_weight_0_shape_w_const_u16, conv2d_1_l_pad_W_0_const_s32, conv2d_1_l_stride_0_const_u16, conv2d_1_t_weight_1_ptr_const_s32, conv2d_1_t_in_0_fmt_zero_const_s8, conv2d_1_t_out_0_fmt_zero_const_s8, conv2d_1_t_in_0_fmt_scale_const_f32, conv2d_1_t_out_0_fmt_scale_const_f32, conv2d_1_t_weight_0_fmt_scale_const_f32, conv2d_1_l_out_ch_format_const_layer_format_type, conv2d_1_t_out_0_ptr_s8, conv2d_1_t_out_0_shape_w_const_u16, 256, conv2d_1_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(0, 1, {(stai_ptr) conv2d_1_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_1 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_2_pad_before */
  {
      const ai_ptr conv2d_2_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 13568);
    ai_ptr conv2d_2_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 8384);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(2, 1, {(stai_ptr) conv2d_2_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_2_pad_before_t_in_0_ptr_const_ptr, conv2d_2_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_2_pad_before_v_pad_constant_value_const_s8), conv2d_2_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_2_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(1280), (ai_i32)(1312), (ai_i32)(1312), (ai_i32)(16), (ai_i32)(16));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(2, 1, {(stai_ptr) conv2d_2_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_2_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_2 */
  {
      const ai_i8* conv2d_2_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 8384);
    const ai_i8* conv2d_2_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 496);
    const ai_i32* conv2d_2_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 640);
    ai_i8* conv2d_2_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 7104);
    ai_i16* conv2d_2_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 115968);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(2, 1, {(stai_ptr) conv2d_2_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_2_t_in_0_ptr_const_s8, conv2d_2_t_in_0_shape_w_const_u16, conv2d_2_t_in_0_shape_h_const_u16, conv2d_2_t_in_0_shape_ch_const_u16, conv2d_2_t_weight_0_ptr_const_s8, conv2d_2_l_stride_1_const_u16, conv2d_2_l_stride_0_const_u16, conv2d_2_t_weight_1_ptr_const_s32, conv2d_2_t_in_0_fmt_zero_const_s8, conv2d_2_t_out_0_fmt_zero_const_s8, conv2d_2_t_in_0_fmt_scale_const_f32, conv2d_2_t_out_0_fmt_scale_const_f32, conv2d_2_t_weight_0_fmt_scale_const_f32, conv2d_2_t_out_0_ptr_s8, conv2d_2_t_out_0_shape_w_const_u16, conv2d_2_t_out_0_shape_h_const_u16, 0, 4592, conv2d_2_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(2, 1, {(stai_ptr) conv2d_2_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_2 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_3 */
  {
      const ai_i8* conv2d_3_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 7104);
    const ai_i8* conv2d_3_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 704);
    const ai_i32* conv2d_3_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 960);
    ai_i8* conv2d_3_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 5824);
    ai_i16* conv2d_3_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 120432);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(3, 1, {(stai_ptr) conv2d_3_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_3_t_in_0_ptr_const_s8, conv2d_3_t_in_0_shape_w_const_u16, conv2d_3_t_in_0_shape_h_const_u16, conv2d_3_l_stride_1_const_u16, conv2d_3_l_stride_0_const_u16, conv2d_3_t_in_0_shape_ch_const_u16, conv2d_3_t_weight_0_ptr_const_s8, conv2d_3_t_out_0_shape_ch_const_u16, conv2d_3_t_weight_1_ptr_const_s32, conv2d_3_t_in_0_fmt_zero_const_s8, conv2d_3_t_out_0_fmt_zero_const_s8, conv2d_3_t_in_0_fmt_scale_const_f32, conv2d_3_t_out_0_fmt_scale_const_f32, conv2d_3_t_weight_0_fmt_scale_const_f32, conv2d_3_l_out_ch_format_const_layer_format_type, conv2d_3_t_out_0_ptr_s8, 1, 128, conv2d_3_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(3, 1, {(stai_ptr) conv2d_3_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_3 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_5_pad_before */
  {
      const ai_ptr conv2d_5_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 5824);
    ai_ptr conv2d_5_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 640);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(4, 1, {(stai_ptr) conv2d_5_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_5_pad_before_t_in_0_ptr_const_ptr, conv2d_5_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_5_pad_before_v_pad_constant_value_const_s8), conv2d_5_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_5_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(1280), (ai_i32)(1312), (ai_i32)(1312), (ai_i32)(16), (ai_i32)(16));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(4, 1, {(stai_ptr) conv2d_5_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_5_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_5 */
  {
      const ai_i8* conv2d_5_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 640);
    const ai_i8* conv2d_5_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 1024);
    const ai_i32* conv2d_5_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 1168);
    ai_i8* conv2d_5_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    ai_i16* conv2d_5_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 115968);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(5, 1, {(stai_ptr) conv2d_5_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_5_t_in_0_ptr_const_s8, conv2d_5_t_in_0_shape_w_const_u16, conv2d_5_t_in_0_shape_h_const_u16, conv2d_5_t_in_0_shape_ch_const_u16, conv2d_5_t_weight_0_ptr_const_s8, conv2d_5_l_stride_1_const_u16, conv2d_5_l_stride_0_const_u16, conv2d_5_t_weight_1_ptr_const_s32, conv2d_5_t_in_0_fmt_zero_const_s8, conv2d_5_t_out_0_fmt_zero_const_s8, conv2d_5_t_in_0_fmt_scale_const_f32, conv2d_5_t_out_0_fmt_scale_const_f32, conv2d_5_t_weight_0_fmt_scale_const_f32, conv2d_5_t_out_0_ptr_s8, conv2d_5_t_out_0_shape_w_const_u16, conv2d_5_t_out_0_shape_h_const_u16, 0, 4592, conv2d_5_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(5, 1, {(stai_ptr) conv2d_5_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_5 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_6 */
  {
      const ai_i8* conv2d_6_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    const ai_i8* conv2d_6_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 1232);
    const ai_i32* conv2d_6_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 1872);
    ai_i8* conv2d_6_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 25920);
    ai_i16* conv2d_6_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 25600);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(6, 1, {(stai_ptr) conv2d_6_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_6_t_in_0_ptr_const_s8, conv2d_6_t_in_0_shape_w_const_u16, conv2d_6_t_in_0_shape_h_const_u16, conv2d_6_l_stride_1_const_u16, conv2d_6_l_stride_0_const_u16, conv2d_6_t_in_0_shape_ch_const_u16, conv2d_6_t_weight_0_ptr_const_s8, conv2d_6_t_out_0_shape_ch_const_u16, conv2d_6_t_weight_1_ptr_const_s32, conv2d_6_t_in_0_fmt_zero_const_s8, conv2d_6_t_out_0_fmt_zero_const_s8, conv2d_6_t_in_0_fmt_scale_const_f32, conv2d_6_t_out_0_fmt_scale_const_f32, conv2d_6_t_weight_0_fmt_scale_const_f32, conv2d_6_l_out_ch_format_const_layer_format_type, conv2d_6_t_out_0_ptr_s8, 1, 320, conv2d_6_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(6, 1, {(stai_ptr) conv2d_6_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_6 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_7_pad_before */
  {
      const ai_ptr conv2d_7_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 25920);
    ai_ptr conv2d_7_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 19360);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(7, 1, {(stai_ptr) conv2d_7_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_7_pad_before_t_in_0_ptr_const_ptr, conv2d_7_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_7_pad_before_v_pad_constant_value_const_s8), conv2d_7_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_7_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(1600), (ai_i32)(1680), (ai_i32)(1680), (ai_i32)(40), (ai_i32)(40));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(7, 1, {(stai_ptr) conv2d_7_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_7_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_7 */
  {
      const ai_i8* conv2d_7_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 19360);
    const ai_i8* conv2d_7_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 2032);
    const ai_i32* conv2d_7_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 2392);
    ai_i8* conv2d_7_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 17760);
    ai_i16* conv2d_7_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(7, 1, {(stai_ptr) conv2d_7_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_7_t_in_0_ptr_const_s8, conv2d_7_t_in_0_shape_w_const_u16, conv2d_7_t_in_0_shape_h_const_u16, conv2d_7_t_in_0_shape_ch_const_u16, conv2d_7_t_weight_0_ptr_const_s8, conv2d_7_l_stride_1_const_u16, conv2d_7_l_stride_0_const_u16, conv2d_7_t_weight_1_ptr_const_s32, conv2d_7_t_in_0_fmt_zero_const_s8, conv2d_7_t_out_0_fmt_zero_const_s8, conv2d_7_t_in_0_fmt_scale_const_f32, conv2d_7_t_out_0_fmt_scale_const_f32, conv2d_7_t_weight_0_fmt_scale_const_f32, conv2d_7_t_out_0_ptr_s8, conv2d_7_t_out_0_shape_w_const_u16, conv2d_7_t_out_0_shape_h_const_u16, 0, 4784, conv2d_7_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(7, 1, {(stai_ptr) conv2d_7_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_7 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_8 */
  {
      const ai_i8* conv2d_8_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 17760);
    const ai_i8* conv2d_8_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 2552);
    const ai_i32* conv2d_8_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 4152);
    ai_i8* conv2d_8_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 16160);
    ai_i16* conv2d_8_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(8, 1, {(stai_ptr) conv2d_8_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_8_t_in_0_ptr_const_s8, conv2d_8_t_in_0_shape_w_const_u16, conv2d_8_t_in_0_shape_h_const_u16, conv2d_8_l_stride_1_const_u16, conv2d_8_l_stride_0_const_u16, conv2d_8_t_in_0_shape_ch_const_u16, conv2d_8_t_weight_0_ptr_const_s8, conv2d_8_t_out_0_shape_ch_const_u16, conv2d_8_t_weight_1_ptr_const_s32, conv2d_8_t_in_0_fmt_zero_const_s8, conv2d_8_t_out_0_fmt_zero_const_s8, conv2d_8_t_in_0_fmt_scale_const_f32, conv2d_8_t_out_0_fmt_scale_const_f32, conv2d_8_t_weight_0_fmt_scale_const_f32, conv2d_8_l_out_ch_format_const_layer_format_type, conv2d_8_t_out_0_ptr_s8, 1, 320, conv2d_8_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(8, 1, {(stai_ptr) conv2d_8_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_8 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_10_pad_before */
  {
      const ai_ptr conv2d_10_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 16160);
    ai_ptr conv2d_10_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 9600);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(9, 1, {(stai_ptr) conv2d_10_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_10_pad_before_t_in_0_ptr_const_ptr, conv2d_10_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_10_pad_before_v_pad_constant_value_const_s8), conv2d_10_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_10_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(1600), (ai_i32)(1680), (ai_i32)(1680), (ai_i32)(40), (ai_i32)(40));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(9, 1, {(stai_ptr) conv2d_10_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_10_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_10 */
  {
      const ai_i8* conv2d_10_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 9600);
    const ai_i8* conv2d_10_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 4312);
    const ai_i32* conv2d_10_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 4672);
    ai_i8* conv2d_10_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 80160);
    ai_i16* conv2d_10_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(10, 1, {(stai_ptr) conv2d_10_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_10_t_in_0_ptr_const_s8, conv2d_10_t_in_0_shape_w_const_u16, conv2d_10_t_in_0_shape_h_const_u16, conv2d_10_t_in_0_shape_ch_const_u16, conv2d_10_t_weight_0_ptr_const_s8, conv2d_10_l_stride_1_const_u16, conv2d_10_l_stride_0_const_u16, conv2d_10_t_weight_1_ptr_const_s32, conv2d_10_t_in_0_fmt_zero_const_s8, conv2d_10_t_out_0_fmt_zero_const_s8, conv2d_10_t_in_0_fmt_scale_const_f32, conv2d_10_t_out_0_fmt_scale_const_f32, conv2d_10_t_weight_0_fmt_scale_const_f32, conv2d_10_t_out_0_ptr_s8, conv2d_10_t_out_0_shape_w_const_u16, conv2d_10_t_out_0_shape_h_const_u16, 0, 4784, conv2d_10_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(10, 1, {(stai_ptr) conv2d_10_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_10 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_11 */
  {
      const ai_i8* conv2d_11_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 80160);
    const ai_i8* conv2d_11_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 4832);
    const ai_i32* conv2d_11_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 7712);
    ai_i8* conv2d_11_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 576);
    ai_i16* conv2d_11_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(11, 1, {(stai_ptr) conv2d_11_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_11_t_in_0_ptr_const_s8, conv2d_11_t_in_0_shape_w_const_u16, conv2d_11_t_in_0_shape_h_const_u16, conv2d_11_l_stride_1_const_u16, conv2d_11_l_stride_0_const_u16, conv2d_11_t_in_0_shape_ch_const_u16, conv2d_11_t_weight_0_ptr_const_s8, conv2d_11_t_out_0_shape_ch_const_u16, conv2d_11_t_weight_1_ptr_const_s32, conv2d_11_t_in_0_fmt_zero_const_s8, conv2d_11_t_out_0_fmt_zero_const_s8, conv2d_11_t_in_0_fmt_scale_const_f32, conv2d_11_t_out_0_fmt_scale_const_f32, conv2d_11_t_weight_0_fmt_scale_const_f32, conv2d_11_l_out_ch_format_const_layer_format_type, conv2d_11_t_out_0_ptr_s8, 1, 576, conv2d_11_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(11, 1, {(stai_ptr) conv2d_11_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_11 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_12_pad_before */
  {
      const ai_ptr conv2d_12_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 576);
    ai_ptr conv2d_12_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 29376);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(12, 1, {(stai_ptr) conv2d_12_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_12_pad_before_t_in_0_ptr_const_ptr, conv2d_12_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_12_pad_before_v_pad_constant_value_const_s8), conv2d_12_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_12_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(1440), (ai_i32)(1584), (ai_i32)(1584), (ai_i32)(72), (ai_i32)(72));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(12, 1, {(stai_ptr) conv2d_12_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_12_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_12 */
  {
      const ai_i8* conv2d_12_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 29376);
    const ai_i8* conv2d_12_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 8000);
    const ai_i32* conv2d_12_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 8648);
    ai_i8* conv2d_12_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 64224);
    ai_i16* conv2d_12_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(12, 1, {(stai_ptr) conv2d_12_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_12_t_in_0_ptr_const_s8, conv2d_12_t_in_0_shape_w_const_u16, conv2d_12_t_in_0_shape_h_const_u16, conv2d_12_t_in_0_shape_ch_const_u16, conv2d_12_t_weight_0_ptr_const_s8, conv2d_12_l_stride_1_const_u16, conv2d_12_l_stride_0_const_u16, conv2d_12_t_weight_1_ptr_const_s32, conv2d_12_t_in_0_fmt_zero_const_s8, conv2d_12_t_out_0_fmt_zero_const_s8, conv2d_12_t_in_0_fmt_scale_const_f32, conv2d_12_t_out_0_fmt_scale_const_f32, conv2d_12_t_weight_0_fmt_scale_const_f32, conv2d_12_t_out_0_ptr_s8, conv2d_12_t_out_0_shape_w_const_u16, conv2d_12_t_out_0_shape_h_const_u16, 0, 5040, conv2d_12_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(12, 1, {(stai_ptr) conv2d_12_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_12 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_13 */
  {
      const ai_i8* conv2d_13_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 64224);
    const ai_i8* conv2d_13_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 8936);
    const ai_i32* conv2d_13_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 14120);
    ai_i8* conv2d_13_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 576);
    ai_i16* conv2d_13_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(13, 1, {(stai_ptr) conv2d_13_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_13_t_in_0_ptr_const_s8, conv2d_13_t_in_0_shape_w_const_u16, conv2d_13_t_in_0_shape_h_const_u16, conv2d_13_l_stride_1_const_u16, conv2d_13_l_stride_0_const_u16, conv2d_13_t_in_0_shape_ch_const_u16, conv2d_13_t_weight_0_ptr_const_s8, conv2d_13_t_out_0_shape_ch_const_u16, conv2d_13_t_weight_1_ptr_const_s32, conv2d_13_t_in_0_fmt_zero_const_s8, conv2d_13_t_out_0_fmt_zero_const_s8, conv2d_13_t_in_0_fmt_scale_const_f32, conv2d_13_t_out_0_fmt_scale_const_f32, conv2d_13_t_weight_0_fmt_scale_const_f32, conv2d_13_l_out_ch_format_const_layer_format_type, conv2d_13_t_out_0_ptr_s8, 1, 576, conv2d_13_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(13, 1, {(stai_ptr) conv2d_13_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_13 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_14_pad_before */
  {
      const ai_ptr conv2d_14_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 576);
    ai_ptr conv2d_14_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 29376);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(14, 1, {(stai_ptr) conv2d_14_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_14_pad_before_t_in_0_ptr_const_ptr, conv2d_14_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_14_pad_before_v_pad_constant_value_const_s8), conv2d_14_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_14_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(1440), (ai_i32)(1584), (ai_i32)(1584), (ai_i32)(72), (ai_i32)(72));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(14, 1, {(stai_ptr) conv2d_14_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_14_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_14 */
  {
      const ai_i8* conv2d_14_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 29376);
    const ai_i8* conv2d_14_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 14408);
    const ai_i32* conv2d_14_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 15056);
    ai_i8* conv2d_14_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 64224);
    ai_i16* conv2d_14_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(14, 1, {(stai_ptr) conv2d_14_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_14_t_in_0_ptr_const_s8, conv2d_14_t_in_0_shape_w_const_u16, conv2d_14_t_in_0_shape_h_const_u16, conv2d_14_t_in_0_shape_ch_const_u16, conv2d_14_t_weight_0_ptr_const_s8, conv2d_14_l_stride_1_const_u16, conv2d_14_l_stride_0_const_u16, conv2d_14_t_weight_1_ptr_const_s32, conv2d_14_t_in_0_fmt_zero_const_s8, conv2d_14_t_out_0_fmt_zero_const_s8, conv2d_14_t_in_0_fmt_scale_const_f32, conv2d_14_t_out_0_fmt_scale_const_f32, conv2d_14_t_weight_0_fmt_scale_const_f32, conv2d_14_t_out_0_ptr_s8, conv2d_14_t_out_0_shape_w_const_u16, conv2d_14_t_out_0_shape_h_const_u16, 0, 5040, conv2d_14_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(14, 1, {(stai_ptr) conv2d_14_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_14 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_15 */
  {
      const ai_i8* conv2d_15_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 64224);
    const ai_i8* conv2d_15_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 15344);
    const ai_i32* conv2d_15_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 20528);
    ai_i8* conv2d_15_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 576);
    ai_i16* conv2d_15_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(15, 1, {(stai_ptr) conv2d_15_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_15_t_in_0_ptr_const_s8, conv2d_15_t_in_0_shape_w_const_u16, conv2d_15_t_in_0_shape_h_const_u16, conv2d_15_l_stride_1_const_u16, conv2d_15_l_stride_0_const_u16, conv2d_15_t_in_0_shape_ch_const_u16, conv2d_15_t_weight_0_ptr_const_s8, conv2d_15_t_out_0_shape_ch_const_u16, conv2d_15_t_weight_1_ptr_const_s32, conv2d_15_t_in_0_fmt_zero_const_s8, conv2d_15_t_out_0_fmt_zero_const_s8, conv2d_15_t_in_0_fmt_scale_const_f32, conv2d_15_t_out_0_fmt_scale_const_f32, conv2d_15_t_weight_0_fmt_scale_const_f32, conv2d_15_l_out_ch_format_const_layer_format_type, conv2d_15_t_out_0_ptr_s8, 1, 576, conv2d_15_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(15, 1, {(stai_ptr) conv2d_15_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_15 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_17_pad_before */
  {
      const ai_ptr conv2d_17_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 576);
    ai_ptr conv2d_17_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 29376);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(16, 1, {(stai_ptr) conv2d_17_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_17_pad_before_t_in_0_ptr_const_ptr, conv2d_17_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_17_pad_before_v_pad_constant_value_const_s8), conv2d_17_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_17_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(1440), (ai_i32)(1584), (ai_i32)(1584), (ai_i32)(72), (ai_i32)(72));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(16, 1, {(stai_ptr) conv2d_17_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_17_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_17 */
  {
      const ai_i8* conv2d_17_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 29376);
    const ai_i8* conv2d_17_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 20816);
    const ai_i32* conv2d_17_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 21464);
    ai_i8* conv2d_17_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 69264);
    ai_i16* conv2d_17_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 64224);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(17, 1, {(stai_ptr) conv2d_17_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_17_t_in_0_ptr_const_s8, conv2d_17_t_in_0_shape_w_const_u16, conv2d_17_t_in_0_shape_h_const_u16, conv2d_17_t_in_0_shape_ch_const_u16, conv2d_17_t_weight_0_ptr_const_s8, conv2d_17_l_stride_1_const_u16, conv2d_17_l_stride_0_const_u16, conv2d_17_t_weight_1_ptr_const_s32, conv2d_17_t_in_0_fmt_zero_const_s8, conv2d_17_t_out_0_fmt_zero_const_s8, conv2d_17_t_in_0_fmt_scale_const_f32, conv2d_17_t_out_0_fmt_scale_const_f32, conv2d_17_t_weight_0_fmt_scale_const_f32, conv2d_17_t_out_0_ptr_s8, conv2d_17_t_out_0_shape_w_const_u16, conv2d_17_t_out_0_shape_h_const_u16, 0, 5040, conv2d_17_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(17, 1, {(stai_ptr) conv2d_17_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_17 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_18 */
  {
      const ai_i8* conv2d_18_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 69264);
    const ai_i8* conv2d_18_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 21752);
    const ai_i32* conv2d_18_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 32696);
    ai_i8* conv2d_18_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 30592);
    ai_i16* conv2d_18_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 29376);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(18, 1, {(stai_ptr) conv2d_18_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_18_t_in_0_ptr_const_s8, conv2d_18_t_in_0_shape_w_const_u16, conv2d_18_t_in_0_shape_h_const_u16, conv2d_18_l_stride_1_const_u16, conv2d_18_l_stride_0_const_u16, conv2d_18_t_in_0_shape_ch_const_u16, conv2d_18_t_weight_0_ptr_const_s8, conv2d_18_t_out_0_shape_ch_const_u16, conv2d_18_t_weight_1_ptr_const_s32, conv2d_18_t_in_0_fmt_zero_const_s8, conv2d_18_t_out_0_fmt_zero_const_s8, conv2d_18_t_in_0_fmt_scale_const_f32, conv2d_18_t_out_0_fmt_scale_const_f32, conv2d_18_t_weight_0_fmt_scale_const_f32, conv2d_18_l_out_ch_format_const_layer_format_type, conv2d_18_t_out_0_ptr_s8, 1, 1216, conv2d_18_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(18, 1, {(stai_ptr) conv2d_18_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_18 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_19_pad_before */
  {
      const ai_ptr conv2d_19_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 30592);
    ai_ptr conv2d_19_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 45792);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(19, 1, {(stai_ptr) conv2d_19_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_19_pad_before_t_in_0_ptr_const_ptr, conv2d_19_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_19_pad_before_v_pad_constant_value_const_s8), conv2d_19_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_19_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(1520), (ai_i32)(1824), (ai_i32)(1824), (ai_i32)(152), (ai_i32)(152));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(19, 1, {(stai_ptr) conv2d_19_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_19_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_19 */
  {
      const ai_i8* conv2d_19_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 45792);
    const ai_i8* conv2d_19_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 33304);
    const ai_i32* conv2d_19_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 34672);
    ai_i8* conv2d_19_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 67680);
    ai_i16* conv2d_19_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 29376);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(19, 1, {(stai_ptr) conv2d_19_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_19_t_in_0_ptr_const_s8, conv2d_19_t_in_0_shape_w_const_u16, conv2d_19_t_in_0_shape_h_const_u16, conv2d_19_t_in_0_shape_ch_const_u16, conv2d_19_t_weight_0_ptr_const_s8, conv2d_19_l_stride_1_const_u16, conv2d_19_l_stride_0_const_u16, conv2d_19_t_weight_1_ptr_const_s32, conv2d_19_t_in_0_fmt_zero_const_s8, conv2d_19_t_out_0_fmt_zero_const_s8, conv2d_19_t_in_0_fmt_scale_const_f32, conv2d_19_t_out_0_fmt_scale_const_f32, conv2d_19_t_weight_0_fmt_scale_const_f32, conv2d_19_t_out_0_ptr_s8, conv2d_19_t_out_0_shape_w_const_u16, conv2d_19_t_out_0_shape_h_const_u16, 0, 5680, conv2d_19_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(19, 1, {(stai_ptr) conv2d_19_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_19 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_20 */
  {
      const ai_i8* conv2d_20_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 67680);
    const ai_i8* conv2d_20_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 35280);
    const ai_i32* conv2d_20_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 58384);
    ai_i8* conv2d_20_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 30592);
    ai_i16* conv2d_20_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 29376);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(20, 1, {(stai_ptr) conv2d_20_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_20_t_in_0_ptr_const_s8, conv2d_20_t_in_0_shape_w_const_u16, conv2d_20_t_in_0_shape_h_const_u16, conv2d_20_l_stride_1_const_u16, conv2d_20_l_stride_0_const_u16, conv2d_20_t_in_0_shape_ch_const_u16, conv2d_20_t_weight_0_ptr_const_s8, conv2d_20_t_out_0_shape_ch_const_u16, conv2d_20_t_weight_1_ptr_const_s32, conv2d_20_t_in_0_fmt_zero_const_s8, conv2d_20_t_out_0_fmt_zero_const_s8, conv2d_20_t_in_0_fmt_scale_const_f32, conv2d_20_t_out_0_fmt_scale_const_f32, conv2d_20_t_weight_0_fmt_scale_const_f32, conv2d_20_l_out_ch_format_const_layer_format_type, conv2d_20_t_out_0_ptr_s8, 1, 1216, conv2d_20_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(20, 1, {(stai_ptr) conv2d_20_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_20 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_22_pad_before */
  {
      const ai_ptr conv2d_22_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 30592);
    ai_ptr conv2d_22_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 45792);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(21, 1, {(stai_ptr) conv2d_22_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_22_pad_before_t_in_0_ptr_const_ptr, conv2d_22_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_22_pad_before_v_pad_constant_value_const_s8), conv2d_22_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_22_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(1520), (ai_i32)(1824), (ai_i32)(1824), (ai_i32)(152), (ai_i32)(152));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(21, 1, {(stai_ptr) conv2d_22_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_22_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_22 */
  {
      const ai_i8* conv2d_22_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 45792);
    const ai_i8* conv2d_22_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 58992);
    const ai_i32* conv2d_22_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 60360);
    ai_i8* conv2d_22_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 73360);
    ai_i16* conv2d_22_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 67680);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(22, 1, {(stai_ptr) conv2d_22_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_22_t_in_0_ptr_const_s8, conv2d_22_t_in_0_shape_w_const_u16, conv2d_22_t_in_0_shape_h_const_u16, conv2d_22_t_in_0_shape_ch_const_u16, conv2d_22_t_weight_0_ptr_const_s8, conv2d_22_l_stride_1_const_u16, conv2d_22_l_stride_0_const_u16, conv2d_22_t_weight_1_ptr_const_s32, conv2d_22_t_in_0_fmt_zero_const_s8, conv2d_22_t_out_0_fmt_zero_const_s8, conv2d_22_t_in_0_fmt_scale_const_f32, conv2d_22_t_out_0_fmt_scale_const_f32, conv2d_22_t_weight_0_fmt_scale_const_f32, conv2d_22_t_out_0_ptr_s8, conv2d_22_t_out_0_shape_w_const_u16, conv2d_22_t_out_0_shape_h_const_u16, 0, 5680, conv2d_22_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(22, 1, {(stai_ptr) conv2d_22_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_22 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_23 */
  {
      const ai_i8* conv2d_23_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 73360);
    const ai_i8* conv2d_23_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 60968);
    const ai_i32* conv2d_23_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 104744);
    ai_i8* conv2d_23_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 48096);
    ai_i16* conv2d_23_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 45792);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(23, 1, {(stai_ptr) conv2d_23_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_23_t_in_0_ptr_const_s8, conv2d_23_t_in_0_shape_w_const_u16, conv2d_23_t_in_0_shape_h_const_u16, conv2d_23_l_stride_1_const_u16, conv2d_23_l_stride_0_const_u16, conv2d_23_t_in_0_shape_ch_const_u16, conv2d_23_t_weight_0_ptr_const_s8, conv2d_23_t_out_0_shape_ch_const_u16, conv2d_23_t_weight_1_ptr_const_s32, conv2d_23_t_in_0_fmt_zero_const_s8, conv2d_23_t_out_0_fmt_zero_const_s8, conv2d_23_t_in_0_fmt_scale_const_f32, conv2d_23_t_out_0_fmt_scale_const_f32, conv2d_23_t_weight_0_fmt_scale_const_f32, conv2d_23_l_out_ch_format_const_layer_format_type, conv2d_23_t_out_0_ptr_s8, 1, 2304, conv2d_23_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(23, 1, {(stai_ptr) conv2d_23_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_23 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_24_pad_before */
  {
      const ai_ptr conv2d_24_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 48096);
    ai_ptr conv2d_24_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 55296);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(24, 1, {(stai_ptr) conv2d_24_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_24_pad_before_t_in_0_ptr_const_ptr, conv2d_24_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_24_pad_before_v_pad_constant_value_const_s8), conv2d_24_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_24_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(1440), (ai_i32)(2016), (ai_i32)(2016), (ai_i32)(288), (ai_i32)(288));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(24, 1, {(stai_ptr) conv2d_24_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_24_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_24 */
  {
      const ai_i8* conv2d_24_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 55296);
    const ai_i8* conv2d_24_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 105896);
    const ai_i32* conv2d_24_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 108488);
    ai_i8* conv2d_24_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 69408);
    ai_i16* conv2d_24_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 45792);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(24, 1, {(stai_ptr) conv2d_24_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_24_t_in_0_ptr_const_s8, conv2d_24_t_in_0_shape_w_const_u16, conv2d_24_t_in_0_shape_h_const_u16, conv2d_24_t_in_0_shape_ch_const_u16, conv2d_24_t_weight_0_ptr_const_s8, conv2d_24_l_stride_1_const_u16, conv2d_24_l_stride_0_const_u16, conv2d_24_t_weight_1_ptr_const_s32, conv2d_24_t_in_0_fmt_zero_const_s8, conv2d_24_t_out_0_fmt_zero_const_s8, conv2d_24_t_in_0_fmt_scale_const_f32, conv2d_24_t_out_0_fmt_scale_const_f32, conv2d_24_t_weight_0_fmt_scale_const_f32, conv2d_24_t_out_0_ptr_s8, conv2d_24_t_out_0_shape_w_const_u16, conv2d_24_t_out_0_shape_h_const_u16, 0, 6768, conv2d_24_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(24, 1, {(stai_ptr) conv2d_24_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_24 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_25 */
  {
      const ai_i8* conv2d_25_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 69408);
    const ai_i8* conv2d_25_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 109640);
    const ai_i32* conv2d_25_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 192584);
    ai_i8* conv2d_25_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 48096);
    ai_i16* conv2d_25_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 45792);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(25, 1, {(stai_ptr) conv2d_25_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_25_t_in_0_ptr_const_s8, conv2d_25_t_in_0_shape_w_const_u16, conv2d_25_t_in_0_shape_h_const_u16, conv2d_25_l_stride_1_const_u16, conv2d_25_l_stride_0_const_u16, conv2d_25_t_in_0_shape_ch_const_u16, conv2d_25_t_weight_0_ptr_const_s8, conv2d_25_t_out_0_shape_ch_const_u16, conv2d_25_t_weight_1_ptr_const_s32, conv2d_25_t_in_0_fmt_zero_const_s8, conv2d_25_t_out_0_fmt_zero_const_s8, conv2d_25_t_in_0_fmt_scale_const_f32, conv2d_25_t_out_0_fmt_scale_const_f32, conv2d_25_t_weight_0_fmt_scale_const_f32, conv2d_25_l_out_ch_format_const_layer_format_type, conv2d_25_t_out_0_ptr_s8, 1, 2304, conv2d_25_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(25, 1, {(stai_ptr) conv2d_25_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_25 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_26_pad_before */
  {
      const ai_ptr conv2d_26_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 48096);
    ai_ptr conv2d_26_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 55296);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(26, 1, {(stai_ptr) conv2d_26_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_26_pad_before_t_in_0_ptr_const_ptr, conv2d_26_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_26_pad_before_v_pad_constant_value_const_s8), conv2d_26_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_26_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(1440), (ai_i32)(2016), (ai_i32)(2016), (ai_i32)(288), (ai_i32)(288));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(26, 1, {(stai_ptr) conv2d_26_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_26_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_26 */
  {
      const ai_i8* conv2d_26_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 55296);
    const ai_i8* conv2d_26_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 193736);
    const ai_i32* conv2d_26_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 196328);
    ai_i8* conv2d_26_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 69408);
    ai_i16* conv2d_26_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 45792);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(26, 1, {(stai_ptr) conv2d_26_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_26_t_in_0_ptr_const_s8, conv2d_26_t_in_0_shape_w_const_u16, conv2d_26_t_in_0_shape_h_const_u16, conv2d_26_t_in_0_shape_ch_const_u16, conv2d_26_t_weight_0_ptr_const_s8, conv2d_26_l_stride_1_const_u16, conv2d_26_l_stride_0_const_u16, conv2d_26_t_weight_1_ptr_const_s32, conv2d_26_t_in_0_fmt_zero_const_s8, conv2d_26_t_out_0_fmt_zero_const_s8, conv2d_26_t_in_0_fmt_scale_const_f32, conv2d_26_t_out_0_fmt_scale_const_f32, conv2d_26_t_weight_0_fmt_scale_const_f32, conv2d_26_t_out_0_ptr_s8, conv2d_26_t_out_0_shape_w_const_u16, conv2d_26_t_out_0_shape_h_const_u16, 0, 6768, conv2d_26_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(26, 1, {(stai_ptr) conv2d_26_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_26 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_27 */
  {
      const ai_i8* conv2d_27_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 69408);
    const ai_i8* conv2d_27_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 197480);
    const ai_i32* conv2d_27_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 280424);
    ai_i8* conv2d_27_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 48096);
    ai_i16* conv2d_27_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 45792);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(27, 1, {(stai_ptr) conv2d_27_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_27_t_in_0_ptr_const_s8, conv2d_27_t_in_0_shape_w_const_u16, conv2d_27_t_in_0_shape_h_const_u16, conv2d_27_l_stride_1_const_u16, conv2d_27_l_stride_0_const_u16, conv2d_27_t_in_0_shape_ch_const_u16, conv2d_27_t_weight_0_ptr_const_s8, conv2d_27_t_out_0_shape_ch_const_u16, conv2d_27_t_weight_1_ptr_const_s32, conv2d_27_t_in_0_fmt_zero_const_s8, conv2d_27_t_out_0_fmt_zero_const_s8, conv2d_27_t_in_0_fmt_scale_const_f32, conv2d_27_t_out_0_fmt_scale_const_f32, conv2d_27_t_weight_0_fmt_scale_const_f32, conv2d_27_l_out_ch_format_const_layer_format_type, conv2d_27_t_out_0_ptr_s8, 1, 2304, conv2d_27_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(27, 1, {(stai_ptr) conv2d_27_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_27 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_28_pad_before */
  {
      const ai_ptr conv2d_28_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 48096);
    ai_ptr conv2d_28_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 55296);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(28, 1, {(stai_ptr) conv2d_28_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_28_pad_before_t_in_0_ptr_const_ptr, conv2d_28_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_28_pad_before_v_pad_constant_value_const_s8), conv2d_28_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_28_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(1440), (ai_i32)(2016), (ai_i32)(2016), (ai_i32)(288), (ai_i32)(288));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(28, 1, {(stai_ptr) conv2d_28_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_28_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_28 */
  {
      const ai_i8* conv2d_28_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 55296);
    const ai_i8* conv2d_28_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 281576);
    const ai_i32* conv2d_28_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 284168);
    ai_i8* conv2d_28_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 69408);
    ai_i16* conv2d_28_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 45792);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(28, 1, {(stai_ptr) conv2d_28_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_28_t_in_0_ptr_const_s8, conv2d_28_t_in_0_shape_w_const_u16, conv2d_28_t_in_0_shape_h_const_u16, conv2d_28_t_in_0_shape_ch_const_u16, conv2d_28_t_weight_0_ptr_const_s8, conv2d_28_l_stride_1_const_u16, conv2d_28_l_stride_0_const_u16, conv2d_28_t_weight_1_ptr_const_s32, conv2d_28_t_in_0_fmt_zero_const_s8, conv2d_28_t_out_0_fmt_zero_const_s8, conv2d_28_t_in_0_fmt_scale_const_f32, conv2d_28_t_out_0_fmt_scale_const_f32, conv2d_28_t_weight_0_fmt_scale_const_f32, conv2d_28_t_out_0_ptr_s8, conv2d_28_t_out_0_shape_w_const_u16, conv2d_28_t_out_0_shape_h_const_u16, 0, 6768, conv2d_28_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(28, 1, {(stai_ptr) conv2d_28_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_28 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_29 */
  {
      const ai_i8* conv2d_29_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 69408);
    const ai_i8* conv2d_29_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 285320);
    const ai_i32* conv2d_29_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 368264);
    ai_i8* conv2d_29_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 48096);
    ai_i16* conv2d_29_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 45792);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(29, 1, {(stai_ptr) conv2d_29_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_29_t_in_0_ptr_const_s8, conv2d_29_t_in_0_shape_w_const_u16, conv2d_29_t_in_0_shape_h_const_u16, conv2d_29_l_stride_1_const_u16, conv2d_29_l_stride_0_const_u16, conv2d_29_t_in_0_shape_ch_const_u16, conv2d_29_t_weight_0_ptr_const_s8, conv2d_29_t_out_0_shape_ch_const_u16, conv2d_29_t_weight_1_ptr_const_s32, conv2d_29_t_in_0_fmt_zero_const_s8, conv2d_29_t_out_0_fmt_zero_const_s8, conv2d_29_t_in_0_fmt_scale_const_f32, conv2d_29_t_out_0_fmt_scale_const_f32, conv2d_29_t_weight_0_fmt_scale_const_f32, conv2d_29_l_out_ch_format_const_layer_format_type, conv2d_29_t_out_0_ptr_s8, 1, 2304, conv2d_29_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(29, 1, {(stai_ptr) conv2d_29_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_29 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_30_pad_before */
  {
      const ai_ptr conv2d_30_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 48096);
    ai_ptr conv2d_30_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 55296);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(30, 1, {(stai_ptr) conv2d_30_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_30_pad_before_t_in_0_ptr_const_ptr, conv2d_30_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_30_pad_before_v_pad_constant_value_const_s8), conv2d_30_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_30_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(1440), (ai_i32)(2016), (ai_i32)(2016), (ai_i32)(288), (ai_i32)(288));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(30, 1, {(stai_ptr) conv2d_30_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_30_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_30 */
  {
      const ai_i8* conv2d_30_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 55296);
    const ai_i8* conv2d_30_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 369416);
    const ai_i32* conv2d_30_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 372008);
    ai_i8* conv2d_30_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 69408);
    ai_i16* conv2d_30_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 45792);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(30, 1, {(stai_ptr) conv2d_30_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_30_t_in_0_ptr_const_s8, conv2d_30_t_in_0_shape_w_const_u16, conv2d_30_t_in_0_shape_h_const_u16, conv2d_30_t_in_0_shape_ch_const_u16, conv2d_30_t_weight_0_ptr_const_s8, conv2d_30_l_stride_1_const_u16, conv2d_30_l_stride_0_const_u16, conv2d_30_t_weight_1_ptr_const_s32, conv2d_30_t_in_0_fmt_zero_const_s8, conv2d_30_t_out_0_fmt_zero_const_s8, conv2d_30_t_in_0_fmt_scale_const_f32, conv2d_30_t_out_0_fmt_scale_const_f32, conv2d_30_t_weight_0_fmt_scale_const_f32, conv2d_30_t_out_0_ptr_s8, conv2d_30_t_out_0_shape_w_const_u16, conv2d_30_t_out_0_shape_h_const_u16, 0, 6768, conv2d_30_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(30, 1, {(stai_ptr) conv2d_30_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_30 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_31 */
  {
      const ai_i8* conv2d_31_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 69408);
    const ai_i8* conv2d_31_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 373160);
    const ai_i32* conv2d_31_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 456104);
    ai_i8* conv2d_31_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 48096);
    ai_i16* conv2d_31_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 45792);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(31, 1, {(stai_ptr) conv2d_31_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_31_t_in_0_ptr_const_s8, conv2d_31_t_in_0_shape_w_const_u16, conv2d_31_t_in_0_shape_h_const_u16, conv2d_31_l_stride_1_const_u16, conv2d_31_l_stride_0_const_u16, conv2d_31_t_in_0_shape_ch_const_u16, conv2d_31_t_weight_0_ptr_const_s8, conv2d_31_t_out_0_shape_ch_const_u16, conv2d_31_t_weight_1_ptr_const_s32, conv2d_31_t_in_0_fmt_zero_const_s8, conv2d_31_t_out_0_fmt_zero_const_s8, conv2d_31_t_in_0_fmt_scale_const_f32, conv2d_31_t_out_0_fmt_scale_const_f32, conv2d_31_t_weight_0_fmt_scale_const_f32, conv2d_31_l_out_ch_format_const_layer_format_type, conv2d_31_t_out_0_ptr_s8, 1, 2304, conv2d_31_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(31, 1, {(stai_ptr) conv2d_31_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_31 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_32_pad_before */
  {
      const ai_ptr conv2d_32_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 48096);
    ai_ptr conv2d_32_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 55296);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(32, 1, {(stai_ptr) conv2d_32_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_32_pad_before_t_in_0_ptr_const_ptr, conv2d_32_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_32_pad_before_v_pad_constant_value_const_s8), conv2d_32_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_32_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(1440), (ai_i32)(2016), (ai_i32)(2016), (ai_i32)(288), (ai_i32)(288));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(32, 1, {(stai_ptr) conv2d_32_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_32_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_32 */
  {
      const ai_i8* conv2d_32_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 55296);
    const ai_i8* conv2d_32_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 457256);
    const ai_i32* conv2d_32_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 459848);
    ai_i8* conv2d_32_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 69408);
    ai_i16* conv2d_32_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 45792);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(32, 1, {(stai_ptr) conv2d_32_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_32_t_in_0_ptr_const_s8, conv2d_32_t_in_0_shape_w_const_u16, conv2d_32_t_in_0_shape_h_const_u16, conv2d_32_t_in_0_shape_ch_const_u16, conv2d_32_t_weight_0_ptr_const_s8, conv2d_32_l_stride_1_const_u16, conv2d_32_l_stride_0_const_u16, conv2d_32_t_weight_1_ptr_const_s32, conv2d_32_t_in_0_fmt_zero_const_s8, conv2d_32_t_out_0_fmt_zero_const_s8, conv2d_32_t_in_0_fmt_scale_const_f32, conv2d_32_t_out_0_fmt_scale_const_f32, conv2d_32_t_weight_0_fmt_scale_const_f32, conv2d_32_t_out_0_ptr_s8, conv2d_32_t_out_0_shape_w_const_u16, conv2d_32_t_out_0_shape_h_const_u16, 0, 6768, conv2d_32_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(32, 1, {(stai_ptr) conv2d_32_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_32 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_33 */
  {
      const ai_i8* conv2d_33_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 69408);
    const ai_i8* conv2d_33_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 461000);
    const ai_i32* conv2d_33_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 543944);
    ai_i8* conv2d_33_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 48096);
    ai_i16* conv2d_33_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 45792);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(33, 1, {(stai_ptr) conv2d_33_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_33_t_in_0_ptr_const_s8, conv2d_33_t_in_0_shape_w_const_u16, conv2d_33_t_in_0_shape_h_const_u16, conv2d_33_l_stride_1_const_u16, conv2d_33_l_stride_0_const_u16, conv2d_33_t_in_0_shape_ch_const_u16, conv2d_33_t_weight_0_ptr_const_s8, conv2d_33_t_out_0_shape_ch_const_u16, conv2d_33_t_weight_1_ptr_const_s32, conv2d_33_t_in_0_fmt_zero_const_s8, conv2d_33_t_out_0_fmt_zero_const_s8, conv2d_33_t_in_0_fmt_scale_const_f32, conv2d_33_t_out_0_fmt_scale_const_f32, conv2d_33_t_weight_0_fmt_scale_const_f32, conv2d_33_l_out_ch_format_const_layer_format_type, conv2d_33_t_out_0_ptr_s8, 1, 2304, conv2d_33_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(33, 1, {(stai_ptr) conv2d_33_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_33 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_34 */
  {
      const ai_i8* conv2d_34_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 48096);
    const ai_i8* conv2d_34_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 545096);
    const ai_i32* conv2d_34_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 549704);
    ai_i8* conv2d_34_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 128);
    ai_i16* conv2d_34_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(34, 1, {(stai_ptr) conv2d_34_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_34_t_in_0_ptr_const_s8, conv2d_34_t_in_0_shape_w_const_u16, conv2d_34_t_in_0_shape_h_const_u16, conv2d_34_l_stride_1_const_u16, conv2d_34_l_stride_0_const_u16, conv2d_34_t_in_0_shape_ch_const_u16, conv2d_34_t_weight_0_ptr_const_s8, conv2d_34_t_out_0_shape_ch_const_u16, conv2d_34_t_weight_1_ptr_const_s32, conv2d_34_t_in_0_fmt_zero_const_s8, conv2d_34_t_out_0_fmt_zero_const_s8, conv2d_34_t_in_0_fmt_scale_const_f32, conv2d_34_t_out_0_fmt_scale_const_f32, conv2d_34_t_weight_0_fmt_scale_const_f32, conv2d_34_l_out_ch_format_const_layer_format_type, conv2d_34_t_out_0_ptr_s8, 1, 128, conv2d_34_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(34, 1, {(stai_ptr) conv2d_34_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_34 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_36_pad_before */
  {
      const ai_ptr conv2d_36_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 128);
    ai_ptr conv2d_36_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 29376);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(36, 1, {(stai_ptr) conv2d_36_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_36_pad_before_t_in_0_ptr_const_ptr, conv2d_36_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_36_pad_before_v_pad_constant_value_const_s8), conv2d_36_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_36_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(80), (ai_i32)(112), (ai_i32)(112), (ai_i32)(16), (ai_i32)(16));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(36, 1, {(stai_ptr) conv2d_36_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_36_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_36 */
  {
      const ai_i8* conv2d_36_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 29376);
    const ai_i8* conv2d_36_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 549768);
    const ai_i32* conv2d_36_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 552072);
    ai_i8* conv2d_36_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 30160);
    ai_i16* conv2d_36_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 45792);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(36, 1, {(stai_ptr) conv2d_36_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_deep_3x3_sssa8_ch(conv2d_36_t_in_0_ptr_const_s8, conv2d_36_t_in_0_shape_w_const_u16, conv2d_36_t_in_0_shape_h_const_u16, conv2d_36_t_in_0_shape_ch_const_u16, conv2d_36_t_weight_0_ptr_const_s8, conv2d_36_t_out_0_shape_ch_const_u16, conv2d_36_t_weight_1_ptr_const_s32, conv2d_36_t_in_0_fmt_zero_const_s8, conv2d_36_t_out_0_fmt_zero_const_s8, conv2d_36_t_in_0_fmt_scale_const_f32, conv2d_36_t_out_0_fmt_scale_const_f32, conv2d_36_t_weight_0_fmt_scale_const_f32, conv2d_36_l_out_ch_format_const_layer_format_type, conv2d_36_t_out_0_ptr_s8, conv2d_36_t_out_0_shape_w_const_u16, conv2d_36_t_out_0_shape_h_const_u16, 1, 704, conv2d_36_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(36, 1, {(stai_ptr) conv2d_36_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_36 */
  /* LITE_KERNEL_SECTION BEGIN resize_35 */
  {
    
  forward_lite_upsample_nearest_resize_35(net_ctx);
  }
  /* LITE_KERNEL_SECTION END resize_35 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_37 */
  {
      const ai_i8* conv2d_37_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 30592);
    const ai_i8* conv2d_37_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 552136);
    const ai_i32* conv2d_37_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 554568);
    ai_i8* conv2d_37_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 47392);
    ai_i16* conv2d_37_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(37, 1, {(stai_ptr) conv2d_37_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_37_t_in_0_ptr_const_s8, conv2d_37_t_in_0_shape_w_const_u16, conv2d_37_t_in_0_shape_h_const_u16, conv2d_37_l_stride_1_const_u16, conv2d_37_l_stride_0_const_u16, conv2d_37_t_in_0_shape_ch_const_u16, conv2d_37_t_weight_0_ptr_const_s8, conv2d_37_t_out_0_shape_ch_const_u16, conv2d_37_t_weight_1_ptr_const_s32, conv2d_37_t_in_0_fmt_zero_const_s8, conv2d_37_t_out_0_fmt_zero_const_s8, conv2d_37_t_in_0_fmt_scale_const_f32, conv2d_37_t_out_0_fmt_scale_const_f32, conv2d_37_t_weight_0_fmt_scale_const_f32, conv2d_37_l_out_ch_format_const_layer_format_type, conv2d_37_t_out_0_ptr_s8, 1, 128, conv2d_37_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(37, 1, {(stai_ptr) conv2d_37_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_37 */
  /* LITE_KERNEL_SECTION BEGIN eltwise_38 */
  {
    
  forward_lite_eltwise_integer_INT8_eltwise_38(net_ctx);
  }
  /* LITE_KERNEL_SECTION END eltwise_38 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_40_pad_before */
  {
      const ai_ptr conv2d_40_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 30560);
    ai_ptr conv2d_40_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 32160);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(40, 1, {(stai_ptr) conv2d_40_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_40_pad_before_t_in_0_ptr_const_ptr, conv2d_40_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_40_pad_before_v_pad_constant_value_const_s8), conv2d_40_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_40_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(160), (ai_i32)(192), (ai_i32)(192), (ai_i32)(16), (ai_i32)(16));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(40, 1, {(stai_ptr) conv2d_40_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_40_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_40 */
  {
      const ai_i8* conv2d_40_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 32160);
    const ai_i8* conv2d_40_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 554632);
    const ai_i32* conv2d_40_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 556936);
    ai_i8* conv2d_40_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 34464);
    ai_i16* conv2d_40_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 29376);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(40, 1, {(stai_ptr) conv2d_40_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_deep_3x3_sssa8_ch(conv2d_40_t_in_0_ptr_const_s8, conv2d_40_t_in_0_shape_w_const_u16, conv2d_40_t_in_0_shape_h_const_u16, conv2d_40_t_in_0_shape_ch_const_u16, conv2d_40_t_weight_0_ptr_const_s8, conv2d_40_t_out_0_shape_ch_const_u16, conv2d_40_t_weight_1_ptr_const_s32, conv2d_40_t_in_0_fmt_zero_const_s8, conv2d_40_t_out_0_fmt_zero_const_s8, conv2d_40_t_in_0_fmt_scale_const_f32, conv2d_40_t_out_0_fmt_scale_const_f32, conv2d_40_t_weight_0_fmt_scale_const_f32, conv2d_40_l_out_ch_format_const_layer_format_type, conv2d_40_t_out_0_ptr_s8, conv2d_40_t_out_0_shape_w_const_u16, conv2d_40_t_out_0_shape_h_const_u16, 1, 704, conv2d_40_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(40, 1, {(stai_ptr) conv2d_40_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_40 */
  /* LITE_KERNEL_SECTION BEGIN resize_39 */
  {
    
  forward_lite_upsample_nearest_resize_39(net_ctx);
  }
  /* LITE_KERNEL_SECTION END resize_39 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_41 */
  {
      const ai_i8* conv2d_41_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 576);
    const ai_i8* conv2d_41_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 557000);
    const ai_i32* conv2d_41_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 558152);
    ai_i8* conv2d_41_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 42464);
    ai_i16* conv2d_41_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(41, 1, {(stai_ptr) conv2d_41_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_41_t_in_0_ptr_const_s8, conv2d_41_t_in_0_shape_w_const_u16, conv2d_41_t_in_0_shape_h_const_u16, conv2d_41_l_stride_1_const_u16, conv2d_41_l_stride_0_const_u16, conv2d_41_t_in_0_shape_ch_const_u16, conv2d_41_t_weight_0_ptr_const_s8, conv2d_41_t_out_0_shape_ch_const_u16, conv2d_41_t_weight_1_ptr_const_s32, conv2d_41_t_in_0_fmt_zero_const_s8, conv2d_41_t_out_0_fmt_zero_const_s8, conv2d_41_t_in_0_fmt_scale_const_f32, conv2d_41_t_out_0_fmt_scale_const_f32, conv2d_41_t_weight_0_fmt_scale_const_f32, conv2d_41_l_out_ch_format_const_layer_format_type, conv2d_41_t_out_0_ptr_s8, 1, 128, conv2d_41_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(41, 1, {(stai_ptr) conv2d_41_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_41 */
  /* LITE_KERNEL_SECTION BEGIN eltwise_42 */
  {
    
  forward_lite_eltwise_integer_INT8_eltwise_42(net_ctx);
  }
  /* LITE_KERNEL_SECTION END eltwise_42 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_43_pad_before */
  {
      const ai_ptr conv2d_43_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 0);
    ai_ptr conv2d_43_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 6400);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(43, 1, {(stai_ptr) conv2d_43_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_43_pad_before_t_in_0_ptr_const_ptr, conv2d_43_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_43_pad_before_v_pad_constant_value_const_s8), conv2d_43_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_43_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(320), (ai_i32)(352), (ai_i32)(352), (ai_i32)(16), (ai_i32)(16));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(43, 1, {(stai_ptr) conv2d_43_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_43_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_43 */
  {
      const ai_i8* conv2d_43_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 6400);
    const ai_i8* conv2d_43_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 558216);
    const ai_i32* conv2d_43_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 560520);
    ai_i8* conv2d_43_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 14144);
    ai_i16* conv2d_43_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(43, 1, {(stai_ptr) conv2d_43_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_deep_3x3_sssa8_ch(conv2d_43_t_in_0_ptr_const_s8, conv2d_43_t_in_0_shape_w_const_u16, conv2d_43_t_in_0_shape_h_const_u16, conv2d_43_t_in_0_shape_ch_const_u16, conv2d_43_t_weight_0_ptr_const_s8, conv2d_43_t_out_0_shape_ch_const_u16, conv2d_43_t_weight_1_ptr_const_s32, conv2d_43_t_in_0_fmt_zero_const_s8, conv2d_43_t_out_0_fmt_zero_const_s8, conv2d_43_t_in_0_fmt_scale_const_f32, conv2d_43_t_out_0_fmt_scale_const_f32, conv2d_43_t_weight_0_fmt_scale_const_f32, conv2d_43_l_out_ch_format_const_layer_format_type, conv2d_43_t_out_0_ptr_s8, conv2d_43_t_out_0_shape_w_const_u16, conv2d_43_t_out_0_shape_h_const_u16, 1, 704, conv2d_43_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(43, 1, {(stai_ptr) conv2d_43_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_43 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_44_pad_before */
  {
      const ai_ptr conv2d_44_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 14144);
    ai_ptr conv2d_44_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(44, 1, {(stai_ptr) conv2d_44_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_44_pad_before_t_in_0_ptr_const_ptr, conv2d_44_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_44_pad_before_v_pad_constant_value_const_s8), conv2d_44_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_44_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(320), (ai_i32)(352), (ai_i32)(352), (ai_i32)(16), (ai_i32)(16));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(44, 1, {(stai_ptr) conv2d_44_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_44_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_44 */
  {
      const ai_i8* conv2d_44_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 0);
    const ai_i8* conv2d_44_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 560584);
    const ai_i32* conv2d_44_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 560728);
    ai_i8* conv2d_44_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 20544);
    ai_i16* conv2d_44_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 7744);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(44, 1, {(stai_ptr) conv2d_44_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_44_t_in_0_ptr_const_s8, conv2d_44_t_in_0_shape_w_const_u16, conv2d_44_t_in_0_shape_h_const_u16, conv2d_44_t_in_0_shape_ch_const_u16, conv2d_44_t_weight_0_ptr_const_s8, conv2d_44_l_stride_1_const_u16, conv2d_44_l_stride_0_const_u16, conv2d_44_t_weight_1_ptr_const_s32, conv2d_44_t_in_0_fmt_zero_const_s8, conv2d_44_t_out_0_fmt_zero_const_s8, conv2d_44_t_in_0_fmt_scale_const_f32, conv2d_44_t_out_0_fmt_scale_const_f32, conv2d_44_t_weight_0_fmt_scale_const_f32, conv2d_44_t_out_0_ptr_s8, conv2d_44_t_out_0_shape_w_const_u16, conv2d_44_t_out_0_shape_h_const_u16, 0, 4592, conv2d_44_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(44, 1, {(stai_ptr) conv2d_44_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_44 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_45 */
  {
      const ai_i8* conv2d_45_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 20544);
    const ai_i8* conv2d_45_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 560792);
    const ai_i32* conv2d_45_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 561816);
    ai_i8* conv2d_45_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 36064);
    ai_i16* conv2d_45_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(45, 1, {(stai_ptr) conv2d_45_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_45_t_in_0_ptr_const_s8, conv2d_45_t_in_0_shape_w_const_u16, conv2d_45_t_in_0_shape_h_const_u16, conv2d_45_l_stride_1_const_u16, conv2d_45_l_stride_0_const_u16, conv2d_45_t_in_0_shape_ch_const_u16, conv2d_45_t_weight_0_ptr_const_s8, conv2d_45_t_out_0_shape_ch_const_u16, conv2d_45_t_weight_1_ptr_const_s32, conv2d_45_t_in_0_fmt_zero_const_s8, conv2d_45_t_out_0_fmt_zero_const_s8, conv2d_45_t_in_0_fmt_scale_const_f32, conv2d_45_t_out_0_fmt_scale_const_f32, conv2d_45_t_weight_0_fmt_scale_const_f32, conv2d_45_l_out_ch_format_const_layer_format_type, conv2d_45_t_out_0_ptr_s8, 1, 512, conv2d_45_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(45, 1, {(stai_ptr) conv2d_45_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_45 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_46_pad_before */
  {
      const ai_ptr conv2d_46_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 36064);
    ai_ptr conv2d_46_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 61664);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(46, 1, {(stai_ptr) conv2d_46_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_46_pad_before_t_in_0_ptr_const_ptr, conv2d_46_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_46_pad_before_v_pad_constant_value_const_s8), conv2d_46_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_46_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(1280), (ai_i32)(1408), (ai_i32)(1408), (ai_i32)(64), (ai_i32)(64));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(46, 1, {(stai_ptr) conv2d_46_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_46_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_46 */
  {
      const ai_i8* conv2d_46_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 61664);
    const ai_i8* conv2d_46_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 562072);
    const ai_i32* conv2d_46_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 562648);
    ai_i8* conv2d_46_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 36064);
    ai_i16* conv2d_46_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(46, 1, {(stai_ptr) conv2d_46_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_46_t_in_0_ptr_const_s8, conv2d_46_t_in_0_shape_w_const_u16, conv2d_46_t_in_0_shape_h_const_u16, conv2d_46_t_in_0_shape_ch_const_u16, conv2d_46_t_weight_0_ptr_const_s8, conv2d_46_l_stride_1_const_u16, conv2d_46_l_stride_0_const_u16, conv2d_46_t_weight_1_ptr_const_s32, conv2d_46_t_in_0_fmt_zero_const_s8, conv2d_46_t_out_0_fmt_zero_const_s8, conv2d_46_t_in_0_fmt_scale_const_f32, conv2d_46_t_out_0_fmt_scale_const_f32, conv2d_46_t_weight_0_fmt_scale_const_f32, conv2d_46_t_out_0_ptr_s8, conv2d_46_t_out_0_shape_w_const_u16, conv2d_46_t_out_0_shape_h_const_u16, 0, 4976, conv2d_46_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(46, 1, {(stai_ptr) conv2d_46_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_46 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_47 */
  {
      const ai_i8* conv2d_47_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 36064);
    const ai_i8* conv2d_47_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 562904);
    const ai_i32* conv2d_47_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 567000);
    ai_i8* conv2d_47_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 61664);
    ai_i16* conv2d_47_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(47, 1, {(stai_ptr) conv2d_47_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_47_t_in_0_ptr_const_s8, conv2d_47_t_in_0_shape_w_const_u16, conv2d_47_t_in_0_shape_h_const_u16, conv2d_47_l_stride_1_const_u16, conv2d_47_l_stride_0_const_u16, conv2d_47_t_in_0_shape_ch_const_u16, conv2d_47_t_weight_0_ptr_const_s8, conv2d_47_t_out_0_shape_ch_const_u16, conv2d_47_t_weight_1_ptr_const_s32, conv2d_47_t_in_0_fmt_zero_const_s8, conv2d_47_t_out_0_fmt_zero_const_s8, conv2d_47_t_in_0_fmt_scale_const_f32, conv2d_47_t_out_0_fmt_scale_const_f32, conv2d_47_t_weight_0_fmt_scale_const_f32, conv2d_47_l_out_ch_format_const_layer_format_type, conv2d_47_t_out_0_ptr_s8, 1, 512, conv2d_47_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(47, 1, {(stai_ptr) conv2d_47_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_47 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_48_pad_before */
  {
      const ai_ptr conv2d_48_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 61664);
    ai_ptr conv2d_48_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 87264);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(48, 1, {(stai_ptr) conv2d_48_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_48_pad_before_t_in_0_ptr_const_ptr, conv2d_48_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_48_pad_before_v_pad_constant_value_const_s8), conv2d_48_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_48_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(1280), (ai_i32)(1408), (ai_i32)(1408), (ai_i32)(64), (ai_i32)(64));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(48, 1, {(stai_ptr) conv2d_48_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_48_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_48 */
  {
      const ai_i8* conv2d_48_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 87264);
    const ai_i8* conv2d_48_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 567256);
    const ai_i32* conv2d_48_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 568408);
    ai_i8* conv2d_48_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 2320);
    ai_i16* conv2d_48_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 0);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(48, 1, {(stai_ptr) conv2d_48_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_deep_3x3_sssa8_ch(conv2d_48_t_in_0_ptr_const_s8, conv2d_48_t_in_0_shape_w_const_u16, conv2d_48_t_in_0_shape_h_const_u16, conv2d_48_t_in_0_shape_ch_const_u16, conv2d_48_t_weight_0_ptr_const_s8, conv2d_48_t_out_0_shape_ch_const_u16, conv2d_48_t_weight_1_ptr_const_s32, conv2d_48_t_in_0_fmt_zero_const_s8, conv2d_48_t_out_0_fmt_zero_const_s8, conv2d_48_t_in_0_fmt_scale_const_f32, conv2d_48_t_out_0_fmt_scale_const_f32, conv2d_48_t_weight_0_fmt_scale_const_f32, conv2d_48_l_out_ch_format_const_layer_format_type, conv2d_48_t_out_0_ptr_s8, conv2d_48_t_out_0_shape_w_const_u16, conv2d_48_t_out_0_shape_h_const_u16, 1, 2320, conv2d_48_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(48, 1, {(stai_ptr) conv2d_48_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_48 */
  /* LITE_KERNEL_SECTION BEGIN nl_49 */
  {
    
  forward_lite_nl_integer_nl_49(net_ctx);
  }
  /* LITE_KERNEL_SECTION END nl_49 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_51_pad_before */
  {
      const ai_ptr conv2d_51_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 61664);
    ai_ptr conv2d_51_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 87264);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(51, 1, {(stai_ptr) conv2d_51_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_51_pad_before_t_in_0_ptr_const_ptr, conv2d_51_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_51_pad_before_v_pad_constant_value_const_s8), conv2d_51_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_51_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(1280), (ai_i32)(1408), (ai_i32)(1408), (ai_i32)(64), (ai_i32)(64));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(51, 1, {(stai_ptr) conv2d_51_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_51_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_51 */
  {
      const ai_i8* conv2d_51_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 87264);
    const ai_i8* conv2d_51_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 568416);
    const ai_i32* conv2d_51_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 573024);
    ai_i8* conv2d_51_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_outputs[7] + 0);
    ai_i16* conv2d_51_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 800);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(51, 1, {(stai_ptr) conv2d_51_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_deep_3x3_sssa8_ch(conv2d_51_t_in_0_ptr_const_s8, conv2d_51_t_in_0_shape_w_const_u16, conv2d_51_t_in_0_shape_h_const_u16, conv2d_51_t_in_0_shape_ch_const_u16, conv2d_51_t_weight_0_ptr_const_s8, conv2d_51_t_out_0_shape_ch_const_u16, conv2d_51_t_weight_1_ptr_const_s32, conv2d_51_t_in_0_fmt_zero_const_s8, conv2d_51_t_out_0_fmt_zero_const_s8, conv2d_51_t_in_0_fmt_scale_const_f32, conv2d_51_t_out_0_fmt_scale_const_f32, conv2d_51_t_weight_0_fmt_scale_const_f32, conv2d_51_l_out_ch_format_const_layer_format_type, conv2d_51_t_out_0_ptr_s8, conv2d_51_t_out_0_shape_w_const_u16, conv2d_51_t_out_0_shape_h_const_u16, 1, 2368, conv2d_51_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(51, 1, {(stai_ptr) conv2d_51_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_51 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_53_pad_before */
  {
      const ai_ptr conv2d_53_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 61664);
    ai_ptr conv2d_53_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 87264);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(53, 1, {(stai_ptr) conv2d_53_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_53_pad_before_t_in_0_ptr_const_ptr, conv2d_53_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_53_pad_before_v_pad_constant_value_const_s8), conv2d_53_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_53_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(1280), (ai_i32)(1408), (ai_i32)(1408), (ai_i32)(64), (ai_i32)(64));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(53, 1, {(stai_ptr) conv2d_53_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_53_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_53 */
  {
      const ai_i8* conv2d_53_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 87264);
    const ai_i8* conv2d_53_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 573056);
    const ai_i32* conv2d_53_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 584576);
    ai_i8* conv2d_53_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_outputs[1] + 0);
    ai_i16* conv2d_53_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 6368);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(53, 1, {(stai_ptr) conv2d_53_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_deep_3x3_sssa8_ch(conv2d_53_t_in_0_ptr_const_s8, conv2d_53_t_in_0_shape_w_const_u16, conv2d_53_t_in_0_shape_h_const_u16, conv2d_53_t_in_0_shape_ch_const_u16, conv2d_53_t_weight_0_ptr_const_s8, conv2d_53_t_out_0_shape_ch_const_u16, conv2d_53_t_weight_1_ptr_const_s32, conv2d_53_t_in_0_fmt_zero_const_s8, conv2d_53_t_out_0_fmt_zero_const_s8, conv2d_53_t_in_0_fmt_scale_const_f32, conv2d_53_t_out_0_fmt_scale_const_f32, conv2d_53_t_weight_0_fmt_scale_const_f32, conv2d_53_l_out_ch_format_const_layer_format_type, conv2d_53_t_out_0_ptr_s8, conv2d_53_t_out_0_shape_w_const_u16, conv2d_53_t_out_0_shape_h_const_u16, 1, 2464, conv2d_53_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(53, 1, {(stai_ptr) conv2d_53_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_53 */
  /* LITE_KERNEL_SECTION BEGIN pad_55 */
  {
      const ai_ptr pad_55_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 14144);
    ai_ptr pad_55_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 6368);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(55, 1, {(stai_ptr) pad_55_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(pad_55_t_in_0_ptr_const_ptr, pad_55_t_out_0_ptr_ptr, (ai_handle)(pad_55_v_pad_constant_value_const_s8), pad_55_t_in_0_fmt_bitsize_const_s16, pad_55_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(320), (ai_i32)(352), (ai_i32)(352), (ai_i32)(16), (ai_i32)(16));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(55, 1, {(stai_ptr) pad_55_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END pad_55 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_56 */
  {
      const ai_i8* conv2d_56_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 6368);
    const ai_i8* conv2d_56_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 584656);
    const ai_i32* conv2d_56_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 586960);
    ai_i8* conv2d_56_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 1504);
    ai_i16* conv2d_56_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 800);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(56, 1, {(stai_ptr) conv2d_56_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_sssa8_ch(conv2d_56_t_in_0_ptr_const_s8, conv2d_56_t_in_0_shape_w_const_u16, conv2d_56_t_in_0_shape_h_const_u16, conv2d_56_t_in_0_shape_ch_const_u16, conv2d_56_t_weight_0_ptr_const_s8, conv2d_56_t_out_0_shape_ch_const_u16, conv2d_56_t_weight_0_shape_w_const_u16, conv2d_56_t_weight_0_shape_h_const_u16, conv2d_56_l_stride_1_const_u16, conv2d_56_l_stride_0_const_u16, conv2d_56_l_pad_W_0_const_s32, conv2d_56_l_pad_H_0_const_s32, conv2d_56_t_weight_1_ptr_const_s32, conv2d_56_t_in_0_fmt_zero_const_s8, conv2d_56_t_out_0_fmt_zero_const_s8, conv2d_56_t_in_0_fmt_scale_const_f32, conv2d_56_t_out_0_fmt_scale_const_f32, conv2d_56_t_weight_0_fmt_scale_const_f32, conv2d_56_l_out_ch_format_const_layer_format_type, conv2d_56_t_out_0_ptr_s8, conv2d_56_t_out_0_shape_w_const_u16, conv2d_56_t_out_0_shape_h_const_u16, 1, 704, conv2d_56_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(56, 1, {(stai_ptr) conv2d_56_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_56 */
  /* LITE_KERNEL_SECTION BEGIN eltwise_57 */
  {
    
  forward_lite_eltwise_integer_INT8_eltwise_57(net_ctx);
  }
  /* LITE_KERNEL_SECTION END eltwise_57 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_73_pad_before */
  {
      const ai_ptr conv2d_73_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 6368);
    ai_ptr conv2d_73_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 800);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(73, 1, {(stai_ptr) conv2d_73_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_73_pad_before_t_in_0_ptr_const_ptr, conv2d_73_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_73_pad_before_v_pad_constant_value_const_s8), conv2d_73_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_73_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(160), (ai_i32)(192), (ai_i32)(192), (ai_i32)(16), (ai_i32)(16));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(73, 1, {(stai_ptr) conv2d_73_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_73_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_73 */
  {
      const ai_i8* conv2d_73_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 800);
    const ai_i8* conv2d_73_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 587024);
    const ai_i32* conv2d_73_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 589328);
    ai_i8* conv2d_73_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 8672);
    ai_i16* conv2d_73_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 7968);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(73, 1, {(stai_ptr) conv2d_73_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_deep_3x3_sssa8_ch(conv2d_73_t_in_0_ptr_const_s8, conv2d_73_t_in_0_shape_w_const_u16, conv2d_73_t_in_0_shape_h_const_u16, conv2d_73_t_in_0_shape_ch_const_u16, conv2d_73_t_weight_0_ptr_const_s8, conv2d_73_t_out_0_shape_ch_const_u16, conv2d_73_t_weight_1_ptr_const_s32, conv2d_73_t_in_0_fmt_zero_const_s8, conv2d_73_t_out_0_fmt_zero_const_s8, conv2d_73_t_in_0_fmt_scale_const_f32, conv2d_73_t_out_0_fmt_scale_const_f32, conv2d_73_t_weight_0_fmt_scale_const_f32, conv2d_73_l_out_ch_format_const_layer_format_type, conv2d_73_t_out_0_ptr_s8, conv2d_73_t_out_0_shape_w_const_u16, conv2d_73_t_out_0_shape_h_const_u16, 1, 704, conv2d_73_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(73, 1, {(stai_ptr) conv2d_73_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_73 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_74_pad_before */
  {
      const ai_ptr conv2d_74_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 8672);
    ai_ptr conv2d_74_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 800);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(74, 1, {(stai_ptr) conv2d_74_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_74_pad_before_t_in_0_ptr_const_ptr, conv2d_74_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_74_pad_before_v_pad_constant_value_const_s8), conv2d_74_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_74_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(160), (ai_i32)(192), (ai_i32)(192), (ai_i32)(16), (ai_i32)(16));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(74, 1, {(stai_ptr) conv2d_74_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_74_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_74 */
  {
      const ai_i8* conv2d_74_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 800);
    const ai_i8* conv2d_74_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 589392);
    const ai_i32* conv2d_74_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 589536);
    ai_i8* conv2d_74_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 12560);
    ai_i16* conv2d_74_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 7968);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(74, 1, {(stai_ptr) conv2d_74_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_74_t_in_0_ptr_const_s8, conv2d_74_t_in_0_shape_w_const_u16, conv2d_74_t_in_0_shape_h_const_u16, conv2d_74_t_in_0_shape_ch_const_u16, conv2d_74_t_weight_0_ptr_const_s8, conv2d_74_l_stride_1_const_u16, conv2d_74_l_stride_0_const_u16, conv2d_74_t_weight_1_ptr_const_s32, conv2d_74_t_in_0_fmt_zero_const_s8, conv2d_74_t_out_0_fmt_zero_const_s8, conv2d_74_t_in_0_fmt_scale_const_f32, conv2d_74_t_out_0_fmt_scale_const_f32, conv2d_74_t_weight_0_fmt_scale_const_f32, conv2d_74_t_out_0_ptr_s8, conv2d_74_t_out_0_shape_w_const_u16, conv2d_74_t_out_0_shape_h_const_u16, 0, 4592, conv2d_74_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(74, 1, {(stai_ptr) conv2d_74_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_74 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_75 */
  {
      const ai_i8* conv2d_75_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 12560);
    const ai_i8* conv2d_75_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 589600);
    const ai_i32* conv2d_75_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 590624);
    ai_i8* conv2d_75_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 30560);
    ai_i16* conv2d_75_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 800);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(75, 1, {(stai_ptr) conv2d_75_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_75_t_in_0_ptr_const_s8, conv2d_75_t_in_0_shape_w_const_u16, conv2d_75_t_in_0_shape_h_const_u16, conv2d_75_l_stride_1_const_u16, conv2d_75_l_stride_0_const_u16, conv2d_75_t_in_0_shape_ch_const_u16, conv2d_75_t_weight_0_ptr_const_s8, conv2d_75_t_out_0_shape_ch_const_u16, conv2d_75_t_weight_1_ptr_const_s32, conv2d_75_t_in_0_fmt_zero_const_s8, conv2d_75_t_out_0_fmt_zero_const_s8, conv2d_75_t_in_0_fmt_scale_const_f32, conv2d_75_t_out_0_fmt_scale_const_f32, conv2d_75_t_weight_0_fmt_scale_const_f32, conv2d_75_l_out_ch_format_const_layer_format_type, conv2d_75_t_out_0_ptr_s8, 1, 512, conv2d_75_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(75, 1, {(stai_ptr) conv2d_75_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_75 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_76_pad_before */
  {
      const ai_ptr conv2d_76_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 30560);
    ai_ptr conv2d_76_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 7968);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(76, 1, {(stai_ptr) conv2d_76_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_76_pad_before_t_in_0_ptr_const_ptr, conv2d_76_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_76_pad_before_v_pad_constant_value_const_s8), conv2d_76_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_76_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(640), (ai_i32)(768), (ai_i32)(768), (ai_i32)(64), (ai_i32)(64));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(76, 1, {(stai_ptr) conv2d_76_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_76_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_76 */
  {
      const ai_i8* conv2d_76_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 7968);
    const ai_i8* conv2d_76_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 590880);
    const ai_i32* conv2d_76_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 591456);
    ai_i8* conv2d_76_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 35536);
    ai_i16* conv2d_76_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 30560);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(76, 1, {(stai_ptr) conv2d_76_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_76_t_in_0_ptr_const_s8, conv2d_76_t_in_0_shape_w_const_u16, conv2d_76_t_in_0_shape_h_const_u16, conv2d_76_t_in_0_shape_ch_const_u16, conv2d_76_t_weight_0_ptr_const_s8, conv2d_76_l_stride_1_const_u16, conv2d_76_l_stride_0_const_u16, conv2d_76_t_weight_1_ptr_const_s32, conv2d_76_t_in_0_fmt_zero_const_s8, conv2d_76_t_out_0_fmt_zero_const_s8, conv2d_76_t_in_0_fmt_scale_const_f32, conv2d_76_t_out_0_fmt_scale_const_f32, conv2d_76_t_weight_0_fmt_scale_const_f32, conv2d_76_t_out_0_ptr_s8, conv2d_76_t_out_0_shape_w_const_u16, conv2d_76_t_out_0_shape_h_const_u16, 0, 4976, conv2d_76_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(76, 1, {(stai_ptr) conv2d_76_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_76 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_77 */
  {
      const ai_i8* conv2d_77_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 35536);
    const ai_i8* conv2d_77_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 591712);
    const ai_i32* conv2d_77_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 595808);
    ai_i8* conv2d_77_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 7968);
    ai_i16* conv2d_77_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 800);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(77, 1, {(stai_ptr) conv2d_77_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_77_t_in_0_ptr_const_s8, conv2d_77_t_in_0_shape_w_const_u16, conv2d_77_t_in_0_shape_h_const_u16, conv2d_77_l_stride_1_const_u16, conv2d_77_l_stride_0_const_u16, conv2d_77_t_in_0_shape_ch_const_u16, conv2d_77_t_weight_0_ptr_const_s8, conv2d_77_t_out_0_shape_ch_const_u16, conv2d_77_t_weight_1_ptr_const_s32, conv2d_77_t_in_0_fmt_zero_const_s8, conv2d_77_t_out_0_fmt_zero_const_s8, conv2d_77_t_in_0_fmt_scale_const_f32, conv2d_77_t_out_0_fmt_scale_const_f32, conv2d_77_t_weight_0_fmt_scale_const_f32, conv2d_77_l_out_ch_format_const_layer_format_type, conv2d_77_t_out_0_ptr_s8, 1, 512, conv2d_77_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(77, 1, {(stai_ptr) conv2d_77_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_77 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_78_pad_before */
  {
      const ai_ptr conv2d_78_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 7968);
    ai_ptr conv2d_78_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 30560);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(78, 1, {(stai_ptr) conv2d_78_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_78_pad_before_t_in_0_ptr_const_ptr, conv2d_78_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_78_pad_before_v_pad_constant_value_const_s8), conv2d_78_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_78_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(640), (ai_i32)(768), (ai_i32)(768), (ai_i32)(64), (ai_i32)(64));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(78, 1, {(stai_ptr) conv2d_78_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_78_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_78 */
  {
      const ai_i8* conv2d_78_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 30560);
    const ai_i8* conv2d_78_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 596064);
    const ai_i32* conv2d_78_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 597216);
    ai_i8* conv2d_78_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 14368);
    ai_i16* conv2d_78_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 800);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(78, 1, {(stai_ptr) conv2d_78_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_deep_3x3_sssa8_ch(conv2d_78_t_in_0_ptr_const_s8, conv2d_78_t_in_0_shape_w_const_u16, conv2d_78_t_in_0_shape_h_const_u16, conv2d_78_t_in_0_shape_ch_const_u16, conv2d_78_t_weight_0_ptr_const_s8, conv2d_78_t_out_0_shape_ch_const_u16, conv2d_78_t_weight_1_ptr_const_s32, conv2d_78_t_in_0_fmt_zero_const_s8, conv2d_78_t_out_0_fmt_zero_const_s8, conv2d_78_t_in_0_fmt_scale_const_f32, conv2d_78_t_out_0_fmt_scale_const_f32, conv2d_78_t_weight_0_fmt_scale_const_f32, conv2d_78_l_out_ch_format_const_layer_format_type, conv2d_78_t_out_0_ptr_s8, conv2d_78_t_out_0_shape_w_const_u16, conv2d_78_t_out_0_shape_h_const_u16, 1, 2320, conv2d_78_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(78, 1, {(stai_ptr) conv2d_78_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_78 */
  /* LITE_KERNEL_SECTION BEGIN nl_79 */
  {
    
  forward_lite_nl_integer_nl_79(net_ctx);
  }
  /* LITE_KERNEL_SECTION END nl_79 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_81_pad_before */
  {
      const ai_ptr conv2d_81_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 7968);
    ai_ptr conv2d_81_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 30560);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(81, 1, {(stai_ptr) conv2d_81_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_81_pad_before_t_in_0_ptr_const_ptr, conv2d_81_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_81_pad_before_v_pad_constant_value_const_s8), conv2d_81_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_81_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(640), (ai_i32)(768), (ai_i32)(768), (ai_i32)(64), (ai_i32)(64));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(81, 1, {(stai_ptr) conv2d_81_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_81_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_81 */
  {
      const ai_i8* conv2d_81_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 30560);
    const ai_i8* conv2d_81_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 597224);
    const ai_i32* conv2d_81_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 601832);
    ai_i8* conv2d_81_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_outputs[4] + 0);
    ai_i16* conv2d_81_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 14368);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(81, 1, {(stai_ptr) conv2d_81_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_deep_3x3_sssa8_ch(conv2d_81_t_in_0_ptr_const_s8, conv2d_81_t_in_0_shape_w_const_u16, conv2d_81_t_in_0_shape_h_const_u16, conv2d_81_t_in_0_shape_ch_const_u16, conv2d_81_t_weight_0_ptr_const_s8, conv2d_81_t_out_0_shape_ch_const_u16, conv2d_81_t_weight_1_ptr_const_s32, conv2d_81_t_in_0_fmt_zero_const_s8, conv2d_81_t_out_0_fmt_zero_const_s8, conv2d_81_t_in_0_fmt_scale_const_f32, conv2d_81_t_out_0_fmt_scale_const_f32, conv2d_81_t_weight_0_fmt_scale_const_f32, conv2d_81_l_out_ch_format_const_layer_format_type, conv2d_81_t_out_0_ptr_s8, conv2d_81_t_out_0_shape_w_const_u16, conv2d_81_t_out_0_shape_h_const_u16, 1, 2368, conv2d_81_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(81, 1, {(stai_ptr) conv2d_81_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_81 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_83_pad_before */
  {
      const ai_ptr conv2d_83_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 7968);
    ai_ptr conv2d_83_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 30560);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(83, 1, {(stai_ptr) conv2d_83_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_83_pad_before_t_in_0_ptr_const_ptr, conv2d_83_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_83_pad_before_v_pad_constant_value_const_s8), conv2d_83_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_83_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(640), (ai_i32)(768), (ai_i32)(768), (ai_i32)(64), (ai_i32)(64));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(83, 1, {(stai_ptr) conv2d_83_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_83_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_83 */
  {
      const ai_i8* conv2d_83_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 30560);
    const ai_i8* conv2d_83_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 601864);
    const ai_i32* conv2d_83_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 613384);
    ai_i8* conv2d_83_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_outputs[3] + 0);
    ai_i16* conv2d_83_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 7968);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(83, 1, {(stai_ptr) conv2d_83_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_deep_3x3_sssa8_ch(conv2d_83_t_in_0_ptr_const_s8, conv2d_83_t_in_0_shape_w_const_u16, conv2d_83_t_in_0_shape_h_const_u16, conv2d_83_t_in_0_shape_ch_const_u16, conv2d_83_t_weight_0_ptr_const_s8, conv2d_83_t_out_0_shape_ch_const_u16, conv2d_83_t_weight_1_ptr_const_s32, conv2d_83_t_in_0_fmt_zero_const_s8, conv2d_83_t_out_0_fmt_zero_const_s8, conv2d_83_t_in_0_fmt_scale_const_f32, conv2d_83_t_out_0_fmt_scale_const_f32, conv2d_83_t_weight_0_fmt_scale_const_f32, conv2d_83_l_out_ch_format_const_layer_format_type, conv2d_83_t_out_0_ptr_s8, conv2d_83_t_out_0_shape_w_const_u16, conv2d_83_t_out_0_shape_h_const_u16, 1, 2464, conv2d_83_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(83, 1, {(stai_ptr) conv2d_83_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_83 */
  /* LITE_KERNEL_SECTION BEGIN pad_58 */
  {
      const ai_ptr pad_58_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 6368);
    ai_ptr pad_58_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 7968);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(58, 1, {(stai_ptr) pad_58_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(pad_58_t_in_0_ptr_const_ptr, pad_58_t_out_0_ptr_ptr, (ai_handle)(pad_58_v_pad_constant_value_const_s8), pad_58_t_in_0_fmt_bitsize_const_s16, pad_58_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(160), (ai_i32)(192), (ai_i32)(192), (ai_i32)(16), (ai_i32)(16));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(58, 1, {(stai_ptr) pad_58_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END pad_58 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_59 */
  {
      const ai_i8* conv2d_59_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 7968);
    const ai_i8* conv2d_59_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 613464);
    const ai_i32* conv2d_59_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 615768);
    ai_i8* conv2d_59_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 2504);
    ai_i16* conv2d_59_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 1800);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(59, 1, {(stai_ptr) conv2d_59_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_sssa8_ch(conv2d_59_t_in_0_ptr_const_s8, conv2d_59_t_in_0_shape_w_const_u16, conv2d_59_t_in_0_shape_h_const_u16, conv2d_59_t_in_0_shape_ch_const_u16, conv2d_59_t_weight_0_ptr_const_s8, conv2d_59_t_out_0_shape_ch_const_u16, conv2d_59_t_weight_0_shape_w_const_u16, conv2d_59_t_weight_0_shape_h_const_u16, conv2d_59_l_stride_1_const_u16, conv2d_59_l_stride_0_const_u16, conv2d_59_l_pad_W_0_const_s32, conv2d_59_l_pad_H_0_const_s32, conv2d_59_t_weight_1_ptr_const_s32, conv2d_59_t_in_0_fmt_zero_const_s8, conv2d_59_t_out_0_fmt_zero_const_s8, conv2d_59_t_in_0_fmt_scale_const_f32, conv2d_59_t_out_0_fmt_scale_const_f32, conv2d_59_t_weight_0_fmt_scale_const_f32, conv2d_59_l_out_ch_format_const_layer_format_type, conv2d_59_t_out_0_ptr_s8, conv2d_59_t_out_0_shape_w_const_u16, conv2d_59_t_out_0_shape_h_const_u16, 1, 704, conv2d_59_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(59, 1, {(stai_ptr) conv2d_59_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_59 */
  /* LITE_KERNEL_SECTION BEGIN eltwise_60 */
  {
    
  forward_lite_eltwise_integer_INT8_eltwise_60(net_ctx);
  }
  /* LITE_KERNEL_SECTION END eltwise_60 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_61_pad_before */
  {
      const ai_ptr conv2d_61_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 1800);
    ai_ptr conv2d_61_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 2200);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(61, 1, {(stai_ptr) conv2d_61_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_61_pad_before_t_in_0_ptr_const_ptr, conv2d_61_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_61_pad_before_v_pad_constant_value_const_s8), conv2d_61_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_61_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(80), (ai_i32)(112), (ai_i32)(112), (ai_i32)(16), (ai_i32)(16));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(61, 1, {(stai_ptr) conv2d_61_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_61_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_61 */
  {
      const ai_i8* conv2d_61_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 2200);
    const ai_i8* conv2d_61_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 615832);
    const ai_i32* conv2d_61_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 618136);
    ai_i8* conv2d_61_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 1800);
    ai_i16* conv2d_61_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 6368);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(61, 1, {(stai_ptr) conv2d_61_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_deep_3x3_sssa8_ch(conv2d_61_t_in_0_ptr_const_s8, conv2d_61_t_in_0_shape_w_const_u16, conv2d_61_t_in_0_shape_h_const_u16, conv2d_61_t_in_0_shape_ch_const_u16, conv2d_61_t_weight_0_ptr_const_s8, conv2d_61_t_out_0_shape_ch_const_u16, conv2d_61_t_weight_1_ptr_const_s32, conv2d_61_t_in_0_fmt_zero_const_s8, conv2d_61_t_out_0_fmt_zero_const_s8, conv2d_61_t_in_0_fmt_scale_const_f32, conv2d_61_t_out_0_fmt_scale_const_f32, conv2d_61_t_weight_0_fmt_scale_const_f32, conv2d_61_l_out_ch_format_const_layer_format_type, conv2d_61_t_out_0_ptr_s8, conv2d_61_t_out_0_shape_w_const_u16, conv2d_61_t_out_0_shape_h_const_u16, 1, 704, conv2d_61_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(61, 1, {(stai_ptr) conv2d_61_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_61 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_62_pad_before */
  {
      const ai_ptr conv2d_62_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 1800);
    ai_ptr conv2d_62_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 2200);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(62, 1, {(stai_ptr) conv2d_62_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_62_pad_before_t_in_0_ptr_const_ptr, conv2d_62_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_62_pad_before_v_pad_constant_value_const_s8), conv2d_62_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_62_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(80), (ai_i32)(112), (ai_i32)(112), (ai_i32)(16), (ai_i32)(16));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(62, 1, {(stai_ptr) conv2d_62_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_62_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_62 */
  {
      const ai_i8* conv2d_62_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 2200);
    const ai_i8* conv2d_62_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 618200);
    const ai_i32* conv2d_62_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 618344);
    ai_i8* conv2d_62_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 1800);
    ai_i16* conv2d_62_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 12432);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(62, 1, {(stai_ptr) conv2d_62_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_62_t_in_0_ptr_const_s8, conv2d_62_t_in_0_shape_w_const_u16, conv2d_62_t_in_0_shape_h_const_u16, conv2d_62_t_in_0_shape_ch_const_u16, conv2d_62_t_weight_0_ptr_const_s8, conv2d_62_l_stride_1_const_u16, conv2d_62_l_stride_0_const_u16, conv2d_62_t_weight_1_ptr_const_s32, conv2d_62_t_in_0_fmt_zero_const_s8, conv2d_62_t_out_0_fmt_zero_const_s8, conv2d_62_t_in_0_fmt_scale_const_f32, conv2d_62_t_out_0_fmt_scale_const_f32, conv2d_62_t_weight_0_fmt_scale_const_f32, conv2d_62_t_out_0_ptr_s8, conv2d_62_t_out_0_shape_w_const_u16, conv2d_62_t_out_0_shape_h_const_u16, 0, 4592, conv2d_62_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(62, 1, {(stai_ptr) conv2d_62_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_62 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_63 */
  {
      const ai_i8* conv2d_63_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 1800);
    const ai_i8* conv2d_63_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 618408);
    const ai_i32* conv2d_63_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 619432);
    ai_i8* conv2d_63_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 6368);
    ai_i16* conv2d_63_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 2200);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(63, 1, {(stai_ptr) conv2d_63_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_63_t_in_0_ptr_const_s8, conv2d_63_t_in_0_shape_w_const_u16, conv2d_63_t_in_0_shape_h_const_u16, conv2d_63_l_stride_1_const_u16, conv2d_63_l_stride_0_const_u16, conv2d_63_t_in_0_shape_ch_const_u16, conv2d_63_t_weight_0_ptr_const_s8, conv2d_63_t_out_0_shape_ch_const_u16, conv2d_63_t_weight_1_ptr_const_s32, conv2d_63_t_in_0_fmt_zero_const_s8, conv2d_63_t_out_0_fmt_zero_const_s8, conv2d_63_t_in_0_fmt_scale_const_f32, conv2d_63_t_out_0_fmt_scale_const_f32, conv2d_63_t_weight_0_fmt_scale_const_f32, conv2d_63_l_out_ch_format_const_layer_format_type, conv2d_63_t_out_0_ptr_s8, 1, 512, conv2d_63_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(63, 1, {(stai_ptr) conv2d_63_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_63 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_64_pad_before */
  {
      const ai_ptr conv2d_64_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 6368);
    ai_ptr conv2d_64_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 12432);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(64, 1, {(stai_ptr) conv2d_64_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_64_pad_before_t_in_0_ptr_const_ptr, conv2d_64_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_64_pad_before_v_pad_constant_value_const_s8), conv2d_64_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_64_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(320), (ai_i32)(448), (ai_i32)(448), (ai_i32)(64), (ai_i32)(64));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(64, 1, {(stai_ptr) conv2d_64_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_64_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_64 */
  {
      const ai_i8* conv2d_64_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 12432);
    const ai_i8* conv2d_64_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 619688);
    const ai_i32* conv2d_64_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 620264);
    ai_i8* conv2d_64_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 6368);
    ai_i16* conv2d_64_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 15568);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(64, 1, {(stai_ptr) conv2d_64_t_in_0_ptr_const_s8});
    
  forward_lite_dw_3x3_sssa8_ch(conv2d_64_t_in_0_ptr_const_s8, conv2d_64_t_in_0_shape_w_const_u16, conv2d_64_t_in_0_shape_h_const_u16, conv2d_64_t_in_0_shape_ch_const_u16, conv2d_64_t_weight_0_ptr_const_s8, conv2d_64_l_stride_1_const_u16, conv2d_64_l_stride_0_const_u16, conv2d_64_t_weight_1_ptr_const_s32, conv2d_64_t_in_0_fmt_zero_const_s8, conv2d_64_t_out_0_fmt_zero_const_s8, conv2d_64_t_in_0_fmt_scale_const_f32, conv2d_64_t_out_0_fmt_scale_const_f32, conv2d_64_t_weight_0_fmt_scale_const_f32, conv2d_64_t_out_0_ptr_s8, conv2d_64_t_out_0_shape_w_const_u16, conv2d_64_t_out_0_shape_h_const_u16, 0, 4976, conv2d_64_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(64, 1, {(stai_ptr) conv2d_64_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_64 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_65 */
  {
      const ai_i8* conv2d_65_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 6368);
    const ai_i8* conv2d_65_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 620520);
    const ai_i32* conv2d_65_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 624616);
    ai_i8* conv2d_65_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 7968);
    ai_i16* conv2d_65_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 1800);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(65, 1, {(stai_ptr) conv2d_65_t_in_0_ptr_const_s8});
    
  forward_lite_pw_sssa8_ch(conv2d_65_t_in_0_ptr_const_s8, conv2d_65_t_in_0_shape_w_const_u16, conv2d_65_t_in_0_shape_h_const_u16, conv2d_65_l_stride_1_const_u16, conv2d_65_l_stride_0_const_u16, conv2d_65_t_in_0_shape_ch_const_u16, conv2d_65_t_weight_0_ptr_const_s8, conv2d_65_t_out_0_shape_ch_const_u16, conv2d_65_t_weight_1_ptr_const_s32, conv2d_65_t_in_0_fmt_zero_const_s8, conv2d_65_t_out_0_fmt_zero_const_s8, conv2d_65_t_in_0_fmt_scale_const_f32, conv2d_65_t_out_0_fmt_scale_const_f32, conv2d_65_t_weight_0_fmt_scale_const_f32, conv2d_65_l_out_ch_format_const_layer_format_type, conv2d_65_t_out_0_ptr_s8, 1, 512, conv2d_65_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(65, 1, {(stai_ptr) conv2d_65_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_65 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_66_pad_before */
  {
      const ai_ptr conv2d_66_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 7968);
    ai_ptr conv2d_66_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 12432);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(66, 1, {(stai_ptr) conv2d_66_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_66_pad_before_t_in_0_ptr_const_ptr, conv2d_66_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_66_pad_before_v_pad_constant_value_const_s8), conv2d_66_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_66_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(320), (ai_i32)(448), (ai_i32)(448), (ai_i32)(64), (ai_i32)(64));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(66, 1, {(stai_ptr) conv2d_66_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_66_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_66 */
  {
      const ai_i8* conv2d_66_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 12432);
    const ai_i8* conv2d_66_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 624872);
    const ai_i32* conv2d_66_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 626024);
    ai_i8* conv2d_66_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_activations[0] + 1800);
    ai_i16* conv2d_66_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 15568);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(66, 1, {(stai_ptr) conv2d_66_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_deep_3x3_sssa8_ch(conv2d_66_t_in_0_ptr_const_s8, conv2d_66_t_in_0_shape_w_const_u16, conv2d_66_t_in_0_shape_h_const_u16, conv2d_66_t_in_0_shape_ch_const_u16, conv2d_66_t_weight_0_ptr_const_s8, conv2d_66_t_out_0_shape_ch_const_u16, conv2d_66_t_weight_1_ptr_const_s32, conv2d_66_t_in_0_fmt_zero_const_s8, conv2d_66_t_out_0_fmt_zero_const_s8, conv2d_66_t_in_0_fmt_scale_const_f32, conv2d_66_t_out_0_fmt_scale_const_f32, conv2d_66_t_weight_0_fmt_scale_const_f32, conv2d_66_l_out_ch_format_const_layer_format_type, conv2d_66_t_out_0_ptr_s8, conv2d_66_t_out_0_shape_w_const_u16, conv2d_66_t_out_0_shape_h_const_u16, 1, 2320, conv2d_66_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(66, 1, {(stai_ptr) conv2d_66_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_66 */
  /* LITE_KERNEL_SECTION BEGIN nl_67 */
  {
    
  forward_lite_nl_integer_nl_67(net_ctx);
  }
  /* LITE_KERNEL_SECTION END nl_67 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_69_pad_before */
  {
      const ai_ptr conv2d_69_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 7968);
    ai_ptr conv2d_69_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 12432);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(69, 1, {(stai_ptr) conv2d_69_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_69_pad_before_t_in_0_ptr_const_ptr, conv2d_69_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_69_pad_before_v_pad_constant_value_const_s8), conv2d_69_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_69_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(320), (ai_i32)(448), (ai_i32)(448), (ai_i32)(64), (ai_i32)(64));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(69, 1, {(stai_ptr) conv2d_69_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_69_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_69 */
  {
      const ai_i8* conv2d_69_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 12432);
    const ai_i8* conv2d_69_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 626032);
    const ai_i32* conv2d_69_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 630640);
    ai_i8* conv2d_69_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_outputs[5] + 0);
    ai_i16* conv2d_69_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 15568);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(69, 1, {(stai_ptr) conv2d_69_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_deep_3x3_sssa8_ch(conv2d_69_t_in_0_ptr_const_s8, conv2d_69_t_in_0_shape_w_const_u16, conv2d_69_t_in_0_shape_h_const_u16, conv2d_69_t_in_0_shape_ch_const_u16, conv2d_69_t_weight_0_ptr_const_s8, conv2d_69_t_out_0_shape_ch_const_u16, conv2d_69_t_weight_1_ptr_const_s32, conv2d_69_t_in_0_fmt_zero_const_s8, conv2d_69_t_out_0_fmt_zero_const_s8, conv2d_69_t_in_0_fmt_scale_const_f32, conv2d_69_t_out_0_fmt_scale_const_f32, conv2d_69_t_weight_0_fmt_scale_const_f32, conv2d_69_l_out_ch_format_const_layer_format_type, conv2d_69_t_out_0_ptr_s8, conv2d_69_t_out_0_shape_w_const_u16, conv2d_69_t_out_0_shape_h_const_u16, 1, 2368, conv2d_69_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(69, 1, {(stai_ptr) conv2d_69_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_69 */
  /* LITE_KERNEL_SECTION BEGIN conv2d_71_pad_before */
  {
      const ai_ptr conv2d_71_pad_before_t_in_0_ptr_const_ptr = (ai_ptr)(net_ctx->_activations[0] + 7968);
    ai_ptr conv2d_71_pad_before_t_out_0_ptr_ptr = (ai_ptr)(net_ctx->_activations[0] + 12432);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(71, 1, {(stai_ptr) conv2d_71_pad_before_t_in_0_ptr_const_ptr});
    
  forward_lite_pad_constant(conv2d_71_pad_before_t_in_0_ptr_const_ptr, conv2d_71_pad_before_t_out_0_ptr_ptr, (ai_handle)(conv2d_71_pad_before_v_pad_constant_value_const_s8), conv2d_71_pad_before_t_in_0_fmt_bitsize_const_s16, conv2d_71_pad_before_t_in_0_shape_h_const_u32, (ai_i32)(1), (ai_i32)(320), (ai_i32)(448), (ai_i32)(448), (ai_i32)(64), (ai_i32)(64));
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(71, 1, {(stai_ptr) conv2d_71_pad_before_t_out_0_ptr_ptr});
  }
  /* LITE_KERNEL_SECTION END conv2d_71_pad_before */
  /* LITE_KERNEL_SECTION BEGIN conv2d_71 */
  {
      const ai_i8* conv2d_71_t_in_0_ptr_const_s8 = (ai_i8*)(net_ctx->_activations[0] + 12432);
    const ai_i8* conv2d_71_t_weight_0_ptr_const_s8 = (ai_i8*)(net_ctx->_weights[0] + 630672);
    const ai_i32* conv2d_71_t_weight_1_ptr_const_s32 = (ai_i32*)(net_ctx->_weights[0] + 642192);
    ai_i8* conv2d_71_t_out_0_ptr_s8 = (ai_i8*)(net_ctx->_outputs[6] + 0);
    ai_i16* conv2d_71_t_scratch_0_ptr_s16 = (ai_i16*)(net_ctx->_activations[0] + 6368);
  
  _STAI_NETWORK_EVENT_NODE_START_CB(71, 1, {(stai_ptr) conv2d_71_t_in_0_ptr_const_s8});
    
  forward_lite_conv2d_deep_3x3_sssa8_ch(conv2d_71_t_in_0_ptr_const_s8, conv2d_71_t_in_0_shape_w_const_u16, conv2d_71_t_in_0_shape_h_const_u16, conv2d_71_t_in_0_shape_ch_const_u16, conv2d_71_t_weight_0_ptr_const_s8, conv2d_71_t_out_0_shape_ch_const_u16, conv2d_71_t_weight_1_ptr_const_s32, conv2d_71_t_in_0_fmt_zero_const_s8, conv2d_71_t_out_0_fmt_zero_const_s8, conv2d_71_t_in_0_fmt_scale_const_f32, conv2d_71_t_out_0_fmt_scale_const_f32, conv2d_71_t_weight_0_fmt_scale_const_f32, conv2d_71_l_out_ch_format_const_layer_format_type, conv2d_71_t_out_0_ptr_s8, conv2d_71_t_out_0_shape_w_const_u16, conv2d_71_t_out_0_shape_h_const_u16, 1, 2464, conv2d_71_t_scratch_0_ptr_s16);
    
  _STAI_NETWORK_EVENT_NODE_STOP_CB(71, 1, {(stai_ptr) conv2d_71_t_out_0_ptr_s8});
  }
  /* LITE_KERNEL_SECTION END conv2d_71 */
  return net_ctx->_return_code;
}

/*****************************************************************************/
/*  Getters APIs Section  */
STAI_API_ENTRY
stai_size stai_network_get_context_size()
{
  return (stai_size)STAI_NETWORK_CONTEXT_SIZE;
}

#if defined(HAVE_NETWORK_INFO)
STAI_API_ENTRY
stai_return_code stai_network_get_info(
  stai_network* network,
  stai_network_info* info)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, info==NULL, STAI_ERROR_NETWORK_INVALID_INFO, net_ctx->_return_code)

  // Copy of network info struct
  *info = g_network_info;

  return STAI_SUCCESS;
}
#endif


STAI_API_ENTRY
stai_return_code stai_network_get_activations(
  stai_network* network, stai_ptr* activations, stai_size* n_activations)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)

  _STAI_SET_ERROR(net_ctx, !n_activations, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  *n_activations = STAI_NETWORK_ACTIVATIONS_NUM;
for (stai_size idx=0; activations && (idx<STAI_NETWORK_ACTIVATIONS_NUM); idx++) {
    // get address of the activations buffers
    activations[idx] = net_ctx->_activations[idx];
  }return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_get_weights(
  stai_network* network, stai_ptr* weights, stai_size* n_weights)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, !n_weights, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  *n_weights = STAI_NETWORK_WEIGHTS_NUM;
for (stai_size idx=0; weights && (idx<STAI_NETWORK_WEIGHTS_NUM); idx++) {
    // get address of the weights buffers
    weights[idx] = net_ctx->_weights[idx];
  }return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_get_inputs(
  stai_network* network, stai_ptr* inputs, stai_size* n_inputs)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, !n_inputs, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  *n_inputs = STAI_NETWORK_IN_NUM;
  for (stai_size idx=0; inputs && (idx<STAI_NETWORK_IN_NUM); idx++) {
    inputs[idx] = net_ctx->_inputs[idx];
  }
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_get_outputs(
  stai_network* network, stai_ptr* outputs, stai_size* n_outputs)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, !n_outputs, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  *n_outputs = STAI_NETWORK_OUT_NUM;
  for (stai_size idx=0; outputs && (idx<STAI_NETWORK_OUT_NUM); idx++) {
    outputs[idx] = net_ctx->_outputs[idx];
  }
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_get_error(
  stai_network* network)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)

  /* return 1st generated error or STAI_SUCCESS if no errors so far */
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_get_states(
  stai_network* network, stai_ptr* states, stai_size* n_states)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, !n_states, STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  /* get the number of internals states (supporting multi-heap also for internal states) */
  *n_states = STAI_NETWORK_STATES_NUM;

  STAI_UNUSED(states)
return net_ctx->_return_code;
}


/*****************************************************************************/
/*  Setters APIs Section  */

STAI_API_ENTRY
stai_return_code stai_network_set_activations(
  stai_network* network,
  const stai_ptr* activations,
  const stai_size n_activations)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
const uintptr_t _activations_alignment[] = STAI_NETWORK_ACTIVATIONS_ALIGNMENTS;
  STAI_PRINT("  [stai_network_set_activations] network(%p) activations[%d]: %p\n\n", net_ctx, n_activations, activations)
  _STAI_SET_ERROR(net_ctx, !activations,
                  STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  _STAI_SET_ERROR(net_ctx, n_activations!=STAI_NETWORK_ACTIVATIONS_NUM,
                  STAI_ERROR_NETWORK_INVALID_ACTIVATIONS_NUM, net_ctx->_return_code)

  for (stai_size idx=0; activations && idx<STAI_NETWORK_ACTIVATIONS_NUM; idx++) {
    STAI_PRINT("  activation[%d]: %p\n", idx, activations[idx])
    _STAI_SET_ERROR(net_ctx, activations[idx]==NULL,
                    STAI_ERROR_NETWORK_INVALID_ACTIVATIONS_PTR, net_ctx->_return_code)
    _STAI_SET_ERROR(net_ctx, ((uintptr_t)activations[idx]) & (_activations_alignment[idx]-1),
                    STAI_ERROR_INVALID_BUFFER_ALIGNMENT, net_ctx->_return_code)
    net_ctx->_activations[idx] = activations[idx];
  }
  net_ctx->_inputs[0] = activations[0] + 40464;

  net_ctx->_outputs[0] = activations[0] + 800;

  net_ctx->_outputs[1] = activations[0] + 20544;

  net_ctx->_outputs[2] = activations[0] + 1852;

  net_ctx->_outputs[3] = activations[0] + 10432;

  net_ctx->_outputs[4] = activations[0] + 1000;

  net_ctx->_outputs[5] = activations[0] + 1904;

  net_ctx->_outputs[6] = activations[0] + 2104;

  net_ctx->_outputs[7] = activations[0] + 3168;

  net_ctx->_outputs[8] = activations[0] + 0;
_stai_network_check(net_ctx);
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_set_weights(
  stai_network* network,
  const stai_ptr* weights,
  const stai_size n_weights)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
const uintptr_t _weights_alignment[] = STAI_NETWORK_WEIGHTS_ALIGNMENTS;
  _STAI_SET_ERROR(net_ctx, !weights,
                  STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  _STAI_SET_ERROR(net_ctx, n_weights!=STAI_NETWORK_WEIGHTS_NUM,
                  STAI_ERROR_NETWORK_INVALID_WEIGHTS_NUM, net_ctx->_return_code)
  for (stai_size idx=0; weights && idx<STAI_NETWORK_WEIGHTS_NUM; idx++) {
    STAI_PRINT("  weight[%d]: %p\n", idx, weights[idx])
    _STAI_SET_ERROR(net_ctx, weights[idx]==NULL,
                    STAI_ERROR_NETWORK_INVALID_WEIGHTS_PTR, net_ctx->_return_code)
    _STAI_SET_ERROR(net_ctx, ((uintptr_t)weights[idx]) & (_weights_alignment[idx]-1),
                    STAI_ERROR_INVALID_BUFFER_ALIGNMENT, net_ctx->_return_code)
    net_ctx->_weights[idx] = weights[idx];
  }_stai_network_check(net_ctx);
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_set_inputs(
  stai_network* network,
  const stai_ptr* inputs,
  const stai_size n_inputs)
{
  const uintptr_t _inputs_alignment[] = STAI_NETWORK_IN_ALIGNMENTS;
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, !inputs,
                  STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  _STAI_SET_ERROR(net_ctx, n_inputs!=STAI_NETWORK_IN_NUM,
                  STAI_ERROR_NETWORK_INVALID_IN_NUM, net_ctx->_return_code)

  for (stai_size idx=0; inputs && idx<STAI_NETWORK_IN_NUM; idx++) {
    STAI_PRINT("  input[%d]: %p\n", idx, inputs[idx])
    _STAI_SET_ERROR(net_ctx, inputs[idx]==NULL,
                    STAI_ERROR_NETWORK_INVALID_IN_PTR, net_ctx->_return_code)
    _STAI_SET_ERROR(net_ctx, ((uintptr_t)inputs[idx]) & (_inputs_alignment[idx]-1),
                    STAI_ERROR_INVALID_BUFFER_ALIGNMENT, net_ctx->_return_code)
    net_ctx->_inputs[idx] = inputs[idx];
  }

  _stai_network_check(net_ctx);
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_set_outputs(
  stai_network* network,
  const stai_ptr* outputs,
  const stai_size n_outputs)
{
  const uintptr_t _outputs_alignment[] = STAI_NETWORK_OUT_ALIGNMENTS;
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  _STAI_SET_ERROR(net_ctx, !outputs,
                  STAI_ERROR_NETWORK_INVALID_API_ARGUMENTS, net_ctx->_return_code)
  _STAI_SET_ERROR(net_ctx, n_outputs!=STAI_NETWORK_OUT_NUM,
                  STAI_ERROR_NETWORK_INVALID_OUT_NUM, net_ctx->_return_code)

  for (stai_size idx=0; outputs && idx<n_outputs; idx++) {
    STAI_PRINT("  output[%d]: %p\n", idx, outputs[idx])
    _STAI_SET_ERROR(net_ctx, outputs[idx]==NULL,
                    STAI_ERROR_NETWORK_INVALID_OUT_PTR, net_ctx->_return_code)
    _STAI_SET_ERROR(net_ctx, ((uintptr_t)outputs[idx]) & (_outputs_alignment[idx]-1),
                    STAI_ERROR_INVALID_BUFFER_ALIGNMENT, net_ctx->_return_code)
    net_ctx->_outputs[idx] = outputs[idx];
  }

  _stai_network_check(net_ctx);
  return net_ctx->_return_code;
}


STAI_API_ENTRY
stai_return_code stai_network_set_states(
  stai_network* network,
  const stai_ptr* states,
  const stai_size n_states)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)

  STAI_UNUSED(states)
  STAI_UNUSED(n_states)
_stai_network_check(net_ctx);
  return net_ctx->_return_code;
}

STAI_API_ENTRY
stai_return_code stai_network_set_callback(
  stai_network* network, const stai_event_cb cb, void* cb_cookie)
{
  _STAI_CONTEXT_ACQUIRE(net_ctx, network)
  STAI_PRINT("  set_callback %p cb %p cookie %p\n", net_ctx, cb, cb_cookie)
  // _STAI_SET_ERROR(net_ctx, cb==NULL, STAI_ERROR_NETWORK_INVALID_CALLBACK, net_ctx->_return_code)
  net_ctx->_callback = cb;
  net_ctx->_callback_cookie = cb_cookie;
  return net_ctx->_return_code;
}

#undef _STAI_SET_ERROR
#undef _STAI_CONTEXT_ALIGNMENT
#undef _STAI_CONTEXT_ACQUIRE
#undef _STAI_NETWORK_EVENT_NODE_START_CB
#undef _STAI_NETWORK_EVENT_NODE_STOP_CB
#undef _STAI_NETWORK_MODEL_SIGNATURE
#undef _STAI_NETWORK_DATETIME
#undef _STAI_NETWORK_COMPILE_DATETIME

