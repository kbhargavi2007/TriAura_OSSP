/*
 * LinuxCore v6.0
 * A simple custom Linux shell written in C.
 *
 * Built-ins:
 * help, pwd, cd, ls [path], echo, history, sysinfo, cpu, memory,
 * processes, disk [path], date, createfile <name>, permissions <file>,
 * readfile <file>, writefile <file>, clear, exit
 *
 * Other Linux commands are launched using fork(), execvp(), and waitpid().
 *
 * Compile:
 *   gcc -Wall -Wextra -std=c11 main.c -o linuxcore
 */

#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <sys/utsname.h>

#define LINUXCORE_VERSION "6.0"
#define LINUXCORE_INPUT_SIZE 1024
#define MAX_ARGS 100
#define MAX_HISTORY 500

static char history[MAX_HISTORY][LINUXCORE_INPUT_SIZE];
static int history_count = 0;

static void trim_newline(char *text)
{
    if (text == NULL) return;
    text[strcspn(text, "\n")] = '\0';
}

static void add_history(const char *line)
{
    if (line == NULL || line[0] == '\0') return;

    if (history_count < MAX_HISTORY) {
        snprintf(history[history_count], sizeof(history[history_count]), "%s", line);
        history_count++;
    } else {
        memmove(history, history + 1, sizeof(history[0]) * (MAX_HISTORY - 1));
        snprintf(history[MAX_HISTORY - 1], sizeof(history[MAX_HISTORY - 1]), "%s", line);
    }
}

static int tokenize(char *line, char *args[], int max_args)
{
    int count = 0;
    char *saveptr = NULL;
    char *token = strtok_r(line, " \t", &saveptr);

    while (token != NULL && count < max_args - 1) {
        args[count++] = token;
        token = strtok_r(NULL, " \t", &saveptr);
    }
    args[count] = NULL;
    return count;
}

static void print_help(void)
{
    printf("\nLinuxCore v%s - Available Commands\n", LINUXCORE_VERSION);
    printf("--------------------------------------------------\n");
    printf("help                     Show this help menu\n");
    printf("pwd                      Print current directory\n");
    printf("cd <directory>           Change current directory\n");
    printf("ls [path]                List files in a directory\n");
    printf("echo <text>              Print text\n");
    printf("history                  Show commands entered this session\n");
    printf("sysinfo                  Show basic system information\n");
    printf("cpu                      Show CPU information\n");
    printf("memory                   Show memory information\n");
    printf("processes                List numeric process directories in /proc\n");
    printf("disk [path]              Show disk usage for a path\n");
    printf("date                     Show current date and time\n");
    printf("createfile <name>        Create a new empty file\n");
    printf("permissions <file>       Show file permissions\n");
    printf("readfile <file>          Display a text file (v6.0)\n");
    printf("writefile <file>         Write text into a file (v6.0)\n");
    printf("clear                    Clear the terminal\n");
    printf("exit                     Exit LinuxCore\n");
    printf("\nOther installed Linux commands can also be used.\n");
    printf("Note: paths containing spaces should be entered without spaces in this simple shell.\n\n");
}

static void command_pwd(void)
{
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
    } else {
        perror("pwd");
    }
}

static void command_cd(char *args[])
{
    const char *target = args[1];

    if (target == NULL || strcmp(target, "~") == 0) {
        target = getenv("HOME");
        if (target == NULL) {
            fprintf(stderr, "cd: HOME is not set\n");
            return;
        }
    }

    if (chdir(target) != 0) {
        perror("cd");
    }
}

static void command_ls(char *args[], int argc)
{
    if (argc > 1 && args[1][0] == '-') {
        /* Let the real ls handle flags such as -l and -a. */
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return;
        }
        if (pid == 0) {
            execvp("ls", args);
            perror("ls");
            _exit(127);
        }
        while (waitpid(pid, NULL, 0) < 0) {
            if (errno != EINTR) {
                perror("waitpid");
                break;
            }
        }
        return;
    }

    const char *path = (argc >= 2) ? args[1] : ".";
    DIR *dir = opendir(path);
    if (dir == NULL) {
        perror("ls");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] != '.') {
            printf("%s  ", entry->d_name);
        }
    }
    printf("\n");
    closedir(dir);
}

