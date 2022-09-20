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
#include <paths.h>
#include "./config.h"
#include "./libaskpass.h"
#include "./util.h"
#define eprintf(...) fprintf(stderr, __VA_ARGS__)
#define strerr (errno == 0 ? "Error" : strerror(errno))
//#define DEBUG
extern char **environ;
int usage(char *argv0) {
	eprintf("\
Usage: %s [options] [command] [argv]...\n\
	-k --keep : keep all environment variables\n\
	-r --reset : reset all environment variables\n\
		HOME, USER, LOGNAME and PWD are set by default\n\
	-c --cwd : set current working directory to the home directory\n\
	-u --user [user] : the user to run as, defaults to UID 0, which is usually root\n\
		if numeric, it'll find a user with the UID, otherwise with that name\n\
	-t --notty : use stdin/stderr instead of tty for password input\n\
", argv0);
	return 2;
}
int main(int argc, char *argv[]) {
#define INVALID return usage(argv[0]);
	if (geteuid() != 0) {
		eprintf("Must run as setuid root\n"); return 1;
	}
	if (argc <= 1) INVALID;
	char *cmd_argv[argc];
	int cmd_argc = 0;
	char *user_str = NULL;
	bool user_flag = 0;
	bool keep_flag = 0;
	bool reset_flag = 0;
	bool cwd_flag = 0;
	bool notty_flag = 0;
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
			if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--reset") == 0) {
				if (reset_flag) INVALID;
				reset_flag = 1;
			} else
			if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--cwd") == 0) {
				if (cwd_flag) INVALID;
				cwd_flag = 1;
			} else
			if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--notty") == 0) {
				if (notty_flag) INVALID;
				notty_flag = 1;
			} else
			INVALID;
		} else {
			flag_done = 1;
			cmd_argv[cmd_argc++] = argv[i]; // add to argv
		}
	}
	if (keep_flag && reset_flag) INVALID;
	if (user_flag) INVALID;
	if (cmd_argc <= 0) INVALID;
	cmd_argv[cmd_argc] = NULL;
	bool user_numeric = 1;
	uid_t user_id;
	if (user_str) { long a; user_numeric = str_to_number(user_str, &a); user_id = a; }
	else user_id = 0;
	struct passwd *pwd;
	errno = 0;
	if (user_numeric) {
		pwd = getpwuid(user_id);
		if (errno) { eprintf("getpwuid: %s\n",           strerr);   return errno; }
		if (!pwd)  { eprintf(odus_cannot_find_user_id,   user_id);  return 1; }
	} else {
		pwd = getpwnam(user_str);
		if (errno) { eprintf("getpwnam: %s\n",           strerr);   return errno; }
		if (!pwd)  { eprintf(odus_cannot_find_user_name, user_str); return 1; }
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
#endif
	if (reset_flag) {
		clearenv();
	} else if (!keep_flag) {
		for (size_t i = 0; environ[i]; ++i) {
			errno = 0;
			char *o = environ[i];
			char *eq = strchr(o, '=');
			if (eq) {
				size_t l = eq - o;
				bool b = 0;
				odus_env_keep_define;
				for (size_t j = 0; odus_env_keep[j]; ++j) {
					if (memcmp(o, odus_env_keep[j], l) == 0) {
						b = 1;
						break;
					}
				}
				if (b) continue;
				char *e = malloc(l + 1);
				if (errno) { eprintf("malloc: %s\n", strerr); return errno; }
				memcpy(e, o, l);
				e[l] = '\0';
				o = e;
			}
			errno = 0;
			unsetenv(o);
			if (eq) free(o);
			if (errno) { eprintf("unsetenv: %s\n", strerr); return errno; }
		}
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
			if (strcmp(grp->gr_name, odus_group) == 0) {
				has_group = 1;
				break;
			}
		}
		free(groups);
		if (!has_group) {
			eprintf("%s is not in group %s\n", my_pwd->pw_name, odus_group);
			return 1;
		}
		errno = 0;
		if (password_check(my_pwd, odus_prompt, notty_flag, -1) != 1) {
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
		chdir("/");
		if (errno) { eprintf("chdir: %s\n", strerr); return errno; }
		chdir(pwd->pw_dir);
		if (errno) { eprintf("chdir: %s\n", strerr); }
	}
	errno = 0;
	char *cwd = malloc(PATH_MAX);
	if (errno) { eprintf("malloc: %s\n", strerr); return errno; }
	getcwd(cwd, PATH_MAX);
	if (errno) { eprintf("getcwd: %s\n", strerr); return errno; }
	setenv("PWD", cwd, 1);
	setenv("HOME", pwd->pw_dir, 1);
	setenv("USER", pwd->pw_name, 1);
	setenv("LOGNAME", pwd->pw_name, 1);
	if (!keep_flag) {
		setenv("PATH", odus_default_path, 1);
	}
	errno = 0;
	execvp(cmd_argv[0], cmd_argv);
	eprintf(odus_execvp_error, cmd_argv[0], strerr);
	return 1;
}
