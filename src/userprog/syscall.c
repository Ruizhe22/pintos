#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "devices/shutdown.h"
#include "userprog/exception.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/process.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "devices/input.h"
#include "lib/kernel/stdio.h"
#include "vm/swap.h"
#include "vm/frame.h"
#include "vm/page.h"
#include "vm/stack.h"
#include "vm/mmap.h"

static void syscall_handler(struct intr_frame *);

static bool check_pointer(struct intr_frame *, const void *);


void
syscall_init(void) {
    intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
}

/** xv6 style, syscalls */
static void sys_halt(struct intr_frame *);
static void sys_exit(struct intr_frame *);
static void sys_exec(struct intr_frame *);
static void sys_wait(struct intr_frame *);
static void sys_create(struct intr_frame *);
static void sys_remove(struct intr_frame *);
static void sys_open(struct intr_frame *);
static void sys_filesize(struct intr_frame *);
static void sys_read(struct intr_frame *);
static void sys_write(struct intr_frame *);
static void sys_seek(struct intr_frame *);
static void sys_tell(struct intr_frame *);
static void sys_close(struct intr_frame *);
static void sys_mmap(struct intr_frame *);
static void sys_munmap(struct intr_frame *);

static void (*syscalls[])(struct intr_frame *) = {
        [SYS_HALT] = sys_halt,
        [SYS_EXIT] = sys_exit,
        [SYS_EXEC] = sys_exec,
        [SYS_WAIT] = sys_wait,
        [SYS_CREATE] = sys_create,
        [SYS_REMOVE] = sys_remove,
        [SYS_OPEN] = sys_open,
        [SYS_FILESIZE] = sys_filesize,
        [SYS_READ] = sys_read,
        [SYS_WRITE] = sys_write,
        [SYS_SEEK] = sys_seek,
        [SYS_TELL] = sys_tell,
        [SYS_CLOSE] = sys_close,
        [SYS_MMAP] = sys_mmap,
        [SYS_MUNMAP] = sys_munmap
};


/** read syscall number, and invoke the corresponding syscall function installed in 'syscalls' vector  */
static void
syscall_handler(struct intr_frame *f) {
    int num;
    if(check_pointer(f, f->esp)){
        num = *(int *)(f->esp);
        if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
            syscalls[num](f);
        }
        else{
            exit(-1);
        }
    }
    else{
        exit(-1);
    }
}

/** use the first method to check if address provided by the user is safe and legal */
static bool
check_pointer(struct intr_frame *f, const void *vaddr)
{
    if (!is_user_vaddr(vaddr) || vaddr < USER_VADDR_BOTTOM){
        return false;
    }
    struct page *page = find_page(&thread_current()->page_table, vaddr);
    if (page){
        load_page(page);
    }
    else if(stack_addr(f->esp, vaddr)){
        stack_load(pg_round_down(vaddr));
    }
    void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr);
    if (!ptr)
    {
        return false;
    }
    return true;

}


/** Projects 2 and later.
 *
 * every syscall handler is divided into two function,
 * the one starting with "sys" is called directly by syscall handler, retrieving the parameters from the intr_frame.
 * the other one starting without "sys" is called by corresponding "sys" function with the args already retieved.
 * "sys" functions' prototype are all void(struct intr_frame *), so they can be installed into the "syscalls" vector,
 * they checks the validity of the arguments above esp pointer, if the arguments are not valid, calls exit(-1).
 * acquires the file system lock(if needed), calls corresponding function with the argument value,
 * sets the return value in f->eax(if needed), and releases the file system lock(if needed).
 * "no sys" functions implement the substantial work, some of which are made inline functions to accelerate.
 * */

inline void
halt(void)
{
    shutdown_power_off();
}

static void
sys_halt(struct intr_frame *f)
{
    halt();
}

/* updates the status code and terminates the current thread. **/
void
exit(int status)
{
    struct thread *cur = thread_current();
    printf ("%s: exit(%d)\n", cur->name, status);
    cur->as_child->status_code = status;
    cur->as_child->terminate_by_exit = true;
    thread_exit();
}

static void
sys_exit(struct intr_frame *f)
{
    int *arg = f->esp + 4;
    if(check_pointer(f,arg)){
        exit(*arg);
    }
    else{
        exit(-1);
    }
}

/** calls process_execute(file) to create a new process. return the tid of new thread. */
inline tid_t
exec(const char *file)
{
    return process_execute(file);
}

static void sys_exec(struct intr_frame *f)
{
    char **arg = f->esp + 4;

    if(check_pointer(f,arg) && check_pointer(f,*arg)){
        filesys_lock_acquire();
        f->eax = exec(*arg);
        filesys_lock_release();
    }
    else{
        exit(-1);
    }
}

