#include <stdio.h>
#include <stdlib.h>
#include <stdint-gcc.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

/// NOTE: I am not a C programmer, so this will likely make you barf if you are.

// There are no real builtin libraries in C.
// So I'm literally going to write everything I need in this one file.
// Prepare for tl;dr, and very awful C-code.
// Actual entrypoint at the bottom.
//
//
// Normally, I would split this up into multiple files, but I want to keep this repository as "clean" as possible-
//		only one code file per language.
// So naturally, since we have to define everything in C, that means there's a bit of work to do to create 'modern' data structures...
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BLOB: Kinda Constants

// #define DEBUG 2
// #define DEBUG 1
// #define DEBUG 0

const char* _NULLSTR = "<NULL>";
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BLOB: Macros

// Macro that gets the length of an array.
#define ARRAY_LENGTH(A) ((sizeof(A) / sizeof((A)[0])))
/// Swap as a macro. This is a multi-line macro,
/// and enclosed in its own scope to not clobber or conflict with other variables defined around.
///		it is 'generic' in the sense that it could be used to swap two locations in any array,
///		as the TYPE is a parameter.
#define SWAP(TYPE, ARR, IA, IB) {\
	{\
		int32_t _ia = (IA);\
		int32_t _ib = (IB);\
		TYPE _TEMP = ARR[_ia]; ARR[_ia] = ARR[_ib]; ARR[_ib] = _TEMP;\
	}\
}

/// This is kinda disgusting, but should work to copy longer structs that can't simply be array-swapped, due to void*s in use.
/// Carefully created to hopefully not conflict with existing names,
/// As well as to only execute given expressions a single time
/// SIZE - size_t of elements stored from PTR
/// PTR - Base memory location of swap, better be void*
/// IA - Index of first element
/// IB - Index of second element
#define LONGSWAP(SIZE, PTR, IA, IB) {\
	{\
		size_t __size = (SIZE);\
		void* __ptr = (PTR);\
		void* __ia = (__ptr) + (IA) * __size;\
		void* __ib = (__ptr) + (IB) * __size;\
		if (__ia != __ib) { \
			char __junk[__size];\
			memcpy(__junk, __ia, __size);\
			memcpy(__ia, __ib, __size);\
			memcpy(__ib, __junk, __size);\
		}\
	}\
}

#define IS_STRUCT(EXPR) __is_class(typeof(EXPR))


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BLOB: 'Small' stuff

/// Get a standardized timestamp
/// Best we can get that is nearly 100% portable...
/// Requires the #include <sys/time.h>
int64_t microtime_() {
	struct timeval t;
	gettimeofday(&t, NULL);
	int64_t time = t.tv_sec;
	time *= 1000000LL; // seconds -> microseconds
	time += t.tv_usec; // unfortunately not nanos ;.;
	return time;
}

/// Typedef for comparison function used in sorting
typedef int32_t (*Compare)(void*, void*);

/// partition helper for qqsort
int32_t qqpartition(void* arr, size_t size, Compare compare, int32_t left, int32_t right, int32_t pivot) {
	LONGSWAP(size, arr, left, pivot);
	void* p = arr + (left * size);
	int32_t s = left;
	
	for (int32_t i = left+1; i <= right; i += 1) {
		void* ip = arr + (i * size);
		int32_t comp = compare(ip, p);
		if (comp < 0) {
			s = s + 1;
			LONGSWAP(size, arr, s, i);
		}
		
	}
	LONGSWAP(size, arr, left, s);
	
	//printf("\t\tQQPARTITION @ 0x%x on %d bytes from %d to %d, pivoting %d -> %d\n", arr, size, left, right, pivot, s);
	return s;
}

/// stdlib already has a qsort, but we just write our own because we're cool like that.
void qqsort(void* arr, size_t size, Compare compare, int32_t left, int32_t right) {
	//printf("\t\tQQSORT @ 0x%x on %d bytes from %d to %d\n", arr, size, left, right);
	int32_t span = right-left;
	if (span < 1) { return; }
	if (span < 2) {
		void* lp = arr + (left*size);
		void* rp = arr + (right*size);
		int32_t comp = compare(lp, rp);
		if (comp > 0) {
			LONGSWAP(size, arr, left, right);
		}
		return;
	}
	// Picking middle value is easy and unlikely to degenerate to worst case on sorted data.
	int32_t pivot = (left + right) / 2;
	int32_t part = qqpartition(arr, size, compare, left, right, pivot);
	qqsort(arr, size, compare, left, part-1);
	qqsort(arr, size, compare, part+1, right);
}

/// Fast hasher based on polynomial crunching
/// Expects a char** str, but uses void* to fit into "generic" interface
int32_t strHash(void* str) {
	const char* cptr = *(const char**)str;
	// Start hash
	int32_t hash = 0;
	// cache length
	int32_t len = strlen(cptr);
	// Iterate string, sum, multiplying by 31 for each extra character
	for (int32_t i = 0; i < len; i++) {
		const char c = *(cptr + i);
		hash = (hash * 31) + c;
	}
	
	return hash;
}

/// Helper function to compare two char** (aka string*) with strcmp
int32_t strCompare(void* a, void* b) {
	return (int32_t)strcmp(*(const char**)a, *(const char**)b);
}

/// Home-grown string split function
/// Passes over a string, and removes any characters in a delim string.
/// Creates a new array of duplicates of all split strings within the region.
/// Writes out the number of strings it created to the int32_t* outLength
char** strSplit(char* str, const char* delim, int32_t* outLength) { 
	// Get lengths that won't change
	int32_t length = strlen(str);
	int32_t delims = strlen(delim);
	
	// Count the number of splits we make
	int32_t splits = 0;
	// Count the current state of the state machine
	int32_t state = 0;

	// Loop back over original string, count splits
	for (int32_t i = 0; i < length; i++) {
		for (int32_t k = 0; k < delims; k++){
			// also making any delimited characters nulls before doing so
			// because we might as well go straight to hell
			// thought strtok would do this but it doesn't so f it.
			if (str[i] == delim[k]) { str[i] = '\0'; }
		}
		// Tiny state machine to remember 
		// if we are in a sequence of nulls 
		// or valid chars string
		if (state == 0) {
			// Searching for non-nulls.
			if (str[i] != '\0') { 
				splits++; 
				state = 1; 
			}
		} else if (state == 1) {
			// Searching for nulls
			if (str[i] == '\0') { state = 0; }
		}
	}
	
	// Create space for split strings
	char** result = malloc(splits * sizeof(char*));
	state = 0;	
	int32_t index = 0;
	// Loop back over original string, inserting splits
	for (int32_t i = 0; i < length; i++) {
		if (state == 0) {
			// Searching for non-nulls.
			if (str[i] != '\0') { 
				result[index] = strdup(str + i); // Duplicate 
				index++;
				state = 1; 
			}
		} else if (state == 1) {
			// Searching for nulls
			if (str[i] == '\0') { state = 0; }
		}
	}
	

	*outLength = splits;
	return result;	
}

