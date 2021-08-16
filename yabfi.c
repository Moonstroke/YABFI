#include <errno.h> /* for errno, EISDIR */
#include <limits.h> /* for LONG_MAX */
#include <stddef.h> /* for ptrdiff_t, size_t */
#include <stdint.h> /* for uint8_t */
#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* for strcmp */



#define TAPE_SIZE 32768
#define MAX_LOOP_DEPTH 512


enum bf_rescode {
	BF_SUCCESS = 0, /* No error */
	BF_USAGE   = 1, /* Not an error. Print usage and quit */

	BF_ERROR_ENV          = 0x20, /* Generic code for an error unrelated to the
	                                 code */
	BF_ERROR_INVALID_ARGS = 0x21, /* Incorrect command-line arguments */
	BF_ERROR_IO           = 0x22, /* I/O error: file not found, read/write
	                                 failure */
	BF_ERROR_NOMEM        = 0x23, /* Impossible to allocate memory */

	BF_ERROR_PROGRAM        = 0x40, /* Generic code for errors in the Brainfuck
	                                   source */
	BF_ERROR_TAPE_OVERFLOW  = 0x41, /* Access to a < 0 cell on the tape */
	BF_ERROR_TAPE_UNDERFLOW = 0x42, /* Tape pointer beyond scope (< 32,768) */
	BF_ERROR_LOOP_UNMATCHED = 0x43, /* Unbalanced [ or ] loop bound */
};


enum bf_rescode parse_args(int argc, char *const *argv, char **program,
                           int *needs_free) {
	const char *src_file = NULL;
	if (argc == 2) {
		if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
			return BF_USAGE;
		}
		src_file = argv[1];
	} else if (argc == 3) {
		if (argv[1][0] == '-') {
			if (argv[1][1] == 'x') {
				*program = argv[2];
				*needs_free = 0;
				return BF_SUCCESS;
			} else if (argv[1][1] == 'f') {
				src_file = argv[2];
			} else {
				return BF_ERROR_INVALID_ARGS;
			}
		} else {
			return BF_ERROR_INVALID_ARGS;
		}
	} else {
		return BF_ERROR_INVALID_ARGS;
	}
	/* File path given in src_file. Read from the file into *program */
	FILE *file = fopen(src_file, "r");
	if (file == NULL) {
		perror(src_file);
		return BF_ERROR_IO;
	}
	long length = 0;
	if (fseek(file, 0, SEEK_END) < 0
	    || (length = ftell(file)) < 0 || length == LONG_MAX
	    || fseek(file, 0, SEEK_SET) < 0) {
		if (length == LONG_MAX) {
			/* On my Debian 9.13 with libc6 v2.24 and Linux 4.9.0 fopening a
			   directory works, but ftelling it afterwards returns LONG_MAX. We
			   assume it is the same case here and change the error to the
			   appropriate flag so that perror outputs the intended message */
			errno = EISDIR;
		}
		perror(src_file);
		fclose(file);
		return BF_ERROR_IO;
	}
	*program = malloc(length);
	if (*program == NULL) {
		perror(NULL);
		fclose(file);
		return BF_ERROR_NOMEM;
	}
	if (fread(*program, 1, length, file) < (unsigned long) length) {
		perror(src_file);
		free(*program);
		fclose(file);
		return BF_ERROR_IO;
	}
	fclose(file);
	*needs_free = 1;
	return BF_SUCCESS;
}

enum bf_rescode parse_source(const char *program, const char **loop_bounds,
                             ptrdiff_t *loop_wrap_diffs) {
	const char *const program_backup = program;
	size_t index = 0;
	for (; *program; ++program) {
		ptrdiff_t diff;
		unsigned int loop_depth;
		if (*program == '[') {
			loop_bounds[index] = program;
			for (diff = 1, loop_depth = 1; loop_depth > 0; ++diff) {
				if (program[diff] == '[') {
					++loop_depth;
				} else if (program[diff] == ']') {
					--loop_depth;
				} else if (program[diff] == '\0') {
					return BF_ERROR_LOOP_UNMATCHED;
				}
			}
			loop_wrap_diffs[index++] = diff - 1;
		} else if (*program == ']') {
			loop_bounds[index] = program;
			for (diff = -1, loop_depth = 1; loop_depth > 0; --diff) {
				if (program[diff] == '[') {
					--loop_depth;
				} else if (program[diff] == ']') {
					++loop_depth;
				} else if (diff == program_backup - program) {
					return BF_ERROR_LOOP_UNMATCHED;
				}
			}
			loop_wrap_diffs[index++] = diff + 1;
		}
	}
	return BF_SUCCESS;
}

