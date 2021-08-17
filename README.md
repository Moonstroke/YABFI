# YABFI
## Yet Another BrainFuck Interpreter

Standard C99 compatible [Brainfuck] interpreter


### Presentation

The current implementation has a low memory footprint: it uses 48 KiB of
dynamically-allocated memory and a negligible amount of static memory. The
counterpart is that the program's data is capped at 32 KiB (32768 unsigned
octets exactly), and the loop depth is limited to 512 levels of nesting. The
Brainfuck source is copied in additional dynamic memory only if it is being
read from a file (passing a code string on the command-line means no dynamic
memory is allocated for it).

Proper bound-checking is also implemented to provide a safe program, and various
result codes are used by the executable interpreter to identify the source of
failure when the executions ends with error.


### Usage

The executable accepts either a single path argument to a Brainfuck program
file, the option `-f` with once again a path argument to a Brainfuck program
file, or the `-x` option with as argument a character string containing the
Brainfuck code to execute.

It reads from `stdin`, and outputs to `stdout`; redirection from or to files
will have to be performed through the shell.
`stderr` is only written to when an error occurs; the errors here correspond to
calls to standard library functions failing. The source (if available) and
reason of the failure are output to `stderr`.

Otherwise, non-zero exit codes can be used to diagnose errors; these codes are
defined as `enum` constants at the top of the source file.


### Compilation

Compilation using compilers accepting GNU-style options, namely [gcc], [clang],
[tcc], etc. is facilitated thorough the use of [GNU Make]; if you have it
installed, just run `make` to compile.
The Makefile also provides the rules `debug`, to build a debuggable executable,
and `clean` to remove the compiled files (objects and executable).

If you don't have (or don't want to use) Make, you can easily use your own
compiler as the source is only a single file.

[Brainfuck]:https://en.wikipedia.org/wiki/Brainfuck
[gcc]:https://gcc.gnu.org/
[tcc]:https://bellard.org/tcc/
[clang]:https://clang.llvm.org/
[GNU Make]:https://www.gnu.org/software/make/
