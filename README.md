<div align="center">

# kitten

**A small C utility that prints source trees with file metadata and inline text previews.**

[![C99](https://img.shields.io/badge/C-C99-555555?style=flat-square&logo=c&logoColor=white)](https://en.cppreference.com/w/c/99)
[![POSIX.1-2008](https://img.shields.io/badge/API-POSIX.1--2008-6f665c?style=flat-square&logo=linux&logoColor=white)](https://pubs.opengroup.org/onlinepubs/9699919799/)
[![Dependencies](https://img.shields.io/badge/dependencies-none-61966f?style=flat-square)](#requirements)
[![License](https://img.shields.io/badge/license-BSD--2--Clause-d9a86c?style=flat-square)](LICENSE)

<br>

<img src="assets/preview.png" alt="kitten showing a compact source tree in a terminal" width="100%">

<br>

[`Quick start`](#quick-start) · [`Install`](#install) · [`Options`](#options) · [`Behavior`](#behavior) · [`Packaging`](#packaging)

</div>

## Why kitten?

Sometimes `tree` does not show enough and opening every file shows far too much. `kitten` sits between the two: it prints the structure, file sizes and readable text contents in one terminal-friendly snapshot.

| Tool | Tree structure | File metadata | Inline contents |
|:--|:--:|:--:|:--:|
| `tree` | ✓ | limited | — |
| `cat` | — | — | ✓ |
| **`kitten`** | ✓ | ✓ | ✓ |

It is useful for:

- inspecting small source trees;
- attaching readable context to bug reports;
- preparing code-review notes;
- sharing a project snapshot without opening every file;
- feeding a bounded amount of repository context into another tool.

> [!NOTE]
> `kitten` is intentionally conservative. It does not follow symlinks, skips binary and oversized content, and reports read failures through its exit status.

## Requirements

kitten targets C99 and POSIX.1-2008. It has no third-party runtime dependencies.
Building requires a C compiler and `make`; building the optional Info manual also
requires Texinfo's `makeinfo`.

The normal build is:

```sh
make
```

To compile the source files directly:

```sh
cc -std=c99 -Wall -Wextra -Wpedantic -O2 \
  src/main.c src/walk.c src/output.c -o kitten
```

It uses POSIX interfaces including `readdir`, `fdopendir`, `getline`, `fnmatch`,
`lstat`, `readlink` and non-blocking `open`.

## Install

Install under `/usr/local` by default:

```sh
make
make install
```

Packagers can select a prefix and stage the result without writing to the live
filesystem:

```sh
make PREFIX=/usr
make PREFIX=/usr DESTDIR="$pkgdir" install
```

`make install` installs the executable and man page. The Info manual is optional:

```sh
make info
make PREFIX=/usr DESTDIR="$pkgdir" install-info
```

Compiler and linker settings can be overridden in the usual way:

```sh
make CC=clang CFLAGS='-O2 -pipe' LDFLAGS='-Wl,--as-needed'
```

## Quick start

```sh
# Inspect the current directory, including text contents
./kitten

# Inspect another project
./kitten path/to/project

# Print only the tree and metadata
./kitten --no-content src

# Skip generated files and print totals
./kitten --exclude=.git --exclude='*.o' --summary .

# Use Russian diagnostics and human-readable sizes
./kitten --language=ru --human-readable src
```

With one directory argument, `kitten` enters that directory before printing the tree. Multiple paths are printed as separate roots.

## Examples

### A compact source tree

```console
$ ./kitten --no-content src
./
├── include/
│   └── kitten.h [file, 3276 bytes]
├── main.c [file, 43108 bytes]
└── parser.c [file, 18842 bytes]
```

### Limit depth and content size

```sh
./kitten -L 2 -m 64K path/to/project
```

`-L 2` descends at most two directory levels. `-m 64K` caps each text preview at 64 KiB.

### Select what should be shown

```sh
./kitten --exclude=.git --exclude='build/*' --summary .
./kitten --content=never README.md src
./kitten --dirs-only .
./kitten --ascii --no-content .
```

Exclude patterns use shell-style wildcards and are matched against both names and paths. Repeat `--exclude` for more than one pattern.

## Options

| Option | Description |
|:--|:--|
| `-m BYTES`, `--max-bytes=BYTES` | Change the per-file preview limit. Accepts bytes or `K`, `M`, `G` suffixes. |
| `-L DEPTH`, `--depth=DEPTH` | Descend at most `DEPTH` directory levels. |
| `--exclude=PATTERN` | Skip matching names or paths. May be used more than once. |
| `--no-content` | Print only the tree and file metadata. |
| `--content=auto\|never` | Enable the default preview behavior or disable contents. |
| `--raw-content` | Print file contents without escaping terminal controls. |
| `--dirs-only` | Print and descend into real directories only. |
| `--ascii` | Replace Unicode tree lines with portable ASCII characters. |
| `--unicode` | Force Unicode tree lines, regardless of the locale. |
| `-H`, `--human-readable` | Print sizes in IEC units such as KiB and MiB. |
| `-U`, `--unsorted` | Keep directory memory bounded by using filesystem order. |
| `--language=auto\|en\|ru` | Select the message language; the default is `auto`. |
| `--summary` | Print directory, file, symlink, byte, skipped and error totals. |
| `--version` | Print version and license information, then exit. |
| `-h`, `--help` | Print usage and exit. |

Both `--option=value` and the separated `--option value` form are accepted where a value is required.
Options may appear before or after path operands. Use `--` before a path that
begins with a dash.

## Behavior

### Text previews

- Text contents are shown by default.
- The default per-file limit is **256 KiB**.
- Files containing a NUL byte anywhere within the preview limit are marked as binary.
- Empty files, unreadable files and truncated previews receive explicit labels.
- `--no-content` and `--content=never` keep output to the tree and metadata.
- Control bytes are escaped before file contents are written to the terminal.
- `--raw-content` disables escaping for contents only; names and link targets
  remain escaped.

### Traversal

- Entries are sorted according to the active locale.
- `--unsorted` uses filesystem order and retains only one pending entry, which
  is useful for directories too large to sort comfortably in memory.
- `.` and `..` are ignored.
- Symlink targets are printed, but symlinked directories are never followed.
- Long symlink targets are read without a fixed `PATH_MAX` assumption.
- Empty directories and special files are identified in the tree.
- `--dirs-only` hides regular files, links and special entries.
- `-L 0` prints only the root; `-L 1` includes its immediate children.

### Summary and exit status

`--summary` prints totals after traversal:

```text
summary:
  directories: 3
  files: 7
  symlinks: 0
  special: 0
  bytes: 68421
  skipped: 1
  errors: 0
```

| Status | Meaning |
|:--:|:--|
| `0` | Every requested path was inspected successfully. |
| `1` | Invalid arguments, an unreadable path or at least one traversal/read error. |

Skipped binary or oversized content is counted in the summary but is not treated as an error.

### Localization

kitten selects messages from the active system locale. Russian locales use
Russian help, labels and diagnostics; every other locale falls back to English.
The normal POSIX precedence of `LC_ALL`, `LC_MESSAGES` and `LANG` applies.

Use `--language=en` or `--language=ru` for reproducible output in scripts and
bug reports. `--language=auto` restores locale-based selection. Option names
and values are never translated.

Tree drawing also follows the locale: UTF-8 locales use Unicode, while other
locales use ASCII. `--ascii` and `--unicode` override that choice; when both
are present, the last one wins.

### Output safety

Directory entry names and symlink targets can legally contain control bytes.
kitten prints those bytes as escapes so a crafted filename cannot add fake tree
lines or send an escape sequence to the terminal. Backslashes in names are
escaped as well.

By default, text previews preserve tabs and valid locale-encoded characters,
but escape ASCII controls, multibyte control characters and invalid encoded
bytes. `--raw-content` is intended for trusted files: their contents can then
write terminal control sequences. Filename and symlink-target escaping is
never disabled.

## Portability

The supported baseline is C99 plus POSIX.1-2008. The implementation avoids
third-party libraries, fixed `PATH_MAX` buffers and GNU-only command-line
parsing. It is intended to build on current GNU/Linux and BSD systems with the
platform C compiler.

Native Windows APIs are not supported. Windows users need a POSIX environment
such as Cygwin or an equivalent compatibility layer. Regular files and
directories are opened non-blocking and without following a final symlink,
then checked with `fstat`. The opened device and inode must match the preceding
`lstat` result. This prevents a file replaced by a FIFO from hanging the process
and rejects replacement by another regular file. Filesystem races are reported
when they become observable, but kitten is an inspection utility and does not
provide a transactional snapshot of a changing tree.

## Packaging

Release packages should build with the distribution compiler flags and stage
installation through `DESTDIR`:

```sh
make PREFIX=/usr
make PREFIX=/usr DESTDIR="$pkgdir" install
```

For Arch-style packages, use the SPDX license identifier `BSD-2-Clause`; a
minimal `package()` function can call the staged install command above. An
upstream `PKGBUILD` is intentionally not included before a stable source archive
and checksum exist. Arch's packaging guide likewise recommends staged
installation through `make DESTDIR="$pkgdir" install` in its
[package creation guide](https://wiki.archlinux.org/title/Creating_packages).

For GNU evaluation, `doc/kitten.texi` supplies the preferred Texinfo source.
[GNU's evaluation guidance](https://www.gnu.org/help/evaluation) normally
recommends GPLv3-or-later for accepted packages, while kitten currently remains
BSD-2-Clause. Treat any future license change as a separate copyright-holder
decision.

Packagers may omit the Info manual and install only the executable and man page,
or run `make info` followed by `make install-info` when Texinfo is available.

## Repository layout

```text
.
├── assets/
│   └── preview.png
├── doc/
│   ├── kitten.1
│   └── kitten.texi
├── docs/
│   └── plans/
├── src/
│   ├── kitten.h
│   ├── main.c
│   └── walk.c
├── Makefile
├── LICENSE
└── README.md
```

## License

`kitten` is available under the [BSD 2-Clause License](LICENSE).
