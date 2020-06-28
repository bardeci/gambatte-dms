
#include "scaler.h"
#include <SDL/SDL.h>

#ifdef __GNUC__
#       define unlikely(x)     __builtin_expect((x),0)
#       define prefetch(x, y)  __builtin_prefetch((x),(y))
#else
#       define unlikely(x)     (x)
#   define prefetch(x, y)
#endif

/* Ayla's fullscreen upscaler */
/* Upscale from 160x144 to 320x240 */
void fullscreen_upscale(uint32_t *to, uint32_t *from)
{
    uint32_t reg1, reg2, reg3, reg4;
    unsigned int x,y;

    /* Before:
     *    a b
     *    c d
     *    e f
     *
     * After (parenthesis = average):
     *    a      a      b      b
     *    (a,c)  (a,c)  (b,d)  (b,d)
     *    c      c      d      d
     *    (c,e)  (c,e)  (d,f)  (d,f)
     *    e      e      f      f
     */

    for (y=0; y < 240/5; y++) {
        for(x=0; x < 320/4; x++) {
            prefetch(to+4, 1);

            /* Read b-a */
            reg2 = *from;
            reg1 = reg2 & 0xffff0000;
            reg1 |= reg1 >> 16;

            /* Write b-b */
            *(to+1) = reg1;
            reg2 = reg2 & 0xffff;
            reg2 |= reg2 << 16;

            /* Write a-a */
            *to = reg2;

            /* Read d-c */
            reg4 = *(from + 160/2);
            reg3 = reg4 & 0xffff0000;
            reg3 |= reg3 >> 16;

            /* Write d-d */
            *(to + 2*320/2 +1) = reg3;
            reg4 = reg4 & 0xffff;
            reg4 |= reg4 << 16;

            /* Write c-c */
            *(to + 2*320/2) = reg4;

            /* Write (b,d)-(b,d) */
            if (unlikely(reg1 != reg3))
                reg1 = ((reg1 & 0xf7def7de) >> 1) + ((reg3 & 0xf7def7de) >> 1);
            *(to + 320/2 +1) = reg1;

            /* Write (a,c)-(a,c) */
            if (unlikely(reg2 != reg4))
                reg2 = ((reg2 & 0xf7def7de) >> 1) + ((reg4 & 0xf7def7de) >> 1);
            *(to + 320/2) = reg2;

            /* Read f-e */
            reg2 = *(from++ + 2*160/2);
            reg1 = reg2 & 0xffff0000;
            reg1 |= reg1 >> 16;

            /* Write f-f */
            *(to + 4*320/2 +1) = reg1;
            reg2 = reg2 & 0xffff;
            reg2 |= reg2 << 16;

            /* Write e-e */
            *(to + 4*320/2) = reg2;

            /* Write (d,f)-(d,f) */
            if (unlikely(reg2 != reg4))
                reg2 = ((reg2 & 0xf7def7de) >> 1) + ((reg4 & 0xf7def7de) >> 1);
            *(to++ + 3*320/2) = reg2;

            /* Write (c,e)-(c,e) */
            if (unlikely(reg1 != reg3))
                reg1 = ((reg1 & 0xf7def7de) >> 1) + ((reg3 & 0xf7def7de) >> 1);
            *(to++ + 3*320/2) = reg1;
        }

        to += 4*320/2;
        from += 2*160/2;
    }
}

/* Ayla's 1.5x Upscaler - 160x144 to 240x216 */
void scale15x(uint32_t *to, uint32_t *from)
{
    /* Before:
     *    a b c d
     *    e f g h
     *
     * After (parenthesis = average):
     *    a      (a,b)      b      c      (c,d)      d
     *    (a,e)  (a,b,e,f)  (b,f)  (c,g)  (c,d,g,h)  (d,h)
     *    e      (e,f)      f      g      (g,h)      h
     */

    uint32_t reg1, reg2, reg3, reg4, reg5;
    unsigned int x, y;

    for (y=0; y<216/3; y++) {
        for (x=0; x<240/6; x++) {
            prefetch(to+4, 1);

            /* Read b-a */
            reg1 = *from;
            reg5 = reg1 >> 16;
            if (unlikely((reg1 & 0xffff) != reg5)) {
                reg2 = (reg1 & 0xf7de0000) >> 1;
                reg1 = (reg1 & 0xffff) + reg2 + ((reg1 & 0xf7de) << 15);
            }

            /* Write (a,b)-a */
            *to = reg1;

            /* Read f-e */
            reg3 = *(from++ + 160/2);
            reg2 = reg3 >> 16;
            if (unlikely((reg3 & 0xffff) != reg2)) {
                reg4 = (reg3 & 0xf7de0000) >> 1;
                reg3 = (reg3 & 0xffff) + reg4 + ((reg3 & 0xf7de) << 15);
            }

            /* Write (e,f)-e */
            *(to + 2*320/2) = reg3;

            /* Write (a,b,e,f)-(a,e) */
            if (unlikely(reg1 != reg3))
                reg1 = ((reg1 & 0xf7def7de) >> 1) + ((reg3 & 0xf7def7de) >> 1);
            *(to++ + 320/2) = reg1;

            /* Read d-c */
            reg1 = *from;
            reg4 = reg1 << 16;

            /* Write c-b */
            reg5 |= reg4;
            *to = reg5;

            /* Read h-g */
            reg3 = *(from++ + 160/2);

            /* Write g-f */
            reg2 |= (reg3 << 16);
            *(to + 2*320/2) = reg2;

            /* Write (c,g)-(b,f) */
            if (unlikely(reg2 != reg5))
                reg2 = ((reg5 & 0xf7def7de) >> 1) + ((reg2 & 0xf7def7de) >> 1);
            *(to++ + 320/2) = reg2;

            /* Write d-(c,d) */
            if (unlikely((reg1 & 0xffff0000) != reg4)) {
                reg2 = (reg1 & 0xf7def7de) >> 1;
                reg1 = (reg1 & 0xffff0000) | ((reg2 + (reg2 >> 16)) & 0xffff);
            }
            *to = reg1;

            /* Write h-(g,h) */
            if (unlikely((reg3 & 0xffff) != reg3 >> 16)) {
                reg2 = (reg3 & 0xf7def7de) >> 1;
                reg3 = (reg3 & 0xffff0000) | ((reg2 + (reg2 >> 16)) & 0xffff);
            }
            *(to + 2*320/2) = reg3;

            /* Write (d,h)-(c,d,g,h) */
            if (unlikely(reg1 != reg3))
                reg1 = ((reg1 & 0xf7def7de) >> 1) + ((reg3 & 0xf7def7de) >> 1);
            *(to++ + 320/2) = reg1;
        }

        to += 2*360/2;
        from += 160/2;
    }
}

