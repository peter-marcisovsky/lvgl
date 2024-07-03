/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include "unity.h"
#include "esp_log.h"
#include <malloc.h>
#include <inttypes.h>

#include "lv_draw_sw_blend.h"
#include "lv_draw_sw_blend_to_l8.h"
#include "lv_draw_sw_blend_to_al88.h"
#include "lv_draw_sw_blend_to_rgb565.h"
#include "lv_draw_sw_blend_to_argb8888.h"
#include "lv_draw_sw_blend_to_rgb888.h"

#include "lv_blend_esp32.h"
#include "lv_fill_common.h"

// ------------------------------------------------- Defines -----------------------------------------------------------

#define DBG_PRINT_OUTPUT false
#define CANARY_BYTES 4

// ------------------------------------------------- Macros and Types --------------------------------------------------

static const char* TAG_LV_FILL_FUNC = "LV Fill Functionality";

static lv_color_t test_color = {
    .blue = 0x56,
    .green = 0x34,
    .red = 0x12,
};

// ------------------------------------------------ Static function headers --------------------------------------------

/**
 * @brief Generate all the functionality test combinations
 *
 * - generate functionality test combinations, based on the provided test_params struct
 *
 * @param[in] test_params Pointer to structure with test_parameters
 */
static void functionality_combinations(func_test_params_t *test_params);

/**
 * @brief ARGB8888 functionality test case entry
 *
 * - initialize the ARGB8888 functionality test case
 */
static void argb8888_functionality(void);

/**
 * @brief RGB565 functionality test case entry
 *
 * - initialize the RGB565 functionality test case
 */
static void rgb565_functionality(void);

/**
 * @brief The actual LV Fill ARGB8888 functionality test
 */
static void lv_fill_argb8888_functionality(int w, int h, int stride, int unalign_byte);

/**
 * @brief The actual LV Fill RGB565 functionality test
 */
static void lv_fill_rgb565_functionality(int w, int h, int stride, int unalign_byte);

// ------------------------------------------------ Test cases ---------------------------------------------------------

TEST_CASE_MULTIPLE_STAGES("LV Fill functionality", "[lv_fill][functionality]",
        argb8888_functionality,
        rgb565_functionality)

// ------------------------------------------------ Test cases stages --------------------------------------------------

static void argb8888_functionality(void)
{
    func_test_params_t test_params = {
        .color = test_color,
        .color_format = ARGB8888,
        .min_w = 8,             // 8 is the lower limit for the esp32s3 asm implementation, otherwise esp32 is executed
        .min_h = 1,
        .max_w = 16,
        .max_h = 16,
        .min_unalign_byte = 0,
        .max_unalign_byte = 16,
        .unalign_step = 1,
        .stride_step = 1,
        .test_combinations_count = 0,
    };
    functionality_combinations(&test_params);
}

static void rgb565_functionality(void)
{
    func_test_params_t test_params = {
        .color = test_color,
        .color_format = RGB565,
        .min_w = 8,             // 8 is the lower limit for the esp32s3 asm implementation, otherwise esp32 is executed
        .min_h = 1,
        .max_w = 16,
        .max_h = 16,
        .min_unalign_byte = 0,
        .max_unalign_byte = 16,
        .unalign_step = 1,
        .stride_step = 1,
        .test_combinations_count = 0,
    };
    functionality_combinations(&test_params);
}

// ------------------------------------------------ Static test functions ----------------------------------------------


static void functionality_combinations(func_test_params_t *test_params)
{
    // Step width
    for (int w = test_params->min_w; w <= test_params->max_w; w++) {

        // Step height
        for (int h = test_params->min_h; h <= test_params->max_h; h++) {

            // Step stride
            for (int stride = w; stride <= w * 2; stride += test_params->stride_step) {

                // Step unalignment
                for (int unalign_byte = test_params->min_unalign_byte; unalign_byte <= test_params->max_unalign_byte; unalign_byte += test_params->unalign_step) {
                    
                    switch (test_params->color_format) {
                    case ARGB8888: {
                        lv_fill_argb8888_functionality(w, h, stride, unalign_byte);
                        break;
                    }
                    case RGB565: {
                        lv_fill_rgb565_functionality(w, h, stride, unalign_byte);
                        break;
                    }
                    default:
                        break;
                    }
                    test_params->test_combinations_count++;
                }
            }
        }
    }
    ESP_LOGI(TAG_LV_FILL_FUNC, "Test combinations: %d\n", test_params->test_combinations_count);
}

