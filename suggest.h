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

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <strings.h>
#include <getopt.h>
#include <sys/wait.h>
#include <time.h>
#include <errno.h>

// Types
#ifndef _INTTYPES_H
typedef unsigned char uint8_t;
#endif

// Dependencies
#include "levenstein.h"

// Service
#define handle_error(msg) \
	do { perror(msg); exit(EXIT_FAILURE); } while (0)

// Dict
size_t load_dict (const char *filename, char **addr);
void unload_dict (char *addr, size_t file_size);
void print_closest (const char *dict, size_t dict_size, const char *word, short max_length_diff, short max_lev_diff,
					short parallel_proc_count);
void print_closest_fork (const char *data, uint8_t segment_size, int segments_count, size_t word_len,
						 const char *word, short max_length_diff, short max_lev_diff, short parallel_proc_count);
void print_closest_iterations (FILE *stream, int start, int stop, int step, const char *data, uint8_t segment_size,
                               size_t word_len, const char *word, short max_length_diff, short max_lev_diff);
void print_closest_segment (FILE *stream, size_t* levenstein_buffer, const char *segment, uint8_t segment_size,
							size_t word_len, const char *word, short max_length_diff, short max_lev_diff);

// Options
struct Options {
	uint8_t verbose;
	int runs;
	short max_length_diff;
	short max_lev_diff;
	uint8_t parallel_proc_count;
	char *file_name;
	const char **words;
};
void read_opts (const int argc, const char **argv, struct Options *opts);

// Timer
struct TimePair {
	long sec;
	long nano;
};
void diff_time(struct timespec start_point, struct TimePair *time_pair);