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

#ifndef LEVENSTEIN_H_INCLUDED
#define LEVENSTEIN_H_INCLUDED

#include <stddef.h>
#include <stdlib.h>
#include <assert.h>

/* Calculates the levenstein distance between sleft and srite
 * nleft and nrite are the lengths of the sleft and srite strings
 * buff should point to a square array of size bsize * bsize
 * with the first row initialized with 0, 1, 2, 3, etc
 * and the first column with 0, 1, 2, 3 etc (the 0 is the same as for the row)
 *
 * 0 1 2 3 4 ... (bsize - 1)
 * 1
 * 2
 * ...
 * (bsize - 1)
 *
 * the function does not write to these initial values
 *
 * If any pointer parameter is NULL the behaviour is undefined
 * If `buff` is not bsize*bsize the behaviour is undefined
 */
size_t levenstein(const char *a, size_t alen,
                   const char *b, size_t blen,
                   size_t *buff, size_t bsize);

size_t* levenstein_init_buffer(size_t size);

void levenstein_free_buffer(size_t* buff);

#endif
