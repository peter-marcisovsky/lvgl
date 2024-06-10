/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// This is LVGL ARGB8888 simple fill for ESP32S3 processor.
    .text
    .align  4
    .global lv_color_blend_to_argb8888_aes3
    .type   lv_color_blend_to_argb8888_aes3,@function
// The function implements the following C code:
// void lv_color_blend_to_argb8888(_lv_draw_sw_blend_fill_dsc_t * dsc);


// Input params
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
    ee.zero.q  q0                               // dummy TIE instruction, to enable the TIE

    l32i.n   a3,    a2,    4                    // a3 - dest_buff
    l32i.n   a4,    a2,    8                    // a4 - dest_w                in bytes
    l32i.n   a5,    a2,    12                   // a5 - dest_h                in bytes
    l32i.n   a6,    a2,    16                   // a6 - dest_stride           in bits
    l32i.n   a7,    a2,    20                   // a7 - src_buff (color)
    l32i.n   a8,    a7,    0                    // a8 - color as value
    slli     a11,   a4,    2                    // a11 - dest_w_bits

    movi     a7,    0xff000000                  // oppactiy mask
    or       a10,    a7,    a8                  // apply oppacity

    ee.movi.32.q   q0,   a10,  0                // fill q0 register from a10 by 32 bits
    ee.movi.32.q   q0,   a10,  1
    ee.movi.32.q   q0,   a10,  2
    ee.movi.32.q   q0,   a10,  3


    // Check dest_buff alignment
    movi.n   a7,   0xf                                  // 0xf alignment mask (16-byte alignment)
    and     a15,   a7,  a3                              // 16-byte alignment mask AND dest_buff pointer
    bnez    a15,   _unaligned_by_byte                   // branch if a15 not equals to zero

    // Check dest_stride alignment
    and     a15,   a7,  a6                              // 16-byte alignment mask AND dest_stride
    bnez    a15,   _unaligned_by_byte                   // branch if a15 not equals to zero

    // Check dest_w_bits alignment
    and     a15,   a7,  a11                             // 16-byte alignment mask AND dest_w_bits
    bnez    a15,   _unaligned_by_byte                   // branch if a15 not equals to zero

//**********************************************************************************************************************

    // all aligned, the most ideal case

    // dest_buff   (a3) - 16-byte aligned
    // dest_stride (a6) - 16-byte multiple
    // dest_w      (a4) - 16-byte multiple

    srli    a9,    a4,   2                              // a9 - loop_len = dest_w / 4
    sub     a6,    a6,   a11                            // dest_stride = dest_stride - dest_w_bits

    .outer_loop_aligned:

        loopnez  a9, ._main_loop_aligned                // 16 bytes (4 argb8888) in one loop
            ee.vst.128.ip q0, a3, 16                    // store 16 bytes from q0 to dest_buff a3
        ._main_loop_aligned:

        add     a3,  a3,  a6                            // dest_buff + dest_stride
        addi.n  a5,  a5,  -1                            // decrease the outer loop
    bnez a5, .outer_loop_aligned

    movi.n   a2, 0
    retw.n                                              // return

    _unaligned_by_byte:

    // Check dest_buff alignment
    movi.n   a7,    0x3                                 // 0x3 alignment mask (4-byte alignment)
    and     a15,    a7,   a3                            // 4-byte alignment mask AND dest_buff pointer
    bnez    a15,    _unaligned_by_bit                   // branch if a15 not equals to zero

    // Check dest_stride alignment
    and     a15,    a7,   a6                            // 4-byte alignment mask AND dest_stride pointer
    bnez    a15,    _unaligned_by_bit                   // branch if a15 not equals to zero

