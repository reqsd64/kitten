<div align="center">

# kitten v0.1.0

**Readable source-tree snapshots in one small C utility.**

<br>

<img src="https://raw.githubusercontent.com/reqsd64/kitten/v0.1.0/assets/license-preview.png" alt="kitten displaying LICENSE metadata and contents in a terminal" width="60%">

</div>

This is the first public release of `kitten`, a small C99 utility that combines
a directory tree, file metadata, and inline text previews in one
terminal-friendly view. It is intended for inspecting small source trees,
preparing review context, and sharing bounded repository snapshots.

## Highlights

- previews readable regular files up to 256 KiB by default;
- identifies directories, regular files, symbolic links, and special files;
- never follows symbolic links and verifies opened entries against inspected
  metadata;
- escapes terminal control data unless raw content is explicitly requested;
- supports locale-aware sorted traversal and memory-bounded filesystem-order
  traversal;
- provides depth limits, repeatable exclusions, directory-only output,
  human-readable sizes, and aggregate summaries;
- includes English and Russian diagnostics.

## Download

| File | Description |
|:--|:--|
| [`kitten-0.1.0-linux-x86_64.tar.gz`](https://github.com/reqsd64/kitten/releases/download/v0.1.0/kitten-0.1.0-linux-x86_64.tar.gz) | Prebuilt release for Linux x86_64 |
| [`kitten-0.1.0-linux-x86_64.tar.gz.sha256`](https://github.com/reqsd64/kitten/releases/download/v0.1.0/kitten-0.1.0-linux-x86_64.tar.gz.sha256) | SHA-256 checksum |

```text
0625c76c95ee3bd4617c77c235f679b281dc0de637d770d5f30e8b6c49349d68  kitten-0.1.0-linux-x86_64.tar.gz
```

Verify and unpack the archive:

```sh
sha256sum -c kitten-0.1.0-linux-x86_64.tar.gz.sha256
tar -xzf kitten-0.1.0-linux-x86_64.tar.gz
./kitten-0.1.0-linux-x86_64/kitten --version
```

## Quick start

Inspect the current directory:

```sh
./kitten
```

Common variants:

```sh
./kitten --no-content src
./kitten -L 2 -m 64K path/to/project
./kitten --exclude=.git --exclude='*.o' --summary .
```

Build and install from source:

```sh
make
make install
```

## Platform notes

- C99 and POSIX.1-2008;
- no third-party runtime dependencies;
- POSIX-like operating systems;
- native Windows APIs are not supported.

See the [complete documentation](https://github.com/reqsd64/kitten/blob/v0.1.0/docs/DOCS.md)
for all options, traversal and preview rules, portability notes, and packaging
instructions.

`kitten` is distributed under the
[BSD 2-Clause License](https://github.com/reqsd64/kitten/blob/v0.1.0/LICENSE).
