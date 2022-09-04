char *askpass(FILE *input, FILE *output, int tty, const char *prompt, bool echo);
char *askpasstty_(int tty, const char *prompt, bool echo);
char *askpasstty(const char *prompt, bool echo);
