# Operating Systems Lab

# Week 4-1 Practical – File System System Calls

## Aim

To understand and demonstrate file-system related system calls in C, including file opening, reading, writing, file-offset manipulation, file-descriptor duplication, file synchronization, file information retrieval, permission modification, access checking, file truncation, and file deletion.

---

## Procedure

### Step 1: Navigate to the Practical Directory

**Command**

```bash
cd ~/Documents/ForkFusion_OSSP/2520030541-Practical/"Week 4-1 Practical"
```

**Output**

```text
(No output)
```

**Explanation**

Changes the current working directory to the Week 4-1 practical directory.

---

### Step 2: Verify the Practical Files

**Command**

```bash
ls
```

**Output**

```text
commands.md
README.md
week41
week41.c
```

**Explanation**

Lists the files currently present in the practical directory.

---

### Step 3: Open the C Program

**Command**

```bash
open -a TextEdit week41.c
```

**Output**

```text
(TextEdit opens the file)
```

**Explanation**

Opens `week41.c` for viewing or editing.

---

### Step 4: Compile the Program

**Command**

```bash
clang week41.c -o week41
```

**Output**

```text
1 warning generated.
```

**Explanation**

Compiles `week41.c` and creates the executable `week41`. The warning concerns the format specifier used for `fileInfo.st_size`, but compilation completes successfully.

---

### Step 5: Execute the Program

**Command**

```bash
./week41
```

**Output**

```text
1. File opened successfully.
 File Descriptor = 3

2. Data written successfully.
3. File offset moved to beginning.

4. Data read from file:
 Hello Linux File System Calls

5. dup() successful.
 Original FD = 3

6. dup2() successful.
 Original FD = 3
 Specified Duplicate = 10

7. Data written using duplicated descriptor.

8. fsync() successful.
 File data synchronized with storage.

File closed successfully.

File Information:
File size: 56 bytes
File permissions: 644

10. chmod() successful.
 File permissions changed to 600.

11. access() successful.
 File exists.
 Read permission is available.
 Write permission is available.

12. truncate() successful.
 File size changed to 10 bytes.

File deleted successfully.
```

**Explanation**

Executes the program and demonstrates a sequence of file-system operations.

The program opens and writes to a file, moves the file offset, reads the stored data, duplicates file descriptors using `dup()` and `dup2()`, synchronizes file data using `fsync()`, obtains file information using `stat()`, changes permissions using `chmod()`, checks accessibility using `access()`, changes the file size using `truncate()`, and finally removes the file using `unlink()`.

---

### Step 6: Verify the Directory After Execution

**Command**

```bash
ls
```

**Output**

```text
commands.md
README.md
week41
week41.c
```

**Explanation**

Displays the files remaining after execution. The temporary `sample.txt` file is not present because it was deleted by the program using `unlink()`.

---

### Step 7: Remove the Compiled Executable

**Command**

```bash
rm week41
```

**Output**

```text
(No output)
```

**Explanation**

Removes the compiled executable so that only the source program and documentation remain in the practical directory.

---

### Step 8: Verify the Final Practical Files

**Command**

```bash
ls
```

**Output**

```text
commands.md
README.md
week41.c
```

**Explanation**

Confirms that the practical directory contains the source code and documentation required for submission.

---

## Result

Successfully compiled and executed the file-system system-call program and demonstrated file opening, reading, writing, file-descriptor duplication, synchronization, file information retrieval, permission management, access checking, truncation, and deletion.
