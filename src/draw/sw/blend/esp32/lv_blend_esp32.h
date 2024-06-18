/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file lv_blend_esp32.h
 *
 */

#ifndef LV_BLEND_ESP32_H
#define LV_BLEND_ESP32_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/

#include <stdint.h>
#include "esp_log.h"
#include <sdkconfig.h>

/*********************
 *      DEFINES
 *********************/

#ifndef LV_DRAW_SW_COLOR_BLEND_TO_ARGB8888
#define LV_DRAW_SW_COLOR_BLEND_TO_ARGB8888(dsc) \
    _lv_color_blend_to_argb8888_esp32(dsc)
#endif

#ifndef LV_DRAW_SW_COLOR_BLEND_TO_RGB565
#define LV_DRAW_SW_COLOR_BLEND_TO_RGB565(dsc) \
    _lv_color_blend_to_rgb565_esp32(dsc)
#endif

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
    uint32_t opa;
    void * dst_buf;
    uint32_t dst_w;
    uint32_t dst_h;
    uint32_t dst_stride;
    const void * src_buf;
    uint32_t src_stride;
    const lv_opa_t * mask_buf;
    uint32_t mask_stride;
} asm_dsc_t;

/**********************
 *  STATIC VARIABLES
 **********************/

static const char *LV_BLEND_H = "LV_BLEND_H";

/**********************
 * GLOBAL PROTOTYPES
 **********************/

extern int lv_color_blend_to_argb8888_esp32_aes3(asm_dsc_t * asm_dsc);        // ESP32S3 assembly implementation
extern int lv_color_blend_to_argb8888_esp32_ae32(asm_dsc_t * asm_dsc);        // ESP32 assembly implementation

static inline lv_result_t _lv_color_blend_to_argb8888_esp32(_lv_draw_sw_blend_fill_dsc_t * dsc)
{
    asm_dsc_t asm_dsc = {
        .dst_buf = dsc->dest_buf,
        .dst_w = dsc->dest_w,
        .dst_h = dsc->dest_h,
        .dst_stride = dsc->dest_stride,
        .src_buf = &dsc->color,
    };

    ESP_LOGD(LV_BLEND_H, "Calling asm file");
    #if CONFIG_IDF_TARGET_ESP32S3
        const int ret = lv_color_blend_to_argb8888_esp32_aes3(&asm_dsc);
    #else
        const int ret = lv_color_blend_to_argb8888_esp32_ae32(&asm_dsc);
    #endif
    ESP_LOGD(LV_BLEND_H, "asm return: %d\n", ret);
    return LV_RESULT_OK;
}

extern int lv_color_blend_to_rgb565_esp32_aes3(asm_dsc_t * asm_dsc);        // ESP32S3 assembly implementation
extern int lv_color_blend_to_rgb565_esp32_ae32(asm_dsc_t * asm_dsc);        // ESP32 assembly implementation

static inline lv_result_t _lv_color_blend_to_rgb565_esp32(_lv_draw_sw_blend_fill_dsc_t * dsc)
{
    asm_dsc_t asm_dsc = {
        .dst_buf = dsc->dest_buf,
        .dst_w = dsc->dest_w,
        .dst_h = dsc->dest_h,
        .dst_stride = dsc->dest_stride,
        .src_buf = &dsc->color,
    };

    ESP_LOGD(LV_BLEND_H, "Calling asm file");
    //#if CONFIG_IDF_TARGET_ESP32S3
    //    const int ret = lv_color_blend_to_argb8888_esp32_aes3(&asm_dsc);
    //#else
    //    const int ret = lv_color_blend_to_argb8888_esp32_ae32(&asm_dsc);
    //#endif
    //int ret = lv_color_blend_to_rgb565_aes3(&asm_dsc);
    int ret = lv_color_blend_to_rgb565_esp32_ae32(&asm_dsc);
    ESP_LOGD(LV_BLEND_H, "asm return: %d\n", ret);
    return LV_RESULT_OK;
}

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_BLEND_ESP32_H*/