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

 #include "suggest.h"

// Main

int main (const int argc, const char **argv)
{
	struct Options opts;
	opts.verbose = 0;
	opts.runs = 1;
	opts.max_length_diff = 5;
	opts.max_lev_diff = 5; 
	opts.parallel_proc_count = 4;
	opts.file_name = "dictionary";
	
	read_opts(argc, argv, &opts);

	char *dict;
	size_t dict_size = load_dict(opts.file_name, &dict);

	struct timespec start_point;
	clock_gettime(CLOCK_MONOTONIC_RAW, &start_point);
	
	int i = 1;
	while (i <= opts.runs) {
		opts.verbose && fprintf(stderr, "Run #%d\r", i);
		struct timespec before_point;
		clock_gettime(CLOCK_MONOTONIC_RAW, &before_point);

		int word_index = 0;
		while (opts.words[word_index]) {
			print_closest(dict, dict_size, opts.words[word_index], opts.max_length_diff, opts.max_lev_diff, opts.parallel_proc_count);
			word_index++;
		}

		struct TimePair time_pair;
		diff_time(before_point, &time_pair);
		opts.verbose && fprintf(stderr, "Run %d time: %ld.%09ld s\n", i, time_pair.sec, time_pair.nano);

		i++;
	}
	opts.verbose && fprintf(stderr, "\n", i);

	struct TimePair time_pair;
	diff_time(start_point, &time_pair);
	opts.verbose && fprintf(stderr, "Overal execution time: %ld.%09ld s\n", time_pair.sec, time_pair.nano);
		
	unload_dict(dict, dict_size);
}

// Dict

size_t load_dict (const char *filename, char **addr)
{
	int fd = open(filename, O_RDONLY);
	if (fd == -1) {
		handle_error("open");
	}

	struct stat sb;
	if (fstat(fd, &sb) == -1) {
		handle_error("fstat");
	}
	size_t file_size = sb.st_size;

	*addr = (void*)mmap(NULL, file_size, PROT_READ, MAP_SHARED, fd, 0);
	if (addr == MAP_FAILED) {
		handle_error("mmap");
	}
	
	close(fd);
	
	return file_size;
}

void unload_dict (char *addr, size_t file_size)
{
	int r = munmap(addr, file_size);
	if (r == -1) {
		handle_error("munmap");
	}
}

void print_closest (const char *dict, size_t dict_size, const char *word, short max_length_diff, short max_lev_diff,
					short parallel_proc_count)
{
	size_t word_len = strlen(word);
	uint8_t segment_size = *dict;
	size_t offset = sizeof(uint8_t);
	size_t data_size = dict_size - offset;
	const char *data = dict + offset;
	size_t segments_count = data_size / segment_size;
	
	print_closest_fork(data, segment_size, segments_count, word_len, word, max_length_diff, max_lev_diff, parallel_proc_count);
}

void print_closest_fork (const char *data, uint8_t segment_size, int segments_count, size_t word_len,
						 const char *word, short max_length_diff, short max_lev_diff, short parallel_proc_count)
{
	short children_count = parallel_proc_count;
	short last_child = 0;

	int pipefd[children_count][2];
	
	while (last_child < children_count) {

		if (pipe(pipefd[last_child]))
		{
			handle_error("pipe");
		}

		pid_t pid = fork();
		if (pid < 0) {
			handle_error("fork");
			
		} else if (pid == 0) { // child

			close(pipefd[last_child][0]);
			FILE *stream = fdopen(pipefd[last_child][1], "w");

			print_closest_iterations(stream, last_child, segments_count, children_count, data, segment_size,
			                         word_len, word, max_length_diff, max_lev_diff);
			fclose(stream);
			close(pipefd[last_child][1]);
			exit(0);
			
		} else { // parent

			close(pipefd[last_child][1]);
			last_child++;
		}
	}

	for (int i = 0; i < last_child - 1; i++) {
		char buf[255];
		while (read(pipefd[i][0], &buf, 255) > 0) {
			write(STDOUT_FILENO, &buf, strlen(buf) + 1);
		}
	}
	
	// parent only
	while (1) {
		int status;
		pid_t done = wait(&status);
		if (done == -1) {
			if (errno == ECHILD) break; // no more child processes
		} else {
			if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
				handle_error("wait");
			}
		}
	}
}