static void lv_fill_argb8888_functionality(int w, int h, int stride, int unalign_byte)
{
    const unsigned int total_len = ((h * stride) + (CANARY_BYTES * 2));

    uint32_t *mem_asm   = (uint32_t *)memalign(16, (total_len * sizeof(uint32_t)) + (unalign_byte * sizeof(uint8_t)));
    uint32_t *mem_ansi  = (uint32_t *)memalign(16, (total_len * sizeof(uint32_t)) + (unalign_byte * sizeof(uint8_t)));

    uint8_t *dest_buff_asm = NULL;
    uint8_t *dest_buff_ansi = NULL;

    dest_buff_asm = (uint8_t *)mem_asm + unalign_byte;
    dest_buff_ansi = (uint8_t *)mem_ansi + unalign_byte;

    for (int i = 0; i < CANARY_BYTES; i++){
        ((uint32_t *)dest_buff_asm)[i] = 0;
        ((uint32_t *)dest_buff_ansi)[i] = 0;
    }

    for (int i = CANARY_BYTES; i < (h * stride) + CANARY_BYTES; i++){
        ((uint32_t *)dest_buff_asm)[i] = i;
        ((uint32_t *)dest_buff_ansi)[i] = i;
    }

    for (int i = total_len - CANARY_BYTES; i < total_len; i++){
        ((uint32_t *)dest_buff_asm)[i] = 0;
        ((uint32_t *)dest_buff_ansi)[i] = 0;
    }

    dest_buff_asm += CANARY_BYTES * sizeof(uint32_t);
    dest_buff_ansi += CANARY_BYTES * sizeof(uint32_t);

    _lv_draw_sw_blend_fill_dsc_t dsc_asm = {
        .dest_buf = (void *)dest_buff_asm,
        .dest_h = h,
        .dest_w = w,
        .dest_stride = stride * sizeof(uint32_t),
        .color = test_color,
        .mask_buf = NULL,
        .opa = LV_OPA_MAX,
        .use_asm = true,
    };

    _lv_draw_sw_blend_fill_dsc_t dsc_ansi = {
        .dest_buf = (void *)dest_buff_ansi,
        .dest_h = h,
        .dest_w = w,
        .dest_stride = stride * sizeof(uint32_t),
        .color = test_color,
        .mask_buf = NULL,
        .opa = LV_OPA_MAX,
        .use_asm = false
    };

    ESP_LOGD(TAG_LV_FILL_FUNC, "ARGB8888 Calling ASM file");
    lv_draw_sw_blend_color_to_argb8888(&dsc_asm);
    ESP_LOGD(TAG_LV_FILL_FUNC, "ARGB8888 Calling ANSI file");
    lv_draw_sw_blend_color_to_argb8888(&dsc_ansi);

    dest_buff_asm -= CANARY_BYTES * sizeof(uint32_t);
    dest_buff_ansi -= CANARY_BYTES * sizeof(uint32_t);

    // Print results
    #if DBG_PRINT_OUTPUT
    for (uint32_t i = 0; i < total_len; i++){
        printf("dest_buff[%"PRIi32"] %s ansi = %8"PRIx32" \t asm = %8"PRIx32" \n", i, ((i < 10) ? (" ") : ("")), ((uint32_t *)dest_buff_ansi)[i], ((uint32_t *)dest_buff_asm)[i]);
    }
    printf("\n");
    #endif

    char msg_buff[128];
    sprintf(msg_buff, "LV Fill ARGB8888: w = %d, h = %d, stride = %d, unalign_byte = %d\n", w, h, stride, unalign_byte);
    TEST_ASSERT_EACH_EQUAL_UINT32_MESSAGE(0, (uint32_t *)dest_buff_ansi, CANARY_BYTES, msg_buff);
    TEST_ASSERT_EACH_EQUAL_UINT32_MESSAGE(0, (uint32_t *)dest_buff_asm, CANARY_BYTES, msg_buff);
    TEST_ASSERT_EQUAL_UINT32_ARRAY_MESSAGE((uint32_t *)dest_buff_asm + CANARY_BYTES, (uint32_t *)dest_buff_ansi + CANARY_BYTES, h * stride, msg_buff);
    TEST_ASSERT_EACH_EQUAL_UINT32_MESSAGE(0, (uint32_t *)dest_buff_ansi + (total_len - CANARY_BYTES), CANARY_BYTES, msg_buff);
    TEST_ASSERT_EACH_EQUAL_UINT32_MESSAGE(0, (uint32_t *)dest_buff_asm + (total_len - CANARY_BYTES), CANARY_BYTES, msg_buff);

    free(mem_ansi);
    free(mem_asm);
}

