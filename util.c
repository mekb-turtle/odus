#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <shadow.h>
#include <stdint.h>
#include "./libaskpass.h"
#define eprintf(...) fprintf(stderr, __VA_ARGS__)
#define strerr (errno == 0 ? "Error" : strerror(errno))
extern char *util_no_password;
extern int util_password_tries;
extern char *util_invalid_password;
extern char *util_no_more_tries;
bool str_to_long(const char *str, long *out) {
	// if all characters in str are numbers, convert to a long, otherwise leave as is
	bool numeric = 1;
	size_t len = strlen(str);
	for (size_t i = 0; i < len; ++i) {
		if (str[i] < '0' || str[i] > '9') { numeric = 0; break; }
	}
	if (numeric) *out = atol(str);
	return numeric;
}
bool getgrouplist_(const char *user, gid_t group, gid_t **groups_, int *ngroups_) {
	// getgrouplist but we don't know the size yet
	int cur_ngroups = 0;
	int result = -1;
	int ngroups;
	gid_t *groups = NULL;
	while (result == -1) {
		if (groups) free(groups);
		cur_ngroups += 8;
		groups = calloc(cur_ngroups, sizeof(gid_t));
		ngroups = cur_ngroups;
		result = getgrouplist(user, group, groups, &ngroups);
	}
	if (ngroups < 0) return 0;
	*groups_ = groups;
	*ngroups_ = ngroups;
	return 1;
}
bool password_check(struct passwd *pw, char *prompt, bool notty, int tty) {
	char *p = pw->pw_passwd;
	if (p[0] == 'x' && p[1] == '\0') {
		errno = 0;
		struct spwd *spw = getspnam(pw->pw_name);
		if (errno) { eprintf("getspnam: %s\n",          strerr);      return 0; }
		if (!spw)  { eprintf("Cannot find shadow %s\n", pw->pw_name); return 0; }
		p = spw->sp_pwdp;
	}
	if (p[0] == '!' || p[0] == '*') {
		eprintf(util_no_password);
		return 0;
	} else if (p[0] == '\0') {
		return 1;
	}
	errno = 0;
	char *prompt_ = malloc(strlen(prompt) + strlen(pw->pw_name));
	if (errno) { eprintf("malloc: %s\n", strerr); return 0; }
	sprintf(prompt_, prompt, pw->pw_name);
	for (int i = 1; i <= util_password_tries; ++i) {
		errno = 0;
		char *input;
		bool notty_ = 0;
		if (!notty) {
			if (tty < 0) {
				input = askpasstty(prompt_, 0, &notty_);
			} else {
				input = askpasstty_(tty, prompt_, 0, &notty_);
			}
		}
		if (notty_ || notty) {
			input = askpass(stdin, stderr, STDIN_FILENO, prompt_, 0);
		}
		if (errno) { eprintf("askpass: %s\n", strerr); return 0; }
		if (!input) { return 0; }
		if (ferror(stdin) || feof(stdin)) return 0;
		errno = 0;
		char *c = crypt(input, p);
		if (errno) { eprintf("crypt: %s\n", strerr); return 0; }
		if (!c) { eprintf("crypt\n"); return 0; }
		if (strcmp(p, c) == 0) {
			return 1;
		} else {
			eprintf(i == util_password_tries ? util_no_more_tries : util_invalid_password);
			continue;
		}
	}
	return 0;
}
char *clone_string(const char *str) {
	size_t l = strlen(str) + 1;
	errno = 0;
	char *clone = malloc(l);
	if (errno) { eprintf("malloc: %s\n", strerr); return NULL; }
	memcpy(clone, str, l);
	if (errno) { eprintf("memcpy: %s\n", strerr); return NULL; }
	return clone;
}
// copy the content of the pointer to another pointer because
// getpwuid etc will reuse the same pointer
struct passwd *clone_passwd(struct passwd *ptr) {
	errno = 0;
	struct passwd *clone = malloc(sizeof(struct passwd));
	if (errno) { eprintf("malloc: %s\n", strerr); return NULL; }
	if (!(clone->pw_name   = clone_string(ptr->pw_name)))   { free(clone); return NULL; }
	if (!(clone->pw_passwd = clone_string(ptr->pw_passwd))) { free(clone); return NULL; }
	clone->pw_uid = ptr->pw_uid;
	clone->pw_gid = ptr->pw_gid;
	if (!(clone->pw_gecos  = clone_string(ptr->pw_gecos)))  { free(clone); return NULL; }
	if (!(clone->pw_dir    = clone_string(ptr->pw_dir)))    { free(clone); return NULL; }
	if (!(clone->pw_shell  = clone_string(ptr->pw_shell)))  { free(clone); return NULL; }
	return clone;
}
