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
#include "./libaskpass.h"
#define ODUS_GROUP "odus"
#define PROMPT "[odus] password for %s: "
#define DEFAULT_PATH "/bin:/sbin:/usr/bin:/usr/sbin"
#define eprintf(...) fprintf(stderr, __VA_ARGS__)
#define strerr (errno == 0 ? "Error" : strerror(errno))
//#define DEBUG
int usage(char *argv0) {
	eprintf("\
Usage: %s [options] [command] [argv]...\n\
	-k --keep : keep environment variables\n\
		HOME, USER, and PWD are set by default\n\
	-c --cwd : set current working directory to the home directory\n\
	-u --user [user] : the user to run as, defaults to UID 0, which is usually root\n\
		if numeric, it'll find a user with the UID, otherwise with that name\n\
", argv0);
	return 2;
}
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
#ifndef DEBUG
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
bool password_check(struct passwd *pw) {
	char *p = pw->pw_passwd;
	if (p[0] == 'x' && p[1] == '\0') {
		errno = 0;
		struct spwd *spw = getspnam(pw->pw_name);
		if (errno) { eprintf("getspnam: %s\n",          strerr);      return 0; }
		if (!spw)  { eprintf("Cannot find shadow %s\n", pw->pw_name); return 0; }
		p = spw->sp_pwdp;
	}
	if (p[0] == '!' || p[0] == '*') {
		eprintf("Permission denied\n");
		return 0;
	} else if (p[0] == '\0') {
		return 1;
	}
	errno = 0;
	char *prompt = malloc(100);
	if (errno) { eprintf("malloc: %s\n", strerr); return 0; }
	sprintf(prompt, PROMPT, pw->pw_name);
	for (int i = 0; i < 3; ++i) {
		errno = 0;
		char *input = askpass(stdin, stderr, prompt);
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
			eprintf("Invalid password\n");
			continue;
		}
	}
	return 0;
}
#endif
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
int main(int argc, char *argv[]) {
#define INVALID return usage(argv[0]);
#ifndef DEBUG
	if (geteuid() != 0) {
		eprintf("Must run as setuid root\n"); return 1;
	}
#endif
	if (argc <= 1) INVALID;
	char *cmd_argv[argc];
	int cmd_argc = 0;
	char *user_str = NULL;
	bool user_flag = 0;
	bool keep_flag = 0;
	bool cwd_flag = 0;
	bool flag_done = 0;
	for (int i = 1; i < argc; ++i) {
		if (user_flag) {
			user_str = argv[i];
			user_flag = 0;
		} else if (argv[i][0] == '-' && argv[i][1] != '\0' && !flag_done) {
			if (argv[i][1] == '-' && argv[i][2] == '\0') flag_done = 1; else // -- denotes end of flags
			if (strcmp(argv[i], "-u") == 0 || strcmp(argv[i], "--user") == 0) {
				if (user_str) INVALID;
				user_flag = 1;
			} else
			if (strcmp(argv[i], "-k") == 0 || strcmp(argv[i], "--keep") == 0) {
				if (keep_flag) INVALID;
				keep_flag = 1;
			} else
			if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--cwd") == 0) {
				if (cwd_flag) INVALID;
				cwd_flag = 1;
			} else
			INVALID;
		} else {
			flag_done = 1;
			cmd_argv[cmd_argc++] = argv[i]; // add to argv
		}
	}
	if (cmd_argc <= 0) INVALID;
	cmd_argv[cmd_argc] = NULL;
	bool user_numeric = 1;
	uid_t user_id;
	if (user_str) { long a; user_numeric = str_to_long(user_str, &a); user_id = a; }
	else user_id = 0;
	struct passwd *pwd;
	errno = 0;
	if (user_numeric) {
		pwd = getpwuid(user_id);
		if (errno) { eprintf("getpwuid: %s\n",        strerr);   return errno; }
		if (!pwd)  { eprintf("Cannot find user %i\n", user_id);  return 1; }
	} else {
		pwd = getpwnam(user_str);
		if (errno) { eprintf("getpwnam: %s\n",        strerr);   return errno; }
		if (!pwd)  { eprintf("Cannot find user %s\n", user_str); return 1; }
	}
	pwd = clone_passwd(pwd);
