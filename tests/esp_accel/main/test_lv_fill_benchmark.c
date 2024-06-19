/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include "unity.h"
#include "esp_log.h"
#include <malloc.h>

#include "freertos/FreeRTOS.h"
#include "freertos/portable.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "lv_draw_sw_blend.h"
#include "lv_draw_sw_blend_to_l8.h"
#include "lv_draw_sw_blend_to_al88.h"
#include "lv_draw_sw_blend_to_rgb565.h"
#include "lv_draw_sw_blend_to_argb8888.h"
#include "lv_draw_sw_blend_to_rgb888.h"

#include "lv_blend_esp32.h"
#include "lv_fill_common.h"

#define WIDTH 128
#define HEIGHT 128
#define STRIDE WIDTH
#define UNALIGN_BITS 1
#define BENCHMARK_CYCLES 1000

// ------------------------------------------------- Macros and Types --------------------------------------------------

static const char* TAG_LV_FILL_BENCH = "LV Fill Benchmark";
static portMUX_TYPE testnlock = portMUX_INITIALIZER_UNLOCKED;

// ------------------------------------------------ Static function headers --------------------------------------------

/**
 * @brief ARGB8888 benchmark test case entry
 *
 * - initialize the ARGB8888 benchmark test case
 */
static void argb8888_benchmark(void);

/**
 * @brief RGB565 benchmark test case entry
 *
 * - initialize the RGB565 benchmark test case
 */
static void rgb565_benchmark(void);

/**
 * @brief Initialize the benchmark test
 */
static void lv_fill_benchmark_init(bench_test_params_t *test_params);

/**
 * @brief Run the benchmark test
 */
static float lv_fill_benchmark_run(_lv_draw_sw_blend_fill_dsc_t *dsc, bench_test_params_t *test_params);

// ------------------------------------------------ Test cases ---------------------------------------------------------

TEST_CASE_MULTIPLE_STAGES("LV Fill benchmark", "[lv_fill][benchmark]",
        argb8888_benchmark,
        rgb565_benchmark)

// ------------------------------------------------ Test cases stages --------------------------------------------------

static void argb8888_benchmark(void)
{
    ESP_LOGI(TAG_LV_FILL_BENCH, "for ARGB8888 color format");
    uint32_t *dest_array_align16  = (uint32_t *)memalign(16, WIDTH * HEIGHT * sizeof(uint32_t) + (UNALIGN_BITS * sizeof(uint8_t)));
    uint32_t *dest_array_align1 = dest_array_align16 + (UNALIGN_BITS * sizeof(uint8_t));

    bench_test_params_t test_params = {
        .lv_fill_func = &lv_draw_sw_blend_color_to_argb8888,
        .color_format = ARGB8888,
        .height = HEIGHT,
        .width = WIDTH,
        .stride = STRIDE * sizeof(uint32_t),
        .cc_height = HEIGHT - 1,
        .cc_width = WIDTH - 1,
        .benchmark_cycles = BENCHMARK_CYCLES,
        .array_align16 = (void *)dest_array_align16,
        .array_align1 = (void *)dest_array_align1,
    };
    lv_fill_benchmark_init(&test_params);
    free(dest_array_align16);
}

static void rgb565_benchmark(void)
{
    ESP_LOGI(TAG_LV_FILL_BENCH, "for RGB565 color format");
    uint16_t *dest_array_align16  = (uint16_t *)memalign(16, WIDTH * HEIGHT * sizeof(uint16_t) + (UNALIGN_BITS * sizeof(uint8_t)));
    uint16_t *dest_array_align1 = dest_array_align16 + (UNALIGN_BITS * sizeof(uint8_t));

    bench_test_params_t test_params = {
        .lv_fill_func = &lv_draw_sw_blend_color_to_rgb565,
        .color_format = RGB565,
        .height = HEIGHT,
        .width = WIDTH,
        .stride = STRIDE * sizeof(uint16_t),
        .cc_height = HEIGHT - 1,
        .cc_width = WIDTH - 1,
        .benchmark_cycles = BENCHMARK_CYCLES,
        .array_align16 = (void *)dest_array_align16,
        .array_align1 = (void *)dest_array_align1,
    };
    lv_fill_benchmark_init(&test_params);
    free(dest_array_align16);
}
// ------------------------------------------------ Static test functions ----------------------------------------------

static void lv_fill_benchmark_init(bench_test_params_t *test_params)
{
    lv_color_t test_color = {
        .red = 0x12,
        .green = 0x34,
        .blue = 0x56,
    };

    _lv_draw_sw_blend_fill_dsc_t dsc = {
        .dest_buf = (void *)test_params->array_align16,
        .dest_h = test_params->height,
        .dest_w = test_params->width,
        .dest_stride = test_params->stride,
        .color = test_color,
        .mask_buf = NULL,
        .opa = LV_OPA_MAX,
    };

    // Run benchmark with the most ideal input parameters
    // Dest array is 16 byte aligned, dest_w and dest_h are dividable by 4
    dsc.use_asm = true;
    float cycles_asm  = lv_fill_benchmark_run(&dsc, test_params);
    dsc.use_asm = false;
    float cycles_ansi = lv_fill_benchmark_run(&dsc, test_params);
    float improvement = cycles_ansi / cycles_asm;
    ESP_LOGI(TAG_LV_FILL_BENCH, "Benchmark aes3 ideal case: %.2f per sample", cycles_asm);
    ESP_LOGI(TAG_LV_FILL_BENCH, "Benchmark ansi ideal case: %.2f per sample", cycles_ansi);
    ESP_LOGI(TAG_LV_FILL_BENCH, "Improvement: %.2f times\n", improvement);

    dsc.dest_buf = test_params->array_align1;
    dsc.dest_h = test_params->cc_height;
    dsc.dest_w = test_params->cc_width;

    // Run benchmark with the corner case parameters
    // Dest array is 1 byte aligned, dest_w and dest_h are not dividable by 4
    dsc.use_asm = true;
    cycles_asm  = lv_fill_benchmark_run(&dsc, test_params);
    dsc.use_asm = false;
    cycles_ansi = lv_fill_benchmark_run(&dsc, test_params);
    improvement = cycles_ansi / cycles_asm;
    ESP_LOGI(TAG_LV_FILL_BENCH, "Benchmark aes3 common case: %.2f per sample", cycles_asm);
    ESP_LOGI(TAG_LV_FILL_BENCH, "Benchmark ansi common case: %.2f per sample", cycles_ansi);
    ESP_LOGI(TAG_LV_FILL_BENCH, "Improvement: %.2f times", improvement);

}

static float lv_fill_benchmark_run(_lv_draw_sw_blend_fill_dsc_t *dsc, bench_test_params_t *test_params)
{
    portENTER_CRITICAL(&testnlock);
    (test_params->lv_fill_func)(dsc);          // Call the DUT function for the first time to init the benchmark test

    const unsigned int start_b = xthal_get_ccount();
    for (int i = 0; i < test_params->benchmark_cycles; i++) {
        (test_params->lv_fill_func)(dsc);
    }
    const unsigned int end_b = xthal_get_ccount();
    portEXIT_CRITICAL(&testnlock);

    const float total_b = end_b - start_b;
    const float cycles = total_b / (test_params->benchmark_cycles);
    return cycles;
}