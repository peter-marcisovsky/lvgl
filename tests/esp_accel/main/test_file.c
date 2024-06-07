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

#define W_LEN 7
#define H_LEN 7
#define STRIDE W_LEN
#define CANARY_BITS 4
#define TOTAL_LEN ((H_LEN * STRIDE) + (CANARY_BITS * 2))
#define DEST_ALIGNED 1
#define UNALIGNMENT_BITS 0

static const char* TAG_ARGB888_TEST = "SW_BLEND_ARGB8888_TEST";

lv_color_t test_color = {
    .blue = 0x56,
    .green = 0x34,
    .red = 0x12,
};

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
    dsc_asm.color = test_color;
    dsc_asm.mask_buf = NULL;
    dsc_asm.opa = LV_OPA_MAX;

    _lv_draw_sw_blend_fill_dsc_t dsc_ansi;
    dsc_ansi.dest_buf = (void*)dest_buff_ansi;
    dsc_ansi.dest_h = H_LEN;
    dsc_ansi.dest_w = W_LEN;
    dsc_ansi.dest_stride = STRIDE * sizeof(uint32_t);
    dsc_ansi.color = test_color;
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

static void lv_fill_argb8888_functionality(int w, int h, int stride, int unalign_bit) 
{

    const unsigned int total_len = ((h * stride) + (CANARY_BITS * 2));

    uint32_t *mem_asm   = (uint32_t *)memalign(16, (total_len * sizeof(uint32_t)) + (unalign_bit * sizeof(uint8_t)));
    uint32_t *mem_ansi  = (uint32_t *)memalign(16, (total_len * sizeof(uint32_t)) + (unalign_bit * sizeof(uint8_t)));

    uint8_t *dest_buff_asm = NULL;
    uint8_t *dest_buff_ansi = NULL;

    dest_buff_asm = (uint8_t *)mem_asm + unalign_bit;
    dest_buff_ansi = (uint8_t *)mem_ansi + unalign_bit;

    for (int i = 0; i < CANARY_BITS; i++){
        ((uint32_t *)dest_buff_asm)[i] = 0;
        ((uint32_t *)dest_buff_ansi)[i] = 0;
    }

    for (int i = CANARY_BITS; i < (h * stride) + CANARY_BITS; i++){
        ((uint32_t *)dest_buff_asm)[i] = 0x1;
        ((uint32_t *)dest_buff_ansi)[i] = 0x1;
    }

    for (int i = total_len - CANARY_BITS; i < total_len; i++){
        ((uint32_t *)dest_buff_asm)[i] = 0;
        ((uint32_t *)dest_buff_ansi)[i] = 0;
    }

    dest_buff_asm += CANARY_BITS * sizeof(uint32_t);
    dest_buff_ansi += CANARY_BITS * sizeof(uint32_t);

    _lv_draw_sw_blend_fill_dsc_t dsc_asm = {
        .dest_buf = (void *)dest_buff_asm,
        .dest_h = h,
        .dest_w = w,
        .dest_stride = stride * sizeof(uint32_t),
        .color = test_color,
        .mask_buf = NULL,
        .opa = LV_OPA_MAX,
    };

    _lv_draw_sw_blend_fill_dsc_t dsc_ansi = {
        .dest_buf = (void *)dest_buff_ansi,
        .dest_h = h,
        .dest_w = w,
        .dest_stride = stride * sizeof(uint32_t),
        .color = test_color,
        .mask_buf = NULL,
        .opa = LV_OPA_MAX,
    };

    //ESP_LOGI("TAG", "USE ASM");
    lv_draw_sw_blend_color_to_argb8888(&dsc_asm, true);
    //ESP_LOGI("TAG", "USE ANSI");
    lv_draw_sw_blend_color_to_argb8888(&dsc_ansi, false);

    dest_buff_asm -= CANARY_BITS * sizeof(uint32_t);
    dest_buff_ansi -= CANARY_BITS * sizeof(uint32_t);

    TEST_ASSERT_EACH_EQUAL_UINT32(0, (uint32_t *)dest_buff_ansi, CANARY_BITS);
    TEST_ASSERT_EACH_EQUAL_UINT32(0, (uint32_t *)dest_buff_asm, CANARY_BITS);
    TEST_ASSERT_EQUAL_UINT32_ARRAY((uint32_t *)dest_buff_asm + CANARY_BITS, (uint32_t *)dest_buff_ansi + CANARY_BITS, h * stride);
    TEST_ASSERT_EACH_EQUAL_UINT32(0, (uint32_t *)dest_buff_ansi + (total_len - CANARY_BITS), CANARY_BITS);
    TEST_ASSERT_EACH_EQUAL_UINT32(0, (uint32_t *)dest_buff_asm + (total_len - CANARY_BITS), CANARY_BITS);

    free(mem_ansi);
    free(mem_asm);
}

