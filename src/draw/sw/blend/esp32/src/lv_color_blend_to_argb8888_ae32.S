/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// This is memory access for ESP32S3 processor.
    .text
    .align  4
    .global lv_color_blend_to_argb8888_ae32
    .type   lv_color_blend_to_argb8888_ae32,@function
// The function implements the following C code:
// void lv_color_blend_to_argb8888_ae32(_lv_draw_sw_blend_fill_dsc_t * dsc);

// Input params                 Variables
//
// arr_dest - a2                loop_len    - a5
// set_val  - a3                p_arr_dest  - a8
// set_size - a4                8_bit_set   - a7
//                              16_bit_set  - a9
//                              32_bit_set  - a10
//                              align_mask  - a11

// Input params                 Variables
//
// dsc - a2                     

/*
esp32s3 optimized memset function works with both, aligned and unaligned data.

arr_dest aligned         - _main_loop, 16 bytes in one loop, only aligned data
                         - Check modulos to finish copying remaining data outside of the cycle
                         - Modulo 8 - S3 instruction for aligned data, the rest of the modulos are generic

arr_dest unaligned       - First, use generic instructions to align the arr_dest data (keep increasing 
                           the arr_dest pointer until the pointer is aligned)
                         - Once arr_dest is aligned treat the rest of the data as aligned, same as above

if the set_size is less than 16, jump to _less_than_16 label and set data without any s3 instructions or cycles
*/

lv_color_blend_to_argb8888_ae32:


    // ansi version of the memset (without TIE instructions) for testing purposes

    entry    a1,    32
    mov      a8,    a2                              // a8 - save arr_dest pointer

    movi.n  a7,    0xff                             // 0xff one-byte mask
    and     a7,    a7,   a3                         // mask upper 24 bits of a3

    slli    a6,    a7,   8                          // a6 - (masked)set_val << 8
    or      a9,    a6,   a7                         // a9 - (masked)set_val << 8 + (masked)set_val
                                                    // a9 - 16-bit set
    slli    a15,    a9,   16                        // a15 - a9 << 16
    or      a10,    a9,   a15                       // broadcast 8 bits from a3 to 32 bits 

    srli    a5,    a4,   4                          // a5 - loop_len = arr_len / 16

    // Run main loop which sets 16 bytes in one loop run
    loopnez a5, ._ansi_loop
        s32i.n      a10,  a2,  0                    // save 32 bits from a15 to arr_dest a2
        s32i.n      a10,  a2,  4                    // save 32 bits from a14 to arr_dest a2
        s32i.n      a10,  a2,  8                    // save 32 bits from a14 to arr_dest a2
        s32i.n      a10,  a2,  12                   // save 32 bits from a14 to arr_dest a2
        addi.n      a2,   a2,  16                   // increment arr_dest pointer by 8 bytes
    ._ansi_loop:

    // Finish the remaining bytes out of the loop
    // Check modulo 8 of the arr_len, if - then set 8 bytes
    bbci a4, 3, _mod_8_check                        // branch if 2-nd bit of arr_len is clear
        s32i.n      a10,  a2,  0                    // save 32 bits from a10 to arr_dest a2, offset 0 bytes
        s32i.n      a10,  a2,  4                    // save 32 bits from a10 to arr_dest a2, offset 0 bytes
        addi.n      a2,   a2,  8                    // increment arr_dest pointer by 4 bytes
    _mod_8_check:

    // Check modulo 4 of the arr_len, if - then set 4 bytes
    bbci a4, 2, _mod_4_check                        // branch if 2-nd bit of arr_len is clear
        s32i.n      a10,  a2,  0                    // save 32 bits from a10 to arr_dest a2, offset 0 bytes
        addi.n      a2,   a2,  4                    // increment arr_dest pointer by 4 bytes
    _mod_4_check:

    // Check modulo 2 of the arr_len, if - then set 2 bytes
    bbci a4, 1, _mod_2_check                        // branch if 1-st bit of arr_len is clear
        s16i        a9,  a2,  0                     // save 16 bits from a7 to arr_dest a2, offset 0 bytes
        addi.n      a2,  a2,  2                     // increment arr_dest pointer by 2 bytes
    _mod_2_check:

    // Check modulo 1 of the arr_len, if - then set 1 byte
    bbci a4, 0, _mod_1_check                        // branch if 0-th bit of arr_len is clear
        s8i         a7,  a2,   0                    // save 8 bits from a3 to arr_dest a2, offset 0 bytes
    _mod_1_check:

    mov      a2,    a8                              // copy the initial arr_dest pointer from a8 to arr_dest a2
    retw.n                                          // return
