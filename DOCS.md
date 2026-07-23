# kitten documentation

`kitten` prints filesystem trees with entry metadata and optional inline
previews of regular text files. This document describes version 0.1.0 as
implemented in `src/main.c`, `src/walk.c`, and `src/output.c`.

## Requirements

The runtime has no third-party dependencies. The supported baseline is:

- a C99 implementation;
- POSIX.1-2008 filesystem and locale interfaces;
- a POSIX-like operating system.

Building requires a C compiler and `make`. Building the optional Info manual
also requires Texinfo's `makeinfo`.

## Building

The normal build creates `./kitten`:

```sh
make
```

The Makefile respects the usual build variables:

```sh
make CC=clang CFLAGS='-O2 -pipe' LDFLAGS='-Wl,--as-needed'
```

`WARNINGS` defaults to `-Wall -Wextra -Wpedantic`, and the Makefile always
selects `-std=c99`. A direct build equivalent to the default source selection
is:

```sh
cc -std=c99 -O2 -Wall -Wextra -Wpedantic \
  src/main.c src/walk.c src/output.c -o kitten
```

Remove generated files with:

```sh
make clean
```

Make targets are:

| Target | Result |
|:--|:--|
| `all` | Build `kitten`; this is the default target. |
| `kitten` | Compile the three C sources when a source or `kitten.h` changed. |
| `info` | Build `doc/kitten.info` from `doc/kitten.texi`. |
| `install` | Build and install the executable and man page. |
| `install-info` | Build and install the Info manual. |
| `uninstall` | Remove all three installed files. |
| `clean` | Remove `kitten` and generated `doc/kitten.info`. |

`INSTALL`, `MAKEINFO`, and `RM` can be overridden for packaging environments.
The executable is compiled in one compiler invocation; there are no
intermediate object-file targets.

## Installation

The default prefix is `/usr/local`. `make install` installs the executable and
the manual page:

```sh
make
make install
```

Use `PREFIX` to select another hierarchy and `DESTDIR` to stage a package:

```sh
make PREFIX=/usr
make PREFIX=/usr DESTDIR="$pkgdir" install
```

The Info manual is optional:

```sh
make info
make PREFIX=/usr DESTDIR="$pkgdir" install-info
```

`make uninstall` removes the executable, man page, and Info manual from the
selected `PREFIX` and `DESTDIR`. It does not remove their parent directories.

## Command line

```text
kitten [OPTION]... [PATH]...
```

With no path, `kitten` inspects the current directory. Options may appear
before or after path operands. `--` ends option parsing, so it is required
before a path beginning with `-`. A lone `-` is treated as a path.

When exactly one directory is supplied, `kitten` securely enters that
directory and prints it as `./`. With multiple operands, every path is printed
as a separate root without changing the working directory.

Options that set the same mode are applied from left to right. Consequently,
the last `--ascii` or `--unicode` wins, and `--content=auto` can re-enable
previews after `--no-content`.

## Options

| Option | Default | Effect |
|:--|:--|:--|
| `-m BYTES`, `--max-bytes=BYTES` | `256K` | Set the maximum preview size per regular file. |
| `-L DEPTH`, `--depth=DEPTH` | unlimited | Limit descent to the non-negative depth. |
| `--exclude=PATTERN` | none | Skip matching directory entries; repeatable. |
| `--no-content` | off | Disable regular-file previews. |
| `--content=auto\|never` | `auto` | Enable the default preview policy or disable previews. |
| `--raw-content` | off | Do not escape control data in preview text. |
| `--dirs-only` | off | Show and descend into real directories only. |
| `-H`, `--human-readable` | off | Format sizes using KiB, MiB, GiB, or TiB. |
| `-U`, `--unsorted` | off | Use filesystem order and bounded directory memory. |
| `--ascii` | locale-dependent | Draw trees with ASCII characters. |
| `--unicode` | locale-dependent | Draw trees with Unicode box characters. |
| `--summary` | off | Print aggregate counters after traversal. |
| `--language=auto\|en\|ru` | `auto` | Select diagnostic and label language. |
| `--version` | — | Print version and license information, then exit. |
| `-h`, `--help` | — | Print help, then exit. |