TEST_CASE("lv_fill_argb8888 functionality all", "[test]")
{

    const unsigned int min_w = 4;
    const unsigned int min_h = 4;
    const unsigned int max_w = 16;
    const unsigned int max_h = 16;
    const unsigned int min_unalign_bit = 0;
    const unsigned int max_unalign_bit = 128;
    unsigned int test_combinations = 0;


    for (int w = min_w; w <= max_w; w++) {
        for (int h = min_h; h <= max_h; h++) {
            for (int stride = w; stride <= w * 2; stride++) {
                for (int unalign_bit = min_unalign_bit; unalign_bit <= max_unalign_bit; unalign_bit += 32) {
                    lv_fill_argb8888_functionality(w, h, stride, unalign_bit);
                    test_combinations++;
                }
            }
        }
    }

    ESP_LOGI(TAG_ARGB888_TEST, "Test combinations: %d\n", test_combinations);

}

static portMUX_TYPE testnlock = portMUX_INITIALIZER_UNLOCKED;

static float lv_fill_argb8888_benchmark(_lv_draw_sw_blend_fill_dsc_t *dsc, bool use_asm)
{
    const unsigned int repeat_count = 1000;

    portENTER_CRITICAL(&testnlock);
    lv_draw_sw_blend_color_to_argb8888(dsc, use_asm);

    const unsigned int start_b = xthal_get_ccount();
    for (int i = 0; i < repeat_count; i++) {
        lv_draw_sw_blend_color_to_argb8888(dsc, use_asm);
    }
    const unsigned int end_b = xthal_get_ccount();
    portEXIT_CRITICAL(&testnlock);

    const float total_b = end_b - start_b;
    const float cycles = total_b / (repeat_count);
    return cycles;
}

TEST_CASE("lv_fill_argb8888_aes3 benchmark", "[test]")
{
    const unsigned int w = 128;
    const unsigned int h = 128;
    const unsigned int stride = h;
    const unsigned int unalign_bit = 4;

    uint32_t *dest_buff_align16  = (uint32_t *)memalign(16, w * h * sizeof(uint32_t) + (unalign_bit * sizeof(uint8_t)));
    uint32_t *dest_buff_align4 = dest_buff_align16 + (unalign_bit * sizeof(uint8_t));

    _lv_draw_sw_blend_fill_dsc_t dsc = {
        .dest_buf = (void *)dest_buff_align16,
        .dest_h = h,
        .dest_w = w,
        .dest_stride = stride * sizeof(uint32_t),
        .color = test_color,
        .mask_buf = NULL,
        .opa = LV_OPA_MAX,
    };

    float cycles_asm  = lv_fill_argb8888_benchmark(&dsc, true);
    float cycles_ansi = lv_fill_argb8888_benchmark(&dsc, false);
    float improvement = cycles_ansi / cycles_asm;
    ESP_LOGI(TAG_ARGB888_TEST, "Benchmark aes3 ideal case: %.2f per sample", cycles_asm);
    ESP_LOGI(TAG_ARGB888_TEST, "Benchmark ansi ideal case: %.2f per sample", cycles_ansi);
    ESP_LOGI(TAG_ARGB888_TEST, "Improvement: %.2f times\n", improvement);

    dsc.dest_buf = dest_buff_align4;
    dsc.dest_h = w - 1;
    dsc.dest_w = h - 1;

    cycles_asm  = lv_fill_argb8888_benchmark(&dsc, true);
    cycles_ansi = lv_fill_argb8888_benchmark(&dsc, false);
    improvement = cycles_ansi / cycles_asm;
    ESP_LOGI(TAG_ARGB888_TEST, "Benchmark aes3 common case: %.2f per sample", cycles_asm);
    ESP_LOGI(TAG_ARGB888_TEST, "Benchmark ansi common case: %.2f per sample", cycles_ansi);
    ESP_LOGI(TAG_ARGB888_TEST, "Improvement: %.2f times", improvement);

    free(dest_buff_align16);
}