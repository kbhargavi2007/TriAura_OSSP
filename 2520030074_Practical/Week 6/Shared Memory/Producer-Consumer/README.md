# System V Shared Memory – Producer–Consumer

## 1. Objective

To demonstrate **Inter-Process Communication (IPC) using System V Shared Memory**, where a Producer process writes the marks of five subjects into a shared-memory segment and a Consumer process reads the marks and calculates the total and average.

---

## 2. Problem Statement

The program demonstrates communication between two independent processes using shared memory.

The **Producer**:

- Creates a shared-memory segment.
- Accepts marks for five subjects.
- Stores the marks in shared memory.

The **Consumer**:

- Accesses the existing shared-memory segment.
- Reads the five marks.
- Calculates the total marks.
- Calculates the average marks.
- Detaches from the shared memory.
- Removes the shared-memory segment after use.

---

## 3. Producer–Consumer Model

```text
                 PRODUCER
                    |
                    | Writes marks
                    v
          +----------------------+
          |   SHARED MEMORY      |
          |                      |
          |  Subject 1           |
          |  Subject 2           |
          |  Subject 3           |
          |  Subject 4           |
          |  Subject 5           |
          +----------------------+
                    |
                    | Reads marks
                    v
                 CONSUMER
                    |
                    v
             Total & Average
```

---

## 4. Program Structure

The implementation consists of two C programs.

### 4.1 `producer.c`

The Producer process:

1. Generates a shared-memory key using `ftok()`.
2. Creates a shared-memory segment using `shmget()`.
3. Attaches the shared memory using `shmat()`.
4. Accepts marks for five subjects.
5. Stores the marks in shared memory.
6. Detaches from the shared memory using `shmdt()`.

### 4.2 `consumer.c`

The Consumer process:

1. Generates the same shared-memory key using `ftok()`.
2. Accesses the existing shared-memory segment using `shmget()`.
3. Attaches to the shared memory using `shmat()`.
4. Reads the five subject marks.
5. Calculates the total marks.
6. Calculates the average marks.
7. Detaches from shared memory using `shmdt()`.
8. Removes the shared-memory segment using `shmctl()`.

---

## 5. System Calls / Functions Used

### `ftok()`

Generates a System V IPC key that is used to identify the shared-memory segment.

### `shmget()`

Creates a new shared-memory segment or accesses an existing shared-memory segment.

### `shmat()`

Attaches the shared-memory segment to the process's address space.

### `shmdt()`

Detaches the shared-memory segment from the process's address space.

### `shmctl()`

Performs control operations on the shared-memory segment and is used here to remove it after communication is complete.

---

## 6. Header Files Used

The program uses the following standard and System V IPC header files:

```c
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
```

---

## 7. Compilation

Compile the Producer:

```bash
gcc producer.c -o producer
```

Compile the Consumer:

```bash
gcc consumer.c -o consumer
```

---

## 8. Execution

### Step 1 — Create the key file

```bash
touch shmfile
```

### Step 2 — Run the Producer

```bash
./producer
```

Enter the marks for five subjects.

### Step 3 — Run the Consumer

```bash
./consumer
```

The Consumer reads the marks from shared memory and displays the total and average.

---

## 9. Sample Input

```text
Subject 1: 85
Subject 2: 90
Subject 3: 78
Subject 4: 92
Subject 5: 88
```

---

## 10. Sample Output

### Producer

```text
========================================
          PRODUCER PROCESS
========================================

Enter marks for 5 subjects:

Subject 1: 85
Subject 2: 90
Subject 3: 78
Subject 4: 92
Subject 5: 88

Marks successfully written to shared memory.
Producer detached from shared memory.
```

### Consumer

```text
========================================
          CONSUMER PROCESS
========================================

Student marks read from shared memory:

Subject 1 : 85
Subject 2 : 90
Subject 3 : 78
Subject 4 : 92
Subject 5 : 88

Total Marks : 433
Average     : 86.60

Shared memory removed successfully.
```

---

## 11. Concepts Demonstrated

This program demonstrates:

- Inter-Process Communication (IPC)
- System V Shared Memory
- Producer–Consumer communication
- Shared-memory creation
- Shared-memory attachment
- Shared-memory detachment
- Shared-memory cleanup
- Data sharing between independent processes
- Calculation using shared data

---

## 12. Expected Result

The Producer successfully writes five subject marks into the shared-memory segment.

The Consumer accesses the same shared-memory segment, reads the marks, calculates the total and average, and displays the results.

After the Consumer finishes, the shared-memory segment is removed using `shmctl()`.

---

## 13. Files

```text
Producer-Consumer/
│
├── producer.c
├── consumer.c
├── README.md
└── shmfile
```

> **Note:** `shmfile` is used as the key-generation file for `ftok()` and does not contain the student marks themselves.

---

## 14. Conclusion

The program successfully demonstrates **System V Shared Memory IPC** using a Producer–Consumer model. The Producer writes data into shared memory, while the Consumer reads and processes the shared data before cleaning up the shared-memory segment.
