

/*
 * Copied from https://www.softprayog.in/programming/interprocess-communication-using-posix-shared-memory-in-linux
 * Copyright © 2007-2017 SoftPrayog.in. All Rights Reserved.
 *      
 * Suggestion to use malloc_stats() from www.linuxjournal.com/article/6390
 */

//input output commands
#include <stdio.h>
//contains malloc(...) and free(...)
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
//includes _exit for sudden exit
#include <unistd.h>
#include <semaphore.h>
#include <sys/mman.h>
//for malloc_stats() which writes to stderr
#include <malloc.h>
//for uint16_t, 32_t, 64_t declarations
#include <stdint.h>

// Buffer data structures
#define MAX_BUFFERS 10

//not really needed
//before I happened to stumble on this code, this file
//was used as log, to see the messages written by clients
//It was written with the usual open, write, close commands
#define LOGFILE "/tmp/example.log"

#define SEM_MUTEX_NAME "/sem-mutex"
#define SEM_BUFFER_COUNT_NAME "/sem-buffer-count"
#define SEM_SPOOL_SIGNAL_NAME "/sem-spool-signal"
#define SHARED_MEM_NAME "/posix-shared-mem-example"

#define VALUE_ALLOC_2k 2048
#define VALUE_ALLOC_4k 4096
#define VALUE_ALLOC_8k 8192
struct shared_memory {
    char buf [MAX_BUFFERS] [256];
//hello
    int buffer_index;
    int buffer_print_index;
};

//printing an error via perror to stderr
//AND exit with status 1 (what a waste of info)
void error (char *msg);

int main (int argc, char **argv)
{
    struct shared_memory *shared_mem_ptr;
// hello, i do not need those, but I have to know what they do
    sem_t *mutex_sem, *buffer_count_sem, *spool_signal_sem;
//as i said
    int fd_shm;
    char mybuf [256];
    void *dyn_mem;
    printf("no alloc:\n--------\n");
    malloc_stats();
    getchar();

   
    // Get shared memory 
    if ((fd_shm = shm_open (SHARED_MEM_NAME, O_RDWR | O_CREAT, 0660)) == -1)
        error ("shm_open");

    // Setting the size for the shared memory
    if (ftruncate (fd_shm, sizeof (struct shared_memory)) == -1)
       error ("ftruncate");
    
    // mapping the shared memory to process memory
    if ((shared_mem_ptr = mmap (NULL, sizeof (struct shared_memory), PROT_READ | PROT_WRITE, MAP_SHARED,
            fd_shm, 0)) == MAP_FAILED)
       error ("mmap");

    //looking at malloc
    printf("\nno alloc->shmem allocated:\n--------\n");
    printf("size of (struct shared_memory): %ld\n--------\n",
            sizeof(struct shared_memory) );
    malloc_stats();
    getchar();

    //using an actual malloc
    dyn_mem = malloc( VALUE_ALLOC_4k );
    //looking at malloc
    printf("\nno alloc->shmem allocated->malloc:\n--------\n");
    malloc_stats();
    getchar();

   //  mutual exclusion semaphore, mutex_sem with an initial value 0.
    if ((mutex_sem = sem_open (SEM_MUTEX_NAME, O_CREAT, 0660, 0)) == SEM_FAILED)
        error ("sem_open");
    
    // Initialize the shared memory, I do not need that in this code
    //shared_mem_ptr -> buffer_index = shared_mem_ptr -> buffer_print_index = 0;

    // counting semaphore, indicating the number of available buffers. Initial value = MAX_BUFFERS
    if ((buffer_count_sem = sem_open (SEM_BUFFER_COUNT_NAME, O_CREAT, 0660, 
        MAX_BUFFERS)) == SEM_FAILED)
        error ("sem_open");

    // counting semaphore, indicating the number of strings to be printed. Initial value = 0
    if ((spool_signal_sem = sem_open (SEM_SPOOL_SIGNAL_NAME, O_CREAT, 0660, 
        0)) == SEM_FAILED)
        error ("sem_open");

    //looking at malloc
    printf("\nmore variables looked at:\n--------\n");
    malloc_stats();
    getchar();

    //I do not need that
    // Initialization complete; now we can set mutex semaphore as 1 to 
    // indicate shared memory segment is available
    //if (sem_post (mutex_sem) == -1)
        //error ("sem_post: mutex_sem");

//no action needed
/*    
    while (1) {  // forever
        // Is there a string to print? P (spool_signal_sem);
        if (sem_wait (spool_signal_sem) == -1)
            error ("sem_wait: spool_signal_sem");
    
        strcpy (mybuf, shared_mem_ptr -> buf [shared_mem_ptr -> buffer_print_index]);

//*        /* Since there is only one process (the logger) using the 
 //      buffer_print_index, mutex semaphore is not necessary *//*
        (shared_mem_ptr -> buffer_print_index)++;
        if (shared_mem_ptr -> buffer_print_index == MAX_BUFFERS)
           shared_mem_ptr -> buffer_print_index = 0;

//    /*    /* Contents of one buffer has been printed.
           One more buffer is available for use by producers.
  //         Release buffer: V (buffer_count_sem);  *//*
        if (sem_post (buffer_count_sem) == -1)
            error ("sem_post: buffer_count_sem");
        
    }
//*/
    /*since I prefer to clean my system ressources*/
if ( munmap ( shared_mem_ptr, sizeof (struct shared_memory) ) == -1 )
    error ("munmap");
    //looking at malloc
    printf("\nno alloc->shmem allocated->malloc->munmap:\n--------\n");
    malloc_stats();
    getchar();

    if ( close(fd_shm) == -1 )
        error ("close(shm)");
    if ( shm_unlink(SHARED_MEM_NAME) == -1 )
        error ("shm_unlink");

    if ( sem_close(mutex_sem) == -1 )
        error ("sem_close");
    if ( sem_close(buffer_count_sem) == -1 )
        error ("sem_close");
    if ( sem_close(spool_signal_sem) == -1 )
        error ("sem_close");
    if ( sem_unlink( SEM_MUTEX_NAME ) == -1 ) //"/sem-mutex"
        error ("sem_unlink");
    if ( sem_unlink( SEM_BUFFER_COUNT_NAME ) == -1 ) //"/sem-buffer-count"
        error ("sem_unlink");
    if ( sem_unlink( SEM_SPOOL_SIGNAL_NAME ) == -1 ) //"/sem-spool-signal"
        error ("sem_unlink");
    
    //looking at malloc
    printf("\nclearing the different types:\n--------\n");
    malloc_stats();
    getchar();

    //freeing the dyn variable
    printf("\n dyn_mem variable 0x%lux \n---\nfree\n---\n", (uint64_t) dyn_mem);
    free(dyn_mem);
    //out of curiosity
    printf("\n dyn_mem variable 0x%lux \n", (uint64_t) dyn_mem);

    //looking at malloc
    printf("\nno alloc->shmem allocated->malloc->munmap->free previous malloc:\n--------\n");
    malloc_stats();
    getchar();
} //I realize there is not exit, sorry for that dear Linux prof

// Print system error and exit
void error (char *msg)
{
    perror (msg);
    exit (1);
}

