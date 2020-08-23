
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


static char global_teenycss_hashsecret[16];


#define TEENYCSSHASHTYPE_BYTES 0
#define TEENYCSSHASHTYPE_STRING 1
#define TEENYCSSHASHTYPE_NUMBER 2
#define TEENYCSSHASHTYPE_STRINGTOSTRING 3

typedef struct teenycss_hashmap_bucket teenycss_hashmap_bucket;

typedef struct teenycss_hashmap_bucket {
    char *bytes; uint64_t byteslen;
    uint64_t number;
    teenycss_hashmap_bucket *next, *prev;
} teenycss_hashmap_bucket;


typedef struct teenycss_hashmap {
    int type;
    int bucket_count;
    teenycss_hashmap_bucket **buckets;
} teenycss_hashmap;


uint64_t teenycss_hash_StringHash(const char *s) {
    return teenycss_hash_ByteHash(s, strlen(s));
}


teenycss_hashmap *teenycss_hash_NewBytesMap(int buckets) {
    teenycss_hashmap *map = malloc(sizeof(*map));
    if (!map)
        return NULL;
    memset(map, 0, sizeof(*map));
    map->bucket_count = buckets;
    map->buckets = malloc(sizeof(void*) * buckets);
    if (!map->buckets) {
        free(map);
        return NULL;
    }
    memset(map->buckets, 0, sizeof(void*) * buckets);
    return map;
}

teenycss_hashmap *teenycss_hash_NewStringMap(int buckets) {
    teenycss_hashmap *map = teenycss_hash_NewBytesMap(buckets);
    if (!map)
        return NULL;
    map->type = TEENYCSSHASHTYPE_STRING;
    return map;
}

teenycss_hashmap *teenycss_hash_NewStringToStringMap(int buckets) {
    teenycss_hashmap *map = teenycss_hash_NewBytesMap(buckets);
    if (!map)
        return NULL;
    map->type = TEENYCSSHASHTYPE_STRINGTOSTRING;
    return map;
}

teenycss_hashmap *teenycss_hash_NewIntMap(int buckets) {
    teenycss_hashmap *map = teenycss_hash_NewBytesMap(buckets);
    if (!map)
        return NULL;
    map->type = TEENYCSSHASHTYPE_NUMBER;
    return map;
}


static int _teenycss_hash_MapGetBucket(
        teenycss_hashmap *map, const char *bytes, int byteslen) {
    uint64_t hash = teenycss_hash_ByteHash(bytes, byteslen);
    uint64_t bucket = (uint64_t)(hash % ((uint64_t)map->bucket_count));
    return bucket;
}

static int _teenycss_hash_MapSet(
        teenycss_hashmap *map, const char *bytes,
        int byteslen, uint64_t result) {
    int i = _teenycss_hash_MapGetBucket(map, bytes, byteslen);
    teenycss_hashmap_bucket *bk = malloc(sizeof(*bk));
    if (!bk)
        return 0;
    memset(bk, 0, sizeof(*bk));
    bk->bytes = malloc(byteslen);
    if (!bk->bytes) {
        free(bk);
        return 0;
    }
    memcpy(bk->bytes, bytes, byteslen);
    bk->byteslen = byteslen;
    teenycss_hashmap_bucket *prevbk = NULL;
    if (map->buckets[i]) {
        prevbk = map->buckets[i];
        while (prevbk && prevbk->next)
            prevbk = prevbk->next;
    }
    bk->number = result;
    bk->prev = prevbk;
    if (!prevbk) {
        map->buckets[i] = bk;
    } else {
        prevbk->next = bk;
    }
    return 1;
}


static int _teenycss_hash_MapGet(
        teenycss_hashmap *map, const char *bytes,
        uint64_t byteslen, uint64_t *result) {
    int i = _teenycss_hash_MapGetBucket(map, bytes, byteslen);
    if (!map->buckets[i])
        return 0;
    teenycss_hashmap_bucket *bk = map->buckets[i];
    while (bk) {
        if (bk->byteslen == byteslen &&
                memcmp(bk->bytes, bytes, byteslen) == 0) {
            *result = bk->number;
            return 1;
        }
        bk = bk->next;
    }
    return 0;
}


