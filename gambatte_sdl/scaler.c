
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



