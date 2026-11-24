/* SPDX-License-Identifier: BSD-2-Clause */
#define _GNU_SOURCE 1
#include "../../gpu/sweetgl/ProgramCacheStore.h"

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); exit(1); } } while (0)

static int publish(const char *root, const unsigned char id[32],
    const unsigned char key[32], unsigned kind)
{
    unsigned char request[sizeof(SweetRenderPipelineConfig) + 2] = {0};
    request[sizeof(request) - 2] = 'v';
    request[sizeof(request) - 1] = 'f';
    const unsigned char binary[] = {1, 2, 3};
    const int is_request = kind == SWEET_PROGRAM_CACHE_REQUEST;
    const unsigned char *payload = is_request ? request : binary;
    const size_t payload_bytes = is_request ? sizeof(request) : sizeof(binary);
    unsigned char record[SWEET_PROGRAM_CACHE_HEADER + sizeof(request)];
    CHECK(sweet_cache_header(record, kind, is_request ? 0 : 0x875f, id, key, payload, payload_bytes, 1, 1));
    memcpy(record + SWEET_PROGRAM_CACHE_HEADER, payload, payload_bytes);
    return sweet_cache_write_file(root, id, key, kind, record, SWEET_PROGRAM_CACHE_HEADER + payload_bytes);
}

int main(int argc, char **argv)
{
    CHECK(argc == 2);
    const char *root = argv[1];
    CHECK(mkdir(root, 0700) == 0);
    const unsigned char id[32] = {0x37};
    unsigned char key[32] = {0};
#ifdef __APPLE__
    const unsigned pairs = 32;
#else
    const unsigned pairs = 16;
#endif
    for (unsigned i = 0; i < pairs; ++i) {
        key[0] = (unsigned char)i;
        CHECK(publish(root, id, key, SWEET_PROGRAM_CACHE_REQUEST) == 0);
        CHECK(publish(root, id, key, SWEET_PROGRAM_CACHE_BINARY) == 0);
    }
    CHECK(publish(root, id, key, SWEET_PROGRAM_CACHE_BINARY) == -EEXIST);
    char path[1024];
    unsigned char *record;
    size_t bytes;
    CHECK(sweet_cache_path(root, id, key, SWEET_PROGRAM_CACHE_BINARY, path) == 0);
    CHECK(sweet_cache_read_file(path, &record, &bytes) == 0);
    CHECK(sweet_cache_validate(record, bytes, SWEET_PROGRAM_CACHE_BINARY, id, key));
    free(record);
    key[0] = (unsigned char)pairs;
    CHECK(publish(root, id, key, SWEET_PROGRAM_CACHE_REQUEST) == -ENOSPC);
    CHECK(publish(root, id, key, SWEET_PROGRAM_CACHE_BINARY) == -ENOSPC);
    DIR *dir = opendir(root);
    CHECK(dir);
    unsigned files = 0;
    struct dirent *entry;
    while ((entry = readdir(dir))) {
        if (entry->d_name[0] != '.') ++files;
    }
    CHECK(closedir(dir) == 0 && files == 64);
    printf("cache publication: %u pairs, %u names, full budget rejected, existing record readable\n", pairs, files);
    return 0;
}
