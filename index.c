// index.c — Staging area implementation
//
// Text format of .pes/index (one entry per line, sorted by path):
//
//   <mode-octal> <64-char-hex-hash> <mtime-seconds> <size> <path>
//
// Example:
//   100644 a1b2c3d4e5f6...  1699900000 42 README.md
//   100644 f7e8d9c0b1a2...  1699900100 128 src/main.c
//
// This is intentionally a simple text format. No magic numbers, no
// binary parsing. The focus is on the staging area CONCEPT (tracking
// what will go into the next commit) and ATOMIC WRITES (temp+rename).
//
// PROVIDED functions: index_find, index_remove, index_status
// TODO functions:     index_load, index_save, index_add

#include "index.h"
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>

int object_write(ObjectType type, const void *data, size_t len, ObjectID *id_out);

// ─── PROVIDED ────────────────────────────────────────────────────────────────

// Find an index entry by path (linear scan).
IndexEntry* index_find(Index *index, const char *path) {
    for (int i = 0; i < index->count; i++) {
        if (strcmp(index->entries[i].path, path) == 0)
            return &index->entries[i];
    }
    return NULL;
}

// Remove a file from the index.
// Returns 0 on success, -1 if path not in index.
int index_remove(Index *index, const char *path) {
    for (int i = 0; i < index->count; i++) {
        if (strcmp(index->entries[i].path, path) == 0) {
            int remaining = index->count - i - 1;
            if (remaining > 0)
                memmove(&index->entries[i], &index->entries[i + 1],
                        remaining * sizeof(IndexEntry));
            index->count--;
            return index_save(index);
        }
    }
    fprintf(stderr, "error: '%s' is not in the index\n", path);
    return -1;
}

// Print the status of the working directory.
//
// Identifies files that are staged, unstaged (modified/deleted in working dir),
// and untracked (present in working dir but not in index).
// Returns 0.
int index_status(const Index *index) {
    printf("Staged changes:\n");
    int staged_count = 0;
    // Note: A true Git implementation deeply diffs against the HEAD tree here. 
    // For this lab, displaying indexed files represents the staging intent.
    for (int i = 0; i < index->count; i++) {
        printf("  staged:     %s\n", index->entries[i].path);
        staged_count++;
    }
    if (staged_count == 0) printf("  (nothing to show)\n");
    printf("\n");

    printf("Unstaged changes:\n");
    int unstaged_count = 0;
    for (int i = 0; i < index->count; i++) {
        struct stat st;
        if (stat(index->entries[i].path, &st) != 0) {
            printf("  deleted:    %s\n", index->entries[i].path);
            unstaged_count++;
        } else {
            // Fast diff: check metadata instead of re-hashing file content
            if (st.st_mtime != (time_t)index->entries[i].mtime_sec || st.st_size != (off_t)index->entries[i].size) {
                printf("  modified:   %s\n", index->entries[i].path);
                unstaged_count++;
            }
        }
    }
    if (unstaged_count == 0) printf("  (nothing to show)\n");
    printf("\n");

    printf("Untracked files:\n");
    int untracked_count = 0;
    DIR *dir = opendir(".");
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            // Skip hidden directories, parent directories, and build artifacts
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
            if (strcmp(ent->d_name, ".pes") == 0) continue;
            if (strcmp(ent->d_name, "pes") == 0) continue; // compiled executable
            if (strstr(ent->d_name, ".o") != NULL) continue; // object files

            // Check if file is tracked in the index
            int is_tracked = 0;
            for (int i = 0; i < index->count; i++) {
                if (strcmp(index->entries[i].path, ent->d_name) == 0) {
                    is_tracked = 1; 
                    break;
                }
            }
            
            if (!is_tracked) {
                struct stat st;
                stat(ent->d_name, &st);
                if (S_ISREG(st.st_mode)) { // Only list regular files for simplicity
                    printf("  untracked:  %s\n", ent->d_name);
                    untracked_count++;
                }
            }
        }
        closedir(dir);
    }
    if (untracked_count == 0) printf("  (nothing to show)\n");
    printf("\n");

    return 0;
}

// ─── TODO: Implement these ───────────────────────────────────────────────────

static int compare_index_entries_by_path(const void *a, const void *b) {
    const IndexEntry *ea = (const IndexEntry *)a;
    const IndexEntry *eb = (const IndexEntry *)b;
    return strcmp(ea->path, eb->path);
}

// Load the index from .pes/index.
//
// HINTS - Useful functions:
//   - fopen (with "r"), fscanf, fclose : reading the text file line by line
//   - hex_to_hash                      : converting the parsed string to ObjectID
//
// Returns 0 on success, -1 on error.
int index_load(Index *index) {
    if (!index) return -1;

    index->count = 0;

    FILE *f = fopen(INDEX_FILE, "r");
    if (!f) {
        if (errno == ENOENT) return 0;
        return -1;
    }

    char line[2048];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '\n' || line[0] == '\0') continue;
        if (!strchr(line, '\n') && !feof(f)) {
            fclose(f);
            return -1;
        }
        if (index->count >= MAX_INDEX_ENTRIES) {
            fclose(f);
            return -1;
        }

        IndexEntry *entry = &index->entries[index->count];
        char hash_hex[HASH_HEX_SIZE + 1];
        unsigned long long mtime_sec;
        unsigned int size;

        int scanned = sscanf(line, "%o %64s %llu %u %511[^\n]",
                             &entry->mode,
                             hash_hex,
                             &mtime_sec,
                             &size,
                             entry->path);
        if (scanned != 5 || hex_to_hash(hash_hex, &entry->hash) != 0) {
            fclose(f);
            return -1;
        }

        entry->mtime_sec = (uint64_t)mtime_sec;
        entry->size = size;
        entry->path[strcspn(entry->path, "\r")] = '\0';
        index->count++;
    }

    if (ferror(f)) {
        fclose(f);
        return -1;
    }

    return fclose(f) == 0 ? 0 : -1;
}

