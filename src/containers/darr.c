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
