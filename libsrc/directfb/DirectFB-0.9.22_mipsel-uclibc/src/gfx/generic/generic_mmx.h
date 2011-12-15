/*
   (c) Copyright 2000-2002  convergence integrated media GmbH.
   (c) Copyright 2002-2004  convergence GmbH.

   All rights reserved.

   Written by Denis Oliver Kropp <dok@directfb.org>,
              Andreas Hundt <andi@fischlustig.de>,
              Sven Neumann <neo@directfb.org> and
              Ville Syrjl <syrjala@sci.fi>.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifdef __GNUC__
# define __aligned( n )  __attribute__ ((aligned((n))))
#else
# define __aligned( n )
#endif


static void SCacc_add_to_Dacc_MMX( GenefxState *gfxs )
{
     __asm__ __volatile__ (
               "    movq     %2, %%mm0\n"
               ".align   16\n"
               "1:\n"
               "    movq     (%0), %%mm1\n"
               "    paddw    %%mm0, %%mm1\n"
               "    movq     %%mm1, (%0)\n"
               "    add      $8, %0\n"
               "    dec      %1\n"
               "    jnz      1b\n"
               "    emms"
               : /* no outputs */
               : "D" (gfxs->Dacc), "c" (gfxs->length), "m" (gfxs->SCacc)
               : "%st", "memory");
}

static void Dacc_modulate_argb_MMX( GenefxState *gfxs )
{
     __asm__ __volatile__ (
               "movq     %2, %%mm0\n\t"
               ".align   16\n"
               "1:\n\t"
               "testw    $0xF000, 6(%0)\n\t"
               "jnz      2f\n\t"
               "movq     (%0), %%mm1\n\t"
               "pmullw   %%mm0, %%mm1\n\t"
               "psrlw    $8, %%mm1\n\t"
               "movq     %%mm1, (%0)\n"
               ".align   16\n"
               "2:\n\t"
               "add      $8, %0\n\t"
               "dec      %1\n\t"
               "jnz      1b\n\t"
               "emms"
               : /* no outputs */
               : "D" (gfxs->Dacc), "c" (gfxs->length), "m" (gfxs->Cacc)
               : "%st", "memory");
}

static void Sacc_add_to_Dacc_MMX( GenefxState *gfxs )
{
     __asm__ __volatile__ (
               ".align   16\n"
               "1:\n\t"
               "movq     (%2), %%mm0\n\t"
               "movq     (%0), %%mm1\n\t"
               "paddw    %%mm1, %%mm0\n\t"
               "movq     %%mm0, (%0)\n\t"
               "add      $8, %0\n\t"
               "add      $8, %2\n\t"
               "dec      %1\n\t"
               "jnz      1b\n\t"
               "emms"
               : /* no outputs */
               : "D" (gfxs->Dacc), "c" (gfxs->length), "S" (gfxs->Sacc)
               : "%st", "memory");
}

static void Sacc_to_Aop_rgb16_MMX( GenefxState *gfxs )
{
     static const long preload[] = { 0xFF00FF00, 0x0000FF00 };
     static const long mask[]    = { 0x00FC00F8, 0x000000F8 };
     static const long pm[]      = { 0x01000004, 0x00000004 };

     __asm__ __volatile__ (
	       "movq     %3, %%mm7\n\t"
	       "movq     %4, %%mm5\n\t"
	       "movq     %5, %%mm4\n\t"
               ".align   16\n"
               "1:\n\t"
               "testw    $0xF000, 6(%2)\n\t"
               "jnz      2f\n\t"
               "movq     (%2), %%mm0\n\t"
               "paddusw  %%mm7, %%mm0\n\t"
               "pand     %%mm5, %%mm0\n\t"
               "pmaddwd  %%mm4, %%mm0\n\t"
               "psrlq    $5, %%mm0\n\t"
               "movq     %%mm0, %%mm1\n\t"
               "psrlq    $21, %%mm0\n\t"
               "por      %%mm1, %%mm0\n\t"
               "movd     %%mm0, %%eax\n\t"
               "movw     %%ax, (%0)\n\t"
               ".align 16\n"
               "2:\n\t"
               "add      $8, %2\n\t"
	       "add      $2, %0\n\t"
               "dec      %1\n\t"
               "jnz      1b\n\t"
               "emms"
               : /* no outputs */
               : "D" (gfxs->Aop), "c" (gfxs->length), "S" (gfxs->Sacc),
                 "m" (*preload), "m" (*mask), "m" (*pm)
               : "%eax", "%st", "memory");
}

