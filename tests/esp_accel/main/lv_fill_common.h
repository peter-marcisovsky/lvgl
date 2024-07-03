/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "lv_draw_sw_blend.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ------------------------------------------------- Macros and Types --------------------------------------------------

/**
 * @brief Color formats
 */
typedef enum {
    ARGB8888,
    RGB565,
    RGB888,
    AL88,
    L8,
} color_format_t;

/**
 * @brief Functionality test parameters
 */
typedef struct {
    lv_color_t color;                       // Color 24 bit, (RGB, each 8 bit)
    color_format_t color_format;            // LV color format
    unsigned int min_w;                     // Minimum width of the test array
    unsigned int min_h;                     // Minimum height of the test array
    unsigned int max_w;                     // Maximum width of the test array
    unsigned int max_h;                     // Maximum height of the test array
    unsigned int min_unalign_byte;          // Minimum amount of unaligned bytes of the test array
    unsigned int max_unalign_byte;          // Maximum amount of unaligned bytes of the test array
    unsigned int unalign_step;              // Increment step in bytes unalignment of the test array
    unsigned int stride_step;               // Increment step in stride of the test array
    unsigned int test_combinations_count;   // Count of fest combinations
} func_test_params_t;

/**
 * @brief Benchmark test parameters
 */
typedef struct {
    void (*lv_fill_func)(_lv_draw_sw_blend_fill_dsc_t*);  // pointer to the DUT function
    color_format_t color_format;            // LV color format
    unsigned int height;                    // Test array height
    unsigned int width;                     // Test array width
    unsigned int stride;                    // Test array stride
    unsigned int cc_height;                 // Corner case test array height
    unsigned int cc_width;                  // Corner case test array width
    unsigned int benchmark_cycles;          // Count of benchmark cycles
    void *array_align16;                    // test array with 16 byte alignment - testing most ideal case
    void *array_align1;                     // test array with 1 byte alignment - testing wort case
} bench_test_params_t;

#ifdef __cplusplus
} /*extern "C"*/
#endif
