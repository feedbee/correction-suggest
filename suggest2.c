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

 #include "suggest2.h"

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
	size_t dict_size = load_dict(opts.file_name, &dict, opts.parallel_proc_count);

	struct timespec start_point;
	clock_gettime(CLOCK_MONOTONIC, &start_point);
	
	int i = 1;
	while (i <= opts.runs) {
		opts.verbose && fprintf(stderr, "Run #%d\r", i);
		struct timespec before_point;
		clock_gettime(CLOCK_MONOTONIC, &before_point);

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
		
	unload_dict(dict);
}

// Dict

size_t load_dict (const char *filename, char **addr, uint8_t parallel_proc_count)
{
	FILE *file = fopen(filename, "r");
	int fd = open(filename, O_RDONLY);
	if (fd == -1) {
		handle_error("open");
	}

	struct stat sb;
	if (fstat(fd, &sb) == -1) {
		handle_error("fstat");
	}
	size_t file_size = sb.st_size;
	*addr = (void*)malloc(file_size + sizeof(uint8_t));
	if (!addr) {
		handle_error("malloc for dict");
	}

	char *offsets[parallel_proc_count];
	for (int i = 0; i < parallel_proc_count; i++) {
		offsets[i] = *(addr) + sizeof(uint8_t) + (file_size / parallel_proc_count + 1) * i;
	}

	const uint8_t buf_size = 250;
	uint8_t max_string_length = 0;
	char buf[buf_size];
	int line = 0;
	while (fgets(buf, buf_size, file)) {
		//fprintf(stderr, "%d\r", line);
		
		uint8_t process = line % parallel_proc_count;
		uint8_t string_length = (uint8_t)strlen(buf);
		if (string_length == buf_size - 1 && buf[buf_size - 2] != 0x13)
		{
			fprintf(stderr, "Is string %d greater than 250 symbols? Aborting\n", line + 1);
			return 1;
		} else {
			while (buf[string_length - 1] == 10 || buf[string_length - 1] == 13) {
				buf[string_length - 1] = 0;
				string_length--;
			}

			*offsets[process] = string_length;
			strcpy((offsets[process] + 1), buf);
			offsets[process] += 1 + string_length;
		}

		if (max_string_length < string_length) {
			max_string_length = string_length;
		}

		line++;
	}

	**addr = max_string_length;

	
	for (uint8_t j = 0; j < parallel_proc_count; j++) {
		offsets[j] = 0;
	}
	
	return file_size;
}

void unload_dict (char *addr)
{
	free(addr);
}

void print_closest (const char *dict, size_t dict_size, const char *word, short max_length_diff, short max_lev_diff,
					short parallel_proc_count)
{
	size_t word_len = strlen(word);
	uint8_t max_string_length = *dict;
	size_t offset = sizeof(uint8_t);
	const char *data = dict + offset;
	
	print_closest_fork(data, max_string_length, dict_size, word_len, word, max_length_diff, max_lev_diff, parallel_proc_count);
}

void print_closest_fork (const char *data, uint8_t max_string_length, size_t dict_size, size_t word_len,
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

			const char *offset = data + sizeof(uint8_t) + (dict_size / parallel_proc_count + 1) * last_child;

			print_closest_iterations(stream, offset, max_string_length,
			                         word_len, word, max_length_diff, max_lev_diff);
			fclose(stream);
			close(pipefd[last_child][1]);
			exit(0);
			
		} else { // parent

			close(pipefd[last_child][1]);
			last_child++;
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

	for (int i = 0; i <= last_child - 1; i++) {
		char buf[255];
		ssize_t read_bytes;
		while ((read_bytes = read(pipefd[i][0], &buf, 255)) > 0) {
			ssize_t write_result = write(STDOUT_FILENO, &buf, read_bytes);
			assert(write_result == read_bytes);
		}
	}
}

void print_closest_iterations (FILE *stream, const char *offset, uint8_t max_string_length,
			     			   size_t word_len, const char *word, short max_length_diff, short max_lev_diff)
{
	// printf("%d\t%d\t%d\t%d\n", getpid(), start, stop, step); // DEBUG
	size_t* levenstein_buffer = levenstein_init_buffer(max_string_length);
	size_t local_offset = 0;
	size_t sizeof_uint8_t = sizeof(uint8_t);
	while (1) {
		uint8_t local_word_length = *(offset + local_offset);
		const char *local_word = offset + local_offset + sizeof_uint8_t;
		print_closest_segment(stream, levenstein_buffer, local_word, local_word_length, max_string_length, word_len, word, max_length_diff, max_lev_diff);

		local_offset += sizeof_uint8_t + local_word_length;

		if (*(offset + local_offset) == 0) {
			break;
		}
	}
	levenstein_free_buffer(levenstein_buffer);
}

void print_closest_segment (FILE *stream, size_t* levenstein_buffer, const char *local_word, uint8_t local_word_length, uint8_t max_string_length, 
							size_t word_len, const char *word, short max_length_diff, short max_lev_diff)
{
	size_t result = -1;

	if (word_len == local_word_length && !strncmp(local_word, word, local_word_length)) {
		result = 0;
		
	} else {
		
		if (abs(word_len - local_word_length) <= max_length_diff) {
			result = levenstein(word, word_len, local_word, local_word_length, levenstein_buffer, max_string_length);
		}
	}

	if (result >= 0 && result <= max_lev_diff) {
		char out_buf[local_word_length + 1];
		strncpy(out_buf, local_word, local_word_length);
		out_buf[local_word_length] = 0;
		fprintf(stream, "[%d] :: %s\n", (int)result, out_buf);
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
	clock_gettime(CLOCK_MONOTONIC, &end_point);

	if (end_point.tv_nsec < start_point.tv_nsec) {
		time_pair->nano = 1000000000 - start_point.tv_nsec + end_point.tv_nsec;
		time_pair->sec = end_point.tv_sec - start_point.tv_sec - 1;
	} else {
		time_pair->nano = end_point.tv_nsec - start_point.tv_nsec;
		time_pair->sec = end_point.tv_sec - start_point.tv_sec;
	}
}