/// Properly frees the data created by strSplit
void freeSplits(char** splits, int32_t length) {
	for (int32_t i = 0; i < length; i++) { free(splits[i]); }
	free(splits);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BLOB: bitfields
// Dang, didn't actually make use of this one... maybe...

// Get a bit from a bitfield pointer
// returns -1 if index is negative
// returns -2 if bitfield is null
// returns 1 if the bit is set
// returns 0 if the bit is not set
int32_t getBit(uint32_t* bitfield, int32_t index) {
	if (index < 0) { return -1; }
	if (bitfield == NULL) { return -2; }
	
	const uint32_t* ptr = bitfield + (index/32);
	const int32_t pos = index % 32;
	
	uint32_t data = *ptr;
	const uint32_t mask = 1 << pos;
	data &= mask;
	if (data > 0) { return 1; }
	
	return 0;
}
// Set a bit in a bitfield pointer
// returns -1 if index is negative
// returns -2 if bitfield is null
// returns -3 if invalid value was passed in as val (must be 0 or 1)
// returns 0 if successful
int32_t setBit(uint32_t* bitfield, int32_t index, int32_t val) {
	if (index < 0) { return -1; }
	if (bitfield == NULL) { return -2; }
	if (val < 0 || val > 1) { return -3; }
	
	uint32_t* ptr = bitfield + (index/32);
	const int32_t pos = index % 32;
	
	uint32_t data = *ptr;
	const uint32_t mask = 1 << pos;
	if (val == 0) {
		data &= ~mask;
	} else if (val == 1) {
		data |= mask;
	}
	
	*ptr = data;
	return 0;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BLOB: List

///"""""GENERIC""""" array data struct
///Should be endian-independent
typedef struct List {
	///Size of an element in the given array
	size_t size;
	///Total capacity allocated for the current array
	int32_t capacity;
	///Current count of elements in the current array
	int32_t count;
	///Internal Helper to track versioning changes.
	int32_t version;
	///Data Array pointer
	void* data;
} List;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LIST:PROTOTYPES:

///Grabs an element from a List, at index.
///If given invalid parameters, returns a NULL (NULL list or out of range index)
///Returns the element in the list at position index
///Params:
/// this - List to get from
/// index - position in list to get item from
void* ListGet(List* this, int32_t index);

///Ensures a list has at least a certain amount of capacity.
///Will only increase the capacity of a list. Will not decrease capacity.
///Memory of increased capacity will be memset to 0 as uint8_t
///Returns:
/// 0 on success
/// -1 if given any invalid parameters
/// -2 if given capacity is smaller than current capacity
/// -3 if realloc fails
///Params:
/// this - List to operate on
/// capacity - capacity to ensure.
int32_t ListEnsureCapacity(List* this, int32_t capacity);

///Adds a single element to a List.
///Adds the element by
///Returns:
/// if successful, the new count of the List
/// if unsuccessful, -1
///Params:
/// this - List to add element to
/// elem - pointer to data to add to the list
int32_t ListAdd(List* this, void* elem);

///Removes an element at the given index from the list.
///Removes the element, and memcpys the remaining ones over.
///Returns:
/// if successful, 0
/// -1 if index is invalid
///Params:
/// this - List to remove from
/// index - index to remove at 
int32_t ListRemove(List* this, int32_t index);

///Adds num things to the list starting at item
///Returns:
/// if successful, the new count of the List
/// if unsuccessful, -1
///Params:
///	this - List to add to
/// item - pointer to first element to add
/// num - number of elements to add to list
int32_t ListAddMany(List* this, void* item, int32_t num);

///Clears content of list
///ATCHUNG: Be sure if you are storing pointers to stuff 
///		within the list, you free each thing within the list before calling this method
///		This is a 'dumb' clear which just frees List's data and then the List itself.
///Params:
///	this - List to clear
void ListClear(List* this);

///Copies content of List* a into List* b.
///ATCHUNG: Be sure if you are storing pointers to stuff 
///		within the list, you free each thing within the destination list before calling this method
///		This is a 'dumb' copy
///Returns:
///	number of copied elements (may be 0 when out of memory)
/// or
/// 0 - nothing copied
/// -1 - lists have different data sizes, and copy is refused.
///Params:
///	a - source list
///	b - destination list
int32_t ListCopy(List* a, List* b);

// See if the list contains the target thing via the given compare function.
int32_t ListContains(List* this, void* target, Compare compare);

///Creates a new List object with a given size and capacity.
///Returns the pointer to the newly created list data.
///New list has members:
/// size of the given size
/// capacity of the list equal to initCapacity
/// count of 0
/// newly allocated array, memset contain all zeros.
///If allocation fails, returns a null.
///Params:
/// size - size of a single element in the list (in uint8_ts)
/// initialCapacity - number of initial slots for data elements in allocated array.
List* newList_cap(size_t size, int32_t initCapacity);

///Creates a new List object with a given size and capacity of 16.
///Returns the pointer to the newly created list data
///Equivelent to the call newList(size, 16);
///If allocation fails, returns a null.
///Params:
/// size - size of a single element in the new list (in uint8_ts)
List* newList(size_t size);

///deconstructs and frees memory of a list object.
///ATCHUNG: Be sure if you are storing pointers to stuff  
///		within the list, you free each thing within the list before calling this method
///		This is a 'dumb' clear which just frees List's data and then the List itself.
///Params:
/// this - List to kill
void killList(List* this);

// Debug printing list. Prints data in the list struct.
/// this - List to print
void printList(List* this);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LIST:CONTENT:


///Grabs an element from a List, at index.
///If given invalid parameters, returns a NULL (NULL list or out of range index)
///Returns the element in the list at position index
///Params:
/// this - List to get from
/// index - position in list to get item from
void* ListGet(List* this, int32_t index) {
	if (!this) { return NULL; }
	if (index < 0 || index >= (this->count)) { return NULL; }
	
	return this->data + index * this->size;
}

///Ensures a list has at least a certain amount of capacity.
///Will only increase the capacity of a list. Will not decrease capacity.
///Memory of increased capacity will be memset to 0 as uint8_t
///Returns:
/// 0 on success
/// -1 if given any invalid parameters
/// -2 if given capacity is smaller than current capacity
/// -3 if realloc fails
///Params:
/// this - List to operate on
/// capacity - capacity to ensure.
int32_t ListEnsureCapacity(List* this, int32_t capacity) {
	if (capacity < 0 || this == NULL) { return -1;}
	if (capacity <= this->capacity) { return -2; }

	//New total length (in uint8_t's)
	const int32_t length = capacity * this->size;
	//Old total length (in uint8_t's)
	const int32_t oldLength = this->capacity * this->size;
	//Additional length (in uint8_t's)
	const int32_t addLength = length - oldLength;

	void* array = realloc(this->data, length);
	if (array != NULL) {
		this->data = array;
		this->capacity = capacity;
		void* oldEnd = array + oldLength;
		memset(oldEnd, 0, addLength);
		return 0;
	}
	return -3;
}

///Adds a single element to a List.
///Adds the element by
///Returns:
/// if successful, the new count of the List
/// if unsuccessful, -1
///Params:
/// this - List to add element to
/// elem - pointer to data to add to the list
int32_t ListAdd(List* this, void* elem) {
	if (this->capacity == this->count) {
		#ifdef DEBUG
		#if DEBUG > 1
			printf("\tListAdd: Resizing capacity from %d to %d\n", this->capacity, 2 * this->capacity);
		#endif // DEBUG > 0
		#endif // DEBUG
		if (ListEnsureCapacity(this, (this->capacity)*2) < 0) { return 0; }
	}
	// just faster to memcpy...
	memcpy(this->data + (this->size * this->count), elem, this->size);
	this->count+=1;
	return this->count;
}

///Removes an element at the given index from the list.
///Removes the element, and memcpys the remaining ones over.
///Returns:
/// if successful, 0
/// -1 if index is invalid
///Params:
/// this - List to remove from
/// index - index to remove at 
int32_t ListRemove(List* this, int32_t index) {
	if (this == NULL) { return -1; }
	if (index < 0) { return -1; }
	int32_t count = this->count;
	if (index >= count) { return -1; }
	
	size_t size = this->size;
	void* data = this->data;
	this->count--;
	count = this->count;
	if (index < count) {
		void* to = data + (size * index);
		void* from = to + size;
		// just faster to memmove...
		memmove(to, from, size * (count - index));
	}
	memset(data + size * count, 0, size);
	
	return 0;
}

///Adds num things to the list starting at item
///Returns:
/// if successful, the new count of the List
/// if unsuccessful, -1
///Params:
///	this - List to add to
/// item - pointer to first element to add
/// num - number of elements to add to list
int32_t ListAddMany(List* this, void* item, int32_t num) {
	const int32_t newCount = this->count + num;
	if (newCount > this->capacity) {
		int32_t err = ListEnsureCapacity(this, newCount);
		if (err) { return -1; }
	}
	// just faster to memcpy...
	memcpy(this->data + (this->size * this->count), item, this->size * num);
	this->count += num;
	return this->count;
}

///Clears content of list
///ATCHUNG: Be sure if you are storing pointers to stuff 
///		within the list, you free each thing within the list before calling this method
///		This is a 'dumb' clear which just frees List's data and then the List itself.
///Params:
///	this - List to clear
void ListClear(List* this) {
	memset(this->data, 0, this->size * this->count);
	this->count = 0;
}

int32_t ListContains(List* this, void* target, Compare compare) {
	int32_t count = this->count;
	
	for (int32_t i = 0; i < count; i++) {
		void* data = ListGet(this, i);
		int32_t compared = compare(target, data);
		if (compared == 0) {
			return 1;
		}	
	}
	
	return 0;
}

void ListSort(List* this, Compare compare) {
	qqsort(this->data, this->size, compare, 0, this->count-1);
}

///Copies content of List* a into List* b.
///ATCHUNG: Be sure if you are storing pointers to stuff 
///		within the list, you free each thing within the destination list before calling this method
///		This is a 'dumb' clear which just frees List's data and then the List itself.
///Returns:
///	number of copied elements (may be 0 when out of memory)
/// or
/// 0 - nothing copied
/// -1 - lists have different data sizes, and copy is refused.
///Params:
///	a - source list
///	b - destination list
int32_t ListCopy(List* a, List* b) {
	if (a->size != b->size) { return -1; }
	
#ifdef DEBUG
#if DEBUG > 0
	printf("\t\tListCopy: Copying from list with %i/%i elements into list with %i/%i elements.\n",
		a->count, a->capacity,
		b->count, b->capacity
	);
#endif
#endif
	
	if (b->capacity < a->count) {
#ifdef DEBUG
#if DEBUG > 0
		printf("\t\tListCopy: Resizing capacity from %i to %i\n", b->capacity, a->count);
#endif
#endif
		int32_t err = ListEnsureCapacity(b, a->count);
	
#ifdef DEBUG
#if DEBUG > 0
		printf("\t\tListCopy: Error? %i\n", err);
		printList(a);
		printList(b);
#endif
#endif
		if (err) { return 0; }
	}
	
#ifdef DEBUG
#if DEBUG > 0
	printf("\t\tListCopy: Doing memcopy(0x%x, 0x%x, %i);\n", (int32_t)b->data, (int32_t)a->data, a->size*a->capacity);
#endif
#endif
	memcpy(b->data, a->data, a->size*a->count);
	
#ifdef DEBUG
#if DEBUG > 0
	printf("\t\tListCopy: Setting b->count from %i to %i\n", b->count, a->count);
#endif
#endif
	b->count = a->count;
	return a->count;
}

///Creates a new List object with a given size and capacity.
///Returns the pointer to the newly created list data.
///New list has members:
/// size of the given size
/// capacity of the list equal to initCapacity
/// count of 0
/// newly allocated array, memset contain all zeros.
///If allocation fails, returns a null.
///Params:
/// size - size of a single element in the list (in uint8_ts)
/// initialCapacity - number of initial slots for data elements in allocated array.
List* newList_cap(size_t size, int32_t initCapacity) {
	if (initCapacity <= 0 || size == 0) { return NULL; }

	List* list = malloc(sizeof(List));
	void* data = malloc(size * initCapacity);
	if (!data || !list) {
		free(data);
		free(list);
		return NULL;
	}
	memset(data, 0, size * initCapacity);

	list->size = size;
	list->capacity = initCapacity;
	list->count = 0;
	list->data = data;

	return list;
}

///Creates a new List object with a given size and capacity of 4.
///Returns the pointer to the newly created list data
///Equivelent to the call newList(size, 16);
///If allocation fails, returns a null.
///Params:
/// size - size of a single element in the new list (in uint8_ts)
List* newList(size_t size) { return newList_cap(size, 4); }

///deconstructs and frees memory of a list object.
///ATCHUNG: Be sure if you are storing pointers to stuff 
///		within the list, you free each thing within the list before calling this method
///		This is a 'dumb' clear which just frees List's data and then the List itself.
///Params:
/// this - List to kill
void killList(List* this) {
	free(this->data);
	free(this);
}

// Debug printing list. Prints data in the list struct.
/// this - List to print
void printList(List* this) {
	printf("List 0x%x -> { size: %i, capacity: %i, count: %i, data: 0x%x }\n",
		(int32_t)this,
		this->size,
		this->capacity,
		this->count,
		(int32_t)this->data
	);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BLOB: Map
/// Simple map implementation 

/// Typedef for hash function.
/// Used to hash keys into int32s 
/// It is assumed the implementer knows the correct process and size of whatever is being used as keys
/// When they construct their hashmap.
typedef int32_t (*HashFn)(void*);
/// Typedef for compare function.
/// Used to decide if two keys are the same or different
/// It is assumed the implmenter knows the correct process and size of whatever is being used as keys
typedef int32_t (*CompareFn)(void*, void*);

/// Kinda generic map type.
/// Preferred to use value types, but it would be possible (but unadvisable) to use pointers.
typedef struct Map {
	size_t keySize;
	size_t valSize;
	HashFn hashFn;
	CompareFn compareFn;
	int32_t count;
	int32_t capacity;
	List* pairs;
	List** buckets;
	// Scratch data, size is (keySize + valSize);
	void* scratch;
} Map;

/// Typedef for a single kvp to be easily passed around inside Map functions
/// They will refer to the same, contiguous memory, be it in direct KVPs
typedef struct KVP { void* key; void* val; } KVP;

Map* newMap_cap(int32_t initCapacity, size_t keySize, size_t valSize, HashFn hashFn, CompareFn compareFn) {
	if (initCapacity <= 0 || keySize == 0 || valSize == 0 || hashFn == NULL || compareFn == NULL) { return NULL; }
	
	Map* map = malloc(sizeof(Map));
	List* pairs = newList(keySize + valSize);
	List** buckets = malloc(sizeof(List*) * initCapacity);
	void* scratch = malloc(keySize + valSize);
	if (!map || !pairs || !buckets || !scratch) {
		free(map);
		free(pairs);
		free(buckets);
		free(scratch); 
		return NULL;
	}
	memset(buckets, 0, (sizeof(List*)) * initCapacity);
	
	map->keySize = keySize;
	map->valSize = valSize;
	map->hashFn = hashFn;
	map->compareFn = compareFn;
	map->count = 0;
	map->capacity = initCapacity;
	map->pairs = pairs;
	map->buckets = buckets;
	map->scratch = scratch;
	
	return map;
}

Map* newMap(size_t keySize, size_t valSize, HashFn hashFn, CompareFn compareFn) {
	return newMap_cap(131, keySize, valSize, hashFn, compareFn);
}

KVP _MapKVP(void* loc, size_t keySize) {
	KVP out;
	out.key = loc;
	out.val = loc + (keySize);
	return out;
}

void MapRehash(Map* this, int32_t multiplier) {
	if (multiplier < 1) { return; }
		
#ifdef DEBUG
#if DEBUG > 0
	if (multiplier > 1) {
		printf("\t----MapRehash: Debug: Resizing table from %i to %i\n", this->capacity, this->capacity*multiplier);
	} else {
		printf("\t----MapRehash: Debug: Rehashing table of size %i\n", this->capacity);
	}
#endif
#endif

	int32_t newCapacity = this->capacity * multiplier;
	List** next = malloc(newCapacity * sizeof(List*));
	if (!next) {
#ifdef DEBUG
#if DEBUG > 0
		printf("\t----MapRehash: Debug: Resizing table failed.");
#endif
#endif
		return;
	}
	List* pairs = this->pairs;
	HashFn hashFn = this->hashFn;
	memset(next, 0, (sizeof(List*)) * newCapacity);
	int32_t cnt = pairs->count;
	int32_t error = 0;
	for (int32_t i = 0; i < cnt; i++) {
		void* ptr = ListGet(pairs, i);
		KVP pair = _MapKVP(ptr, this->keySize);
		
		int32_t hash = hashFn(pair.key);
		if (hash < 0) { hash *= -1; }
		int32_t place = hash % newCapacity;
		List* bucket = next[place];
		
		if (bucket == NULL) { 
#ifdef DEBUG
#if DEBUG > 3
			printf("\t-----MapRehash: re-creating bucket %i\n", place);
#endif
#endif
			bucket = newList(sizeof(void*));
			if (bucket == NULL) { 
				
#ifdef DEBUG
#if DEBUG > 0
				printf("\t-----MapRehash: Out of memory when allocating new bucket.");
#endif
#endif
				error = 1; break;
			} else {
				next[place] = bucket;
			}
		}
		
		// Copy ptr into bucket.
		ListAdd(bucket, &ptr);
	}
		
	
	List** toFree;
	int32_t freeCnt;
	
	if (error) {
#ifdef DEBUG
#if DEBUG > 0
		printf("\t-----MapRehash: Failed, some error occurred when resizing, need to free() junk data.\n");
#endif
#endif
		toFree = next;
		freeCnt = newCapacity;
	} else {
#ifdef DEBUG
#if DEBUG > 0
		printf("\t-----MapRehash: Resize successful, need to free() old structures.\n");
#endif
#endif
		toFree = this->buckets;
		freeCnt = this->capacity;
		
		this->capacity = newCapacity;
		this->buckets = next;
	}
	
	for (int32_t i = 0; i < freeCnt; i++) {
		List* list = toFree[i];
		if (list != NULL) { 
			
#ifdef DEBUG
#if DEBUG > 3
			printf("\t------MapRehash: Killing list %i\n", i);
#endif
#endif
			killList(list); 
		}
	}
	
	free(toFree);
	
#ifdef DEBUG
#if DEBUG > 0
	printf("\t----MapRehash: Finished..\n");
#endif
#endif
}

int32_t MapPut(Map* this, void* key, void* val) {
	if (this == NULL) { return -1; } // Null map wtf.
	if (key == NULL) { return -2; } // No null keys allowed
	if (val == NULL) { return -3; } // No null vals allowed 
	// printf("MapPut: test %s -> %s\n", *(char**)key, *(char**)val);
	
	const size_t keySize = this->keySize; // Unpack everything, since we'll be using everything.
	const size_t valSize = this->valSize;
	const HashFn hashFn = this->hashFn;
	const CompareFn compareFn = this->compareFn;
	int32_t count = this->count;
	int32_t capacity = this->capacity;
	List* pairs = this->pairs;
	List** buckets = this->buckets;
	void* scratch = this->scratch;
	
	int32_t hash = hashFn(key);
	if (hash < 0) { hash *= -1; }
	int32_t place = hash % capacity;
	
#ifdef DEBUG
#if DEBUG > 1
	printf("\tMapPut: Key hashed to %i, bucket %i.\n", hash, place);
#endif
#endif
	List* bucket = buckets[place];
	if (bucket == NULL) { 
		bucket = newList(sizeof(void*));
#ifdef DEBUG
#if DEBUG > 1
		printf("\tMapPut: Creating new bucket\n");
#endif
#endif
		
		if (bucket == NULL) {
			
#ifdef DEBUG
#if DEBUG > 1
			printf("\tMapPut: Out of memory when allocating new bucket.\n");
#endif
#endif
			// panic, out of memory when allocating new bucket.
			return -4;
		} else {
			buckets[place] = bucket;
		}
	} else {
#ifdef DEBUG
#if DEBUG > 1
		printf("\tMapPut: Bucket already exists!!!!\n");
#endif
#endif
	}
	
	// Try to find existing key in map
	int32_t cnt = bucket->count;
	for (int32_t i = 0; i < cnt; i++) {
		void* ptr = ListGet(bucket, i);
		KVP pair = _MapKVP(ptr, keySize);
		int32_t compared = compareFn(pair.key, key);
		
		if (compared == 0) {
#ifdef DEBUG
#if DEBUG > 0
			printf("\tMapPut: Updating existing key/value pair\n");
#endif
#endif
			// If we have a match, copy the value over
			memcpy(pair.val, val, valSize);	
			// And we're done here.
			return 0;
		}
	}
	
	// If we're here, we don't already have the key.
	// Put key/val data into map scratch area
	memcpy(scratch, key, keySize);
	memcpy(scratch + keySize, val, valSize);
	
	// Add copy of scratch to pairs list
	
#ifdef DEBUG
#if DEBUG > 0
	printf("\tMapPut: Adding new key/value pair\n");
#endif
#endif
	int32_t lastCapacity = pairs->capacity;
	ListAdd(pairs, scratch);
	// If the list resized, we unfortunately need to rehash our buckets.
	if (pairs->capacity != lastCapacity) { 
		MapRehash(this, 1);
		bucket = this->buckets[place];
	}
	// Increment count
	
	this->count++;
	void* kvp = ListGet(pairs, pairs->count-1);
	
	
	
	ListAdd(bucket, &kvp);
	// Copy kvp into bucket
	
	// Check if we need to resize and rehash
	if (bucket->count > 10) { MapRehash(this, 3); }
	return 0;
}

void* MapGet(Map* this, void* key) {
	if (this == NULL) { return NULL; }
	if (key == NULL) { return NULL; }
	const size_t keySize = this->keySize;
	const size_t valSize = this->valSize;
	const HashFn hashFn = this->hashFn;
	const CompareFn compareFn = this->compareFn;
	int32_t count = this->count;
	int32_t capacity = this->capacity;
	List* pairs = this->pairs;
	List** buckets = this->buckets;
	void* scratch = this->scratch;
	
	
	int32_t hash = hashFn(key);
	if (hash < 0) { hash *= -1; }
	int32_t place = hash % capacity;
#ifdef DEBUG
#if DEBUG > 0
	printf("\t--MapGet: Key hashed to %i, bucket %i.\n", hash, place);
#endif
#endif
	List* bucket = buckets[place];
	if (bucket == NULL) { 
#ifdef DEBUG
#if DEBUG > 0
		printf("\t--MapGet: No bucket, you get a NULL.\n");
#endif
#endif
		return NULL; 
	}
	
#ifdef DEBUG
#if DEBUG > 0
	printf("\t--MapGet: Got the bucket, looking for the thingy.\n");
#endif
#endif
	int32_t cnt = bucket->count;
	for (int32_t i = 0; i < cnt; i++) {
#ifdef DEBUG
#if DEBUG > 0
		printf("\t--MapGet: Checking bucket %i[%i].\n", place, i);
#endif
#endif
		// List stores pointers to kvp's in the listing.
		void* ptr = *(void**)ListGet(bucket, i);
		
		KVP pair = _MapKVP(ptr, keySize);
		// char** kk = pair.key;
		// char** vv = pair.val;
		// printf("MapGet: raw grab: %s -> %s\n", *kk, *vv);
		
		int32_t compared = compareFn(pair.key, key);
#ifdef DEBUG
#if DEBUG > 0
		printf("\t--MapGet: Keys compared to %i.\n", compared);
#endif
#endif
		if (compared == 0) {
			return pair.val;
		}
	}
	
#ifdef DEBUG
#if DEBUG > 0
	printf("\t--MapGet: Got the bucket, Key not found, have a NULL.\n");
#endif
#endif
	return NULL;
}

/// Helper map for interning strings
Map* interned;
/// Helper function to intern and check strings 
char* intern(char* str) {
	if (str == NULL) { return NULL; }
	
	char** check = MapGet(interned, &str);
	if (check == NULL) {
		char* copy = strdup(str);
		MapPut(interned, &copy, &copy);
		return copy;
	}
	return *check;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ENDBLOBS:
/// Stuff for the actual program??? MAYBE???
int32_t compareInt32s(void* a, void* b) {
	int32_t av = *(int32_t*)a;
	int32_t bv = *(int32_t*)b;
	return av - bv;
}

typedef struct Vector4 {
	float x,y,z,w;
} Vector4;
Vector4 V4(float a, float b, float c, float d){
	Vector4 v; v.x = a; v.y = b; v.z = c; v.w = d; return v;
}
int32_t compareV4ByZ(void* a, void* b) {
	Vector4* av = (Vector4*)a;
	Vector4* bv = (Vector4*)b;
	return ((int32_t)av->z)-((int32_t)bv->z);
}

void testGarbage() {
	printf("hello world\n");
	
	{ // Sorting numbers
		int32_t crap[] = { 5, 3, 7, 6, 0, 9, 1, 8, 4, 2 };
		printf("sorting %d stuffs\n\n", ARRAY_LENGTH(crap));
		
		qqsort((void*)crap, sizeof(int32_t), compareInt32s, 0, ARRAY_LENGTH(crap)-1);
		
		printf("I think I did it!\n");
		for (int32_t i = 0; i < ARRAY_LENGTH(crap); i++) {
			printf("%d, ", crap[i]);
		}
	}
	
	{ // Sorting other stuff...
		Vector4 vs[] = {
			V4(1,2,3,4),
			V4(2,3,4,1),
			V4(3,4,1,2),
			V4(4,1,2,3),
		};
		printf("sorting %d stuffs\n\n", ARRAY_LENGTH(vs));
		qqsort((void*)vs, sizeof(Vector4), compareV4ByZ, 0, ARRAY_LENGTH(vs)-1);
		for (int32_t i = 0; i < ARRAY_LENGTH(vs); i++) {
			Vector4 v = vs[i];
			printf("(%f, %f, %f, %f)\n", v.x, v.y, v.z, v.w);
		}
	}
		
		
	{ // Bitflags 
		uint32_t junk[] = { 0,0,0,0,0,0,0,0 };
		size_t junkLength = ARRAY_LENGTH(junk);
		size_t junkSize = junkLength * 32;
		
		setBit(junk, 1, 1);
		int32_t last = 1;
		for (int32_t i = 2; i < junkSize;) {
			setBit(junk, i, 1);
			int32_t temp = i;
			i = i + last;
			last = temp;
		}
		
		for (int32_t i = 0; i < junkLength; i++) {
			printf("%i: 0x%08x\n", i, junk[i]);
		}
		printf("\n\n");
		for (int32_t i = 0; i < junkSize; i+= 2) {
			setBit(junk, i, 0);
		}
		
		for (int32_t i = 0; i < junkLength; i++) {
			printf("%i: 0x%08x\n", i, junk[i]);
		}
		printf("\n\n");
	}
	{ // String splitting
		char* stuff = "O H  M   Y    G     O      D       I        A         M         O           N            F             I              R               E";
		char copy[256];
		strcpy(copy, stuff);
		
		int32_t length;
		char** split = strSplit(copy, " ", &length);
		
		for (int32_t i = 0; i < length; i++) {
			printf("[%s]", split[i]);
		}
		printf("\n");
		freeSplits(split, length);
	}
	
	{ // String interning
		char* ayy = intern("ayy");
		char* lmao = intern("lmao");
		char* yeet = intern("yeet");
		
		char garbage[50];
		strcpy(garbage, "ayy");
		
		char* check = intern(garbage);
		
		printf("Addresses are 0x%08x and 0x%08x", ayy, check);
	}
	
}

void testList() {
	{ // Cramming stuff into a list and removing some of it to make sure it works properly.
		List* strs = newList(sizeof(char**));
		char* strarr[] = { 
			"ayy", "bee", "cee", "dee", "aee", 
			"eff", "gee", "eich", "aye", "jay",
			"kay", "ell", "emm", "enn", "owh", 
			"pee", "queue", "arr", "ess", "tea",
			"you", "vee", "dubya", "ecks", "why", 
			"zeeee" };
			
		for (int32_t i = 0; i < ARRAY_LENGTH(strarr); i++) {
			char* str = strarr[i];
			ListAdd(strs, &str);
		}
		
		// ListAdd(strs, &a);
		// ListAdd(strs, &b);
		// ListAdd(strs, &c);
		printf("\n\n");
		
		for (int i = 0; i < strs->count; i++) {
			char** cs = (char**) ListGet(strs, i);
			printf("%i: %s\n", i, *cs);
		}
		
		int32_t result = ListRemove(strs, 0);
		printf("Remove result: %d\n", result);
		
		
		for (int i = 0; i < strs->count; i++) {
			char** cs = (char**) ListGet(strs, i);
			printf("%i: %s\n", i, *cs);
		}
		
		result = ListRemove(strs, strs->count-1);
		printf("Remove result2: %d\n", result);
		for (int i = 0; i < strs->count; i++) {
			char** cs = (char**) ListGet(strs, i);
			printf("%i: %s\n", i, *cs);
		}
		
		
		result = ListRemove(strs, 9);
		printf("Remove result3: %d\n", result);
		for (int i = 0; i < strs->count; i++) {
			char** cs = (char**) ListGet(strs, i);
			printf("%i: %s\n", i, *cs);
		}
		killList(strs);	
	}
		
	{ // Does a list make a good strbuilder? Lessee.
		List* strbuilder = newList(sizeof(char));
		char* a = "hello";
		char* b = "world";
		char* space = " ";
		char null = '\0';
		ListAddMany(strbuilder, a, strlen(a));
		ListAddMany(strbuilder, space, strlen(space));
		ListAddMany(strbuilder, a, strlen(a));
		ListAddMany(strbuilder, space, strlen(space));
		ListAddMany(strbuilder, b, strlen(b));
		ListAddMany(strbuilder, space, strlen(space));
		ListAddMany(strbuilder, b, strlen(b));
		ListAdd(strbuilder, &null);
		
		printf("built a string!: \"%s\"\n", strbuilder->data);
		
		killList(strbuilder);
	}
	
}


void testMap() {
	printf("We maps now\n\n");
	char* keyarr[] = { 
		"ayy", "bee", "cee", "dee", "aee", 
		"eff", "gee", "eich", "aye", "jay",
		"kay", "ell", "emm", "enn", "owh", 
		"pee", "queue", "arr", "ess", "tea",
		"you", "vee", "dubya", "ecks", "why", 
		"zeeee",
		"These keys do not exist", "awwwww",
		"bawwww", "yeeeret", "arjhskdjgsakd",
		"dsfjhjsdfh", "ajsdrfhsdfjh", "sajjsdfhyjds",
		
	};
	char* valarr[] = { 
		"ayaya", "b movie", "seasick", "dickbutt", "eeeeeeeeeeeeee", 
		"pay respects", "gee gee gee baby baby baby", "aeiou", "dunno lol", "J U S T",
		"k", "oh ell", "and emm", "ope", "oh baby a tripple", 
		"down my leg", "dequeue", "array", "ucc", "party",
		"can (not) x", "vidya gaems", "bush did 911", "ecks dee", "because", 
		"not zed" 
	};
		
	Map* map = newMap(sizeof(char*), sizeof(char*), strHash, strCompare);
	
	for (int32_t i = 0; i < ARRAY_LENGTH(keyarr) && i < ARRAY_LENGTH(valarr); i++) {
		char* key = keyarr[i];
		char* val = valarr[i];
		MapPut(map, &key, &val);
	}
	printf("MapPuts finished.\n\n");
	
	// Grab for debug purposes.
	for (int32_t i = 0; i < ARRAY_LENGTH(keyarr) && i < ARRAY_LENGTH(valarr); i++) {
		void* pos = ListGet(map->pairs, i);
		KVP pair = _MapKVP(pos, map->keySize);
		printf("Raw grab of pair: %s -> %s\n", *(char**)pair.key, *(char**)pair.val);
	}
	
	for (int32_t i = 0; i < ARRAY_LENGTH(keyarr); i++) {
		char* key = keyarr[i];
		// char* val = valarr[i];
		printf("MapGet'ing %s", key);
		char** gotten = MapGet(map, &key);
		if (gotten == NULL) { gotten = (char**)&_NULLSTR; }
		printf("\n\nLets see what we got for %s -> %s.\n", key, *gotten);
	}
	
	printf("done maps\n");
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////
// End testing, time for actual program stuff.

/// Struct for holding where we currently are in the graph.
typedef struct WorkData {
	/// Starting location
	char* start;
	/// Current location
	char* at;
	/// Current rate
	double rate;
	/// Current score
	double score;
	/// Path to get to current node 
	List* path;
} WorkData;

// Global data structures.
// Can't initialize them statically, has to be done in main.
/// Map<char*, Map<char*, double>> containing the graph nodes and transitions.
Map* graph;
/// List<WorkData> to act as a queue of work to do 
/// Probably a few ms could be shaved off the time if I implemented a proper linked-list queue.
List* queue;
/// List<WorkData> to be filled with profitable cycles as they are discovereed.
List* profitable;


/// Helper function to step a WorkData to a new place.
/// Returns a copy of data in the new location
/// (since C is always pass-by-value unless you explicitly pass pointers)
WorkData stepTo(WorkData data, char* place) {
	// Compare names of nodes to see if they are at the same place 
	int32_t diff = strcmp(data.at, place);
	// If it's not already there...
	if (diff != 0) {
		// Get Check the graph for transition from data.at to place
		Map** check1 = MapGet(graph, &data.at);
		if (check1 != NULL) { 
			double* check2 = MapGet(*check1, &place);
			if (check2 != NULL) { 
				// exchange rate extracted from graph
				double rate = *check2;
				
				// Create data to return 
				WorkData next;
				// Starts at same node
				next.start = data.start;
				// transitioned to new node
				next.at = place;
				// Apply exchange rate
				next.rate = data.rate * rate;
				// And score based on total trades
				next.score = (next.rate - 1.0) / data.path->count;
				// Create a new list with enough space for the path to the new node
				next.path = newList_cap(sizeof(char*), data.path->count+1);
				// Copy old path
				ListCopy(data.path, next.path);
				// Add new path on end
				ListAdd(next.path, &place);
				
				// Finally return transitioned data....
				return next;
			}
		}
	}
	// Or return copy of data if there's no transition to take
	return data;
}

/// Helper function to step a WorkData back to its starting location.
WorkData stepToStart(WorkData data) {
	return stepTo(data, data.start);
}

/// Compare function to sort WorkData by their rates
int32_t compareWorkByRateAscending(void* a, void* b) {
	// cast/dereference values we care about
	double aval = ((WorkData*)a)->rate;
	double bval = ((WorkData*)b)->rate;
	return aval < bval ? -1 
		: (aval > bval ? 1 : 0);
}
/// Compare function to reverse sort WorkData by their rates
int32_t compareWorkByRateDescending(void* a, void* b) {
	double aval = ((WorkData*)a)->rate;
	double bval = ((WorkData*)b)->rate;
	return aval < bval ? 1 
		: (aval > bval ? -1 : 0);
}
/// Compare function to reverse sort WorkData by their scores
int32_t compareWorkByScoreDescending(void* a, void* b) {
	double aval = ((WorkData*)a)->score;
	double bval = ((WorkData*)b)->score;
	return aval < bval ? 1 
		: (aval > bval ? -1 : 0);
}

/// Helper function to print a List<char*> to console or to a List<char>
void printPath(List* path, List* to) {
	// Extract count once and put on stack
	int32_t count = path->count;
	
	// No destination List<char>, we printf
	if (to == NULL) {
		// Beginning marker
		printf("[ ");
		// Iterate path
		for (int32_t i = 0; i < count; i++) {
			// Get next ptr
			char* str = *(char**)ListGet(path, i);
			// print it out 
			printf("%s ", str);
			// and an arrow if we're not last 
			if (i+1 < count) { printf("-> "); }
		}
		// Ending marker
		printf("]");
	} else {
		// Otherwise, print to List<char>
		// Add beginning chars
		ListAddMany(to, "[ ", 2);
		// oversized buffer for snprintf to use
		char buffer[1024];
		// Iterate path
		for (int32_t i = 0; i < count; i++) {
			// Get next ptr
			char* str = *(char**)ListGet(path, i);
			// snprintf (String Number Print Formatted) into buffer
			int32_t printed = snprintf(buffer, 1024, "%s ", str);
			// Then add characters to the List<char>
			ListAddMany(to, buffer, printed);
			// Also add arrows if we're not last.
			if (i+1 < count) {  ListAddMany(to, "-> ", 3);  }
		}
		// print ending and null to terminate string
		ListAddMany(to, "]\0", 2);
	}
}

/// Finally, entry point.
int main(char** argv) {
	// Initialize string interning Map<char*, char*>
	interned = newMap(sizeof(char*), sizeof(char*), strHash, strCompare);
	
	// Used these while I was developing the libraries to make sure stuff worked.
	// testGarbage();
	// testList();
	// testMap();
	
	
// } int restOfMain(char** argv) {

	//*
	// Define data structures.
	// Calling methods that act like constructors to create Maps and lists.
	// Map<char*, Map<char*, double>>
	graph = newMap(sizeof(char*), sizeof(Map*), strHash, strCompare);
	// List<WorkData>
	queue = newList(sizeof(WorkData));
	// List<WorkData>
	profitable = newList(sizeof(WorkData));
	
	// Open file for reading.
	FILE* fp = fopen("stuff.dat", "r");
	if (fp == NULL) { 
		printf("\n Error: Failed to open 'stuff.dat'\n\n"); 
		return -1; 
	}
	printf("\nOpened file 'stuff.dat'\n");
	// Oversized buffer.
	char line[256]; memset(line, 0, ARRAY_LENGTH(line)); // no cruft thanks
	
	int32_t lineNum = 0;
	while (fgets(line, ARRAY_LENGTH(line), fp) != NULL) {
		char* lineCpy = strdup(line); // dupe line for split (gets modified)
		// Output variable for how many parts we split
		int32_t parts;
		char** split = strSplit(lineCpy, "\t", &parts);
		
		// If we have exactly 3, must be an input line!
		if (parts == 3) {
			//printf("Line %i: %s", lineNum, line);
			
			// Interning strings is a cheap/easy way to make sure they don't get free'd.
			// (So long as you have a table you can store your strings in, easily check for duplicates,
			//		and return canonical copies...)
			// If a string has not been interned yet, I make a cannonical copy
			//		and stick it inside of a hashmap as both the key AND value.
			// If it already exists, I just return the canonical string from the hashmap.
			char* from = intern(split[0]);
			char* to = intern(split[1]);
			char* rateStr = split[2];
			//printf("[%s] => [%s] : %s", from, to, rateStr);
			// like Double.parseDouble in java, but no throw, just returns 0 on failure.
			double rate = atof(rateStr);
			if (rate > 0) {
				// Make sure both currencies have graphs
				Map* fromMap;
				Map* toMap;
				Map** check;
				
				// Check if the graph already has the nodes, and add them if they don't
				if ((check = MapGet(graph, &from)) == NULL) {
					// new Map<char*, double>
					fromMap = newMap(sizeof(char*), sizeof(double), strHash, strCompare);
					MapPut(graph, &from, &fromMap);
				} else { fromMap = *check; } 
				
				if ((check = MapGet(graph, &to)) == NULL) {
					// new Map<char*, double>
					toMap = newMap(sizeof(char*), sizeof(double), strHash, strCompare);
					MapPut(graph, &to, &toMap);
				} else { toMap = *check; }
				
				
				// Create transitions between currencies.
				double invRate = 1.0 / rate;
				MapPut(fromMap, &to, &rate);
				MapPut(toMap, &from, &invRate);
			}
		}
		
		lineNum++;
		
		// Free the extra memory used by strSplit()...
		// We retain our interned strings
		freeSplits(split, parts);
		// Also free the strdup(line);
		free(lineCpy);
	}
	// Can't forget to do this...
	fclose(fp);
	
	printf("Done loading graph, there are %i currencies.\n", graph->count);
	
	//printf("Printing out json representation of the graph, with lots of printfs:\n");
	//printf("\t(also seeding work queue with workdatas!)\n");
	//printf("{\n");
	// Iterate all of graph's keys/values...
	int32_t nodes = graph->count;
	for (int32_t i = 0; i < nodes; i++) {
		// Get pointer to pair
		void* pos = ListGet(graph->pairs, i);
		// Turn pos into key/value pointers...
		KVP pair = _MapKVP(pos, graph->keySize);
		// Dereference pointers to get actual values
		char* nodeName = *(char**)pair.key;
		Map* nodeRates = *(Map**)pair.val;
		
		//printf("\t%s: { ", nodeName);
		// int32_t transitionCount = nodeRates->count;
		// for (int32_t k = 0; k < transitionCount; k++) {
		// 	void* pos2 = ListGet(nodeRates->pairs, k);
		// 	KVP pair2 = _MapKVP(pos2, nodeRates->keySize);
		// 	char* targetName = *(char**)pair2.key;
		// 	double targetRate = *(double*)pair2.val;
			
		// 	//printf("%s: %lf, ", targetName, targetRate);
		// }
		//printf("}\n");
		
		// Create new work data.
		WorkData data;
		// Starting node name
		data.start = nodeName;
		// Currently at start
		data.at = nodeName;
		// starting with $1.00 of whatever currency.
		data.rate = 1.0;
		// Default score, doesn't matter till profits are made
		data.score = 0.0;
		// Create a new List<char*>...
		data.path = newList(sizeof(char*));
		// and add the starting node name to the list.
		ListAdd(data.path, &nodeName);
		
		// Copy data into queue.
		ListAdd(queue, &data);
	}
	
	//printf("}\n\n");
	printf("Graph seeded with %i queued work.\n", queue->count);
	printf("Going now...\n", queue->count);
	int64_t start = microtime_();
	while (queue->count > 0) {
		//printf("Only %i left!\n", queue->count);
		// Copy out work and remove from queue.
		WorkData data = *(WorkData*)ListGet(queue, 0);
		ListRemove(queue, 0);
		
		//printf("Currently at %s\n", data.at);
		//printPath(data.path, NULL);
		//printf("\n");
		// Score current place by stepping it to the starting location
		WorkData base = stepToStart(data);
		//printPath(base.path, NULL);
		//printf("\n");
		
		// Iterate all nodes
		for (int32_t i = 0; i < nodes; i++) {
			// Get key/value pointers
			void* pos = ListGet(graph->pairs, i);
			KVP pair = _MapKVP(pos, graph->keySize);
			// Extract key
			char* key = *(char**)pair.key;
			
			// See if this work has already been to that node
			if (!ListContains(data.path, &key, strCompare)) {
				//printf("Haven't been to %s yet...\n", key);
				// If we haven't, step there
				WorkData trace = stepTo(data, key);
				// then normalize and score it by stepping back to start
				WorkData final = stepToStart(trace);
				
				
				// If we did better by taking that step, add the unfinished one into the queue.
				if (final.rate >= base.rate) { ListAdd(queue, &trace); }
				else { 
					// Otherwise, prevent memory leak by nuking trace's path.
					killList(trace.path);
				} 
				
				// If we are profitable by taking that step, add it to the list of profitable nodes.
				if (final.rate > 1) { ListAdd(profitable, &final); }
				else { killList(final.path); } // Otherwise, prevent memory leak by nuking final's path.
				
			} else { /* skip places we've already been. */ }
		}
		
		// Only thing that is malloc'd in WorkData is the path List<char*>, 
		// kill them to prevent a memory leak.
		// Also, it's possible that stepTo() returns a copy of the same WorkData
		// (Eg, we are already at the starting location and step to starting location)
		
		int32_t diff = strcmp(data.at, base.at);
		killList(data.path);
		if (diff != 0) {
			killList(base.path);
		}
	}
	int64_t end = microtime_();
	double diff = (end - start)/1000.0;
	printf("\n\nDone!\n");
	
	// Create a List<char> as a StringBuilder...
	List* strbuilder = newList_cap(sizeof(char), 1024);
	// Another List<char> for basically an arena buffer for printing paths to
	List* pathstr = newList_cap(sizeof(char), 1024);
	// Sort data by ascending.
	
	// Sort by rate ascending
	ListSort(profitable, compareWorkByRateAscending);
	
	// Temp buffer and counter for snprintf
	// Honestly could've used another List*/List<char> sized to 1024 space to begin with
	char buffer[1024];
	int32_t printed;
	// Print all profitable paths by iterating the profitable List<WorkData>
	int32_t profitableCount = profitable->count;
	for (int32_t i = 0 ; i < profitableCount; i++) {
		// Extract WorkData from list
		WorkData data = *(WorkData*)ListGet(profitable, i);
		// Clear and print the path into the pathstr List<char>
		ListClear(pathstr);
		printPath(data.path, pathstr);
		
		// earned amount from $1000
		double earned = 1000.0 * data.rate;
		// Print it all into the buffer
		printed = snprintf(buffer, 1024, "Path: %s, $1000 => $%.2f\n", pathstr->data, earned);
		
		// Copy buffer into output file strbuilder
		ListAddMany(strbuilder, buffer, printed);
	}
	
	// Print other stuff out into both console and file
	printed = snprintf(buffer, 1024, "Took %.3fms\nFound %i profitable cycles!", diff, profitableCount);
	printf("%s", buffer);
	ListAddMany(strbuilder, buffer, printed);
	
	// Find best/most efficient paths
	
	// Sort by criteria
	ListSort(profitable, compareWorkByRateDescending);
	// Grab thing from list
	WorkData bestRate = *(WorkData*)ListGet(profitable, 0);
	// Clear/ print path into pathstr
	ListClear(pathstr);
	printPath(bestRate.path, pathstr);
	// Print to buffer
	printed = snprintf(buffer, 1024, "\n\nBest Overall Arbitrage: {\n\tRate: %.3f,\n\tScore: %.3f,\n\tPath: %s\n}", bestRate.rate, bestRate.score, pathstr->data);
	// print buffer to both console and file
	printf("%s", buffer);
	ListAddMany(strbuilder, buffer, printed);
	
	// And again, sort by criteria
	ListSort(profitable, compareWorkByScoreDescending);
	// Grab thing from list
	WorkData bestScore = *(WorkData*)ListGet(profitable, 0);
	// Clear/ print path into pathstr
	ListClear(pathstr);
	printPath(bestScore.path, pathstr);
	// Print to buffer
	printed = snprintf(buffer, 1024, "\n\nMost Efficient Arbitrage: {\n\tRate: %.3f,\n\tScore: %.3f,\n\tPath: %s\n}", bestScore.rate, bestScore.score, pathstr->data);
	// Print buffer to both console and file
	printf("%s", buffer);
	ListAddMany(strbuilder, buffer, printed);
	
	//*/
	
	// Finally, open file to write out stuff
	FILE* fw = fopen("outputc.txt", "w");
	if (fw == NULL) {
		printf("\nError: Failed to open 'outputc.txt'\n\n");
		return -1;
	}
	
	char nullc = '\0';
	ListAdd(strbuilder, &nullc);
	// Print strbuilder's buffer to file
	fprintf(fw, "%s", strbuilder->data);
	fclose(fw);
	
	// Clean up our string builders
	killList(strbuilder);
	killList(pathstr);
	
	// Clean up profitable data structures
	for (int i = 0; i < profitableCount; i++) {
		WorkData data = *(WorkData*)ListGet(profitable, i);
		killList(data.path);
	}
	killList(profitable);
	// Nothing should be left in queue.
	killList(queue);
	
	/// TBD: ouch. Iterate graph, clean it up
	//killMap(graph);
	// Also iterate interned Map and clean that up
	//killMap(interned);
	
	printf("\n\nDone cleaning up.\n");
	return 0;
}