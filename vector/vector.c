/**
 * Vector
 * CS 241 - Fall 2019
 */
#include "vector.h"
#include <assert.h>
#include <stdio.h>
/**
 * 'INITIAL_CAPACITY' the initial size of the dynamically.
 */
const size_t INITIAL_CAPACITY = 8;
/**
 * 'GROWTH_FACTOR' is how much the vector will grow by in automatic reallocation
 * (2 means double).
 */
const size_t GROWTH_FACTOR = 2;

struct vector {
    /* The function callback for the user to define the way they want to copy
     * elements */
    copy_constructor_type copy_constructor;

    /* The function callback for the user to define the way they want to destroy
     * elements */
    destructor_type destructor;

    /* The function callback for the user to define the way they a default
     * element to be constructed */
    default_constructor_type default_constructor;

    /* Void pointer to the beginning of an array of void pointers to arbitrary
     * data. */
    void **array;

    /**
     * The number of elements in the vector.
     * This is the number of actual objects held in the vector,
     * which is not necessarily equal to its capacity.
     */
    size_t size;

    /**
     * The size of the storage space currently allocated for the vector,
     * expressed in terms of elements.
     */
    size_t capacity;
};

/**
 * IMPLEMENTATION DETAILS
 *
 * The following is documented only in the .c file of vector,
 * since it is implementation specfic and does not concern the user:
 *
 * This vector is defined by the struct above.
 * The struct is complete as is and does not need any modifications.
 *
 * The only conditions of automatic reallocation is that
 * they should happen logarithmically compared to the growth of the size of the
 * vector inorder to achieve amortized constant time complexity for appending to
 * the vector.
 *
 * For our implementation automatic reallocation happens when -and only when-
 * adding to the vector makes its new  size surpass its current vector capacity
 * OR when the user calls on vector_reserve().
 * When this happens the new capacity will be whatever power of the
 * 'GROWTH_FACTOR' greater than or equal to the target capacity.
 * In the case when the new size exceeds the current capacity the target
 * capacity is the new size.
 * In the case when the user calls vector_reserve(n) the target capacity is 'n'
 * itself.
 * We have provided get_new_capacity() to help make this less ambigious.
 */

static size_t get_new_capacity(size_t target) {
    /**
     * This function works according to 'automatic reallocation'.
     * Start at 1 and keep multiplying by the GROWTH_FACTOR untl
     * you have exceeded or met your target capacity.
     */
    size_t new_capacity = 1;
    while (new_capacity < target) {
        new_capacity *= GROWTH_FACTOR;
    }
    return new_capacity;
}

vector *vector_create(copy_constructor_type copy_constructor,
                      destructor_type destructor,
                      default_constructor_type default_constructor) {
    // your code here
    // Casting to void to remove complier error. Remove this line when you are
    // ready.

    vector * new_vector = malloc(sizeof(vector));
    //NULL check; if one function is shallow they should all be
    int use_shallow = copy_constructor == NULL || destructor == NULL || default_constructor == NULL;
    
    //copying parameters into new variables
    //reduces redundancy of if/else chain for assignment
    copy_constructor_type cconstruct = copy_constructor;
    destructor_type destruct = destructor;
    default_constructor_type dconstruct = default_constructor;

    //modify function pointers to shallow versions only if user-requested
    if (use_shallow) {
        cconstruct = shallow_copy_constructor;
        destruct = shallow_destructor;
        dconstruct = shallow_default_constructor;
    } 

    new_vector->copy_constructor = cconstruct;
    new_vector->destructor = destruct;
    new_vector->default_constructor = dconstruct;


    new_vector->array = malloc(INITIAL_CAPACITY * sizeof(void *)); //establish initial space for array
    new_vector->size = 0; //array contains 0 elems
    new_vector->capacity = INITIAL_CAPACITY;
    return new_vector;
}

void vector_destroy(vector *this) {
    assert(this);

    size_t idx = 0;
    for (; idx < this->size; idx++) {
        if (this->array[idx]) {
            this->destructor(this->array[idx]);
        }
        
    }

    free(this->array);
    free(this);
    
}

void **vector_begin(vector *this) {
    assert(this);
    if (this->size == 0) {
        return NULL;
    }

    return this->array + 0;
}

void **vector_end(vector *this) {
    assert(this);
    return this->array + this->size;
}

size_t vector_size(vector *this) {
    assert(this);
    return this->size;
}

/**
 * checks n relative to vector capacity for resize method
 * 0 = n < vector->size
 * 1 = n > vector->size
 * 2 = n > vector->capacity
 * 
 **/

int resize_method(const vector * this, size_t n) {
    if (n < this->size) return 0;
    if (n > this->size && n <= this->capacity) return 1;
    return 2;
}


/**Resizes if method == 0
 */

void resize_smaller(vector * this, size_t n) {

    size_t idx = n;
    for(; idx < this->size; idx++) {
        //ptr to elem
        void * elem_ptr = this->array[idx];
        if (elem_ptr) {
            this->destructor(elem_ptr);
            elem_ptr = NULL;
        }

        this->array[idx] = NULL;
        
    }

    this->size = n;
}

//resizes if method == 1
void resize_within_capacity(vector * this, size_t n) {
    size_t idx = this->size;

    for(; idx < n; idx++) {
        this->array[idx] = this->default_constructor();
    }

    this->size = n;
}

