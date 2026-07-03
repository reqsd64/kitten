#define _POSIX_C_SOURCE 200809L

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int skip_dot(const struct dirent *entry)
{
	return strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0;
}

static char *join_path(const char *left, const char *right)
{
	size_t left_len = strlen(left);
	size_t right_len = strlen(right);
	int need_slash = left_len > 0 && left[left_len - 1] != '/';
	char *path = malloc(left_len + right_len + (need_slash ? 2 : 1));

	if (!path)
		return NULL;

	memcpy(path, left, left_len);
	if (need_slash)
		path[left_len++] = '/';
	memcpy(path + left_len, right, right_len);
	path[left_len + right_len] = '\0';
	return path;
}

static const char *base_name(const char *path)
{
	const char *slash = strrchr(path, '/');

	return slash ? slash + 1 : path;
}

static char *extend_prefix(const char *prefix, int is_last)
{
	static const char *tail_last = "    ";
	static const char *tail_mid = "│   ";
	const char *tail = is_last ? tail_last : tail_mid;
	size_t prefix_len = strlen(prefix);
	char *next = malloc(prefix_len + 5);

	if (!next)
		return NULL;

	memcpy(next, prefix, prefix_len);
	memcpy(next + prefix_len, tail, 4);
	next[prefix_len + 4] = '\0';
	return next;
}

static char *read_symlink_target(const char *path)
{
	size_t cap = 128;
	char *target = malloc(cap);

	if (!target)
		return NULL;

	for (;;) {
		ssize_t len = readlink(path, target, cap - 1);
		char *grown;

		if (len < 0) {
			free(target);
			return NULL;
		}

		if ((size_t)len < cap - 1) {
			target[len] = '\0';
			return target;
		}

		cap *= 2;
		grown = realloc(target, cap);
		if (!grown) {
			free(target);
			return NULL;
		}
		target = grown;
	}
}

static int file_looks_binary(FILE *fp)
{
	unsigned char buf[4096];
	size_t nread;
	size_t i;

	nread = fread(buf, 1, sizeof(buf), fp);
	if (nread == 0) {
		rewind(fp);
		return 0;
	}

	for (i = 0; i < nread; ++i) {
		if (buf[i] == '\0') {
			rewind(fp);
			return 1;
		}
	}

	rewind(fp);
	return 0;
}

static void print_file_contents(const char *path, const char *content_prefix)
{
	FILE *fp = fopen(path, "rb");
	char *line = NULL;
	size_t cap = 0;
	ssize_t nread;
	int saw_content = 0;

	printf("%s┌── contents\n", content_prefix);
	if (!fp) {
		printf("%s│ [cannot open: %s]\n", content_prefix, strerror(errno));
		printf("%s└──\n", content_prefix);
		return;
	}

	if (file_looks_binary(fp)) {
		printf("%s│ [binary file]\n", content_prefix);
		fclose(fp);
		printf("%s└──\n", content_prefix);
		return;
	}

	while ((nread = getline(&line, &cap, fp)) != -1) {
		while (nread > 0 && (line[nread - 1] == '\n' || line[nread - 1] == '\r'))
			line[--nread] = '\0';
		if (nread == 0)
			printf("%s│\n", content_prefix);
		else
			printf("%s│ %s\n", content_prefix, line);
		saw_content = 1;
	}

	if (ferror(fp))
		printf("%s│ [read error]\n", content_prefix);
	else if (!saw_content)
		printf("%s│ [empty file]\n", content_prefix);

	free(line);
	fclose(fp);
	printf("%s└──\n", content_prefix);
}

static void walk_path(const char *path, const char *prefix, int is_last, int is_root);

