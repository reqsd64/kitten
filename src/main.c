#define _POSIX_C_SOURCE 200809L

#include <dirent.h>
#include <errno.h>
#include <fnmatch.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define DEFAULT_PREVIEW_LIMIT ((size_t)256 * 1024)

static size_t content_preview_limit = DEFAULT_PREVIEW_LIMIT;
static long max_depth = -1;
static int show_content = 1;
static int ascii_tree;
static int show_summary;

static char **exclude_patterns;
static size_t exclude_count;

static unsigned long total_dirs;
static unsigned long total_files;
static unsigned long total_symlinks;
static unsigned long total_special;
static unsigned long total_skipped;
static unsigned long total_errors;
static unsigned long long total_bytes;

static const char *branch_for(int is_last)
{
	if (ascii_tree)
		return is_last ? "`--" : "|--";
	return is_last ? "└──" : "├──";
}

static const char *stem_for(int is_last)
{
	if (ascii_tree)
		return is_last ? "    " : "|   ";
	return is_last ? "    " : "│   ";
}

static int show_dirent(const struct dirent *entry)
{
	return strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0;
}

static int is_excluded(const char *path, const char *name)
{
	size_t i;

	for (i = 0; i < exclude_count; ++i) {
		if (fnmatch(exclude_patterns[i], name, 0) == 0 ||
		    fnmatch(exclude_patterns[i], path, FNM_PATHNAME) == 0)
			return 1;
		if (path[0] == '.' && path[1] == '/' &&
		    fnmatch(exclude_patterns[i], path + 2, FNM_PATHNAME) == 0)
			return 1;
	}

	return 0;
}

static int add_exclude(const char *pattern)
{
	char **patterns;
	char *copy;

	if (!pattern[0])
		return -1;

	copy = strdup(pattern);
	if (!copy)
		return -1;

	patterns = realloc(exclude_patterns, (exclude_count + 1) * sizeof(*exclude_patterns));
	if (!patterns) {
		free(copy);
		return -1;
	}

	exclude_patterns = patterns;
	exclude_patterns[exclude_count++] = copy;
	return 0;
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

	printf("%s%s contents\n", prefix, ascii_tree ? "+--" : "┌──");

	if (size > (off_t)content_preview_limit) {
		printf("%s%s [skipped: %lld bytes exceeds %zu byte limit]\n",
		    prefix, ascii_tree ? "|" : "│",
		    (long long)size, content_preview_limit);
		printf("%s%s\n", prefix, ascii_tree ? "`--" : "└──");
		++total_skipped;
		return 0;
	}

	fp = fopen(path, "rb");
	if (!fp) {
		printf("%s%s [cannot open: %s]\n", prefix, ascii_tree ? "|" : "│", strerror(errno));
		printf("%s%s\n", prefix, ascii_tree ? "`--" : "└──");
		++total_errors;
		return 1;
	}

	if (has_nul_byte(fp)) {
		printf("%s%s [binary file]\n", prefix, ascii_tree ? "|" : "│");
		fclose(fp);
		printf("%s%s\n", prefix, ascii_tree ? "`--" : "└──");
		++total_skipped;
		return 0;
	}

	while ((nread = getline(&line, &cap, fp)) != -1) {
		line_bytes = (size_t)nread;
		if (line_bytes > content_preview_limit - bytes_read) {
			printf("%s%s [truncated after %zu bytes]\n",
			    prefix, ascii_tree ? "|" : "│", content_preview_limit);
			++total_skipped;
			saw_content = 1;
			break;
		}
		bytes_read += line_bytes;
		while (nread > 0 && (line[nread - 1] == '\n' || line[nread - 1] == '\r'))
			line[--nread] = '\0';
		if (nread == 0)
			printf("%s%s\n", prefix, ascii_tree ? "|" : "│");
		else
			printf("%s%s %s\n", prefix, ascii_tree ? "|" : "│", line);
		saw_content = 1;
	}

	if (ferror(fp)) {
		printf("%s%s [read error]\n", prefix, ascii_tree ? "|" : "│");
		had_error = 1;
		++total_errors;
	} else if (!saw_content) {
		printf("%s%s [empty file]\n", prefix, ascii_tree ? "|" : "│");
	}

	free(line);
	fclose(fp);
	printf("%s%s\n", prefix, ascii_tree ? "`--" : "└──");
	return had_error;
}

static int print_entry(const char *path, const char *prefix, int is_last, int is_root, long depth);

static int set_depth_limit(const char *text)
{
	char *end;
	unsigned long value;

	if (!text[0] || text[0] == '-')
		return -1;

	errno = 0;
	value = strtoul(text, &end, 10);
	if (errno == ERANGE || end == text || *end || value > LONG_MAX)
		return -1;

	max_depth = (long)value;
	return 0;
}

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