//**********************************************************************************************************************

    // either dest_buff or dest_stride is not 16-byte aligned
    // dest_w is always 4-byte multiple
    // all of the following are 4-byte aligned

    // dest_buff   (a3) - 16-byte, or 4-byte aligned
    // dest_stride (a6) - 16-byte, or 4-byte multiple
    // dest_w      (a4) - 4-byte multiple

    sub      a6,    a6,   a11                           // dest_stride = dest_stride - dest_w_bits
    movi.n   a7,    0xf                                 // 0xf alignment mask

    .outer_loop_aligned_by_byte:

        // alignment check
        and     a15,   a7,  a3                          // 0xf (alignment mask) AND dest_buff pointer
        mov     a12,   a11                              // a12 - local_dest_w_bits = dest_w_bits
        beqz    a15,   _dest_buff_aligned_by_byte       // branch if a15 equals to zero


            movi.n  a14,   16                           // a14 - 16
            sub     a15,   a14,   a15                   // a15 = 16 - unalignment (lower 4 bits of dest_buff address)
            sub     a12,   a12,   a15                   // local_dest_w_bits = len - (16 - unalignment)

            // keep setting until dest_buff is aligned
            // Check modulo 8 of the unalignment, if - then set 8 bytes
            bbci    a15,  3, _aligning_mod_8_check_byte // branch if 3-rd bit of unalignment a15 is clear
                s32i.n      a10,  a3,  0                // save 32 bits from a10 to dest_buff a3, offset 0 bytes
                s32i.n      a10,  a3,  4                // save 32 bits from a10 to dest_buff a3, offset 4 bytes
                addi.n      a3,   a3,  8                // increment dest_buff pointer by 8 bytes
            _aligning_mod_8_check_byte:

            // Check modulo 4 of the unalignment, if - then set 4 bytes
            bbci a15, 2, _aligning_mod_4_check_byte     // branch if 2-nd bit unalignment a15 is clear
                s32i.n      a10,  a3,  0                // save 32 bits from a10 to dest_buff a3, offset 0 bytes
                addi.n      a3,   a3,  4                // increment dest_buff pointer by 4 bytes
            _aligning_mod_4_check_byte:

        _dest_buff_aligned_by_byte:
        // Calculate main loop_len
        srli    a9,    a12,   4                         // a9 - loop_len = local_dest_w_bits / 16

        // Main loop
        loopnez  a9, ._main_loop_aligned_by_byte      // 16 bytes (4 argb8888) in one loop
            ee.vst.128.ip q0, a3, 16                    // store 16 bytes from q0 to dest_buff a3
        ._main_loop_aligned_by_byte:

        // Check modulo 8 of the dest_w, if - then set 8 bytes
        bbci a12, 3, _aligned_mod_8_check_byte          // branch if 3-rd bit of local_dest_w_bits a12 is clear
            ee.vst.l.64.ip    q0,  a3,  8               // save lower 64 bits from q0 to dest_buff a3, increase dest_buff pointer by 8 bytes
        _aligned_mod_8_check_byte:

        // Check modulo 4 of the dest_w, if - then set 4 bytes
        bbci a12, 2, _aligned_mod_4_check_byte          // branch if 2-nd bit of local_dest_w_bits a12 is clear
            s32i.n      a10,  a3,  0                    // save 32 bits from a10 to dest_buff a3, offset 0 bytes
            addi.n      a3,   a3,  4                    // increment dest_buff pointer by 4 bytes
        _aligned_mod_4_check_byte:

        add     a3,  a3,  a6                            // dest_buff + dest_stride
        addi.n  a5,  a5,  -1                            // decrease the outer loop
    bnez a5, .outer_loop_aligned_by_byte

    movi.n   a2, 1
    retw.n                                              // return

    _unaligned_by_bit:

