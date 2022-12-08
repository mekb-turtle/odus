#define ggetty_username_prompt "Username: "
#define ggetty_password_prompt "Password for %s: "
#define ggetty_autologin_text "Autologin: %s\n"
#define ggetty_default_path "/bin:/usr/bin"
#define ggetty_no_user "\x1b[38;5;9mUser does not exist\x1b[0m\n"
#define askpass_default_prompt "Password: "
#define odus_cannot_find_user_id "\x1b[0;38;5;9mCannot find user ID %i\x1b[0m\n"
#define odus_cannot_find_user_name "\x1b[0;38;5;9mCannot find user %s\x1b[0m\n"
#define odus_execvp_error "\x1b[0;38;5;9m%s: %s\x1b[0m\n"
#define odus_not_in_group "\x1b[0;38;5;9m%s is not in group %s\x1b[0m\n"
#define odus_group "wheel"
#define odus_prompt "\x1b[0;38;5;7m[odus] \x1b[0mpassword for \x1b[38;5;14m%s\x1b[0m: "
#define odus_default_path "/bin:/sbin:/usr/bin:/usr/sbin"
#define odus_env_keep {\
		"EDITOR", "SSH_CLIENT", "SSH_CONNECTION", "SSH_TTY", "TERM", "CLICOLOR", "COLORS",\
		"TERMINFO", "XAUTHORIZATION", "XAUTHORITY", "TZ", "LANG", "LC_ALL", "PATH", "HOSTNAME",\
		"LS_COLORS", "EXA_COLORS", "DISPLAY", "GTK2_RC_FILES", "XCOMPOSEFILE",\
		"_JAVA_AWT_WM_NONREPARENTING", "QT_QPA_PLATFORM", "SDL_VIDEODRIVER", "WAYLAND_DISPLAY", "GDK_BACKEND", \
		NULL }
/*
		"XDG_DATA_HOME", "XDG_CONFIG_HOME", "XDG_SESSION_TYPE", "XDG_CACHE_HOME",\
		"XDG_STATE_HOME", "XDG_RUNTIME_DIR", "XDG_BACKEND", NULL }
*/
#define util_invalid_password "\x1b[0;38;5;9mIncorrect password\x1b[0m\n"
#define util_no_more_tries "\x1b[0;38;5;9mToo many incorrect attempts\x1b[0m\n"
#define util_no_password "\x1b[0;38;5;9mPermission denied\x1b[0m\n"
#define util_password_tries 3
#define libaskpass_max_length 4096
#define libaskpass_password_char "*"
#define libaskpass_password_color "\x1b[0;38;5;10m"
#define libaskpass_use_char true
