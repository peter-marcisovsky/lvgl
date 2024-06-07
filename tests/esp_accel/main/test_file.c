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

#define W_LEN 4
#define H_LEN 4
#define STRIDE W_LEN
#define CANARY_BITS 4
#define TOTAL_LEN ((H_LEN * STRIDE) + (CANARY_BITS * 2))
#define DEST_ALIGNED 1
#define UNALIGNMENT_BITS 1

TEST_CASE("lv_fill_argb8888 functionality", "[test]")
{
    #if DEST_ALIGNED
    uint32_t *mem_asm   = (uint32_t *)memalign(16, (TOTAL_LEN * sizeof(uint32_t)) + (UNALIGNMENT_BITS * sizeof(uint8_t)));
    uint32_t *mem_ansi  = (uint32_t *)memalign(16, (TOTAL_LEN * sizeof(uint32_t)) + (UNALIGNMENT_BITS * sizeof(uint8_t)));
    #else
    uint32_t *dest_buff_asm   = (uint32_t *)malloc(TOTAL_LEN * sizeof(uint32_t)) + (UNALIGNMENT_BITS * sizeof(uint8_t));
    uint32_t *dest_buff_ansi  = (uint32_t *)malloc(TOTAL_LEN * sizeof(uint32_t)) + (UNALIGNMENT_BITS * sizeof(uint8_t));
    #endif

    uint8_t *dest_buff_asm = NULL;
    uint8_t *dest_buff_ansi = NULL;

    lv_color_t color = {
        .blue = 0x56,
        .green = 0x34,
        .red = 0x12,
    };

    dest_buff_asm = (uint8_t *)mem_asm + UNALIGNMENT_BITS;
    dest_buff_ansi = (uint8_t *)mem_ansi + UNALIGNMENT_BITS;

    for (int i = 0; i < CANARY_BITS; i++){
        ((uint32_t *)dest_buff_asm)[i] = 0;
        ((uint32_t *)dest_buff_ansi)[i] = 0;
    }

    for (int i = CANARY_BITS; i < (H_LEN * STRIDE) + CANARY_BITS; i++){
        ((uint32_t *)dest_buff_asm)[i] = 0x1;
        ((uint32_t *)dest_buff_ansi)[i] = 0x1;
    }

    for (int i = TOTAL_LEN - CANARY_BITS; i < TOTAL_LEN; i++){
        ((uint32_t *)dest_buff_asm)[i] = 0;
        ((uint32_t *)dest_buff_ansi)[i] = 0;
    }

    dest_buff_asm += CANARY_BITS * sizeof(uint32_t);
    dest_buff_ansi += CANARY_BITS * sizeof(uint32_t);

    _lv_draw_sw_blend_fill_dsc_t dsc_asm;
    dsc_asm.dest_buf = (void*)dest_buff_asm;
    dsc_asm.dest_h = H_LEN;
    dsc_asm.dest_w = W_LEN;
    dsc_asm.dest_stride = STRIDE * sizeof(uint32_t);
    dsc_asm.color = color;
    dsc_asm.mask_buf = NULL;
    dsc_asm.opa = LV_OPA_MAX;

    _lv_draw_sw_blend_fill_dsc_t dsc_ansi;
    dsc_ansi.dest_buf = (void*)dest_buff_ansi;
    dsc_ansi.dest_h = H_LEN;
    dsc_ansi.dest_w = W_LEN;
    dsc_ansi.dest_stride = STRIDE * sizeof(uint32_t);
    dsc_ansi.color = color;
    dsc_ansi.mask_buf = NULL;
    dsc_ansi.opa = LV_OPA_MAX;

    ESP_LOGI("TAG", "USE ASM");
    lv_draw_sw_blend_color_to_argb8888(&dsc_asm, true);
    ESP_LOGI("TAG", "USE ANSI");
    lv_draw_sw_blend_color_to_argb8888(&dsc_ansi, false);

    dest_buff_asm -= CANARY_BITS * sizeof(uint32_t);
    dest_buff_ansi -= CANARY_BITS * sizeof(uint32_t);

    for (int i = 0; i < TOTAL_LEN; i++){
        printf("dest_buff[%d] ansi = %8lx \t asm = %8lx \n", i, ((uint32_t *)dest_buff_ansi)[i], ((uint32_t *)dest_buff_asm)[i]);
    }

    TEST_ASSERT_EACH_EQUAL_UINT32(0, (uint32_t *)dest_buff_ansi, CANARY_BITS);
    TEST_ASSERT_EACH_EQUAL_UINT32(0, (uint32_t *)dest_buff_asm, CANARY_BITS);
    TEST_ASSERT_EQUAL_UINT32_ARRAY((uint32_t *)dest_buff_asm + CANARY_BITS, (uint32_t *)dest_buff_ansi + CANARY_BITS, H_LEN * STRIDE);
    TEST_ASSERT_EACH_EQUAL_UINT32(0, (uint32_t *)dest_buff_ansi + (TOTAL_LEN - CANARY_BITS), CANARY_BITS);
    TEST_ASSERT_EACH_EQUAL_UINT32(0, (uint32_t *)dest_buff_asm + (TOTAL_LEN - CANARY_BITS), CANARY_BITS);

    free(mem_asm);
    free(mem_ansi);
}