Options requiring a value accept either a separate operand or the long
`--name=value` form. Short options cannot be grouped.

`BYTES` is an unsigned decimal number, optionally followed by one `K`, `M`, or
`G` suffix. Suffixes are case-insensitive and use powers of 1024. Values that
do not fit in `size_t` are rejected.

`DEPTH` is an unsigned decimal integer no larger than `LONG_MAX`:

- `-L 0` prints only the root;
- `-L 1` includes immediate children but does not descend into them;
- omitting the option allows unlimited descent.

### Defaults and parser rules

The complete initial option state is:

| Setting | Initial value |
|:--|:--|
| preview limit | 262144 bytes |
| maximum depth | unlimited (`-1` internally) |
| content previews | enabled |
| raw preview output | disabled |
| directories only | disabled |
| human-readable sizes | disabled |
| unsorted traversal | disabled |
| exclusions | empty list |
| message language | selected from the locale |
| tree style | Unicode only when the locale codeset is UTF-8 |

The parser recognizes only the spellings shown in the option table:

- short options cannot be combined (`-HU` is invalid) or joined to their
  values (`-m64K` is invalid);
- long options with values accept `--name value` and `--name=value`;
- numbers must begin with an ASCII digit—leading signs, whitespace, fractions,
  and trailing text are rejected;
- a size accepts at most one suffix character, so `64KiB`, `1KB`, and `1.5M`
  are invalid;
- depth does not accept size suffixes;
- an empty exclusion pattern is invalid;
- scalar modes follow command-line order, while every valid `--exclude`
  appends another pattern.

Unknown options and missing or invalid values produce one diagnostic on
standard error and status `1`. `--help` and `--version` print to standard
output and exit as soon as the main parser reaches them; later operands are
not processed.

Parser diagnostics use the prefix `kitten:` and one of six reasons:
`invalid preview limit`, `invalid depth`, `invalid exclude pattern`,
`invalid content mode`, `invalid language`, or `unknown option`. The offending
value is escaped and appended when one is available. Missing values have no
appended argument.

Path operands are compacted in `argv` during parsing. This is why options can
follow paths without changing their order. After `--`, every remaining
argument is a path, including strings that look like options.

## Examples

Preview the current directory:

```sh
kitten
```

Print only tree structure and metadata:

```sh
kitten --no-content src
```

Inspect at most two levels and preview files no larger than 64 KiB:

```sh
kitten -L 2 -m 64K path/to/project
```

Exclude common repository noise and print totals:

```sh
kitten --exclude=.git --exclude='*.o' --exclude='build/*' --summary .
```

Use deterministic English labels and portable tree characters:

```sh
kitten --language=en --ascii --no-content .
```

Print several roots:

```sh
kitten README.md src LICENSE
```

Inspect a path beginning with a dash:

```sh
kitten -- -example
```

## Tree and metadata output

