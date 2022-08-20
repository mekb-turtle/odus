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
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "./libaskpass.h"
#include "./util.h"
#define USERNAME_PROMPT "Username: "
#define PASSWORD_PROMPT "Password for %s: "
#define AUTOLOGIN_TEXT "Autologin: %s\n"
#define DEFAULT_PATH "/bin:/usr/bin"
#define eprintf(...) fprintf(stderr, __VA_ARGS__)
#define strerr (errno == 0 ? "Error" : strerror(errno))
//#define DEBUG
int usage(char *argv0) {
	eprintf("\
Usage: %s\n\
	-a --login [user] : login as a specific user by default instead of letting the user decide\n\
	-n --nopassword : skip asking for a password if --login is supplied\n\
	-t --term [term] : set TERM variable, default is 'linux'\n\
	-y --tty [tty] : set the tty to use, must start with '/dev/tty'\n\
", argv0);
	return 2;
}
int main(int argc, char *argv[]) {
#define INVALID return usage(argv[0]);
	if (geteuid() != 0 || getuid() != 0) {
		eprintf("Must run as root\n"); return 1;
	}
	bool flag_done = 0;
	char *login_user = NULL;
	bool login_flag = 0;
	bool nopassword_flag = 0;
	char *term = NULL;
	bool term_flag = 0;
	char *tty = NULL;
	bool tty_flag = 0;
	for (int i = 1; i < argc; ++i) {
		if (login_flag) {
			login_user = argv[i];
			login_flag = 0;
		} else if (term_flag) {
			term = argv[i];
			term_flag = 0;
		} else if (tty_flag) {
			if (strncmp(argv[i], "/dev/tty", 8) != 0) INVALID;
			tty = argv[i];
			tty_flag = 0;
		} else if (argv[i][0] == '-' && argv[i][1] != '\0' && !flag_done) {
			if (argv[i][1] == '-' && argv[i][2] == '\0') flag_done = 1; else // -- denotes end of flags
			if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--login") == 0) {
				if (login_user) INVALID;
				login_flag = 1;
			} else
			if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--term") == 0) {
				if (term) INVALID;
				term_flag = 1;
			} else
			if (strcmp(argv[i], "-y") == 0 || strcmp(argv[i], "--tty") == 0) {
				if (tty) INVALID;
				tty_flag = 1;
			} else
			if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--nopassword") == 0) {
				if (nopassword_flag) INVALID;
				nopassword_flag = 1;
			} else
			INVALID;
		} else INVALID;
	}
	if (login_user && nopassword_flag) INVALID;
	if (login_flag) INVALID;
	if (term_flag) INVALID;
	if (tty_flag) INVALID;
	if (!tty) INVALID;
	if (!term) term = "linux";
	errno = 0;
	// some of this tty code is from suckless ubase getty
	pid_t f = fork();
	if (f == -1) { eprintf("fork: %s\n", strerr); return errno; }
	if (f == 0) {
		int fd  = open(tty, O_RDWR);
		int fd2 = open(tty, O_RDWR);
		if (fd < 0 || fd2 < 0) { eprintf("open: %s: %s\n", strerr, tty); return errno; } errno = 0;
		if (isatty(fd)   == 0) { eprintf("%s is not a tty\n",      tty); return errno; } errno = 0;
		if (fchown(fd, 0, 0)            < 0) { eprintf("fchown: %i: %s\n", fd, strerr); return errno; }
		if (fchmod(fd, S_IWUSR|S_IRUSR) < 0) { eprintf("fchmod: %i: %s\n", fd, strerr); return errno; }
		if (ioctl(fd, TIOCSCTTY, (void*)1) != 0) { eprintf("ioctl: TIOCSCTTY: %s\n", strerr); } errno = 0;
		dup2(fd, 0); dup2(fd, 1); dup2(fd, 2);
		if (fd > 2) close(fd);
		char *username;
		if (login_user) {
			username = login_user;
			eprintf(AUTOLOGIN_TEXT, username);
		} else {
			username = askpass(stdin, stderr, USERNAME_PROMPT, 1);
			if (feof(stdin)) return 1;
		}
		if (!username) return 1;
		if (username[0] == '\0') return 1;
		errno = 0;
		struct passwd *pwd = getpwnam(username);
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
		if (!nopassword_flag) {
			if (password_check(pwd, PASSWORD_PROMPT) != 1) {
				if (errno) return errno;
				return 1;
			}
		}
		gid_t *groups;
		int ngroups;
		if (!getgrouplist_(pwd->pw_name, pwd->pw_gid, &groups, &ngroups)) {
			eprintf("Failed to get groups of %s\n", pwd->pw_name); return 1;
		}
		if (fchown(fd2, 1, 0)            < 0) { eprintf("fchown: %i: %s\n", fd2, strerr); return errno; }
		if (fchmod(fd2, S_IWUSR|S_IRUSR) < 0) { eprintf("fchmod: %i: %s\n", fd2, strerr); return errno; }
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
		setenv("TERM", term, 1);
		errno = 0;
		execvp(pwd->pw_shell, (char*[]) { pwd->pw_shell, NULL });
		eprintf("execvp: %s: %s\n", pwd->pw_shell, strerr);
	} else {
		waitpid(f, NULL, 0);
	}
	return 1;
}