static void command_echo(char *args[], int argc)
{
    for (int i = 1; i < argc; i++) {
        printf("%s", args[i]);
        if (i + 1 < argc) putchar(' ');
    }
    putchar('\n');
}

static void command_history(void)
{
    for (int i = 0; i < history_count; i++) {
        printf("%3d  %s\n", i + 1, history[i]);
    }
}

static void command_sysinfo(void)
{
    struct utsname info;
    if (uname(&info) != 0) {
        perror("sysinfo");
        return;
    }

    printf("LinuxCore System Information\n");
    printf("----------------------------\n");
    printf("System:         %s\n", info.sysname);
    printf("Node name:      %s\n", info.nodename);
    printf("Kernel release: %s\n", info.release);
    printf("Kernel version: %s\n", info.version);
    printf("Architecture:   %s\n", info.machine);
}

static void command_cpu(void)
{
    FILE *file = fopen("/proc/cpuinfo", "r");
    if (file == NULL) {
        perror("cpu");
        return;
    }

    char line[512];
    int printed_model = 0;
    int printed_cores = 0;

    printf("CPU Information\n");
    printf("---------------\n");
    while (fgets(line, sizeof(line), file) != NULL) {
        if (!printed_model && (strstr(line, "model name") != NULL ||
                               strstr(line, "Hardware") != NULL)) {
            printf("%s", line);
            printed_model = 1;
        }
        if (strncmp(line, "processor", 9) == 0) {
            printed_cores++;
        }
    }
    if (printed_cores > 0) {
        printf("Logical processors reported: %d\n", printed_cores);
    }
    if (!printed_model) {
        printf("CPU model details are not available in /proc/cpuinfo.\n");
    }
    fclose(file);
}

static void command_memory(void)
{
    FILE *file = fopen("/proc/meminfo", "r");
    if (file == NULL) {
        perror("memory");
        return;
    }

    char line[256];
    const char *wanted[] = {"MemTotal:", "MemFree:", "MemAvailable:",
                            "Buffers:", "Cached:", "SwapTotal:", "SwapFree:"};
    printf("Memory Information (/proc/meminfo)\n");
    printf("----------------------------------\n");

    while (fgets(line, sizeof(line), file) != NULL) {
        for (size_t i = 0; i < sizeof(wanted) / sizeof(wanted[0]); i++) {
            if (strncmp(line, wanted[i], strlen(wanted[i])) == 0) {
                printf("%s", line);
            }
        }
    }
    fclose(file);
}

static int is_numeric_name(const char *name)
{
    if (name == NULL || name[0] == '\0') return 0;
    for (size_t i = 0; name[i] != '\0'; i++) {
        if (!isdigit((unsigned char)name[i])) return 0;
    }
    return 1;
}

static void command_processes(void)
{
    DIR *proc = opendir("/proc");
    if (proc == NULL) {
        perror("processes");
        return;
    }

    printf("Process IDs found in /proc\n");
    printf("--------------------------\n");
    struct dirent *entry;
    int count = 0;

    while ((entry = readdir(proc)) != NULL) {
        if (is_numeric_name(entry->d_name)) {
            printf("%s  ", entry->d_name);
            count++;
            if (count % 10 == 0) putchar('\n');
        }
    }
    if (count % 10 != 0) putchar('\n');
    printf("Total process entries: %d\n", count);
    closedir(proc);
}