/*
 * Approximately bilinear scalers
 *
 * Copyright (C) 2019 hi-ban, Nebuleon <nebuleon.fumika@gmail.com>
 *
 * This function and all auxiliary functions are free software; you can
 * redistribute them and/or modify them under the terms of the GNU Lesser
 * General Public License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * These functions are distributed in the hope that they will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#define cR(A) (((A) & 0xf800) >> 8)
#define cG(A) (((A) & 0x7e0) >> 3)
#define cB(A) (((A) & 0x1f) << 3)

#define Weight1_1(A, B)  ((((cR(A) + cR(B)) / 2) & 0xf8) << 8 | (((cG(A) + cG(B)) / 2) & 0xfc) << 3 | (((cB(A) + cB(B)) / 2) & 0xf8) >> 3)
#define Weight1_2(A, B)  ((((cR(A) + cR(B) + cR(B)) / 3) & 0xf8) << 8 | (((cG(A) + cG(B) + cG(B)) / 3) & 0xfc) << 3 | (((cB(A) + cB(B) + cB(B)) / 3) & 0xf8) >> 3)
#define Weight2_1(A, B)  ((((cR(A) + cR(A) + cR(B)) / 3) & 0xf8) << 8 | (((cG(A) + cG(A) + cG(B)) / 3) & 0xfc) << 3 | (((cB(A) + cB(A) + cB(B)) / 3) & 0xf8) >> 3)
#define Weight1_4(A, B)  ((((cR(A) + cR(B) + cR(B) + cR(B)+ cR(B)) / 5) & 0xf8) << 8 | (((cG(A) + cG(B) + cG(B) + cG(B) + cG(B)) / 5) & 0xfc) << 3 | (((cB(A) + cB(B) + cB(B) + cB(B) + cB(B)) / 5) & 0xf8) >> 3)
#define Weight4_1(A, B)  ((((cR(A) + cR(A) + cR(A) + cR(A)+ cR(B)) / 5) & 0xf8) << 8 | (((cG(A) + cG(A) + cG(A) + cG(A) + cG(B)) / 5) & 0xfc) << 3 | (((cB(A) + cB(A) + cB(A) + cB(A) + cB(B)) / 5) & 0xf8) >> 3)
#define Weight2_3(A, B)  ((((cR(A) + cR(A) + cR(B) + cR(B)+ cR(B)) / 5) & 0xf8) << 8 | (((cG(A) + cG(A) + cG(B) + cG(B) + cG(B)) / 5) & 0xfc) << 3 | (((cB(A) + cB(A) + cB(B) + cB(B) + cB(B)) / 5) & 0xf8) >> 3)
#define Weight3_2(A, B)  ((((cR(A) + cR(A) + cR(A) + cR(B)+ cR(B)) / 5) & 0xf8) << 8 | (((cG(A) + cG(A) + cG(A) + cG(B) + cG(B)) / 5) & 0xfc) << 3 | (((cB(A) + cB(A) + cB(A) + cB(B) + cB(B)) / 5) & 0xf8) >> 3)

#define Weight1_1_1_1(A, B, C, D)  ((((cR(A) + cR(B) + cR(C) + cR(D)) / 4) & 0xf8) << 8 | (((cG(A) + cG(B) + cG(C) + cG(D)) / 4) & 0xfc) << 3 | (((cB(A) + cB(B) + cB(C) + cB(D)) / 4) & 0xf8) >> 3)

/* Upscales a 160x144 image to 240x216 using a pseudo-bilinear resampling algorithm.
 *
 * Input:
 *   src: A packed 160x144 pixel image. The pixel format of this image is RGB 565.
 * Output:
 *   dst: A packed 240x216 pixel image. The pixel format of this image is RGB 565.
 */

void scale15x_pseudobilinear(uint32_t* dst, uint32_t* src)
{
    uint16_t* Src16 = (uint16_t*) src;
    uint16_t* Dst16 = (uint16_t*) dst;

    // There are 80 blocks of 2 pixels horizontally, and 72 of 2 vertically.
    // Each block of 2x2 becomes 3x3.

    uint32_t BlockX, BlockY;
    uint16_t* BlockSrc;
    uint16_t* BlockDst;
    for (BlockY = 0; BlockY < 72; BlockY++)
    {
        BlockSrc = Src16 + BlockY * 160 * 2;
        BlockDst = Dst16 + BlockY * 320 * 3;
        for (BlockX = 0; BlockX < 80; BlockX++)
        {   
            // HORIZONTAL:
            // Before:          After:
            // (a)(b)--->(c)    (a)(abb)(bbc)
            //
            //
            // VERTICAL:
            // Before:          After:
            // (a)              (a)
            // (b)              (abb)
            //  |               (bbc)
            //  V
            // (c)

            if((BlockY == 71) && (BlockX == 79)){
                // -- Row 1 --
                uint16_t  _1 = *(BlockSrc               );
                *(BlockDst               ) = _1;
                uint16_t  _2 = *(BlockSrc            + 1);
                *(BlockDst            + 1) = Weight1_2( _1,  _2);
                *(BlockDst            + 2) = _2;

                // -- Row 2 --
                uint16_t  _4 = *(BlockSrc + 160 *  1    );
                *(BlockDst + 320 *  1    ) = Weight1_2( _1,  _4);
                uint16_t  _5 = *(BlockSrc + 160 *  1 + 1);
                *(BlockDst + 320 *  1 + 1) = Weight1_2(Weight1_2( _1,  _2), Weight1_2( _4,  _5));
                *(BlockDst + 320 *  1 + 2) = Weight1_2( _2, _5);

                // -- Row 3 --
                *(BlockDst + 320 *  2    ) = _4;
                *(BlockDst + 320 *  2 + 1) = Weight1_2( _4,  _5);
                *(BlockDst + 320 *  2 + 2) = _5;

            } else if(BlockX == 79){
                // -- Row 1 --
                uint16_t  _1 = *(BlockSrc               );
                *(BlockDst               ) = _1;
                uint16_t  _2 = *(BlockSrc            + 1);
                *(BlockDst            + 1) = Weight1_2( _1,  _2);
                *(BlockDst            + 2) = _2;

                // -- Row 2 --
                uint16_t  _4 = *(BlockSrc + 160 *  1    );
                *(BlockDst + 320 *  1    ) = Weight1_2( _1,  _4);
                uint16_t  _5 = *(BlockSrc + 160 *  1 + 1);
                *(BlockDst + 320 *  1 + 1) = Weight1_2(Weight1_2( _1,  _2), Weight1_2( _4,  _5));
                *(BlockDst + 320 *  1 + 2) = Weight1_2( _2, _5);

                // -- Row 3 --
                uint16_t  _7 = *(BlockSrc + 160 *  2    );
                *(BlockDst + 320 *  2    ) = Weight2_1( _4,  _7);
                uint16_t  _8 = *(BlockSrc + 160 *  2 + 1);
                *(BlockDst + 320 *  2 + 1) = Weight2_1(Weight1_2( _4,  _5), Weight1_2( _7,  _8));
                *(BlockDst + 320 *  2 + 2) = Weight2_1( _5, _8);

            } else if(BlockY == 71){
                // -- Row 1 --
                uint16_t  _1 = *(BlockSrc               );
                *(BlockDst               ) = _1;
                uint16_t  _2 = *(BlockSrc            + 1);
                *(BlockDst            + 1) = Weight1_2( _1,  _2);
                uint16_t  _3 = *(BlockSrc            + 2);
                *(BlockDst            + 2) = Weight2_1( _2,  _3);

                // -- Row 2 --
                uint16_t  _4 = *(BlockSrc + 160 *  1    );
                *(BlockDst + 320 *  1    ) = Weight1_2( _1,  _4);
                uint16_t  _5 = *(BlockSrc + 160 *  1 + 1);
                *(BlockDst + 320 *  1 + 1) = Weight1_2(Weight1_2( _1,  _2), Weight1_2( _4,  _5));
                uint16_t  _6 = *(BlockSrc + 160 *  1 + 2);
                *(BlockDst + 320 *  1 + 2) = Weight1_2(Weight2_1( _2,  _3), Weight2_1( _5,  _6));

                // -- Row 3 --
                *(BlockDst + 320 *  2    ) = _4;
                *(BlockDst + 320 *  2 + 1) = Weight1_2( _4,  _5);
                *(BlockDst + 320 *  2 + 2) = Weight2_1( _5,  _6);

            } else {
                // -- Row 1 --
                uint16_t  _1 = *(BlockSrc               );
                *(BlockDst               ) = _1;
                uint16_t  _2 = *(BlockSrc            + 1);
                *(BlockDst            + 1) = Weight1_2( _1,  _2);
                uint16_t  _3 = *(BlockSrc            + 2);
                *(BlockDst            + 2) = Weight2_1( _2,  _3);

                // -- Row 2 --
                uint16_t  _4 = *(BlockSrc + 160 *  1    );
                *(BlockDst + 320 *  1    ) = Weight1_2( _1,  _4);
                uint16_t  _5 = *(BlockSrc + 160 *  1 + 1);
                *(BlockDst + 320 *  1 + 1) = Weight1_2(Weight1_2( _1,  _2), Weight1_2( _4,  _5));
                uint16_t  _6 = *(BlockSrc + 160 *  1 + 2);
                *(BlockDst + 320 *  1 + 2) = Weight1_2(Weight2_1( _2,  _3), Weight2_1( _5,  _6));

                // -- Row 3 --
                uint16_t  _7 = *(BlockSrc + 160 *  2    );
                *(BlockDst + 320 *  2    ) = Weight2_1( _4,  _7);
                uint16_t  _8 = *(BlockSrc + 160 *  2 + 1);
                *(BlockDst + 320 *  2 + 1) = Weight2_1(Weight1_2( _4,  _5), Weight1_2( _7,  _8));
                uint16_t  _9 = *(BlockSrc + 160 *  2 + 2);
                *(BlockDst + 320 *  2 + 2) = Weight2_1(Weight2_1( _5,  _6), Weight2_1( _8,  _9));
            }

            BlockSrc += 2;
            BlockDst += 3;
        }
    }
}

