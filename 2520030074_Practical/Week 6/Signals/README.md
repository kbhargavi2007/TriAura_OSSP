# Linux Signals – Signal Handling Programs

## 1. Objective

To demonstrate the concept of **signals in Linux/POSIX systems** through different C programs.

These programs demonstrate signal handling, signal generation, signal handlers, infinite-loop behaviour, normal program execution, and segmentation-fault (`SIGSEGV`) handling.

---

## 2. Introduction

A **signal** is a software notification sent to a process to inform it that a particular event has occurred.

Signals are an important mechanism for communication between the operating system and processes.

A process can:

- Receive a signal.
- Handle a signal using a signal handler.
- Generate a signal.
- Terminate because of a signal.
- Perform a specific action when a signal is received.

---

## 3. Programs Included

This folder contains six C programs demonstrating different signal concepts.

| Program | Concept Demonstrated |
|---|---|
| `signal_handler.c` | Signal handling using a signal handler |
| `signal_infiniteloop.c` | Signal handling with an infinite loop |
| `signal_no_infiniteloop.c` | Signal handling without an infinite loop |
| `signal_raise.c` | Generating a signal using `raise()` |
| `signal_SEGV.c` | Handling `SIGSEGV` |
| `signal_SEGV_pointer.c` | `SIGSEGV` caused by invalid pointer access |

---

# 4. Program Details

## 4.1 `signal_handler.c`

### Concept

**Signal Handler**

### Description

This program demonstrates how a process can register a function to handle a signal.

When the specified signal is received, the registered signal-handler function is executed instead of allowing the default signal action to occur.

### Key Concept

```c
signal()
```

The `signal()` function is used to associate a signal with a handler function.

### Demonstrates

- Signal registration
- Signal handler function
- Receiving a signal
- Custom handling of a signal

---

## 4.2 `signal_infiniteloop.c`

### Concept

**Signal Handling with Infinite Loop**

### Description

This program demonstrates a process that continues executing inside an infinite loop while signals can be received during its execution.

The infinite loop keeps the process running so that signal behaviour can be observed.

### Demonstrates

- Infinite-loop process execution
- Signal reception
- Signal handling while a process is continuously running

---

## 4.3 `signal_no_infiniteloop.c`

### Concept

**Signal Handling without Infinite Loop**

### Description

This program demonstrates signal handling in a program that does not continuously execute inside an infinite loop.

It helps demonstrate the difference between a continuously running process and a normally executing process when signals are involved.

### Demonstrates

- Signal handling
- Normal program execution
- Process termination
- Difference between continuous and normal execution

---

## 4.4 `signal_raise.c`

### Concept

**Generating a Signal using `raise()`**

### Description

This program demonstrates how a process can explicitly generate a signal for itself using the `raise()` function.

### Key Function

```c
raise()
```

The `raise()` function sends a signal to the calling process.

### Demonstrates

- Explicit signal generation
- Self-signalling
- Signal handling
- Use of the `raise()` function

---

## 4.5 `signal_SEGV.c`

### Concept

**SIGSEGV – Segmentation Fault**

### Description

This program demonstrates the `SIGSEGV` signal.

`SIGSEGV` is associated with an invalid memory access. A program may receive this signal when it attempts to access memory in an invalid manner.

The program demonstrates how `SIGSEGV` can be detected or handled.

### Key Signal

```text
SIGSEGV
```

### Demonstrates

- Segmentation fault
- Invalid memory access
- `SIGSEGV`
- Signal handling for memory-access errors

---

## 4.6 `signal_SEGV_pointer.c`

### Concept

**SIGSEGV caused by Invalid Pointer Access**

### Description

This program demonstrates a segmentation fault resulting from invalid pointer or memory access.

An invalid pointer operation can cause the operating system to generate the `SIGSEGV` signal for the process.

### Demonstrates

- Pointers
- Invalid memory access
- Segmentation fault
- `SIGSEGV`
- Signal handling

---

# 5. Important Signal Concepts

## 5.1 Signal

A signal is a software notification delivered to a process to indicate that an event has occurred.

Examples include:

```text
SIGINT
SIGTERM
SIGSEGV
SIGKILL
SIGSTOP
```

---

## 5.2 Signal Handler

A signal handler is a function that is executed when a process receives a particular signal.

Conceptually:

```text
Signal received
       |
       v
Signal Handler
       |
       v
Perform required action
```

---

## 5.3 `signal()`

The `signal()` function is used to specify the function that should be executed when a particular signal is received.

Example:

```c
signal(SIGINT, handler);
```

---

## 5.4 `raise()`

The `raise()` function allows a process to send a signal to itself.

Example:

```c
raise(SIGINT);
```

---

## 5.5 SIGSEGV

`SIGSEGV` stands for **Segmentation Violation**.

It is generated when a process performs an invalid memory access.

A common example is attempting to access memory through an invalid pointer.

---

# 6. Compilation

Each program can be compiled separately using `gcc`.

### `signal_handler.c`

```bash
gcc signal_handler.c -o signal_handler
```

### `signal_infiniteloop.c`

```bash
gcc signal_infiniteloop.c -o signal_infiniteloop
```

### `signal_no_infiniteloop.c`

```bash
gcc signal_no_infiniteloop.c -o signal_no_infiniteloop
```

### `signal_raise.c`

```bash
gcc signal_raise.c -o signal_raise
```

### `signal_SEGV.c`

```bash
gcc signal_SEGV.c -o signal_SEGV
```

### `signal_SEGV_pointer.c`

```bash
gcc signal_SEGV_pointer.c -o signal_SEGV_pointer
```

---

# 7. Execution

After compilation, execute the required program using:

```bash
./signal_handler
```

or:

```bash
./signal_infiniteloop
```

or:

```bash
./signal_no_infiniteloop
```

or:

```bash
./signal_raise
```

or:

```bash
./signal_SEGV
```

or:

```bash
./signal_SEGV_pointer
```

---

# 8. Signal Program Flow

The general signal-handling flow can be represented as:

```text
             Process
                |
                v
        Signal is generated
                |
                v
        Signal is delivered
                |
                v
        Signal handler checks
        the received signal
                |
                v
        Handler executes
                |
                v
        Process continues
        or terminates
```

---

# 9. Learning Outcomes

After completing these programs, the following concepts can be understood:

- Basics of Linux/POSIX signals
- Signal generation
- Signal delivery
- Signal handling
- Signal handlers
- `signal()` function
- `raise()` function
- Infinite-loop process behaviour
- Normal process execution
- `SIGSEGV`
- Invalid memory access
- Interaction between processes and the operating system

---

# 10. File Structure

```text
Signals/
│
├── signal_handler.c
├── signal_infiniteloop.c
├── signal_no_infiniteloop.c
├── signal_raise.c
├── signal_SEGV.c
├── signal_SEGV_pointer.c
└── README.md
```

---

# 11. Conclusion

The programs in this folder demonstrate fundamental Linux/POSIX signal concepts through practical C implementations.

The programs cover signal handlers, signal generation using `raise()`, infinite-loop and normal process behaviour, and segmentation-fault handling using `SIGSEGV`.

These practicals provide an understanding of how signals are used by the operating system to notify and control processes.
