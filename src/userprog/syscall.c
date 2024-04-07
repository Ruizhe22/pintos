#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "devices/shutdown.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/process.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "devices/input.h"
#include "lib/kernel/stdio.h"

static void syscall_handler(struct intr_frame *);

static bool check_pointer(const void *);


void
syscall_init(void) {
    intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
}
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
        [SYS_CLOSE] = sys_close
};


static void
syscall_handler(struct intr_frame *f) {
    int num;
    if(check_pointer(f->esp)){
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

static bool
check_pointer(const void *vaddr)
{
    if (!is_user_vaddr(vaddr))
    {
        return false;
    }
    void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr);
    if (!ptr)
    {
        return false;
    }
    return true;
}


/** Projects 2 and later. */

inline void
halt(void)
{
    shutdown_power_off();
}

static void sys_halt(struct intr_frame *f)
{
    halt();
}

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
    if(check_pointer(arg)){
        exit(*arg);
    }
    else{
        exit(-1);
    }
}

inline tid_t
exec(const char *file)
{
    return process_execute(file);
}

static void sys_exec(struct intr_frame *f)
{
    char **arg = f->esp + 4;

    if(check_pointer(arg) && check_pointer(*arg)){
        filesys_lock_acquire();
        f->eax = exec(*arg);
        filesys_lock_release();
    }
    else{
        exit(-1);
    }
}

inline int
wait(tid_t pid)
{
    return process_wait(pid);
}

static void sys_wait(struct intr_frame *f)
{
    int *arg = f->esp + 4;
    if(check_pointer(arg)){
        f->eax = wait(*arg);
    }
    else{
        exit(-1);
    }
}

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
    if(check_pointer(arg1) && check_pointer(*arg1) && check_pointer(arg2)){
        filesys_lock_acquire();
        f->eax = create(*arg1, *arg2);
        filesys_lock_release();
    }
    else{
        exit(-1);
    }
}

inline bool
remove(const char *file)
{
    return filesys_remove(file);
}

static void
sys_remove(struct intr_frame *f)
{
    char **arg = f->esp + 4;
    if(check_pointer(arg) && check_pointer(*arg)){
        filesys_lock_acquire();
        f->eax = remove(*arg);
        filesys_lock_release();
    }
    else{
        exit(-1);
    }
}

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
    if(check_pointer(arg) && check_pointer(*arg)){
        filesys_lock_acquire();
        f->eax = open(*arg);
        filesys_lock_release();
    }
    else{
        exit(-1);
    }
}

inline int
filesize(int fd)
{
    struct file_descriptor *fileDescriptor = fd_find(fd);
    if(!fileDescriptor) exit(-1);
    return file_length(fileDescriptor->file_ptr);
}

static void sys_filesize(struct intr_frame *f)
{
    int *arg = f->esp + 4;
    if(check_pointer(arg)){
        filesys_lock_acquire();
        f->eax = filesize(*arg);
        filesys_lock_release();
    }
    else{
        exit(-1);
    }

}

int
read(int fd, void *buffer, unsigned length)
{
    if(fd==0){
        for(int i = 0; i < length; ++i){
            *(char *)(buffer+i) = input_getc();
        }
        return length;
    }
    else{
        struct file_descriptor *fileDescriptor = fd_find(fd);
        if(!fileDescriptor) exit(-1);
        return file_read(fileDescriptor->file_ptr, buffer, length);
    }
}

static void
sys_read(struct intr_frame *f)
{
    int *arg1 = f->esp + 4;
    void **arg2 = f->esp + 8;
    unsigned *arg3 = f->esp + 12;

    if(check_pointer(arg1) && check_pointer(arg2) && check_pointer(*arg2) && check_pointer(arg3) ){
        filesys_lock_acquire();
        f->eax = read(*arg1, *arg2, *arg3);
        filesys_lock_release();
    }
    else{
        exit(-1);
    }
}

int
write(int fd, const void *buffer, unsigned length)
{
    if(fd==1){
        putbuf(buffer, length);
        return length;
    }
    else{
        struct file_descriptor *fileDescriptor = fd_find(fd);
        if(!fileDescriptor) exit(-1);
        return file_write(fileDescriptor->file_ptr, buffer, length);
    }
}

static void
sys_write(struct intr_frame *f)
{
    int *arg1 = f->esp + 4;
    void **arg2 = f->esp + 8;
    unsigned *arg3 = f->esp + 12;

    if(check_pointer(arg1) && check_pointer(arg2) && check_pointer(*arg2) && check_pointer(arg3) ){
        filesys_lock_acquire();
        f->eax = write(*arg1, *arg2, *arg3);
        filesys_lock_release();
    }
    else{
        exit(-1);
    }
}

void
seek(int fd, unsigned position)
{
    struct file_descriptor *fileDescriptor = fd_find(fd);
    if(!fileDescriptor) exit(-1);;
    file_seek(fileDescriptor->file_ptr, position);
}

static void
sys_seek(struct intr_frame *f)
{
    int *arg1 = f->esp + 4;
    unsigned *arg2 = f->esp + 8;
    if(check_pointer(arg1) && check_pointer(arg2)){
        filesys_lock_acquire();
        seek(*arg1, *arg2);
        filesys_lock_release();
    }
    else{
        exit(-1);
    }
}

unsigned
tell(int fd)
{
    struct file_descriptor *fileDescriptor = fd_find(fd);
    if(!fileDescriptor) exit(-1);;
    return file_tell(fileDescriptor->file_ptr);
}

static void
sys_tell(struct intr_frame *f)
{
    int *arg = f->esp + 4;
    if(check_pointer(arg)){
        filesys_lock_acquire();
        f->eax = tell(*arg);
        filesys_lock_release();
    }
    else{
        exit(-1);
    }
}

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
    if(check_pointer(arg)){
        filesys_lock_acquire();
        close(*arg);
        filesys_lock_release();
    }
    else{
        exit(-1);
    }
}