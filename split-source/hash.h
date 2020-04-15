#ifndef TEENYCSS_HASH_H_
#define TEENYCSS_HASH_H_

#include <stdint.h>
#include <stdlib.h>


typedef struct teenycss_hashmap teenycss_hashmap;


uint64_t teenycss_hash_ByteHash(const char *bytes, uint64_t byteslen);


void teenycss_hash_FreeMap(teenycss_hashmap *map);

void teenycss_hash_ClearMap(teenycss_hashmap *map);


teenycss_hashmap *teenycss_hash_NewBytesMap(int buckets);
int teenycss_hash_BytesMapSet(
    teenycss_hashmap *map, const char *bytes,
    size_t byteslen, uint64_t number
);
int teenycss_hash_BytesMapGet(
    teenycss_hashmap *map, const char *bytes,
    size_t byteslen, uint64_t *number
);
int teenycss_hash_BytesMapUnset(
    teenycss_hashmap *map, const char *bytes,
    size_t byteslen
);
int teenycss_hash_BytesMapIterate(
    teenycss_hashmap *map,
    int (*callback)(
        teenycss_hashmap *map, const char *bytes,
        uint64_t byteslen, uint64_t number,
        void *userdata
    ),
    void *userdata
);

teenycss_hashmap *teenycss_hash_NewStringMap(int buckets);
int teenycss_hash_StringMapSet(
    teenycss_hashmap *map, const char *s, uint64_t number
);
int teenycss_hash_StringMapGet(
    teenycss_hashmap *map, const char *s, uint64_t *number
);
int teenycss_hash_StringMapUnset(teenycss_hashmap *map, const char *s);


teenycss_hashmap *teenycss_hash_NewIntMap(int buckets);
int teenycss_hash_IntMapSet(
    teenycss_hashmap *map, int64_t key, uint64_t number
);
int teenycss_hash_IntMapGet(
    teenycss_hashmap *map, int64_t key, uint64_t *number
);
int teenycss_hash_IntMapUnset(teenycss_hashmap *map, int64_t key);


teenycss_hashmap *teenycss_hash_NewStringToStringMap(int buckets);
int teenycss_hash_STSMapSet(
    teenycss_hashmap *map, const char *key, const char *value
);
const char *teenycss_hash_STSMapGet(
    teenycss_hashmap *map, const char *key
);
int teenycss_hash_STSMapUnset(teenycss_hashmap *map, const char *key);
int teenycss_hash_STSMapIterate(
    teenycss_hashmap *map,
    int (*cb)(teenycss_hashmap *map, const char *key,
              const char *value, void *ud),
    void *ud
);

typedef struct teenycss_hashset teenycss_hashset;
teenycss_hashset *teenycss_hashset_New(int buckets);
int teenycss_hashset_Contains(
    teenycss_hashset *set,
    const void *itemdata, size_t itemdatasize
);
int teenycss_hashset_Add(
    teenycss_hashset *set, const void *itemdata, size_t itemdatasize
);
void teenycss_hashset_Remove(
    teenycss_hashset *set, const void *itemdata, size_t itemdatasize
);
void teenycss_hashset_Free(teenycss_hashset *set);

#endif  // TEENYCSS_HASH_H_
