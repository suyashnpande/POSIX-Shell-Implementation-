#  Custom POSIX Shell in C++

##  Overview

This project is a **POSIX-compliant interactive shell** implemented in **C++** for **Advanced Operating Systems (Assignment 2, Monsoon 2025)**. 

It supports **foreground/background execution, built-in commands (cd, ls, echo, pwd, pinfo, search), pipes, redirection, signal handling, command history, and autocomplete**, mimicking key features of a modern UNIX shell.  

---
## Requirements

- g++ compiler
- `readline` and `history` libraries
- Linux/Unix system

## Build & Run

1. Open a terminal in the project directory.
2. Build the shell using:

```bash
make 
make run
```
---

##  Features Implemented

### 1.  Shell Prompt
```bash
username@hostname:current_directory>
```

When the shell starts, it displays information like **username, hostname, and current working directory (CWD)**.  

#### Implementation: 
1. **Username & Home Directory**  
   - Use `getuid()` to obtain the user ID of the calling process.  
   - Pass this ID to `getpwuid()` to fetch user details from `/etc/passwd`.  
   - From this, extract:  
     - **Username**  
     - **Home Directory**

2. **Hostname**  
   - Use `gethostname()` to retrieve the system hostname.  
   - Internally, this value is read from `/etc/hostname`, which the kernel updates on reboot.

3. **Current Working Directory (CWD)**  
   - Use `getcwd()` to fetch the current directory path.  
   - This system call asks the kernel, which retrieves the information from the **Process Control Block (PCB)**.  
   - In Linux, the PCB is maintained by the `task_struct` structure, which stores metadata about each process.

---

### 2.  cd, echo , pwd 
```bash
 echo "abc 'ac' "  abc"
 ls -<flags> <Directory/File_name>
 ls -a -l . .. ~
 pwd
```
- **cd** → Implemented with `chdir()`, changes current directory (defaults to home if no path given).  
- **echo** → Implemented with `std::cout`, prints given arguments.  
- **pwd** → Implemented with `getcwd()`, shows current working directory.  
---
###  3. ls
```bash
ls 
ls -a
ls -l
ls .
ls ..
ls ~
ls -a -l
ls -la / ls -al
ls <Directory/File_name>
ls -<flags> <Directory/File_name>
```
The `ls` command is implemented using **`opendir()`**, **`readdir()`**, and **`stat()`** system calls.  
- Supports flags: **`-a`** (show hidden files) and **`-l`** (detailed view with permissions, owner, group, size, timestamp).  
- Handles multiple flags (`-al`, `-la`) and directories (`ls dir1 dir2`).  
- Uses `lstat()` to handle symbolic links and display targets in `-l` mode.  

---
### 4. System Commands (Foreground & Background), with and without arguments:
```bash
gedit &
sleep 10 &
```
- Foreground commands (e.g., `gedit`) run with **`fork()` + `execvp()`**, and the shell **waits** for completion using `waitpid()`.  
- Background commands (e.g., `sleep 10 &`) run with `&`, where the shell **spawns a child process**, **prints its PID**, and **continues accepting user input** without waiting.  

---
### `pinfo` Command
```bash
 pinfo 
 pinfo <pid>
```
- **Process State & Memory Usage** : Retrieved from `/proc/<pid>/stat` file.  
- **Executable Path** : Retrieved using `/proc/<pid>/exe` symbolic link.  \

---
 
### 6. `search` Command
```bash
   search xyz.txt
```

The `search` command recursively checks for a file/folder starting from the **current working directory (CWD)**.  
- Uses **`opendir()`**, **`readdir()`**, and **`stat()`** to traverse directories.  
- Returns **True** if the file is found, otherwise **False**.  
 
---


### I/O Redirection

```bash
   echo "hello" > output.txt
   cat < example.txt
   sort < file1.txt > lines_sorted.txt
```
1. **Parsing**  
   - Check if `<`, `>`, or `>>` are present in the command.  
   - Set flags accordingly and store file names.  
   - Remove redirection symbols from the command and tokenize it.

2. **Redirection Setup**  
   - Use `dup2()` (from `<unistd.h>`) to change standard input/output:  
     - `<` → open file in read-only, redirect `STDIN`.  
     - `>` → open file in write mode (truncate), redirect `STDOUT`.  
     - `>>` → open file in append mode, redirect `STDOUT`.

3. **Execution**  
   - After setting redirection, call `runCommand()` with the tokenized command.

---
### 8 & 9.  Pipelines + I/O Redirection
```bash
   cat sample2.txt | head -7 | tail -5
   cat file.txt | wc
   ls | grep “.txt” > out.txt; cat < in.txt | wc -l > lines.txt
```

- Supports chaining commands with `|` and combining with `<`, `>`, `>>`.  
- Implemented using **`pipe()`**, **`fork()`**, and **`dup2()`** to connect stdin/stdout between processes.  
- Redirection is applied to the **first** (`< infile`) or **last** (`> outfile`, `>> outfile`) command.  
- Parent closes all pipe ends and waits for child processes to prevent zombies.  

---
### 10. Signal Handling
```bash
   ctrl-Z
   ctrl-C
   ctrl-D
```
- **CTRL+Z** → Sends `SIGTSTP` to the foreground job, moving it to background (stopped state).  
- **CTRL+C** → Sends `SIGINT` to interrupt the foreground job.  
- **CTRL+D** → Exits the custom shell gracefully without affecting the terminal.  


---
### Autocomplete
``` bash
Examples:  
- `ec<TAB>` → auto-fills to `echo`  
- `cat alp<TAB>` → completes to `alpha.txt`  
- `cat al<TAB>` → shows all matches (`alpha.txt`, `alnum.txt`)  
```
Autocomplete is implemented using the **readline library** with a custom completion function.  
- Works for both **built-in** and **system commands**.  
- Also completes **files/directories** in the current path.  
- Triggered using the **TAB** key.  


---
### History
```bash
history        # shows last 10 commands  
history 5      # shows last 5 commands  
```
- Implements a custom `history` command, storing up to **20 commands** across sessions (`~/.my_shell_history`).  
- Default: shows last **10 commands**, or `history n` for the latest *n* commands.  
- Supports **Up/Down arrow keys** to cycle through previous commands for quick reuse/editing.  
- Track history across all sessions.
 

---






