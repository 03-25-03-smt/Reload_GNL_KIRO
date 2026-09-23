*This project has been created as part of the 42 curriculum by vladyslb.*

# get_next_line

## Description

`get_next_line` reads a file descriptor **one line at a time**. Each call returns
the next line, including its trailing newline character, and `NULL` once there is
nothing left to read or an error occurs.

```c
char *get_next_line(int fd);
```

The C standard library has no such function: `read` gives you an arbitrary chunk
of bytes with no regard for line boundaries. The goal of the project is to build
that missing abstraction from three primitives only — `read`, `malloc` and
`free` — and in doing so learn the two ideas the subject is really about:

- **static variables**, because data read past the end of one line must survive
  until the next call;
- **manual memory management**, because every returned line is heap-allocated
  and every intermediate buffer has to be released on both the success and the
  failure path.

The buffer size is not hardcoded. It is injected at compile time via
`-D BUFFER_SIZE=n`, and the implementation must behave identically for
`BUFFER_SIZE=1` and `BUFFER_SIZE=10000000`.

The **bonus** part extends this to any number of descriptors read in parallel:
interleaved calls on different file descriptors must not disturb one another,
while still using a single static variable.

### Feature list

| | Mandatory | Bonus |
|---|---|---|
| Reads one line per call, `\n` included | yes | yes |
| Works for any `BUFFER_SIZE` | yes | yes |
| Handles a final line without `\n` | yes | yes |
| Handles empty files and files of only newlines | yes | yes |
| Returns `NULL` on invalid fd or read error | yes | yes |
| Works on `stdin` and pipes | yes | yes |
| Multiple file descriptors in parallel | no | yes |
| Static variables used | 1 | 1 |

## Instructions

### Requirements

A C compiler (`cc` / `gcc` / `clang`) and a POSIX system. There is no
`Makefile`: the subject asks for source files only, and the evaluator compiles
them directly together with their own `main`.

### Files

| File | Part | Contents |
|---|---|---|
| `get_next_line.h` | mandatory | prototypes, default `BUFFER_SIZE` 42 |
| `get_next_line.c` | mandatory | `get_next_line` and its static helpers |
| `get_next_line_utils.c` | mandatory | `ft_strchr`, `ft_strlen`, `ft_strdup`, `ft_strlcat`, `ft_strjoin` |
| `get_next_line_bonus.h` | bonus | same plus `<sys/select.h>` for `FD_SETSIZE`, default `BUFFER_SIZE` 1024 |
| `get_next_line_bonus.c` | bonus | per-descriptor state |
| `get_next_line_utils_bonus.c` | bonus | the same utilities |

### Compilation

```bash
# mandatory
cc -Wall -Wextra -Werror get_next_line.c get_next_line_utils.c main.c

# bonus
cc -Wall -Wextra -Werror get_next_line_bonus.c get_next_line_utils_bonus.c main.c
```

Override the buffer size with `-D BUFFER_SIZE=n`:

```bash
cc -Wall -Wextra -Werror -D BUFFER_SIZE=1 \
   get_next_line.c get_next_line_utils.c main.c
```

Two rules matter here:

- **Never compile the mandatory and bonus parts together.** Both define the
  symbol `get_next_line`, so the link step fails with a duplicate symbol.
- **Each `.c` must include its own header.** If `get_next_line_utils_bonus.c`
  includes `get_next_line.h`, it still builds locally because both headers sit
  in the same directory — but during evaluation, where the bonus is compiled
  from bonus files only, it dies with
  `fatal error: get_next_line.h: No such file or directory`.

### Integration

Copy the three files of the part you need into your project, include the
matching header, and link them in:

```c
#include "get_next_line.h"
#include <fcntl.h>
#include <stdio.h>

int	main(void)
{
	int		fd;
	char	*line;

	fd = open("file.txt", O_RDONLY);
	while ((line = get_next_line(fd)))
	{
		printf("%s", line);
		free(line);   /* the caller owns the string */
	}
	close(fd);
	return (0);
}
```

## How to test

Everything below was run against this code; the outputs shown are the real ones.

### 1. A minimal test program

Create `main.c`:

```bash
touch main.c
vim main.c
```