static void lv_fill_rgb565_functionality(int w, int h, int stride, int unalign_byte)
{
    const unsigned int total_len = ((h * stride) + (CANARY_BYTES * 2));

    uint16_t *mem_asm   = (uint16_t *)memalign(16, (total_len * sizeof(uint16_t)) + (unalign_byte * sizeof(uint8_t)));
    uint16_t *mem_ansi  = (uint16_t *)memalign(16, (total_len * sizeof(uint16_t)) + (unalign_byte * sizeof(uint8_t)));

    uint8_t *dest_buff_asm = NULL;
    uint8_t *dest_buff_ansi = NULL;

    dest_buff_asm = (uint8_t *)mem_asm + unalign_byte;
    dest_buff_ansi = (uint8_t *)mem_ansi + unalign_byte;

    for (int i = 0; i < CANARY_BYTES; i++){
        ((uint16_t *)dest_buff_asm)[i] = 0;
        ((uint16_t *)dest_buff_ansi)[i] = 0;
    }

    for (int i = CANARY_BYTES; i < (h * stride) + CANARY_BYTES; i++){
        ((uint16_t *)dest_buff_asm)[i] = i;
        ((uint16_t *)dest_buff_ansi)[i] = i;
    }

    for (int i = total_len - CANARY_BYTES; i < total_len; i++){
        ((uint16_t *)dest_buff_asm)[i] = 0;
        ((uint16_t *)dest_buff_ansi)[i] = 0;
    }

    dest_buff_asm += CANARY_BYTES * sizeof(uint16_t);
    dest_buff_ansi += CANARY_BYTES * sizeof(uint16_t);

    _lv_draw_sw_blend_fill_dsc_t dsc_asm = {
        .dest_buf = (void *)dest_buff_asm,
        .dest_h = h,
        .dest_w = w,
        .dest_stride = stride * sizeof(uint16_t),
        .color = test_color,
        .mask_buf = NULL,
        .opa = LV_OPA_MAX,
        .use_asm = true,
    };

    _lv_draw_sw_blend_fill_dsc_t dsc_ansi = {
        .dest_buf = (void *)dest_buff_ansi,
        .dest_h = h,
        .dest_w = w,
        .dest_stride = stride * sizeof(uint16_t),
        .color = test_color,
        .mask_buf = NULL,
        .opa = LV_OPA_MAX,
        .use_asm = false,
    };

    ESP_LOGD(TAG_LV_FILL_FUNC, "RGB565 Calling ASM file");
    lv_draw_sw_blend_color_to_rgb565(&dsc_asm);
    ESP_LOGD(TAG_LV_FILL_FUNC, "RGB565 Calling ANSI file");
    lv_draw_sw_blend_color_to_rgb565(&dsc_ansi);

    dest_buff_asm -= CANARY_BYTES * sizeof(uint16_t);
    dest_buff_ansi -= CANARY_BYTES * sizeof(uint16_t);

    // Print results
    #if DBG_PRINT_OUTPUT
    for (uint32_t i = 0; i < total_len; i++){
        printf("dest_buff[%"PRIi32"] %s ansi = %4"PRIx16" \t asm = %4"PRIx16" \n", i, ((i < 10) ? (" ") : ("")), ((uint16_t *)dest_buff_ansi)[i], ((uint16_t *)dest_buff_asm)[i]);
    }
    printf("\n");
    #endif

    char msg_buff[128];
    sprintf(msg_buff, "LV Fill RGB565: w = %d, h = %d, stride = %d, unalign_byte = %d\n", w, h, stride, unalign_byte);
    TEST_ASSERT_EACH_EQUAL_UINT16_MESSAGE(0, (uint16_t *)dest_buff_ansi, CANARY_BYTES, msg_buff);
    TEST_ASSERT_EACH_EQUAL_UINT16_MESSAGE(0, (uint16_t *)dest_buff_asm, CANARY_BYTES, msg_buff);
    TEST_ASSERT_EQUAL_UINT16_ARRAY_MESSAGE((uint16_t *)dest_buff_asm + CANARY_BYTES, (uint16_t *)dest_buff_ansi + CANARY_BYTES, h * stride, msg_buff);
    TEST_ASSERT_EACH_EQUAL_UINT16_MESSAGE(0, (uint16_t *)dest_buff_ansi + (total_len - CANARY_BYTES), CANARY_BYTES, msg_buff);
    TEST_ASSERT_EACH_EQUAL_UINT16_MESSAGE(0, (uint16_t *)dest_buff_asm + (total_len - CANARY_BYTES), CANARY_BYTES, msg_buff);

    free(mem_asm);
    free(mem_ansi);
}