void print_closest_iterations (FILE *stream, int start, int stop, int step, const char *data, uint8_t segment_size,
			     			   size_t word_len, const char *word, short max_length_diff, short max_lev_diff)
{
	// printf("%d\t%d\t%d\t%d\n", getpid(), start, stop, step); // DEBUG
	size_t* levenstein_buffer = levenstein_init_buffer(segment_size);
	for (int i = start; i < stop; i += step) {
		const char *segment = data + segment_size * i;
		print_closest_segment(stream, levenstein_buffer, segment, segment_size, word_len, word, max_length_diff, max_lev_diff);
	}
	levenstein_free_buffer(levenstein_buffer);
}

void print_closest_segment (FILE *stream, size_t* levenstein_buffer, const char *segment, uint8_t segment_size, 
							size_t word_len, const char *word, short max_length_diff, short max_lev_diff)
{
	uint8_t segment_length = (uint8_t)strlen(segment);
	
	if (abs(word_len - segment_length) <= max_length_diff) {
		if (word_len == segment_length && !strcasecmp(word, segment)) {
			fprintf(stream, "0\t%s\n", segment);
			
		} else {

			size_t word_len = strlen(word);
			size_t segment_len = strlen(segment);
			
			size_t result = levenstein(word, word_len, segment, segment_len, levenstein_buffer, segment_size);
			
			if (result <= max_lev_diff) {
				fprintf(stream, "%d\t%s\n", (int)result, segment);
			}
		}
	}
}

// Options

void read_opts (const int argc, const char **argv, struct Options *opts)
{
	while (1)
	{
		static struct option long_options[] =
		{
			{"verbose",       required_argument, 0, 'v'},
			{"runs",          required_argument, 0, 'r'},
			{"strlen-diff",   required_argument, 0, 's'},
			{"lev-diff",      required_argument, 0, 'l'},
			{"parallel-proc", required_argument, 0, 'p'},
			{"dict-file",     required_argument, 0, 'd'},
			{"help",          no_argument,       0, 'h'},
			{0, 0, 0, 0}
		};

		int option_index = 0;
		int c = getopt_long(argc, (char**)argv, "v:r:s:l:p:d:h", long_options, &option_index);


		if (c == -1)
			break;

		switch (c)
		{
			case 'v': /* --verbose */
				opts->verbose = (uint8_t)atoi(optarg);
				break;

			case 'r': /* --runs */
				opts->runs = atoi(optarg);
				break;

			case 's': /* --strlen_diff */
				opts->max_length_diff = atoi(optarg);
				break;

			case 'l': /* --lev_diff */
				opts->max_lev_diff = atoi(optarg);
				break;

			case 'p': /* --parallel-proc */
				opts->parallel_proc_count = atoi(optarg);
				break;

			case 'd': /* --dict_file */
				opts->file_name = optarg;
				break;

			case 'h': /* --help */
				printf ("Usage: %s [-s max_strlen_diff] [-l max_levenstein_diff] [-p parallel_proc_count] [-r runs] [-d dict_file] word | -h\n", argv[0]);
				exit(0);
				break;

			case '?':
				/* getopt_long already printed an error message. */
				break;

			default:
				abort();
		}
	}
	
	if (optind < argc)
	{
		int i = 0;
		int words_count = argc - optind;
		const char **words = calloc(words_count + 1, sizeof(char*));
		while (optind + i < argc) {
			words[i] = argv[optind + i];
			i++;
		}
		words[words_count] = NULL;
		opts->words = words;
		
	} else {
		fprintf (stderr, "One or more words is required!\n");
		printf ("Usage: %s [-s max_strlen_diff] [-l max_levenstein_diff] [-p parallel_proc_count] [-r runs] [-d dict_file] word | -h\n", argv[0]);
		exit(1);
	}
}

// Timer

void diff_time(struct timespec start_point, struct TimePair *time_pair)
{
	struct timespec end_point;
	clock_gettime(CLOCK_MONOTONIC_RAW, &end_point);

	if (end_point.tv_nsec < start_point.tv_nsec) {
		time_pair->nano = 1000000000 - start_point.tv_nsec + end_point.tv_nsec;
		time_pair->sec = end_point.tv_sec - start_point.tv_sec - 1;
	} else {
		time_pair->nano = end_point.tv_nsec - start_point.tv_nsec;
		time_pair->sec = end_point.tv_sec - start_point.tv_sec;
	}
}