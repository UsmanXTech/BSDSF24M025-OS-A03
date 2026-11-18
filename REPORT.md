# OS Assignment 03 — Custom UNIX Shell: Implementation Report

**Student:** Usman Hassan (BSDSF24M025)
**Course:** Operating Systems, FCIT (University of the Punjab)
**Repository:** [UsmanXTech/BSDSF24M025-OS-A03](https://github.com/UsmanXTech/BSDSF24M025-OS-A03)
**Instructor:** Muhammad Arif Butt, PhD

This report documents every feature of the shell, how it was implemented
(key functions and system calls), and the test evidence for each release.
One section per feature, in release order.

---

## Feature 1 — The Base Shell (Release 1, tag `v1.0-base`, 5 marks)

**What was done:** Set up the public repository `UsmanXTech/BSDSF24M025-OS-A03`
and populated it with the instructor's unmodified starter code from
`arifpucit/OS-Codes`, `Programming-Assignments/PA-03/base-assignment-03`
(`src/main.c`, `src/shell.c`, `src/execute.c`, `include/shell.h`).

**How it works:** The main loop in `main.c` repeats three steps —
`read_cmd()` reads one line and prints the `FCIT> ` prompt, `tokenize()`
splits it into an argument list, and `execute()` runs it with the classic
fork–exec–wait cycle (`fork()`, child calls `execvp()`, parent `waitpid()`).
Limits: `MAX_LEN` 512 characters per line, `MAXARGS` 10 arguments,
`ARGLEN` 30 characters per argument.

**Build notes:** The instructor's base project ships no `Makefile`, so one
was written (in the style of PA-02): sources in `src/`, headers in
`include/`, objects in `obj/`, final binary `bin/myshell` (the PDF's name;
the course README says `bin/psh`, but the assignment PDF is the spec).

**Portability fix (after the tag, commit `228dd2e`):** The starter code
uses `bzero()` in `shell.c`. GCC 14 (Kali) rejects the implicit declaration
as an error, while GCC 13 only warned about it. Fixed by replacing
`bzero(arglist[i], ARGLEN)` with `memset(arglist[i], 0, ARGLEN)`.
The `v1.0-base` tag itself stays the unmodified starter code, exactly as
the assignment requires; the fix lives on `main` after the tag.

**Test evidence:** `make` builds cleanly (one warning before the fix, none
after). Scripted run `printf 'ls -l\npwd\nwhoami\nexit\n' | ./bin/myshell`
— `ls -l`, `pwd` and `whoami` all executed correctly as external commands.

**Release:** [Release 1 - The Base Shell](https://github.com/UsmanXTech/BSDSF24M025-OS-A03/releases/tag/v1.0-base)
(tag `v1.0-base`, binary `myshell` attached, 16,680 bytes).

---

## Feature 2 — Built-in Commands (Release 2, tag `v2`, 10 marks)

**What was done:** Added the built-in commands `exit`, `cd`, `help` and
`jobs`. They run inside the shell process itself — no `fork()` — because a
child process cannot change its parent's working directory (`cd`) and
exiting a child would leave the shell running (`exit`).

**How it was implemented:**
- New file `src/builtin.c` with `int handle_builtin(char** arglist)` (prototype
  in `include/shell.h`). It returns `1` if the command was a built-in and was
  handled, `0` otherwise.
- `main.c` now checks built-ins before forking:
  `if (!handle_builtin(arglist)) execute(arglist);`
- `exit` — prints `Shell exited.` and calls `exit(0)`.
- `cd <directory>` — calls the `chdir()` system call; on failure `perror("cd")`
  prints e.g. `cd: No such file or directory`. A bare `cd` (no argument)
  changes to `$HOME` (bash behaviour); if `HOME` is unset it prints an error.
- `help` — prints the list of all four built-ins with a one-line description each.
- `jobs` — placeholder for now: prints `Job control not yet implemented.`
  (real job control arrives in Feature 6).

**Test evidence** (scripted piped input, all passed):
1. `printf 'help\njobs\nexit\n' | ./bin/myshell` — `help` listed all four
   built-ins, `jobs` printed the placeholder, `exit` closed the shell
   gracefully (exit code 0).
2. `printf 'pwd\ncd /tmp\npwd\ncd /nonexistent_dir_xyz\npwd\ncd\npwd\nexit\n' | ./bin/myshell`
   — `pwd` showed the repo directory, then `/tmp` after `cd /tmp` (proving
   `cd` changed the shell's own directory, since `pwd` runs as an external
   child), `cd` to a missing directory printed
   `cd: No such file or directory` and left the directory unchanged, and
   bare `cd` returned to `$HOME` (`/home/hatch`).
3. `printf 'echo hello-from-external\nwhoami\nexit\n' | ./bin/myshell` —
   external commands still work normally after the change.

**Build:** `make` clean — no errors, no warnings (`-Wall -Wextra -std=c11`).

**Release:** [Release 2 - Built-in Commands](https://github.com/UsmanXTech/BSDSF24M025-OS-A03/releases/tag/v2)
(tag `v2`, binary `myshell` attached).

---

## Feature 3 — Command History (Release 3, tag `v3`, 10 marks)

**What was done:** The shell now remembers commands. A `history` built-in
lists stored commands with line numbers starting from 1, and `!n`
re-executes history entry number `n`.

**How it was implemented:**
- Storage: a plain array `char* history[HISTORY_SIZE]` with
  `HISTORY_SIZE 20` in `src/shell.c`, plus `hist_count` (how many are
  stored). When the array is full, the oldest entry (index 0) is freed
  and the rest shift one slot left (`memmove`), so the newest command
  always lands in the last slot — the last 20 commands are kept.
- `add_to_history()` (in `shell.c`) stores a copy of each command line
  read by the main loop. Empty / whitespace-only lines are skipped, so
  they never take up a history slot.
- `history` built-in: added to `handle_builtin()` in `src/builtin.c`;
  it calls `print_history()`, which prints `%d  %s` — line number from 1,
  then the command.
- `!n` re-execution: handled in the `main.c` loop BEFORE tokenization
  and BEFORE adding anything to history, exactly as the PDF requires —
  the literal `!n` text is never stored. `expand_history()` (in
  `shell.c`) parses `n` with `strtol()`, returns a fresh copy of entry
  `n`, and the loop proceeds as if the user had typed that command
  directly (so the re-executed command IS added to history, like bash).
  A non-numeric `!` (`!abc`) or an out-of-bounds number (`!0`, `!99`
  when fewer entries exist) prints
  `myshell: !n: event not found` and the line is skipped.
- `help` now lists `history` and the `!n` syntax.

**Test evidence** (scripted piped input, all passed):
1. `printf 'echo one\necho two\necho three\nhistory\n!2\nhistory\n!99\n!0\n!abc\nexit\n' | ./bin/myshell`
   — `history` listed `1 echo one` … `4 history`; `!2` re-ran `echo two`
   (printed `two` again); the second `history` showed the re-executed
   `echo two` stored as entry 5 (not the literal `!2`); `!99`, `!0` and
   `!abc` each printed `myshell: !n: event not found` on stderr and the
   shell kept running.
2. 25 `echo cmdNN` commands followed by `history` — exactly 20 entries
   shown, oldest `1 echo cmd07` (cmd01–cmd06 dropped, `history` itself
   stored as entry 20). Proves the overwrite-oldest rule.
3. `printf '\n\necho first\n\nhelp\nhistory\n!1\nexit\n' | ./bin/myshell`
   — blank lines produced no history entries (`history` showed only
   `1 echo first`, `2 help`, `3 history`); `!1` re-ran `echo first`;
   `help` lists `history` and the `!n` hint.

**Build:** `make` clean — no errors, no warnings (`-Wall -Wextra -std=c11`).

**Release:** [Release 3 - Command History](https://github.com/UsmanXTech/BSDSF24M025-OS-A03/releases/tag/v3)
(tag `v3`, binary `myshell` attached).

---

## Feature 4 — Tab Completion with GNU Readline (Release 4, tag `v4`, 15 marks)

**What was implemented:** the starter's `getc` input loop was replaced with
the GNU Readline library, giving the shell professional line editing: Tab
completion (default readline completion — file/command names) and up/down
arrow-key history navigation.

**How (key functions / changes):**
- `src/shell.c` — `read_cmd()` now just calls `readline(prompt)`. Readline
  prints the prompt, reads the line and returns a `malloc`'d string
  (`NULL` on EOF / Ctrl-D, same contract as before, so the main loop is
  unchanged). The `fp` parameter is kept so the signature matches the
  starter code; readline always reads from stdin. Includes added:
  `#include <readline/readline.h>` and `#include <readline/history.h>`.
- `src/main.c` — after each command is read (and after any `!n` expansion,
  so the *resolved* line is stored, matching Feature 3), the line is fed
  to readline's own history with `add_history(cmdline)` — non-empty lines
  only (whitespace-only skipped, like Feature 3). This is what the
  up/down arrows cycle through; the `history` built-in still prints from
  the shell's own Feature 3 array, and both lists stay in sync.
- `Makefile` — `LDFLAGS = -lreadline`. Build needs the `libreadline-dev`
  package (headers); it was installed on this machine with
  `sudo apt-get install -y libreadline-dev` (runtime `libreadline8` was
  already present). No custom completion function was written — the PDF
  says to start with readline's default completion behaviour.

**Test evidence (2026-11-16, branch `feature-readline`, commit `1a04f61`):**
- `make clean && make` — clean build, **no errors, no warnings**.
- Scripted piped input
  (`printf 'echo hello-readline\npwd\nhistory\n!1\nexit\n' | ./bin/myshell`)
  works with readline: prompt printed, `echo`/`pwd` ran, `history` listed
  3 commands, `!1` re-ran `echo hello-readline`, `exit` printed
  "Shell exited." — Feature 3 behaviour unchanged.
- Interactive PTY test (Python `pty` module): typed `echo arrow-works`,
  pressed Up-arrow — the line `echo arrow-works` reappeared; Enter
  re-ran it and printed `arrow-works`. Typed `READ` + Tab — completed to
  `README.md `; typed `ls src/exe` + Tab — completed to
  `ls src/execute.c `. Ctrl-D printed "Shell exited." with exit code 0.
- Release: tag `v4` (annotated), merged to main (`bb5d017`),
  [Release 4](https://github.com/UsmanXTech/BSDSF24M025-OS-A03/releases/tag/v4)
  with compiled `bin/myshell` asset (17,408 bytes).

---

## Feature 5 — I/O Redirection and Pipes (Release 5, tag `v5`, 15 marks)

**What was implemented:** `>` output redirection, `<` input redirection,
and `|` pipes, exactly as the PDF specifies — all by manipulating file
descriptors in the child process, after `fork()` but before `execvp()`,
so the shell's own stdin/stdout are never touched.

- `src/execute.c` — `parse_redirection()` (shared helper) scans the
  argument list for `<` / `>` tokens, records the filenames, and builds
  a clean `argv` with the operators and filenames removed (so the program
  never sees them); a missing filename prints
  `myshell: syntax error: missing filename after '>'` (or `'<'`) and the
  command is not run. `setup_redirection()` does the fd work:
  `open(file, O_WRONLY | O_CREAT | O_TRUNC, 0644)` + `dup2(fd,
  STDOUT_FILENO)` for `>`, `open(file, O_RDONLY)` + `dup2(fd,
  STDIN_FILENO)` for `<`, then `close(fd)`. A file that cannot be opened
  prints `perror` and exits only the child (status 1) — the shell survives.
  `execute_pipe()` calls `pipe()` once in the parent, forks twice: the
  left child `dup2()`s the pipe write end onto its stdout, the right
  child `dup2()`s the read end onto its stdin, and **all unused pipe ends
  are closed in all three processes** (both children + parent) — if the
  parent kept the write end open, the reader would never see EOF and the
  pipe would hang. The parent `waitpid()`s both children. Each pipe side
  runs the same `parse_redirection()`/`setup_redirection()`, so a side
  may also redirect (e.g. `cat < file | grep x > out`).
- `src/main.c` — when the line contains `|`, it is split at the pipe
  *before* tokenizing and both sides are tokenized separately and run
  through `execute_pipe()` (an empty side prints `syntax error near '|'`).
  Only one pipe per line is supported; a second `|` would land inside the
  right side's token. Built-ins do not work inside a pipe (both sides are
  `execvp`'d) — the PDF's pipe example uses external commands only.
- `src/shell.c` / `include/shell.h` — `free_arglist()` helper (frees every
  token + the array) shared by the normal and pipe paths; `#include
  <fcntl.h>` for `open()` flags. No Makefile change was needed
  (`-lreadline` from Feature 4 is unchanged).

**Environment note:** this run's sandbox had lost the `libreadline-dev`
headers (F4 had installed them; the VM was reset since). `apt-get update`
hung for 10+ minutes on a slow proxy mirror, so the headers were
installed directly instead: the matching `libreadline-dev_8.2-4build1`
.deb was downloaded from archive.ubuntu.com and installed with
`sudo dpkg -i` (its dependency `libncurses-dev` was already present).
Build is unaffected on a machine where `libreadline-dev` is installed
normally.

**Test evidence (2026-11-17 ~01:20 PKT, branch `feature-io-redirection`,
commit `5e75a91`, temp dir `/tmp/f5test`):**
- `make clean && make` — clean build, **no errors, no warnings**.
- PDF examples, all pass: `ls -l > file_list.txt` created `file_list.txt`
  with the correct `ls -l` output; `sort < file_list.txt` printed the
  lines sorted; `cat /etc/passwd | grep root` printed
  `root:x:0:0:root:/root:/bin/bash`.
- Combinations: `cat < out.txt | grep hello` printed `hello`;
  `cat /etc/passwd | grep root > roots.txt` wrote the root line into
  `roots.txt`. No hang in any pipe test (parent closes both pipe ends).
- Error cases (shell survived every one, exit code 0 at the end):
  `sort < missing.txt` → `missing.txt: No such file or directory`;
  `echo oops >` → `myshell: syntax error: missing filename after '>'`;
  `| grep x` → `myshell: syntax error near '|'`.
- Regression (F2–F4): `pwd`, `history` (14 entries, numbered from 1),
  `!2` re-ran `sort < file_list.txt` correctly, `exit` printed
  "Shell exited.".
- Release: tag `v5` (annotated), merged to main (`4f2511e`),
  [Release 5](https://github.com/UsmanXTech/BSDSF24M025-OS-A03/releases/tag/v5)
  with compiled `bin/myshell` asset (21,872 bytes).

---

## Feature 6 — Command Chaining and Background Execution (Release 6, tag `v6`, 15 marks)

**What was implemented (PDF §Feature-6, pp. 8–9):**
- `;` chaining — the main loop splits each input line into segments at
  `;` (and at `&`, see below) and runs them one after another;
  `execute()` `waitpid()`s each foreground command, so the next segment
  only starts after the previous one finishes.
- `&` background — a segment terminated by `&` runs in the background:
  the parent does NOT `waitpid()`; it records the child's PID in the
  jobs list, prints `[n] <pid>` (bash style), and returns to the prompt
  at once. `&` also works as a mid-line separator
  (`sleep 1 & echo done` runs echo immediately).
- `jobs` built-in (real now, was the F2 placeholder) lists live
  background jobs as `[n]  <pid>  <command>`; empty list prints
  `No background jobs.`
- Zombie reaping — `reap_zombies()` runs before every prompt and loops
  `waitpid(-1, &status, WNOHANG)` (non-blocking, so the shell never
  freezes); each reaped PID is removed from the jobs list and a
  `[n] Done <cmd>` line is printed. Foreground children are never
  reaped here — `execute()` already `waitpid()`s its own child.

**How (key functions / syscalls):**
- `src/main.c` — `run_single_command(segment, background)` (per-segment:
  pipe check → built-in → `execute(arglist, background)`); the main
  loop splits the line at `;`/`&` in place and calls it per segment.
  `trim_segment()` strips leading/trailing blanks per segment.
- `src/jobs.c` (new module) — fixed array of 64 `Job { pid, number,
  cmd }` entries; `add_job()` / `remove_job()` / `print_jobs()` /
  `reap_zombies()`; job numbers are sequential (1, 2, 3, …) and never
  reused within a session.
- `src/execute.c` — `execute(arglist, background)`: background parent
  builds the jobs display string by re-joining the clean argv and
  calls `add_job()`; foreground parent unchanged (`waitpid`).
- `src/builtin.c` — `jobs` now calls `print_jobs()`; `help` text
  updated (`jobs` no longer "not yet implemented").
- `include/shell.h` — `MAX_JOBS 64`, new prototypes, `execute()` gained
  the `background` parameter.
- Extra: a background pipe (`cmd1 | cmd2 &`) runs in one extra forked
  child that calls `execute_pipe()` and `_exit()`s; the parent tracks
  that child PID as the job (the full pipeline text is shown in `jobs`
  — the `|` is restored after tokenizing). A built-in followed by `&`
  (e.g. `cd /tmp &`) runs in the foreground, since built-ins execute
  in the shell process itself and there is no child to background.
  `fflush(stdout)` after every `[n] pid` / `Done` message, so piped
  (non-TTY) output stays in order — a forked child inherits the stdio
  buffer otherwise.

**Known limits (not in the PDF):** `&&` / `||` are not implemented —
`a && b` behaves like `a & b` (the empty segment between the two `&`
is skipped). Quoted strings are still not parsed (pre-existing base
behaviour): the PDF example `echo "First command" ; …` runs and
chains correctly, but prints the quotes literally (`"First command"`).

**Test evidence (2026-11-17 ~03:05–03:15 PKT, branch
`feature-multitasking`, commit `1ec2ac9`, temp dir `/tmp/f6test`):**
- `make clean && make` — clean build, **no errors, no warnings**.
- Chaining: `echo first ; sleep 1 ; echo second` printed `first`,
  waited, printed `second`; PDF example with quotes chained correctly.
- Background + jobs + reaping: `sleep 2 &` printed `[1] 4151` and the
  prompt returned at once; `jobs` showed `[1]  4151  sleep 2`; after a
  foreground `sleep 3`, `[1] Done sleep 2` appeared before the next
  prompt and `jobs` printed `No background jobs.`
- `&` as separator: `sleep 1 & echo bg-separator-works` printed the
  echo output immediately while the sleep ran as job `[2]`.
- Background pipe: `sleep 2 | cat &` → job `[3]`, `jobs` listed
  `[3]  4160  sleep 2 | cat`; both jobs reaped with `Done` lines after
  a foreground `sleep 3`.
- `!n` on a chained line: `!1` re-ran `pwd ; whoami` (both segments).
- Regression (F2–F5): `pwd ; whoami` (chained built-in + external),
  `history` (whole chained lines stored as single entries, numbered
  from 1), `echo hi > f6.txt` + `sort < f6.txt`, `cat /etc/passwd |
  grep root` → `root:x:0:0:root:/root:/bin/bash`; `exit` printed
  "Shell exited.", shell exit code 0.
- No zombies: `ps aux | grep defunct` after exit showed none.
- Release: tag `v6` (annotated), merged to main (`4d7f806`),
  [Release 6](https://github.com/UsmanXTech/BSDSF24M025-OS-A03/releases/tag/v6)
  with compiled `bin/myshell` asset (22,416 bytes).

---

## Feature 7 — The `if-then-else-fi` Control Structure (Release 7, tag `v7`, 10 marks)

**What was implemented**

- Multi-line `if-then-else-fi`: a line starting with the keyword `if` is
  intercepted in the main loop (before the `;`/`&` split) and handed to
  `run_if_statement()` in `src/main.c`, which reads further lines at a
  secondary `> ` prompt until a line whose first word is `fi`.
- The lines between `then` and `else`/`fi` are stored in one array, the
  lines between `else` and `fi` in a second array (max 32 lines each,
  `MAX_IF_LINES`); the `else` block is optional.
- The condition is the rest of the `if` line. It is run first, and the
  branch is picked from its exit status: `0` runs the then-block,
  non-zero runs the else-block.
- To make that possible, `execute()` and `execute_pipe()` now **return**
  the child's exit status instead of always returning 0: foreground
  `execute()` returns `WEXITSTATUS(status)` (a signal-killed child counts
  as failure, status 1); `execute_pipe()` returns the **right** child's
  status (bash rule: a pipeline's status is its last command's status).
  `run_single_command()` and the new `run_line()` propagate that status
  (a chained condition is judged by its last segment).
- Block lines are re-run through `run_line()`, so `;` chaining, `&`
  background, pipes and redirection work inside a block exactly as on a
  normal command line. Continuation lines (`then`, `echo ...`, `fi`) are
  added to both our own history and readline's history, like bash.
- Syntax errors abort **without executing anything**: bare `if` (no
  command), a command line before `then`, `else` before `then`, missing
  `then`, or EOF/Ctrl-D before `fi` — each prints
  `myshell: syntax error: ...` and the stray lines up to `fi` are
  consumed, so they are not executed as separate commands.
- `help` now lists the `if ... then ... else ... fi` construct.
- Limitation (documented in the code): **nested if-blocks are not
  supported** — an `if` line inside a block would be run as an unknown
  external command.

**Key functions / syscalls (viva)**

- `run_if_statement()` (`src/main.c`): parsing + state machine
  (`seen_then`, `in_else`, `seen_fi`, `ok`), block storage, branch choice.
- `WEXITSTATUS(status)` on the `waitpid()` status in `execute()` /
  `execute_pipe()` — the macro that extracts the child's real exit code.
- `first_token_is()` — keyword matching that does not mistake `ifx` for
  `if`; `dup_string()` — `strdup()` equivalent without POSIX feature
  macros (build is `-std=c11`).

**Test evidence** (scripted piped input, `printf '...' | ./bin/myshell`)

- `make clean && make` — clean build, **no errors, no warnings**.
- `if true` → then-block ran (`T1-THEN-RAN`); `if false` → else-block
  ran (`T2-ELSE-RAN`); `if false` with no `else` → nothing printed.
- `if grep root /etc/passwd` → then (`T3-FOUND`); `if grep
  nosuchpattern123 /etc/passwd` → else (`T4-NOTFOUND`).
- `if [ -f /tmp/f7test.txt ]` → then (`T6-EXISTS`); missing file →
  else (`T7-MISSING`).
- Redirection in the condition: `if grep root /etc/passwd > /dev/null`
  → then, with no grep output on screen. Pipe in the condition:
  `if cat /etc/passwd | grep root` → then.
- `;` chaining inside a block: `echo T10-A ; echo T10-B` printed both.
- Bad syntax: bare `if` → `myshell: syntax error: expected a command
  after 'if'`, block not executed; missing `then` → `myshell: syntax
  error: expected 'then'`, block not executed.
- Regression (F2–F6): `history` shows the if-block lines numbered
  (`if true`, `then`, `echo ...`, `fi` as entries 2–5); `!1` re-ran
  `pwd`; `sleep 1 &` + `jobs` listed the job; `echo hi > file` +
  `sort < file`; `cat /etc/passwd | grep root` → `root:x:0:0:...`;
  `help` lists the if-construct; `exit` printed "Shell exited.",
  shell exit code 0.
- Release: tag `v7` (annotated), merged to main (`8f198f4`),
  [Release 7](https://github.com/UsmanXTech/BSDSF24M025-OS-A03/releases/tag/v7)
  with compiled `bin/myshell` asset (26,704 bytes).

---

## Feature 8 — Shell Variables (Release 8, tag `v8`, 10 marks)

**What was implemented**

- `VARNAME=value` assignment (no spaces around `=`), handled as a
  built-in action — no `fork()`, no execution. A line starting with a
  valid variable name directly followed by `=` is detected in
  `run_single_command()` (`src/main.c`) *before* tokenizing or pipe
  handling, so the value may contain spaces when quoted:
  `MESSAGE="Hello, World!"` stores `Hello, World!` (matching quotes
  stripped, like bash). Single quotes work the same way; `EMPTY=`
  stores an empty string; re-assignment replaces the old value.
- Storage: a singly linked list of key-value pairs
  (`src/variables.c`, `struct var_node { name, value, next }`) — the
  PDF's suggested data structure. `set_variable()` inserts a new node
  or replaces the value in place; `get_variable()` searches the list
  and falls back to the process environment (`getenv()`), so `$HOME`
  keeps working.
- Expansion: after tokenizing but before executing (PDF Task 3),
  `expand_variables()` walks the arglist and replaces every token
  starting with `$` by the variable's value — so it works for
  built-ins (`cd $DIR`), external commands (`echo $MSG`), both sides
  of a pipe, `;`-chained segments and if-conditions. Each expanded
  token is a fresh `malloc`'d string (the PDF's warning: the value can
  be longer than tokenize's 30-byte `ARGLEN` buffer, so writing in
  place would overflow). Trailing characters after the name are kept
  (`$MSG!` → value + `!`); an undefined variable expands to the empty
  string (bash behaviour). A bare `$` or an invalid name
  (`1BAD=x`) is not an assignment and falls through to `execute()`,
  which reports "Command not found" — like bash.
- New built-in `set` (`handle_builtin()`, `src/builtin.c`): prints all
  defined shell variables as `NAME=value`, one per line.
  Environment variables are not listed — only variables this shell
  defined. `help` now lists `set`, `VARNAME=value` and `$VARNAME`.
- Environment note (sandbox): this VM generation shipped without
  `libreadline-dev` headers, so the first `make` failed on
  `readline/history.h`. Fixed by installing `libreadline-dev` +
  `libncurses-dev` from the local apt cache (`dpkg -i`, the system
  `apt-get update` held the lock at the time). No code change needed.

**Key functions / syscalls (viva)**

- `is_assignment_line()` / `handle_assignment()` (`src/variables.c`):
  name validation (letter/`_` first, then alnum/`_`) and quote
  stripping; `set_variable()` / `get_variable()` / `print_variables()`
  — linked-list insert-or-replace, lookup with `getenv()` fallback.
- `expand_variables()` — pre-execution step; fresh allocation per
  token; `free()` of the old token; undefined → `""`.
- Hook points in `run_single_command()` (`src/main.c`): assignment
  check right after trimming (before the pipe split), expansion right
  after each `tokenize()` (normal path and both pipe sides).

**Test evidence** (scripted piped input, `printf '...' | ./bin/myshell`)

- `make clean && make` — clean build, **no errors, no warnings**.
- PDF tests: `MSG="hello"` → `set` shows `MSG=hello` → `echo $MSG`
  prints `hello`; re-assign `MSG="world"` → `echo $MSG` prints
  `world`; `MESSAGE="Hello, World!"` → `echo $MESSAGE` prints
  `Hello, World!`; `echo $UNDEFINED` prints an empty line;
  `VAR1=one` → `echo $VAR1` prints `one`.
- Extra: `A=foo; echo $A` → `foo` (chaining); `echo $A | cat` →
  `foo` (pipe expansion); `DIR=/tmp; cd $DIR; pwd` → `/tmp`;
  `NAME='single quoted'` → `single quoted`; `EMPTY=` → `echo
  $EMPTY` prints empty; `1BAD=x` → "Command not found" (invalid
  name); `if true` / `then` / `echo IF-VAR-OK $A` / `fi` → `IF-VAR-OK
  foo` (expansion inside an if-block); `help` lists `set` and the
  variable syntax.
- Limitation (documented): only tokens *starting* with `$` expand —
  `pre$VAR` and `${VAR}` are not expanded, and the tokenizer is not
  quote-aware, so `"X${EMPTY}Y"` prints literally. The PDF only
  requires words beginning with `$`, so this is in spec.
- Regression (F2–F7): `history` numbered 1–5; `!1` re-executed
  `EMPTY=`; `echo hi > file` + `sort < file` → `hi`; `sleep 1 &` +
  `jobs` listed `[1] <pid> sleep 1`; `cat /etc/passwd | grep root` →
  `root:x:0:0:...`; `exit` printed "Shell exited.", shell exit code 0.
- Release: tag `v8` (annotated), merged to main (`9d5efee`),
  [Release 8](https://github.com/UsmanXTech/BSDSF24M025-OS-A03/releases/tag/v8)
  with compiled `bin/myshell` asset (27,120 bytes).

---

## Feature 9 — Git Workflow with a Secondary Account (Release 9, tag `v9`, 10 marks)

**What was implemented**

- The secondary GitHub account is Samiq-56 (bsdsf24m056@pucit.edu.pk) —
  the classmate who did the F9 work as the contributor, while this repo
  (UsmanXTech/BSDSF24M025-OS-A03) is the primary. The secondary account
  forked the primary repo, created the mock-bug issue, fixed it on a
  branch, and opened the pull request; the primary (UsmanXTech) reviewed
  and merged it.
- Mock bug (verified before the fix): `cd` silently ignored extra
  arguments — `cd /tmp extra` moved to /tmp with no error. The issue
  ([Issue #1](https://github.com/UsmanXTech/BSDSF24M025-OS-A03/issues/1),
  "Bug: `cd` extra arguments ignore kar deta hai") records the expected
  bash behaviour: print `cd: too many arguments` and do not change
  directory.
- Fix (branch `fix-cd-extra-args`, commit `0648e21`, PR
  [#2](https://github.com/UsmanXTech/BSDSF24M025-OS-A03/pull/2), 1 file,
  +6 lines in `src/builtin.c`): inside the `cd` block, right after
  `const char* dir = arglist[1];`, the check `if (arglist[1] != NULL &&
  arglist[2] != NULL) { fprintf(stderr, "cd: too many arguments\n");
  return 1; }`. The `arglist[1]` guard is needed because for a bare `cd`
  the slot after the NULL terminator is not guaranteed to be NULL.
- Workflow (PDF Tasks 1–5): fork from the secondary account; issue on
  the primary repo; clone the fork elsewhere, branch, fix, commit, push,
  open PR to primary; primary reviews and merges (`3dd5551`, "Merge pull
  request #2 from Samiq-56/fix-cd-extra-args", 2026-11-18 10:34 PKT);
  issue #1 auto-closed by "Fixes #1"; `git pull` on main; all feature
  branches pushed to origin (`git push origin --all` — all 7 feature
  branches plus main are on the remote).
- Tested: `cd /tmp extra` prints the error and the directory is unchanged
  (verified live in the built binary); `cd /tmp`, bare `cd` and
  `cd /nonexistent` behave as before (PR author's tests); the fix touches
  only the `cd` block in `src/builtin.c`. `make clean && make`: no errors,
  no warnings.
- Release: tag `v9` (annotated), merged to main (`3dd5551`),
  [Release 9](https://github.com/UsmanXTech/BSDSF24M025-OS-A03/releases/tag/v9)
  with compiled `bin/myshell` asset (27,120 bytes).
