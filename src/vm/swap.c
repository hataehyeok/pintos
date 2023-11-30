#include "userprog/process.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "userprog/gdt.h"
#include "userprog/pagedir.h"
#include "userprog/tss.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/flags.h"
#include "threads/init.h"
#include "threads/interrupt.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"
#include "vm/page.h"
#include "lib/kernel/bitmap.h"
#include "devices/block.h"

struct block *swap_block;
struct bitmap *swap_bitmap;

void swap_init(void) {
    swap_block = block_get_role(BLOCK_SWAP);
    if (swap_block == NULL) {
        return NULL;
    }
    swap_bitmap = bitmap_create(block_size(swap_block) / SECTORS_PER_PAGE);
    if (swap_bitmap == NULL) {
        return NULL;
    }
    bitmap_set_all(swap_bitmap, false);
}

size_t swap_out(void *kaddr) {
    size_t free_index = bitmap_scan_and_flip(swap_bitmap, 0, 1, false);
    if (free_index == BITMAP_ERROR) {
        return NULL;
    }

    for (int i = 0; i < SECTORS_PER_PAGE; ++i) {
        block_write(swap_block, free_index * SECTORS_PER_PAGE + i, kaddr + i * BLOCK_SECTOR_SIZE);
    }
    return free_index;
}


void swap_in(size_t used_index, void *kaddr) {
    for (int i = 0; i < SECTORS_PER_PAGE; ++i) {
        block_read(swap_block, used_index * SECTORS_PER_PAGE + i, kaddr + i * BLOCK_SECTOR_SIZE);
    }
    bitmap_set(swap_bitmap, used_index, false);
}