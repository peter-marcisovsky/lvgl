/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "esp_heap_caps.h"

static size_t before_free_8bit;
static size_t before_free_32bit;

#define TEST_MEMORY_LEAK_THRESHOLD (-530)
static void check_leak(size_t before_free, size_t after_free, const char *type)
{
    ssize_t delta = after_free - before_free;
    printf("MALLOC_CAP_%s: Before %u bytes free, After %u bytes free (delta %d)\n", type, before_free, after_free, delta);
    TEST_ASSERT_MESSAGE(delta >= TEST_MEMORY_LEAK_THRESHOLD, "memory leak");
}

void app_main(void)
{

    // ______  _____ ______   _               _   
    // |  _  \/  ___|| ___ \ | |             | |  
    // | | | |\ `--. | |_/ / | |_   ___  ___ | |_ 
    // | | | | `--. \|  __/  | __| / _ \/ __|| __|
    // | |/ / /\__/ /| |     | |_ |  __/\__ \| |_ 
    // |___/  \____/ \_|      \__| \___||___/ \__|

    printf("______  _____ ______   _               _   \r\n");
    printf("|  _  \\/  ___|| ___ \\ | |             | |  \r\n");
    printf("| | | |\\ `--. | |_/ / | |_   ___  ___ | |_ \r\n");
    printf("| | | | `--. \\|  __/  | __| / _ \\/ __|| __|\r\n");
    printf("| |/ / /\\__/ /| |     | |_ |  __/\\__ \\| |_ \r\n");
    printf("|___/  \\____/ \\_|      \\__| \\___||___/ \\__|\r\n");


    UNITY_BEGIN();
    unity_run_menu();
    UNITY_END();
}

/* setUp runs before every test */
void setUp(void)
{
    before_free_8bit = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    before_free_32bit = heap_caps_get_free_size(MALLOC_CAP_32BIT);
}

/* tearDown runs after every test */
void tearDown(void)
{
    size_t after_free_8bit = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    size_t after_free_32bit = heap_caps_get_free_size(MALLOC_CAP_32BIT);
    check_leak(before_free_8bit, after_free_8bit, "8BIT");
    check_leak(before_free_32bit, after_free_32bit, "32BIT");
}