#ifdef DEBUG
	printf("user: %s\n", pwd->pw_name);
	printf("uid: %i\n", pwd->pw_uid);
	printf("gid: %i\n", pwd->pw_gid);
	printf("argc: %i\n", cmd_argc);
	for (int i = 0; i < cmd_argc; ++i) {
		printf("argv[%i]: %s\n", i, cmd_argv[i]);
	}
#else
	if (!keep_flag) {
		clearenv();
	}
	uid_t my_uid = getuid();
	errno = 0;
	struct passwd *my_pwd = getpwuid(my_uid);
	if (errno)   { eprintf("getpwuid: %s\n",        strerr); return errno; }
	if (!my_pwd) { eprintf("Cannot find user %i\n", my_uid); return 1; }
	my_pwd = clone_passwd(my_pwd);
	gid_t *groups;
	int ngroups;
	if (my_uid != 0) {
		if (!getgrouplist_(my_pwd->pw_name, my_pwd->pw_gid, &groups, &ngroups)) {
			eprintf("Failed to get groups of %s\n", my_pwd->pw_name); return 1;
		}
		bool has_group = 0;
		for (int i = 0; i < ngroups; ++i) {
			errno = 0;
			struct group *grp = getgrgid(groups[i]);
			if (errno) { eprintf("getgrgid: %s\n",         strerr); return errno; }
			if (!grp)  { eprintf("Cannot find group %i\n", my_uid); return 1; }
			if (strcmp(grp->gr_name, ODUS_GROUP) == 0) {
				has_group = 1;
				break;
			}
		}
		free(groups);
		if (!has_group) {
			eprintf("%s is not in group %s\n", my_pwd->pw_name, ODUS_GROUP);
			return 1;
		}
		errno = 0;
		if (password_check(my_pwd) != 1) {
			if (errno) return errno;
			return 1;
		}
	}
	if (!getgrouplist_(pwd->pw_name, pwd->pw_gid, &groups, &ngroups)) {
		eprintf("Failed to get groups of %s\n", pwd->pw_name); return 1;
	}
	errno = 0; if (setgroups(ngroups, groups) != 0) { eprintf("setgroups: %s\n", strerr); return errno || 1; }
	errno = 0; if (setgid(pwd->pw_gid)        != 0) { eprintf("setgid: %s\n",    strerr); return errno || 1; }
	errno = 0; if (setegid(pwd->pw_gid)       != 0) { eprintf("setegid: %s\n",   strerr); return errno || 1; }
	errno = 0; if (setuid(pwd->pw_uid)        != 0) { eprintf("setuid: %s\n",    strerr); return errno || 1; }
	errno = 0; if (seteuid(pwd->pw_uid)       != 0) { eprintf("seteuid: %s\n",   strerr); return errno || 1; }
	errno = 0;
	if (cwd_flag) {
		chdir(pwd->pw_dir);
		if (errno) { eprintf("chdir: %s\n",  strerr); return errno; }
		setenv("PWD", pwd->pw_dir, 1);
	} else {
		char *cwd = malloc(PATH_MAX);
		if (errno) { eprintf("malloc: %s\n", strerr); return errno; }
		getcwd(cwd, PATH_MAX);
		if (errno) { eprintf("getcwd: %s\n", strerr); return errno; }
		setenv("PWD", cwd, 1);
	}
	setenv("HOME", pwd->pw_dir, 1);
	setenv("USER", pwd->pw_name, 1);
	if (!keep_flag) {
		setenv("PATH", DEFAULT_PATH, 1);
	}
	errno = 0;
	execvp(cmd_argv[0], cmd_argv);
	eprintf("execvp: %s: %s\n", cmd_argv[0], strerr);
#endif
	return 1;
}
