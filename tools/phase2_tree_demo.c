#include "pes.h"
#include "tree.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int object_write(ObjectType type, const void *data, size_t len, ObjectID *id_out);
void object_path(const ObjectID *id, char *path_out, size_t path_size);

static int ensure_dir(const char *path) {
    if (mkdir(path, 0755) == 0) return 0;
    return access(path, F_OK) == 0 ? 0 : -1;
}

static int write_demo_index(const ObjectID *readme_id, const ObjectID *main_id) {
    char readme_hex[HASH_HEX_SIZE + 1];
    char main_hex[HASH_HEX_SIZE + 1];
    hash_to_hex(readme_id, readme_hex);
    hash_to_hex(main_id, main_hex);

    FILE *f = fopen(INDEX_FILE, "w");
    if (!f) return -1;

    fprintf(f, "100644 %s 0 11 README.md\n", readme_hex);
    fprintf(f, "100644 %s 0 29 src/main.c\n", main_hex);

    if (fclose(f) != 0) return -1;
    return 0;
}

int main(void) {
    const char *readme = "hello tree\n";
    const char *main_c = "int main(void) { return 0; }\n";
    ObjectID readme_id, main_id, tree_id;
    char tree_hex[HASH_HEX_SIZE + 1];
    char tree_path[512];

    if (system("rm -rf .pes") != 0) return 1;

    if (ensure_dir(PES_DIR) != 0) return 1;
    if (ensure_dir(OBJECTS_DIR) != 0) return 1;
    if (ensure_dir(".pes/refs") != 0) return 1;
    if (ensure_dir(REFS_DIR) != 0) return 1;

    if (object_write(OBJ_BLOB, readme, strlen(readme), &readme_id) != 0) return 1;
    if (object_write(OBJ_BLOB, main_c, strlen(main_c), &main_id) != 0) return 1;
    if (write_demo_index(&readme_id, &main_id) != 0) return 1;
    if (tree_from_index(&tree_id) != 0) return 1;

    hash_to_hex(&tree_id, tree_hex);
    object_path(&tree_id, tree_path, sizeof(tree_path));

    printf("Root tree hash: %s\n", tree_hex);
    printf("Root tree path: %s\n", tree_path);
    return 0;
}