static void Sacc_to_Aop_rgb32_MMX( GenefxState *gfxs )
{
     static const long preload[]  = { 0xFF00FF00, 0x0000FF00 };
     static const long postload[] = { 0x00FF00FF, 0x000000FF };
     static const long pm[]       = { 0x01000001, 0x00000001 };

     __asm__ __volatile__ (
	       "movq     %3, %%mm1\n\t"
	       "movq     %4, %%mm2\n\t"
	       "movq     %5, %%mm3\n\t"
               ".align   16\n"
               "1:\n\t"
               "testw    $0xF000, 6(%2)\n\t"
               "jnz      2f\n\t"
               "movq     (%2), %%mm0\n\t"
               "paddusw  %%mm1, %%mm0\n\t"
               "pand     %%mm2, %%mm0\n\t"
               "pmaddwd  %%mm3, %%mm0\n\t"
               "movq     %%mm0, %%mm4\n\t"
               "psrlq    $16, %%mm0\n\t"
               "por      %%mm0, %%mm4\n\t"
               "movd     %%mm4, (%0)\n\t"
               ".align 16\n"
               "2:\n\t"
               "add      $8, %2\n\t"
	       "add      $4, %0\n\t"
               "dec      %1\n\t"
               "jnz      1b\n\t"
               "emms"
               : /* no outputs */
               : "D" (gfxs->Aop), "c" (gfxs->length), "S" (gfxs->Sacc),
                 "m" (*preload), "m" (*postload), "m" (*pm)
               : "%st", "memory");
}

__attribute__((no_instrument_function))
static void Sop_argb_Sto_Dacc_MMX( GenefxState *gfxs )
{
     static const long zeros[]  = { 0, 0 };
     int i = 0;

     __asm__ __volatile__ (
	       "movq     %5, %%mm0\n\t"
               ".align   16\n"
               "1:\n\t"
               "movd     (%3), %%mm1\n\t"
               "punpcklbw %%mm0, %%mm1\n\t"
               ".align   16\n"
               "2:\n\t"
               "movq     %%mm1, (%1)\n\t"
               "dec      %2\n\t"
               "jz       3f\n\t"
               "addl     $8, %1\n\t"
               "addl     %4, %0\n\t"
               "testl    $0xFFFF0000, %0\n\t"
               "jz       2b\n\t"
               "movl     %0, %%edx\n\t"
               "andl     $0xFFFF0000, %%edx\n\t"
               "shrl     $14, %%edx\n\t"
               "add      %%edx, %3\n\t"
               "andl     $0xFFFF, %0\n\t"
               "jmp      1b\n"
               "3:\n\t"
               "emms"
               : "=r" (i)
               : "D" (gfxs->Dacc), "c" (gfxs->length), "S" (gfxs->Sop),
                 "a" (gfxs->SperD), "m" (*zeros), "0" (i)
               : "%edx", "%st", "memory");
}

static void Sop_argb_to_Dacc_MMX( GenefxState *gfxs )
{
     static const long zeros[]  = { 0, 0 };

     __asm__ __volatile__ (
	       "movq     %3, %%mm0\n\t"
               ".align   16\n"
               "1:\n\t"
               "movd     (%2), %%mm1\n\t"
               "punpcklbw %%mm0, %%mm1\n\t"
               "movq     %%mm1, (%0)\n\t"
               "addl     $4, %2\n\t"
               "addl     $8, %0\n\t"
               "dec      %1\n\t"
               "jnz      1b\n\t"
               "emms"
               : /* no outputs */
               : "D" (gfxs->Dacc), "c" (gfxs->length),
                 "S" (gfxs->Sop), "m" (*zeros)
               : "%st", "memory");
}