//**********************************************************************************************************************

    // either dest_buff or dest_stride is not 4-byte aligned
    // dest_w is always 4-byte multiple

    // dest_buff   (a3) - 4-byte, or 1-byte aligned
    // dest_stride (a6) - 4-byte, or 1-byte multiple
    // dest_w      (a4) - 4-byte multiple


    mov   a13, a3

    ee.orq          q1,   q1,   q0                      // copy q0 to q1
    sub      a6,    a6,   a11                           // dest_stride = dest_stride - dest_w_bits
    movi.n   a7,    0xf                                 // 0xf alignment mask

    .outer_loop_aligned_by_bit:

        // alignment check
        and     a15,   a7,  a3                          // 0xf (alignment mask) AND dest_buff pointer
        mov     a12,   a11                              // a12 - local_dest_w_bits = dest_w_bits
        beqz    a15,   _dest_buff_aligned_by_bit        // branch if a15 equals to zero


            movi.n  a14,   16                           // a14 - 16
            sub     a15,   a14,   a15                   // a15 = 16 - unalignment (lower 4 bits of dest_buff address)
            sub     a12,   a12,   a15                   // local_dest_w_bits = len - (16 - unalignment)

            wur.sar_byte     a15                        // apply unalignment to the SAR_BYTE
            ee.src.q   q2,  q0,  q1                     // Shift

            // keep setting until dest_buff is aligned
            // Check modulo 8 of the unalignment, if - then set 8 bytes
            bbci    a15,  3, _aligning_mod_8_check_bit  // branch if 3-rd bit of unalignment a15 is clear
                s32i.n      a10,  a3,  0                // save 32 bits from a10 to dest_buff a3, offset 0 bytes
                s32i.n      a10,  a3,  4                // save 32 bits from a10 to dest_buff a3, offset 4 bytes
                addi.n      a3,   a3,  8                // increment dest_buff pointer by 8 bytes
            _aligning_mod_8_check_bit:

            // Check modulo 4 of the unalignment, if - then set 4 bytes
            bbci a15, 2, _aligning_mod_4_check_bit      // branch if 2-nd bit unalignment a15 is clear
                s32i.n      a10,  a3,  0                // save 32 bits from a10 to dest_buff a3, offset 0 bytes
                addi.n      a3,   a3,  4                // increment dest_buff pointer by 4 bytes
            _aligning_mod_4_check_bit:

            movi.n  a14, 0x3
            and     a14,    a14,   a15                            // 4-byte alignment mask AND dest_buff pointer
            beqz    a14,    _branch                   // branch if a15 not equals to zero
                s32i.n      a10,  a3,  0                // save 32 bits from a10 to dest_buff a3, offset 0 bytes
                addi.n      a3,   a3,  4                // increment dest_buff pointer by 4 bytes
            _branch:

            //// Check modulo 2 of the unalignment, if - then set 2 bytes
            //bbci a15, 1, _aligning_mod_2_check_bit          // branch if 1-st bit unalignment a15 is clear
            //    srli        a13,   a10,  16
            //    //addi.n      a3,    a3,  2                // increment dest_buff pointer by 2 bytes
            //    s16i        a13,   a3,  2                // save 16 bits from a10 to dest_buff a3, offset 0 bytes


            //    bbci a15, 0, _dest_buff_aligned_by_bit
            //        srli    a13,   a10,  8
            //        s8i     a13,   a3,   1
            //        addi.n  a3,    a3,   3
            //        j _dest_buff_aligned_by_bit
            //_aligning_mod_2_check_bit:

        //    // Check modulo 1 of the unalignment, if - then copy 1 byte
        //    bbci a15, 0, _dest_buff_aligned_by_bit              // branch if 0-th bit unalignment a15 is clear
        //        srli    a13,   a10,  24
        //        s8i         a13,   a3,  3                // save 8 bits from a10 to dest_buff a3, offset 0 bytes
        //        addi.n      a3,   a3,  3                // increment dest_buff pointer by 1 byte
        _dest_buff_aligned_by_bit:
        // Calculate main loop_len

        srli    a9,    a12,   4                         // a9 - loop_len = local_dest_w_bits / 16

        // Main loop
        loopnez  a9, ._main_loop_unaligned_by_bit       // 16 bytes (4 argb8888) in one loop
            ee.vst.128.ip q2, a3, 16                    // store 16 bytes from q0 to dest_buff a3
        ._main_loop_unaligned_by_bit:

        //// Check modulo 2 of the dest_w, if - then set 2 bytes
        bbci a12, 1, _aligned_mod_2_check_bit            // branch if 1-st bit of dest_w a12 is clear
            addi    a3, a3, -4
            srli    a14,   a10,  16
            s16i    a14,   a3,   2
            addi    a3, a3, 4

            // Check modulo 1 of the dest_w, if - then set 1 byte
            bbci a12, 0, _aligned_end            // branch if 0-th bit of dest_w a12 is clear
                addi    a3, a3, -4
                srli    a14,   a10,  8
                s8i     a14,   a3,   1
                addi    a3, a3, 4
                j _aligned_end
        _aligned_mod_2_check_bit:

        // Check modulo 1 of the dest_w, if - then set 1 byte
        bbci a12, 0, _aligned_end            // branch if 0-th bit of dest_w a12 is clear
            addi    a3, a3, -4
            srli    a14,   a10,  24
            s8i     a14,   a3,   3
            addi    a3, a3, 4
        _aligned_mod_1_check_bit:

        _aligned_end:

        // Check modulo 8 of the dest_w, if - then set 8 bytes
        bbci a12, 3, _aligned_mod_8_check_bit           // branch if 3-rd bit of local_dest_w_bits a12 is clear
            s32i.n      a10,  a3,  0                    // save 32 bits from a10 to dest_buff a3, offset 0 bytes
            s32i.n      a10,  a3,  4                    // save 32 bits from a10 to dest_buff a3, offset 0 bytes
            addi.n      a3,   a3,  8                    // increment dest_buff pointer by 4 bytes
            //ee.vst.l.64.ip    q2,  a3,  8               // save lower 64 bits from q0 to dest_buff a3, increase dest_buff pointer by 8 bytes
        _aligned_mod_8_check_bit:

        // Check modulo 4 of the dest_w, if - then set 4 bytes
        bbci a12, 2, _aligned_mod_4_check_bit           // branch if 2-nd bit of local_dest_w_bits a12 is clear
            s32i.n      a10,  a3,  0                    // save 32 bits from a10 to dest_buff a3, offset 0 bytes
            addi.n      a3,   a3,  4                    // increment dest_buff pointer by 4 bytes
        _aligned_mod_4_check_bit:
        //j _ret

        add     a3,  a3,  a6                            // dest_buff + dest_stride
        addi.n  a5,  a5,  -1                            // decrease the outer loop
    bnez a5, .outer_loop_aligned_by_bit

    movi.n   a2, 1
    retw.n                                              // return


_ret:
    movi.n   a2, 1
    retw.n 