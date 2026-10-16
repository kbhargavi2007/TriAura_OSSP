# Week 8 – Named Pipe (FIFO) and Parent-Child Process Communication

## Objective

To understand Inter-Process Communication (IPC) using Named Pipes (FIFOs) and to study communication between parent and child processes using the `fork()` system call.

---

# Program 1 – User A Using Named Pipes

## File

`program1_userA.c`

## Description

This program represents User A in a two-way communication system using two Named Pipes.

The program creates:

- `fifo1` – used to send messages from User A to User B.
- `fifo2` – used to receive messages from User B.

User A enters a message and sends it through `fifo1`. After sending the message, User A waits for a response through `fifo2`.

The communication continues until either user enters `bye`.

## Main Functions Used

- `mkfifo()` – creates a Named Pipe.
- `open()` – opens the FIFO.
- `write()` – sends data.
- `read()` – receives data.
- `close()` – closes the FIFO.
- `fgets()` – reads user input.
- `strcmp()` – compares strings.
- `strcspn()` – removes the newline character.

## Communication

```text
User A
   |
   | write()
   v
 fifo1
   |
   v
User B

User A
   ^
   | read()
   |
 fifo2
   ^
   |
User B
```

---

# Program 2 – User B Using Named Pipes

## File

`program2_userB.c`

## Description

This program represents User B in the same two-way communication system.

User B first receives a message from User A through `fifo1`.

After receiving the message, User B enters a response and sends it through `fifo2`.

The communication continues until either user enters `bye`.

## Communication

```text
User A
   |
   | fifo1
   v
User B
   |
   | fifo2
   v
User A
```

---

# Program 3 – Parent-Child Communication Using fork()

## File

`program3_fork_fifo.c`

## Description

This program demonstrates the use of the `fork()` system call together with a Named Pipe.

Two FIFOs are created:

- `A2B`
- `B2A`

The `fork()` system call creates a child process.

The parent process acts as User A and sends messages through `A2B`.

The child process acts as User B and receives messages from `A2B`.

## Process Structure

```text
                 Program
                    |
                  fork()
                 /      \
                /        \
           Parent        Child
           User A        User B
           Sender        Receiver
              |             |
              +---- A2B ----+
```

## fork() Return Values

### `pid < 0`

The child process could not be created.

```c
perror("Failed to create child");
```

### `pid == 0`

The current process is the child process.

### `pid > 0`

The current process is the parent process.

---

# Program 4 – Parent-Child Communication Using fork()

## File

`program4_fork_fifo.c`

## Description

This program also demonstrates `fork()` and Named Pipe communication.

The child process receives messages from User A through `A2B`.

The parent process acts as User B and sends messages through `B2A`.

## Process Structure

```text
                 Program
                    |
                  fork()
                 /      \
                /        \
           Parent        Child
           User B        User A
           Sender        Receiver
              |             |
              |             |
             B2A           A2B
```

---

# Named Pipes / FIFOs

A FIFO is a Named Pipe used for Inter-Process Communication.

The `mkfifo()` function creates a FIFO in the file system.

Example:

```c
mkfifo("fifo1", 0666);
```

The permission value `0666` allows the FIFO to be read and written according to the system's permission rules.

---

# Parent and Child Processes

The `fork()` system call creates a new child process.

After `fork()`, both parent and child continue execution from the point after the `fork()` call.

The return value identifies which process is executing:

```text
fork()
  |
  +-- pid < 0  → fork failed
  |
  +-- pid == 0 → child process
  |
  +-- pid > 0  → parent process
```

---

# Important System Calls

| Function | Purpose |
|---|---|
| `mkfifo()` | Creates a Named Pipe |
| `fork()` | Creates a child process |
| `open()` | Opens a FIFO |
| `read()` | Reads data |
| `write()` | Writes data |
| `close()` | Closes a file descriptor |
| `perror()` | Displays an error message |
| `exit()` | Terminates the process |

---

# Overall Learning Outcomes

After completing these programs, the following concepts are demonstrated:

1. Inter-Process Communication (IPC)
2. Named Pipes
3. FIFOs
4. `mkfifo()`
5. File descriptors
6. `open()`
7. `read()`
8. `write()`
9. `close()`
10. `fork()`
11. Parent processes
12. Child processes
13. Two-way communication
14. Process creation and communication

---

# Conclusion

Week 8 demonstrates Inter-Process Communication using Named Pipes and the creation of child processes using `fork()`.

Programs 1 and 2 demonstrate communication between User A and User B using two FIFOs.

Programs 3 and 4 demonstrate the use of `fork()` to create parent and child processes and use FIFOs for communication between them.
