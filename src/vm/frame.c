#include <debug.h>
#include "threads/palloc.h"
#include "userprog/pagedir.h"
#include "vm/frame.h"
#include "vm/page.h"
#include "vm/swap.h"

struct list frame_table;
struct lock ft_lock;

void
frame_table_init(void)
{
  list_init(&frame_table);
  lock_init(&ft_lock);
}

void
add_frame_to_frame_table(struct frame *frame)
{
  list_push_back(&frame_table, &frame->ft_elem);
}

void
del_frame_from_frame_table(struct frame *frame)
{
  list_remove(&frame->ft_elem);
}

struct frame *
palloc_frame (enum palloc_flags flags)
{
  struct frame *frame;
  frame = (struct frame *) malloc (sizeof (struct frame));
  if (frame == NULL){
    return NULL;
  }

  memset (frame, 0, sizeof (struct frame));
  frame->owner_thread = thread_current ();
  frame->kaddr = palloc_get_page (flags);

  if (frame->kaddr == NULL) {
    frame->kaddr = lru_clock_algorithm(flags);
  }

  add_frame_to_frame_table(frame);
  return frame;
}

void
free_frame(void *kaddr)
{
  struct frame *frame = NULL;
  struct list_elem *e;

  for (e = list_begin (&frame_table); e != list_end (&frame_table); e = list_next (e)){
    struct frame *temp_frame = list_entry (e, struct frame, ft_elem);
    if (temp_frame->kaddr == kaddr){
        _free_frame(temp_frame);
        break;
    }
  }
}

void
_free_frame(struct frame* frame)
{
  pagedir_clear_page (frame->owner_thread->pagedir, frame->vme->vaddr);
  del_frame_from_frame_table(frame);
  palloc_free_page(frame->kaddr);
  free(frame);
}

static struct list_elem*
find_victim(void) {
  struct list_elem *victim;
  while (true) {
    for (victim = list_begin(&frame_table); victim != list_end(&frame_table); victim = list_next(victim)) {
      struct frame *f = list_entry(victim, struct frame, ft_elem);
      if (pagedir_is_accessed(f->owner_thread->pagedir, f->vme->vaddr)) {
        pagedir_set_accessed(f->owner_thread->pagedir, f->vme->vaddr, false);
      }
      else {
        return victim;
      }
    }
  }
  // ASSERT ("무조건 포문 안에서 찾아야되나?");
  // 한 사이클 돌려서 안 찾아졌을 때, 찾아질 때까지 계속 돌려야 하는지는 잘 모르겠음
  // victim으로 선정되는 페이지는 무조건 프로세스 data seg | stack에 포함되어에 하는 것 같음
  return NULL;
}

void*
lru_clock_algorithm(enum palloc_flags flags) {
  lock_acquire(&ft_lock);
  struct frame *victim_frame = list_entry(find_victim(), struct frame, ft_elem);
  struct thread *victim_thread = victim_frame->owner_thread;
  struct vm_entry *victim_vme = victim_frame->vme;

  if (victim_vme->type == VM_BIN || victim_vme->type == VM_ANON) {
    victim_vme->swap_slot = swap_out(victim_frame->kaddr);
    if (victim_vme->type == VM_BIN){
      victim_vme->type = VM_ANON;
    }
  }
  else if (victim_vme->type == VM_FILE) {
    if (pagedir_is_dirty(victim_thread->pagedir, victim_vme->vaddr)) {
      victim_vme->swap_slot = swap_out(victim_vme);
    }
    file_write_at(victim_vme->file, victim_vme->vaddr, victim_vme->read_bytes, victim_vme->offset);
  }
  else {
    // ASSERT("lru_clock_algorithm: victim_page->type is not VM_ANON or VM_FILE");
  }

  _free_frame(victim_frame);
  lock_release(&ft_lock);
  return palloc_get_page(flags);
}
