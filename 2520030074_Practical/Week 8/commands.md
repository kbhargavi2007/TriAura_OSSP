# Week 8 – Commands

## 1. Navigate to the Practical Directory

```bash
cd ~/Documents/ForkFusion_OSSP/2520030541-Practical
```

## 2. Create Week 8 Directory

```bash
mkdir "Week 8"
```

## 3. Enter Week 8

```bash
cd "Week 8"
```

## 4. Create Source and Documentation Files

```bash
touch program1_userA.c program2_userB.c program3_fork_fifo.c program4_fork_fifo.c README.md commands.md
```

## 5. List the Files

```bash
ls
```

---

# Program 1

## Create/Edit Program 1

```bash
nano program1_userA.c
```

## Compile

```bash
clang program1_userA.c -o program1_userA
```

## Run

```bash
./program1_userA
```

Program 1 should be run together with Program 2 in separate terminals.

---

# Program 2

## Create/Edit Program 2

```bash
nano program2_userB.c
```

## Compile

```bash
clang program2_userB.c -o program2_userB
```

## Run

```bash
./program2_userB
```

Program 2 should be run together with Program 1 in a separate terminal.

---

# Testing Programs 1 and 2

Terminal 1:

```bash
./program1_userA
```

Terminal 2:

```bash
./program2_userB
```

Example communication:

```text
User A: Hello
User A: Hello
User B: Hi
User B: Hi
```

The exact order displayed depends on which terminal is currently waiting for input.

To terminate the communication, enter:

```text
bye
```

---

# Remove FIFO Files After Testing

```bash
rm -f fifo1 fifo2
```

---

# Program 3

## Create/Edit Program 3

```bash
nano program3_fork_fifo.c
```

## Compile

```bash
clang program3_fork_fifo.c -o program3_fork_fifo
```

## Run

```bash
./program3_fork_fifo
```

## FIFO Files

Program 3 creates:

```text
A2B
B2A
```

The parent process sends messages through `A2B`.

The child process receives messages from `A2B`.

---

# Program 4

## Create/Edit Program 4

```bash
nano program4_fork_fifo.c
```

## Compile

```bash
clang program4_fork_fifo.c -o program4_fork_fifo
```

## Run

```bash
./program4_fork_fifo
```

## FIFO Files

Program 4 creates:

```text
A2B
B2A
```

The parent process sends messages through `B2A`.

The child process receives messages from `A2B`.

---

# Remove FIFO Files After Testing

```bash
rm -f A2B B2A
```

---

# Check All Files

```bash
ls -l
```

---

# Check FIFO Files

```bash
ls -l fifo1 fifo2
```

or:

```bash
ls -l A2B B2A
```

A FIFO is displayed with a `p` at the beginning of its permission information.

---

# Check Source Files

```bash
ls *.c
```

---

# Check Documentation Files

```bash
ls *.md
```

---

# Compile All Programs

```bash
clang program1_userA.c -o program1_userA
clang program2_userB.c -o program2_userB
clang program3_fork_fifo.c -o program3_fork_fifo
clang program4_fork_fifo.c -o program4_fork_fifo
```

---

# Check Compilation Results

```bash
ls
```

Expected files include:

```text
program1_userA
program1_userA.c
program2_userB
program2_userB.c
program3_fork_fifo
program3_fork_fifo.c
program4_fork_fifo
program4_fork_fifo.c
README.md
commands.md
```

---

# Git Status

From the repository root:

```bash
cd ~/Documents/ForkFusion_OSSP
git status
```

---

# Add Week 8

```bash
git add "2520030541-Practical/Week 8"
```

---

# Commit Week 8

```bash
git commit -m "Added Week 8 FIFO and fork IPC practical"
```

---

# Pull Before Push

```bash
git pull --rebase origin main
```

---

# Push to GitHub

```bash
git push origin main
```