// Save the index to .pes/index atomically.
//
// HINTS - Useful functions and syscalls:
//   - qsort                            : sorting the entries array by path
//   - fopen (with "w"), fprintf        : writing to the temporary file
//   - hash_to_hex                      : converting ObjectID for text output
//   - fflush, fileno, fsync, fclose    : flushing userspace buffers and syncing to disk
//   - rename                           : atomically moving the temp file over the old index
//
// Returns 0 on success, -1 on error.
int index_save(const Index *index) {
    if (!index) return -1;

    IndexEntry *sorted_entries = NULL;
    if (index->count > 0) {
        sorted_entries = malloc((size_t)index->count * sizeof(IndexEntry));
        if (!sorted_entries) return -1;
        memcpy(sorted_entries, index->entries,
               (size_t)index->count * sizeof(IndexEntry));
    }

    if (index->count > 1) {
        qsort(sorted_entries, index->count, sizeof(IndexEntry),
              compare_index_entries_by_path);
    }

    char tmp_path[sizeof(INDEX_FILE) + 16];
    if (snprintf(tmp_path, sizeof(tmp_path), "%s.XXXXXX", INDEX_FILE) >= (int)sizeof(tmp_path))
        return -1;

    int fd = mkstemp(tmp_path);
    if (fd < 0) return -1;

    FILE *f = fdopen(fd, "w");
    if (!f) {
        close(fd);
        unlink(tmp_path);
        free(sorted_entries);
        return -1;
    }

    int rc = 0;
    for (int i = 0; i < index->count; i++) {
        char hash_hex[HASH_HEX_SIZE + 1];
        hash_to_hex(&sorted_entries[i].hash, hash_hex);

        if (fprintf(f, "%o %s %" PRIu64 " %u %s\n",
                    sorted_entries[i].mode,
                    hash_hex,
                    sorted_entries[i].mtime_sec,
                    sorted_entries[i].size,
                    sorted_entries[i].path) < 0) {
            rc = -1;
            break;
        }
    }

    if (rc == 0 && fflush(f) != 0) rc = -1;
    if (rc == 0 && fsync(fileno(f)) != 0) rc = -1;
    if (fclose(f) != 0 && rc == 0) rc = -1;

    if (rc == 0) {
        if (rename(tmp_path, INDEX_FILE) != 0) rc = -1;
    }

    if (rc != 0) {
        unlink(tmp_path);
        free(sorted_entries);
        return -1;
    }

    free(sorted_entries);
    return 0;
}

// Stage a file for the next commit.
//
// HINTS - Useful functions and syscalls:
//   - fopen, fread, fclose             : reading the target file's contents
//   - object_write                     : saving the contents as OBJ_BLOB
//   - stat / lstat                     : getting file metadata (size, mtime, mode)
//   - index_find                       : checking if the file is already staged
//
// Returns 0 on success, -1 on error.
int index_add(Index *index, const char *path) {
    if (!index || !path) return -1;

    struct stat st;
    if (lstat(path, &st) != 0) return -1;
    if (!S_ISREG(st.st_mode)) return -1;
    if ((uint64_t)st.st_size > UINT32_MAX) return -1;

    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    size_t len = (size_t)st.st_size;
    void *data = NULL;
    if (len > 0) {
        data = malloc(len);
        if (!data) {
            fclose(f);
            return -1;
        }

        if (fread(data, 1, len, f) != len) {
            free(data);
            fclose(f);
            return -1;
        }
    }

    if (fclose(f) != 0) {
        free(data);
        return -1;
    }

    ObjectID hash;
    if (object_write(OBJ_BLOB, data, len, &hash) != 0) {
        free(data);
        return -1;
    }
    free(data);

    IndexEntry new_entry;
    memset(&new_entry, 0, sizeof(new_entry));
    new_entry.mode = (st.st_mode & S_IXUSR) ? 0100755 : 0100644;
    new_entry.hash = hash;
    new_entry.mtime_sec = (uint64_t)st.st_mtime;
    new_entry.size = (uint32_t)st.st_size;

    if (snprintf(new_entry.path, sizeof(new_entry.path), "%s", path) >= (int)sizeof(new_entry.path))
        return -1;

    IndexEntry *existing = index_find(index, path);
    if (existing) {
        *existing = new_entry;
    } else {
        if (index->count >= MAX_INDEX_ENTRIES) return -1;
        index->entries[index->count++] = new_entry;
    }

    return index_save(index);
}
