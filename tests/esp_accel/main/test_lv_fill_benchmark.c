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

// ------------------------------------------------- Macros and Types --------------------------------------------------

static const char* TAG_LV_FILL_BENCH = "LV Fill Benchmark";

static lv_color_t test_color = {
    .red = 0x12,
    .green = 0x34,
    .blue = 0x56,
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

static void lv_fill_benchmark_decode(color_format_t color_format);
static float lv_fill_benchmark_common(_lv_draw_sw_blend_fill_dsc_t *dsc, bool use_asm);

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
static void lv_fill_argb8888_benchmark(void);

/**
 * @brief The actual LV Fill RGB565 functionality test
 */
static void lv_fill_rgb565_benchmark(void);

// ------------------------------------------------ Test cases ---------------------------------------------------------

TEST_CASE_MULTIPLE_STAGES("LV Fill benchmark", "[lv_fill][benchmark]",
        argb8888_benchmark,
        rgb565_benchmark)

// ------------------------------------------------ Test cases stages --------------------------------------------------

static void argb8888_benchmark(void)
{
    lv_fill_benchmark_decode(ARGB8888);
}

static void rgb565_benchmark(void)
{
    lv_fill_benchmark_decode(RGB565);
}
// ------------------------------------------------ Static test functions ----------------------------------------------

static void lv_fill_benchmark_decode(color_format_t color_format)
{
    switch(color_format) {
    case ARGB8888: {
        lv_fill_argb8888_benchmark();
        break;
    }
    case RGB565: {
        lv_fill_rgb565_benchmark();
        break;
    }
    default:
        break;
    }
}

static portMUX_TYPE testnlock = portMUX_INITIALIZER_UNLOCKED;

static float lv_fill_benchmark_common(_lv_draw_sw_blend_fill_dsc_t *dsc, bool use_asm)
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

static void lv_fill_argb8888_benchmark(void)
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

    float cycles_asm  = lv_fill_benchmark_common(&dsc, true);
    float cycles_ansi = lv_fill_benchmark_common(&dsc, false);
    float improvement = cycles_ansi / cycles_asm;
    ESP_LOGI(TAG_LV_FILL_BENCH, "Benchmark aes3 ideal case: %.2f per sample", cycles_asm);
    ESP_LOGI(TAG_LV_FILL_BENCH, "Benchmark ansi ideal case: %.2f per sample", cycles_ansi);
    ESP_LOGI(TAG_LV_FILL_BENCH, "Improvement: %.2f times\n", improvement);

    dsc.dest_buf = dest_buff_align4;
    dsc.dest_h = w - 1;
    dsc.dest_w = h - 1;

    cycles_asm  = lv_fill_benchmark_common(&dsc, true);
    cycles_ansi = lv_fill_benchmark_common(&dsc, false);
    improvement = cycles_ansi / cycles_asm;
    ESP_LOGI(TAG_LV_FILL_BENCH, "Benchmark aes3 common case: %.2f per sample", cycles_asm);
    ESP_LOGI(TAG_LV_FILL_BENCH, "Benchmark ansi common case: %.2f per sample", cycles_ansi);
    ESP_LOGI(TAG_LV_FILL_BENCH, "Improvement: %.2f times", improvement);

    free(dest_buff_align16);
}

static void lv_fill_rgb565_benchmark(void)
{
    
}