<div align="center">

# kitten

**Readable source-tree snapshots for the terminal**

[Documentation](docs/DOCS.md) •
[Русский](.github/README.ru.md) •
[Security](.github/SECURITY.md) •
[License](LICENSE)

<br>

[![Version](https://img.shields.io/badge/version-0.1.0-d9a86c?style=flat-square)](docs/DOCS.md)
[![C99](https://img.shields.io/badge/C-C99-555555?style=flat-square&logo=c&logoColor=white)](https://en.cppreference.com/w/c/99)
[![POSIX.1-2008](https://img.shields.io/badge/API-POSIX.1--2008-6f665c?style=flat-square&logo=linux&logoColor=white)](https://pubs.opengroup.org/onlinepubs/9699919799/)
[![Dependencies](https://img.shields.io/badge/runtime_dependencies-none-61966f?style=flat-square)](docs/DOCS.md#requirements)
[![License](https://img.shields.io/badge/license-BSD--2--Clause-d9a86c?style=flat-square)](LICENSE)

<br>

<div align="center">
<img src="assets/assets-preview.jpg"
     alt="kitten displaying LICENSE metadata and contents in a terminal"
     width="40%">
</div>

</div>

`kitten` combines a directory tree, file metadata, and inline text previews in
one terminal-friendly view. It is useful for inspecting small source trees,
preparing review context, and sharing bounded repository snapshots.

**Navigate:** [Features](#features) • [Quick start](#quick-start) •
[Usage](#usage) • [Installation](#installation) •
[Documentation](#documentation)

## Features

- previews readable files up to 256 KiB by default;
- identifies directories, regular files, symlinks, and special files;
- avoids symlink traversal and detects files replaced during inspection;
- escapes terminal control data unless raw output is explicitly requested;
- supports sorted and memory-bounded filesystem-order traversal;
- provides English and Russian diagnostics.

## Quick start

Building requires a C99 compiler and `make`:

```sh
make
./kitten
```

With no path, `kitten` inspects the current directory.

## Usage

```text
kitten [OPTION]... [PATH]...
```

| Command | Result |
|:--|:--|
| `./kitten --no-content src` | Print the tree and metadata without file previews. |
| `./kitten -L 2 -m 64K path/to/project` | Limit traversal depth and preview size. |
| `./kitten --exclude=.git --exclude='*.o' --summary .` | Exclude matching entries and print totals. |
| `./kitten --language=ru .` | Use Russian diagnostics and labels. |

Run `./kitten --help` for the complete option summary.

## Installation

The default installation prefix is `/usr/local`:

```sh
make
make install
```

Set `PREFIX` to use another hierarchy or `DESTDIR` to stage a package:

```sh
make PREFIX=/usr DESTDIR="$pkgdir" install
```

## Documentation

- [Complete documentation](docs/DOCS.md)
- [Manual page source](doc/kitten.1)
- [Texinfo manual source](doc/kitten.texi)
- [Security policy](.github/SECURITY.md)

## License

`kitten` is distributed under the [BSD 2-Clause License](LICENSE).
