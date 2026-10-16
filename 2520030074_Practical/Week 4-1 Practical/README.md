# Week 4-1 Practical – File System System Calls

## Objective

To demonstrate and understand commonly used file-system system calls in C.

## Description

The program demonstrates file operations using system calls for opening, writing, reading, changing the file offset, duplicating file descriptors, synchronizing file data, obtaining file information, changing permissions, checking file accessibility, changing file size, and deleting a file.

## System Calls Demonstrated

- `open()`
- `write()`
- `lseek()`
- `read()`
- `dup()`
- `dup2()`
- `fsync()`
- `close()`
- `stat()`
- `chmod()`
- `access()`
- `truncate()`
- `unlink()`

## Program

The program creates and manipulates `sample.txt` using the above file-system system calls. It finally truncates the file and deletes it using `unlink()`.

## Result

The program executed successfully and demonstrated the required file-system operations.