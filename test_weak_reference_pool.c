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

#include "linked_list_hashmap.h"
#include "fixed_arraylist.h"
#include "weak_reference_pool.h"
#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "CuTest.h"

static arraylistf_t *__list = NULL;

static void *__dud = NULL;

static void *__get(
    int num
)
{
    return arraylistf_get(__list, num - 1);
}

static void *__create_obj(
    int *id
)
{
    void *obj;

    obj = calloc(1,sizeof(int));
    *id = arraylistf_add(__list, obj);
    *id += 1;
    return obj;
}

static void __insert(
    void *object,
    int num
)
{
    arraylistf_insert(__list, object, num - 1);
}

static void *__remove(
    int num
)
{
    return arraylistf_remove(__list, num - 1);
}

void __remove_object(
    void *pool,
    int num
)
{
    void *item;

    item = arraylistf_remove(pool, num - 1);
    free(item);
    wr_remove(pool, num);
}

static void __init(
)
{
    if (__list)
    {
        //tea_iter_forall(arraylistf_iter(__list), tea_free);
        arraylistf_free(__list);
    }
    if (__dud)
    {
        free(__dud);
    }
    __dud = calloc(1,sizeof(int));
    __list = arraylistf_new();
}

void TestWR_InitPool(
    CuTest * tc
)
{
    void *pool;

    __init();
    pool = wr_pool_init(__get, __remove, __insert, __dud);
    CuAssertTrue(tc, NULL != pool);

    wr_pool_release(pool);
}

void TestWR_ObtainIncreasesCount(
    CuTest * tc
)
{
    void *pool;

    int id, wr;

    __init();
    pool = wr_pool_init(__get, __remove, __insert, __dud);

    __create_obj(&id);
    CuAssertTrue(tc, 0 == wr_get_num_references(pool, id));

    wr = wr_obtain(pool, id);
    CuAssertTrue(tc, wr == id);
    CuAssertTrue(tc, 1 == wr_get_num_references(pool, id));

    wr_pool_release(pool);
}

void TestWR_GetGetsObject(
    CuTest * tc
)
{
    void *pool, *obj, *obj_get;

    int id, wr;

    __init();
    pool = wr_pool_init(__get, __remove, __insert, __dud);

    obj = __create_obj(&id);
    wr = wr_obtain(pool, id);
    CuAssertTrue(tc, wr == id);

    obj_get = wr_get(pool, &id);
    CuAssertTrue(tc, obj_get == obj);
    CuAssertTrue(tc, 1 == wr_get_num_references(pool, id));

    wr_pool_release(pool);
}

void TestWR_ReleaseLowersRefCountAndFailsGetObject(
    CuTest * tc
)
{
    void *pool, *obj_get;

    int id, wr;

    __init();
    pool = wr_pool_init(__get, __remove, __insert, __dud);

    __create_obj(&id);
    wr = wr_obtain(pool, id);
    CuAssertTrue(tc, wr == id);

    wr_release(pool, &id);
    CuAssertTrue(tc, 0 == wr_get_num_references(pool, wr));
    obj_get = wr_get(pool, &wr);
    CuAssertTrue(tc, obj_get == NULL);

    wr_pool_release(pool);
}

void TestWR_RemovalAndGetLowersRefcountAndFailsGetObject(
    CuTest * tc
)
{
    void *pool, *obj_get;

    int id, wr;

    __init();
    pool = wr_pool_init(__get, __remove, __insert, __dud);

    __create_obj(&id);
    wr = wr_obtain(pool, id);
    CuAssertTrue(tc, wr == id);

    __remove_object(pool, id);
    CuAssertTrue(tc, 1 == wr_get_num_references(pool, id));
    obj_get = wr_get(pool, &id);
    CuAssertTrue(tc, obj_get == NULL);
    CuAssertTrue(tc, 0 == wr_get_num_references(pool, wr));

    wr_pool_release(pool);
}
