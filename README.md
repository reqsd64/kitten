# kitten

`kitten` prints a compact, readable snapshot of files and directories.
It is meant for inspecting small source trees, bug reports, reviews, and
terminal notes where a plain `tree` listing is not enough but opening every
file would be noise.

## What it prints

- directories, sorted by name;
- regular files with their size;
- text files under a `contents` block;
- simple labels for empty, binary, unreadable, and oversized files;
- symlink targets, without following the link.

`kitten` is conservative by default: it does not follow symlinked directories,
does not dump large files unless the limit is raised, and exits non-zero when
something could not be read.

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
./kitten -L 2 --no-content src
./kitten --exclude=.git --exclude='*.o' --summary src
./kitten --ascii --summary src
./kitten --help
```

## Options

```text
Usage: kitten [OPTION]... [PATH]...

  -m, --max-bytes=BYTES   change the per-file content preview limit
  -L, --depth=DEPTH       descend at most DEPTH directory levels
      --exclude=PATTERN   skip names or paths matching PATTERN
      --no-content        print only the tree and file metadata
      --content=WHEN      show content previews: auto or never
      --ascii             use ASCII tree drawing characters
      --summary           print counts after the tree
  -h, --help              display help and exit
```

`BYTES` accepts plain bytes and `K`, `M`, or `G` suffixes. `PATTERN` uses shell
wildcards and is matched against both the entry name and its path.

### Behavior

- No arguments: inspect the current directory.
- One real directory: enter it first, then print the tree from `.`.
- Several paths: print each one as its own root.
- Symlinks stay symlinks, including links that point at directories.
- `-L 0` prints only the root. `-L 1` includes its immediate children.
- `--no-content` and `--content=never` keep the output to tree metadata.
- `--ascii` avoids Unicode box-drawing characters for plain terminals.
- `-h` and `--help` print usage information.

## Output format

- Directories end with `/`.
- Regular files are shown as `[file, N bytes]`.
- Text file previews are capped at 256 KiB unless changed with `-m`.
- Read errors are shown in the tree, and the process exits non-zero if anything could not be read.
- `--summary` reports directories, files, symlinks, special files, byte totals,
  skipped content, and read errors after traversal.

## Exit status

- `0`: every requested path was inspected successfully.
- `1`: an option was invalid, a requested path could not be read, or traversal
  hit at least one read error.

## Repository layout

- `README.md` - documentation
- `src/main.c` - source code
- `kitten.tar.xz` - packaged build artifact

## Badges

[![License](https://img.shields.io/badge/license-BSD%202--Clause-2b7fff?style=flat-square)](LICENSE)
[![C](https://img.shields.io/badge/C-C11-555555?style=flat-square&logo=c&logoColor=white)](https://en.cppreference.com/w/c/11)