static int _teenycss_hash_MapUnset(
        teenycss_hashmap *map, const char *bytes,
        uint64_t byteslen) {
    if (!map || !bytes)
        return 0; 
   int i = _teenycss_hash_MapGetBucket(map, bytes, byteslen);
    if (!map->buckets[i])
        return 0;
    teenycss_hashmap_bucket *prevbk = NULL;
    teenycss_hashmap_bucket *bk = map->buckets[i];
    while (bk) {
        if (bk->byteslen == byteslen &&
                bk->bytes &&
                memcmp(bk->bytes, bytes, byteslen) == 0) {
            if (prevbk)
                prevbk->next = bk->next;
            if (prevbk && prevbk->next)
                prevbk->next->prev = prevbk;
            if (!prevbk)
                map->buckets[i] = bk->next;
            if (bk->bytes)
                free(bk->bytes);
            free(bk);
            return 1;
        }
        prevbk = bk;
        bk = bk->next;
    }
    return 0;
}


int teenycss_hash_BytesMapSet(
        teenycss_hashmap *map, const char *bytes,
        size_t byteslen, uint64_t number) {
    if (!map || map->type != TEENYCSSHASHTYPE_BYTES || !bytes)
        return 0;
    _teenycss_hash_MapUnset(map, bytes, byteslen);
    return _teenycss_hash_MapSet(
        map, bytes, byteslen, number
    );
}

struct bytemapiterateentry {
    char *bytes;
    uint64_t byteslen;
    uint64_t number;
};

int teenycss_hash_BytesMapIterate(
        teenycss_hashmap *map,
        int (*cb)(teenycss_hashmap *map, const char *bytes,
                  uint64_t byteslen, uint64_t number, void *ud),
        void *ud
        ) {
    if (!map || map->type != TEENYCSSHASHTYPE_BYTES)
        return 0;

    struct bytemapiterateentry *entries = NULL;
    int alloc_size = 0;
    int found_entries = 0;

    int i = 0;
    while (i < map->bucket_count) {
        teenycss_hashmap_bucket *bk = map->buckets[i];
        while (bk) {
            if (alloc_size <= found_entries) {
                alloc_size *= 2;
                if (alloc_size < found_entries + 8)
                    alloc_size = found_entries + 8;
                struct bytemapiterateentry *new_entries = realloc(
                    entries, sizeof(*entries) * alloc_size
                );
                if (!new_entries) {
                    allocfail: ;
                    int k = 0;
                    while (k < found_entries) {
                        if (entries[k].bytes)
                            free(entries[k].bytes);
                        k++;
                    }
                    free(entries);
                    return 0;
                }
                entries = new_entries;
            }
            memset(&entries[found_entries],
                   0, sizeof(entries[found_entries]));
            if (bk->byteslen > 0) {
                entries[found_entries].bytes = malloc(bk->byteslen);
                if (!entries[found_entries].bytes)
                    goto allocfail;
                memcpy(entries[found_entries].bytes,
                       bk->bytes, bk->byteslen);
                entries[found_entries].byteslen = bk->byteslen;
            }
            entries[found_entries].number = bk->number;
            bk = bk->next;
        }
        i++;
    }
    i = 0;
    while (i < found_entries) {
        if (!cb(map, entries[i].bytes, entries[i].byteslen,
                entries[i].number, ud))
            break;
        i++;
    }
    i = 0;
    while (i < found_entries) {
        if (entries[i].bytes)
            free(entries[i].bytes);
        i++;
    }
    free(entries);
    return 1;
}

int teenycss_hash_BytesMapGet(
        teenycss_hashmap *map, const char *bytes,
        size_t byteslen, uint64_t *number) {
    if (map->type != TEENYCSSHASHTYPE_BYTES)
        return 0;
    return _teenycss_hash_MapGet(
        map, bytes, byteslen, number
    );
}


int teenycss_hash_BytesMapUnset(
        teenycss_hashmap *map, const char *bytes,
        size_t byteslen) {
    if (map->type != TEENYCSSHASHTYPE_BYTES)
        return 0;
    return _teenycss_hash_MapUnset(map, bytes, byteslen);
}


int teenycss_hash_StringMapSet(
        teenycss_hashmap *map, const char *s, uint64_t number
        ) {
    if (map->type != TEENYCSSHASHTYPE_STRING)
        return 0;
    _teenycss_hash_MapUnset(map, s, strlen(s));
    return _teenycss_hash_MapSet(
        map, s, strlen(s), number
    );
}