/* Upscales a 160x144 image to 320x240 using a pseudo-bilinear resampling algorithm.
 *
 * Input:
 *   src: A packed 160x144 pixel image. The pixel format of this image is RGB 565.
 * Output:
 *   dst: A packed 320x240 pixel image. The pixel format of this image is RGB 565.
 */

void fullscreen_upscale_pseudobilinear(uint32_t* dst, uint32_t* src)
{
    uint16_t* Src16 = (uint16_t*) src;
    uint16_t* Dst16 = (uint16_t*) dst;

    // There are 160 blocks of 1 pixels horizontally, and 48 of 3 vertically.
    // Each block of 1x3 becomes 2x5.

    uint32_t BlockX, BlockY;
    uint16_t* BlockSrc;
    uint16_t* BlockDst;
    for (BlockY = 0; BlockY < 48; BlockY++)
    {
        BlockSrc = Src16 + BlockY * 160 * 3;
        BlockDst = Dst16 + BlockY * 320 * 5;
        for (BlockX = 0; BlockX < 160; BlockX++)
        {   
            // HORIZONTAL:
            // Before:      After:
            // (a)--->(b)   (a)(ab)
            //
            //
            // VERTICAL:
            // Before:      After:
            // (a)          (a)
            // (b)          (aabbb)
            // (c)          (bbbbc)
            //  |           (bcccc)
            //  V           (cccdd)
            // (d)

            if((BlockX == 159) && (BlockY == 47)){
                // -- Row 1 --
                uint16_t  _1 = *(BlockSrc               );
                *(BlockDst               ) = _1;
                *(BlockDst            + 1) = _1;

                // -- Row 2 --
                uint16_t  _2 = *(BlockSrc + 160 *  1    );
                *(BlockDst + 320 *  1    ) = Weight2_3( _1,  _2);
                *(BlockDst + 320 *  1 + 1) = Weight2_3( _1,  _2);

                // -- Row 3 --
                uint16_t  _3 = *(BlockSrc + 160 *  2    );
                *(BlockDst + 320 *  2    ) = Weight4_1( _2,  _3);
                *(BlockDst + 320 *  2 + 1) = Weight4_1( _2,  _3);

                // -- Row 4 --
                *(BlockDst + 320 *  3    ) = Weight1_4( _2,  _3);
                *(BlockDst + 320 *  3 + 1) = Weight1_4( _2,  _3);

                // -- Row 5 --
                *(BlockDst + 320 *  4    ) = _3;
                *(BlockDst + 320 *  4 + 1) = _3;

            } else if(BlockX == 159){
                // -- Row 1 --
                uint16_t  _1 = *(BlockSrc               );
                *(BlockDst               ) = _1;
                *(BlockDst            + 1) = _1;

                // -- Row 2 --
                uint16_t  _2 = *(BlockSrc + 160 *  1    );
                *(BlockDst + 320 *  1    ) = Weight2_3( _1,  _2);
                *(BlockDst + 320 *  1 + 1) = Weight2_3( _1,  _2);

                // -- Row 3 --
                uint16_t  _3 = *(BlockSrc + 160 *  2    );
                *(BlockDst + 320 *  2    ) = Weight4_1( _2,  _3);
                *(BlockDst + 320 *  2 + 1) = Weight4_1( _2,  _3);

                // -- Row 4 --
                *(BlockDst + 320 *  3    ) = Weight1_4( _2,  _3);
                *(BlockDst + 320 *  3 + 1) = Weight1_4( _2,  _3);

                // -- Row 5 --
                uint16_t  _4 = *(BlockSrc + 160 *  3    );
                *(BlockDst + 320 *  4    ) = Weight3_2( _3,  _4);
                *(BlockDst + 320 *  4 + 1) = Weight3_2( _3,  _4);

            } else if(BlockY == 47){
                // -- Row 1 --
                uint16_t  _1 = *(BlockSrc               );
                *(BlockDst               ) = _1;
                uint16_t  _A = *(BlockSrc            + 1);
                *(BlockDst            + 1) = Weight1_1( _1,  _A);

                // -- Row 2 --
                uint16_t  _2 = *(BlockSrc + 160 *  1    );
                *(BlockDst + 320 *  1    ) = Weight2_3( _1,  _2);
                uint16_t  _B = *(BlockSrc + 160 *  1 + 1);
                *(BlockDst + 320 *  1 + 1) = Weight2_3(Weight1_1( _1,  _A), Weight1_1( _2,  _B));

                // -- Row 3 --
                uint16_t  _3 = *(BlockSrc + 160 *  2    );
                *(BlockDst + 320 *  2    ) = Weight4_1( _2,  _3);
                uint16_t  _C = *(BlockSrc + 160 *  2 + 1);
                *(BlockDst + 320 *  2 + 1) = Weight4_1(Weight1_1( _2,  _B), Weight1_1( _3,  _C));

                // -- Row 4 --
                *(BlockDst + 320 *  3    ) = Weight1_4( _2,  _3);
                *(BlockDst + 320 *  3 + 1) = Weight1_4(Weight1_1( _2,  _B), Weight1_1( _3,  _C));

                // -- Row 5 --
                *(BlockDst + 320 *  4    ) = _3;
                *(BlockDst + 320 *  4 + 1) = Weight1_1( _3,  _C);

            } else {
                // -- Row 1 --
                uint16_t  _1 = *(BlockSrc               );
                *(BlockDst               ) = _1;
                uint16_t  _A = *(BlockSrc            + 1);
                *(BlockDst            + 1) = Weight1_1( _1,  _A);

                // -- Row 2 --
                uint16_t  _2 = *(BlockSrc + 160 *  1    );
                *(BlockDst + 320 *  1    ) = Weight2_3( _1,  _2);
                uint16_t  _B = *(BlockSrc + 160 *  1 + 1);
                *(BlockDst + 320 *  1 + 1) = Weight2_3(Weight1_1( _1,  _A), Weight1_1( _2,  _B));

                // -- Row 3 --
                uint16_t  _3 = *(BlockSrc + 160 *  2    );
                *(BlockDst + 320 *  2    ) = Weight4_1( _2,  _3);
                uint16_t  _C = *(BlockSrc + 160 *  2 + 1);
                *(BlockDst + 320 *  2 + 1) = Weight4_1(Weight1_1( _2,  _B), Weight1_1( _3,  _C));

                // -- Row 4 --
                *(BlockDst + 320 *  3    ) = Weight1_4( _2,  _3);
                *(BlockDst + 320 *  3 + 1) = Weight1_4(Weight1_1( _2,  _B), Weight1_1( _3,  _C));

                // -- Row 5 --
                uint16_t  _4 = *(BlockSrc + 160 *  3    );
                *(BlockDst + 320 *  4    ) = Weight3_2( _3,  _4);
                uint16_t  _D = *(BlockSrc + 160 *  3 + 1);
                *(BlockDst + 320 *  4 + 1) = Weight3_2(Weight1_1( _3,  _C), Weight1_1( _4,  _D));

            }

            BlockSrc += 1;
            BlockDst += 2;
        }
    }
}