```c
#include "get_next_line.h"
#include <fcntl.h>
#include <stdio.h>

int	main(void)
{
	int		fd;
	char	*line;

	fd = open("test.txt", O_RDONLY);
	if (fd == -1)
		return (1);
	while ((line = get_next_line(fd)) != NULL)
	{
		printf("%s", line);
		free(line);
	}
	close(fd);
	return (0);
}
```

Save and quit (`:wq`).

### 2. A test file

```bash
echo -e "Hello\nWorld\n42" > test.txt
cat test.txt
```

```
Hello
World
42
```

### 3. Compile and run

```bash
cc -Wall -Wextra -Werror get_next_line.c get_next_line_utils.c main.c
./a.out
```

```
Hello
World
42
```

The output must match `cat test.txt` exactly. Because `printf("%s", line)` adds
nothing of its own, any missing or duplicated newline is a bug in
`get_next_line`, not in the test.

### 4. Vary BUFFER_SIZE

This is the single most important test. `BUFFER_SIZE` changes how many `read`
calls are needed and where the chunk boundaries land, but the output must never
change.

```bash
cc -Wall -Wextra -Werror -D BUFFER_SIZE=1 \
   get_next_line.c get_next_line_utils.c main.c
./a.out
```

```
Hello
World
42
```

```bash
cc -Wall -Wextra -Werror -D BUFFER_SIZE=1000 \
   get_next_line.c get_next_line_utils.c main.c
./a.out
```

```
Hello
World
42
```

```bash
cc -Wall -Wextra -Werror -D BUFFER_SIZE=10000000 \
   get_next_line.c get_next_line_utils.c main.c
./a.out
```

```
Hello
World
42
```

`BUFFER_SIZE=1` reads one byte per call, so a newline can only ever be the last
byte of a chunk. `BUFFER_SIZE=10000000` swallows the whole file in a single
`read`, so every line has to be sliced out of the stash with no further reading
at all. Identical output in both cases means the buffer boundary logic is sound.

### 5. Reading from stdin

`stdin` is file descriptor `0`, and it is not seekable — a good check that the
implementation never relies on rewinding.

Create `main_stdin.c`:

```c
#include "get_next_line.h"
#include <stdio.h>
#include <stdlib.h>

int	main(void)
{
	char	*line;

	while ((line = get_next_line(0)) != NULL)
	{
		printf("READ: %s", line);
		free(line);
	}
	return (0);
}
```

```bash
cc -Wall -Wextra -Werror get_next_line.c get_next_line_utils.c main_stdin.c
./a.out
```

Type lines and press `Ctrl-D` to signal EOF:

```
one
READ: one
two
READ: two
```

It also works with a pipe or a redirect, which is easier to script:

```bash
printf 'abc\ndef\n' | ./a.out
```

```
READ: abc
READ: def
```

```bash
./a.out < test.txt
```

```
READ: Hello
READ: World
READ: 42
```

### 6. Invalid file descriptor

`get_next_line` must return `NULL` instead of crashing when the descriptor was
never opened.

Create `main_error.c`:

```c
#include "get_next_line.h"
#include <stdio.h>
#include <stdlib.h>

int	main(void)
{
	char	*line;

	line = get_next_line(42);
	if (line == NULL)
		printf("NULL: error handled correctly\n");
	else
	{
		printf("ERROR: expected NULL\n");
		free(line);
	}
	return (0);
}
```

```bash
cc -Wall -Wextra -Werror get_next_line.c get_next_line_utils.c main_error.c
./a.out
```

```
NULL: error handled correctly
```

Descriptor `42` is not open, so `read` fails with `EBADF` and returns `-1`.
`fill_stash` recognises the negative return, discards the stash and reports
failure, and `get_next_line` returns `NULL`. Replacing `42` with `-1` gives the
same result.

### 7. Edge-case files

Worth running through the `main.c` from step 1 with these `test.txt` variants:

```bash
printf ''                > test.txt   # empty file          -> no output
printf '\n\n\n'          > test.txt   # newlines only       -> three blank lines
printf 'no newline here' > test.txt   # no trailing newline -> one line, no '\n'
printf 'single\n'        > test.txt   # exactly one line
```

The third case is the one people get wrong most often: the final line must still
be returned even though it does not end in `\n`.

Also check a directory descriptor — `read` on a directory fails, so the result
must be `NULL`:

```c
int fd = open("/tmp", O_RDONLY);   /* get_next_line(fd) -> NULL */
```

### 8. Memory

Every returned line is heap-allocated and must be freed by the caller. Run any
of the programs above under a leak checker:

```bash
valgrind --leak-check=full ./a.out
# or
cc -fsanitize=address -g get_next_line.c get_next_line_utils.c main.c && ./a.out
```

Reading a file all the way to `NULL` releases the internal stash too, so a
correct run reports no reachable blocks. If you stop halfway, one allocation
stays alive by design — see *Known trade-offs*.

### 9. Testing the bonus

Create `main_bonus.c` and read two files alternately:

```c
#include "get_next_line_bonus.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

int	main(void)
{
	int		fd1;
	int		fd2;
	char	*line;

	fd1 = open("f1.txt", O_RDONLY);
	fd2 = open("f2.txt", O_RDONLY);
	if (fd1 == -1 || fd2 == -1)
		return (1);
	while (1)
	{
		line = get_next_line(fd1);
		if (line)
		{
			printf("fd1: %s", line);
			free(line);
		}
		line = get_next_line(fd2);
		if (line)
		{
			printf("fd2: %s", line);
			free(line);
		}
		else if (!line)
			break ;
	}
	close(fd1);
	close(fd2);
	return (0);
}
```

```bash
printf 'a1\na2\na3\n' > f1.txt
printf 'b1\nb2\n'     > f2.txt
cc -Wall -Wextra -Werror \
   get_next_line_bonus.c get_next_line_utils_bonus.c main_bonus.c
./a.out
```

```
fd1: a1
fd2: b1
fd1: a2
fd2: b2
fd1: a3
```

Each descriptor keeps its own position: `fd1` is still on `a2` while `fd2` has
moved to `b2`. If the two streams ever bleed into each other, the per-descriptor
state is broken.

## Algorithm

### The problem

`read` knows nothing about lines. A single call returns up to `BUFFER_SIZE`
bytes, and a newline can land anywhere — or nowhere:

```
file:          Hello\nWorld\n42\n
BUFFER_SIZE=4: |Hell|o\nWo|rld\n|42\n|
```

The first chunk contains no newline at all. The second contains one, plus two
bytes that belong to the *next* line and must not be thrown away. So the
algorithm needs two things: a way to keep reading until a newline shows up, and
a place to park the leftover bytes between calls.

### The chosen design: a persistent stash

A single static `char *stash` holds every byte that has been read but not yet
handed to the caller. One call proceeds in four steps:

```
  1. ensure stash exists                 stash = ft_strdup("")

  2. read until stash holds '\n'
     ┌───────────────────────────────┐
     │ while (no '\n' in stash)      │
     │     read BUFFER_SIZE bytes    │   fill_stash()
     │     stash = stash + chunk     │
     │     read returned <= 0 -> stop│
     └───────────────────────────────┘

  3. cut the line out                    fill_line()
     stash = "Hello\nWorld\n"  ->  line = "Hello\n"

  4. keep the remainder                  stash_cut()
     stash = "Hello\nWorld\n"  ->  stash = "World\n"

  return line
```

On the next call step 2 finds a newline already sitting in the stash and
performs **no `read` at all** — the line is served straight from memory. That is
what makes the amortised cost one `read` per `BUFFER_SIZE` bytes of file rather
than one per line.

The four helpers map one-to-one onto these steps:

| Function | Role |
|---|---|
| `fill_stash` | one `read`, appends the chunk to the stash; on a read error frees the stash and sets it to `NULL` |
| `fill_line` | allocates and returns a copy of the stash up to and including the first `'\n'` |
| `stash_cut` | allocates and returns the remainder of the stash after that `'\n'` |
| `free_stash` | frees the stash, sets it to `NULL`, returns `NULL` so callers can `return free_stash(&stash);` |

### Why this design

**Why a static variable and not a global or a parameter?** The signature is
fixed by the subject — there is no out-parameter and no context struct, so the
leftover bytes have nowhere else to live. `static` gives exactly the lifetime
needed (whole program) with the narrowest possible scope (one function), so
nothing outside `get_next_line` can touch or corrupt the read position.

