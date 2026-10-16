# Skill (TASK-14 PART2) - Dynamic Memory Management

## 1. Overview

This folder contains the second part of **Skill 2**, focused on dynamic memory management in C.

The work covers:

- Dynamic memory allocation using `malloc()`
- Zero-initialized dynamic memory allocation using `calloc()`
- Resizing dynamically allocated memory using `realloc()`
- Increasing and decreasing memory size
- Heap memory
- 20-byte memory allocation
- Preventing buffer overflow
- Managing dynamically allocated linked lists
- Releasing memory correctly using `free()`
- Verifying memory usage and memory leaks

The programs were developed and tested on **macOS Apple Silicon (ARM64)**.

---

## 2. Objectives

The main objectives of this practical work are:

1. Understand dynamic memory allocation.
2. Allocate memory dynamically using `malloc()`.
3. Allocate zero-initialized memory using `calloc()`.
4. Resize dynamically allocated memory using `realloc()`.
5. Understand heap memory.
6. Allocate a specific amount of memory, such as 20 bytes.
7. Prevent access beyond the allocated memory.
8. Create and manage linked-list nodes dynamically.
9. Release dynamically allocated memory using `free()`.
10. Verify that dynamically allocated memory is properly released.

---

## 3. Static Array vs Dynamic Memory

A normal array has a fixed size.

Example:

```c
int ar[10];
```

The array can store a maximum of 10 integers.

Dynamic memory allows the required amount of memory to be requested during program execution.

Example:

```c
int *p;

p = (int *)malloc(n * sizeof(int));
```

The amount of memory depends on the value of `n`.

---

## 4. Heap Memory

Dynamic memory allocated using functions such as `malloc()`, `calloc()`, and `realloc()` is obtained from the heap.

Basic flow:

```text
Program
   |
   v
malloc()
   |
   v
Heap Memory
   |
   v
Use allocated memory
   |
   v
free()
   |
   v
Memory Released
```

For example:

```c
int *p;

p = (int *)malloc(5 * sizeof(int));
```

This requests enough heap memory to store five integers.

---

# 5. Programs Implemented

## 5.1 `normal_array.c`

This program demonstrates a normal fixed-size array.

```c
int n, ar[10];
```

The array has a fixed capacity of 10 integers.

The program:

1. Accepts the required number of elements.
2. Stores the elements in the array.
3. Displays the stored elements.
4. Prevents the user from entering more than 10 elements.

---

## 5.2 `dynamic_memory.c`

This program demonstrates dynamic memory allocation using `malloc()`.

Basic operation:

```c
p = (int *)malloc(n * sizeof(int));
```

The program:

1. Accepts the required number of elements.
2. Allocates memory dynamically.
3. Stores values in the allocated memory.
4. Displays the values.
5. Releases the memory using `free()`.

Memory flow:

```text
User enters n
     |
     v
malloc(n * sizeof(int))
     |
     v
Heap memory allocated
     |
     v
Store elements
     |
     v
Display elements
     |
     v
free(p)
```

---

## 5.3 `calloc_memory.c`

This program demonstrates dynamic memory allocation using `calloc()`.

```c
p = (int *)calloc(n, sizeof(int));
```

Unlike the basic `malloc()` example, the allocated memory is initialized to zero.

For example, when five integers are allocated, the initial values are:

```text
0
0
0
0
0
```

The program then accepts new values, displays them, and releases the allocated memory.

---

## 5.4 `realloc_memory.c`

This program demonstrates resizing dynamically allocated memory using `realloc()`.

The memory can be resized using:

```c
p = (int *)realloc(p, new_size * sizeof(int));
```

The program demonstrates increasing the number of elements in a dynamically allocated array.

Example:

```text
Initial allocation
       |
       v
[10][20][30]
       |
       | realloc()
       v
[10][20][30][40][50]
```

---

## 5.5 `malloc_realloc.c`

This program combines dynamic allocation and memory resizing.

The sequence is:

```text
malloc()
   |
   v
Allocate memory
   |
   v
Store elements
   |
   v
realloc()
   |
   v
Increase memory
   |
   v
Add additional elements
   |
   v
free()
```

The program demonstrates the complete allocation, resizing, and release process.

---

## 5.6 `real.c`

This program is based on the dynamic-memory example provided during the practical work.

It demonstrates:

- Initial allocation using `malloc()`
- Increasing memory using `realloc()`
- Adding elements after increasing the allocation
- Decreasing memory using `realloc()`
- Releasing memory using `free()`

The overall flow is:

```text
Initial memory
      |
      v
   malloc()
      |
      v
Store elements
      |
      v
realloc(n + 2)
      |
      v
Increase memory
      |
      v
Add elements
      |
      v
realloc(n - 3)
      |
      v
Decrease memory
      |
      v
   free()
```

The program also checks the input size before decreasing the allocation.

---

## 5.7 `heap_20bytes.c`

This program demonstrates allocating **20 bytes of heap memory**.

The allocation is performed using:

```c
p = (char *)malloc(20 * sizeof(char));
```

The program accepts a string with a maximum of 19 characters.

The reason for the 19-character limit is that one byte is required for the null terminator:

