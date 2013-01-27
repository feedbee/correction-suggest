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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <inttypes.h>
#include <getopt.h>
#include <string.h>

struct Word {
	char *word;
	struct Word *next;
};

#ifndef _INTTYPES_H
typedef unsigned char uint8_t;
#endif

# if !(_SVID_SOURCE || _BSD_SOURCE || _XOPEN_SOURCE >= 500 || _XOPEN_SOURCE && _XOPEN_SOURCE_EXTENDED \
           || _POSIX_C_SOURCE >= 200809L)
char *strdup (const char *s) {
	char *d = malloc (strlen (s) + 1);
	if (d == NULL) return NULL;
	strcpy (d,s);
	return d;
}
# endif

int main (int argc, char **argv) {
	uint8_t verbose = 1;
	uint8_t max_word_length = 0;
	
	struct Word *first = NULL;
	struct Word *last = NULL;
	
	const uint8_t buf_size = 250;
	char buf[buf_size];
	int line = 0;
	while (fgets(buf, buf_size, stdin)) {
		line++;
		uint8_t string_length = (uint8_t)strlen(buf);
		if (string_length == buf_size - 1 && buf[buf_size - 2] != 0x13)
		{
			fprintf(stderr, "Is string %d greater than 250 symbols? Aborting\n", line);
			return 1;
		} else {
			if (verbose) {
				buf[string_length - 1] = 0x0;
				fprintf(stderr, "%s > ", buf);
				fprintf(stderr, "%d\n", (int)strlen(buf));
			}
			
			if (max_word_length < string_length - 1) {
				max_word_length = string_length - 1;
			}
			
			struct Word *new = (struct Word*)malloc(sizeof(struct Word));
			if (!new) {
				fprintf(stderr, "Out of memory at line %d\n", line);
				return 1;
			}
			new->word = strdup(buf);
			if (!new->word) {
				fprintf(stderr, "Out of memory at line %d\n", line);
				return 1;
			}
			
			if (!first) {
				first = last = new;
			} else {
				last->next = new;
				last = new;
			}
		}
	}
	
	if (verbose) {
		fprintf(stderr, "\nMax word length is %d\n", max_word_length);
	}
	
	
	// ---

	if (!first) {
		return 0;
	}
	
	uint8_t real_segment_length = max_word_length + 1;
	
	fwrite(&real_segment_length, sizeof(uint8_t), 1, stdout);
	
	struct Word *next = first;
	while (next) {
		uint8_t size = (uint8_t)strlen(next->word);
		fwrite(next->word, size, 1, stdout);
		for (int i = 0; i < real_segment_length - size; i++) {
			fwrite("", 1, 1, stdout);
		}
		last = next;
		next = next->next;
		free(last);
	}
	
	return 0;
}