/* Upscales a 160x144 image to 240x216 using a faster resampling algorithm.
 *
 * Input:
 *   src: A packed 160x144 pixel image. The pixel format of this image is RGB 565.
 * Output:
 *   dst: A packed 240x216 pixel image. The pixel format of this image is RGB 565.
 */

void scale15x_fast(uint32_t* dst, uint32_t* src)
{
    uint16_t* Src16 = (uint16_t*) src;
    uint16_t* Dst16 = (uint16_t*) dst;

    // There are 80 blocks of 2 pixels horizontally, and 72 of 2 vertically.
    // Each block of 2x2 becomes 3x3.

    uint32_t BlockX, BlockY;
    uint16_t* BlockSrc;
    uint16_t* BlockDst;
    for (BlockY = 0; BlockY < 72; BlockY++)
    {
        BlockSrc = Src16 + BlockY * 160 * 2;
        BlockDst = Dst16 + BlockY * 320 * 3;
        for (BlockX = 0; BlockX < 80; BlockX++)
        {   
            // HORIZONTAL:
            // Before:          After:
            // (a)(b)           (a)(ab)(b)
            //
            //
            // VERTICAL:
            // Before:          After:
            // (a)              (a)
            // (b)              (ab)
            //                  (b)

            // -- Row 1 --
            uint16_t  _1 = *(BlockSrc               );
            *(BlockDst               ) = _1;
            uint16_t  _2 = *(BlockSrc            + 1);
            *(BlockDst            + 1) = Weight1_1( _1,  _2);
            *(BlockDst            + 2) = _2;

            // -- Row 2 --
            uint16_t  _3 = *(BlockSrc + 160 *  1    );
            *(BlockDst + 320 *  1    ) = Weight1_1( _1,  _3);
            uint16_t  _4 = *(BlockSrc + 160 *  1 + 1);
            *(BlockDst + 320 *  1 + 1) = Weight1_1_1_1( _1,  _2,  _3,  _4);
            *(BlockDst + 320 *  1 + 2) = Weight1_1( _2,  _4);

            // -- Row 3 --
            *(BlockDst + 320 *  2    ) = _3;
            *(BlockDst + 320 *  2 + 1) = Weight1_1( _3,  _4);
            *(BlockDst + 320 *  2 + 2) = _4;

            BlockSrc += 2;
            BlockDst += 3;
        }
    }
}

/* Upscales a 160x144 image to 266x240 using a faster resampling algorithm.
 *
 * Input:
 *   src: A packed 160x144 pixel image. The pixel format of this image is RGB 565.
 * Output:
 *   dst: A packed 266x240 pixel image. The pixel format of this image is RGB 565.
 */

void scale166x_fast(uint32_t* dst, uint32_t* src)
{
    uint16_t* Src16 = (uint16_t*) src;
    uint16_t* Dst16 = (uint16_t*) dst;

    // There are 53(+1) blocks of 3 pixels horizontally, and 48 of 3 vertically.
    // Each block of 3x3 becomes 5x5. There is a last column of blocks of 1x3 which becomes 1x5.

    uint32_t BlockX, BlockY;
    uint16_t* BlockSrc;
    uint16_t* BlockDst;
    for (BlockY = 0; BlockY < 48; BlockY++)
    {
        BlockSrc = Src16 + BlockY * 160 * 3;
        BlockDst = Dst16 + BlockY * 320 * 5;
        for (BlockX = 0; BlockX < 54; BlockX++)
        {   
            // HORIZONTAL:
            // Before:          After:
            // (a)(b)(c)          (a)(aab)(b)(bcc)(c)
            //
            // VERTICAL:
            // Before:          After:
            // (a)              (a)
            // (b)              (aab)
            // (c)              (b)
            //                  (bcc)
            //                  (c)

            if(BlockX == 53){
                // -- Row 1 --
                uint16_t  _1 = *(BlockSrc               );
                *(BlockDst               ) = _1;
                // -- Row 2 --
                uint16_t  _2 = *(BlockSrc + 160 *  1    );
                *(BlockDst + 320 *  1    ) = Weight2_1( _1,  _2);
                // -- Row 3 --
                *(BlockDst + 320 *  2    ) = _2;
                // -- Row 4 --
                uint16_t  _3 = *(BlockSrc + 160 *  2    );
                *(BlockDst + 320 *  3    ) = Weight1_2( _2,  _3);
                // -- Row 5 --
                *(BlockDst + 320 *  4    ) = _3;

            } else {
                // -- Row 1 --
                uint16_t  _1 = *(BlockSrc               );
                *(BlockDst               ) = _1;
                uint16_t  _2 = *(BlockSrc            + 1);
                *(BlockDst            + 1) = Weight2_1( _1,  _2);
                *(BlockDst            + 2) = _2;
                uint16_t  _3 = *(BlockSrc            + 2);
                *(BlockDst            + 3) = Weight1_2( _2,  _3);
                *(BlockDst            + 4) = _3;

                // -- Row 2 --
                uint16_t  _4 = *(BlockSrc + 160 *  1    );
                *(BlockDst + 320 *  1    ) = Weight2_1( _1,  _4);
                uint16_t  _5 = *(BlockSrc + 160 *  1 + 1);
                *(BlockDst + 320 *  1 + 1) = Weight2_1(Weight2_1( _1,  _2), Weight2_1( _4,  _5));
                *(BlockDst + 320 *  1 + 2) = Weight2_1( _2,  _5);
                uint16_t  _6 = *(BlockSrc + 160 *  1 + 2);
                *(BlockDst + 320 *  1 + 3) = Weight2_1(Weight1_2( _2,  _3), Weight1_2( _5,  _6));
                *(BlockDst + 320 *  1 + 4) = Weight2_1( _3,  _6);

                // -- Row 3 --
                *(BlockDst + 320 *  2    ) = _4;
                *(BlockDst + 320 *  2 + 1) = Weight2_1( _4,  _5);
                *(BlockDst + 320 *  2 + 2) = _5;
                *(BlockDst + 320 *  2 + 3) = Weight1_2( _5,  _6);
                *(BlockDst + 320 *  2 + 4) = _6;

                // -- Row 4 --
                uint16_t  _7 = *(BlockSrc + 160 *  2    );
                *(BlockDst + 320 *  3    ) = Weight1_2( _4,  _7);
                uint16_t  _8 = *(BlockSrc + 160 *  2 + 1);
                *(BlockDst + 320 *  3 + 1) = Weight1_2(Weight2_1( _4,  _5), Weight2_1( _7,  _8));
                *(BlockDst + 320 *  3 + 2) = Weight1_2( _5,  _8);
                uint16_t  _9 = *(BlockSrc + 160 *  2 + 2);
                *(BlockDst + 320 *  3 + 3) = Weight1_2(Weight1_2( _5,  _6), Weight1_2( _8,  _9));
                *(BlockDst + 320 *  3 + 4) = Weight1_2( _6,  _9);

                // -- Row 5 --
                *(BlockDst + 320 *  4    ) = _7;
                *(BlockDst + 320 *  4 + 1) = Weight2_1( _7,  _8);
                *(BlockDst + 320 *  4 + 2) = _8;
                *(BlockDst + 320 *  4 + 3) = Weight1_2( _8,  _9);
                *(BlockDst + 320 *  4 + 4) = _9;

            }

            BlockSrc += 3;
            BlockDst += 5;
        }
    }
}