static int set_content_mode(const char *text)
{
	if (strcmp(text, "auto") == 0) {
		show_content = 1;
		return 0;
	}

	if (strcmp(text, "never") == 0) {
		show_content = 0;
		return 0;
	}

	return -1;
}

static void print_summary(void)
{
	printf("\nsummary:\n");
	printf("  directories: %lu\n", total_dirs);
	printf("  files:       %lu\n", total_files);
	printf("  symlinks:    %lu\n", total_symlinks);
	printf("  special:     %lu\n", total_special);
	printf("  bytes:       %llu\n", total_bytes);
	printf("  skipped:     %lu\n", total_skipped);
	printf("  errors:      %lu\n", total_errors);
}

static int print_directory(const char *path, const char *prefix, long depth)
{
	struct dirent **entries = NULL;
	int count = scandir(path, &entries, show_dirent, alphasort);
	int i;
	int j;
	int had_error = 0;
	int is_last_visible;

	if (count < 0) {
		printf("%s%s [cannot read directory: %s]\n",
		    prefix, branch_for(1), strerror(errno));
		++total_errors;
		return 1;
	}

	if (count == 0) {
		printf("%s%s [empty directory]\n", prefix, branch_for(1));
		free(entries);
		return 0;
	}

	for (i = 0; i < count; ++i) {
		char *child_path = join_path(path, entries[i]->d_name);

		if (!child_path) {
			printf("%s%s %s [out of memory]\n",
			    prefix, branch_for(i == count - 1), entries[i]->d_name);
			had_error = 1;
			++total_errors;
			free(entries[i]);
			continue;
		}

		if (is_excluded(child_path, entries[i]->d_name)) {
			++total_skipped;
			free(child_path);
			free(entries[i]);
			continue;
		}

		is_last_visible = 1;
		for (j = i + 1; j < count; ++j) {
			char *next_path = join_path(path, entries[j]->d_name);

			if (!next_path || !is_excluded(next_path, entries[j]->d_name)) {
				free(next_path);
				is_last_visible = 0;
				break;
			}
			free(next_path);
		}

		if (print_entry(child_path, prefix, is_last_visible, 0, depth) != 0)
			had_error = 1;
		free(child_path);
		free(entries[i]);
	}

	free(entries);
	return had_error;
}

