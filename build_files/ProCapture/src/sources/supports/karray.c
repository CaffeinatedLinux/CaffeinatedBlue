////////////////////////////////////////////////////////////////////////////////
// CONFIDENTIAL and PROPRIETARY software of Magewell Electronics Co., Ltd.
// Copyright (c) 2011-2014 Magewell Electronics Co., Ltd. (Nanjing)
// All rights reserved.
// This copyright notice MUST be reproduced on all authorized copies.
////////////////////////////////////////////////////////////////////////////////

#include "karray.h"

void karray_init(struct karray *array, long element_size)
{
    array->data = NULL;
    array->count = 0;
    array->max_count = 0;
    array->element_size = element_size;
    array->alloc_size = 0;
}

void karray_free(struct karray *array)
{
    if (array->data != NULL) {
        os_free(array->data);
    }

    array->data = NULL;
    array->count = 0;
    array->max_count = 0;
    array->alloc_size = 0;
}

int karray_set_element(struct karray *array, long element_nr, void *src)
{
    void *dst;

    if (karray_is_empty(array) || element_nr >= array->count)
        return OS_RETURN_ERROR;

    dst = &array->data[element_nr * array->element_size];
    os_memcpy(dst, src, array->element_size);

    return OS_RETURN_SUCCESS;
}

bool karray_set_count(struct karray *array, long count, bool keep_orig)
{
    void *data = NULL;
    size_t alloc_size = 0;
    long element_size = array->element_size;

    if (count == 0) {
        karray_free(array);
        return true;
    }

    if (count <= array->max_count) {
        array->count = count;
        return true;
    }

    alloc_size = array->element_size * count;
    data = os_malloc(alloc_size);

    if (data == NULL)
        return false;

    alloc_size = os_mem_get_actual_size(data);

    if (keep_orig && array->data != NULL)
        os_memcpy(data, array->data, array->count * array->element_size);

    karray_free(array);
    array->data = data;
    array->count = count;
    array->element_size = element_size;
    array->max_count = alloc_size / element_size;
    array->alloc_size = alloc_size;

    return true;
}

bool karray_copy(struct karray *array, void *src, long count)
{
    if (!karray_set_count(array, count, false))
        return false;

    os_memcpy(array->data, src, array->element_size * count);
    return true;
}

bool karray_append(struct karray *array, void *src, long count)
{
    long old_count = array->count;

    if (!karray_set_count(array, array->count + count, true))
        return false;

    os_memcpy(&array->data[old_count * array->element_size], src,
            array->element_size * count);

    return true;
}

void karray_delete_element(struct karray *array, long element_nr)
{
    if (karray_is_empty(array) || element_nr >= array->count)
        return;

    array->count--;
    if (array->count == element_nr)
        return;

    os_memmove(&array->data[element_nr * array->element_size],
            &array->data[(element_nr + 1) * (array->element_size)],
            array->count - element_nr);
}