void resize_upgrade_capacity(vector * this, size_t n) {
    size_t new_capacity = get_new_capacity(n);
    this->array = realloc(this->array, (new_capacity * sizeof(void *)));
    this->capacity = new_capacity;

    resize_within_capacity(this, n);

}

void vector_resize(vector *this, size_t n) {
    assert(this);
    int method = resize_method(this, n);
    //printf("value is %d\n", *elem_ptr);
    //printf("resize method is %d\n", method);
    switch (method) {
        case 0:
            resize_smaller(this, n);
            break;
        case 1:
            resize_within_capacity(this, n);
            break;
        case 2:
            resize_upgrade_capacity(this, n);
    }

}

size_t vector_capacity(vector *this) {
    assert(this);
    return this->capacity;
}

bool vector_empty(vector *this) {
    assert(this);
    return this->size == 0;
}

void vector_reserve(vector *this, size_t n) {
    assert(this);
    //no action needed if the vector capacity is sufficient
    if (n <= this->capacity) {
        return;
    }

    //resize vector otherwise
    resize_upgrade_capacity(this, n);
}

void **vector_at(vector *this, size_t position) {
    assert(this);
    assert(position < this->size);
    assert(position >= 0);

    return this->array + position;
}

void vector_set(vector *this, size_t position, void *element) {
    assert(this);
    assert(position >= 0 && position < this->size);
    //assert(position >= 0);
    void * existing_elem = this->array[position];
    if (existing_elem) {
        this->destructor(existing_elem);
        existing_elem = NULL;
    }

    this->array[position] = this->copy_constructor(element);
}

void *vector_get(vector *this, size_t position) {
    assert(this);
    assert(position < this->size);
    assert(position >= 0);
    return this->array[position];
}

void **vector_front(vector *this) {
    assert(this);
    assert(this->size > 0);
    return this->array + 0;
}

void **vector_back(vector *this) {
    assert(this);
    assert(this->size > 0);
    return this->array + (this->size - 1);
    
}

void vector_push_back(vector *this, void *element) {
    assert(this);
    //TODO update this to account for resizing!!
    if (this->size + 1 > this->capacity) {
        vector_resize(this, (this->size + 1));
    } else {
        this->size++;
    }
    this->array[this->size - 1] = this->copy_constructor(element);
}

void vector_pop_back(vector *this) {
    assert(this);
    assert(this->size > 0); 
    void * last_element = this->array[this->size - 1];
    if (last_element) {
        this->destructor(last_element);
        last_element = NULL;
    }
    
    this->size--;
}

void vector_insert(vector *this, size_t position, void *element) {
    assert(this);
    assert(position >= 0 && position < this->size);

    if ((this->size + 1) > this->capacity) {
        vector_resize(this, this->size + 1);
    } else {
        this->size++;
    }
    //assume that vector now has enough capacity for +1 element
    //copy all values 
    size_t idx = this->size - 1;
    for(; idx > position; idx--) {
        void * current = this->array[idx - 1];
        this->array[idx] = current;
    }
    
    this->array[position] = this->copy_constructor(element);
   
    
    // your code here
}

void vector_erase(vector *this, size_t position) {
    assert(this);
    assert(position < this->size);
    assert(position >= 0);

    size_t idx = position;
    void * target = this->array[position];
    if(target) {
        this->destructor(target);
        target = NULL;
    }
    for(; idx < this->size - 1; idx++) { 
        void * current = this->array[idx + 1];
        this->array[idx] = current;
    }
    this->size--;
    // your code here
}

void vector_clear(vector *this) {
    size_t idx = 0;
    for(; idx < this->size; idx++) {
        void * current = this->array[idx];
        if (current) {
            this->destructor(current);
        }

        this->array[idx] = NULL;
    }

    this->size = 0;

}

// The following is code generated:
vector *shallow_vector_create() {
    return vector_create(shallow_copy_constructor, shallow_destructor,
                         shallow_default_constructor);
}
vector *string_vector_create() {
    return vector_create(string_copy_constructor, string_destructor,
                         string_default_constructor);
}
vector *char_vector_create() {
    return vector_create(char_copy_constructor, char_destructor,
                         char_default_constructor);
}
vector *double_vector_create() {
    return vector_create(double_copy_constructor, double_destructor,
                         double_default_constructor);
}
vector *float_vector_create() {
    return vector_create(float_copy_constructor, float_destructor,
                         float_default_constructor);
}
vector *int_vector_create() {
    return vector_create(int_copy_constructor, int_destructor,
                         int_default_constructor);
}
vector *long_vector_create() {
    return vector_create(long_copy_constructor, long_destructor,
                         long_default_constructor);
}
vector *short_vector_create() {
    return vector_create(short_copy_constructor, short_destructor,
                         short_default_constructor);
}
vector *unsigned_char_vector_create() {
    return vector_create(unsigned_char_copy_constructor,
                         unsigned_char_destructor,
                         unsigned_char_default_constructor);
}
vector *unsigned_int_vector_create() {
    return vector_create(unsigned_int_copy_constructor, unsigned_int_destructor,
                         unsigned_int_default_constructor);
}
vector *unsigned_long_vector_create() {
    return vector_create(unsigned_long_copy_constructor,
                         unsigned_long_destructor,
                         unsigned_long_default_constructor);
}
vector *unsigned_short_vector_create() {
    return vector_create(unsigned_short_copy_constructor,
                         unsigned_short_destructor,
                         unsigned_short_default_constructor);
}
