# YABFI
## Yet Another BrainFuck Interpreter

C89-compatible [Brainfuck](https://en.wikipedia.org/wiki/Brainfuck) interpreter


### Presentation

The current implementation has a low static memory footprint (it uses 64 KiB of
memory). The counterpart is that the program's data is capped at 32 KiB (32768
unsigned octets exactly), and the loop depth is limited to 512 levels of
nesting.

Proper bound-checking is also implemented to provide a safe program, and various
result codes are used by the executable interpreter to identify the source of
failure when the executions ends with error.


### Usage

The executable accepts a single argument, a character string containing the
Brainfuck code to execute (using command-line options to read from a file is a
planned evolution).

It reads from `stdin`, and outputs to `stdout`.


### Compilation

Just run `make` to compile, or use your own compiler.

Other Make rules provided are `debug`, to build a debuggable executable, and
`clean` to remove the compiled files (objects and executable).
