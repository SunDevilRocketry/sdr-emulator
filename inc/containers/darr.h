/**
 * @file darr.h
 *
 * Dynamic array implemented via dark voodoo magic.
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

#ifndef D_ARR_H_
#define D_ARR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <assert.h>

/**
 * @brief Contains information relating to dynamic array state
 */
struct _darray_secret_header {
        size_t elementSize;     /**< How big each element of the array is */
        size_t arrElementCount; /**< How many elements are currently in the array */ 
        size_t arrCapacity; /** < How many elements the array can currently hold */
};

/**
 * @brief The factor which the size of the array will grow by on a realloc
 */
#define DARRAY_GROWTH_FACTOR 2

/**
 * @breif The initial capcity if it is set to zero when array is created
 */
#define DARRAY_INITIAL_CAPACITY 1

/**
 * @brief Internal macro for retrieving a dynamic array header from the array
 */
#define _DARRAY_GET_HEADER(ARR) (((struct _darray_secret_header*)ARR) - 1)

/**
 * @brief Internal macro that returns whether the an array is within range for a dynamic array
 */
#define _DARRAY_CHECK_BOUNDS(ARR, INDEX) (INDEX < _DARRAY_GET_HEADER(ARR)->arrElementCount)

/**
 * @brief Internal mcaro that returns whether the array should resize
 */
#define _DARRAY_CHECK_SHOULD_RESIZE(ARR) ( _DARRAY_GET_HEADER(ARR)->arrElementCount < _DARRAY_GET_HEADER(ARR)->arrCapacity )

/**
 * @brief Internal macro that adds a value to the end of the array
 */
#define _DARRAY_ADD_TO_ARR_END(ARR, VALUE) ARR[_DARRAY_GET_HEADER(ARR)->arrElementCount] = VALUE 

/**
 * @brief Internal macro that calls the grow function to increase the array size
 */
#define _DARRAY_GROW(ARR) _darray_grow_func((void**)ARR)

/**
 * @brief Internal macro which pushes a value onto a dyanamic array referenced via double pointer
 */
#define _DARRAY_PUSH_DP(ARR, VALUE)  \
        ( (_DARRAY_CHECK_SHOULD_RESIZE((*ARR))) ? ((void)0) : (_DARRAY_GROW(ARR)), \
        _DARRAY_ADD_TO_ARR_END((*ARR), VALUE), \
        _DARRAY_GET_HEADER((*ARR))->arrElementCount +=1, VALUE )

/**
 * @brief Internal macro which pushes a value onto a dyanmic array referenced via single pointer
 */
#define _DARRAY_PUSH_SP(ARR, VALUE)  \
        ( (_DARRAY_CHECK_SHOULD_RESIZE(ARR)) ? ((void)0) : (_DARRAY_GROW(&ARR)), \
        _DARRAY_ADD_TO_ARR_END(ARR, VALUE), \
        _DARRAY_GET_HEADER(ARR)->arrElementCount +=1, VALUE )

/**
 * @brief Creates a new dynamic array via the new function
 */
#define DARRAY_NEW(TYPE, SIZE) _darray_new_func(sizeof(TYPE) * SIZE + sizeof(struct _darray_secret_header), SIZE, sizeof(TYPE))

/**
 * @brief Returns the number of elements currently in the array
 */
#define DARRAY_SIZE(ARR) (_DARRAY_GET_HEADER(ARR)->arrElementCount)

/** darray_push macro that either calls a resize function or just assigns the index
 * @brief Attempts to push a value onto the passed darray. Resizes if insufficient space.
 */
#define DARRAY_PUSH(ARR, VALUE) _DARRAY_PUSH_SP(ARR, VALUE)

/** 
 * @brief Attempts to push a value onto the passed darray. Resizes if insufficient space.
 *
 * The passed array should be referenced via double pointer. This macro is intended to accomodate resizing dynamic arrays from a different
 * stack frame in which it was allocated. You may either use this macro, or return the dynamic array from all functions it may have been resized in.
 */
#define DARRAY_PUSH_ESF(ARR, VALUE) _DARRAY_PUSH_DP(ARR, VALUE)

/**
 * @brief Removes and returns the value on the end of the array
 */
#define DARRAY_POP(ARR) (_DARRAY_GET_HEADER(ARR)->arrElementCount--, ARR[_DARRAY_GET_HEADER(ARR)->arrElementCount])

/**
 * @brief Frees the dynamic array
 *
 * @warning Do not try to free a dynamic array without this function
 */
#define DARRAY_FREE(ARR) free(_DARRAY_GET_HEADER(ARR))

/**
 * @brief Returns the value at the passed index from the dynamic array (bounds checked)
 *
 * @warning Contains an assert that will terminate execution if index is OOB
 */
#define DARRAY_GET(ARR, INDEX) (assert(_DARRAY_CHECK_BOUNDS(ARR, INDEX)), (ARR[INDEX]))

/**
 * @brief Sets the given index of the dynamic array to the passed value
 */
#define DARRAY_SET(ARR, INDEX, VALUE) (assert(_DARRAY_CHECK_BOUNDS(ARR, INDEX)), ARR[INDEX] = VALUE)

/**
 * @brief Returns the total capacity of the dynamic array
 */
#define DARRAY_GET_CAPACITY(ARR) (_DARRAY_GET_HEADER(ARR)->arrCapacity)

void _darray_grow_func(void** array);
        
void* _darray_new_func(size_t sizeInBytes, size_t capacity, size_t elementSize);

#ifdef __cplusplus
}
#endif

#endif /* D_ARR_H_ */
