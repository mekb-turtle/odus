#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <termios.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <paths.h>
#include "./libaskpass.h"
extern char *libaskpass_password_color;
extern size_t libaskpass_max_length;
extern bool libaskpass_use_char;
extern char *libaskpass_password_char;
char *notation(char c, char *out) {
	if (c == 0x7f && c == 0xff) {
		sprintf(out, "^?");
	} else if (c > 0x7f) {
		if (c >= 0xa0) {
			sprintf(out, "M-%c", c-0x80);
		} else {
			sprintf(out, "M-^%c", c-0x40);
		}
	} else if (c != '\n' && c < 0x20) {
		sprintf(out, "^%c", c+0x40);
	} else {
		sprintf(out, "%c", c);
	}
	return out;
}
bool readline(size_t *size, char **str, FILE *input_f, FILE *output_f, bool echo) {
	char *input = NULL;
	size_t len = 0;
	bool bksp = 0;
	char *notation_ = malloc(8);
	if (!notation_) return 0;
	fputs(libaskpass_password_color, output_f);
	while (1) {
		if (feof(input_f)) break;
		if (ferror(input_f)) {
			if (input) free(input);
			fputs("\x1b[0m", output_f);
			fflush(output_f);
			return 0;
		}
		input = realloc(input, len + 2);
		int c = fgetc(input_f);
		if (c <= 0) continue;
		input[len] = c;
		fflush(output_f);
		bksp = 0;
		if (c == '\x0d' || c == '\x0a' || c == '\x04') break;
		if (c == '\x03' || c == '\x1a' || c == '\x11') {
			if (input) free(input);
			fputs("\x1b[0m", output_f);
			fflush(output_f);
			return 0;
		}
		else if (c == '\x7f') { if (len > 0) { --len; bksp = 1; }}
		else if (len >= libaskpass_max_length) continue;
		else ++len;
		if (echo) {
			switch (c) {
				case '\x7f':
					if (!bksp) break;
					c = input[len];
					for (size_t i = 0; i < strlen(notation(c, notation_)); ++i) {
						fputs("\x08 \x08", output_f);
					}
					break;
				default:
					fputs(notation(c, notation_), output_f);
					break;
			}
		} else if (libaskpass_use_char) {
			switch (c) {
				case '\x7f':
					if (!bksp) break;
					for (size_t i = 0; i < strlen(libaskpass_password_char); ++i) {
						fputs("\x08 \x08", output_f);
					}
					break;
				default:
					fputs(libaskpass_password_char, output_f);
					break;
			}
		}
		fflush(output_f);
	}
	for (size_t j = 0; j < len; ++j) {
		for (size_t i = 0; i < strlen(libaskpass_password_char); ++i) {
			fputs("\x08 \x08", output_f);
		}
	}
	input[len] = 0;
	*str = input;
	*size = len;
	fputs("\x1b[0m", output_f);
	fflush(output_f);
	return 1;
}
char *askpass(FILE *input, FILE *output, int tty, const char *prompt, bool echo) { // some code is from util-linux
	if (tty < 0 || !input || !output) return NULL;
	errno = 0;
	struct termios term_old, term_new, term_2;
	int istty = isatty(tty);
	if (istty) {
		if (tcgetattr(tty, &term_old) != 0) return NULL;
		term_new = term_old;
		// ECHO echos keys the user presses
		// ECHOE echos backspace, ECHOK echos kill, ECHONL echos new line
		term_new.c_iflag &= ~(IGNCR | ICRNL);
		term_new.c_lflag &= ~(ISIG | ICANON | ECHOK | ECHO | ECHOCTL | ECHOE | ECHONL | IEXTEN);
		errno = 0;
		if (tcsetattr(tty, TCSANOW, &term_new) != 0) return NULL;
		if (tcgetattr(tty, &term_2) != 0) return NULL; // check if it has changed
		if (term_2.c_iflag != term_new.c_iflag) return NULL;
		if (term_2.c_lflag != term_new.c_lflag) return NULL;
	}
	char *pass = NULL;
	size_t len;
	fflush(input); fflush(output);
	fputs(prompt, output);
	fflush(input); fflush(output);
	errno = 0;
	bool res = readline(&len, &pass, input, output, echo);
	int e = errno;
	if (istty) fputc('\n', output);
	fflush(input); fflush(output);
	if (istty) {
		if (tcsetattr(tty, TCSANOW, &term_old) != 0) { free(pass); return NULL; }
		if (tcgetattr(tty, &term_2) != 0) { free(pass); return NULL; } // check if it has changed
		if (term_2.c_iflag != term_old.c_iflag) { free(pass); return NULL; }
		if (term_2.c_lflag != term_old.c_lflag) { free(pass); return NULL; }
	}
	errno = e;
	if (!res) { free(pass); return NULL; }
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
