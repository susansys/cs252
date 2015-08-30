//
// CS252: MyMalloc Project
//
// The current implementation gets memory from the OS
// every time memory is requested and never frees memory.
//
// You will implement the allocator as indicated in the handout.
// 
// Also you will need to add the necessary locking mechanisms to
// support multi-threaded programs.
//

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <pthread.h>
#include "MyMalloc.h"

static pthread_mutex_t mutex;

const int ArenaSize = 2097152;
const int NumberOfFreeLists = 1;

// Header of an object. Used both when the object is allocated and freed
struct ObjectHeader {
    size_t _objectSize;         // Real size of the object.
    int _allocated;             // 1 = yes, 0 = no 2 = sentinel
    struct ObjectHeader * _next;       // Points to the next object in the freelist (if free).
    struct ObjectHeader * _prev;       // Points to the previous object.
};

struct ObjectFooter {
    size_t _objectSize;
    int _allocated;
};

  //STATE of the allocator

  // Size of the heap
  static size_t _heapSize;

  // initial memory pool
  static void * _memStart;

  // number of chunks request from OS
  static int _numChunks;

  // True if heap has been initialized
  static int _initialized;

  // Verbose mode
  static int _verbose;

  // # malloc calls
  static int _mallocCalls;

  // # free calls
  static int _freeCalls;

  // # realloc calls
  static int _reallocCalls;

  // # realloc calls
  static int _callocCalls;

  static int _miniSize;
  // Free list is a sentinel
  static struct ObjectHeader _freeListSentinel; // Sentinel is used to simplify list operations
  static struct ObjectHeader *_freeList;

  //FUNCTIONS

  //Initializes the heap
  void initialize();

  // Allocates an object 
  void * allocateObject( size_t size );

  // Frees an object
  void freeObject( void * ptr );

  struct ObjectHeader * freeMemoryBlock(struct ObjectHeader * header, size_t size);

  void mergeIntoFreeList(struct ObjectHeader * newHeader, struct ObjectHeader * oldHeader);

  void insertIntoFreeList(struct ObjectHeader * header);

  void removeFromFreeList(struct ObjectHeader * header);
  // Split memory chunk
  void splitMemChunk(struct ObjectHeader * header, size_t size);

  // Returns the size of an object
  size_t objectSize( void * ptr );

  // At exit handler
  void atExitHandler();

  //Prints the heap size and other information about the allocator
  void print();
  void print_list();

  // Gets memory from the OS
  void * getMemoryFromOS( size_t size );

  void * getMemoryFromFreeList ( size_t size);

  void * requestNewMemoryFromOS();

  void increaseMallocCalls() { _mallocCalls++; }

  void increaseReallocCalls() { _reallocCalls++; }

  void increaseCallocCalls() { _callocCalls++; }

  void increaseFreeCalls() { _freeCalls++; }

extern void
atExitHandlerInC()
{
  atExitHandler();
}

void initialize()
{
  // Environment var VERBOSE prints stats at end and turns on debugging
  // Default is on
  _miniSize = 8 + sizeof(struct ObjectHeader) + sizeof(struct ObjectFooter);
  _verbose = 1;
  const char * envverbose = getenv( "MALLOCVERBOSE" );
  if ( envverbose && !strcmp( envverbose, "NO") ) {
    _verbose = 0;
  }

  pthread_mutex_init(&mutex, NULL);
  void * _mem = getMemoryFromOS( ArenaSize + (2*sizeof(struct ObjectHeader)) + (2*sizeof(struct ObjectFooter)) );

  // In verbose mode register also printing statistics at exit
  atexit( atExitHandlerInC );

  //establish fence posts
  struct ObjectFooter * fencepost1 = (struct ObjectFooter *)_mem;
  fencepost1->_allocated = 1;
  fencepost1->_objectSize = 123456789;
  char * temp = 
      (char *)_mem + (2*sizeof(struct ObjectFooter)) + sizeof(struct ObjectHeader) + ArenaSize;
  struct ObjectHeader * fencepost2 = (struct ObjectHeader *)temp;
  fencepost2->_allocated = 1;
  fencepost2->_objectSize = 123456789;
  fencepost2->_next = NULL;
  fencepost2->_prev = NULL;

  //initialize the list to point to the _mem
  temp = (char *) _mem + sizeof(struct ObjectFooter);
  struct ObjectHeader * currentHeader = (struct ObjectHeader *) temp;
  temp = (char *)_mem + sizeof(struct ObjectFooter) + sizeof(struct ObjectHeader) + ArenaSize;
  struct ObjectFooter * currentFooter = (struct ObjectFooter *) temp;
  _freeList = &_freeListSentinel;
  currentHeader->_objectSize = ArenaSize + sizeof(struct ObjectHeader) + sizeof(struct ObjectFooter); //2MB
  currentHeader->_allocated = 0;
  currentHeader->_next = _freeList;
  currentHeader->_prev = _freeList;
  currentFooter->_allocated = 0;
  currentFooter->_objectSize = currentHeader->_objectSize;
  _freeList->_prev = currentHeader;
  _freeList->_next = currentHeader; 
  _freeList->_allocated = 2; // sentinel. no coalescing.
  _freeList->_objectSize = 0;
  _memStart = (char*) currentHeader;
}