static void Sop_yuy2_to_Dacc_MMX( GenefxState *gfxs )
{
     static __u16 __aligned(8) mask[]  = {  0xFFFF,       0,  0xFFFF,       0 };
     static __u16 __aligned(8) sub[]   = {      16,     128,      16,     128 };   
     static __s16 __aligned(8) mul0[]  = {  0x253F,  0x3312,  0x253F,  0x4093 };
     static __s16 __aligned(8) mul1[]  = { -0x0C83, -0x1A04, -0x0C83, -0x1A04 };
     static __u16 __aligned(8) alpha[] = {  0xFF00,  0xFF00,  0xFF00,  0xFF00 };
     
     __asm__ __volatile__ (
               "shrl      $1, %1\n\t"       // gfx->length /= 2
               "jz        2f\n\t"           // return if 0
               "pxor      %%mm7, %%mm7\n\t"
               ".align 16\n"
               "1:\n\t"
               "movd       (%2), %%mm0\n\t" // 00 00 00 00 cr y1 cb y0
               "punpcklbw %%mm7, %%mm0\n\t" // 00 cr 00 y1 00 cb 00 y0
               "psubw        %3, %%mm0\n\t" // luma -= 16, chroma -= 128 
               "psllw        $3, %%mm0\n\t" // precision
               "movq      %%mm0, %%mm1\n\t" // save
                
               "pmulhw       %5, %%mm0\n\t" // 00 vr 00 y1 00 ub 00 y0
               "movq      %%mm0, %%mm2\n\t"
               "pand         %4, %%mm2\n\t" // 00 00 00 y1 00 00 00 y0
               "movq      %%mm2, %%mm3\n\t"
               "pslld       $16, %%mm3\n\t" // 00 y1 00 00 00 y0 00 00
               "por       %%mm3, %%mm2\n\t" // 00 y1 00 y1 00 y0 00 y0
               "psrad       $16, %%mm0\n\t" // 00 00 00 vr 00 00 00 ub
               "packssdw  %%mm0, %%mm0\n\t" // 00 vr 00 ub 00 vr 00 ub
               "paddw     %%mm2, %%mm0\n\t" // 00 r1 00 b1 00 r0 00 b0
               "packuswb  %%mm0, %%mm0\n\t" // r1 b1 r0 b0 r1 b1 r0 b0
               
               "psrad       $16, %%mm1\n\t" // 00 00 00 cr 00 00 00 cb
               "packssdw  %%mm1, %%mm1\n\t" // 00 cr 00 cb 00 cr 00 cb
               "pmulhw       %6, %%mm1\n\t" // 00 vg 00 ug 00 vg 00 ug
               "movq      %%mm1, %%mm3\n\t"
               "psrld       $16, %%mm3\n\t" // 00 00 00 vg 00 00 00 vg
               "paddw     %%mm3, %%mm1\n\t"
               "paddw     %%mm1, %%mm2\n\t" // 00 XX 00 g1 00 XX 00 g0
               "packuswb  %%mm2, %%mm2\n\t" // XX g1 XX g0 XX g1 XX g0
               "por          %7, %%mm2\n\t" // FF g1 FF g0 FF g1 FF g0
               
               "punpcklbw %%mm2, %%mm0\n\t" // FF r1 g1 b1 FF r0 g0 b0
               "movq      %%mm0, %%mm1\n\t"
               "punpcklbw %%mm7, %%mm0\n\t" // 00 FF 00 r0 00 g0 00 b0
               "punpckhbw %%mm7, %%mm1\n\t" // 00 FF 00 r1 00 g1 00 b1
               "movq      %%mm0,  (%0)\n\t"
               "movq      %%mm1, 8(%0)\n\t"
               "addl         $4,    %2\n\t"
               "addl        $16,    %0\n\t"
               "decl         %1\n\t"
               "jnz          1b\n\t"
               "emms\n\t"
               "2:"
               : /* no outputs */
               : "D" (gfxs->Dacc), "c" (gfxs->length), "S" (gfxs->Sop),
                 "m" (*sub), "m" (*mask), "m" (*mul0), "m" (*mul1), "m" (*alpha)
               : "memory" );
}

