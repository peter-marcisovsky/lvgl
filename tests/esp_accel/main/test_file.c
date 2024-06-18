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

// ------------------------------------------------- Defines -----------------------------------------------------------


#define CANARY_BITS 4

// ------------------------------------------------- Macros and Types --------------------------------------------------

static const char* TAG_LV_FILL = "LV Fill";

lv_color_t test_color = {
    .blue = 0x56,
    .green = 0x34,
    .red = 0x12,
};

typedef enum {
    ARGB8888,
    RGB565,
} color_format_t;

typedef struct {
    lv_color_t color;                       // Color 24 bit, (RGB, each 8 bit)
    color_format_t color_format;            // LV data type
    unsigned int min_w;                     // Minimum width
    unsigned int min_h;                     // Minimum height
    unsigned int max_w;                     // Maximum width
    unsigned int max_h;                     // Maximum height
    unsigned int min_unalign_bit;           // Minimum amount of unaligned bits
    unsigned int max_unalign_bit;           // Maximum amount of unaligned bits
    unsigned int unalign_step;              // Increment step in bits unalignment
    unsigned int stride_step;               // Increment step in stride
    unsigned int test_combinations_count;   // Count of fest combinations
} func_test_params_t;

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
 * @brief The actual LV Fill ARGB8888 functionality test
 */
static void lv_fill_argb8888_functionality(int w, int h, int stride, int unalign_bit);

/**
 * @brief The actual LV Fill RGB565 functionality test
 */
static void lv_fill_rgb565_functionality(int w, int h, int stride, int unalign_bit);

// ------------------------------------------------ Test cases ---------------------------------------------------------

TEST_CASE_MULTIPLE_STAGES("LV Fill functionality", "[lv_fill][functionality]",
        argb8888_functionality,
        rgb565_functionality)

TEST_CASE_MULTIPLE_STAGES("LV Fill benchmark", "[lv_fill][benchmark]",
        argb8888_benchmark,
        rgb565_benchmark)

// ------------------------------------------------ Test cases stages --------------------------------------------------

static void argb8888_functionality(void)
{
    func_test_params_t test_params = {
        .color = test_color,
        .color_format = ARGB8888,
        .min_w = 4,
        .min_h = 4,
        .max_w = 16,
        .max_h = 16,
        .min_unalign_bit = 0,
        .max_unalign_bit = 128,
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
        .min_w = 4,
        .min_h = 4,
        .max_w = 16,
        .max_h = 16,
        .min_unalign_bit = 0,
        .max_unalign_bit = 128,
        .unalign_step = 1,
        .stride_step = 1,
        .test_combinations_count = 0,
    };
    functionality_combinations(&test_params);
}

static void argb8888_benchmark(void)
{

}

