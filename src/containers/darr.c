/**
 * @file darr.c
 *
 * Functions implementations to handle non-macroable DARRAY procedures
 *
 * @copyright
 *       Copyright 2026 Robert Camacho
 *       
 *       Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
 *       to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 *       and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so.
 *       
 *       THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *       FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 *       LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
 *       IN THE SOFTWARE.
 */

#include "containers/darr.h"
#include <stdlib.h>

/**
 * @brief Grows the passed dynamic array by the growth factor. Initialized to initial capacity if empty.
 *
 * @warning Avoid calling this without using the macro
 */
void _darray_grow_func(void** array) {
        struct _darray_secret_header* arrayHeader = _DARRAY_GET_HEADER(*array);
        if ( arrayHeader->arrCapacity > 0)
            {
            arrayHeader->arrCapacity *= DARRAY_GROWTH_FACTOR; 
            }
        else
            {
            arrayHeader->arrCapacity = DARRAY_INITIAL_CAPACITY;
            }

        void* tmp = realloc(arrayHeader, arrayHeader->elementSize * arrayHeader->arrCapacity + sizeof(struct _darray_secret_header));
        if (tmp == NULL) {
                free(*array);
                *array = NULL;
                abort();
                return;
        }
        
        *array = ((struct _darray_secret_header*)tmp) + 1;
}

/**
 * @brief creates and returns a new dynamic array
 *
 * @warning Avoid calling this without using the macro
 */
void* _darray_new_func(size_t sizeInBytes, size_t capacity, size_t elementSize) {
        /* This is never zero, as the size of the header will always be allocated */
        void* allocation = malloc(sizeInBytes);

        if ( allocation == NULL ) {
                return NULL;
        }
        *((struct _darray_secret_header*)allocation) = (struct _darray_secret_header){.elementSize = elementSize, .arrElementCount = 0, .arrCapacity = capacity};

        allocation = ((struct _darray_secret_header*)allocation) + 1;

        return allocation;
}
