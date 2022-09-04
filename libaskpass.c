#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <termios.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <paths.h>
char *askpass(FILE *input, FILE *output, int tty, const char *prompt, bool echo) { // some code is from util-linux
	if (tty < 0 || !input || !output) return NULL;
	errno = 0;
	struct termios term_old, term_new, term_2;
	int input_istty = isatty(tty);
	if (input_istty) {
		if (tcgetattr(tty, &term_old) != 0) return NULL;
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
		if (tcsetattr(tty, TCSANOW, &term_new) != 0) return NULL;
		if (tcgetattr(tty, &term_2) != 0) return NULL; // check if it has changed
		if (term_2.c_lflag != term_new.c_lflag) return NULL;
	}
	char *pass = NULL;
	size_t n = 0;
	fflush(input); fflush(output);
	fputs(prompt, output);
	fflush(input); fflush(output);
	errno = 0;
	ssize_t len = getline(&pass, &n, input);
	int e = errno;
	if (input_istty && (!echo || feof(input))) fputc('\n', output);
	fflush(input); fflush(output);
	if (input_istty) {
		if (tcsetattr(tty, TCSANOW, &term_old) != 0) { free(pass); return NULL; }
		if (tcgetattr(tty, &term_2) != 0) { free(pass); return NULL; } // check if it has changed
		if (term_2.c_lflag != term_old.c_lflag) { free(pass); return NULL; }
	}
	errno = e;
	if (len < 0 && errno) { free(pass); return NULL; }
	if (len > 0 && pass[len-1] == '\n') pass[len-1] = '\0';
	return pass;
}
char *askpasstty_(int tty, const char *prompt, bool echo) {
	if (tty < 0) return NULL;
	//FILE *ttyfile = fdopen(tty, "rwb");
	FILE *input = fdopen(tty, "rb");
	FILE *output = fdopen(tty, "wb");
	char *res = askpass(input, output, tty, prompt, echo);
	return res;
}
char *askpasstty(const char *prompt, bool echo) {
	errno = 0;
	int tty = open(_PATH_TTY, O_RDWR|O_CLOEXEC);
	return askpasstty_(tty, prompt, echo);
}
