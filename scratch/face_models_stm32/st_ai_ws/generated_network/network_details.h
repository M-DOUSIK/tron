/**
  ******************************************************************************
  * @file    network.h
  * @date    2026-08-31T13:18:09+0530
  * @brief   ST.AI Tool Automatic Code Generator for Embedded NN computing
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
#ifndef STAI_NETWORK_DETAILS_H
#define STAI_NETWORK_DETAILS_H

#include "stai.h"
#include "layers.h"

const stai_network_details g_network_details = {
  .tensors = (const stai_tensor[106]) {
   { .size_bytes = 76801, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 160, 160, 3}}, .scale = {1, (const float[1]){0.003921568859368563}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "serving_default_input_10_output" },
   { .size_bytes = 102400, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 80, 80, 16}}, .scale = {1, (const float[1]){0.004335746634751558}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_1_output" },
   { .size_bytes = 107584, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 82, 82, 16}}, .scale = {1, (const float[1]){0.004335746634751558}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_2_pad_before_output" },
   { .size_bytes = 102400, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 80, 80, 16}}, .scale = {1, (const float[1]){0.013141044415533543}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_2_output" },
   { .size_bytes = 102400, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 80, 80, 16}}, .scale = {1, (const float[1]){0.00846110749989748}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_3_output" },
   { .size_bytes = 107584, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 82, 82, 16}}, .scale = {1, (const float[1]){0.00846110749989748}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_5_pad_before_output" },
   { .size_bytes = 25600, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 40, 40, 16}}, .scale = {1, (const float[1]){0.005124824121594429}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_5_output" },
   { .size_bytes = 64000, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 40, 40, 40}}, .scale = {1, (const float[1]){0.005847045686095953}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_6_output" },
   { .size_bytes = 70560, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 42, 42, 40}}, .scale = {1, (const float[1]){0.005847045686095953}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_7_pad_before_output" },
   { .size_bytes = 64000, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 40, 40, 40}}, .scale = {1, (const float[1]){0.005930582992732525}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_7_output" },
   { .size_bytes = 64000, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 40, 40, 40}}, .scale = {1, (const float[1]){0.0034911902621388435}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_8_output" },
   { .size_bytes = 70560, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 42, 42, 40}}, .scale = {1, (const float[1]){0.0034911902621388435}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_10_pad_before_output" },
   { .size_bytes = 16000, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 20, 20, 40}}, .scale = {1, (const float[1]){0.004611345939338207}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_10_output" },
   { .size_bytes = 28800, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 20, 20, 72}}, .scale = {1, (const float[1]){0.0022572644520550966}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_11_output" },
   { .size_bytes = 34848, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 22, 22, 72}}, .scale = {1, (const float[1]){0.0022572644520550966}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_12_pad_before_output" },
   { .size_bytes = 28800, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 20, 20, 72}}, .scale = {1, (const float[1]){0.00447167968377471}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_12_output" },
   { .size_bytes = 28800, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 20, 20, 72}}, .scale = {1, (const float[1]){0.003036969341337681}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_13_output" },
   { .size_bytes = 34848, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 22, 22, 72}}, .scale = {1, (const float[1]){0.003036969341337681}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_14_pad_before_output" },
   { .size_bytes = 28800, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 20, 20, 72}}, .scale = {1, (const float[1]){0.0049627092666924}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_14_output" },
   { .size_bytes = 28800, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 20, 20, 72}}, .scale = {1, (const float[1]){0.0046248785220086575}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_15_output" },
   { .size_bytes = 34848, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 22, 22, 72}}, .scale = {1, (const float[1]){0.0046248785220086575}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_17_pad_before_output" },
   { .size_bytes = 7200, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 10, 10, 72}}, .scale = {1, (const float[1]){0.0088109839707613}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_17_output" },
   { .size_bytes = 15200, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 10, 10, 152}}, .scale = {1, (const float[1]){0.003233096329495311}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_18_output" },
   { .size_bytes = 21888, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 12, 12, 152}}, .scale = {1, (const float[1]){0.003233096329495311}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_19_pad_before_output" },
   { .size_bytes = 15200, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 10, 10, 152}}, .scale = {1, (const float[1]){0.007384846452623606}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_19_output" },
   { .size_bytes = 15200, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 10, 10, 152}}, .scale = {1, (const float[1]){0.0035589300096035004}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_20_output" },
   { .size_bytes = 21888, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 12, 12, 152}}, .scale = {1, (const float[1]){0.0035589300096035004}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_22_pad_before_output" },
   { .size_bytes = 3800, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 5, 5, 152}}, .scale = {1, (const float[1]){0.003517482429742813}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_22_output" },
   { .size_bytes = 7200, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 5, 5, 288}}, .scale = {1, (const float[1]){0.005404532887041569}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_23_output" },
   { .size_bytes = 14112, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 288}}, .scale = {1, (const float[1]){0.005404532887041569}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_24_pad_before_output" },
   { .size_bytes = 7200, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 5, 5, 288}}, .scale = {1, (const float[1]){0.003411282319575548}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_24_output" },
   { .size_bytes = 7200, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 5, 5, 288}}, .scale = {1, (const float[1]){0.0020968448370695114}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_25_output" },
   { .size_bytes = 14112, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 288}}, .scale = {1, (const float[1]){0.0020968448370695114}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_26_pad_before_output" },
   { .size_bytes = 7200, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 5, 5, 288}}, .scale = {1, (const float[1]){0.002392140682786703}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_26_output" },
   { .size_bytes = 7200, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 5, 5, 288}}, .scale = {1, (const float[1]){0.002094843192026019}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_27_output" },
   { .size_bytes = 14112, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 288}}, .scale = {1, (const float[1]){0.002094843192026019}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_28_pad_before_output" },
   { .size_bytes = 7200, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 5, 5, 288}}, .scale = {1, (const float[1]){0.0020765908993780613}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_28_output" },
   { .size_bytes = 7200, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 5, 5, 288}}, .scale = {1, (const float[1]){0.0015556998550891876}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_29_output" },
   { .size_bytes = 14112, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 288}}, .scale = {1, (const float[1]){0.0015556998550891876}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_30_pad_before_output" },
   { .size_bytes = 7200, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 5, 5, 288}}, .scale = {1, (const float[1]){0.001783824060112238}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_30_output" },
   { .size_bytes = 7200, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 5, 5, 288}}, .scale = {1, (const float[1]){0.0013555892510339618}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_31_output" },
   { .size_bytes = 14112, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 288}}, .scale = {1, (const float[1]){0.0013555892510339618}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_32_pad_before_output" },
   { .size_bytes = 7200, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 5, 5, 288}}, .scale = {1, (const float[1]){0.001734352670609951}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_32_output" },
   { .size_bytes = 7200, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 5, 5, 288}}, .scale = {1, (const float[1]){0.0012167595559731126}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_33_output" },
   { .size_bytes = 400, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 5, 5, 16}}, .scale = {1, (const float[1]){0.0007190321339294314}}, .zeropoint = {1, (const int16_t[1]){20}}, .name = "conv2d_34_output" },
   { .size_bytes = 784, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 16}}, .scale = {1, (const float[1]){0.0007190321339294314}}, .zeropoint = {1, (const int16_t[1]){20}}, .name = "conv2d_36_pad_before_output" },
   { .size_bytes = 400, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 5, 5, 16}}, .scale = {1, (const float[1]){0.00038219871930778027}}, .zeropoint = {1, (const int16_t[1]){18}}, .name = "conv2d_36_output" },
   { .size_bytes = 1600, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 10, 10, 16}}, .scale = {1, (const float[1]){0.0007190321339294314}}, .zeropoint = {1, (const int16_t[1]){20}}, .name = "resize_35_output" },
   { .size_bytes = 1600, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 10, 10, 16}}, .scale = {1, (const float[1]){0.0016279983101412654}}, .zeropoint = {1, (const int16_t[1]){5}}, .name = "conv2d_37_output" },
   { .size_bytes = 1600, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 10, 10, 16}}, .scale = {1, (const float[1]){0.0016279983101412654}}, .zeropoint = {1, (const int16_t[1]){5}}, .name = "eltwise_38_output" },
   { .size_bytes = 2304, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 12, 12, 16}}, .scale = {1, (const float[1]){0.0016279983101412654}}, .zeropoint = {1, (const int16_t[1]){5}}, .name = "conv2d_40_pad_before_output" },
   { .size_bytes = 1600, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 10, 10, 16}}, .scale = {1, (const float[1]){0.0009802478598430753}}, .zeropoint = {1, (const int16_t[1]){1}}, .name = "conv2d_40_output" },
   { .size_bytes = 6400, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 20, 20, 16}}, .scale = {1, (const float[1]){0.0016279983101412654}}, .zeropoint = {1, (const int16_t[1]){5}}, .name = "resize_39_output" },
   { .size_bytes = 6400, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 20, 20, 16}}, .scale = {1, (const float[1]){0.0027904396411031485}}, .zeropoint = {1, (const int16_t[1]){-19}}, .name = "conv2d_41_output" },
   { .size_bytes = 6400, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 20, 20, 16}}, .scale = {1, (const float[1]){0.0027904396411031485}}, .zeropoint = {1, (const int16_t[1]){-19}}, .name = "eltwise_42_output" },
   { .size_bytes = 7744, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 22, 22, 16}}, .scale = {1, (const float[1]){0.0027904396411031485}}, .zeropoint = {1, (const int16_t[1]){-19}}, .name = "conv2d_43_pad_before_output" },
   { .size_bytes = 6400, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 20, 20, 16}}, .scale = {1, (const float[1]){0.001759668462909758}}, .zeropoint = {1, (const int16_t[1]){-6}}, .name = "conv2d_43_output" },
   { .size_bytes = 7744, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 22, 22, 16}}, .scale = {1, (const float[1]){0.001759668462909758}}, .zeropoint = {1, (const int16_t[1]){-6}}, .name = "conv2d_44_pad_before_output" },
   { .size_bytes = 6400, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 20, 20, 16}}, .scale = {1, (const float[1]){0.006812004838138819}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_44_output" },
   { .size_bytes = 25600, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 20, 20, 64}}, .scale = {1, (const float[1]){0.004793986212462187}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_45_output" },
   { .size_bytes = 30976, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 22, 22, 64}}, .scale = {1, (const float[1]){0.004793986212462187}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_46_pad_before_output" },
   { .size_bytes = 25600, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 20, 20, 64}}, .scale = {1, (const float[1]){0.0055921501480042934}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_46_output" },
   { .size_bytes = 25600, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 20, 20, 64}}, .scale = {1, (const float[1]){0.011582382023334503}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_47_output" },
   { .size_bytes = 30976, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 22, 22, 64}}, .scale = {1, (const float[1]){0.011582382023334503}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_48_pad_before_output" },
   { .size_bytes = 800, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 20, 20, 2}}, .scale = {1, (const float[1]){0.0390465185046196}}, .zeropoint = {1, (const int16_t[1]){92}}, .name = "conv2d_48_output" },
   { .size_bytes = 800, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 20, 20, 2}}, .scale = {1, (const float[1]){0.00390625}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "nl_49_output" },
   { .size_bytes = 30976, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 22, 22, 64}}, .scale = {1, (const float[1]){0.011582382023334503}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_51_pad_before_output" },
   { .size_bytes = 3200, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 20, 20, 8}}, .scale = {1, (const float[1]){0.019490458071231842}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_51_output" },
   { .size_bytes = 30976, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 22, 22, 64}}, .scale = {1, (const float[1]){0.011582382023334503}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_53_pad_before_output" },
   { .size_bytes = 8000, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 20, 20, 20}}, .scale = {1, (const float[1]){0.02451060339808464}}, .zeropoint = {1, (const int16_t[1]){-5}}, .name = "conv2d_53_output" },
   { .size_bytes = 7744, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 22, 22, 16}}, .scale = {1, (const float[1]){0.001759668462909758}}, .zeropoint = {1, (const int16_t[1]){-6}}, .name = "pad_55_output" },
   { .size_bytes = 1600, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 10, 10, 16}}, .scale = {1, (const float[1]){0.0006985461805015802}}, .zeropoint = {1, (const int16_t[1]){-17}}, .name = "conv2d_56_output" },
   { .size_bytes = 1600, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 10, 10, 16}}, .scale = {1, (const float[1]){0.0009802478598430753}}, .zeropoint = {1, (const int16_t[1]){1}}, .name = "eltwise_57_output" },
   { .size_bytes = 2304, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 12, 12, 16}}, .scale = {1, (const float[1]){0.0009802478598430753}}, .zeropoint = {1, (const int16_t[1]){1}}, .name = "conv2d_73_pad_before_output" },
   { .size_bytes = 1600, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 10, 10, 16}}, .scale = {1, (const float[1]){0.0006639527273364365}}, .zeropoint = {1, (const int16_t[1]){16}}, .name = "conv2d_73_output" },
   { .size_bytes = 2304, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 12, 12, 16}}, .scale = {1, (const float[1]){0.0006639527273364365}}, .zeropoint = {1, (const int16_t[1]){16}}, .name = "conv2d_74_pad_before_output" },
   { .size_bytes = 1600, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 10, 10, 16}}, .scale = {1, (const float[1]){0.002446890575811267}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_74_output" },
   { .size_bytes = 6400, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 10, 10, 64}}, .scale = {1, (const float[1]){0.0035019763745367527}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_75_output" },
   { .size_bytes = 9216, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 12, 12, 64}}, .scale = {1, (const float[1]){0.0035019763745367527}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_76_pad_before_output" },
   { .size_bytes = 6400, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 10, 10, 64}}, .scale = {1, (const float[1]){0.004130890127271414}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_76_output" },
   { .size_bytes = 6400, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 10, 10, 64}}, .scale = {1, (const float[1]){0.010677007026970387}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_77_output" },
   { .size_bytes = 9216, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 12, 12, 64}}, .scale = {1, (const float[1]){0.010677007026970387}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_78_pad_before_output" },
   { .size_bytes = 200, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 10, 10, 2}}, .scale = {1, (const float[1]){0.036569274961948395}}, .zeropoint = {1, (const int16_t[1]){61}}, .name = "conv2d_78_output" },
   { .size_bytes = 200, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 10, 10, 2}}, .scale = {1, (const float[1]){0.00390625}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "nl_79_output" },
   { .size_bytes = 9216, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 12, 12, 64}}, .scale = {1, (const float[1]){0.010677007026970387}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_81_pad_before_output" },
   { .size_bytes = 800, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 10, 10, 8}}, .scale = {1, (const float[1]){0.01881924830377102}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_81_output" },
   { .size_bytes = 9216, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 12, 12, 64}}, .scale = {1, (const float[1]){0.010677007026970387}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_83_pad_before_output" },
   { .size_bytes = 2000, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 10, 10, 20}}, .scale = {1, (const float[1]){0.023661937564611435}}, .zeropoint = {1, (const int16_t[1]){-18}}, .name = "conv2d_83_output" },
   { .size_bytes = 2304, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 12, 12, 16}}, .scale = {1, (const float[1]){0.0009802478598430753}}, .zeropoint = {1, (const int16_t[1]){1}}, .name = "pad_58_output" },
   { .size_bytes = 400, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 5, 5, 16}}, .scale = {1, (const float[1]){0.0002102396683767438}}, .zeropoint = {1, (const int16_t[1]){25}}, .name = "conv2d_59_output" },
   { .size_bytes = 400, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 5, 5, 16}}, .scale = {1, (const float[1]){0.00038219871930778027}}, .zeropoint = {1, (const int16_t[1]){18}}, .name = "eltwise_60_output" },
   { .size_bytes = 784, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 16}}, .scale = {1, (const float[1]){0.00038219871930778027}}, .zeropoint = {1, (const int16_t[1]){18}}, .name = "conv2d_61_pad_before_output" },
   { .size_bytes = 400, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 5, 5, 16}}, .scale = {1, (const float[1]){0.00044369601528160274}}, .zeropoint = {1, (const int16_t[1]){0}}, .name = "conv2d_61_output" },
   { .size_bytes = 784, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 16}}, .scale = {1, (const float[1]){0.00044369601528160274}}, .zeropoint = {1, (const int16_t[1]){0}}, .name = "conv2d_62_pad_before_output" },
   { .size_bytes = 400, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 5, 5, 16}}, .scale = {1, (const float[1]){0.0011108251055702567}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_62_output" },
   { .size_bytes = 1600, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 5, 5, 64}}, .scale = {1, (const float[1]){0.0011557439574971795}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_63_output" },
   { .size_bytes = 3136, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 64}}, .scale = {1, (const float[1]){0.0011557439574971795}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_64_pad_before_output" },
   { .size_bytes = 1600, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 5, 5, 64}}, .scale = {1, (const float[1]){0.0012280665105208755}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_64_output" },
   { .size_bytes = 1600, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 5, 5, 64}}, .scale = {1, (const float[1]){0.005131934769451618}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_65_output" },
   { .size_bytes = 3136, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 64}}, .scale = {1, (const float[1]){0.005131934769451618}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_66_pad_before_output" },
   { .size_bytes = 50, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 5, 5, 2}}, .scale = {1, (const float[1]){0.02307526022195816}}, .zeropoint = {1, (const int16_t[1]){127}}, .name = "conv2d_66_output" },
   { .size_bytes = 50, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 5, 5, 2}}, .scale = {1, (const float[1]){0.00390625}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "nl_67_output" },
   { .size_bytes = 3136, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 64}}, .scale = {1, (const float[1]){0.005131934769451618}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_69_pad_before_output" },
   { .size_bytes = 200, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 5, 5, 8}}, .scale = {1, (const float[1]){0.015103490091860294}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_69_output" },
   { .size_bytes = 3136, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 7, 7, 64}}, .scale = {1, (const float[1]){0.005131934769451618}}, .zeropoint = {1, (const int16_t[1]){-128}}, .name = "conv2d_71_pad_before_output" },
   { .size_bytes = 500, .flags = (STAI_FLAG_HAS_BATCH|STAI_FLAG_CHANNEL_LAST), .format = STAI_FORMAT_S8, .shape = {4, (const int32_t[4]){1, 5, 5, 20}}, .scale = {1, (const float[1]){0.01342423353344202}}, .zeropoint = {1, (const int16_t[1]){-20}}, .name = "conv2d_71_output" }
  },
  .nodes = (const stai_node_details[105]){
    {.id = 0, .type = AI_LAYER_OPTIMIZED_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){0}}, .output_tensors = {1, (const int32_t[1]){1}} }, /* conv2d_1 */
    {.id = 2, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){1}}, .output_tensors = {1, (const int32_t[1]){2}} }, /* conv2d_2_pad_before */
    {.id = 2, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){2}}, .output_tensors = {1, (const int32_t[1]){3}} }, /* conv2d_2 */
    {.id = 3, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){3}}, .output_tensors = {1, (const int32_t[1]){4}} }, /* conv2d_3 */
    {.id = 4, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){4}}, .output_tensors = {1, (const int32_t[1]){5}} }, /* conv2d_5_pad_before */
    {.id = 5, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){5}}, .output_tensors = {1, (const int32_t[1]){6}} }, /* conv2d_5 */
    {.id = 6, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){6}}, .output_tensors = {1, (const int32_t[1]){7}} }, /* conv2d_6 */
    {.id = 7, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){7}}, .output_tensors = {1, (const int32_t[1]){8}} }, /* conv2d_7_pad_before */
    {.id = 7, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){8}}, .output_tensors = {1, (const int32_t[1]){9}} }, /* conv2d_7 */
    {.id = 8, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){9}}, .output_tensors = {1, (const int32_t[1]){10}} }, /* conv2d_8 */
    {.id = 9, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){10}}, .output_tensors = {1, (const int32_t[1]){11}} }, /* conv2d_10_pad_before */
    {.id = 10, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){11}}, .output_tensors = {1, (const int32_t[1]){12}} }, /* conv2d_10 */
    {.id = 11, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){12}}, .output_tensors = {1, (const int32_t[1]){13}} }, /* conv2d_11 */
    {.id = 12, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){13}}, .output_tensors = {1, (const int32_t[1]){14}} }, /* conv2d_12_pad_before */
    {.id = 12, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){14}}, .output_tensors = {1, (const int32_t[1]){15}} }, /* conv2d_12 */
    {.id = 13, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){15}}, .output_tensors = {1, (const int32_t[1]){16}} }, /* conv2d_13 */
    {.id = 14, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){16}}, .output_tensors = {1, (const int32_t[1]){17}} }, /* conv2d_14_pad_before */
    {.id = 14, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){17}}, .output_tensors = {1, (const int32_t[1]){18}} }, /* conv2d_14 */
    {.id = 15, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){18}}, .output_tensors = {1, (const int32_t[1]){19}} }, /* conv2d_15 */
    {.id = 16, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){19}}, .output_tensors = {1, (const int32_t[1]){20}} }, /* conv2d_17_pad_before */
    {.id = 17, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){20}}, .output_tensors = {1, (const int32_t[1]){21}} }, /* conv2d_17 */
    {.id = 18, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){21}}, .output_tensors = {1, (const int32_t[1]){22}} }, /* conv2d_18 */
    {.id = 19, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){22}}, .output_tensors = {1, (const int32_t[1]){23}} }, /* conv2d_19_pad_before */
    {.id = 19, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){23}}, .output_tensors = {1, (const int32_t[1]){24}} }, /* conv2d_19 */
    {.id = 20, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){24}}, .output_tensors = {1, (const int32_t[1]){25}} }, /* conv2d_20 */
    {.id = 21, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){25}}, .output_tensors = {1, (const int32_t[1]){26}} }, /* conv2d_22_pad_before */
    {.id = 22, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){26}}, .output_tensors = {1, (const int32_t[1]){27}} }, /* conv2d_22 */
    {.id = 23, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){27}}, .output_tensors = {1, (const int32_t[1]){28}} }, /* conv2d_23 */
    {.id = 24, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){28}}, .output_tensors = {1, (const int32_t[1]){29}} }, /* conv2d_24_pad_before */
    {.id = 24, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){29}}, .output_tensors = {1, (const int32_t[1]){30}} }, /* conv2d_24 */
    {.id = 25, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){30}}, .output_tensors = {1, (const int32_t[1]){31}} }, /* conv2d_25 */
    {.id = 26, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){31}}, .output_tensors = {1, (const int32_t[1]){32}} }, /* conv2d_26_pad_before */
    {.id = 26, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){32}}, .output_tensors = {1, (const int32_t[1]){33}} }, /* conv2d_26 */
    {.id = 27, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){33}}, .output_tensors = {1, (const int32_t[1]){34}} }, /* conv2d_27 */
    {.id = 28, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){34}}, .output_tensors = {1, (const int32_t[1]){35}} }, /* conv2d_28_pad_before */
    {.id = 28, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){35}}, .output_tensors = {1, (const int32_t[1]){36}} }, /* conv2d_28 */
    {.id = 29, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){36}}, .output_tensors = {1, (const int32_t[1]){37}} }, /* conv2d_29 */
    {.id = 30, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){37}}, .output_tensors = {1, (const int32_t[1]){38}} }, /* conv2d_30_pad_before */
    {.id = 30, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){38}}, .output_tensors = {1, (const int32_t[1]){39}} }, /* conv2d_30 */
    {.id = 31, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){39}}, .output_tensors = {1, (const int32_t[1]){40}} }, /* conv2d_31 */
    {.id = 32, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){40}}, .output_tensors = {1, (const int32_t[1]){41}} }, /* conv2d_32_pad_before */
    {.id = 32, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){41}}, .output_tensors = {1, (const int32_t[1]){42}} }, /* conv2d_32 */
    {.id = 33, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){42}}, .output_tensors = {1, (const int32_t[1]){43}} }, /* conv2d_33 */
    {.id = 34, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){43}}, .output_tensors = {1, (const int32_t[1]){44}} }, /* conv2d_34 */
    {.id = 36, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){44}}, .output_tensors = {1, (const int32_t[1]){45}} }, /* conv2d_36_pad_before */
    {.id = 36, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){45}}, .output_tensors = {1, (const int32_t[1]){46}} }, /* conv2d_36 */
    {.id = 35, .type = AI_LAYER_UPSAMPLE_TYPE, .input_tensors = {1, (const int32_t[1]){44}}, .output_tensors = {1, (const int32_t[1]){47}} }, /* resize_35 */
    {.id = 37, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){25}}, .output_tensors = {1, (const int32_t[1]){48}} }, /* conv2d_37 */
    {.id = 38, .type = AI_LAYER_ELTWISE_INTEGER_TYPE, .input_tensors = {2, (const int32_t[2]){48, 47}}, .output_tensors = {1, (const int32_t[1]){49}} }, /* eltwise_38 */
    {.id = 40, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){49}}, .output_tensors = {1, (const int32_t[1]){50}} }, /* conv2d_40_pad_before */
    {.id = 40, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){50}}, .output_tensors = {1, (const int32_t[1]){51}} }, /* conv2d_40 */
    {.id = 39, .type = AI_LAYER_UPSAMPLE_TYPE, .input_tensors = {1, (const int32_t[1]){49}}, .output_tensors = {1, (const int32_t[1]){52}} }, /* resize_39 */
    {.id = 41, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){19}}, .output_tensors = {1, (const int32_t[1]){53}} }, /* conv2d_41 */
    {.id = 42, .type = AI_LAYER_ELTWISE_INTEGER_TYPE, .input_tensors = {2, (const int32_t[2]){53, 52}}, .output_tensors = {1, (const int32_t[1]){54}} }, /* eltwise_42 */
    {.id = 43, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){54}}, .output_tensors = {1, (const int32_t[1]){55}} }, /* conv2d_43_pad_before */
    {.id = 43, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){55}}, .output_tensors = {1, (const int32_t[1]){56}} }, /* conv2d_43 */
    {.id = 44, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){56}}, .output_tensors = {1, (const int32_t[1]){57}} }, /* conv2d_44_pad_before */
    {.id = 44, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){57}}, .output_tensors = {1, (const int32_t[1]){58}} }, /* conv2d_44 */
    {.id = 45, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){58}}, .output_tensors = {1, (const int32_t[1]){59}} }, /* conv2d_45 */
    {.id = 46, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){59}}, .output_tensors = {1, (const int32_t[1]){60}} }, /* conv2d_46_pad_before */
    {.id = 46, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){60}}, .output_tensors = {1, (const int32_t[1]){61}} }, /* conv2d_46 */
    {.id = 47, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){61}}, .output_tensors = {1, (const int32_t[1]){62}} }, /* conv2d_47 */
    {.id = 48, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){62}}, .output_tensors = {1, (const int32_t[1]){63}} }, /* conv2d_48_pad_before */
    {.id = 48, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){63}}, .output_tensors = {1, (const int32_t[1]){64}} }, /* conv2d_48 */
    {.id = 49, .type = AI_LAYER_NL_TYPE, .input_tensors = {1, (const int32_t[1]){64}}, .output_tensors = {1, (const int32_t[1]){65}} }, /* nl_49 */
    {.id = 51, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){62}}, .output_tensors = {1, (const int32_t[1]){66}} }, /* conv2d_51_pad_before */
    {.id = 51, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){66}}, .output_tensors = {1, (const int32_t[1]){67}} }, /* conv2d_51 */
    {.id = 53, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){62}}, .output_tensors = {1, (const int32_t[1]){68}} }, /* conv2d_53_pad_before */
    {.id = 53, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){68}}, .output_tensors = {1, (const int32_t[1]){69}} }, /* conv2d_53 */
    {.id = 55, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){56}}, .output_tensors = {1, (const int32_t[1]){70}} }, /* pad_55 */
    {.id = 56, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){70}}, .output_tensors = {1, (const int32_t[1]){71}} }, /* conv2d_56 */
    {.id = 57, .type = AI_LAYER_ELTWISE_INTEGER_TYPE, .input_tensors = {2, (const int32_t[2]){51, 71}}, .output_tensors = {1, (const int32_t[1]){72}} }, /* eltwise_57 */
    {.id = 73, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){72}}, .output_tensors = {1, (const int32_t[1]){73}} }, /* conv2d_73_pad_before */
    {.id = 73, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){73}}, .output_tensors = {1, (const int32_t[1]){74}} }, /* conv2d_73 */
    {.id = 74, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){74}}, .output_tensors = {1, (const int32_t[1]){75}} }, /* conv2d_74_pad_before */
    {.id = 74, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){75}}, .output_tensors = {1, (const int32_t[1]){76}} }, /* conv2d_74 */
    {.id = 75, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){76}}, .output_tensors = {1, (const int32_t[1]){77}} }, /* conv2d_75 */
    {.id = 76, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){77}}, .output_tensors = {1, (const int32_t[1]){78}} }, /* conv2d_76_pad_before */
    {.id = 76, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){78}}, .output_tensors = {1, (const int32_t[1]){79}} }, /* conv2d_76 */
    {.id = 77, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){79}}, .output_tensors = {1, (const int32_t[1]){80}} }, /* conv2d_77 */
    {.id = 78, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){80}}, .output_tensors = {1, (const int32_t[1]){81}} }, /* conv2d_78_pad_before */
    {.id = 78, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){81}}, .output_tensors = {1, (const int32_t[1]){82}} }, /* conv2d_78 */
    {.id = 79, .type = AI_LAYER_NL_TYPE, .input_tensors = {1, (const int32_t[1]){82}}, .output_tensors = {1, (const int32_t[1]){83}} }, /* nl_79 */
    {.id = 81, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){80}}, .output_tensors = {1, (const int32_t[1]){84}} }, /* conv2d_81_pad_before */
    {.id = 81, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){84}}, .output_tensors = {1, (const int32_t[1]){85}} }, /* conv2d_81 */
    {.id = 83, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){80}}, .output_tensors = {1, (const int32_t[1]){86}} }, /* conv2d_83_pad_before */
    {.id = 83, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){86}}, .output_tensors = {1, (const int32_t[1]){87}} }, /* conv2d_83 */
    {.id = 58, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){72}}, .output_tensors = {1, (const int32_t[1]){88}} }, /* pad_58 */
    {.id = 59, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){88}}, .output_tensors = {1, (const int32_t[1]){89}} }, /* conv2d_59 */
    {.id = 60, .type = AI_LAYER_ELTWISE_INTEGER_TYPE, .input_tensors = {2, (const int32_t[2]){46, 89}}, .output_tensors = {1, (const int32_t[1]){90}} }, /* eltwise_60 */
    {.id = 61, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){90}}, .output_tensors = {1, (const int32_t[1]){91}} }, /* conv2d_61_pad_before */
    {.id = 61, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){91}}, .output_tensors = {1, (const int32_t[1]){92}} }, /* conv2d_61 */
    {.id = 62, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){92}}, .output_tensors = {1, (const int32_t[1]){93}} }, /* conv2d_62_pad_before */
    {.id = 62, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){93}}, .output_tensors = {1, (const int32_t[1]){94}} }, /* conv2d_62 */
    {.id = 63, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){94}}, .output_tensors = {1, (const int32_t[1]){95}} }, /* conv2d_63 */
    {.id = 64, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){95}}, .output_tensors = {1, (const int32_t[1]){96}} }, /* conv2d_64_pad_before */
    {.id = 64, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){96}}, .output_tensors = {1, (const int32_t[1]){97}} }, /* conv2d_64 */
    {.id = 65, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){97}}, .output_tensors = {1, (const int32_t[1]){98}} }, /* conv2d_65 */
    {.id = 66, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){98}}, .output_tensors = {1, (const int32_t[1]){99}} }, /* conv2d_66_pad_before */
    {.id = 66, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){99}}, .output_tensors = {1, (const int32_t[1]){100}} }, /* conv2d_66 */
    {.id = 67, .type = AI_LAYER_NL_TYPE, .input_tensors = {1, (const int32_t[1]){100}}, .output_tensors = {1, (const int32_t[1]){101}} }, /* nl_67 */
    {.id = 69, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){98}}, .output_tensors = {1, (const int32_t[1]){102}} }, /* conv2d_69_pad_before */
    {.id = 69, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){102}}, .output_tensors = {1, (const int32_t[1]){103}} }, /* conv2d_69 */
    {.id = 71, .type = AI_LAYER_PAD_TYPE, .input_tensors = {1, (const int32_t[1]){98}}, .output_tensors = {1, (const int32_t[1]){104}} }, /* conv2d_71_pad_before */
    {.id = 71, .type = AI_LAYER_CONV2D_TYPE, .input_tensors = {1, (const int32_t[1]){104}}, .output_tensors = {1, (const int32_t[1]){105}} } /* conv2d_71 */
  },
  .n_nodes = 105
};
#endif