static void command_disk(char *args[], int argc)
{
    const char *path = (argc >= 2) ? args[1] : ".";
    struct statvfs info;

    if (statvfs(path, &info) != 0) {
        perror("disk");
        return;
    }

    unsigned long long block_size = (unsigned long long)info.f_frsize;
    unsigned long long total = (unsigned long long)info.f_blocks * block_size;
    unsigned long long free_space = (unsigned long long)info.f_bfree * block_size;
    unsigned long long available = (unsigned long long)info.f_bavail * block_size;
    unsigned long long used = (total >= free_space) ? total - free_space : 0;

    printf("Disk Information for: %s\n", path);
    printf("-----------------------------\n");
    printf("Total:     %.2f GB\n", (double)total / (1024.0 * 1024.0 * 1024.0));
    printf("Used:      %.2f GB\n", (double)used / (1024.0 * 1024.0 * 1024.0));
    printf("Free:      %.2f GB\n", (double)free_space / (1024.0 * 1024.0 * 1024.0));
    printf("Available: %.2f GB\n", (double)available / (1024.0 * 1024.0 * 1024.0));
}

static void command_date(void)
{
    time_t now = time(NULL);
    struct tm *local = localtime(&now);
    char buffer[128];

    if (local == NULL || strftime(buffer, sizeof(buffer), "%a %b %d %H:%M:%S %Y", local) == 0) {
        fprintf(stderr, "date: unable to format current date/time\n");
        return;
    }
    printf("%s\n", buffer);
}

static void command_createfile(char *args[], int argc)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: createfile <name>\n");
        return;
    }

    int fd = open(args[1], O_WRONLY | O_CREAT | O_EXCL, 0644);
    if (fd < 0) {
        perror("createfile");
        return;
    }

    if (close(fd) != 0) {
        perror("createfile: close");
        return;
    }
    printf("File '%s' created successfully.\n", args[1]);
}

static void print_permission_set(mode_t mode)
{
    putchar((mode & S_IRUSR) ? 'r' : '-');
    putchar((mode & S_IWUSR) ? 'w' : '-');
    putchar((mode & S_IXUSR) ? 'x' : '-');
    putchar((mode & S_IRGRP) ? 'r' : '-');
    putchar((mode & S_IWGRP) ? 'w' : '-');
    putchar((mode & S_IXGRP) ? 'x' : '-');
    putchar((mode & S_IROTH) ? 'r' : '-');
    putchar((mode & S_IWOTH) ? 'w' : '-');
    putchar((mode & S_IXOTH) ? 'x' : '-');
}

static void command_permissions(char *args[], int argc)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: permissions <file>\n");
        return;
    }

    struct stat info;
    if (stat(args[1], &info) != 0) {
        perror("permissions");
        return;
    }

    printf("File: %s\n", args[1]);
    printf("Permissions: ");
    print_permission_set(info.st_mode);
    printf("\nNumeric permissions: %03o\n", (unsigned int)(info.st_mode & 0777));
    printf("Owner read:  %s\n", (info.st_mode & S_IRUSR) ? "yes" : "no");
    printf("Owner write: %s\n", (info.st_mode & S_IWUSR) ? "yes" : "no");
    printf("Owner exec:  %s\n", (info.st_mode & S_IXUSR) ? "yes" : "no");
    printf("Group read:  %s\n", (info.st_mode & S_IRGRP) ? "yes" : "no");
    printf("Group write: %s\n", (info.st_mode & S_IWGRP) ? "yes" : "no");
    printf("Group exec:  %s\n", (info.st_mode & S_IXGRP) ? "yes" : "no");
    printf("Other read:  %s\n", (info.st_mode & S_IROTH) ? "yes" : "no");
    printf("Other write: %s\n", (info.st_mode & S_IWOTH) ? "yes" : "no");
    printf("Other exec:  %s\n", (info.st_mode & S_IXOTH) ? "yes" : "no");
}

static void command_readfile(char *args[], int argc)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: readfile <file>\n");
        return;
    }

    FILE *file = fopen(args[1], "r");
    if (file == NULL) {
        perror("readfile");
        return;
    }

    char buffer[512];
    int had_content = 0;
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        fputs(buffer, stdout);
        had_content = 1;
    }
    if (ferror(file)) {
        perror("readfile");
    } else if (!had_content) {
        printf("(File is empty)\n");
    }
    fclose(file);
}

