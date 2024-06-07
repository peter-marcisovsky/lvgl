/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// This is memory access for ESP32S3 processor.
    .text
    .align  4
    .global lv_color_blend_to_argb8888_aes3
    .type   lv_color_blend_to_argb8888_aes3,@function
// The function implements the following C code:
// void lv_color_blend_to_argb8888_aes3(_lv_draw_sw_blend_fill_dsc_t * dsc);

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

// typedef struct {
//     uint32_t opa;                l32i    0
//     void * dst_buf;              l32i    4
//     uint32_t dst_w;              l32i    8
//     uint32_t dst_h;              l32i    12
//     uint32_t dst_stride;         l32i    16
//     const void * src_buf;        l32i    20
//     uint32_t src_stride;         l32i    24
//     const lv_opa_t * mask_buf;   l32i    28
//     uint32_t mask_stride;        l32i    32
// } asm_dsc_t;


lv_color_blend_to_argb8888_aes3:

    entry   a1,    32
    ee.zero.q  q0                 // dummy TIE instruction

    l32i.n   a3,    a2,    4      // a3 - dest_buff
    l32i.n   a4,    a2,    8      // a4 - dest_w                in bytes
    l32i.n   a5,    a2,    12     // a5 - dest_h                in bytes
    l32i.n   a6,    a2,    16     // a6 - dest_stride           in bits
    l32i.n   a7,    a2,    20     // a7 - src_buff (color)
    l32i.n   a8,    a7,    0      // a8 - color as value
    //movi     a12    a4            // a12 - local_dest_w

    movi     a7,    0xff000000                  // oppactiy mask
    or       a10,    a7,    a8                  // apply oppacity

    ee.movi.32.q   q0,   a10,  0                // fill q0 register from a10 by 32 bits
    ee.movi.32.q   q0,   a10,  1
    ee.movi.32.q   q0,   a10,  2
    ee.movi.32.q   q0,   a10,  3


    movi.n   a7,    0xf                             // 0xf alignment mask
    and     a15,   a7,  a3                          // 0xf (alignment mask) AND arr_dest pointer
    bnez    a15,   _unaligned                       // branch if a15 not equals to zero

    and     a15,   a7,  a6                          // 0xf stride mask
    bnez    a15,   _unaligned                       // branch if a15 not equals to zero

    movi.n   a7,    0x3                             // 0x3 multiple of 4 mask
    and     a15,   a7,  a4                          // dest_w must be multiple of 4
    bnez    a15,   _unaligned


    // dest buff is aligned, dest_w can be divided by 4, stride can be divided by 16
    srli    a9,    a4,   2                              // a9 - loop_len = dest_w / 4
    slli    a15,   a4,   2                              // dest_w to bits
    sub     a6,    a6,   a15                            // dest_stride = dest_stride - dest_w (in bits)

    .outer_loop_aligned:

        loopnez  a9, ._main_loop_aligned                 // 16 bytes (4 argb8888) in one loop
            ee.vst.128.ip q0, a3, 16                     // store 16 bytes from q0 to dest_buff a3
        ._main_loop_aligned:

        add  a3,  a3,  a6                               // dest_buff + dest_stride

        addi.n  a5,  a5,  -1
    bnez a5, .outer_loop_aligned

    movi.n   a2, 0
    retw.n                                          // return

    _unaligned:

    slli     a7,    a4,   2                         // dest_w to bits
    sub      a6,    a6,   a7                        // dest_stride = dest_stride - dest_w (in bits)
    movi.n   a7,    0xf                             // 0xf alignment mask

    .outer_loop_2:

        // alignment check
        and     a15,   a7,  a3                          // 0xf (alignment mask) AND arr_dest pointer
        l32i.n   a4,    a2,    8                        // a4 - dest_w
        slli     a4,    a4,    2
        beqz    a15,   _arr_dest_aligned                // branch if a15 equals to zero


            movi.n  a14,   16                           // a14 - 16
            sub     a15,   a14,   a15                   // a15 = 16 - unalignment (lower 4 bits of dest_buff address)
            sub     a4,    a4,    a15                   // local_dest_w = len - (16 - unalignment)

            // keep setting until arr_dest is aligned
            // Check modulo 8 of the unalignment, if - then set 8 bytes
            bbci    a15,  3, _aligning_mod_8_check      // branch if 3-rd bit of unalignment a15 is clear
                s32i.n      a10,  a3,  0                // save 32 bits from a10 to arr_dest a3, offset 0 bytes
                s32i.n      a10,  a3,  4                // save 32 bits from a10 to arr_dest a3, offset 4 bytes
                addi.n      a3,   a3,  8                // increment arr_dest pointer by 8 bytes
            _aligning_mod_8_check:

            // Check modulo 4 of the unalignment, if - then set 4 bytes
            bbci a15, 2, _aligning_mod_4_check          // branch if 2-nd bit unalignment a15 is clear
                s32i.n      a10,  a3,  0                // save 32 bits from a10 to arr_dest a3, offset 0 bytes
                addi.n      a3,   a3,  4                // increment arr_dest pointer by 4 bytes
            _aligning_mod_4_check:

            // TODO CHeck if setting correct 16 bit value from a10
            // Check modulo 2 of the unalignment, if - then set 2 bytes
            bbci a15, 1, _aligning_mod_2_check          // branch if 1-st bit unalignment a15 is clear
                s16i        a10,   a3,  0                // save 16 bits from a10 to arr_dest a3, offset 0 bytes
                addi.n      a3,   a3,  2                // increment arr_dest pointer by 2 bytes
            _aligning_mod_2_check:

            // TODO CHeck if setting correct 8 bit value from a10
            // Check modulo 1 of the unalignment, if - then copy 1 byte
            bbci a15, 0, _arr_dest_aligned              // branch if 0-th bit unalignment a15 is clear
                s8i         a10,   a3,  0                // save 8 bits from a10 to arr_dest a3, offset 0 bytes
                addi.n      a3,   a3,  1                // increment arr_dest pointer by 1 byte

        _arr_dest_aligned:
        // Calculate main loop_len
        srli    a9,    a4,   4                          // a9 - loop_len = dest_w (local_dest_w) / 16

        // Main loop
        loopnez  a9, ._main_loop_unaligned              // 16 bytes (4 argb8888) in one loop
            ee.vst.128.ip q0, a3, 16                    // store 16 bytes from q0 to dest_buff a3
        ._main_loop_unaligned:

        // Check modulo 8 of the dest_w, if - then set 8 bytes
        bbci a4, 3, _aligned_mod_8_check                // branch if 3-rd bit of dest_w a4 is clear
            ee.vst.l.64.ip    q0,  a3,  8                // save lower 64 bits from q0 to arr_dest a3, increase arr_dest pointer by 8 bytes
        _aligned_mod_8_check:

        // Check modulo 4 of the dest_w, if - then set 4 bytes
        bbci a4, 2, _aligned_mod_4_check                // branch if 2-nd bit of dest_w a4 is clear
            s32i.n      a10,  a3,  0                    // save 32 bits from a10 to arr_dest a3, offset 0 bytes
            addi.n      a3,   a3,  4                    // increment arr_dest pointer by 4 bytes
        _aligned_mod_4_check:

        // TODO CHeck if setting correct 16 bit value from a10
        // Check modulo 2 of the dest_w, if - then set 2 bytes
        bbci a4, 1, _aligned_mod_2_check                // branch if 1-st bit of dest_w a4 is clear
            s16i        a10,   a3,  0                    // save 16 bits from a10 to arr_dest a3, offset 0 bytes
            addi.n      a3,   a3,  2                    // increment arr_dest pointer by 2 bytes
        _aligned_mod_2_check:

        // TODO CHeck if setting correct 8 bit value from a10
        // Check modulo 1 of the dest_w, if - then set 1 byte
        bbci a4, 0, _aligned_mod_1_check                // branch if 0-th bit of dest_w a4 is clear
            s8i         a10,   a3,  0                    // save 8 bits from a10 to arr_dest a3, offset 0 bytes
        _aligned_mod_1_check:

        add  a3,  a3,  a6                               // dest_buff + dest_stride
        addi.n  a5,  a5,  -1
    bnez a5, .outer_loop_2

    nop
    nop

    movi.n   a2, 1
    retw.n                                          // return

