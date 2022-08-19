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
#include "./util.h"
#define USERNAME_PROMPT "Username: "
#define PASSWORD_PROMPT "Password for %s: "
#define DEFAULT_PATH "/bin:/usr/bin"
#define eprintf(...) fprintf(stderr, __VA_ARGS__)
#define strerr (errno == 0 ? "Error" : strerror(errno))
//#define DEBUG
int usage(char *argv0) {
	eprintf("\
Usage: %s\n\
", argv0);
	return 2;
}
int main(int argc, char *argv[]) {
#define INVALID return usage(argv[0]);
	if (geteuid() != 0 || getuid() != 0) {
		eprintf("Must run as root\n"); return 1;
	}
	if (argc != 1) INVALID;
	char *username = askpass(stdin, stderr, USERNAME_PROMPT, 1);
	if (feof(stdin)) return 1;
	if (!username) return 1;
	if (username[0] == '\0') return 1;
	struct passwd *pwd;
	errno = 0;
	pwd = getpwnam(username);
	if (errno) { eprintf("getpwnam: %s\n", strerr); return errno; }
	if (!pwd)  { eprintf("User does not exist\n");  return 1; }
	pwd = clone_passwd(pwd);
#ifdef DEBUG
	printf("user: %s\n", pwd->pw_name);
	printf("uid: %i\n", pwd->pw_uid);
	printf("gid: %i\n", pwd->pw_gid);
#endif
	clearenv();
	errno = 0;
	if (password_check(pwd, PASSWORD_PROMPT) != 1) {
		if (errno) return errno;
		return 1;
	}
	gid_t *groups;
	int ngroups;
	if (!getgrouplist_(pwd->pw_name, pwd->pw_gid, &groups, &ngroups)) {
		eprintf("Failed to get groups of %s\n", pwd->pw_name); return 1;
	}
	errno = 0; if (setgroups(ngroups, groups) != 0) { eprintf("setgroups: %s\n", strerr); return errno || 1; }
	errno = 0; if (setgid(pwd->pw_gid)        != 0) { eprintf("setgid: %s\n",    strerr); return errno || 1; }
	errno = 0; if (setegid(pwd->pw_gid)       != 0) { eprintf("setegid: %s\n",   strerr); return errno || 1; }
	errno = 0; if (setuid(pwd->pw_uid)        != 0) { eprintf("setuid: %s\n",    strerr); return errno || 1; }
	errno = 0; if (seteuid(pwd->pw_uid)       != 0) { eprintf("seteuid: %s\n",   strerr); return errno || 1; }
	errno = 0;
	chdir("/");
	if (errno) { eprintf("chdir: %s\n", strerr); return errno; }
	chdir(pwd->pw_dir);
	if (errno) { eprintf("chdir: %s\n", strerr); }
	char *cwd = malloc(PATH_MAX);
	if (errno) { eprintf("malloc: %s\n", strerr); return errno; }
	getcwd(cwd, PATH_MAX);
	if (errno) { eprintf("getcwd: %s\n", strerr); return errno; }
	setenv("PWD", cwd, 1);
	setenv("HOME", pwd->pw_dir, 1);
	setenv("USER", pwd->pw_name, 1);
	setenv("PATH", DEFAULT_PATH, 1);
	errno = 0;
	execvp(pwd->pw_shell, (char*[]) { pwd->pw_shell, NULL });
	eprintf("execvp: %s: %s\n", pwd->pw_shell, strerr);
	return 1;
}