/* Upscales a 160x144 image to 266x240 using a pseudo-bilinear resampling algorithm.
 *
 * Input:
 *   src: A packed 160x144 pixel image. The pixel format of this image is RGB 565.
 * Output:
 *   dst: A packed 266x240 pixel image. The pixel format of this image is RGB 565.
 */

void scale166x_pseudobilinear(uint32_t* dst, uint32_t* src)
{
    uint16_t* Src16 = (uint16_t*) src;
    uint16_t* Dst16 = (uint16_t*) dst;

    // There are 53(+1) blocks of 3 pixels horizontally, and 48 of 3 vertically.
    // Each block of 3x3 becomes 5x5. There is a last column of blocks of 1x3 which becomes 1x5.

    uint32_t BlockX, BlockY;
    uint16_t* BlockSrc;
    uint16_t* BlockDst;
    for (BlockY = 0; BlockY < 48; BlockY++)
    {
        BlockSrc = Src16 + BlockY * 160 * 3;
        BlockDst = Dst16 + BlockY * 320 * 5;
        for (BlockX = 0; BlockX < 54; BlockX++)
        {   
            // HORIZONTAL:
            // Before:      After:
            // (a)(b)(c)--->(d)   (a)(aabbb)(bbbbc)(bcccc)(cccdd)
            //
            //
            // VERTICAL:
            // Before:      After:
            // (a)          (a)
            // (b)          (aabbb)
            // (c)          (bbbbc)
            //  |           (bcccc)
            //  V           (cccdd)
            // (d)

            if((BlockX == 53) && (BlockY == 47)){
                // -- Row 1 --
                uint16_t  _1 = *(BlockSrc               );
                *(BlockDst               ) = _1;
                // -- Row 2 --
                uint16_t  _2 = *(BlockSrc + 160 *  1    );
                *(BlockDst + 320 *  1    ) = Weight2_3( _1,  _2);
                // -- Row 3 --
                uint16_t  _3 = *(BlockSrc + 160 *  2    );
                *(BlockDst + 320 *  2    ) = Weight4_1( _2,  _3);
                // -- Row 4 --
                *(BlockDst + 320 *  3    ) = Weight1_4( _2,  _3);
                // -- Row 5 --
                *(BlockDst + 320 *  4    ) = _3;

            } else if(BlockX == 53){
                // -- Row 1 --
                uint16_t  _1 = *(BlockSrc               );
                *(BlockDst               ) = _1;
                // -- Row 2 --
                uint16_t  _2 = *(BlockSrc + 160 *  1    );
                *(BlockDst + 320 *  1    ) = Weight2_3( _1,  _2);
                // -- Row 3 --
                uint16_t  _3 = *(BlockSrc + 160 *  2    );
                *(BlockDst + 320 *  2    ) = Weight4_1( _2,  _3);
                // -- Row 4 --
                *(BlockDst + 320 *  3    ) = Weight1_4( _2,  _3);
                // -- Row 5 --
                uint16_t  _4 = *(BlockSrc + 160 *  3    );
                *(BlockDst + 320 *  4    ) = Weight3_2( _3,  _4);

            } else if(BlockY == 47){
                // -- Row 1 --;
                uint16_t  _1 = *(BlockSrc               );
                *(BlockDst               ) = _1;
                uint16_t  _2 = *(BlockSrc            + 1);
                *(BlockDst            + 1) = Weight2_3( _1,  _2);
                uint16_t  _3 = *(BlockSrc            + 2);
                *(BlockDst            + 2) = Weight4_1( _2,  _3);
                *(BlockDst            + 3) = Weight1_4( _2,  _3);
                uint16_t  _4 = *(BlockSrc            + 3);
                *(BlockDst            + 4) = Weight3_2( _3,  _4);

                // -- Row 2 --
                uint16_t  _5 = *(BlockSrc + 160 *  1    );
                *(BlockDst + 320 *  1    ) = Weight2_3( _1,  _5);
                uint16_t  _6 = *(BlockSrc + 160 *  1 + 1);
                *(BlockDst + 320 *  1 + 1) = Weight2_3(Weight2_3( _1,  _2), Weight2_3( _5,  _6));
                uint16_t  _7 = *(BlockSrc + 160 *  1 + 2);
                *(BlockDst + 320 *  1 + 2) = Weight2_3(Weight4_1( _2,  _3), Weight4_1( _6,  _7));
                *(BlockDst + 320 *  1 + 3) = Weight2_3(Weight1_4( _2,  _3), Weight1_4( _6,  _7));
                uint16_t  _8 = *(BlockSrc + 160 *  1 + 3);
                *(BlockDst + 320 *  1 + 4) = Weight2_3(Weight3_2( _3,  _4), Weight3_2( _7,  _8));

                // -- Row 3 --
                uint16_t  _9 = *(BlockSrc + 160 *  2    );
                *(BlockDst + 320 *  2    ) = Weight4_1( _5,  _9);
                uint16_t _10 = *(BlockSrc + 160 *  2 + 1);
                *(BlockDst + 320 *  2 + 1) = Weight4_1(Weight2_3( _5,  _6), Weight2_3( _9, _10));
                uint16_t _11 = *(BlockSrc + 160 *  2 + 2);
                *(BlockDst + 320 *  2 + 2) = Weight4_1(Weight4_1( _6,  _7), Weight4_1(_10, _11));
                *(BlockDst + 320 *  2 + 3) = Weight4_1(Weight1_4( _6,  _7), Weight1_4(_10, _11));
                uint16_t _12 = *(BlockSrc + 160 *  2 + 3);
                *(BlockDst + 320 *  2 + 4) = Weight4_1(Weight3_2( _7,  _8), Weight3_2(_11, _12));

                // -- Row 4 --
                *(BlockDst + 320 *  3    ) = Weight1_4( _5,  _9);
                *(BlockDst + 320 *  3 + 1) = Weight1_4(Weight2_3( _5,  _6), Weight2_3( _9, _10));
                *(BlockDst + 320 *  3 + 2) = Weight1_4(Weight4_1( _6,  _7), Weight4_1(_10, _11));
                *(BlockDst + 320 *  3 + 3) = Weight1_4(Weight1_4( _6,  _7), Weight1_4(_10, _11));
                *(BlockDst + 320 *  3 + 4) = Weight1_4(Weight3_2( _7,  _8), Weight3_2(_11, _12));

                // -- Row 5 --
                *(BlockDst + 320 *  4    ) = _9;
                *(BlockDst + 320 *  4 + 1) = Weight2_3( _9, _10);
                *(BlockDst + 320 *  4 + 2) = Weight4_1(_10, _11);
                *(BlockDst + 320 *  4 + 3) = Weight1_4(_10, _11);
                *(BlockDst + 320 *  4 + 4) = Weight3_2(_11, _12);

            } else {
                // -- Row 1 --;
                uint16_t  _1 = *(BlockSrc               );
                *(BlockDst               ) = _1;
                uint16_t  _2 = *(BlockSrc            + 1);
                *(BlockDst            + 1) = Weight2_3( _1,  _2);
                uint16_t  _3 = *(BlockSrc            + 2);
                *(BlockDst            + 2) = Weight4_1( _2,  _3);
                *(BlockDst            + 3) = Weight1_4( _2,  _3);
                uint16_t  _4 = *(BlockSrc            + 3);
                *(BlockDst            + 4) = Weight3_2( _3,  _4);

                // -- Row 2 --
                uint16_t  _5 = *(BlockSrc + 160 *  1    );
                *(BlockDst + 320 *  1    ) = Weight2_3( _1,  _5);
                uint16_t  _6 = *(BlockSrc + 160 *  1 + 1);
                *(BlockDst + 320 *  1 + 1) = Weight2_3(Weight2_3( _1,  _2), Weight2_3( _5,  _6));
                uint16_t  _7 = *(BlockSrc + 160 *  1 + 2);
                *(BlockDst + 320 *  1 + 2) = Weight2_3(Weight4_1( _2,  _3), Weight4_1( _6,  _7));
                *(BlockDst + 320 *  1 + 3) = Weight2_3(Weight1_4( _2,  _3), Weight1_4( _6,  _7));
                uint16_t  _8 = *(BlockSrc + 160 *  1 + 3);
                *(BlockDst + 320 *  1 + 4) = Weight2_3(Weight3_2( _3,  _4), Weight3_2( _7,  _8));

                // -- Row 3 --
                uint16_t  _9 = *(BlockSrc + 160 *  2    );
                *(BlockDst + 320 *  2    ) = Weight4_1( _5,  _9);
                uint16_t _10 = *(BlockSrc + 160 *  2 + 1);
                *(BlockDst + 320 *  2 + 1) = Weight4_1(Weight2_3( _5,  _6), Weight2_3( _9, _10));
                uint16_t _11 = *(BlockSrc + 160 *  2 + 2);
                *(BlockDst + 320 *  2 + 2) = Weight4_1(Weight4_1( _6,  _7), Weight4_1(_10, _11));
                *(BlockDst + 320 *  2 + 3) = Weight4_1(Weight1_4( _6,  _7), Weight1_4(_10, _11));
                uint16_t _12 = *(BlockSrc + 160 *  2 + 3);
                *(BlockDst + 320 *  2 + 4) = Weight4_1(Weight3_2( _7,  _8), Weight3_2(_11, _12));

                // -- Row 4 --
                *(BlockDst + 320 *  3    ) = Weight1_4( _5,  _9);
                *(BlockDst + 320 *  3 + 1) = Weight1_4(Weight2_3( _5,  _6), Weight2_3( _9, _10));
                *(BlockDst + 320 *  3 + 2) = Weight1_4(Weight4_1( _6,  _7), Weight4_1(_10, _11));
                *(BlockDst + 320 *  3 + 3) = Weight1_4(Weight1_4( _6,  _7), Weight1_4(_10, _11));
                *(BlockDst + 320 *  3 + 4) = Weight1_4(Weight3_2( _7,  _8), Weight3_2(_11, _12));

                // -- Row 5 --
                uint16_t _13 = *(BlockSrc + 160 *  3    );
                *(BlockDst + 320 *  4    ) = Weight3_2( _9, _13);
                uint16_t _14 = *(BlockSrc + 160 *  3 + 1);
                *(BlockDst + 320 *  4 + 1) = Weight3_2(Weight2_3( _9, _10), Weight2_3(_13, _14));
                uint16_t _15 = *(BlockSrc + 160 *  3 + 2);
                *(BlockDst + 320 *  4 + 2) = Weight3_2(Weight4_1(_10, _11), Weight4_1(_14, _15));
                *(BlockDst + 320 *  4 + 3) = Weight3_2(Weight1_4(_10, _11), Weight1_4(_14, _15));
                uint16_t _16 = *(BlockSrc + 160 *  3 + 3);
                *(BlockDst + 320 *  4 + 4) = Weight3_2(Weight3_2(_11, _12), Weight3_2(_15, _16));

            }

            BlockSrc += 3;
            BlockDst += 5;
        }
    }
}