static void rgb565_benchmark(void)
{

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
                for (int unalign_bit = test_params->min_unalign_bit; unalign_bit <= test_params->max_unalign_bit; unalign_bit += test_params->unalign_step) {
                    
                    switch (test_params->color_format) {
                    case ARGB8888: {
                        lv_fill_argb8888_functionality(w, h, stride, unalign_bit);
                        break;
                    }
                    case RGB565: {
                        lv_fill_rgb565_functionality(w, h, stride, unalign_bit);
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
    ESP_LOGI(TAG_LV_FILL, "Test combinations: %d\n", test_params->test_combinations_count);
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

    ESP_LOGD(TAG_LV_FILL, "ARGB8888 Calling ASM file");
    lv_draw_sw_blend_color_to_argb8888(&dsc_asm, true);
    ESP_LOGD(TAG_LV_FILL, "ARGB8888 Calling ANSI file");
    lv_draw_sw_blend_color_to_argb8888(&dsc_ansi, false);

    dest_buff_asm -= CANARY_BITS * sizeof(uint32_t);
    dest_buff_ansi -= CANARY_BITS * sizeof(uint32_t);

    char msg_buff[128];
    sprintf(msg_buff, "LV Fill ARGB8888: w = %d, h = %d, stride = %d, unalign_bit = %d\n\n", w, h, stride, unalign_bit);
    TEST_ASSERT_EACH_EQUAL_UINT32_MESSAGE(0, (uint32_t *)dest_buff_ansi, CANARY_BITS, msg_buff);
    TEST_ASSERT_EACH_EQUAL_UINT32_MESSAGE(0, (uint32_t *)dest_buff_asm, CANARY_BITS, msg_buff);
    TEST_ASSERT_EQUAL_UINT32_ARRAY_MESSAGE((uint32_t *)dest_buff_asm + CANARY_BITS, (uint32_t *)dest_buff_ansi + CANARY_BITS, h * stride, msg_buff);
    TEST_ASSERT_EACH_EQUAL_UINT32_MESSAGE(0, (uint32_t *)dest_buff_ansi + (total_len - CANARY_BITS), CANARY_BITS, msg_buff);
    TEST_ASSERT_EACH_EQUAL_UINT32_MESSAGE(0, (uint32_t *)dest_buff_asm + (total_len - CANARY_BITS), CANARY_BITS, msg_buff);

    free(mem_ansi);
    free(mem_asm);
}

static void lv_fill_rgb565_functionality(int w, int h, int stride, int unalign_bit)
{
    const unsigned int total_len = ((h * stride) + (CANARY_BITS * 2));

    uint16_t *mem_asm   = (uint16_t *)memalign(16, (total_len * sizeof(uint16_t)) + (unalign_bit * sizeof(uint8_t)));
    uint16_t *mem_ansi  = (uint16_t *)memalign(16, (total_len * sizeof(uint16_t)) + (unalign_bit * sizeof(uint8_t)));

    uint8_t *dest_buff_asm = NULL;
    uint8_t *dest_buff_ansi = NULL;

    dest_buff_asm = (uint8_t *)mem_asm + unalign_bit;
    dest_buff_ansi = (uint8_t *)mem_ansi + unalign_bit;

    for (int i = 0; i < CANARY_BITS; i++){
        ((uint16_t *)dest_buff_asm)[i] = 0;
        ((uint16_t *)dest_buff_ansi)[i] = 0;
    }

    for (int i = CANARY_BITS; i < (h * stride) + CANARY_BITS; i++){
        ((uint16_t *)dest_buff_asm)[i] = i;
        ((uint16_t *)dest_buff_ansi)[i] = i;
    }

    for (int i = total_len - CANARY_BITS; i < total_len; i++){
        ((uint16_t *)dest_buff_asm)[i] = 0;
        ((uint16_t *)dest_buff_ansi)[i] = 0;
    }

    dest_buff_asm += CANARY_BITS * sizeof(uint16_t);
    dest_buff_ansi += CANARY_BITS * sizeof(uint16_t);

    _lv_draw_sw_blend_fill_dsc_t dsc_asm;
    dsc_asm.dest_buf = (void*)dest_buff_asm;
    dsc_asm.dest_h = h;
    dsc_asm.dest_w = w;
    dsc_asm.dest_stride = stride * sizeof(uint16_t);
    dsc_asm.color = test_color;
    dsc_asm.mask_buf = NULL;
    dsc_asm.opa = LV_OPA_MAX;

    _lv_draw_sw_blend_fill_dsc_t dsc_ansi;
    dsc_ansi.dest_buf = (void*)dest_buff_ansi;
    dsc_ansi.dest_h = h;
    dsc_ansi.dest_w = w;
    dsc_ansi.dest_stride = stride * sizeof(uint16_t);
    dsc_ansi.color = test_color;
    dsc_ansi.mask_buf = NULL;
    dsc_ansi.opa = LV_OPA_MAX;

    ESP_LOGD(TAG_LV_FILL, "RGB565 Calling ASM file");
    lv_draw_sw_blend_color_to_rgb565(&dsc_asm, true);
    ESP_LOGD(TAG_LV_FILL, "RGB565 Calling ANSI file");
    lv_draw_sw_blend_color_to_rgb565(&dsc_ansi, false);

    dest_buff_asm -= CANARY_BITS * sizeof(uint16_t);
    dest_buff_ansi -= CANARY_BITS * sizeof(uint16_t);

    // Print results
    for (int i = 0; i < total_len; i++){
        ESP_LOGD(TAG_LV_FILL, "dest_buff[%d] ansi = %8x \t asm = %8x \n", i, ((uint16_t *)dest_buff_ansi)[i], ((uint16_t *)dest_buff_asm)[i]);
    }

    char msg_buff[128];
    sprintf(msg_buff, "LV Fill RGB565: w = %d, h = %d, stride = %d, unalign_bit = %d\n\n", w, h, stride, unalign_bit);
    TEST_ASSERT_EACH_EQUAL_UINT16_MESSAGE(0, (uint16_t *)dest_buff_ansi, CANARY_BITS, msg_buff);
    TEST_ASSERT_EACH_EQUAL_UINT16_MESSAGE(0, (uint16_t *)dest_buff_asm, CANARY_BITS, msg_buff);
    TEST_ASSERT_EQUAL_UINT16_ARRAY_MESSAGE((uint16_t *)dest_buff_asm + CANARY_BITS, (uint16_t *)dest_buff_ansi + CANARY_BITS, h * stride, msg_buff);
    TEST_ASSERT_EACH_EQUAL_UINT16_MESSAGE(0, (uint16_t *)dest_buff_ansi + (total_len - CANARY_BITS), CANARY_BITS, msg_buff);
    TEST_ASSERT_EACH_EQUAL_UINT16_MESSAGE(0, (uint16_t *)dest_buff_asm + (total_len - CANARY_BITS), CANARY_BITS, msg_buff);

    free(mem_asm);
    free(mem_ansi);
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
    const unsigned int unalign_bit = 1;

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
    ESP_LOGI(TAG_LV_FILL, "Benchmark aes3 ideal case: %.2f per sample", cycles_asm);
    ESP_LOGI(TAG_LV_FILL, "Benchmark ansi ideal case: %.2f per sample", cycles_ansi);
    ESP_LOGI(TAG_LV_FILL, "Improvement: %.2f times\n", improvement);

    dsc.dest_buf = dest_buff_align4;
    dsc.dest_h = w - 1;
    dsc.dest_w = h - 1;

    cycles_asm  = lv_fill_argb8888_benchmark(&dsc, true);
    cycles_ansi = lv_fill_argb8888_benchmark(&dsc, false);
    improvement = cycles_ansi / cycles_asm;
    ESP_LOGI(TAG_LV_FILL, "Benchmark aes3 common case: %.2f per sample", cycles_asm);
    ESP_LOGI(TAG_LV_FILL, "Benchmark ansi common case: %.2f per sample", cycles_ansi);
    ESP_LOGI(TAG_LV_FILL, "Improvement: %.2f times", improvement);

    free(dest_buff_align16);
}