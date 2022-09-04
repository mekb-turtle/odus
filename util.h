bool str_to_long(const char *str, long *out);
bool getgrouplist_(const char *user, gid_t group, gid_t **groups_, int *ngroups_);
bool password_check(struct passwd *pw, char *prompt, bool notty, int tty);
char *clone_string(const char *str);
struct passwd *clone_passwd(struct passwd *ptr);
