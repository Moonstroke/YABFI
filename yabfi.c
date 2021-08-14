#include <stdint.h> /* for uint8_t */
#include <stdio.h>
#include <stdlib.h> /* for malloc, free */



#define TAPE_SIZE 32768
#define MAX_LOOP_DEPTH 512


enum bf_error {
	BF_SUCCESS = 0, /* No error */

	BF_ERROR_ENV          = 0x20, /* Generic code for an error unrelated to the code */
	BF_ERROR_INVALID_ARGS = 0x21, /* Incorrect command-line arguments */
	BF_ERROR_IO           = 0x22, /* I/O error: file not found, read/write failure */
	BF_ERROR_NOMEM        = 0x23, /* Impossible to allocate memory */

	BF_ERROR_PROGRAM        = 0x40, /* Generic code for errors in the Brainfuck source */
	BF_ERROR_TAPE_OVERFLOW  = 0x41, /* Access to a < 0 cell on the tape */
	BF_ERROR_TAPE_UNDERFLOW = 0x42, /* Tape pointer beyond scope (< 32,768) */
	BF_ERROR_LOOP_OVERFLOW  = 0x43, /* Maximum loop nesting level exceeded */
	BF_ERROR_LOOP_UNDERFLOW = 0x44, /* Unbalanced ] */
};


enum bf_error read_program(int argc, char *const *argv, char **program, int *needs_free) {
	const char *src_file = NULL;
	if (argc == 2) {
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
		return BF_ERROR_IO;
	}
	long length;
	if (fseek(file, 0, SEEK_END) < 0
	    || (length = ftell(file)) < 0
	    || fseek(file, 0, SEEK_SET) < 0) {
		fclose(file);
		return BF_ERROR_IO;
	}
	*program = malloc(length);
	if(*program == NULL) {
		fclose(file);
		return BF_ERROR_NOMEM;
	}
	if(fread(*program, 1, length, file) < (unsigned long) length) {
		free(*program);
		fclose(file);
		return BF_ERROR_IO;
	}
	fclose(file);
	*needs_free = 1;
	return BF_SUCCESS;
}

enum bf_error parse(const char *program, const char **loop_opens, const char **loop_closes) {
	size_t depth = 0;
	for (; *program; ++program) {
		if (*program == '[') {
			loop_opens[depth++] = program;
			if (depth == MAX_LOOP_DEPTH) return BF_ERROR_LOOP_OVERFLOW;
		} else if (*program == ']') {
			if (depth == 0) return BF_ERROR_LOOP_UNDERFLOW;
			loop_closes[--depth] = program;
		}
	}
	return BF_SUCCESS;
}

enum bf_error run(const char *program, uint8_t *tape, const char *const *loop_opens, const char *const *loop_closes) {
	size_t tape_pointer = 0;
	size_t loop_depth = 0;
	for (; *program; ++program) {
		switch (*program) {
			case '>':
				if (++tape_pointer == TAPE_SIZE) return BF_ERROR_TAPE_OVERFLOW;
				break;
			case '<':
				if (tape_pointer-- == 0) return BF_ERROR_TAPE_UNDERFLOW;
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
					if(input == EOF) {
						if(feof(stdin)) {
							tape[tape_pointer] = 0;
						} else {
							// ferror(stdin) is true
							return BF_ERROR_IO;
						}
					} else {
						tape[tape_pointer] = (uint8_t) input;
					}
				}
				break;
			case '.':
				if (putchar(tape[tape_pointer]) == EOF) return BF_ERROR_IO;
				break;
			case '[':
				if (tape[tape_pointer] == 0) {
					program = loop_closes[loop_depth];
				} else {
					++loop_depth;
				}
				break;
			case ']':
				if (tape[tape_pointer] == 0) {
					--loop_depth;
				} else {
					program = loop_opens[loop_depth - 1];
				}
				break;
		}
	}
	return BF_SUCCESS;
}

int main(int argc, char **argv) {
	char *program;
	int needs_free;
	int rc = read_program(argc, argv, &program, &needs_free);
	if (rc == BF_ERROR_INVALID_ARGS) {
		fprintf(stderr, "USAGE: %s [-f] BRAINFUCK_FILE | -x BRAINFUCK_CODE\n", argv[0]);
		return BF_ERROR_INVALID_ARGS;
	} else if (rc != BF_SUCCESS) {
		return rc;
	}
	uint8_t *tape = calloc(TAPE_SIZE, sizeof *tape);
	const char **loop_opens = NULL;
	const char **loop_closes = NULL;
	if (tape == NULL) {
		rc = BF_ERROR_NOMEM;
	}
	loop_opens = calloc(MAX_LOOP_DEPTH, sizeof *loop_opens);
	if (loop_opens == NULL) {
		rc = BF_ERROR_NOMEM;
	}
	loop_closes = calloc(MAX_LOOP_DEPTH, sizeof *loop_closes);
	if (loop_closes == NULL) {
		rc = BF_ERROR_NOMEM;
	}
	if (rc == BF_SUCCESS) {
		rc = parse(program, loop_opens, loop_closes);
	}
	if (rc == BF_SUCCESS) {
		rc = run(program, tape, loop_opens, loop_closes);
	}

	free(loop_closes);
	free(loop_opens);
	free(tape);
	if(needs_free) {
		free(program);
	}
	return rc;
}
