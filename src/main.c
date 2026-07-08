#define _POSIX_C_SOURCE 200809L

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define DEFAULT_PREVIEW_LIMIT ((size_t)256 * 1024)

static size_t content_preview_limit = DEFAULT_PREVIEW_LIMIT;

static int show_dirent(const struct dirent *entry)
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

static int has_nul_byte(FILE *fp)
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

static int print_file_preview(const char *path, const char *prefix, off_t size)
{
	FILE *fp;
	char *line = NULL;
	size_t cap = 0;
	size_t bytes_read = 0;
	size_t line_bytes;
	ssize_t nread;
	int saw_content = 0;
	int had_error = 0;

	printf("%s┌── contents\n", prefix);

	if (size > (off_t)content_preview_limit) {
		printf("%s│ [skipped: %lld bytes exceeds %zu byte limit]\n",
		    prefix, (long long)size, content_preview_limit);
		printf("%s└──\n", prefix);
		return 0;
	}

	fp = fopen(path, "rb");
	if (!fp) {
		printf("%s│ [cannot open: %s]\n", prefix, strerror(errno));
		printf("%s└──\n", prefix);
		return 1;
	}

	if (has_nul_byte(fp)) {
		printf("%s│ [binary file]\n", prefix);
		fclose(fp);
		printf("%s└──\n", prefix);
		return 0;
	}

	while ((nread = getline(&line, &cap, fp)) != -1) {
		line_bytes = (size_t)nread;
		if (line_bytes > content_preview_limit - bytes_read) {
			printf("%s│ [truncated after %zu bytes]\n", prefix, content_preview_limit);
			saw_content = 1;
			break;
		}
		bytes_read += line_bytes;
		while (nread > 0 && (line[nread - 1] == '\n' || line[nread - 1] == '\r'))
			line[--nread] = '\0';
		if (nread == 0)
			printf("%s│\n", prefix);
		else
			printf("%s│ %s\n", prefix, line);
		saw_content = 1;
	}

	if (ferror(fp)) {
		printf("%s│ [read error]\n", prefix);
		had_error = 1;
	} else if (!saw_content) {
		printf("%s│ [empty file]\n", prefix);
	}

	free(line);
	fclose(fp);
	printf("%s└──\n", prefix);
	return had_error;
}

static int print_entry(const char *path, const char *prefix, int is_last, int is_root);

static int set_preview_limit(const char *text)
{
	char *end;
	unsigned long long value;
	unsigned long long scale = 1;
	unsigned long long total;

	if (!text[0] || text[0] == '-')
		return -1;

	errno = 0;
	value = strtoull(text, &end, 10);
	if (errno == ERANGE || end == text)
		return -1;

	if (*end) {
		if (end[1])
			return -1;
		if (*end == 'k' || *end == 'K')
			scale = 1024;
		else if (*end == 'm' || *end == 'M')
			scale = 1024 * 1024;
		else if (*end == 'g' || *end == 'G')
			scale = 1024 * 1024 * 1024ULL;
		else
			return -1;
	}

	if (value > ULLONG_MAX / scale)
		return -1;
	total = value * scale;
	if (total > (unsigned long long)(size_t)-1)
		return -1;

	content_preview_limit = (size_t)total;
	return 0;
}

static int print_directory(const char *path, const char *prefix)
{
	struct dirent **entries = NULL;
	int count = scandir(path, &entries, show_dirent, alphasort);
	int i;
	int had_error = 0;

	if (count < 0) {
		printf("%s└── [cannot read directory: %s]\n", prefix, strerror(errno));
		return 1;
	}

	if (count == 0) {
		printf("%s└── [empty directory]\n", prefix);
		free(entries);
		return 0;
	}

	for (i = 0; i < count; ++i) {
		char *child_path = join_path(path, entries[i]->d_name);

		if (!child_path) {
			printf("%s%s %s [out of memory]\n",
			    prefix, i == count - 1 ? "└──" : "├──", entries[i]->d_name);
			had_error = 1;
			free(entries[i]);
			continue;
		}

		if (print_entry(child_path, prefix, i == count - 1, 0) != 0)
			had_error = 1;
		free(child_path);
		free(entries[i]);
	}

	free(entries);
	return had_error;
}

