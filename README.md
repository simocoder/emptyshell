# mtsh — Empty Shell —  Minimal Teaching Shell

**mtsh** (pronounced like *em tee shell*) is a tiny, educational Unix-like shell written in C.  
It’s designed to be simple enough to understand line-by-line, while still being a functional interactive shell you can extend over time.

It starts as a bare-bones REPL that can run external commands, and is designed to grow feature-by-feature as you learn. Every line of code is intended to be readable, hackable, and easily extended — making mtsh a hands-on guide to understanding how real shells work under the hood.

## Features

- **POSIX.1-2008 compliant** — portable across most modern Unix-like systems.
- Minimal command loop:
  - Reads a line of input
  - Parses it into arguments
  - Runs external commands via `execvp()`
- Built-in commands:
  - `cd` — change the working directory
  - `exit` — quit the shell
- Proper signal and exit code handling.

## Why mtsh?

There are many full-featured shells. **mtsh** is not one of these!  
Instead, it’s a *teaching tool*: a clean starting point for learning shell internals without wading through tens of thousands of lines of code.

You can:
- Experiment with process management.
- Add more built-ins.
- Implement piping, redirection, job control, or scripting support.
- Learn by modifying a shell you fully understand.

```
  ┌─────────┐
  │  Start  │
  └────┬────┘
       │
       ▼
┌─────────────┐
│  Read line  │  ← get command from user
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  Evaluate   │  ← parse + run the command
└──────┬──────┘
       │
       ▼
┌─────────────┐
│   Print     │  ← show output or error
└──────┬──────┘
       │
       ▼
┌─────────────┐
│   Loop?     │  ← exit if user typed "exit"
└──────┬──────┘
       │yes
       ▼
     (stop)

```
## Build

```sh
# Clone this repository
git clone https://github.com/simocoder/emptyshell.git
cd emptyshell

# Build
cc -std=c11 -Wall -Wextra -O2 -o mtsh mtsh.c
```

Or use `make`:

```sh
make
```

## Install (optional)

You can run **mtsh** directly from the repo, or install it somewhere in your `PATH`:

```sh
sudo cp mtsh /usr/local/bin/
```

## Usage

```sh
./mtsh
```

Example session:
```
$ ./mtsh
                      _             _          _ _ 
  ___ _ __ ___  _ __ | |_ _   _ ___| |__   ___| | |
 / _ \ '_ ` _ \| '_ \| __| | | / __| '_ \ / _ \ | |
|  __/ | | | | | |_) | |_| |_| \__ \ | | |  __/ | |
 \___|_| |_| |_| .__/ \__|\__, |___/_| |_|\___|_|_|
               |_|        |___/                    
 emptyshell — Minimal Teaching Shell v0.1
 POSIX.1-2008 • Simple • Hackable
-----------------------------------------------------

> pwd
/home/user
> cd /tmp
> ls
example.txt
> exit
$
```

## Roadmap Ideas

- Command history (`readline` or custom)
- Tab completion
- Pipelining (`cmd1 | cmd2`)
- I/O redirection
- Background jobs (`&`)
- Configuration files (`.mtshrc`)

## License

MIT — see [LICENSE](LICENSE) for details.
