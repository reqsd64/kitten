# High-priority hardening design

Date: 2026-07-18

## Goal

Close the four high-priority gaps identified in the repository review without
changing kitten's default traversal or preview behavior:

1. add repeatable regression checks and CI;
2. keep the second preview pass within its configured memory bound;
3. make the README, manuals, built-in help, and preview image describe the real
   program;
4. offer explicit controls for total preview volume and reusable exclusions.

The implementation keeps the current C99/POSIX.1-2008 baseline and adds no
runtime dependency.

## Non-goals

This change does not alter the summary counters, locale selection, collation,
SIGPIPE handling, root-path formatting, version management, or default exclude
set. It does not implement gitignore syntax or automatically hide `.git`,
`.env`, private keys, or other sensitive paths.

## Command-line interface

### `--max-total-bytes=BYTES`

`--max-total-bytes` limits source bytes admitted to text preview rendering over
the entire invocation, across every path operand. It accepts both joined and
separated forms and uses the same byte syntax as `--max-bytes`: an unsigned
integer with an optional `K`, `M`, or `G` suffix.

The option is disabled by default, preserving existing output. A value of zero
prints the tree and metadata but skips every otherwise eligible preview. The
per-file `--max-bytes` check remains independent and runs first. If the option
is repeated, the last value wins.

The budget counts source bytes, including line endings, consistently with the
existing per-file accounting. Binary files, files rejected by the per-file
limit, and files that cannot be opened consume no total budget. Once the budget
is exhausted, traversal continues and later regular files receive an explicit
localized marker. Budget exhaustion increments `skipped` but is not an error
and does not by itself change the exit status.

When a text file fits the per-file limit but not the remaining total budget,
kitten prints only complete lines that fit. It then prints a localized total
limit marker, consumes the remaining budget, and continues traversal without
previewing later files. If the first line does not fit, no partial line is
printed and the remaining budget is still considered exhausted.

### `--exclude-from=FILE`

`--exclude-from` reads `fnmatch` patterns from a text file during option
processing. The option accepts joined and separated forms, may be repeated, and
may be mixed with `--exclude`. Patterns from every source are appended to the
same existing pattern list.

Line endings are removed. Empty lines and lines whose first byte is `#` are
ignored; other leading and trailing whitespace remains part of the pattern.
Gitignore negation, directory-only rules, escaping rules, and implicit nesting
do not apply. A `FILE` value of `-` names a literal file called `-`; standard
input is not consumed. Embedded NUL bytes make the exclude file invalid.

Failure to open or read the file, an invalid embedded NUL, or memory exhaustion
produces a localized diagnostic on standard error and exits with status 1
before filesystem traversal starts. Memory failures must not be described as an
invalid pattern.

## Internal state and data flow

`struct kitten_options` gains the configured total limit and a flag indicating
whether it was supplied. `struct kitten_context` gains the mutable remaining
preview budget. The existing `kitten_totals.bytes` field remains the sum of
regular-file metadata sizes and is not reused for preview accounting.

`main.c` parses both new options, loads exclude files, initializes the context
budget, and extends localized help and argument diagnostics. The byte parser is
shared by the per-file and total limits so accepted values cannot drift.

`walk.c` keeps the existing first pass that checks the descriptor, per-file
size, growth, and NUL bytes. The display pass no longer calls unbounded
`getline`. It uses a line reader whose dynamic payload capacity cannot exceed
the effective remaining per-file/total allowance; a terminator and a fixed-size
I/O buffer are the only constant overhead. The reader preserves current
handling of empty lines, CR/LF endings, escaped content, raw content, and output
framing.

The no-follow open, device/inode verification, binary detection, and existing
read-error behavior remain unchanged. A file that changes during the display
pass may be truncated, but it cannot force allocation beyond the configured
allowance.

## Output and errors

New status text is provided in English and Russian for:

- total preview budget reached;
- invalid total byte limit;
- exclude file open/read failure;
- invalid exclude file contents;
- memory exhaustion while loading exclude patterns.

Expected filtering, including total-budget truncation, stays on standard output
inside the tree. Argument and exclude-file failures use standard error. Existing
late output-error handling remains intact.

## Tests and continuous integration

Add a dependency-free POSIX shell suite at `tests/test.sh` and a `make check`
target. Tests run under a fixed C locale and use temporary directory trees.
They cover:

- baseline help, invalid arguments, depth, exclusions, symlinks, binary files,
  and exit status;
- zero, exact, partial, and cross-file total preview budgets;
- a long line larger than the remaining total budget;
- total accounting across multiple path operands;
- repeated `--exclude-from`, comments, blank lines, CRLF, direct exclusions,
  missing files, read failures, and embedded NUL;
- unchanged normal output when neither new option is present.

Add a GitHub Actions workflow with GCC and Clang builds, warnings promoted to
errors, a sanitizer run, `make check`, man-page validation, and Texinfo
generation. CI-only build and documentation tools are not project dependencies.

## Documentation and preview asset

Update the built-in English and Russian help, README, `doc/kitten.1`, and
`doc/kitten.texi` together. Documentation must distinguish the per-file limit
from the optional invocation-wide limit, define exclude-file syntax, and warn
that previews can expose repository configuration, credentials, and other
sensitive data unless users exclude them.

Replace stale examples and the repository layout in README. Recreate
`assets/preview.png` in its existing restrained terminal style, but use factual
output: the current source filenames, the real multiline preview and summary
format, a correct root label, and C99/POSIX branding.

## Compatibility

With neither new option present, output and exit status remain compatible with
version 0.1.0. The only internal behavior change is replacing the second-pass
reader so a concurrently growing line cannot exceed the configured allocation
bound. Both new options are opt-in.

## Acceptance criteria

- Existing invocations produce the same output unless a corrected documentation
  example or asset is being compared.
- The display pass never gives its dynamic line payload more capacity than the
  effective configured preview allowance.
- `--max-total-bytes` applies once across all roots and never stops tree
  traversal.
- `--exclude-from` follows the documented line syntax and reports failures
  before traversal.
- English and Russian help, README, man, and Texinfo agree on every new option.
- The preview image depicts behavior and files that exist in this repository.
- `make check`, compiler matrix jobs, sanitizers, and documentation checks pass.