int teenycss_hash_StringMapGet(
        teenycss_hashmap *map, const char *s, uint64_t *number
        ) {
    if (map->type != TEENYCSSHASHTYPE_STRING)
        return 0;
    return _teenycss_hash_MapGet(
        map, s, strlen(s), number
    );
}


int teenycss_hash_StringMapUnset(teenycss_hashmap *map, const char *s) {
    if (map->type != TEENYCSSHASHTYPE_STRING)
        return 0;
    return _teenycss_hash_MapUnset(map, s, strlen(s));
}

int teenycss_hash_IntMapSet(
        teenycss_hashmap *map, int64_t key, uint64_t number
        ) {
    if (map->type != TEENYCSSHASHTYPE_NUMBER)
        return 0;
    _teenycss_hash_MapUnset(map, (char*)&key, sizeof(key));
    return _teenycss_hash_MapSet(
        map, (char*)&key, sizeof(key), number
    );
}


int teenycss_hash_IntMapGet(teenycss_hashmap *map, int64_t key, uint64_t *number) {
    if (map->type != TEENYCSSHASHTYPE_NUMBER)
        return 0;
    return _teenycss_hash_MapGet(
        map, (char*)&key, sizeof(key), number
    );
}

int teenycss_hash_IntMapUnset(teenycss_hashmap *map, int64_t key) {
    if (map->type != TEENYCSSHASHTYPE_NUMBER)
        return 0;
    return _teenycss_hash_MapUnset(map, (char*)&key, sizeof(key));
}

int siphash(const uint8_t *in, const size_t inlen, const uint8_t *k,
            uint8_t *out, const size_t outlen);

uint64_t teenycss_hash_ByteHash(const char *bytes, uint64_t byteslen) {
    uint64_t hashval = 0;
    siphash((uint8_t*)bytes, byteslen, (uint8_t*)&global_teenycss_hashsecret,
            (uint8_t*)&hashval, sizeof(hashval));
    return hashval;
}

void teenycss_hash_ClearMap(teenycss_hashmap *map) {
    if (!map)
        return;
    if (map->buckets) {
        int i = 0;
        while (i < map->bucket_count) {
            teenycss_hashmap_bucket *bk = map->buckets[i];
            while (bk) {
                if (map->type == TEENYCSSHASHTYPE_STRINGTOSTRING) {
                    char *svalue = (char*)(uintptr_t)bk->number;
                    if (svalue)
                        free(svalue);
                }
                if (bk->bytes)
                    free(bk->bytes);
                teenycss_hashmap_bucket *nextbk = bk->next;
                free(bk);
                bk = nextbk;
            }
            map->buckets[i] = NULL;
            i++;
        }
    }
}

void teenycss_hash_FreeMap(teenycss_hashmap *map) {
    if (!map)
        return;
    teenycss_hash_ClearMap(map);
    if (map->buckets)
        free(map->buckets);
    free(map);
}

__attribute__((constructor)) static void teenycss_hashSetHashSecrets() {
    if (!teenycss_secrandom_GetBytes(
            global_teenycss_hashsecret, sizeof(global_teenycss_hashsecret)
            )) {
        fprintf(stderr,
            "hash.c: FAILED TO INITIALIZE GLOBAL "
            "HASH SECRETS. ABORTING.\n"
        );
        exit(0);
    } 
}

int teenycss_hash_STSMapSet(
        teenycss_hashmap *map, const char *key, const char *value
        ) {
    if (map->type != TEENYCSSHASHTYPE_STRINGTOSTRING)
        return 0;
    uint64_t number = (uintptr_t)strdup(value);
    if (number == 0)
        return 0;
    teenycss_hash_STSMapUnset(map, key);
    if (!_teenycss_hash_MapSet(
            map, key, strlen(key), number
            )) {
        free((char*)number);
        return 0;
    }
    return 1;
}

const char *teenycss_hash_STSMapGet(teenycss_hashmap *map, const char *key) {
    if (map->type != TEENYCSSHASHTYPE_STRINGTOSTRING)
        return NULL;
    uint64_t number = 0;
    if (!_teenycss_hash_MapGet(
            map, key, strlen(key), &number
            ))
        return NULL;
    return (const char *)(uintptr_t)number;
}

