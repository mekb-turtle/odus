#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <shadow.h>
#define ODUS_GROUP "odus"
#define eprintf(...) fprintf(stderr, __VA_ARGS__)
#define strerr strerror(errno)
//#define DEBUG
int usage(char *argv0) {
	eprintf("\
Usage: %s [options] [command] [argv]...\n\
	-l --login : set HOME and USER environment variables\n\
	-u --user [user] : the user to run as, defaults to UID 0, which is usually root\n\
		if numeric, it'll find a user with the UID, otherwise with that name\n\
", argv0);
	return 2;
}
bool to_name_or_id(char *str, long *out) {
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
bool getgrouplist_(char *user, gid_t group, gid_t **groups_, int *ngroups_) {
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
	char *prompt = malloc(100);
	sprintf(prompt, "Password for %s: ", pw->pw_name);
	for (int i = 0; i < 3; ++i) {
		errno = 0;
		char *input = getpass(prompt); // temporary, this function is obsolete
		if (errno) { eprintf("getpass: %s\n", strerr); return 0; }
		errno = 0;
		char *c = crypt(input, p);
		if (errno) { eprintf("crypt: %s\n", strerr); return 0; }
		if (!c) { eprintf("crypt\n"); return 0; }
		if (strcmp(p, c) == 0) {
			return 1;
		} else {
			eprintf("Invalid password\n");
			if (input[0] == '\0') return 0;
			continue;
		}
	}
	eprintf("Permission denied\n");
	return 0;
}
#endif
int main(int argc, char *argv[]) {
#define INVALID return usage(argv[0]);
#ifndef DEBUG
	if (geteuid() != 0) {
		eprintf("Must run as setuid root\n"); return 1;
	}
#endif
	if (argc <= 1) INVALID;
	char *user_str = NULL;
	char *cmd_argv[argc];
	int cmd_argc = 0;
	bool user_flag = 0;
	bool login_flag = 0;
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
			if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--login") == 0) {
				if (login_flag) INVALID;
				login_flag = 1;
			} else
			INVALID;
		} else {
			flag_done = 1;
			cmd_argv[cmd_argc++] = argv[i]; // add to argv
		}
	}
	if (user_flag) INVALID;
	if (cmd_argc <= 0) INVALID;
	cmd_argv[cmd_argc] = NULL;
	bool user_numeric = 1;
	uid_t user_id;
	long a;
	if (user_str) { user_numeric = to_name_or_id(user_str, &a); user_id = a; }
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
#ifdef DEBUG
	printf("user: %s\n", pwd->pw_name);
	printf("uid: %i\n", pwd->pw_uid);
	printf("gid: %i\n", pwd->pw_gid);
	printf("argc: %i\n", cmd_argc);
	for (int i = 0; i < cmd_argc; ++i) {
		printf("argv[%i]: %s\n", i, cmd_argv[i]);
	}
#else
	uid_t my_uid = getuid();
	errno = 0;
	struct passwd *pwd_ = getpwuid(my_uid);
	if (errno) { eprintf("getpwuid: %s\n",        strerr); return errno; }
	if (!pwd_) { eprintf("Cannot find user %i\n", my_uid); return 1; }
	gid_t *groups;
	int ngroups;
	if (!getgrouplist_(pwd_->pw_name, pwd_->pw_gid, &groups, &ngroups)) {
		eprintf("Failed to get your groups\n"); return 1;
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
	if (!has_group) {
		eprintf("%s is not in group %s\n", pwd_->pw_name, ODUS_GROUP);
		return 1;
	}
	errno = 0;
	if (password_check(pwd_) != 1) {
		if (errno) return errno;
		return 1;
	}
	errno = 0; if (setuid(pwd->pw_uid)  != 0) { eprintf("setuid: %s\n",  strerr); return errno; }
	errno = 0; if (setgid(pwd->pw_gid)  != 0) { eprintf("setgid: %s\n",  strerr); return errno; }
	errno = 0; if (seteuid(pwd->pw_uid) != 0) { eprintf("seteuid: %s\n", strerr); return errno; }
	errno = 0; if (setegid(pwd->pw_gid) != 0) { eprintf("setegid: %s\n", strerr); return errno; }
	if (login_flag) {
		setenv("HOME", pwd->pw_dir, 1);
		setenv("USER", pwd->pw_name, 1);
	}
	execvp(cmd_argv[0], cmd_argv);
	eprintf("execvp: %s\n", strerr);
#endif
	return 0;
}
