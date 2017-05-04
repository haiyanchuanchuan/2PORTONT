/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtomcrypt.org
 */

/**
   @file ocb_ntz.c
   OCB implementation, internal function, by Tom St Denis
*/

#include "tomcrypt.h"

#ifdef OCB_MODE

/**
   Returns the number of leading zero bits [from lsb up]
   @param x  The 32-bit value to observe
   @return   The number of bits [from the lsb up] that are zero
*/
int ocb_ntz(unsigned long x)
{
   int c;
   x &= 0xFFFFFFFFUL;
   c = 0;
   while ((x & 1) == 0) {
      ++c;
      x >>= 1;
   }
   return c;
}

#endif

/* $Source: /usr/local/dslrepos/uClinux-dist/user/dropbear-0.48.1/libtomcrypt/src/encauth/ocb/ocb_ntz.c,v $ */
/* $Revision: 1.1 $ */
/* $Date: 2006/06/08 13:40:01 $ */