static void Sop_uyvy_to_Dacc_MMX( GenefxState *gfxs )
{
     static __u16 __aligned(8) sub[]   = {     128,      16,     128,      16 };   
     static __s16 __aligned(8) mul0[]  = {  0x3312,  0x253F,  0x4093,  0x253F };
     static __s16 __aligned(8) mul1[]  = { -0x0C83, -0x1A04, -0x0C83, -0x1A04 };
     static __u16 __aligned(8) alpha[] = {  0xFF00,  0xFF00,  0xFF00,  0xFF00 };
      
     __asm__ __volatile__ (
               "shrl      $1, %1\n\t"       // gfx->length /= 2
               "jz        2f\n\t"           // return if 0
               "pxor      %%mm7, %%mm7\n\t"
               ".align 16\n"
               "1:\n\t"
               "movd       (%2), %%mm0\n\t" // 00 00 00 00 y1 cr y0 cb
               "punpcklbw %%mm7, %%mm0\n\t" // 00 y1 00 cr 00 y0 00 cb
               "psubw        %3, %%mm0\n\t" // luma -= 16, chroma -= 128 
               "psllw        $3, %%mm0\n\t" // precision
               "movq      %%mm0, %%mm1\n\t" // save
                
               "pmulhw       %4, %%mm0\n\t" // 00 y1 00 vr 00 y0 00 ub
               "movq      %%mm0, %%mm2\n\t"
               "psrad       $16, %%mm2\n\t" // 00 00 00 y1 00 00 00 y0
               "packssdw  %%mm2, %%mm2\n\t" // 00 y1 00 y0 00 y1 00 y0
               "punpcklwd %%mm2, %%mm2\n\t" // 00 y1 00 y1 00 y0 00 y0
               "pslld       $16, %%mm0\n\t" // 00 vr 00 00 00 ub 00 00
               "psrad       $16, %%mm0\n\t" // 00 00 00 vr 00 00 00 ub (sign extend)
               "packssdw  %%mm0, %%mm0\n\t" // 00 vr 00 ub 00 vr 00 ub
               "paddw     %%mm2, %%mm0\n\t" // 00 r1 00 b1 00 r0 00 b0
               "packuswb  %%mm0, %%mm0\n\t" // r1 b1 r0 b0 r1 b1 r0 b0
               
               "pslld       $16, %%mm1\n\t" // 00 cr 00 00 00 cb 00 00
               "psrad       $16, %%mm1\n\t" // 00 00 00 cr 00 00 00 cb (sign extend)
               "packssdw  %%mm1, %%mm1\n\t" // 00 cr 00 cb 00 cr 00 cb
               "pmulhw       %5, %%mm1\n\t" // 00 vg 00 ug 00 vg 00 ug
               "movq      %%mm1, %%mm3\n\t"
               "psrld       $16, %%mm3\n\t" // 00 00 00 vg 00 00 00 vg
               "paddw     %%mm3, %%mm1\n\t"
               "paddw     %%mm1, %%mm2\n\t" // 00 XX 00 g1 00 XX 00 g0
               "packuswb  %%mm2, %%mm2\n\t" // XX g1 XX g0 XX g1 XX g0
               "por          %6, %%mm2\n\t" // FF g1 FF g0 FF g1 FF g0
               
               "punpcklbw %%mm2, %%mm0\n\t" // FF r1 g1 b1 FF r0 g0 b0
               "movq      %%mm0, %%mm1\n\t"
               "punpcklbw %%mm7, %%mm0\n\t" // 00 FF 00 r0 00 g0 00 b0
               "punpckhbw %%mm7, %%mm1\n\t" // 00 FF 00 r1 00 g1 00 b1
               "movq      %%mm0,  (%0)\n\t"
               "movq      %%mm1, 8(%0)\n\t"
               "addl         $4,    %2\n\t"
               "addl        $16,    %0\n\t"
               "decl         %1\n\t"
               "jnz          1b\n\t"
               "emms\n\t"
               "2:"
               : /* no outputs */
               : "D" (gfxs->Dacc), "c" (gfxs->length), "S" (gfxs->Sop),
                 "m" (*sub), "m" (*mul0), "m" (*mul1), "m" (*alpha)
               : "memory" );
}