static int print_entry(const char *path, const char *prefix, int is_last, int is_root, long depth)
{
	struct stat st;
	const char *slash = strrchr(path, '/');
	const char *name = is_root || !slash ? path : slash + 1;
	const char *branch = branch_for(is_last);
	char *next_prefix;
	const char *tail;
	size_t prefix_len;

	if (lstat(path, &st) != 0) {
		if (is_root)
			printf("%s [not found: %s]\n", path, strerror(errno));
		else
			printf("%s%s %s [not found: %s]\n", prefix, branch, name, strerror(errno));
		++total_errors;
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

		++total_symlinks;
		if (len < 0)
			++total_errors;
		return len < 0;
	}

	if (is_root) {
		if (S_ISDIR(st.st_mode)) {
			++total_dirs;
			printf("%s/\n", path);
			if (max_depth == 0)
				return 0;
			next_prefix = strdup("");
			if (!next_prefix) {
				printf("%s [out of memory]\n", branch_for(1));
				++total_errors;
				return 1;
			}
			if (print_directory(path, next_prefix, 1) != 0) {
				free(next_prefix);
				return 1;
			}
			free(next_prefix);
			return 0;
		}

		if (S_ISREG(st.st_mode)) {
			++total_files;
			total_bytes += (unsigned long long)st.st_size;
			printf("%s [file, %lld bytes]\n", path, (long long)st.st_size);
			if (!show_content)
				return 0;
			next_prefix = strdup("    ");
			if (!next_prefix) {
				printf("    [out of memory]\n");
				++total_errors;
				return 1;
			}
			if (print_file_preview(path, next_prefix, st.st_size) != 0) {
				free(next_prefix);
				return 1;
			}
			free(next_prefix);
			return 0;
		}

		++total_special;
		printf("%s [special file, %lld bytes]\n", path, (long long)st.st_size);
		return 0;
	}

	if (S_ISDIR(st.st_mode)) {
		++total_dirs;
		printf("%s%s %s/\n", prefix, branch, name);
		if (max_depth >= 0 && depth >= max_depth)
			return 0;
		tail = stem_for(is_last);
		prefix_len = strlen(prefix);
		next_prefix = malloc(prefix_len + 5);
		if (!next_prefix) {
			printf("%s%s [out of memory]\n", prefix, stem_for(is_last));
			++total_errors;
			return 1;
		}
		memcpy(next_prefix, prefix, prefix_len);
		memcpy(next_prefix + prefix_len, tail, 4);
		next_prefix[prefix_len + 4] = '\0';
		if (print_directory(path, next_prefix, depth + 1) != 0) {
			free(next_prefix);
			return 1;
		}
		free(next_prefix);
		return 0;
	}

	if (S_ISREG(st.st_mode)) {
		++total_files;
		total_bytes += (unsigned long long)st.st_size;
		printf("%s%s %s [file, %lld bytes]\n", prefix, branch, name, (long long)st.st_size);
		if (!show_content)
			return 0;
		tail = stem_for(is_last);
		prefix_len = strlen(prefix);
		next_prefix = malloc(prefix_len + 5);
		if (!next_prefix) {
			printf("%s%s [out of memory]\n", prefix, stem_for(is_last));
			++total_errors;
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

	++total_special;
	printf("%s%s %s [special file, %lld bytes]\n", prefix, branch, name, (long long)st.st_size);
	return 0;
}

static void print_usage(const char *argv0)
{
	printf("Usage: %s [OPTION]... [PATH]...\n", argv0);
	printf("\n");
	printf("Print a tree view of the given paths and show text file contents inline.\n");
	printf("When no path is provided, kitten inspects the current directory.\n");
	printf("A single directory argument is opened in place before the tree is shown.\n");
	printf("\n");
	printf("Options:\n");
	printf("  -m, --max-bytes=BYTES   change the per-file content preview limit\n");
	printf("  -L, --depth=DEPTH       descend at most DEPTH directory levels\n");
	printf("      --exclude=PATTERN   skip names or paths matching PATTERN\n");
	printf("      --no-content        print only the tree and file metadata\n");
	printf("      --content=WHEN      show content previews: auto or never\n");
	printf("      --ascii             use ASCII tree drawing characters\n");
	printf("      --summary           print counts after the tree\n");
	printf("  -h, --help              display this help and exit\n");
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

		if (strcmp(argv[first_path], "-L") == 0 || strcmp(argv[first_path], "--depth") == 0) {
			if (++first_path == argc || set_depth_limit(argv[first_path]) != 0) {
				fprintf(stderr, "kitten: invalid depth\n");
				return 1;
			}
			++first_path;
			continue;
		}

		if (strncmp(argv[first_path], "--depth=", 8) == 0) {
			if (set_depth_limit(argv[first_path] + 8) != 0) {
				fprintf(stderr, "kitten: invalid depth\n");
				return 1;
			}
			++first_path;
			continue;
		}

		if (strcmp(argv[first_path], "--exclude") == 0) {
			if (++first_path == argc || add_exclude(argv[first_path]) != 0) {
				fprintf(stderr, "kitten: invalid exclude pattern\n");
				return 1;
			}
			++first_path;
			continue;
		}

		if (strncmp(argv[first_path], "--exclude=", 10) == 0) {
			if (add_exclude(argv[first_path] + 10) != 0) {
				fprintf(stderr, "kitten: invalid exclude pattern\n");
				return 1;
			}
			++first_path;
			continue;
		}

		if (strcmp(argv[first_path], "--no-content") == 0) {
			show_content = 0;
			++first_path;
			continue;
		}

		if (strcmp(argv[first_path], "--content") == 0) {
			if (++first_path == argc || set_content_mode(argv[first_path]) != 0) {
				fprintf(stderr, "kitten: invalid content mode\n");
				return 1;
			}
			++first_path;
			continue;
		}

		if (strncmp(argv[first_path], "--content=", 10) == 0) {
			if (set_content_mode(argv[first_path] + 10) != 0) {
				fprintf(stderr, "kitten: invalid content mode\n");
				return 1;
			}
			++first_path;
			continue;
		}

		if (strcmp(argv[first_path], "--ascii") == 0) {
			ascii_tree = 1;
			++first_path;
			continue;
		}

		if (strcmp(argv[first_path], "--summary") == 0) {
			show_summary = 1;
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
		had_error = print_entry(".", "", 1, 1, 0) != 0;
	} else if (first_path == argc - 1 && lstat(argv[first_path], &st) == 0 && S_ISDIR(st.st_mode)) {
		if (chdir(argv[first_path]) != 0) {
			fprintf(stderr, "kitten: %s: %s\n", argv[first_path], strerror(errno));
			return 1;
		}

		had_error = print_entry(".", "", 1, 1, 0) != 0;
	} else {
		for (i = first_path; i < argc; ++i) {
			if (i > first_path)
				putchar('\n');
			if (print_entry(argv[i], "", i == argc - 1, 1, 0) != 0)
				had_error = 1;
		}
	}

	if (show_summary)
		print_summary();

	return had_error;
}
