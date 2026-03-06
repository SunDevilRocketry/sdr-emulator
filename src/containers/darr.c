#include "containers/darr.h"

void _DARRAY_GROW_FUNC(void** array) {
//        printf("ARRAY ADDRE: %p\n", *array);
 //       printf("GROW_HEADER ADDRE: %p\n", *array);
        _DARRAY_GET_HEADER(*array)->arrCapacity = (_DARRAY_GET_HEADER(*array)->arrCapacity > 0) ? _DARRAY_GET_HEADER(*array)->arrCapacity * DARRAY_GROWTH_FACTOR : DARRAY_INITIAL_CAPACITY;
        void* tmp = realloc(_DARRAY_GET_HEADER(*array), _DARRAY_GET_HEADER(*array)->elementSize * _DARRAY_GET_HEADER(*array)->arrCapacity + sizeof(struct _darray_secret_header));
        if (tmp == NULL) {
                free(*array);
                *array = NULL;
                return;
        }
        
        *array = ((struct _darray_secret_header*)tmp) + 1;
}

void* _DARRAY_NEW_FUNC(size_t sizeInBytes, size_t capacity, size_t elementSize) {
        // This is never zero, as the size of the header will always be allocated
        void* allocation = malloc(sizeInBytes);
        if ( allocation == NULL ) {
                return NULL;
        }
        *((struct _darray_secret_header*)allocation) = (struct _darray_secret_header){.elementSize = elementSize, .arrElementCount = 0, .arrCapacity = capacity};

        allocation = ((struct _darray_secret_header*)allocation) + 1;

        return allocation;
}