**Why store a leftover string instead of a buffer plus an index?** An index into
a fixed `char buf[BUFFER_SIZE]` is the other classic approach and it is faster —
no reallocation at all. But it also forces the buffer, a read position and a
valid-bytes count to stay consistent with each other across calls, and every
off-by-one in that triple becomes a silent data-corruption bug. Keeping a single
NUL-terminated string means there is exactly one invariant to maintain: *the
stash holds every byte read but not yet returned.* Ownership is obvious,
`ft_strchr` alone answers "do I have a full line?", and the failure paths are
short enough to audit by eye. For a project whose real subject is memory
discipline, that trade of speed for verifiability is the right one.

**Why loop on `read` instead of reading once?** A single `read` cannot be
trusted to deliver a whole line: with `BUFFER_SIZE=1` it never will, and on
pipes and terminals it may return less than requested even when more data is
coming. Looping until a newline appears, or until `read` reports `0` or `-1`,
is the only behaviour that is correct for every `BUFFER_SIZE` and every kind of
descriptor.

**Why does `fill_stash` clear the stash on `read` error but not on EOF?** The
two are genuinely different. `read` returning `0` means EOF: whatever is in the
stash is still valid data and must be returned as a final line without a
trailing `\n`. `read` returning `-1` means the descriptor is broken and the
stash can no longer be trusted, so it is dropped and `NULL` is reported. This
distinction is why `bytes < 0` gets its own branch instead of being folded into
`bytes <= 0`.

**Why does `free_stash` return `char *`?** It always returns `NULL`, which lets
every failure path collapse into one line:

```c
if (!line)
	return (free_stash(&stash));
```

Without it each of those sites would need three statements, and the function
would exceed the Norm's 25-line limit. This is the small trick that keeps
`get_next_line` within the Norm without splitting it up artificially.

### Complexity

Let *n* be the length of a line and *B* be `BUFFER_SIZE`.

- **`read` calls:** one per *B* bytes of file, amortised — optimal, since every
  byte must be read once.