static void Sop_rgb16_to_Dacc_MMX( GenefxState *gfxs )
{
     static const long mask[]  = { 0x07E0001F, 0x0000F800 };
     static const long smul[]  = { 0x00200800, 0x00000001 };
     static const long alpha[] = { 0x00000000, 0x00FF0000 };

     __asm__ __volatile__ (
	       "movq     %3, %%mm4\n\t"
	       "movq     %4, %%mm5\n\t"
	       "movq     %5, %%mm7\n\t"
               ".align   16\n"
               "1:\n\t"
               "movq     (%2), %%mm0\n\t"
               /* 1. Konvertierung nach 24 bit interleaved */
	       "movq     %%mm0, %%mm3\n\t"
               "punpcklwd %%mm3, %%mm3\n\t"
               "punpckldq %%mm3, %%mm3\n\t"
               "pand     %%mm4, %%mm3\n\t"
               "pmullw   %%mm5, %%mm3\n\t"
               "psrlw    $8, %%mm3\n\t"
               /* mm3 enthaelt jetzt: 0000 00rr 00gg 00bb des alten pixels */
               "por      %%mm7, %%mm3\n\t"
               "movq     %%mm3, (%0)\n\t"
               "dec      %1\n\t"
               "jz       2f\n\t"
               "psrlq    $16, %%mm0\n\t"
	       "addl     $8, %0\n\t"
               /* 2. Konvertierung nach 24 bit interleaved */
	       "movq     %%mm0, %%mm3\n\t"
               "punpcklwd %%mm3, %%mm3\n\t"
               "punpckldq %%mm3, %%mm3\n\t"
               "pand     %%mm4, %%mm3\n\t"
               "pmullw   %%mm5, %%mm3\n\t"
               "psrlw    $8, %%mm3\n\t"
               /* mm3 enthaelt jetzt: 0000 00rr 00gg 00bb des alten pixels */
               "por      %%mm7, %%mm3\n\t"
               "movq     %%mm3, (%0)\n\t"
               "dec      %1\n\t"
               "jz       2f\n\t"
               "psrlq    $16, %%mm0\n\t"
	       "addl     $8, %0\n\t"
               /* 3. Konvertierung nach 24 bit interleaved */
	       "movq     %%mm0, %%mm3\n\t"
               "punpcklwd %%mm3, %%mm3\n\t"
               "punpckldq %%mm3, %%mm3\n\t"
               "pand     %%mm4, %%mm3\n\t"
               "pmullw   %%mm5, %%mm3\n\t"
               "psrlw    $8, %%mm3\n\t"
               /* mm3 enthaelt jetzt: 0000 00rr 00gg 00bb des alten pixels */
               "por      %%mm7, %%mm3\n\t"
               "movq     %%mm3, (%0)\n\t"
               "dec      %1\n\t"
               "jz       2f\n\t"
               "psrlq    $16, %%mm0\n\t"
	       "addl     $8, %0\n\t"
               /* 4. Konvertierung nach 24 bit interleaved */
	       "movq     %%mm0, %%mm3\n\t"
               "punpcklwd %%mm3, %%mm3\n\t"
               "punpckldq %%mm3, %%mm3\n\t"
               "pand     %%mm4, %%mm3\n\t"
               "pmullw   %%mm5, %%mm3\n\t"
               "psrlw    $8, %%mm3\n\t"
               /* mm3 enthaelt jetzt: 0000 00rr 00gg 00bb des alten pixels */
               "por      %%mm7, %%mm3\n\t"
               "movq     %%mm3, (%0)\n\t"
               "dec      %1\n\t"
               "jz       2f\n\t"
	       "addl     $8, %0\n\t"
	       "addl     $8, %2\n\t"
               "jmp      1b\n"
               "2:\n\t"
               "emms"
               : /* no outputs */
               : "D" (gfxs->Dacc), "c" (gfxs->length), "S" (gfxs->Sop),
                 "m" (*mask), "m" (*smul), "m" (*alpha)
               : "%st", "memory");
}

static void Sop_rgb32_to_Dacc_MMX( GenefxState *gfxs )
{
     static const long alpha[]  = { 0, 0x00FF0000 };
     static const long zeros[]  = { 0, 0 };

     __asm__ __volatile__ (
	       "movq     %3, %%mm7\n\t"
	       "movq     %4, %%mm6\n\t"
               ".align   16\n"
               "1:\n\t"
               "movd     (%2), %%mm0\n\t"
               "punpcklbw %%mm6, %%mm0\n\t"
               "por      %%mm7, %%mm0\n\t"
               "movq     %%mm0, (%0)\n\t"
	       "addl     $4, %2\n\t"
	       "addl     $8, %0\n\t"
               "dec      %1\n\t"
               "jnz      1b\n\t"
               "emms"
               : /* no outputs */
               : "D" (gfxs->Dacc), "c" (gfxs->length), "S" (gfxs->Sop),
                 "m" (*alpha), "m" (*zeros)
               : "%st", "memory");
}