void * allocateObject( size_t size )
{
  //Make sure that allocator is initialized
  if ( !_initialized ) {
    _initialized = 1;
    initialize();
  }

  // Add the ObjectHeader/Footer to the size and round the total size up to a multiple of
  // 8 bytes for alignment.
  size_t roundedSize = (size + sizeof(struct ObjectHeader) + sizeof(struct ObjectFooter) + 7) & ~7;

  // Naively get memory from the OS every time
  // void * _mem = getMemoryFromOS( roundedSize );
  void * _mem = getMemoryFromFreeList( roundedSize);

  // Store the size in the header
  struct ObjectHeader * o = (struct ObjectHeader *) _mem;

  o->_objectSize = roundedSize;

  pthread_mutex_unlock(&mutex);

  // Return a pointer to usable memory
  return (void *) (o + 1);

}

void * getMemoryFromFreeList( size_t size )
{
  struct ObjectHeader* temp = _freeList->_next;
  // Find the free block that is large enough to satisfy request
  // one whole is fully allocated
  do {
      if(temp->_allocated==0) {
          if(temp->_objectSize >= size) {
              break;
          }
      }
      temp = temp->_next;
      if(temp->_allocated == 2) {
          requestNewMemoryFromOS();
      }
  }while(1);

  if(temp->_objectSize - size >= _miniSize) {
  // If the new allocated mem is larger than miniSize, split it
      splitMemChunk(temp, size);
      return temp;
  }
  else {
  // The condition that return the whole block
  // 1. Set allocated flags to 1
  // 2. Set object size to the rounded size
  // 2. Take care of pointers of free list
      temp->_allocated = 1;

      struct ObjectFooter * footer1 = (struct ObjectFooter *)((char *)temp + size - sizeof(struct ObjectFooter));
      footer1->_allocated = 1;
      footer1->_objectSize = size;

      removeFromFreeList(temp);
      return temp;
  }
}

void splitMemChunk(struct ObjectHeader * header, size_t size) {
    // This function will
    // 1. Split large memory chunk according to the given rounded size
    // 2. Create new footer1 for allocated memory.
    // 3. Create new header2 for the left free memory.
    // 4. Take care of 4 pointers for the new header2.
    // 5. Take care of size for 4 object.
    // 6. Set allocated flags for both allocated memory and left free memory
    size_t leftFreeSize = header->_objectSize - size;

    header->_allocated = 1;
    header->_objectSize = size;

    struct ObjectFooter * footer1 = (struct ObjectFooter *)((char *)header + size - sizeof(struct ObjectFooter));
    footer1->_allocated = 1;
    footer1->_objectSize = size;

    struct ObjectHeader * header2 = (struct ObjectHeader *)((char *)header + size);
    header2->_next = header->_next;
    header2->_prev = header->_prev;
    header->_prev->_next = header2;
    header->_next->_prev = header2;
    header2->_allocated = 0;
    header2->_objectSize = leftFreeSize;

    struct ObjectFooter * footer2 = (struct ObjectFooter *)((char *)header2 + header2->_objectSize - sizeof(struct ObjectFooter));
    footer2->_allocated = 0;
    footer2->_objectSize = leftFreeSize;
}