size_t indexof(const char *const *ptrs, const char *ptr) {
	size_t i = 0;
	/* No bounds checks needed because we know, since function parse, that the
	   value is present */
	while (ptrs[i] != ptr) {
		++i;
	}
	return i;
}

enum bf_rescode run(const char *program, uint8_t *tape,
                    const char *const *loop_bounds,
                    const ptrdiff_t *loop_wrap_diffs) {
	size_t tape_pointer = 0; /* Change to 16384 for a symmetrical tape */
	for (; *program; ++program) {
		switch (*program) {
			case '>':
				if (++tape_pointer == TAPE_SIZE) {
					/* Wrap back to 0 for a circular tape */
					return BF_ERROR_TAPE_OVERFLOW;
				}
				break;
			case '<':
				if (tape_pointer-- == 0) {
					/* Wrap to TAPE_SIZE - 1 for a circular tape */
					return BF_ERROR_TAPE_UNDERFLOW;
				}
				break;
			case '+':
				tape[tape_pointer]++;
				break;
			case '-':
				tape[tape_pointer]--;
				break;
			case ',':
				{
					int input = getchar();
					if (input == EOF) {
						if(feof(stdin)) {
							tape[tape_pointer] = 0;
						} else {
							perror(NULL);
							/* here ferror(stdin) is true */
							return BF_ERROR_IO;
						}
					} else {
						tape[tape_pointer] = (uint8_t) input;
					}
				}
				break;
			case '.':
				if (putchar(tape[tape_pointer]) == EOF) {
					perror(NULL);
					return BF_ERROR_IO;
				}
				break;
			case '[':
				if (tape[tape_pointer] == 0) {
					program += loop_wrap_diffs[indexof(loop_bounds, program)];
				}
				break;
			case ']':
				if (tape[tape_pointer] != 0) {
					program += loop_wrap_diffs[indexof(loop_bounds, program)];
				}
				break;
		}
	}
	return BF_SUCCESS;
}

int main(int argc, char **argv) {
	char *program;
	int needs_free;
	int rc = parse_args(argc, argv, &program, &needs_free);
	if (rc == BF_ERROR_INVALID_ARGS || rc == BF_USAGE) {
		fprintf(stderr, "USAGE: %s [-f] BRAINFUCK_FILE | -x BRAINFUCK_CODE\n",
		        argv[0]);
		/* BF_USAGE does not mean failure, exit with success code */
		return rc == BF_USAGE ? BF_SUCCESS : BF_ERROR_INVALID_ARGS;
	} else if (rc != BF_SUCCESS) {
		return rc;
	}
	uint8_t *tape = calloc(TAPE_SIZE, sizeof *tape);
	const char **loop_bounds = NULL;
	ptrdiff_t *loop_wrap_diffs = NULL;
	if (tape == NULL) {
		perror(NULL);
		rc = BF_ERROR_NOMEM;
	}
	loop_bounds = calloc(MAX_LOOP_DEPTH, sizeof *loop_bounds);
	if (loop_bounds == NULL) {
		perror(NULL);
		rc = BF_ERROR_NOMEM;
	}
	loop_wrap_diffs = calloc(MAX_LOOP_DEPTH, sizeof *loop_wrap_diffs);
	if (loop_wrap_diffs == NULL) {
		perror(NULL);
		rc = BF_ERROR_NOMEM;
	}
	if (rc == BF_SUCCESS) {
		rc = parse_source(program, loop_bounds, loop_wrap_diffs);
	}
	if (rc == BF_SUCCESS) {
		rc = run(program, tape, loop_bounds, loop_wrap_diffs);
	}

	free(loop_wrap_diffs);
	free(loop_bounds);
	free(tape);
	if (needs_free) {
		free(program);
	}
	return rc;
}
