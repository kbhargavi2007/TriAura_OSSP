# LinuxCore – Custom Linux Shell

## About the Project

LinuxCore is a custom Linux shell developed using the C programming language as part of the Operating Systems and Systems Programming (OSSP) project.

It provides its own command prompt and supports basic Linux commands, system information, process management, file operations, and execution of external Linux commands.

## Team Members

| Student ID | Name |
|---|---|
| 2520030074 | K. Bhargavi |
| 2520030603 | M. Siri Chandana |
| 2520030300 | G. Nandini |

## Features

- Basic Linux commands
- Command history
- System information
- CPU and memory information
- Process information
- Disk usage information
- File creation, reading, and writing
- File permission checking
- External command execution
- Date and time display

## Technologies Used

- C Programming Language
- Linux Operating System
- GCC Compiler
- Linux System Calls
- Process Management using `fork()`, `execvp()`, and `waitpid()`
- Linux `/proc` filesystem

## Detailed Implementation

LinuxCore is implemented in C using a command-processing approach. It continuously accepts commands from the user, identifies the entered command, and performs the corresponding operation.

### 1. Shell Initialization

When the program starts, LinuxCore displays its welcome message and initializes the command prompt.

The shell enters a continuous loop where it waits for the user to enter a command. This loop continues until the user enters the `exit` command.

### 2. Command Input and Processing

LinuxCore accepts commands through the terminal using standard input functions.

The entered command is read and processed. The shell identifies the command name and separates it from its arguments.

Based on the command entered, LinuxCore decides whether it is a built-in command or an external Linux command.

### 3. Built-in Command Execution

Some commands are directly implemented inside the shell. These are called built-in commands.

Examples include:

- `cd` – Changes the current working directory.
- `pwd` – Displays the present working directory.
- `echo` – Displays the given text.
- `history` – Displays previously entered commands.
- `clear` – Clears the terminal screen.
- `exit` – Terminates the shell.

These commands are handled directly by LinuxCore without creating a separate process.

### 4. External Command Execution

LinuxCore also supports commands that are not directly implemented inside the shell.

For executing external commands, it uses Linux process management system calls:

- **fork():** Creates a child process.
- **execvp():** Executes the requested external command in the child process.
- **waitpid():** Makes the parent shell wait for the child process to complete.

This allows users to execute external Linux commands through the LinuxCore prompt.

### 5. System Information

LinuxCore provides commands to display information about the system.

- `sysinfo` – Displays general system information.
- `cpu` – Displays CPU-related information.
- `memory` – Displays memory usage information.
- `disk` – Displays disk space information.
- `date` – Displays the current date and time.

These features use Linux system information and system interfaces to retrieve the required details.

### 6. Process Management

The `processes` command displays information about currently running processes.

LinuxCore uses the Linux `/proc` filesystem to identify process information. Numeric directory names inside `/proc` represent process IDs (PIDs).

This helps users understand how processes are represented and managed in Linux.

### 7. File Handling

LinuxCore provides basic file operations through its own commands.

- `createfile` – Creates a new file.
- `readfile` – Reads and displays the contents of a file.
- `writefile` – Writes content into a file.

These operations demonstrate file handling using C programming and Linux file operations.

### 8. File Permissions

The `permissions` command displays the permission details of a specified file.

It helps users understand the permission categories in Linux:

- Owner permissions
- Group permissions
- Other user permissions

For example, `644` represents read and write permissions for the owner and read-only permissions for the group and others.

### 9. Command History

LinuxCore maintains a history of commands entered during the shell session.

The `history` command allows users to view previously executed commands. This makes it easier to track the commands used during a session.

### 10. Shell Termination

When the user enters the `exit` command, LinuxCore terminates the shell loop and ends the program execution.

## Project Structure

```text
LinuxCore/
│__LunixCore.c
├── main.c
└── README.md
```

## How to Compile

Open the Linux terminal and navigate to the project directory.

```bash
gcc -Wall -Wextra -std=c11 main.c -o linuxcore
```

## How to Run

```bash
./linuxcore
```

After execution, the LinuxCore command prompt will appear.

## Available Commands

| Command | Description |
|---|---|
| `help` | Displays available commands |
| `pwd` | Shows the current directory |
| `cd` | Changes the directory |
| `ls` | Lists files and directories |
| `echo` | Displays text |
| `history` | Displays command history |
| `sysinfo` | Shows system information |
| `cpu` | Displays CPU information |
| `memory` | Displays memory information |
| `processes` | Lists running processes |
| `disk` | Displays disk usage |
| `date` | Shows date and time |
| `createfile` | Creates a new file |
| `readfile` | Reads file contents |
| `writefile` | Writes content into a file |
| `permissions` | Displays file permissions |
| `clear` | Clears the terminal |
| `exit` | Exits LinuxCore |
