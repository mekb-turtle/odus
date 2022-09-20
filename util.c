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
#include <limits.h>
#include "./config.h"
#include "./libaskpass.h"
#define eprintf(...) fprintf(stderr, __VA_ARGS__)
#define strerr (errno == 0 ? "Error" : strerror(errno))
bool str_to_number(const char *str, long *out) {
	// if all characters in str are numbers, convert to a long, otherwise leave as is
	size_t len = strlen(str);
	if (len > 32) return 0;
	for (size_t i = 0; i < len; ++i) {
		if (str[i] < '0' || str[i] > '9') return 0;
	}
	char *end = NULL;
	errno = 0;
	long o = strtol(str, &end, 10); // strtonum isn't on linux
	if (errno) {
		if (errno == ERANGE) return 0;
		eprintf("strtonum: %s\n", strerr);
		return 0;
	}
	*out = o;
	return 1;
}
bool getgrouplist_(const char *user, gid_t group, gid_t **groups_, int *ngroups_) {
	// getgrouplist but we don't know the size yet
	int ngroups;
	gid_t *groups = calloc(NGROUPS_MAX, sizeof(gid_t));
	if (!groups) {
		eprintf("calloc: %s\n", strerr);
		return 0;
	}
	ngroups = NGROUPS_MAX;
	int result = getgrouplist(user, group, groups, &ngroups);
	if (result == -1 && ngroups > 0) {
		groups = realloc(groups, ngroups * sizeof(gid_t));
		result = getgrouplist(user, group, groups, &ngroups);
	}
	if (result < 0) {
		eprintf("getgrouplist: %s\n", strerr);
		free(groups);
		return 0;
	}
	*groups_ = groups;
	*ngroups_ = ngroups;
	return 1;
}
bool password_check(struct passwd *pw, char *prompt, bool notty, int tty) {
	char *p = pw->pw_passwd;
	struct spwd *spw = NULL;
	if (p[0] == 'x' && p[1] == '\0') {
		errno = 0;
		spw = getspnam(pw->pw_name);
		if (errno) { eprintf("getspnam: %s\n",          strerr);      return 0; }
		if (!spw)  { eprintf("Cannot find shadow %s\n", pw->pw_name); return 0; }
		p = spw->sp_pwdp;
	}
#define CLEAR() {\
		if (spw) {\
			explicit_bzero(spw->sp_namp, strlen(spw->sp_namp));\
			explicit_bzero(spw->sp_pwdp, strlen(spw->sp_pwdp));\
			explicit_bzero(spw, sizeof(struct spwd));\
		}}
	if (p[0] == '!' || p[0] == '*') {
		CLEAR();
		eprintf(util_no_password);
		return 0;
	} else if (p[0] == '\0') {
		CLEAR();
		return 1;
	}
	errno = 0;
	char *prompt_ = malloc(strlen(prompt) + strlen(pw->pw_name));
	if (errno) { eprintf("malloc: %s\n", strerr); CLEAR(); return 0; }
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
		if (errno) { eprintf("askpass: %s\n", strerr); CLEAR(); free(prompt_); return 0; }
		if (!input) { CLEAR(); free(prompt_); return 0; }
		if (ferror(stdin) || feof(stdin)) { CLEAR(); free(prompt_); return 0; }
		errno = 0;
		char *c = crypt(input, p);
		if (!c) { eprintf("crypt: %s\n", strerr); CLEAR(); free(prompt_); return 0; }
		if (strcmp(p, c) == 0) {
			CLEAR();
			explicit_bzero(c, strlen(c));
			free(prompt_);
			return 1;
		} else {
			explicit_bzero(c, strlen(c));
			eprintf(i == util_password_tries ? util_no_more_tries : util_invalid_password);
			continue;
		}
	}
	CLEAR();
	free(prompt_);
	return 0;
}
char *clone_string(const char *str) {
	size_t l = strlen(str) + 1;
	errno = 0;
	char *clone = malloc(l);
	if (!clone) { eprintf("malloc: %s\n", strerr); return NULL; }
	memcpy(clone, str, l);
	if (errno) { eprintf("memcpy: %s\n", strerr); free(clone); return NULL; }
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
