# Teacher–Student Shared Memory

## 1. Objective

To demonstrate **Inter-Process Communication (IPC) using shared memory**, where a Teacher process stores the marks of two students in separate shared-memory regions, and each Student process accesses only its respective marks to calculate the total and average.

---

## 2. Problem Statement

The Teacher process maintains the marks of two students.

- Student 1 has five subject marks stored in `student1_marks`.
- Student 2 has five subject marks stored in `student2_marks`.
- Student 1 can access only `student1_marks`.
- Student 2 can access only `student2_marks`.
- Each student reads their five marks and calculates the total and average.
- After both students complete their calculations, the shared-memory objects are cleaned up.

---

## 3. Program Structure

The implementation consists of three C programs.

### 3.1 `producer.c` — Teacher / Producer

The Teacher process:

1. Creates two separate shared-memory regions:
   - `student1_marks`
   - `student2_marks`
2. Accepts five subject marks for Student 1.
3. Stores Student 1's marks in `student1_marks`.
4. Accepts five subject marks for Student 2.
5. Stores Student 2's marks in `student2_marks`.
6. Starts the respective student processes.
7. Waits for both student processes to complete.
8. Cleans up and unlinks the shared-memory objects.

### 3.2 `student1.c` — Student 1 / Consumer

Student 1:

1. Accesses only the `student1_marks` shared-memory region.
2. Reads the five subject marks.
3. Calculates the total marks.
4. Calculates the average marks.
5. Displays the marks, total, and average.
6. Closes its access to the shared memory.

### 3.3 `student2.c` — Student 2 / Consumer

Student 2:

1. Accesses only the `student2_marks` shared-memory region.
2. Reads the five subject marks.
3. Calculates the total marks.
4. Calculates the average marks.
5. Displays the marks, total, and average.
6. Closes its access to the shared memory.

---

## 4. Producer–Consumer Model

The Teacher acts as the **Producer**, while the two Student processes act as **Consumers**.

```text
                    TEACHER
                  (PRODUCER)
                       |
              -------------------
              |                 |
              v                 v
       student1_marks     student2_marks
              |                 |
              v                 v
         STUDENT 1          STUDENT 2
         (CONSUMER)         (CONSUMER)
              |                 |
              v                 v
        Read 5 marks       Read 5 marks
              |                 |
              v                 v
       Total & Average     Total & Average
              |                 |
              -----------+-------
                        |
                        v
               Both processes finish
                        |
                        v
              Shared-memory cleanup
```

---

## 5. Shared Memory Design

Two independent shared-memory objects are used.

| Shared-Memory Object | Accessed By | Data Stored |
|---|---|---|
| `student1_marks` | Teacher + Student 1 | Five marks of Student 1 |
| `student2_marks` | Teacher + Student 2 | Five marks of Student 2 |

This separation ensures that each student process accesses **only its own marks**.

---

## 6. System Calls / POSIX APIs Used

### `shm_open()`

Creates or opens a POSIX shared-memory object.

### `ftruncate()`

Sets the size of the shared-memory object.

### `mmap()`

Maps the shared-memory object into the process address space so that the process can read or write the shared data.

### `munmap()`

Unmaps the shared-memory region from the process address space.

### `shm_unlink()`

Removes the shared-memory object after the processes have completed.

### `fork()`

Creates child processes for the student processes.

### `wait()`

Allows the Teacher process to wait for the student processes to complete.

---

## 7. Compilation

Compile the three programs using:

```bash
gcc producer.c -o producer
gcc student1.c -o student1
gcc student2.c -o student2
```

---

## 8. Execution

Run the Teacher / Producer program:

```bash
./producer
```

The Teacher accepts five marks for Student 1 and five marks for Student 2.

The student processes then access their respective shared-memory regions and calculate their results.

---

## 9. Sample Input

### Student 1

```text
Subject 1: 85
Subject 2: 90
Subject 3: 78
Subject 4: 92
Subject 5: 88
```

### Student 2

```text
Subject 1: 75
Subject 2: 82
Subject 3: 91
Subject 4: 68
Subject 5: 84
```

---

## 10. Sample Output

### Student 1

```text
========================================
              STUDENT 1
========================================

Subject 1 : 85
Subject 2 : 90
Subject 3 : 78
Subject 4 : 92
Subject 5 : 88

Total   : 433
Average : 86.60
```

### Student 2

```text
========================================
              STUDENT 2
========================================

Subject 1 : 75
Subject 2 : 82
Subject 3 : 91
Subject 4 : 68
Subject 5 : 84

Total   : 400
Average : 80.00
```

After both student processes finish:

```text
Both student processes completed.
Shared-memory objects cleaned up successfully.
```

---

## 11. Concepts Demonstrated

This program demonstrates:

- Inter-Process Communication (IPC)
- POSIX Shared Memory
- Producer–Consumer communication
- Process creation using `fork()`
- Parent–child process synchronization using `wait()`
- Memory mapping using `mmap()`
- Shared-memory cleanup using `shm_unlink()`
- Separate access to shared data
- Communication between multiple processes

---

## 12. Expected Result

The Teacher successfully stores the marks of both students in separate shared-memory regions.

Each Student process accesses only its assigned shared-memory region, calculates the total and average marks, and displays the results.

After both student processes complete their calculations, the Teacher cleans up the shared-memory objects.

---

## 13. Files

```text
Teacher-Student/
│
├── producer.c
├── student1.c
├── student2.c
└── README.md
```

---

## 14. Conclusion

The program successfully demonstrates IPC using shared memory. It shows how a Producer process can share data with multiple Consumer processes while maintaining separate shared-memory regions for independent student data.