static int print_entry(const char *path, const char *prefix, int is_last, int is_root)
{
	struct stat st;
	const char *slash = strrchr(path, '/');
	const char *name = is_root || !slash ? path : slash + 1;
	const char *branch = is_last ? "└──" : "├──";
	char *next_prefix;
	const char *tail;
	size_t prefix_len;

	if (lstat(path, &st) != 0) {
		if (is_root)
			printf("%s [not found: %s]\n", path, strerror(errno));
		else
			printf("%s%s %s [not found: %s]\n", prefix, branch, name, strerror(errno));
		return 1;
	}

	if (S_ISLNK(st.st_mode)) {
		char target[4096];
		ssize_t len = readlink(path, target, sizeof(target) - 1);
		int saved_errno = errno;

		if (len >= 0)
			target[len] = '\0';

		if (is_root)
			printf("%s [symlink -> %s]\n", path, len >= 0 ? target : strerror(saved_errno));
		else
			printf("%s%s %s -> %s [symlink]\n", prefix, branch, name,
			    len >= 0 ? target : strerror(saved_errno));

		return len < 0;
	}

	if (is_root) {
		if (S_ISDIR(st.st_mode)) {
			printf("%s/\n", path);
			next_prefix = strdup("");
			if (!next_prefix) {
				printf("└── [out of memory]\n");
				return 1;
			}
			if (print_directory(path, next_prefix) != 0) {
				free(next_prefix);
				return 1;
			}
			free(next_prefix);
			return 0;
		}

		if (S_ISREG(st.st_mode)) {
			printf("%s [file, %lld bytes]\n", path, (long long)st.st_size);
			next_prefix = strdup("    ");
			if (!next_prefix) {
				printf("    [out of memory]\n");
				return 1;
			}
			if (print_file_preview(path, next_prefix, st.st_size) != 0) {
				free(next_prefix);
				return 1;
			}
			free(next_prefix);
			return 0;
		}

		printf("%s [special file, %lld bytes]\n", path, (long long)st.st_size);
		return 0;
	}

	if (S_ISDIR(st.st_mode)) {
		printf("%s%s %s/\n", prefix, branch, name);
		tail = is_last ? "    " : "│   ";
		prefix_len = strlen(prefix);
		next_prefix = malloc(prefix_len + 5);
		if (!next_prefix) {
			printf("%s%s [out of memory]\n", prefix, is_last ? "    " : "│   ");
			return 1;
		}
		memcpy(next_prefix, prefix, prefix_len);
		memcpy(next_prefix + prefix_len, tail, 4);
		next_prefix[prefix_len + 4] = '\0';
		if (print_directory(path, next_prefix) != 0) {
			free(next_prefix);
			return 1;
		}
		free(next_prefix);
		return 0;
	}

	if (S_ISREG(st.st_mode)) {
		printf("%s%s %s [file, %lld bytes]\n", prefix, branch, name, (long long)st.st_size);
		tail = is_last ? "    " : "│   ";
		prefix_len = strlen(prefix);
		next_prefix = malloc(prefix_len + 5);
		if (!next_prefix) {
			printf("%s%s [out of memory]\n", prefix, is_last ? "    " : "│   ");
			return 1;
		}
		memcpy(next_prefix, prefix, prefix_len);
		memcpy(next_prefix + prefix_len, tail, 4);
		next_prefix[prefix_len + 4] = '\0';
		if (print_file_preview(path, next_prefix, st.st_size) != 0) {
			free(next_prefix);
			return 1;
		}
		free(next_prefix);
		return 0;
	}

	printf("%s%s %s [special file, %lld bytes]\n", prefix, branch, name, (long long)st.st_size);
	return 0;
}

static void print_usage(const char *argv0)
{
	printf("Usage: %s [-m BYTES] [PATH ...]\n", argv0);
	printf("\n");
	printf("Print a tree view of the given paths and show text file contents inline.\n");
	printf("When no path is provided, kitten inspects the current directory.\n");
	printf("A single directory argument is opened in place before the tree is shown.\n");
	printf("Use -m or --max-bytes to change the per-file content preview limit.\n");
}

int main(int argc, char **argv)
{
	int i;
	int first_path = 1;
	struct stat st;
	int had_error = 0;

	while (first_path < argc) {
		if (strcmp(argv[first_path], "--") == 0) {
			++first_path;
			break;
		}

		if (strcmp(argv[first_path], "-h") == 0 || strcmp(argv[first_path], "--help") == 0) {
			print_usage(argv[0]);
			return 0;
		}

		if (strcmp(argv[first_path], "-m") == 0 || strcmp(argv[first_path], "--max-bytes") == 0) {
			if (++first_path == argc || set_preview_limit(argv[first_path]) != 0) {
				fprintf(stderr, "kitten: invalid preview limit\n");
				return 1;
			}
			++first_path;
			continue;
		}

		if (strncmp(argv[first_path], "--max-bytes=", 12) == 0) {
			if (set_preview_limit(argv[first_path] + 12) != 0) {
				fprintf(stderr, "kitten: invalid preview limit\n");
				return 1;
			}
			++first_path;
			continue;
		}

		if (argv[first_path][0] == '-' && argv[first_path][1]) {
			fprintf(stderr, "kitten: unknown option: %s\n", argv[first_path]);
			return 1;
		}

		break;
	}

	if (first_path == argc) {
		return print_entry(".", "", 1, 1) != 0;
	}

	if (first_path == argc - 1 && lstat(argv[first_path], &st) == 0 && S_ISDIR(st.st_mode)) {
		if (chdir(argv[first_path]) != 0) {
			fprintf(stderr, "kitten: %s: %s\n", argv[first_path], strerror(errno));
			return 1;
		}

		return print_entry(".", "", 1, 1) != 0;
	}

	for (i = first_path; i < argc; ++i) {
		if (i > first_path)
			putchar('\n');
		if (print_entry(argv[i], "", i == argc - 1, 1) != 0)
			had_error = 1;
	}

	return had_error;
}