static void Xacc_blend_invsrcalpha_MMX( GenefxState *gfxs )
{
     static const long einser[] = { 0x01000100, 0x01000100 };
     static const long zeros[]  = { 0, 0 };

     __asm__ __volatile__ (
	       "movq     %3, %%mm7\n\t"
               "cmpl     $0, %2\n\t"
               "jne      3f\n\t"
               "movq     %4, %%mm6\n\t"
               "movd     %5, %%mm0\n\t"
               "punpcklbw %%mm6, %%mm0\n\t" /* mm0 = 00aa 00rr 00gg 00bb */
               "punpcklwd %%mm0, %%mm0\n\t" /* mm0 = 00aa 00aa xxxx xxxx */
               "movq      %%mm7, %%mm1\n\t"
               "punpckldq %%mm0, %%mm0\n\t" /* mm0 = 00aa 00aa 00aa 00aa */
               "psubw     %%mm0, %%mm1\n\t"

               ".align   16\n"
               "4:\n\t"                 /* blend from color */
               "testw    $0xF000, 6(%0)\n\t"
               "jnz      1f\n\t"
               "movq     (%0), %%mm0\n\t"
               "pmullw   %%mm1, %%mm0\n\t"
               "psrlw    $8, %%mm0\n\t"
               "movq     %%mm0, (%0)\n"
               "1:\n\t"
	       "addl     $8, %0\n\t"
               "dec      %1\n\t"
               "jnz      4b\n\t"
               "jmp      2f\n\t"

               ".align   16\n"
               "3:\n\t"                      /* blend from Sacc */
               "testw    $0xF000, 6(%0)\n\t"
               "jnz      1f\n\t"
               "movq     (%2), %%mm2\n\t"
               "movq     (%0), %%mm0\n\t"
	       "punpckhwd %%mm2, %%mm2\n\t" /* mm2 = 00aa 00aa xxxx xxxx */
               "movq	  %%mm7, %%mm1\n\t"
               "punpckhdq %%mm2, %%mm2\n\t" /* mm2 = 00aa 00aa 00aa 00aa */
               "psubw    %%mm2, %%mm1\n\t"
               "pmullw   %%mm1, %%mm0\n\t"
               "psrlw    $8, %%mm0\n\t"
               "movq     %%mm0, (%0)\n"
               "1:\n\t"
	       "addl     $8, %2\n\t"
	       "addl     $8, %0\n\t"
               "dec      %1\n\t"
               "jnz      3b\n\t"
               "2:\n\t"
               "emms"
               : /* no outputs */
               : "D" (gfxs->Xacc), "c" (gfxs->length), "S" (gfxs->Sacc),
                 "m" (*einser), "m" (*zeros), "m" (gfxs->color)
               : "%st", "memory");
}

static void Xacc_blend_srcalpha_MMX( GenefxState *gfxs )
{
     static const long ones[]  = { 0x00010001, 0x00010001 };
     static const long zeros[] = { 0, 0 };

     __asm__ __volatile__ (
	       "movq     %3, %%mm7\n\t"
               "cmpl     $0, %2\n\t"
               "jne      3f\n\t"
               "movq     %4, %%mm6\n\t"
               "movd     %5, %%mm0\n\t"
               "punpcklbw %%mm6, %%mm0\n\t" /* mm0 = 00aa 00rr 00gg 00bb */
               "punpcklwd %%mm0, %%mm0\n\t" /* mm0 = 00aa 00aa xxxx xxxx */
               "punpckldq %%mm0, %%mm0\n\t" /* mm0 = 00aa 00aa 00aa 00aa */
               "paddw     %%mm7, %%mm0\n\t"

               ".align   16\n"
               "4:\n\t"                 /* blend from color */
               "testw    $0xF000, 6(%0)\n\t"
               "jnz      1f\n\t"
               "movq     (%0), %%mm1\n\t"
               "pmullw   %%mm0, %%mm1\n\t"
               "psrlw    $8, %%mm1\n\t"
               "movq     %%mm1, (%0)\n"
               "1:\n\t"
	       "addl     $8, %0\n\t"
               "dec      %1\n\t"
               "jnz      4b\n\t"
               "jmp      2f\n\t"

               ".align   16\n"
               "3:\n\t"                      /* blend from Sacc */
               "testw    $0xF000, 6(%0)\n\t"
               "jnz      1f\n\t"
               "movq     (%2), %%mm0\n\t"
               "movq     (%0), %%mm1\n\t"
	       "punpckhwd %%mm0, %%mm0\n\t" /* mm2 = 00aa 00aa xxxx xxxx */
               "punpckhdq %%mm0, %%mm0\n\t" /* mm2 = 00aa 00aa 00aa 00aa */
               "paddw    %%mm7, %%mm0\n\t"
               "pmullw   %%mm0, %%mm1\n\t"
               "psrlw    $8, %%mm1\n\t"
               "movq     %%mm1, (%0)\n"
               "1:\n\t"
	       "addl     $8, %2\n\t"
	       "addl     $8, %0\n\t"
               "dec      %1\n\t"
               "jnz      3b\n\t"
               "2:\n\t"
               "emms"
               : /* no outputs */
               : "D" (gfxs->Xacc), "c" (gfxs->length), "S" (gfxs->Sacc),
                 "m" (*ones), "m" (*zeros), "m" (gfxs->color)
               : "%st", "memory");
}