void * requestNewMemoryFromOS() {
    void * _mem = getMemoryFromOS( ArenaSize + (2*sizeof(struct ObjectHeader)) + (2*sizeof(struct ObjectFooter)) );

    //establish fence posts
    struct ObjectFooter * fencepost1 = (struct ObjectFooter *)_mem;
    fencepost1->_allocated = 1;
    fencepost1->_objectSize = 123456789;
    char * temp =
        (char *)_mem + (2*sizeof(struct ObjectFooter)) + sizeof(struct ObjectHeader) + ArenaSize;
    struct ObjectHeader * fencepost2 = (struct ObjectHeader *)temp;
    fencepost2->_allocated = 1;
    fencepost2->_objectSize = 123456789;
    fencepost2->_next = NULL;
    fencepost2->_prev = NULL;

    //initialize the list to point to the _mem
    temp = (char *) _mem + sizeof(struct ObjectFooter);
    struct ObjectHeader * currentHeader = (struct ObjectHeader *) temp;
    temp = (char *)_mem + sizeof(struct ObjectFooter) + sizeof(struct ObjectHeader) + ArenaSize;
    struct ObjectFooter * currentFooter = (struct ObjectFooter *) temp;
    currentHeader->_objectSize = ArenaSize + sizeof(struct ObjectHeader) + sizeof(struct ObjectFooter); //2MB
    currentHeader->_allocated = 0;

    // _freeList->_prev->_next = currentHeader;
    // currentHeader->_prev = _freeList->_prev;
    // _freeList->_prev = currentHeader;
    // currentHeader->_next = _freeList;

    _freeList->_next = currentHeader;
    _freeList->_prev = currentHeader;
    currentHeader->_prev = _freeList;
    currentHeader->_next = _freeList;

    currentFooter->_objectSize = currentHeader->_objectSize;
    currentFooter->_allocated = 0;
}

void freeObject( void * ptr )
{
  // Add your code here

  struct ObjectHeader * header = (struct ObjectHeader *)((char *)ptr - sizeof(struct ObjectHeader));

  struct ObjectFooter * leftFooter =
      (struct ObjectFooter *)((char *)header - sizeof(struct ObjectFooter));

  if(leftFooter->_allocated == 0) {
      struct ObjectHeader * leftHeader =
          (struct ObjectHeader *)((char *)header - leftFooter->_objectSize);
      leftHeader = freeMemoryBlock(leftHeader, leftHeader->_objectSize + header->_objectSize);

      struct ObjectHeader * rightHeader =
          (struct ObjectHeader *)((char *)header + header->_objectSize);
      if(rightHeader->_allocated == 0) {
          leftHeader = freeMemoryBlock(leftHeader, leftHeader->_objectSize + rightHeader->_objectSize);
          mergeIntoFreeList(leftHeader, rightHeader);
          return;
      }
      else {
          return;
      }
  }
  else {
    struct ObjectHeader * rightHeader =
        (struct ObjectHeader *)((char *) header + header->_objectSize);
    if(rightHeader->_allocated == 0) {
        header = freeMemoryBlock(header, header->_objectSize + rightHeader->_objectSize);
        mergeIntoFreeList(header, rightHeader);
        return;
    }
    else {
        header = freeMemoryBlock(header, header->_objectSize);
        insertIntoFreeList(header);
        return;
    }
  }
}

struct ObjectHeader * freeMemoryBlock( struct ObjectHeader * header, size_t size ) {
    header->_objectSize = size;
    header->_allocated = 0;

    struct ObjectFooter * footer = (struct ObjectFooter *)((char *)header + size - sizeof(struct ObjectFooter));
    footer->_objectSize = size;
    footer->_allocated = 0;
    return header;
}

void mergeIntoFreeList(struct ObjectHeader * newHeader, struct ObjectHeader * oldHeader) {
    oldHeader->_prev->_next = newHeader;
    oldHeader->_next->_prev = newHeader;
    newHeader->_next = oldHeader->_next;
    newHeader->_prev = oldHeader->_prev;
    return;
}

