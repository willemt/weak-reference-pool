/*
 
Copyright (c) 2011, Willem-Hendrik Thiart
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * The names of its contributors may not be used to endorse or promote
      products derived from this software without specific prior written
      permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL WILLEM-HENDRIK THIART BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include "weak_reference_pool.h"
#include "linked_list_hashmap.h"

typedef struct
{
    int id;
    int weakref_cnt;
    bool removed;
} entityMem_t;

typedef struct
{
    hashmap_t *weakref_map;
    void *(*get)(int);
    void *(*remove)(int);
    void (*insert)(void *insert, int);
    void *dudItem;
} wr_pool_t;

static unsigned long __uint_hash(
    const void *e1
)
{
    const long i1 = (unsigned long) e1;

    assert(i1 >= 0);
    return i1;
}

static long __uint_compare(
    const void *e1,
    const void *e2
)
{
    const long i1 = (unsigned long) e1, i2 = (unsigned long) e2;

    return i1 - i2;
}

#define in(x) ((wr_pool_t*)(x))

/**
 * Initialise a weak reference pool.
 * @param get get the actual object using this callback
 * @param remove remove the actual object using this callback
 * @param  insert insert the actual object using this callback
 * @param dudItem a tombstone for marking removed objects
 */
void *wr_pool_init(
    void *(*get) (int),
    void *(*remove) (int),
    void (*insert) (void *, int),
    void *dudItem
)
{
    wr_pool_t *pool;

    assert(get);
    assert(remove);
    assert(dudItem);
    pool = calloc(1,sizeof(wr_pool_t));
    pool->get = get;
    pool->insert = insert;
    pool->remove = remove;
    pool->dudItem = dudItem;
    pool->weakref_map = hashmap_new(__uint_hash, __uint_compare, 11);
    return pool;
}

/**
 * Release all memory related to pool */
void wr_pool_release(
    void *pool
)
{
    hashmap_iterator_t iter;
    void* key;

    for (hashmap_iterator(in(pool)->weakref_map, &iter);
            ((key = hashmap_iterator_next(in(pool)->weakref_map, &iter)));)
    {
        free(hashmap_remove(in(pool)->weakref_map, key));
        free(key);
    }

    hashmap_freeall(in(pool)->weakref_map);
    free(pool);
}

/**
 * Hold a reference to this object
 * @param pool : the pool that registers the reference
 * @param id : the ID of the object
 * @return ID belonging to object, otherwise -1 on error */
int wr_obtain(
    void *pool,
    const int id
)
{
    entityMem_t *emem;
    void *item;

    assert(0 <= id);

    /* 1. entity must exist */
    if (!(item = in(pool)->get(id)))
    {
        return -1;
    }

    /* 2. see if it has been obtained before */
    emem = hashmap_get(in(pool)->weakref_map, (void *) (long) id);
    if (!emem)
    {
        assert(in(pool)->weakref_map);
        /* 2b. allocate this emem */
        emem = calloc(1,sizeof(entityMem_t));
        emem->id = id;
        emem->weakref_cnt = 0;
        assert(!hashmap_get(in(pool)->weakref_map, (void *) (long) id));
        hashmap_put(in(pool)->weakref_map, (void *) (long) id, emem);
    }

    /* 3. increment */
    emem->weakref_cnt += 1;
    return id; /* convienence */
}

static void __remove_emem_from_pool(
    void *pool,
    int *id,
    entityMem_t * emem
)
{
    void *item;

    entityMem_t *emem2;

    /* 3b is the entity inuse?
     * ..having zero weakreferences does not mean it is finished. */
    emem2 = hashmap_remove(in(pool)->weakref_map, (void *) (*id));
    assert(emem2);
    assert(emem2 == emem);
    assert(!hashmap_get(in(pool)->weakref_map, (void *) (*id)));

    free(emem);
    item = in(pool)->get(*id);
    assert(in(pool)->dudItem);
    assert(item);
    assert(!hashmap_get(in(pool)->weakref_map, (void *) (*id)));

    item = in(pool)->get(*id);
    assert(in(pool)->dudItem);
    assert(item);

    /* 3c remove the dudentity */
    if (item == in(pool)->dudItem)
    {
        in(pool)->remove(*id);
    }
}

/**
 * Decrement the reference count on this entity. Will set id to zero
 * @param id the ID of the object we want to release */
void wr_release(
    void *pool,
    int *id
)
{
    entityMem_t *emem;

    /* this is a convienence */
    if (0 == *id)
        return;

    /* 1. must have been obtained before */
    emem = hashmap_get(in(pool)->weakref_map, (void *) (*id));
    assert(emem);
    assert(emem->id == *id);
    assert(0 < emem->weakref_cnt);

    /* 2. decrement */
    emem->weakref_cnt -= 1;

    /* 3. have we hit zero references yet? */
    if (0 == emem->weakref_cnt)
    {
        __remove_emem_from_pool(pool, id, emem);
    }
    *id = 0;
}

/**
 * Set id to zero if this entity has been released.
 * @return NULL if the object is gone! Ref will be released ie. refcnt -= 1 */
void *wr_get(
    void *pool,
    int *id
)
{
    entityMem_t *emem;

    assert(0 <= *id);

    /* more convienence */
    if (0 == *id)
        return NULL;

    if ((emem = hashmap_get(in(pool)->weakref_map, (void *) (*id))))
    {
        void *item;

        item = in(pool)->get(*id);
        if (item == in(pool)->dudItem)
        {
            wr_release(pool, id);
            return NULL;
        }
        else
        {
            return item;
        }
    }
    else
    {
        /*  this is actually a little bit invalid? */
        *id = 0;
        return NULL;
    }
}

int wr_get_num_references(
    void *pool,
    const int id
)
{
    entityMem_t *emem;

    if (0 == id)
    {
        return 0;
    }

    if ((emem = hashmap_get(in(pool)->weakref_map, (void *) (long) id)))
    {
        return emem->weakref_cnt;
    }
    else
    {
        return 0;
    }
}

/* 
 * notify the weak reference that this object has been removed.
 * Replace with dud entity. */
void wr_remove(
    void *pool,
    const int id
)
{
    in(pool)->insert(in(pool)->dudItem, id);
}

