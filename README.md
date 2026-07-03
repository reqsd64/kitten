# kitten

`kitten` is a small tree-view utility for exploring a directory or one or more paths from the terminal.
It prints a readable directory tree and inlines text file contents so you can inspect a project without opening files one by one.

## What it does

- Shows directories as a tree.
- Prints regular files with their size.
- Displays text file contents inline.
- Detects binary files and avoids dumping unreadable bytes.
- Handles symlinks without recursing into them.

## Build

```sh
cc -std=c11 -Wall -Wextra -Wpedantic -O2 src/main.c -o kitten
```

## Usage

```sh
./kitten
./kitten path/to/project
./kitten file.txt other.txt
./kitten --help
```

### Behavior

- With no arguments, `kitten` inspects the current directory.
- With a single directory argument, it changes into that directory and prints the tree from there.
- With multiple arguments, each path is shown separately.
- `-h` and `--help` print usage information.

## Output format

- Directories end with `/`.
- Regular files are shown as `[file, N bytes]`.
- Text file contents are printed under a `contents` block.
- Empty files and binary files are labeled explicitly.

## Repository layout

- `kitten` - built binary
- `README.md` - documentation
- `src/main.c` - source code

## Badges

[![License](https://img.shields.io/badge/license-BSD%202--Clause-2b7fff?style=flat-square)](LICENSE)
[![C](https://img.shields.io/badge/C-C11-555555?style=flat-square&logo=c&logoColor=white)](https://en.cppreference.com/w/c/11)
