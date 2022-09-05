bool readline(size_t *size, char **str, FILE *input_f, FILE *output_f, bool echo);
char *askpass(FILE *input, FILE *output, int tty, const char *prompt, bool echo);
char *askpasstty_(int tty, const char *prompt, bool echo);
char *askpasstty(const char *prompt, bool echo);