void insertIntoFreeList(struct ObjectHeader * header) {
    struct ObjectHeader * prev = _freeList;
    struct ObjectHeader * now = prev->_next;
    // printf("The pointer in the MyMalloc is at: %ld\n", (long)header);
    do {
        if( prev->_allocated == 2 && now->_allocated == 2 ) {
            break;
        }
        if( (long)prev < (long)header && (long)header < (long)now ) {
            break;
        }
        prev = now;
        now = now->_next;
    }while(1);
    // printf("-----\nThe pointer of the prev is at: %ld\n", (long)prev-(long)_memStart);
    // printf("The pointer of header is at: %ld\n", (long)header-(long)_memStart);
    // printf("The pointer of the now is at: %ld\n", (long)now-(long)_memStart);

    header->_next = now;
    header->_prev = prev;
    prev->_next = header;
    now->_prev = header;

    return;
}

void removeFromFreeList(struct ObjectHeader * header) {
    struct ObjectHeader * prev = header->_prev;
    struct ObjectHeader * next = header->_next;

    prev->_next = next;
    next->_prev = prev;

}

size_t objectSize( void * ptr )
{
  // Return the size of the object pointed by ptr. We assume that ptr is a valid obejct.
  struct ObjectHeader * o =
    (struct ObjectHeader *) ( (char *) ptr - sizeof(struct ObjectHeader) );

  // Substract the size of the header
  return o->_objectSize;
}

void print()
{
  printf("\n-------------------\n");

  printf("HeapSize:\t%zd bytes\n", _heapSize );
  printf("# mallocs:\t%d\n", _mallocCalls );
  printf("# reallocs:\t%d\n", _reallocCalls );
  printf("# callocs:\t%d\n", _callocCalls );
  printf("# frees:\t%d\n", _freeCalls );

  printf("\n-------------------\n");
}

void print_list()
{
  printf("FreeList: ");
  if ( !_initialized ) {
    _initialized = 1;
    initialize();
  }
  struct ObjectHeader * ptr = _freeList->_next;
  while(ptr != _freeList){
      long offset = (long)ptr - (long)_memStart;
      printf("[offset:%ld,size:%zd]",offset,ptr->_objectSize);
      ptr = ptr->_next;
      if(ptr != NULL){
          printf("->");
      }
  }
  printf("\n");
}

void * getMemoryFromOS( size_t size )
{
  // Use sbrk() to get memory from OS
  _heapSize += size;
 
  void * _mem = sbrk( size );

  if(!_initialized){
      _memStart = _mem;
  }

  _numChunks++;

  return _mem;
}

void atExitHandler()
{
  // Print statistics when exit
  if ( _verbose ) {
    print();
  }
}

//
// C interface
//

extern void *
malloc(size_t size)
{
  pthread_mutex_lock(&mutex);
  increaseMallocCalls();
  
  return allocateObject( size );
}

extern void
free(void *ptr)
{
  pthread_mutex_lock(&mutex);
  increaseFreeCalls();
  
  if ( ptr == 0 ) {
    // No object to free
    pthread_mutex_unlock(&mutex);
    return;
  }
  
  freeObject( ptr );
}

extern void *
realloc(void *ptr, size_t size)
{
  pthread_mutex_lock(&mutex);
  increaseReallocCalls();
    
  // Allocate new object
  void * newptr = allocateObject( size );

  // Copy old object only if ptr != 0
  if ( ptr != 0 ) {
    
    // copy only the minimum number of bytes
    size_t sizeToCopy =  objectSize( ptr );
    if ( sizeToCopy > size ) {
      sizeToCopy = size;
    }
    
    memcpy( newptr, ptr, sizeToCopy );

    //Free old object
    freeObject( ptr );
  }

  return newptr;
}

extern void *
calloc(size_t nelem, size_t elsize)
{
  pthread_mutex_lock(&mutex);
  increaseCallocCalls();
    
  // calloc allocates and initializes
  size_t size = nelem * elsize;

  void * ptr = allocateObject( size );

  if ( ptr ) {
    // No error
    // Initialize chunk with 0s
    memset( ptr, 0, size );
  }

  return ptr;
}