static void walk_dir(const char *path, const char *prefix)
{
	struct dirent **entries = NULL;
	int count = scandir(path, &entries, skip_dot, alphasort);
	int i;

	if (count < 0) {
		printf("%s└── [cannot read directory: %s]\n", prefix, strerror(errno));
		return;
	}

	if (count == 0) {
		printf("%s└── [empty directory]\n", prefix);
		free(entries);
		return;
	}

	for (i = 0; i < count; ++i) {
		char *child_path = join_path(path, entries[i]->d_name);

		if (!child_path) {
			printf("%s└── [out of memory]\n", prefix);
			free(entries[i]);
			continue;
		}

		walk_path(child_path, prefix, i == count - 1, 0);
		free(child_path);
		free(entries[i]);
	}

	free(entries);
}

static void walk_path(const char *path, const char *prefix, int is_last, int is_root)
{
	struct stat st;
	const char *name = is_root ? path : base_name(path);
	const char *branch = is_last ? "└──" : "├──";
	char *child_prefix;

	if (lstat(path, &st) != 0) {
		if (is_root)
			printf("%s [not found: %s]\n", path, strerror(errno));
		else
			printf("%s%s %s [not found: %s]\n", prefix, branch, name, strerror(errno));
		return;
	}

	if (S_ISLNK(st.st_mode)) {
		char *target = read_symlink_target(path);

		if (is_root)
			printf("%s [symlink -> %s]\n", path, target ? target : strerror(errno));
		else
			printf("%s%s %s -> %s [symlink]\n", prefix, branch, name, target ? target : strerror(errno));

		free(target);
		return;
	}

	if (is_root) {
		if (S_ISDIR(st.st_mode)) {
			printf("%s/\n", path);
			child_prefix = strdup("");
			if (!child_prefix)
				return;
			walk_dir(path, child_prefix);
			free(child_prefix);
			return;
		}

		if (S_ISREG(st.st_mode)) {
			printf("%s [file, %lld bytes]\n", path, (long long)st.st_size);
			child_prefix = strdup("    ");
			if (!child_prefix)
				return;
			print_file_contents(path, child_prefix);
			free(child_prefix);
			return;
		}

		printf("%s [special file, %lld bytes]\n", path, (long long)st.st_size);
		return;
	}

	if (S_ISDIR(st.st_mode)) {
		printf("%s%s %s/\n", prefix, branch, name);
		child_prefix = extend_prefix(prefix, is_last);
		if (!child_prefix)
			return;
		walk_dir(path, child_prefix);
		free(child_prefix);
		return;
	}

	if (S_ISREG(st.st_mode)) {
		printf("%s%s %s [file, %lld bytes]\n", prefix, branch, name, (long long)st.st_size);
		child_prefix = extend_prefix(prefix, is_last);
		if (!child_prefix)
			return;
		print_file_contents(path, child_prefix);
		free(child_prefix);
		return;
	}

	printf("%s%s %s [special file, %lld bytes]\n", prefix, branch, name, (long long)st.st_size);
}

static void print_usage(const char *argv0)
{
	printf("Usage: %s [PATH ...]\n", argv0);
	printf("\n");
	printf("Print a tree view of the given paths and show text file contents inline.\n");
	printf("When no path is provided, kitten inspects the current directory.\n");
	printf("A single directory argument is opened in place before the tree is shown.\n");
}

int main(int argc, char **argv)
{
	int i;
	struct stat st;

	if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
		print_usage(argv[0]);
		return 0;
	}

	if (argc == 1) {
		walk_path(".", "", 1, 1);
		return 0;
	}

	if (argc == 2 && stat(argv[1], &st) == 0 && S_ISDIR(st.st_mode)) {
		if (chdir(argv[1]) != 0) {
			fprintf(stderr, "kitten: %s: %s\n", argv[1], strerror(errno));
			return 1;
		}

		walk_path(".", "", 1, 1);
		return 0;
	}

	for (i = 1; i < argc; ++i) {
		if (i > 1)
			putchar('\n');
		walk_path(argv[i], "", i == argc - 1, 1);
	}

	return 0;
}