Each root is printed first. Child entries use either Unicode branches
(`├──`, `└──`, `│`) or ASCII branches (`|--`, `` `-- ``, `|`).

Entry labels are based on `lstat`:

```text
name/                         directory
name [file, 123 bytes]        regular file
name -> target [symlink]      symbolic link
name [special file, 0 bytes]  any other file type
```

Directories have a trailing slash. Symlink targets are displayed, but never
followed. Special files include FIFOs, sockets, devices, and other
non-directory, non-regular, non-symlink entries.

File sizes come from the metadata observed before previewing. Without
`--human-readable`, English output uses `byte` or `bytes`, while Russian output
uses the appropriate inflection. Human-readable output uses IEC units, prints
an integer for exact multiples, and otherwise prints one decimal place.

Multiple roots are separated by one blank line. A root does not receive a tree
branch. A child does, and its preview block continues under the branch stem:

```text
root/
└── file.txt [file, 6 bytes]
    ┌── contents
    │ hello
    └──
```

ASCII mode uses `+--` for the preview header, `|` for content lines, and
`` `-- `` for the closing line.

### Output streams

Normal tree lines, inline traversal errors, help, version information, and the
summary go to standard output. Argument errors and failure to enter a sole
directory operand go to standard error.

An operating error discovered after a tree entry has been selected stays
inside the tree so the failing location remains visible. The main status
still becomes `1`.

### Inline status labels

The English labels produced inside a tree are:

| Situation | Output |
|:--|:--|
| directory has no visible children | `[empty directory]` |
| directory cannot be opened or read | `[cannot read directory: reason]` |
| directory was replaced before opening | `[directory changed while opening]` |
| path metadata cannot be read | `[not found: reason]` |
| allocation needed for an entry fails | `[out of memory]` |
| regular file is empty | `[empty file]` |
| regular file contains a NUL byte | `[binary file]` |
| current size is above the preview limit | `[skipped: SIZE exceeds LIMIT limit]` |
| file grows above the limit during scanning | `[skipped: file grew beyond LIMIT limit]` |
| second pass reaches the limit before EOF | `[truncated after LIMIT]` |
| regular file cannot be opened | `[cannot open: reason]` |
| regular path was replaced before opening | `[file changed while opening]` |
| preview read or close fails | `[read error]` |

If `readlink` fails, the symlink line prints the system error text where the
target would normally appear, still adds `[symlink]`, increments the symlink
counter, and records an error.

## Traversal

### Ordering

The default mode reads all visible names in a directory, sorts them with
`strcoll` under the active locale, and then visits them. This gives stable,
locale-aware presentation but stores the directory's names in memory.

`--unsorted` keeps filesystem order. It retains only one pending child at each
open directory level so that the final branch can still be drawn correctly.
This bounds directory-entry memory for very large directories; recursive path
and preview allocations still apply.

### Exclusions

`--exclude` uses POSIX `fnmatch` patterns. Every pattern is matched against:

1. the entry's base name;
2. its full traversal path with `/` treated as a path separator;
3. the same path without a leading `./`, when present.

For example:

```sh
kitten --exclude=.git --exclude='*.o' --exclude='build/*' .
```

An excluded directory is neither printed nor entered. Patterns apply to
directory entries, not to root operands explicitly supplied on the command
line. Matching follows `fnmatch`, not `.gitignore` syntax: there is no negation,
repository discovery, or implicit recursive `**` behavior added by `kitten`.

Every filtered entry increments the summary's `skipped` counter.

### Directory-only mode

`--dirs-only` displays only entries for which `lstat` reports a real directory.
Regular files, special files, and symlinks—including symlinks whose targets
are directories—are hidden and counted as skipped.

If metadata cannot be read while deciding visibility, the entry remains
visible so the normal traversal path can report the error instead of silently
discarding it.

### Depth

The root has depth zero. A depth limit controls descent, not whether a
directory at the limit is displayed. Thus `-L 1` prints immediate child
directories but does not list their contents.

### Symlinks

`kitten` uses `lstat` and never follows symlinked directories. Link targets are
read into a dynamically grown buffer rather than a fixed `PATH_MAX` buffer.
Failure to read a target is reported and makes the final exit status non-zero.

### Visibility and boundary cases

- `.` and `..` returned by `readdir` are ignored and do not increment
  `skipped`.
- A directory whose children are all excluded or hidden by `--dirs-only` is
  displayed as empty; the hidden children still increment `skipped`.
- Reaching the depth limit is not a skip. The directory is printed without an
  empty marker, and its children are not examined.
- An explicitly supplied root is not tested against exclusion patterns.
- A non-directory root under `--dirs-only` produces no tree line, increments
  `skipped`, and succeeds unless another error occurs.
- Special files are described from `lstat` metadata and are never opened.
- A metadata failure during the `--dirs-only` visibility check is deliberately
  passed to normal entry processing so it becomes a visible error.

The sorted walker finishes reading and closes a directory before descending
into collected children. If that read ended with an error, already collected
entries are still printed before the directory error marker. The unsorted
walker descends while its parent stream remains open and reports a later
`readdir` or `closedir` failure after the pending child.

## File previews

Only regular files are candidates for previews. Previews are enabled by
default and disabled by `--no-content` or `--content=never`.

The preview sequence is:

1. open the path without following its final symlink;
2. confirm with `fstat` that it is still the same regular file found by
   `lstat`;
3. skip it immediately if its current metadata size exceeds the limit;
4. scan up to the limit for NUL bytes and read errors;
5. rewind and print the file line by line.

A file containing a NUL byte during the scan is labeled binary. A file larger
than the limit is labeled skipped. Neither condition is an error. Empty files
receive an explicit label.

The initial scan and printing pass make file changes observable in several
ways. A replacement, growth beyond the limit, read failure, or overlong second
pass is reported with a specific status where possible. This is still an
inspection of a live filesystem, not a transactional snapshot.

Preview lines are prefixed with tree guides. Trailing CR and LF bytes are
removed from each displayed line, and `kitten` emits its own newline.
`--raw-content` disables character escaping, but it does not turn the preview
into a byte-for-byte `cat`: tree prefixes and line processing remain.

The largest single preview allocation is bounded by the preview limit and the
longest line being displayed. The file is not loaded into one contiguous
buffer.

### Preview decision table

Checks are ordered; an earlier decision prevents later classification. In
particular, a file already larger than the limit is not scanned for NUL bytes.

| Observed condition | Preview result | `skipped` | `errors` |
|:--|:--|:--:|:--:|
| open fails | `cannot open` | — | +1 |
| opened type/device/inode differs | `file changed while opening` | — | +1 |
| opened size exceeds the limit | size-limit marker | +1 | — |
| scan finds NUL | `binary file` | +1 | — |
| scan reads beyond the remaining limit | growth marker | +1 | — |
| scan or rewind fails | `read error` | — | +1 |
| scan reaches EOF with zero bytes | `empty file` | — | — |
| second pass reaches limit before EOF | truncation marker | +1 | — |
| second-pass read or final close fails | `read error` | — | +1 |

The size on the file's metadata line comes from the initial `lstat`. The size
used for the preview-limit decision comes from the verified descriptor's
`fstat`. They can differ when a file changes in place.

A zero preview limit is valid. An empty file is reported empty; any data
observed during the scan or second pass makes the file grown or truncated
relative to that zero-byte limit.

The scan uses 4096-byte reads and rewinds with `fseek`. The printing pass starts
the line buffer at 256 bytes or the remaining limit, whichever is smaller, and
grows it up to that limit. CR and LF removal affects display only; the byte
budget includes those bytes.

## Output safety

Filesystem names may contain tabs, newlines, escape bytes, invalid multibyte
sequences, or other data that can alter terminal output. `kitten` therefore
escapes entry names and symlink targets unconditionally:

- `\n`, `\r`, and `\t` use named escapes;
- backslashes in names and targets are doubled;
- ASCII controls and invalid bytes use `\xNN`;
- multibyte control characters use `\uNNNN` or `\UNNNNNNNN`.

Default preview output preserves tabs and printable characters recognized by
the active locale. It escapes ASCII controls, raw C1 bytes, invalid encoded
bytes, and multibyte control characters. Backslashes in preview text are left
unchanged.

`--raw-content` writes preview text without these checks. A trusted file can
then emit terminal escape sequences. The option never disables escaping for
names or symlink targets.

Diagnostics may include text from the system's `strerror`; their wording and
encoding depend on the host C library and locale.

The default byte handling is:

| Input | Names and link targets | Preview text |
|:--|:--|:--|
| tab | `\t` | literal tab |
| LF or CR | `\n` or `\r` | removed as a line ending; otherwise escaped |
| backslash | doubled | unchanged |
| ASCII byte below 32 or byte 127 | `\xNN` | `\xNN`, except preview tab |
| standalone byte `0x80`–`0x9f` | `\xNN` | `\xNN` |
| invalid or incomplete multibyte sequence | offending byte as `\xNN` | same |
| valid multibyte control character | `\uNNNN` or `\UNNNNNNNN` | same |
| valid printable character | copied | copied |

`mbrtowc` and `iswcntrl` use the active locale. The conversion state is reset
after an invalid or incomplete sequence and after ASCII controls handled
without `mbrtowc`; valid conversions preserve the state.

## Filesystem race handling

Traversal deliberately separates presentation metadata from safe opening:

- `lstat` identifies the entry without following a symlink;
- regular files and directories are opened with `O_NONBLOCK` and `O_NOFOLLOW`;
- the resulting descriptor is checked with `fstat`;
- type, device, and inode must match the earlier observation.

Non-blocking open prevents a regular path replaced by a FIFO from hanging
before its type can be checked. A descriptor for a confirmed regular file is
returned to blocking mode before stdio reads begin.

The same device-and-inode check protects the one-directory convenience mode
before `fchdir`. Observable replacements are reported as changes. These checks
reduce common time-of-check/time-of-use hazards, but a file can still change
in place after it has been opened.

## Summary

`--summary` prints cumulative totals after every requested root:

```text
summary:
  directories: 3
  files: 7
  symlinks: 1
  special: 0
  bytes: 68421
  skipped: 2
  errors: 0
```

The counters mean:

| Counter | Meaning |
|:--|:--|
| `directories` | Displayed real directories, including directory roots. |
| `files` | Displayed regular files. |
| `symlinks` | Displayed symbolic links. |
| `special` | Displayed entries of all other file types. |
| `bytes` | Sum of observed regular-file metadata sizes, whether or not contents were previewed. |
| `skipped` | Entries filtered by options plus previews skipped as binary, oversized, grown, or truncated. |
| `errors` | Metadata, open, read, allocation, and traversal failures observed during the walk. |

The byte total saturates at `UINTMAX_MAX`; if that happens, the summary marks
the value as overflowed.

Because `skipped` combines filtered entries and content decisions, it is not a
count of files alone.

Counter updates follow the displayed entry, not successful content reading:

| Event | Counter effect |
|:--|:--|
| real directory is displayed | `directories +1` |
| regular file is displayed | `files +1`, add its initial metadata size to `bytes` |
| symlink is displayed | `symlinks +1`, even if reading its target fails |
| special file is displayed | `special +1`; its size is not added to `bytes` |
| entry is excluded or hidden by `--dirs-only` | `skipped +1` |
| preview is binary, oversized, grown, or truncated | `skipped +1` |
| depth limit prevents descent | no counter change beyond the displayed directory |
| empty file or empty visible directory | no skip and no error |
| an operation fails | `errors +1` at the reporting point |

A regular file remains counted, and its metadata size remains in `bytes`, when
content is disabled, skipped, or fails to open. A path whose `lstat` fails has
no type counter because its type was never established.

## Exit status

| Status | Meaning |
|:--:|:--|
| `0` | Arguments were valid and every requested traversal completed without an observed error. |
| `1` | Invalid arguments, path/traversal/preview failures, or output stream errors occurred. |

Binary and oversized previews are skips, not failures. Errors from any root
make the final status `1`, while traversal continues where possible. Failure
to flush standard output or standard error also changes the final status to
`1`.

## Localization and tree selection

At startup, `setlocale(LC_ALL, "")` activates the process environment. In
automatic mode, an `ru` locale selects Russian labels and diagnostics; all
other locales select English. Standard locale precedence (`LC_ALL`,
`LC_MESSAGES`, then `LANG`) is handled by the C library.

`--language=en` and `--language=ru` provide reproducible messages.
`--language=auto` returns to locale detection. Option names and accepted option
values are always English and are never translated.

The tree style is chosen separately from the message language. A locale whose
codeset is reported as UTF-8 or UTF8 uses Unicode branches; other codesets use
ASCII. `--ascii` and `--unicode` override this choice.

Before normal argument parsing, `main.c` scans for a valid language option.
This allows a language option later on the command line to control diagnostics
for an earlier invalid option.

Russian locale detection is deliberately narrow: the locale name must begin
with `ru` ignoring case, followed by the end of the string or `_`, `-`, or `.`.
Tree detection compares the codeset with `UTF-8` or `UTF8`, also ignoring
case. If locale activation fails, the C library's current locale remains in
effect.

The preliminary language scan stops at `--` and skips the following argument
of every known value-taking option. This prevents a path or option value equal
to `--language` from being misread. An invalid language value does not change
the preliminary selection; the main pass later reports it as an error.

Localized labels come from the program. Error reasons appended from `strerror`
come from the host and may use a different language if the system does not
provide matching message catalogs.

## Architecture

The program is intentionally split into four source units with no third-party
library layer.

| File | Responsibility |
|:--|:--|
| `src/main.c` | Locale setup, option parsing, root selection, secure entry into a single directory, summary dispatch, cleanup, and final status. |
| `src/walk.c` | Filtering, sorted and unsorted traversal, metadata output, symlink handling, safe opens, preview classification, and error accumulation. |
| `src/output.c` | Tree glyphs, localized size formatting, escaping, preview text output, and summary rendering. |
| `src/kitten.h` | Version/default constants, shared state structures, and the small cross-file function interface. |

### Function map

`src/main.c` owns process-level policy:

| Function | Role |
|:--|:--|
| `locale_is_russian` | Recognize an `ru` message locale. |
| `locale_is_utf8` | Recognize UTF-8 from `nl_langinfo(CODESET)`. |
| `set_language` | Apply `auto`, `en`, or `ru`. |
| `set_depth_limit` | Parse and range-check a decimal depth. |
| `set_preview_limit` | Parse a byte count and optional binary suffix. |
| `set_content_mode` | Apply `auto` or `never`. |
| `add_exclude` | Copy and append one owned pattern. |
| `print_usage` | Emit complete English or Russian help. |
| `print_version` | Emit version, copyright, license, and warranty text. |
| `select_argument_language` | Preliminary language scan for diagnostics. |
| `argument_error_text` | Map parser error identifiers to localized text. |
| `free_options` | Free every exclusion and the pattern array. |
| `finish_program` | Flush both output streams, adjust status, and clean options. |
| `argument_error` | Print one escaped parser diagnostic and finish with status 1. |
| `enter_directory` | Verify, open, and `fchdir` into a sole directory operand. |
| `main` | Initialize, parse, select roots, run traversal/summary, and finish. |

`src/walk.c` owns filesystem state and tree traversal:

| Function | Role |
|:--|:--|
| `add_file_size` | Add a positive regular-file size with saturation. |
| `is_excluded` | Match all exclusion patterns against name and path forms. |
| `join_path` | Allocate `parent/name` with overflow checks. |
| `append_prefix` | Allocate the next tree indentation prefix. |
| `print_tree_status` | Print one localized marker inside the tree. |
| `open_preview` | Safely open and verify a regular file. |
| `scan_preview` | Classify text, binary, growth, or read failure. |
| `read_preview_line` | Read one bounded line and detect truncation. |
| `print_file_preview` | Render the preview box and update skip/error totals. |
| `show_dirent` | Reject only `.` and `..`. |
| `open_directory` | Safely open and verify a directory, returning a `DIR`. |
| `report_directory_error` | Render a localized directory marker and count it. |
| `entry_is_visible` | Apply exclusions and `--dirs-only`. |
| `compare_names` | Adapt `strcoll` for `qsort`. |
| `print_sorted_directory` | Collect, sort, close, and traverse visible names. |
| `print_unsorted_directory` | Traverse `readdir` order with one pending child. |
| `print_directory` | Dispatch to the selected directory algorithm. |
| `read_symlink_target` | Read a link target with dynamic growth. |
| `print_entry` | Classify and render one root or child, then recurse if needed. |
| `kitten_walk_path` | Public entry point for one root. |

`src/output.c` owns formatting:

| Function | Role |
|:--|:--|
| `kitten_tree_branch` | Select the last/non-last ASCII or Unicode branch. |
| `kitten_tree_stem` | Select the indentation stem for descendants. |
| `byte_word` | Choose English or Russian byte inflection. |
| `kitten_format_size` | Format exact bytes or IEC units. |
| `print_escaped_bytes` | Decode and escape bounded name or preview bytes. |
| `kitten_print_escaped` | Escape a NUL-terminated name or target. |
| `kitten_print_preview_text` | Select escaped or raw preview writing. |
| `kitten_print_summary` | Render localized totals and overflow state. |

The public cross-file interface is limited to declarations in `kitten.h`.
Everything else is `static`, so traversal and formatting details do not become
an external API.

### Shared state

`struct kitten_options` contains the fully parsed modes and owned exclusion
patterns. `struct kitten_totals` accumulates counters and byte-overflow state.
`struct kitten_context` passes pointers to both through recursive traversal.
There is no mutable global program state.

`kitten_options` fields map directly to command-line state:

| Field | Meaning |
|:--|:--|
| `preview_limit` | Per-file byte limit. |
| `max_depth` | Non-negative limit or `-1` for unlimited. |
| `show_content` | Whether regular files enter the preview path. |
| `raw_content` | Whether preview escaping is bypassed. |
| `dirs_only` | Whether non-directories are filtered. |
| `unicode_tree` | Unicode rather than ASCII tree glyphs. |
| `human_readable` | IEC rather than exact-byte size formatting. |
| `unsorted` | Filesystem-order rather than collected/sorted traversal. |
| `russian` | Russian rather than English program strings. |
| `exclude_patterns` | Owned array of copied `fnmatch` patterns. |
| `exclude_count` | Number of elements in that array. |

`kitten_totals` contains `directories`, `files`, `symlinks`, `special`,
`skipped`, `errors`, and `bytes`, all as `uintmax_t`, plus the
`bytes_overflow` flag. `kitten_context` only binds constant options to mutable
totals.

The preview state machines use three internal enums: `preview_open`
distinguishes success, system open failure, and identity change;
`preview_scan` distinguishes text, binary, growth, and read failure; and
`preview_line` distinguishes EOF, a complete line, truncation, and error.

### Execution flow

1. `main` initializes defaults and the locale.
2. A preliminary pass selects the language for argument diagnostics.
3. The main parser updates options and compacts path operands in `argv`.
4. A single directory operand is verified, opened, and entered; all other
   roots are passed directly to `kitten_walk_path`.
5. `print_entry` classifies each path with `lstat`.
6. Directories select the sorted or unsorted walker; regular files may invoke
   the preview pipeline; symlinks and special files end at metadata output.
7. Failures increment totals and propagate through return values without
   aborting unrelated roots.
8. `main` optionally prints the summary, flushes both output streams, frees
   exclusion patterns, and returns status 0 or 1.

### Directory algorithms

The sorted walker stores copies of every visible name, sorts the pointer array,
and then constructs child paths for traversal. Its directory-level memory is
linear in the number and total length of visible names.

The unsorted walker constructs child paths as entries arrive and holds one
pending path so it knows whether that entry is last. Its directory-level
pending-entry memory is constant apart from path length. It may keep one
directory stream open per recursive level. The sorted walker closes its stream
before descending. Both use heap-allocated prefixes and paths rather than
fixed-size buffers.

### Error model

Functions return a boolean-like non-zero result when an error occurred and
also increment `kitten_totals.errors` at the point where the failure becomes
visible. This lets traversal print as much useful context as possible while
preserving a failing process status.

Memory exhaustion is handled locally. Where possible, the affected tree line
gets an out-of-memory marker and remaining siblings continue. Failure to build
state essential to a directory read ends that part of the traversal.

### Ownership and invariants

- `main` owns exclusion strings from successful parsing until
  `finish_program`.
- The sorted walker owns its copied names until every collected child has been
  attempted, including the case where `readdir` ended with an error.
- Each joined path and tree prefix is freed by the stack frame that allocated
  it.
- `fdopendir` takes ownership of a verified directory descriptor; `closedir`
  releases it.
- `fdopen` takes ownership of a verified regular-file descriptor; `fclose`
  releases it.
- The descriptor used by `enter_directory` is closed after `fchdir`; the new
  working directory remains active independently of that descriptor.
- The preview line buffer and symlink target buffer are freed on every
  completed path through their owners.

Successful safe-open functions preserve one central invariant: the descriptor
has the expected type and the same `(st_dev, st_ino)` pair as the earlier
`lstat`. A mismatch is reported as a change, not as a generic system error.

`errno` is cleared around `readdir` calls so end-of-directory can be
distinguished from failure. Close failures are retained when no earlier read
failure already explains the same stream.

## Portability and limitations

- Native Windows APIs are not supported. Use a POSIX compatibility
  environment.
- Sorting and printable-character decisions depend on the active locale.
- Binary detection is deliberately simple: a NUL byte means binary. There is
  no MIME detection or encoding conversion.
- Exclusion patterns are POSIX shell patterns, not VCS ignore rules.
- Directory recursion is not given an independent stack-depth guard; use
  `-L` for untrusted or exceptionally deep trees.
- Metadata and contents can describe different moments when files are changing
  concurrently.
- Hard-linked regular files are counted and previewed at every visible path.
- Output is designed for people and text capture; there is no JSON or
  machine-stable schema.
- Only exit statuses 0 and 1 are used.

The implementation avoids fixed `PATH_MAX` buffers and GNU-only command-line
parsing. Interfaces such as `readdir`, `fdopendir`, `fnmatch`, `lstat`,
`readlink`, `fchdir`, and non-blocking `open` are part of the portability
baseline.

## Packaging

Distribution packages should pass their compiler policy through `CC`,
`CPPFLAGS`, `CFLAGS`, `LDFLAGS`, and `LDLIBS`, then stage installation:

```sh
make PREFIX=/usr
make PREFIX=/usr DESTDIR="$pkgdir" install
```

The runtime package needs only the executable and man page. The Info manual may
be built and packaged separately when Texinfo is available:

```sh
make info
make PREFIX=/usr DESTDIR="$pkgdir" install-info
```

Use the SPDX identifier `BSD-2-Clause`. A package should not change the
project's license classification based on recommendations from a target
distribution or upstream collection.

## Repository layout

```text
.
├── assets/
│   ├── license-preview.png
│   └── preview.png
├── doc/
│   ├── kitten.1
│   └── kitten.texi
├── src/
│   ├── kitten.h
│   ├── main.c
│   ├── output.c
│   └── walk.c
├── DOCS.md
├── DOCS.ru.md
├── LICENSE
├── Makefile
├── README.md
├── README.ru.md
├── SUPPORT.md
└── SUPPORT.ru.md
```

When changing user-visible behavior, keep the English and Russian help text,
both Markdown manuals, the man page, and the Texinfo source aligned. Update
`KITTEN_VERSION` in `src/kitten.h` when preparing a new release.

## License

`kitten` is distributed under the [BSD 2-Clause License](LICENSE).
