# odus

> 🔒 A light-weight and highly configurable sudo alternative for Linux written in C

[![license](https://img.shields.io/github/license/mekb-turtle/odus)](https://github.com/mekb-turtle/odus/blob/master/LICENSE) [![PRs welcome](https://img.shields.io/badge/PRs-welcome-ff69b4.svg)](https://github.com/mekb-turtle/odus/issues?q=is%3Aissue+is%3Aopen+label%3A%22help+wanted%22)<br />
<img src="https://raw.githubusercontent.com/mekb-turtle/odus/main/screenshot.png" />


## 🚩 Table of Contents

- [Why Use odus?](#🤖-why-use-odus-over-alternatives-like-sudo)
- [Installing](#📥-installing)
- [Contributing](#-contributing)
- [Used By](#-used-by)
- [License](#-license)


## 🤖 Why Use odus over alternatives like `sudo`?
odus is *much* more configurable. Want to change the text for `sudo`? Into the source code you go. But with odus you can simply edit the `config.h` and recompile!


## 📥 Installing 
### Dependencies:
* `make`
* A C compiler (such as `gcc`)
```sh
git clone https://github.com/mekb-turtle/odus.git
make
make install
```

## ⚙️ Configuring
To configure odus simply edit the `config.h` file and redo the [installation steps](#📥-installing)
## 🔧 Contributing
odus is open source, so you can create a pull request (PR). Simply `git clone` and your ready to go!
### Develop

Fork and simply make your changes and submit a PR
### Pull Request

Before uploading your PR, test all features one last time to check if there are any errors. If it has no errors, commit and push it!

## 📜 License

This software is licensed under the [GPL-3.0 License](https://github.com/mekb-turtle/odus/blob/master/LICENSE).