- **Memory:** O(*n* + *B*) live at any moment. Never the whole file.
- **Time:** the stash is rebuilt on every `read`, so a line of length *n* costs
  O(*n*²/*B*) byte copies.

The quadratic term is the price of the string-based stash and is discussed
under *Known trade-offs*.

### The bonus

Exactly the same algorithm; only the storage changes. `stash` becomes an array
indexed by descriptor:

```c
static char	*stash[FD_SETSIZE];
```

Every reference to `stash` becomes `stash[fd]`, so each descriptor carries an
independent read position and interleaved calls cannot interfere. `FD_SETSIZE`
(from `<sys/select.h>`, 1024 on Linux) is the conventional upper bound on
descriptor numbers, and `fd` is range-checked against it before use. This is
still **one** static variable, as the subject requires — an array is a single
object.

## Contract

| Input | Result |
|---|---|
| line ending in `\n` | the line including its `\n` |
| last line with no `\n` | the line without `\n` |
| EOF / empty file | `NULL` |
| file of only `\n` | `"\n"` on every call |
| `fd < 0` | `NULL` |
| `BUFFER_SIZE <= 0` | `NULL` |
| `fd >= FD_SETSIZE` (bonus) | `NULL` |
| directory descriptor | `NULL` |
| `read` error | `NULL`, stash discarded |
| `malloc` failure | `NULL`, stash discarded |

The caller frees the returned string. Reading a file all the way to `NULL`
releases the internal state on its own.

## Known trade-offs

**A file that is not read to the end leaves the stash allocated.** There is no
public way to reset the state — the signature mandated by the subject has no
room for one — so this is a design consequence rather than a leak. Reading to
`NULL` releases it.

**Growing the stash is quadratic.** Every `read` builds a fresh stash of length
`len(stash) + BUFFER_SIZE`, so a long line with a small `BUFFER_SIZE` costs
O(n²) in byte copies. Measured at `BUFFER_SIZE=1`:

| line length | time |
|---|---|
| 10,000 | 0.20 s |
| 20,000 | 0.69 s |
| 40,000 | 3.04 s |

Doubling the length quadruples the time. On the same 1 MB file with
200,000-character lines, `BUFFER_SIZE=32` takes 10.7 s while
`BUFFER_SIZE=4096` takes 0.09 s. Normal evaluation is unaffected, but a tester
combining very long lines with `BUFFER_SIZE=1` can hit a timeout. The fix would
be to track the stash length separately and `read` into the tail of an
over-allocated buffer, removing the per-chunk copy.

**After `close(fd)` the function first returns whatever is already in the
stash,** and only then `NULL`. Those bytes were read before the descriptor was
closed and are still valid; no further `read` is attempted.

## Verification

```
norminette *.c *.h
get_next_line.c: OK!              get_next_line_bonus.c: OK!
get_next_line_utils.c: OK!        get_next_line_utils_bonus.c: OK!
get_next_line.h: OK!              get_next_line_bonus.h: OK!
```

Both parts compile with `-Wall -Wextra -Werror` without a single warning.

Covered: `BUFFER_SIZE` = 1, 2, 3, 5, 42, 1000, 1024, 4096, 10000000, 0, −1 ·
empty file · file with no trailing `\n` · file of only `\n` · single line ·
200,000-character lines · `fd` = −1, 42, `FD_SETSIZE` · directory descriptor ·
`stdin` through a pipe, a redirect and a terminal · two and three interleaved
descriptors (bonus).

Across every run that reaches EOF, `malloc` and `free` calls are balanced.

## Resources

### Documentation

- `man 2 read`, `man 3 malloc`, `man 3 free` — the three allowed functions.
  Worth reading `read(2)` closely: the distinction between a `0` and a `-1`
  return is the whole of the error handling in this project.
- `man 2 open`, `man 2 close`, `man 7 fd` — descriptors and their lifetime.
- *The C Programming Language*, Kernighan & Ritchie, chapter 7 (Input and
  Output) and chapter 8.2, which develops a `read`-based line reader.
- *Computer Systems: A Programmer's Perspective*, Bryant & O'Hallaron,
  chapter 10 — why `read` may return fewer bytes than requested, and why short
  reads make the loop mandatory rather than defensive.
- [42 Norm](https://github.com/42School/norminette) — the formatting rules
  enforced by `norminette`, including the 25-line and 5-function limits that
  shape how this code is split up.
- `getline(3)` — the POSIX function this project reimplements. Reading its
  interface is a good way to see why the fixed `get_next_line` signature forces
  a static variable.
- [Static variables in C](https://en.cppreference.com/w/c/language/storage_duration)
  — storage duration and linkage, the mechanism the whole project rests on.

### Use of AI

An AI assistant (Kiro CLI) was used on this project. It did **not** design the
algorithm or write the implementation: the four-helper structure, the persistent
stash and all of the logic in the `.c` files are my own work. What the assistant
was used for, specifically:

- **Norm and build auditing.** It ran `norminette` and
  `cc -Wall -Wextra -Werror` across the six files, grouped the errors by type,
  and reported them. It found that the original mandatory part failed to compile
  under `-Werror` because of `-Wmisleading-indentation`: line 99 was indented
  with two spaces while the following line used a tab, which made the statement
  after the loop look like the loop's body. Diagnosing that specific warning was
  the assistant's most useful contribution — the code was correct, only the
  whitespace lied about it.
- **Formatting to the Norm.** It converted the files to tabs, wrapped lines to
  80 columns, added the 42 headers, removed in-function comments flagged as
  `WRONG_SCOPE_COMMENT`, and moved the loop's empty body onto its own line to
  clear `EXP_NEWLINE`. To confirm nothing had been changed beyond whitespace, it
  tokenised the old and new files and compared the token streams: all six came
  out identical.
- **Test harnesses.** It generated the edge-case fixtures (empty file, newlines
  only, missing trailing newline, 200,000-character lines), the invalid-fd and
  directory-descriptor cases, the interleaved multi-descriptor bonus test, and a
  `malloc`/`free` accounting shim built with `-Wl,--wrap` to check allocation
  balance where `valgrind` was unavailable. It also verified byte-for-byte output
  equality between the original and reformatted sources across every
  `BUFFER_SIZE` listed under *Verification*.
- **Performance measurement.** It produced the timing table in *Known
  trade-offs* by generating inputs of increasing line length and observing the
  ×4 growth per doubling that confirms the O(n²) behaviour.
- **This README.** Drafted with the assistant from the verified test results
  above; the algorithm discussion reflects my own design decisions and the
  reasoning I gave for them.

No AI-generated code was submitted. Every command and output reproduced in the
*How to test* section was executed and confirmed.