/** calls process_wait(pid) to wait for the child process to exit, return the status code if child exits by syscall. */

inline int
wait(tid_t pid)
{
    return process_wait(pid);
}

static void sys_wait(struct intr_frame *f)
{
    int *arg = f->esp + 4;
    if(check_pointer(f,arg)){
        f->eax = wait(*arg);
    }
    else{
        exit(-1);
    }
}

/** creat a new file with the specified name and initial size. */

inline bool
create(const char *file, unsigned initial_size)
{
    return filesys_create(file, initial_size);
}

static void
sys_create(struct intr_frame *f)
{
    char **arg1 = f->esp + 4;
    unsigned *arg2 = f->esp + 8;
    if(check_pointer(f,arg1) && check_pointer(f,*arg1) && check_pointer(f,arg2)){
        filesys_lock_acquire();
        f->eax = create(*arg1, *arg2);
        filesys_lock_release();
    }
    else{
        exit(-1);
    }
}

/** remove a file with the specified name. */

inline bool
remove(const char *file)
{
    return filesys_remove(file);
}

static void
sys_remove(struct intr_frame *f)
{
    char **arg = f->esp + 4;
    if(check_pointer(f, arg) && check_pointer(f, *arg)){
        filesys_lock_acquire();
        f->eax = remove(*arg);
        filesys_lock_release();
    }
    else{
        exit(-1);
    }
}

/** open the file using filesys_open(file) and initializes a file descriptor by calling fd_init function where the fd
 * number will be allocated automatically, and the file_descriptor struct will be kept into the thread's struct. */

int
open(const char *file)
{
    struct file *file_ptr = filesys_open(file);
    if(!file_ptr) return -1;
    struct file_descriptor *filedescriptor = (struct file_descriptor *)malloc(sizeof(struct file_descriptor));
    return fd_init(filedescriptor, file_ptr);
}

static void sys_open(struct intr_frame *f)
{
    char **arg = f->esp + 4;
    if(check_pointer(f, arg) && check_pointer(f, *arg)){
        filesys_lock_acquire();
        f->eax = open(*arg);
        filesys_lock_release();
    }
    else{
        exit(-1);
    }
}

/** finds the file descriptor associated with the given file descriptor ID,
 * and then returns the length of the file using file_length. */

inline int
filesize(int fd)
{
    struct file_descriptor *fileDescriptor = fd_find(fd);
    if(!fileDescriptor) {
        filesys_lock_release();
        exit(-1);
    }
    return file_length(fileDescriptor->file_ptr);
}

static void sys_filesize(struct intr_frame *f)
{
    int *arg = f->esp + 4;
    if(check_pointer(f, arg)){
        filesys_lock_acquire();
        f->eax = filesize(*arg);
        filesys_lock_release();
    }
    else{
        exit(-1);
    }

}

/** reads data from a file descriptor or standard input.
 * If fd is 0, it reads from standard input using input_getc().
 * If fd is not 0, it finds the file descriptor associated with the given file descriptor ID and reads from the file.*/

int
read(int fd, void *buffer, unsigned length)
{
    if(fd==0){
        for(unsigned i = 0; i < length; ++i){
            *(char *)(buffer+i) = input_getc();
        }
        return length;
    }
    else{
        struct file_descriptor *fileDescriptor = fd_find(fd);
        if(!fileDescriptor) {
            filesys_lock_release();
            exit(-1);
        }
        return file_read(fileDescriptor->file_ptr, buffer, length);
    }
}

static void
sys_read(struct intr_frame *f)
{
    int *arg1 = f->esp + 4;
    void **arg2 = f->esp + 8;
    unsigned *arg3 = f->esp + 12;

    if(check_pointer(f, arg1) && check_pointer(f, arg2) && check_pointer(f, *arg2) && check_pointer(f, arg3) ){
        filesys_lock_acquire();
        f->eax = read(*arg1, *arg2, *arg3);
        filesys_lock_release();
    }
    else{
        exit(-1);
    }
}

/** write data to a file descriptor or standard output.
 * If fd is 1, it writes to standard output using putbuf(buffer, length).
 * If fd is not 1, it finds the file descriptor associated with the given file descriptor ID and writes to the file. */

int
write(int fd, const void *buffer, unsigned length)
{
    if(fd==1){
        putbuf(buffer, length);
        return length;
    }
    else{
        struct file_descriptor *fileDescriptor = fd_find(fd);
        if(!fileDescriptor) {
            filesys_lock_release();
            exit(-1);
        }
        return file_write(fileDescriptor->file_ptr, buffer, length);
    }
}

