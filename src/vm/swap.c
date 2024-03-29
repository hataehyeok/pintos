#include "vm/swap.h"
#include <bitmap.h>
#include "threads/synch.h"
#include "devices/block.h"
#include "threads/vaddr.h"
#include "threads/interrupt.h"

struct lock swap_lock;
struct bitmap *swap_bitmap;
extern struct lock filesys_lock;

void
swap_init (size_t size)
{
  lock_init (&swap_lock);
  swap_bitmap = bitmap_create (size);
}

void
swap_in (size_t used_index, void *kaddr)
{
  struct block *swap_block;
  swap_block = block_get_role (BLOCK_SWAP);
  used_index--;

  lock_acquire (&filesys_lock);
  lock_acquire (&swap_lock);
  used_index = used_index * 8;
  for (int i = 0; i < 8; i++){
    block_read (swap_block, used_index + i, kaddr + BLOCK_SECTOR_SIZE * i);
  }
  used_index = used_index / 8;
  bitmap_set_multiple (swap_bitmap, used_index, 1, false);
  lock_release (&swap_lock);
  lock_release (&filesys_lock);
}

size_t
swap_out (void *kaddr)
{
  struct block *swap_block;
  size_t swap_index;
  swap_block = block_get_role (BLOCK_SWAP);

  lock_acquire (&filesys_lock);
  lock_acquire (&swap_lock);
  swap_index = bitmap_scan_and_flip (swap_bitmap, 0, 1, false);
  swap_index = swap_index * 8;
  for (int i = 0; i < 8; i++){
    block_write (swap_block, swap_index + i, kaddr + BLOCK_SECTOR_SIZE * i);
  }
  swap_index = swap_index / 8;
  lock_release (&swap_lock);
  lock_release (&filesys_lock);

  return swap_index + 1;
}

void
swap_clear (size_t used_index)
{
  if (used_index == 0){
    return;
  }
  used_index --;
  lock_acquire (&swap_lock);
  bitmap_set_multiple (swap_bitmap, used_index, 1, false);
  lock_release (&swap_lock);
}