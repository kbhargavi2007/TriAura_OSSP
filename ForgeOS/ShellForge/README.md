# LinuxCore – Custom Linux Shell

## 📌 About the Project

LinuxCore is a custom Linux shell developed using the C programming language as part of the Operating Systems and Systems Programming (OSSP) project.

It provides a command-line interface where users can enter commands to interact with the Linux operating system. The project demonstrates how a shell accepts user input, executes commands, and uses operating system features.

## 🎯 Objectives

- To develop a simple custom Linux shell using C.
- To understand how command execution works in Linux.
- To demonstrate process creation and management.
- To perform basic file operations.
- To explore system information and Linux resources.

## ⚙️ Features

- **help** – Displays available commands.
- **pwd** – Displays the current working directory.
- **cd** – Changes the current directory.
- **ls** – Lists files and directories.
- **echo** – Displays text.
- **history** – Displays previously entered commands.
- **sysinfo** – Displays system information.
- **cpu** – Displays CPU information.
- **memory** – Displays memory information.
- **processes** – Lists running process IDs.
- **disk** – Displays disk usage information.
- **date** – Displays the current date and time.
- **createfile** – Creates a new file.
- **permissions** – Displays file permissions.
- **readfile** – Reads and displays file contents.
- **writefile** – Writes text into a file.
- **clear** – Clears the terminal screen.
- **exit** – Exits LinuxCore.
- **External commands** – Runs supported installed Linux commands.

## 🛠️ Technologies Used

- **Programming Language:** C
- **Operating System:** Linux / Ubuntu
- **Environment:** WSL2 (Windows Subsystem for Linux)
- **Compiler:** GCC
- **Concepts:** System Calls, Process Management, File Handling, and Shell Programming

## ▶️ How to Compile and Run

### 1. Clone the Repository

```bash
git clone YOUR_GITHUB_REPOSITORY_URL
```

Replace `YOUR_GITHUB_REPOSITORY_URL` with your actual GitHub repository URL.

### 2. Open the Project Folder

```bash
cd LinuxCore_test
```

Use the actual repository folder name if it differs.

### 3. Compile the Program

```bash
gcc -Wall -Wextra -std=c11 main.c -o linuxcore
```

### 4. Run LinuxCore

```bash
./linuxcore
```

## 🧠 Operating System Concepts Demonstrated

- **Shell as a Service Layer:** Provides a user interface to interact with Linux.
- **Process Management:** Uses `fork()`, `execvp()`, and `waitpid()` to execute external commands.
- **File Systems:** Uses file operations to create, read, write, and inspect files.
- **System Information:** Reads Linux system information from available system interfaces.
- **Command Execution:** Accepts user commands and processes them.

## 📚 Learning Outcomes

Through this project, we learned the basics of shell programming, process creation, command execution, file handling, and interaction with Linux system resources.

## 🎓 Project Purpose

LinuxCore was developed as an academic project for learning and demonstrating Operating Systems and Systems Programming concepts through a simple custom shell.

## 📄 License

This project is intended for educational purposes.