static void
sys_write(struct intr_frame *f)
{
    int *arg1 = f->esp + 4;
    void **arg2 = f->esp + 8;
    unsigned *arg3 = f->esp + 12;

    if(check_pointer(f, arg1) && check_pointer(f, arg2) && check_pointer(f, *arg2) && check_pointer(f, arg3) ){
        filesys_lock_acquire();
        f->eax = write(*arg1, *arg2, *arg3);
        filesys_lock_release();
    }
    else{
        exit(-1);
    }
}

/** find the file descriptor by fd number and calls file_seek to update the position. */

void
seek(int fd, unsigned position)
{
    struct file_descriptor *fileDescriptor = fd_find(fd);
    if(!fileDescriptor) {
        filesys_lock_release();
        exit(-1);
    }
    file_seek(fileDescriptor->file_ptr, position);
}

static void
sys_seek(struct intr_frame *f)
{
    int *arg1 = f->esp + 4;
    unsigned *arg2 = f->esp + 8;
    if(check_pointer(f, arg1) && check_pointer(f, arg2)){
        filesys_lock_acquire();
        seek(*arg1, *arg2);
        filesys_lock_release();
    }
    else{
        exit(-1);
    }
}

/**  find the file descriptor by fd number and calls file_tell to get the current position. */

unsigned
tell(int fd)
{
    struct file_descriptor *fileDescriptor = fd_find(fd);
    if(!fileDescriptor) {
        filesys_lock_release();
        exit(-1);
    }
    return file_tell(fileDescriptor->file_ptr);
}

static void
sys_tell(struct intr_frame *f)
{
    int *arg = f->esp + 4;
    if(check_pointer(f, arg)){
        filesys_lock_acquire();
        f->eax = tell(*arg);
        filesys_lock_release();
    }
    else{
        exit(-1);
    }
}

/** closes the file descriptor associated with the given file descriptor ID,
 * removes the file descriptor from the list, and frees the struct's memory.
 */

void
close(int fd)
{
    struct file_descriptor *filedescriptor = fd_find(fd);
    if(!filedescriptor) return;
    file_close(filedescriptor->file_ptr);
    list_remove(&filedescriptor->elem);
    free(filedescriptor);

}

static void
sys_close(struct intr_frame *f)
{
    int *arg = f->esp + 4;
    if(check_pointer(f, arg)){
        filesys_lock_acquire();
        close(*arg);
        filesys_lock_release();
    }
    else{
        exit(-1);
    }
}

/** map a file to page, lazy load.
 * a little different from other syscall where not check addr in sys_mmap,
 * checking addr is left to creat_insert_mmap(), because there are much more limits about the addr.
 * */

mapid_t
mmap(int fd, void *addr)
{
    if(fd == 0 || fd == 1) return -1;
    struct file *file = fd_find(fd)->file_ptr;
    struct mmap *mmap_t = create_insert_mmap(thread_current(),file,addr);
    if(mmap_t==NULL) return -1;
    mmap_set_page(mmap_t);
    return mmap_t->map_id;
}

static void
sys_mmap(struct intr_frame *f)
{
    int *arg1 = f->esp + 4;
    void **arg2 = f->esp + 8;

    if(check_pointer(f, arg1) && check_pointer(f, arg2)){
        filesys_lock_acquire();
        f->eax = mmap(*arg1, *arg2);
        filesys_lock_release();
    }
    else{
        exit(-1);
    }
}



/** parse all the continuous pages of this mmap, for every page
 * evict the frame if page is in memory frame, and save its content if dirty,
 * then remove page from supplement page table
 * at last remove the mmap in mmap_table;
 */
void
munmap(mapid_t mapid)
{
    struct thread *thread = thread_current();
    struct mmap *the_mmap = find_mmap(&thread->mmap_table, mapid);
    if(the_mmap == NULL) return;
    void *upage = the_mmap->start_upage;
    for(int i = 0; i < the_mmap->page_volume; ++i){
        struct page *page = find_page(&thread->page_table, upage);
        if(page == NULL) continue;
        if (page->status == PAGE_FRAME && page->frame) {
            if(pagedir_is_dirty (thread->pagedir, page->upage)){
                filesys_lock_release();
                write_mmap(page, page->frame);
                filesys_lock_acquire();
            }
            pagedir_clear_page(thread->pagedir, page->upage);
            evict_frame(page->frame);
        }
        hash_delete(&thread->page_table, &page->hash_elem);
        free(page);
        upage += PGSIZE;
    }
    hash_delete(&thread->mmap_table, &the_mmap->hash_elem);
}

static void sys_munmap(struct intr_frame *f)
{
    int *arg1 = f->esp + 4;
    if(check_pointer(f, arg1)){
        filesys_lock_acquire();
        munmap(*arg1);
        filesys_lock_release();
    }
    else{
        exit(-1);
    }
}