```text
20 bytes
|
+-- 19 bytes for characters
|
+-- 1 byte for '\0'
```

The memory is released using:

```c
free(p);
```

---

## 5.8 `buffer_overflow.c`

This program demonstrates safe access to dynamically allocated array memory.

Memory is allocated according to the number of elements:

```c
p = (int *)malloc(n * sizeof(int));
```

The input loop is limited to:

```c
for (int i = 0; i < n; i++)
```

Therefore, only valid indexes from `0` to `n - 1` are accessed.

```text
Allocated memory

[ p[0] ][ p[1] ][ p[2] ] ... [ p[n-1] ]
    |        |        |             |
    +--------+--------+-------------+
              Valid range
```

The program avoids accessing:

```c
p[n]
```

because that position lies outside the allocated memory.

---

# 6. Linked List and Dynamic Memory

## 6.1 `linked_list.c`

A linked list is another important application of dynamic memory.

The node structure is:

```c
struct Node {
    int data;
    struct Node *next;
};
```

Each node is dynamically allocated:

```c
newNode = (struct Node *)malloc(sizeof(struct Node));
```

A simple linked list looks like:

```text
HEAD
 |
 v
+--------+------+     +--------+------+     +--------+------+
|  data  | next | --> |  data  | next | --> |  data  | next | --> NULL
+--------+------+     +--------+------+     +--------+------+
```

Example:

```text
10 -> 20 -> 30 -> NULL
```

Each node is allocated dynamically on the heap.

At the end, every node is released using `free()`.

---

# 7. Memory Release

Dynamically allocated memory should be released after it is no longer required.

The function used is:

```c
free(p);
```

For linked lists, each dynamically allocated node must also be released.

Example:

```c
while (temp != NULL) {
    struct Node *nextNode = temp->next;
    free(temp);
    temp = nextNode;
}
```

This prevents dynamically allocated memory from remaining unnecessarily allocated.

---

# 8. Memory Verification on macOS

The development environment for this work is:

```text
Operating System: macOS
Architecture: ARM64
Compiler: Clang
```

Valgrind was not installed on the macOS environment.

Instead, the macOS-native `leaks` utility was used for memory-leak verification.

The availability of the tool was checked using:

```bash
which leaks
```

Result:

```text
/usr/bin/leaks
```

---

## 8.1 Linked List Memory Verification

The linked-list program was compiled with debugging information:

```bash
clang -g linked_list.c -o linked_list
```

The program was tested with 10 nodes:

```text
1 -> 2 -> 3 -> 4 -> 5 -> 6 -> 7 -> 8 -> 9 -> 10 -> NULL
```

The program reported:

```text
All linked-list memory released successfully.
```

Memory verification was then performed using:

```bash
leaks --atExit -- ./linked_list
```

The final memory report showed:

```text
Process 66963: 189 nodes malloced for 21 KB
Process 66963: 0 leaks for 0 total leaked bytes.
```

Therefore, the linked-list program successfully released its dynamically allocated memory during this test.

---

# 9. Important Dynamic Memory Functions

| Function | Purpose |
|---|---|
| `malloc()` | Allocates dynamic memory |
| `calloc()` | Allocates dynamic memory and initializes it to zero |
| `realloc()` | Changes the size of an existing allocation |
| `free()` | Releases dynamically allocated memory |

---

# 10. Complete Dynamic Memory Flow

```text
                  DYNAMIC MEMORY
                        |
        +---------------+---------------+
        |               |               |
        v               v               v
     malloc()        calloc()       realloc()
        |               |               |
        |               |               |
        +---------------+---------------+
                        |
                        v
                  Heap Memory
                        |
                        v
                   Use Memory
                        |
                        v
                      free()
                        |
                        v
                 Memory Released
```

---

# 11. Concepts Demonstrated

This practical work demonstrates the following concepts:

- Fixed-size arrays
- Pointers
- Heap memory
- Dynamic memory allocation
- `malloc()`
- `calloc()`
- `realloc()`
- `free()`
- Memory resizing
- Increasing allocation size
- Decreasing allocation size
- 20-byte memory allocation
- Array bounds checking
- Buffer overflow prevention
- Dynamic linked-list nodes
- Memory leak verification

---

# 12. Final Folder Structure

```text
Dynamic-Memory/
│
├── normal_array.c
├── normal_array
│
├── dynamic_memory.c
├── dynamic_memory
│
├── calloc_memory.c
├── calloc_memory
│
├── realloc_memory.c
├── realloc_memory
│
├── malloc_realloc.c
├── malloc_realloc
│
├── real.c
├── real
│
├── heap_20bytes.c
├── heap_20bytes
│
├── buffer_overflow.c
├── buffer_overflow
│
├── linked_list.c
├── linked_list
│
└── README.md
```

---

# 13. Conclusion

The Dynamic Memory section of Skill 2 was completed by implementing and testing programs covering allocation, initialization, resizing, heap memory, buffer safety, linked lists, and memory release.

The linked-list implementation was additionally verified using the macOS `leaks` utility. The verification reported:

```text
0 leaks for 0 total leaked bytes.
```

This confirms that the dynamically allocated linked-list nodes were released correctly during the tested execution.
