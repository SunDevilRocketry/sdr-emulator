#ifndef D_ARR_H_
#define D_ARR_H_

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

// 16 bytes
struct _darray_secret_header {
        size_t elementSize;
        size_t arrElementCount;
        size_t arrCapacity;
};

#define DARRAY_GROWTH_FACTOR 2

// The initial capcity if it is set to zero when array is created
#define DARRAY_INITIAL_CAPACITY 1

#define _DARRAY_GET_HEADER(ARR) (((struct _darray_secret_header*)ARR) - 1)
#define _DARRAY_CHECK_BOUNDS(ARR, INDEX) (INDEX < _DARRAY_GET_HEADER(ARR)->arrElementCount)

#define _DARRAY_CHECK_SHOULD_RESIZE(ARR) ( _DARRAY_GET_HEADER(ARR)->arrElementCount < _DARRAY_GET_HEADER(ARR)->arrCapacity )
#define _DARRAY_ADD_TO_ARR_END(ARR, VALUE) ARR[_DARRAY_GET_HEADER(ARR)->arrElementCount] = VALUE 
#define _DARRAY_GROW(ARR) _DARRAY_GROW_FUNC((void**)ARR)

#define DARRAY_NEW(TYPE, SIZE) _DARRAY_NEW_FUNC(sizeof(TYPE) * SIZE + sizeof(struct _darray_secret_header), SIZE, sizeof(TYPE))

#define _DARRAY_PUSH_DP(ARR, VALUE)  \
        ( (_DARRAY_CHECK_SHOULD_RESIZE((*ARR))) ? ((void)0) : (_DARRAY_GROW(ARR)), \
        _DARRAY_ADD_TO_ARR_END((*ARR), VALUE), \
        _DARRAY_GET_HEADER((*ARR))->arrElementCount +=1, VALUE )

#define _DARRAY_PUSH_SP(ARR, VALUE)  \
        ( (_DARRAY_CHECK_SHOULD_RESIZE(ARR)) ? ((void)0) : (_DARRAY_GROW(&ARR)), \
        _DARRAY_ADD_TO_ARR_END(ARR, VALUE), \
        _DARRAY_GET_HEADER(ARR)->arrElementCount +=1, VALUE )

#define DARRAY_SIZE(ARR) (_DARRAY_GET_HEADER(ARR)->arrElementCount)
// darray_push macro that either calls a resize function or just assigns the index
#define DARRAY_PUSH(ARR, VALUE) _DARRAY_PUSH_SP(ARR, VALUE)
// Accepts a double pointer so it can modify the array from any external stack frames
#define DARRAY_PUSH_ESF(ARR, VALUE) _DARRAY_PUSH_DP(ARR, VALUE)

#define DARRAY_POP(ARR) (_DARRAY_GET_HEADER(ARR)->arrElementCount--, ARR[_DARRAY_GET_HEADER(ARR)->arrElementCount])

#define DARRAY_FREE(ARR) free(_DARRAY_GET_HEADER(ARR))

#define DARRAY_GET(ARR, INDEX) (assert(_DARRAY_CHECK_BOUNDS(ARR, INDEX)), (ARR[INDEX]))

#define DARRAY_SET(ARR, INDEX, VALUE) (assert(_DARRAY_CHECK_BOUNDS(ARR, INDEX)), ARR[INDEX] = VALUE)

#define DARRAY_GET_CAPACITY(ARR) (_DARRAY_GET_HEADER(ARR)->arrCapacity)


void _DARRAY_GROW_FUNC(void** array);
        
void* _DARRAY_NEW_FUNC(size_t sizeInBytes, size_t capacity, size_t elementSize);

#endif /* D_ARR_H_ */