TEST_CASE("Test1", "[test]")
{
    printf("Hello from test\n");
}

//static portMUX_TYPE testnlock = portMUX_INITIALIZER_UNLOCKED;
//
//TEST_CASE("lv_fill_argb8888_ansi benchmark", "[test]")
//{
//    uint32_t *dest_buff  = (uint32_t *)malloc(128 * 128 * sizeof(uint32_t));
//    lv_color_t color = {
//        .blue = 0,
//        .green = 0,
//        .red = 0
//    };
//
//    _lv_draw_sw_blend_fill_dsc_t dsc;
//    dsc.dest_buf = (void*)dest_buff;
//    dsc.dest_h = 128;
//    dsc.dest_w = 128;
//    dsc.dest_stride = 128;
//    dsc.color = color;
//    dsc.mask_buf = NULL;
//    dsc.opa = LV_OPA_MAX;
//
//    portENTER_CRITICAL(&testnlock);
//    lv_draw_sw_blend_color_to_argb8888(&dsc, true);
//
//    const unsigned int start_b = xthal_get_ccount();
//    const unsigned int repeat_count = 1000;
//    for (int i = 0; i < repeat_count; i++) {
//        lv_draw_sw_blend_color_to_argb8888(&dsc, true);
//    }
//    const unsigned int end_b = xthal_get_ccount();
//    portEXIT_CRITICAL(&testnlock);
//
//    const float total_b = end_b - start_b;
//    const float cycles = total_b / (repeat_count);
//    printf("\nBenchmark lv_fill_argb8888_ansi - %.2f per sample\n\n", cycles);
//
//    free(dest_buff);
//}
//
//TEST_CASE("lv_fill_argb8888_aes3 benchmark", "[test]")
//{
//    uint32_t *dest_buff  = (uint32_t *)malloc(128 * 128 * sizeof(uint32_t));
//    lv_color_t color = {
//        .blue = 0,
//        .green = 0,
//        .red = 0
//    };
//
//    _lv_draw_sw_blend_fill_dsc_t dsc;
//    dsc.dest_buf = (void*)dest_buff;
//    dsc.dest_h = 128;
//    dsc.dest_w = 128;
//    dsc.dest_stride = 128;
//    dsc.color = color;
//
//    portENTER_CRITICAL(&testnlock);
//    lv_draw_sw_blend_color_to_argb8888(&dsc, true);
//
//    const unsigned int start_b = xthal_get_ccount();
//    const unsigned int repeat_count = 1000;
//    for (int i = 0; i < repeat_count; i++) {
//        lv_draw_sw_blend_color_to_argb8888(&dsc, true);
//    }
//    const unsigned int end_b = xthal_get_ccount();
//    portEXIT_CRITICAL(&testnlock);
//
//    const float total_b = end_b - start_b;
//    const float cycles = total_b / (repeat_count);
//    printf("\nBenchmark lv_fill_argb8888_aes3 - %.2f per sample\n\n", cycles);
//
//    free(dest_buff);
//}