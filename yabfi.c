#include <stdint.h> /* for uint8_t */
#include <stdio.h>



#define TAPE_SIZE 32768
#define MAX_LOOP_DEPTH 512


enum bf_error {
	BF_SUCCESS,
	BF_ERROR_INVALID_ARGS,
	BF_ERROR_TAPE_OVERFLOW,
	BF_ERROR_TAPE_UNDERFLOW,
	BF_ERROR_IO,
	BF_ERROR_LOOP_OVERFLOW,
	BF_ERROR_LOOP_UNDERFLOW,
};


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
					tape[tape_pointer] = (uint8_t) (input == EOF ? 0 : input);
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
	if (argc != 2) {
		fprintf(stderr, "USAGE: %s BRAINFUCK_CODE\n", argv[0]);
		return BF_ERROR_INVALID_ARGS;
	}
	uint8_t tape[TAPE_SIZE] = {0};
	const char *loop_opens[MAX_LOOP_DEPTH] = {0};
	const char *loop_closes[MAX_LOOP_DEPTH] = {0};
	int rc = parse(argv[1], loop_opens, loop_closes);
	if (rc == BF_SUCCESS) {
		rc = run(argv[1], tape, loop_opens, loop_closes);
	}
	return rc;
}
