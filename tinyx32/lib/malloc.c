#include "tinyx32.h"

#include <assert.h>
#include <stdalign.h>
#include <sys/mman.h>

typedef struct AllocedBlock {
  size_t align;
  size_t size;
  char mem[];
} AllocedBlock;

static_assert((sizeof(AllocedBlock) & (sizeof(AllocedBlock) - 1)) == 0,
              "sizeof(AllocedBlock)");

typedef struct FreeBlock {
  struct FreeBlock* next;
  size_t size;
} FreeBlock;

static_assert(sizeof(FreeBlock) <= sizeof(AllocedBlock), "sizeof(FreeBlock)");

static FreeBlock* free_block_chain = NULL;

void* malloc(size_t n) { return aligned_alloc(sizeof(AllocedBlock), n); }

void* calloc(size_t m, size_t n) {
  size_t size;
  if (__builtin_mul_overflow(m, n, &size)) return NULL;
  void* ptr = malloc(size);
  if (ptr) memset(ptr, 0, size);
  return ptr;
}

void* aligned_alloc(size_t align, size_t n) {
  if ((align == 0) || (align & (align - 1)) != 0 || n == 0) return NULL;
  if (align < sizeof(AllocedBlock)) align = sizeof(AllocedBlock);

  n = (n + align - 1) & ~(align - 1);
  size_t total_size = n + align;

  FreeBlock** prev = &free_block_chain;
  for (FreeBlock* p = free_block_chain; p; prev = &p->next, p = p->next) {
    if (((uintptr_t)p & (align - 1)) == 0 && p->size >= total_size) {
      AllocedBlock* alloced =
          (AllocedBlock*)((char*)p + align - sizeof(AllocedBlock));
      if (p->size == total_size) {
        *prev = p->next;
      } else {
        FreeBlock* fb = (FreeBlock*)(alloced->mem + n);
        fb->next = p->next;
        fb->size = p->size - total_size;
        *prev = fb;
      }
      alloced->align = align;
      alloced->size = total_size;
      return alloced->mem;
    }
  }

  size_t mmap_size = (total_size + 65535) & ~65535;
  void* np = fsys_mmap(NULL, mmap_size, PROT_READ | PROT_WRITE,
                       MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  if (fsys_mmap_failed(np)) return NULL;

  if ((uintptr_t)np & (align - 1)) {
    fsys_munmap(np, mmap_size);
    return NULL;
  }

  AllocedBlock* alloced =
      (AllocedBlock*)((char*)np + align - sizeof(AllocedBlock));
  alloced->align = align;
  alloced->size = total_size;

  if (mmap_size > total_size) {
    FreeBlock* fb = (FreeBlock*)((char*)alloced + total_size);
    fb->next = free_block_chain;
    fb->size = mmap_size - total_size;
    free_block_chain = fb;
  }

  return alloced->mem;
}

#ifdef TX32_DUMMY_FREE
void free(void* p) { (void)p; }
#else
void free(void* p) {
  if (p == NULL) return;
  AllocedBlock* ab = (AllocedBlock*)((char*)p - sizeof(AllocedBlock));
  FreeBlock* fb = (FreeBlock*)((char*)p - ab->align);
  fb->size = ab->size;
  fb->next = free_block_chain;
  free_block_chain = fb;
}
#endif
