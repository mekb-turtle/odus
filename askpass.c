#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "./libaskpass.h"
#define eprintf(...) fprintf(stderr, __VA_ARGS__)
#define strerr (errno == 0 ? "Error" : strerror(errno))
int usage(char *argv0) {
	eprintf("\
Usage: %s [prompt]\n\
", argv0);
	return 2;
}
int main(int argc, char *argv[]) {
#define INVALID return usage(argv[0]);
	char *prompt = NULL;
	bool flag_done;
	for (int i = 1; i < argc; ++i) {
		if (argv[i][0] == '-' && argv[i][1] != '\0' && !flag_done) {
			if (argv[i][1] == '-' && argv[i][2] == '\0') flag_done = 1; else INVALID; // -- denotes end of flags
		} else {
			if (prompt) INVALID;
			prompt = argv[i];
		}
	}
	if (!prompt) prompt = "Password: ";
	errno = 0;
	char *input = askpass(stdin, stderr, prompt, 0);
	if (errno) { eprintf("askpass: %s\n", strerr); return 1; }
	if (!input) return 1;
	if (ferror(stdin) || feof(stdin)) return 2;
	fputs(input, stdout);
	fputc('\n', stdout);
}
