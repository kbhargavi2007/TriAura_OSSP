# Skill 2 - Dynamic Memory Management
# Commands Used

## 1. Navigate to the Dynamic-Memory Folder

I moved to the Dynamic-Memory folder inside Task-14:

```bash
cd ~/Documents/ForkFusion_OSSP/2520030541-Skill/Task-14/Dynamic-Memory
```

---

## 2. Check the Folder Contents

I checked the files present in the folder:

```bash
ls -l
```

---

# 3. Normal Array Program

## Create the File

```bash
touch normal_array.c
```

## Open the File

```bash
nano normal_array.c
```

## Compile

```bash
gcc normal_array.c -o normal_array
```

## Execute

```bash
./normal_array
```

The program was tested using a fixed-size integer array.

---

# 4. Dynamic Memory Using `malloc()`

## Create the File

```bash
touch dynamic_memory.c
```

## Open the File

```bash
nano dynamic_memory.c
```

## Compile

```bash
gcc dynamic_memory.c -o dynamic_memory
```

## Execute

```bash
./dynamic_memory
```

The program dynamically allocated memory using `malloc()` and released it using `free()`.

---

# 5. Dynamic Memory Using `calloc()`

## Create the File

```bash
touch calloc_memory.c
```

## Open the File

```bash
nano calloc_memory.c
```

## Compile

```bash
gcc calloc_memory.c -o calloc_memory
```

## Execute

```bash
./calloc_memory
```

The program demonstrated dynamically allocated memory initialized to zero using `calloc()`.

---

# 6. Resize Memory Using `realloc()`

## Create the File

```bash
touch realloc_memory.c
```

## Open the File

```bash
nano realloc_memory.c
```

## Compile

```bash
gcc realloc_memory.c -o realloc_memory
```

## Execute

```bash
./realloc_memory
```

The program demonstrated increasing the size of dynamically allocated memory.

---

# 7. Combined `malloc()` and `realloc()` Program

## Create the File

```bash
touch malloc_realloc.c
```

## Open the File

```bash
nano malloc_realloc.c
```

## Compile

```bash
gcc malloc_realloc.c -o malloc_realloc
```

## Execute

```bash
./malloc_realloc
```

The program demonstrated:

```text
malloc()
   ↓
Allocate memory
   ↓
Store elements
   ↓
realloc()
   ↓
Increase memory
   ↓
Add elements
   ↓
free()
```

---

# 8. `real.c` - Increase and Decrease Memory

The `real.c` program was used to demonstrate the dynamic-memory example provided during the practical work.

## Open the File

```bash
nano real.c
```

## Compile

```bash
gcc real.c -o real
```

## Execute

```bash
./real
```

The program demonstrated:

- Initial memory allocation using `malloc()`
- Increasing memory using `realloc()`
- Adding two elements
- Decreasing memory using `realloc()`
- Releasing memory using `free()`

---

# 9. 20-Byte Heap Memory

## Create the File

```bash
touch heap_20bytes.c
```

## Open the File

```bash
nano heap_20bytes.c
```

## Compile

```bash
gcc heap_20bytes.c -o heap_20bytes
```

## Execute

```bash
./heap_20bytes
```

The program allocated:

```text
20 bytes
```

of heap memory.

The input was limited to 19 characters to leave one byte for the null terminator.

---

# 10. Buffer Overflow Prevention

## Create the File

```bash
touch buffer_overflow.c
```

## Open the File

```bash
nano buffer_overflow.c
```

## Compile

```bash
gcc buffer_overflow.c -o buffer_overflow
```

## Execute

```bash
./buffer_overflow
```

The program used array bounds to ensure that input was stored only within the allocated memory.

---

# 11. Linked List

## Create the File

```bash
touch linked_list.c
```

## Open the File

```bash
nano linked_list.c
```

## Compile with Clang

For memory verification, I compiled the linked-list program with debugging information:

```bash
clang -g linked_list.c -o linked_list
```

## Execute

```bash
./linked_list
```

The program was tested with 10 nodes:

```text
1
2
3
4
5
6
7
8
9
10
```

The resulting linked list was:

```text
1 -> 2 -> 3 -> 4 -> 5 -> 6 -> 7 -> 8 -> 9 -> 10 -> NULL
```

---

# 12. Check for Valgrind

I checked whether Valgrind was available:

```bash
valgrind --version
```

The result on my Mac was:

```text
zsh: command not found: valgrind
```

Therefore, Valgrind was not installed on the macOS environment.

---

# 13. Check the Operating Environment

I checked whether Ubuntu was available:

```bash
which ubuntu
```

Result:

```text
ubuntu not found
```

I also checked the system architecture:

```bash
uname -m
```

Result:

```text
arm64
```

This confirmed that the system is an Apple Silicon ARM64 Mac.

---

# 14. Check macOS Memory Tools

I checked whether the macOS `leaks` utility was available:

```bash
which leaks
```

Result:

```text
/usr/bin/leaks
```

I also checked:

```bash
which malloc_history
```

Result:

```text
/usr/bin/malloc_history
```

The C compiler was checked using:

```bash
which clang
```

Result:

```text
/usr/bin/clang
```

---

# 15. Memory Verification Using macOS `leaks`

Since the development environment is macOS, I used the native `leaks` utility for memory-leak verification.

The linked-list program was compiled with debugging information:

```bash
clang -g linked_list.c -o linked_list
```

The program was then checked using:

```bash
leaks --atExit -- ./linked_list
```

The program was tested with 10 dynamically allocated nodes.

The final memory report showed:

```text
Process 66963: 189 nodes malloced for 21 KB
Process 66963: 0 leaks for 0 total leaked bytes.
```

Therefore, the linked-list program released its dynamically allocated memory correctly during the test.

---

# 16. Important Commands Summary

| Purpose | Command |
|---|---|
| Navigate to folder | `cd ~/Documents/ForkFusion_OSSP/2520030541-Skill/Task-14/Dynamic-Memory` |
| List files | `ls -l` |
| Create C file | `touch filename.c` |
| Edit C file | `nano filename.c` |
| Compile with GCC | `gcc filename.c -o filename` |
| Compile with Clang | `clang -g filename.c -o filename` |
| Run program | `./filename` |
| Check Valgrind | `valgrind --version` |
| Check Ubuntu | `which ubuntu` |
| Check architecture | `uname -m` |
| Check `leaks` | `which leaks` |
| Check `malloc_history` | `which malloc_history` |
| Check Clang | `which clang` |
| Memory verification | `leaks --atExit -- ./linked_list` |

---

# 17. Final Verification

The Dynamic Memory work was tested using multiple C programs covering:

```text
Normal Array
     ↓
malloc()
     ↓
calloc()
     ↓
realloc()
     ↓
Heap Memory
     ↓
20-Byte Allocation
     ↓
Buffer Overflow Prevention
     ↓
Linked List
     ↓
free()
     ↓
Memory Verification
```

The linked-list memory verification on macOS reported:

```text
0 leaks for 0 total leaked bytes.
```

This completed the Dynamic Memory section of Skill 2.
