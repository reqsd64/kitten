#!/bin/sh

set -u

kitten=${1:-./kitten}
case $kitten in
/*) ;;
*) kitten=$(cd "$(dirname "$kitten")" && pwd)/$(basename "$kitten") ;;
esac

LC_ALL=C
export LC_ALL

test_root=${TMPDIR:-/tmp}/kitten-test.$$
umask 077
if ! mkdir "$test_root"; then
	echo "cannot create test directory: $test_root" >&2
	exit 1
fi

clean_test_root()
{
	trap - 0 HUP INT TERM
	rm -rf "$test_root"
}

trap clean_test_root 0
trap 'exit 1' HUP INT TERM

mkdir -p "$test_root/tree/dir/deep" "$test_root/tree/empty-dir" \
	"$test_root/tree/skip" "$test_root/counts/dir"
printf 'first line\nsecond line\n' >"$test_root/tree/text.txt"
printf 'safe\033[31m\n' >"$test_root/tree/control.txt"
printf '\000binary\n' >"$test_root/tree/binary.bin"
: >"$test_root/tree/empty.txt"
printf 'hidden\n' >"$test_root/tree/skip/hidden.txt"
printf 'nested\n' >"$test_root/tree/dir/nested.txt"
printf 'counted\n' >"$test_root/counts/file.txt"
printf 'option path\n' >"$test_root/tree/--help"
ln -s text.txt "$test_root/tree/link"
ln -s missing "$test_root/tree/broken-link"
ln -s file.txt "$test_root/counts/link"
mkfifo "$test_root/tree/pipe" "$test_root/counts/pipe"

passed=0
failed=0

expect_output()
{
	name=$1
	expected_status=$2
	expected_text=$3
	shift 3

	output=$("$@" 2>&1)
	status=$?
	if [ "$status" -eq "$expected_status" ] &&
	    case $output in *"$expected_text"*) true ;; *) false ;; esac
	then
		passed=$((passed + 1))
	else
		echo "not ok - $name" >&2
		echo "  status: $status (expected $expected_status)" >&2
		echo "  missing: $expected_text" >&2
		printf '%s\n' "$output" >&2
		failed=$((failed + 1))
	fi
}

expect_no_output()
{
	name=$1
	expected_status=$2
	unexpected_text=$3
	shift 3

	output=$("$@" 2>&1)
	status=$?
	if [ "$status" -eq "$expected_status" ] &&
	    case $output in *"$unexpected_text"*) false ;; *) true ;; esac
	then
		passed=$((passed + 1))
	else
		echo "not ok - $name" >&2
		echo "  status: $status (expected $expected_status)" >&2
		echo "  unexpected: $unexpected_text" >&2
		printf '%s\n' "$output" >&2
		failed=$((failed + 1))
	fi
}

expect_output "help" 0 "Usage:" "$kitten" --help
expect_output "version" 0 "kitten 0.1.0" "$kitten" --version
expect_output "unknown option" 1 "unknown option" "$kitten" --unknown
expect_output "invalid preview limit" 1 "invalid preview limit" \
	"$kitten" --max-bytes=1T
expect_output "invalid depth" 1 "invalid depth" "$kitten" --depth=-1
expect_output "Russian diagnostics" 1 "неизвестный параметр" \
	"$kitten" --unknown --language=ru

expect_output "text preview" 0 "second line" \
	"$kitten" "$test_root/tree/text.txt"
expect_no_output "disabled preview" 0 "contents" \
	"$kitten" --no-content "$test_root/tree/text.txt"
expect_output "preview limit" 0 "exceeds 4 bytes limit" \
	"$kitten" --max-bytes=4 "$test_root/tree/text.txt"
expect_output "binary preview" 0 "[binary file]" \
	"$kitten" "$test_root/tree/binary.bin"
expect_output "empty preview" 0 "[empty file]" \
	"$kitten" "$test_root/tree/empty.txt"
expect_output "escaped preview" 0 "safe\\x1b[31m" \
	"$kitten" "$test_root/tree/control.txt"

expect_output "symbolic link" 0 "link -> text.txt [symlink]" \
	"$kitten" --no-content "$test_root/tree/link"
expect_output "broken symbolic link" 0 "broken-link -> missing [symlink]" \
	"$kitten" --no-content "$test_root/tree/broken-link"
expect_output "special file" 0 "[special file, 0 bytes]" \
	"$kitten" --no-content "$test_root/tree/pipe"
expect_no_output "depth limit" 0 "nested.txt" \
	"$kitten" --depth=1 --no-content "$test_root/tree"
expect_no_output "excluded directory" 0 "hidden.txt" \
	"$kitten" --exclude=skip --no-content "$test_root/tree"
expect_no_output "directories only" 0 "text.txt" \
	"$kitten" --dirs-only --no-content "$test_root/tree"
expect_output "unsorted traversal" 0 "text.txt" \
	"$kitten" --unsorted --depth=1 --no-content "$test_root/tree"

expect_output "summary directories" 0 "directories: 2" \
	"$kitten" --no-content --summary "$test_root/counts"
expect_output "summary files" 0 "files: 1" \
	"$kitten" --no-content --summary "$test_root/counts"
expect_output "summary symlinks" 0 "symlinks: 1" \
	"$kitten" --no-content --summary "$test_root/counts"
expect_output "summary special files" 0 "special: 1" \
	"$kitten" --no-content --summary "$test_root/counts"
expect_output "multiple roots" 0 "files: 2" \
	"$kitten" "$test_root/tree/text.txt" --no-content \
	"$test_root/tree/empty.txt" --summary
expect_output "missing path" 1 "not found" \
	"$kitten" "$test_root/missing"

output=$(cd "$test_root/tree" && "$kitten" --no-content -- --help 2>&1)
status=$?
if [ "$status" -eq 0 ] &&
    case $output in *"--help [file"*) true ;; *) false ;; esac
then
	passed=$((passed + 1))
else
	echo "not ok - option terminator" >&2
	printf '%s\n' "$output" >&2
	failed=$((failed + 1))
fi

if [ "$failed" -ne 0 ]; then
	echo "$failed of $((passed + failed)) tests failed" >&2
	exit 1
fi

echo "$passed tests passed"
