# kitten

`kitten` is a tiny terminal viewer for small source trees.
It prints the directory shape first, then shows short text files in place so you can inspect a project without opening every file.

## What it prints

- directories, sorted by name;
- regular files with their size;
- text files under a `contents` block;
- simple labels for empty, binary, unreadable, and oversized files;
- symlink targets, without following the link.

## Build

```sh
cc -std=c11 -Wall -Wextra -Wpedantic -O2 src/main.c -o kitten
```

## Usage

```sh
./kitten
./kitten path/to/project
./kitten file.txt other.txt
./kitten -m 64K src
./kitten --help
```

### Behavior

- No arguments: inspect the current directory.
- One real directory: enter it first, then print the tree from `.`.
- Several paths: print each one as its own root.
- Symlinks stay symlinks, including links that point at directories.
- `-m` and `--max-bytes` change the per-file preview limit. Plain bytes, `K`, `M`, and `G` suffixes are accepted.
- `-h` and `--help` print usage information.

## Output format

- Directories end with `/`.
- Regular files are shown as `[file, N bytes]`.
- Text file previews are capped at 256 KiB unless changed with `-m`.
- Read errors are shown in the tree, and the process exits non-zero if anything could not be read.

## Repository layout

- `README.md` - documentation
- `src/main.c` - source code
- `kitten.tar.xz` - packaged build artifact

## Badges

[![License](https://img.shields.io/badge/license-BSD%202--Clause-2b7fff?style=flat-square)](LICENSE)
[![C](https://img.shields.io/badge/C-C11-555555?style=flat-square&logo=c&logoColor=white)](https://en.cppreference.com/w/c/11)