int teenycss_hash_STSMapUnset(teenycss_hashmap *map, const char *key) {
    if (map->type != TEENYCSSHASHTYPE_STRINGTOSTRING)
        return 0;
    uint64_t number = 0;
    if (_teenycss_hash_MapGet(
            map, key, strlen(key), &number
            ) && number > 0) {
        free((void*)(uintptr_t)number);
    }
    return _teenycss_hash_MapUnset(map, key, strlen(key));
}

struct stsmapiterateentry {
    char *key, *value;
};

int teenycss_hash_STSMapIterate(
        teenycss_hashmap *map,
        int (*cb)(teenycss_hashmap *map, const char *key,
                  const char *value, void *ud),
        void *ud
        ) {
    if (!map || map->type != TEENYCSSHASHTYPE_STRINGTOSTRING)
        return 0;

    struct stsmapiterateentry *entries = NULL;
    int alloc_size = 0;
    int found_entries = 0;

    int i = 0;
    while (i < map->bucket_count) {
        teenycss_hashmap_bucket *bk = map->buckets[i];
        while (bk) {
            if (alloc_size <= found_entries) {
                alloc_size *= 2;
                if (alloc_size < found_entries + 8)
                    alloc_size = found_entries + 8;
                struct stsmapiterateentry *new_entries = realloc(
                    entries, sizeof(*entries) * alloc_size
                );
                if (!new_entries) {
                    allocfail: ;
                    int k = 0;
                    while (k < found_entries) {
                        if (entries[k].key)
                            free(entries[k].key);
                        if (entries[k].value)
                            free(entries[k].value);
                        k++;
                    }
                    free(entries);
                    return 0;
                }
                entries = new_entries;
            }
            memset(&entries[found_entries],
                   0, sizeof(entries[found_entries]));
            if (bk->bytes != NULL) {
                entries[found_entries].key = strdup(
                    (const char*)bk->bytes
                );
                if (!entries[found_entries].key)
                    goto allocfail;
            }
            if (bk->number != 0) {
                entries[found_entries].value = strdup(
                    (const char*)(uintptr_t)bk->number
                ); 
                if (!entries[found_entries].value)
                    goto allocfail;
            }
            bk = bk->next;
        }
        i++;
    }
    i = 0;
    while (i < found_entries) {
        if (!cb(map, entries[i].key, entries[i].value, ud))
            break;
        i++;
    }
    i = 0;
    while (i < found_entries) {
        if (entries[i].key)
            free(entries[i].key);
        if (entries[i].value)
            free(entries[i].value);
        i++;
    }
    free(entries);
    return 1;
}



typedef struct teenycss_hashset {
   teenycss_hashmap *map;
} teenycss_hashset;


teenycss_hashset *teenycss_hashset_New(int buckets) {
    teenycss_hashset *set = malloc(sizeof(*set));
    if (!set)
        return NULL;
    memset(set, 0, sizeof(*set));
    set->map = teenycss_hash_NewBytesMap(buckets);
    if (!set->map) {
        free(set);
        return NULL;
    }
    return set;
}

int teenycss_hashset_Contains(
        teenycss_hashset *set,
        const void *itemdata, size_t itemdatasize
        ) {
    uint64_t result;
    if (!teenycss_hash_BytesMapGet(
            set->map, (const char *)itemdata, itemdatasize,
            &result))
        return 0;
    return 1;
}

int teenycss_hashset_Add(
        teenycss_hashset *set, const void *itemdata, size_t itemdatasize
        ) {
    if (teenycss_hashset_Contains(set, itemdata, itemdatasize))
        return 1;
    return teenycss_hash_BytesMapSet(
        set->map, (const char *)itemdata, itemdatasize, (uint64_t)1
    );
}

void teenycss_hashset_Remove(
        teenycss_hashset *set,
        const void *itemdata, size_t itemdatasize
        ) {
    teenycss_hash_BytesMapUnset(
        set->map, (const char *)itemdata, itemdatasize
    );
}

void teenycss_hashset_Free(teenycss_hashset *set) {
    if (!set)
        return;
    if (set->map)
        teenycss_hash_FreeMap(set->map);
    free(set);
}
