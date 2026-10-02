#include "platform.hpp"
#include "../pandora.hpp"

void* MemAlloc_Alloc( int size ) {
  return g_pMemAlloc->Alloc( size );
}

void *MemAlloc_Realloc( void *pMemBlock, int size ) {
  return g_pMemAlloc->Realloc( pMemBlock, size );
}

void MemAlloc_Free( void* pMemBlock ) {
  g_pMemAlloc->Free( pMemBlock );
}
