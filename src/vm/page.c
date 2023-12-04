#include <debug.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "vm/page.h"
#include "filesys/file.h"
#include "lib/kernel/hash.h"
#include "threads/thread.h"
#include "threads/palloc.h"
#include "threads/vaddr.h"
#include "userprog/syscall.h"

static unsigned vm_hash_func (const struct hash_elem *e, void *aux);
static bool vm_less_func (const struct hash_elem *a, const struct hash_elem *b, void *aux);

struct lock vm_lock;

void
vm_init (struct hash *vm_table)
{
  bool success = hash_init(vm_table, vm_hash_func, vm_less_func, NULL);
  lock_init(&vm_lock);
  if (!success) {
    PANIC("vm_init failed 관련 invarient");
  }
}

static unsigned
vm_hash_func (const struct hash_elem *e, void *aux)
{
  struct vm_entry *vme = hash_entry(e, struct vm_entry, elem);
  return hash_int((int)vme->vaddr);
}

static bool
vm_less_func (const struct hash_elem *a, const struct hash_elem *b, void *aux)
{
  struct vm_entry *vme_a = hash_entry(a, struct vm_entry, elem);
  struct vm_entry *vme_b = hash_entry(b, struct vm_entry, elem);
  return vme_a->vaddr < vme_b->vaddr;
}

bool
insert_vme (struct hash *vm, struct vm_entry *vme)
{
  bool is_already_holded = lock_held_by_current_thread(&vm_lock);
  if (!is_already_holded) {
    lock_acquire (&vm_lock);
  }
  bool is_inserted = (hash_insert(vm, &vme->elem) == NULL);
  lock_release(&vm_lock);
  return is_inserted;
}

bool
delete_vme (struct hash *vm, struct vm_entry *vme)
{
  if (!hash_delete (vm, &vme->elem)){
    return false;
  }
  bool is_already_holded = lock_held_by_current_thread(&vm_lock);
  if (!is_already_holded) {
    lock_acquire (&vm_lock);
  }

  if (vme->is_loaded) {
    // palloc_free_page(vme->vaddr);
    // pagedir_clear_page(pd, vme->vaddr);
    free_frame(vme->vaddr);
    swap_clear (vme->swap_slot);
  }
  // free_frame (vme->vaddr);
  // swap_clear (vme->swap_slot);
  vme->type = NULL;
  free (vme);
  lock_release(&vm_lock);
  return true;
}

struct vm_entry*
find_vme (void *vaddr)
{
  struct hash *vm;
  struct vm_entry vme;
  struct hash_elem *elem;

  vm = &thread_current ()->vm_table;
  vme.vaddr = pg_round_down (vaddr);
  elem = hash_find (vm, &vme.elem);
  if (elem) {
    return hash_entry(elem, struct vm_entry, elem);
  }
  return NULL;
}

void
vm_destroy (struct hash *vm)
{
  bool is_already_holded = lock_held_by_current_thread(&vm_lock);
  if (!is_already_holded) {
    lock_acquire (&vm_lock);
  }
  hash_destroy (vm, vm_destroy_func);
  lock_release(&vm_lock);
}

void
vm_destroy_func (struct hash_elem *e, void *aux UNUSED)
{
  struct vm_entry *vme = hash_entry(e, struct vm_entry, elem);
  uint32_t *pd = thread_current()->pagedir;

  if (vme->is_loaded) {
    // palloc_free_page(vme->vaddr);
    // pagedir_clear_page(pd, vme->vaddr);
    free_frame(vme->vaddr);
    swap_clear (vme->swap_slot);
  }
  vme->type = NULL;
  free(vme);
}

bool load_file (void *kaddr, struct vm_entry *vme)
{
  if (file_read_at(vme->file, kaddr, vme->read_bytes, vme->offset) != (int)vme->read_bytes) {
    return false;
  }
  memset(kaddr + vme->read_bytes, 0, vme->zero_bytes);
  return true;
}

void
check_valid_buffer (void* buffer, unsigned size, void* esp, bool to_write)
{
  // for (int i = 0; i < size; i++) {
  //   struct vm_entry *vme = check_valid_address(buffer + i, esp);
  //   if (vme == NULL) {
  //     exit(-1);
  //   }
  //   if (to_write == true && vme->writable == false) {
  //     exit(-1);
  //   }
  //   // if (to_write == false && vme->writable == true) {
  //   //   exit(-1);
  //   // }
  // }
  struct vm_entry *vme;
	unsigned i;
	char *check_buffer = (char *)buffer;
	for(i=0; i<size; i++)
	{
		vme = check_valid_address((void *)check_buffer, esp);
		if(vme != NULL){
			if(to_write == true){
				if(vme->writable == false){
          exit(-1);
        }
			}
		}
		check_buffer++;
	}
}

void
check_valid_string (const void *str, void *esp)
{
  char *check_str = (char *)str;
	check_valid_address((void *)check_str, esp);

	while (*check_str != 0) {
		check_str += 1;
		check_valid_address(check_str, esp);
  }
}