#ifdef VGA_SCREEN

uint16_t hexcolor_to_rgb565(const uint32_t color)
{
    uint8_t colorr = ((color >> 16) & 0xFF);
    uint8_t colorg = ((color >> 8) & 0xFF);
    uint8_t colorb = ((color) & 0xFF);

    uint16_t r = ((colorr >> 3) & 0x1f) << 11;
    uint16_t g = ((colorg >> 2) & 0x3f) << 5;
    uint16_t b = (colorb >> 3) & 0x1f;

    return (uint16_t) (r | g | b);
}

/* Upscales a 160x144 image to 480x432 using a grid-looking upscaler algorithm.
 *
 * Input:
 *   src: A packed 160x144 pixel image. The pixel format of this image is RGB 565.
 *   gridcolor: An hexadecimal color. The format of this color is 0xRRGGBB.
 * Output:
 *   dst: A packed 480x432 pixel image. The pixel format of this image is RGB 565.
 */

void scale3x_dotmatrix(uint32_t* dst, uint32_t* src, const uint32_t gridcolor)
{
    uint16_t* Src16 = (uint16_t*) src;
    uint16_t* Dst16 = (uint16_t*) dst;
    uint16_t gcolor = hexcolor_to_rgb565(gridcolor);

    // There are 160 pixels horizontally, and 144 vertically.
    // Each pixel becomes 3x3 with an added grid pattern.

    uint32_t BlockX, BlockY;
    uint16_t* BlockSrc;
    uint16_t* BlockDst;
    for (BlockY = 0; BlockY < 144; BlockY++)
    {
        BlockSrc = Src16 + BlockY * 160 * 1;
        BlockDst = Dst16 + BlockY * 640 * 3;
        for (BlockX = 0; BlockX < 160; BlockX++)
        {
            // Before:          After:
            // (a)              (x)(a)(a)
            //                  (x)(a)(a)
            //                  (x)(x)(x)

            uint16_t  _1 = *(BlockSrc);
            uint16_t  _2 = Weight3_2( _1, gcolor);

            // -- Row 1 --
            *(BlockDst               ) = _2;
            *(BlockDst            + 1) = _1;
            *(BlockDst            + 2) = _1;

            // -- Row 2 --
            *(BlockDst + 640 *  1    ) = _2;
            *(BlockDst + 640 *  1 + 1) = _1;
            *(BlockDst + 640 *  1 + 2) = _1;

            // -- Row 3 --
            *(BlockDst + 640 *  2    ) = _2;
            *(BlockDst + 640 *  2 + 1) = _2;
            *(BlockDst + 640 *  2 + 2) = _2;

            BlockSrc += 1;
            BlockDst += 3;
        }
    }
}

/* Upscales a 160x144 image to 480x432 using a CRT-looking upscaler algorithm.
 *
 * Input:
 *   src: A packed 160x144 pixel image. The pixel format of this image is RGB 565.
 *   gridcolor: An hexadecimal color. The format of this color is 0xRRGGBB.
 * Output:
 *   dst: A packed 480x432 pixel image. The pixel format of this image is RGB 565.
 */

