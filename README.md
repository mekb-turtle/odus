# odus
A light-weight and highly configurable sudo alternative for Linux written in C

![Screenshot of odus being used in the terminal](screenshot.png)

This project has three programs

- odus, a sudo alternative
- askpass, a tool that asks the user for a password to be called by other programs or shell scripts
- ggetty, an agetty alternative, starts a user login prompt on a tty

Basic configuration is done in [config.h](config.h)

#### This project has not been audited for bugs and vulnerabilites, I don't recommend using this on systems that you care about security. I recommend using doas instead.

TODO:

- [ ] have a cooldown on asking for the password, so it isn't annoying, but could reduce security [odus]
- [ ] make ggetty not error with ioctl TIOCSCTTY, shell is glitchy [ggetty]
- [x] fix termios bug, enter just echos ^M instead in tty [libaskpass]
- [x] pwfeedback, show asterisks instead of blank text when typing the password [libaskpass]
- [ ] get this audited for bugs and vulnerabilities
