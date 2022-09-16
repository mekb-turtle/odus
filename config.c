#include <stddef.h>
#include <stdbool.h>
char *ggetty_username_prompt = "Username: ";
char *ggetty_password_prompt = "Password for %s: ";
char *ggetty_autologin_text = "Autologin: %s\n";
char *ggetty_default_path = "/bin:/usr/bin";
char *ggetty_no_user = "\x1b[38;5;9mUser does not exist\x1b[0m\n";
char *askpass_default_prompt = "Password: ";
char *odus_group = "odus";
char *odus_prompt = "\x1b[0;38;5;7m[odus] \x1b[0mpassword for \x1b[38;5;14m%s\x1b[0m: ";
char *odus_default_path = "/bin:/sbin:/usr/bin:/usr/sbin";
char *util_invalid_password = "\x1b[0;38;5;9mIncorrect password\x1b[0m\n";
char *util_no_more_tries = "\x1b[0;38;5;9mToo many incorrect attempts\x1b[0m\n";
char *util_no_password = "\x1b[0;38;5;9mPermission denied\x1b[0m\n";
int util_password_tries = 3;
size_t libaskpass_max_length = 4096;
char *libaskpass_password_char = "*";
char *libaskpass_password_color = "\x1b[0;38;5;10m";
bool libaskpass_use_char = true;