void scale3x_crt(uint32_t* dst, uint32_t* src)
{
    uint16_t* Src16 = (uint16_t*) src;
    uint16_t* Dst16 = (uint16_t*) dst;
    uint16_t gcolor = hexcolor_to_rgb565(0x000000);

    // There are 160 pixels horizontally, and 144 vertically.
    // Each pixel becomes 3x3 with an added scanline pattern.

    uint32_t BlockX, BlockY;
    uint16_t* BlockSrc;
    uint16_t* BlockDst;
    for (BlockY = 0; BlockY < 144; BlockY++)
    {
        BlockSrc = Src16 + BlockY * 160 * 1;
        BlockDst = Dst16 + BlockY * 640 * 3;
        for (BlockX = 0; BlockX < 160; BlockX++)
        {
            // Before:          After:
            // (a)              (a)(a)(a)
            //                  (x)(x)(x)
            //                  (y)(y)(y)

            uint16_t  _1 = *(BlockSrc);
            uint16_t  _2 = Weight4_1( _1, gcolor);
            uint16_t  _3 = Weight1_1( _1, gcolor);

            // -- Row 1 --
            *(BlockDst               ) = _1;
            *(BlockDst            + 1) = _1;
            *(BlockDst            + 2) = _1;

            // -- Row 2 --
            *(BlockDst + 640 *  1    ) = _2;
            *(BlockDst + 640 *  1 + 1) = _2;
            *(BlockDst + 640 *  1 + 2) = _2;

            // -- Row 3 --
            *(BlockDst + 640 *  2    ) = _3;
            *(BlockDst + 640 *  2 + 1) = _3;
            *(BlockDst + 640 *  2 + 2) = _3;

            BlockSrc += 1;
            BlockDst += 3;
        }
    }
}

void fullscreen_crt(uint32_t* dst, uint32_t* src)
{
    uint16_t* Src16 = (uint16_t*) src;
    uint16_t* Dst16 = (uint16_t*) dst;
    uint16_t gcolor = hexcolor_to_rgb565(0x000000);

    // There are 160 pixels horizontally, and 144 vertically.
    // Each pixel becomes 3x3 with an added scanline pattern.

    uint32_t BlockX, BlockY;
    uint16_t* BlockSrc;
    uint16_t* BlockDst;
    for (BlockY = 0; BlockY < 144; BlockY++)
    {
        BlockSrc = Src16 + BlockY * 160 * 1;
        BlockDst = Dst16 + BlockY * 480 * 3;
        for (BlockX = 0; BlockX < 160; BlockX++)
        {
            // Before:          After:
            // (a)              (a)(a)(a)
            //                  (x)(x)(x)
            //                  (y)(y)(y)

            uint16_t  _1 = *(BlockSrc);
            uint16_t  _2 = Weight4_1( _1, gcolor);
            uint16_t  _3 = Weight1_1( _1, gcolor);

            // -- Row 1 --
            *(BlockDst               ) = _1;
            *(BlockDst            + 1) = _1;
            *(BlockDst            + 2) = _1;

            // -- Row 2 --
            *(BlockDst + 480 *  1    ) = _2;
            *(BlockDst + 480 *  1 + 1) = _2;
            *(BlockDst + 480 *  1 + 2) = _2;

            // -- Row 3 --
            *(BlockDst + 480 *  2    ) = _3;
            *(BlockDst + 480 *  2 + 1) = _3;
            *(BlockDst + 480 *  2 + 2) = _3;

            BlockSrc += 1;
            BlockDst += 3;
        }
    }
}

void scaleborder3x(uint32_t* dst, uint32_t* src)
{
    uint16_t* Src16 = (uint16_t*) src;
    uint16_t* Dst16 = (uint16_t*) dst;

    uint32_t BlockX, BlockY;
    uint16_t* BlockSrc;
    uint16_t* BlockDst;
    for (BlockY = 0; BlockY < 160; BlockY++)
    {
        BlockSrc = Src16 + BlockY * 212 * 1;
        BlockDst = Dst16 + BlockY * 640 * 3;
        for (BlockX = 0; BlockX < 212; BlockX++)
        {
            // Before:          After:
            // (a)              (a)(a)(a)
            //                  (a)(a)(a)
            //                  (a)(a)(a)

            uint16_t  _1 = *(BlockSrc);

            // -- Row 1 --
            *(BlockDst               ) = _1;
            *(BlockDst            + 1) = _1;
            *(BlockDst            + 2) = _1;

            // -- Row 2 --
            *(BlockDst + 640 *  1    ) = _1;
            *(BlockDst + 640 *  1 + 1) = _1;
            *(BlockDst + 640 *  1 + 2) = _1;

            // -- Row 3 --
            *(BlockDst + 640 *  2    ) = _1;
            *(BlockDst + 640 *  2 + 1) = _1;
            *(BlockDst + 640 *  2 + 2) = _1;

            BlockSrc += 1;
            BlockDst += 3;
        }
    }
}

void scaleborder3x_crt(uint32_t* dst, uint32_t* src)
{
    uint16_t* Src16 = (uint16_t*) src;
    uint16_t* Dst16 = (uint16_t*) dst;
    uint16_t gcolor = hexcolor_to_rgb565(0x000000);

    uint32_t BlockX, BlockY;
    uint16_t* BlockSrc;
    uint16_t* BlockDst;
    for (BlockY = 0; BlockY < 160; BlockY++)
    {
        BlockSrc = Src16 + BlockY * 212 * 1;
        BlockDst = Dst16 + BlockY * 640 * 3;
        for (BlockX = 0; BlockX < 212; BlockX++)
        {
            // Before:          After:
            // (a)              (a)(a)(a)
            //                  (x)(x)(x)
            //                  (y)(y)(y)

            uint16_t  _1 = *(BlockSrc);
            uint16_t  _2 = Weight4_1( _1, gcolor);
            uint16_t  _3 = Weight1_1( _1, gcolor);

            // -- Row 1 --
            *(BlockDst               ) = _1;
            *(BlockDst            + 1) = _1;
            *(BlockDst            + 2) = _1;

            // -- Row 2 --
            *(BlockDst + 640 *  1    ) = _2;
            *(BlockDst + 640 *  1 + 1) = _2;
            *(BlockDst + 640 *  1 + 2) = _2;

            // -- Row 3 --
            *(BlockDst + 640 *  2    ) = _3;
            *(BlockDst + 640 *  2 + 1) = _3;
            *(BlockDst + 640 *  2 + 2) = _3;

            BlockSrc += 1;
            BlockDst += 3;
        }
    }
}
#endif

//---------------- BORDER SCALERS ---------------------//

void scaleborder15x(uint32_t* dst, uint32_t* src)
{
    uint16_t* Src16 = (uint16_t*) src;
    uint16_t* Dst16 = (uint16_t*) dst;

    uint32_t BlockX, BlockY;
    uint16_t* BlockSrc;
    uint16_t* BlockDst;
    for (BlockY = 0; BlockY < 80; BlockY++)
    {
        BlockSrc = Src16 + BlockY * 212 * 2;
        BlockDst = Dst16 + BlockY * 320 * 3;
        for (BlockX = 0; BlockX < 106; BlockX++)
        {   
            // -- Row 1 --
            uint16_t  _1 = *(BlockSrc               );
            *(BlockDst               ) = _1;
            uint16_t  _2 = *(BlockSrc            + 1);
            *(BlockDst            + 1) = Weight1_1( _1,  _2);
            *(BlockDst            + 2) = _2;

            // -- Row 2 --
            uint16_t  _3 = *(BlockSrc + 212 *  1    );
            *(BlockDst + 320 *  1    ) = Weight1_1( _1,  _3);
            uint16_t  _4 = *(BlockSrc + 212 *  1 + 1);
            *(BlockDst + 320 *  1 + 1) = Weight1_1_1_1( _1,  _2,  _3,  _4);
            *(BlockDst + 320 *  1 + 2) = Weight1_1( _2,  _4);

            // -- Row 3 --
            *(BlockDst + 320 *  2    ) = _3;
            *(BlockDst + 320 *  2 + 1) = Weight1_1( _3,  _4);
            *(BlockDst + 320 *  2 + 2) = _4;

            BlockSrc += 2;
            BlockDst += 3;
        }
    }
}

