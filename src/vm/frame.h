#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "threads/thread.h"
#include "threads/palloc.h"
#include "lib/kernel/hash.h"
#include "filesys/file.h"
#include "vm/page.h"

struct frame {
    void *kaddr;                        // 페이지의 실제 주소를 가리키는 포인터
    struct vm_entry *vme;               // 해당 페이지에 매핑되는 vm_entry를 가리키는 포인터
    struct thread *owner_thread;        // 해당 페이지를 사용하는 스레드를 가리키는 포인터
    struct list_elem ft_elem;           // frame table(LRU 리스트)에서 사용되는 list_elem 구조체
};

void frame_table_init(void);
void add_frame_to_frame_table(struct frame *frame);
void del_frame_from_frame_table(struct frame *frame);
struct frame *palloc_frame (enum palloc_flags flags);
void free_frame(void *kaddr);
void* lru_clock_algorithm(enum palloc_flags flags);

#endif /* vm/frame.h */