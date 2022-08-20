#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <termios.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
char *askpass(FILE *input, FILE *output, const char *prompt, bool echo) { // some code is from util-linux
	errno = 0;
	int input_fd = fileno(input);
	if (input_fd < 0) return NULL;
	int output_fd = fileno(output);
	if (output_fd < 0) return NULL;
	struct termios term_old, term_new, term_2;
	int input_istty = isatty(input_fd);
	if (input_istty) {
		if (tcgetattr(input_fd, &term_old) != 0) return NULL;
		term_new = term_old;
		// ECHO disables echoing keys the user presses
		// ECHOE echos backspace, ECHOK echos kill, ECHONL echos new line
		term_new.c_lflag |= ICANON | ISIG;
		term_new.c_lflag &= ~ECHOK;
		if (echo) {
			term_new.c_lflag |= ECHO | ECHOCTL | ECHOE | ECHONL;
		} else {
			term_new.c_lflag &= ~(ECHO | ECHOCTL | ECHOE | ECHONL);
		}
		errno = 0;
		if (tcsetattr(input_fd, TCSANOW, &term_new) != 0) return NULL;
		if (tcgetattr(input_fd, &term_2) != 0) return NULL; // check if it has changed
		if (term_2.c_lflag != term_new.c_lflag) return NULL;
	}
	char *pass = NULL;
	size_t n = 0;
	fputs(prompt, output);
	fflush(input);
	fflush(output);
	errno = 0;
	ssize_t len = getline(&pass, &n, input);
	int e = errno;
	if (input_istty && (!echo || feof(input))) fputc('\n', output);
	fflush(input);
	fflush(output);
	if (input_istty) {
		if (tcsetattr(input_fd, TCSANOW, &term_old) != 0) { free(pass); return NULL; }
		if (tcgetattr(input_fd, &term_2) != 0) { free(pass); return NULL; } // check if it has changed
		if (term_2.c_lflag != term_old.c_lflag) { free(pass); return NULL; }
	}
	errno = e;
	if (len < 0 && errno) { free(pass); return NULL; }
	if (len > 0 && pass[len-1] == '\n') pass[len-1] = '\0';
	return pass;
}