void scaleborder166x(uint32_t* dst, uint32_t* src)
{
    uint16_t* Src16 = (uint16_t*) src;
    uint16_t* Dst16 = (uint16_t*) dst;

    uint32_t BlockX, BlockY;
    uint16_t* BlockSrc;
    uint16_t* BlockDst;
    for (BlockY = 0; BlockY < 48; BlockY++)
    {
        BlockSrc = Src16 + BlockY * 192 * 3;
        BlockDst = Dst16 + BlockY * 320 * 5;
        for (BlockX = 0; BlockX < 64; BlockX++)
        {   
            if(BlockX < 8){
                // -- Row 1 --
                uint16_t  _1 = *(BlockSrc               );
                *(BlockDst            + 1) = _1;
                uint16_t  _2 = *(BlockSrc            + 1);
                *(BlockDst            + 2) = _2;
                uint16_t  _3 = *(BlockSrc            + 2);
                *(BlockDst            + 3) = Weight2_1( _2,  _3);
                *(BlockDst            + 4) = _3;
                uint16_t  _A = *(BlockSrc            + 3);
                *(BlockDst            + 5) = Weight1_2( _3,  _A);

                // -- Row 2 --
                uint16_t  _4 = *(BlockSrc + 192 *  1    );
                *(BlockDst + 320 *  1 + 1) = Weight2_1( _1,  _4);
                uint16_t  _5 = *(BlockSrc + 192 *  1 + 1);
                *(BlockDst + 320 *  1 + 2) = Weight2_1( _2,  _5);
                uint16_t  _6 = *(BlockSrc + 192 *  1 + 2);
                *(BlockDst + 320 *  1 + 3) = Weight2_1(Weight2_1( _2,  _3), Weight2_1( _5,  _6));
                *(BlockDst + 320 *  1 + 4) = Weight2_1( _3,  _6);
                uint16_t  _B = *(BlockSrc + 192 *  1 + 3);
                *(BlockDst + 320 *  1 + 5) = Weight2_1(Weight1_2( _3,  _A), Weight1_2( _6,  _B));

                // -- Row 3 --
                *(BlockDst + 320 *  2 + 1) = _4;
                *(BlockDst + 320 *  2 + 2) = _5;
                *(BlockDst + 320 *  2 + 3) = Weight2_1( _5,  _6);
                *(BlockDst + 320 *  2 + 4) = _6;
                *(BlockDst + 320 *  2 + 5) = Weight1_2( _6,  _B);

                // -- Row 4 --
                uint16_t  _7 = *(BlockSrc + 192 *  2    );
                *(BlockDst + 320 *  3 + 1) = Weight1_2( _4,  _7);
                uint16_t  _8 = *(BlockSrc + 192 *  2 + 1);
                *(BlockDst + 320 *  3 + 2) = Weight1_2( _5,  _8);
                uint16_t  _9 = *(BlockSrc + 192 *  2 + 2);
                *(BlockDst + 320 *  3 + 3) = Weight1_2(Weight2_1( _5,  _6), Weight2_1( _8,  _9));
                *(BlockDst + 320 *  3 + 4) = Weight1_2( _6,  _9);
                uint16_t  _C = *(BlockSrc + 192 *  2 + 3);
                *(BlockDst + 320 *  3 + 5) = Weight1_2(Weight1_2( _6,  _B), Weight1_2( _9,  _C));

                // -- Row 5 --
                *(BlockDst + 320 *  4 + 1) = _7;
                *(BlockDst + 320 *  4 + 2) = _8;
                *(BlockDst + 320 *  4 + 3) = Weight2_1( _8,  _9);
                *(BlockDst + 320 *  4 + 4) = _9;
                *(BlockDst + 320 *  4 + 5) = Weight1_2( _9,  _C);
                
            } else if (BlockX > 55){
                // -- Row 1 --
                uint16_t  _1 = *(BlockSrc               );
                *(BlockDst               ) = _1;
                uint16_t  _2 = *(BlockSrc            + 1);
                *(BlockDst            + 1) = Weight1_2( _1,  _2);
                *(BlockDst            + 2) = _2;
                uint16_t  _3 = *(BlockSrc            + 2);
                *(BlockDst            + 3) = _3;
                uint16_t  _A = *(BlockSrc            + 3);
                *(BlockDst            + 4) = Weight2_1( _3,  _A);

                // -- Row 2 --
                uint16_t  _4 = *(BlockSrc + 192 *  1    );
                *(BlockDst + 320 *  1    ) = Weight2_1( _1,  _4);
                uint16_t  _5 = *(BlockSrc + 192 *  1 + 1);
                *(BlockDst + 320 *  1 + 1) = Weight2_1(Weight1_2( _1,  _2), Weight1_2( _4,  _5));
                *(BlockDst + 320 *  1 + 2) = Weight2_1( _2,  _5);
                uint16_t  _6 = *(BlockSrc + 192 *  1 + 2);
                *(BlockDst + 320 *  1 + 3) = Weight2_1( _3,  _6);
                uint16_t  _B = *(BlockSrc + 192 *  1 + 3);
                *(BlockDst + 320 *  1 + 4) = Weight2_1(Weight2_1( _3,  _A), Weight2_1( _6,  _B));

                // -- Row 3 --
                *(BlockDst + 320 *  2    ) = _4;
                *(BlockDst + 320 *  2 + 1) = Weight1_2( _4,  _5);
                *(BlockDst + 320 *  2 + 2) = _5;
                *(BlockDst + 320 *  2 + 3) = _6;
                *(BlockDst + 320 *  2 + 4) = Weight2_1( _6,  _B);

                // -- Row 4 --
                uint16_t  _7 = *(BlockSrc + 192 *  2    );
                *(BlockDst + 320 *  3    ) = Weight1_2( _4,  _7);
                uint16_t  _8 = *(BlockSrc + 192 *  2 + 1);
                *(BlockDst + 320 *  3 + 1) = Weight1_2(Weight1_2( _4,  _5), Weight1_2( _7,  _8));
                *(BlockDst + 320 *  3 + 2) = Weight1_2( _5,  _8);
                uint16_t  _9 = *(BlockSrc + 192 *  2 + 2);
                *(BlockDst + 320 *  3 + 3) = Weight1_2( _6,  _9);
                uint16_t  _C = *(BlockSrc + 192 *  2 + 3);
                *(BlockDst + 320 *  3 + 4) = Weight1_2(Weight2_1( _6,  _B), Weight2_1( _9,  _C));

                // -- Row 5 --
                *(BlockDst + 320 *  4    ) = _7;
                *(BlockDst + 320 *  4 + 1) = Weight1_2( _7,  _8);
                *(BlockDst + 320 *  4 + 2) = _8;
                *(BlockDst + 320 *  4 + 3) = _9;
                *(BlockDst + 320 *  4 + 4) = Weight2_1( _9,  _C);

            } 

            BlockSrc += 3;
            BlockDst += 5;
        }
    }
}