static void command_writefile(char *args[], int argc)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: writefile <file>\n");
        fprintf(stderr, "After running it, type your text. Enter a single . on a line to save.\n");
        return;
    }

    FILE *file = fopen(args[1], "w");
    if (file == NULL) {
        perror("writefile");
        return;
    }

    printf("Enter text for '%s'. Type a single . on a line to save and finish.\n", args[1]);
    char line[LINUXCORE_INPUT_SIZE];

    while (1) {
        printf("> ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            if (ferror(stdin)) perror("writefile: input");
            break;
        }

        trim_newline(line);
        if (strcmp(line, ".") == 0) break;

        if (fprintf(file, "%s\n", line) < 0) {
            perror("writefile");
            break;
        }
    }

    if (fclose(file) != 0) {
        perror("writefile: close");
        return;
    }
    printf("Text saved to '%s'.\n", args[1]);
}

static void execute_external(char *args[])
{
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return;
    }

    if (pid == 0) {
        execvp(args[0], args);
        perror(args[0]);
        _exit(127);
    }

    int status;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) {
            perror("waitpid");
            return;
        }
    }
}

static int run_command(char *args[], int argc)
{
    if (argc == 0) return 1;

    if (strcmp(args[0], "help") == 0) {
        print_help();
    } else if (strcmp(args[0], "pwd") == 0) {
        command_pwd();
    } else if (strcmp(args[0], "cd") == 0) {
        command_cd(args);
    } else if (strcmp(args[0], "ls") == 0) {
        command_ls(args, argc);
    } else if (strcmp(args[0], "echo") == 0) {
        command_echo(args, argc);
    } else if (strcmp(args[0], "history") == 0) {
        command_history();
    } else if (strcmp(args[0], "sysinfo") == 0) {
        command_sysinfo();
    } else if (strcmp(args[0], "cpu") == 0) {
        command_cpu();
    } else if (strcmp(args[0], "memory") == 0) {
        command_memory();
    } else if (strcmp(args[0], "processes") == 0) {
        command_processes();
    } else if (strcmp(args[0], "disk") == 0) {
        command_disk(args, argc);
    } else if (strcmp(args[0], "date") == 0) {
        command_date();
    } else if (strcmp(args[0], "createfile") == 0) {
        command_createfile(args, argc);
    } else if (strcmp(args[0], "permissions") == 0) {
        command_permissions(args, argc);
    } else if (strcmp(args[0], "readfile") == 0) {
        command_readfile(args, argc);
    } else if (strcmp(args[0], "writefile") == 0) {
        command_writefile(args, argc);
    } else if (strcmp(args[0], "clear") == 0) {
        printf("\033[2J\033[H");
        fflush(stdout);
    } else if (strcmp(args[0], "exit") == 0 || strcmp(args[0], "quit") == 0) {
        return 0;
    } else {
        execute_external(args);
    }

    return 1;
}

int main(void)
{
    char input[LINUXCORE_INPUT_SIZE];

    printf("========================================\n");
    printf("       Welcome to LinuxCore v%s\n", LINUXCORE_VERSION);
    printf("       A simple custom Linux shell\n");
    printf("Type 'help' to view available commands.\n");
    printf("========================================\n");

    while (1) {
        /* Keep the prompt simple: do not display the username or directory. */
        printf("LinuxCore$ ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            if (feof(stdin)) {
                putchar('\n');
                break;
            }
            perror("input");
            clearerr(stdin);
            continue;
        }

        trim_newline(input);

        /* Ignore blank commands. */
        char *start = input;
        while (*start != '\0' && isspace((unsigned char)*start)) start++;
        if (*start == '\0') continue;

        add_history(start);

        char working_copy[LINUXCORE_INPUT_SIZE];
        snprintf(working_copy, sizeof(working_copy), "%s", start);

        char *args[MAX_ARGS];
        int argc = tokenize(working_copy, args, MAX_ARGS);

        if (!run_command(args, argc)) {
            printf("Exiting LinuxCore. Goodbye!\n");
            break;
        }
    }

    return 0;
}
