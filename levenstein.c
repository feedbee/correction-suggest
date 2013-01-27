/** 
 * BSD 3-Clause License
 *
 * Copyright (c) 2013, Valera Leontyev.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  - Redistributions of source code must retain the above copyright notice,
 *  - this list of conditions and the following disclaimer.
 *
 *  - Redistributions in binary form must reproduce the above copyright notice,
 *  - this list of conditions and the following disclaimer in the documentation
 *  - and/or other materials provided with the distribution.
 *
 *  - Neither the name of the Valera Leontyev nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
 
#include <stddef.h>
#include "levenstein.h"

/**
 * Initial source: http://cplus.about.com/od/programmingchallenges/a/Programming-Challenge-39-Calculate-Levenshtein-Distance.htm
 * Pedro Graca, http://cplus.about.com/library/downloads/challenges/39/6.zip
 * Realization was modified by Valera Leontyev
 */

size_t levenstein(const char *a, size_t alen,
                   const char *b, size_t blen,
                   size_t *buff, size_t bsize) {
  size_t row, col;
  size_t r1index = 0;
  size_t rindex = bsize;

  /* strip common prefix */
  while (alen && blen && *a == *b) {
    a++;
    b++;
    alen--;
    blen--;
  }

  /* strip common suffix */
  while (alen && blen && (a[alen - 1] == b[blen - 1])) {
    alen--;
    blen--;
  }

  /* special case empty strings */
  if (alen == 0) return blen;
  if (blen == 0) return alen;

  /* make b the bigger string (inner loop) */
  if (blen < alen) {
    size_t slen;
    const char *s;

    slen = blen;
    blen = alen;
    alen = slen;

    s = b;
    b = a;
    a = s;
  }

  for (row = 1; row <= alen; row++) {
    size_t r1 = row - 1;
    for (col = 1; col <= blen; col++) {
      size_t c1 = col - 1;
      size_t min3v, v1, v2, v3;
      size_t extra = 1;

      if (a[r1] == b[c1]) extra = 0;
      v1 = buff[r1index + col] + 1;
      v2 = buff[rindex + c1] + 1;
      v3 = buff[r1index + c1] + extra;
      if (v1 < v2) {
        min3v = v1<v3 ? v1 : v3;
      } else {
        min3v = v2<v3 ? v2 : v3;
      }
      buff[rindex + col] = min3v;
    }
    rindex += bsize;
    r1index += bsize;
  }

  return buff[alen * bsize + blen];
}

size_t* levenstein_init_buffer(size_t size) {
  size_t j, *buff = NULL;
  buff = malloc((size + 1) * (size + 1) * sizeof *buff);
  assert(buff != NULL && "Not enough memory");
  *buff = 0;
  for (j = 1; j <= size; j++) {
    buff[j] = buff[j * (size + 1)] = j;
  }

  return buff;
}

void levenstein_free_buffer(size_t* buff) {
  free(buff);
}