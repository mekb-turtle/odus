#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include "./libaskpass.h"
#define eprintf(...) fprintf(stderr, __VA_ARGS__)
#define strerr (errno == 0 ? "Error" : strerror(errno))
int usage(char *argv0) {
	eprintf("\
Usage: %s [prompt]\n\
	-e --echo  : enables echo\n\
	-t --notty : use stdin/stderr instead of tty\n\
", argv0);
	return 2;
}
int main(int argc, char *argv[]) {
#define INVALID return usage(argv[0]);
	char *prompt = NULL;
	bool flag_done = 0;
	bool echo_flag = 0;
	bool notty_flag = 0;
	for (int i = 1; i < argc; ++i) {
		if (argv[i][0] == '-' && argv[i][1] != '\0' && !flag_done) {
			if (argv[i][1] == '-' && argv[i][2] == '\0') flag_done = 1; else // -- denotes end of flags
			if (strcmp(argv[i], "-e") == 0 || strcmp(argv[i], "--echo") == 0) {
				if (echo_flag) INVALID;
				echo_flag = 1;
			} else
			if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--notty") == 0) {
				if (notty_flag) INVALID;
				notty_flag = 1;
			} else
			INVALID;
		} else {
			if (prompt) INVALID;
			prompt = argv[i];
		}
	}
	if (!prompt) prompt = "Password: ";
	char *input;
	errno = 0;
	if (notty_flag) {
		input = askpass(stdin, stderr, STDIN_FILENO, prompt, echo_flag);
	} else {
		input = askpasstty(prompt, echo_flag);
	}
	if (errno) { eprintf("askpass: %s\n", strerr); return 1; }
	if (!input) return 1;
	if (ferror(stdin) || feof(stdin)) return 2;
	fputs(input, stdout);
	fputc('\n', stdout);
}
