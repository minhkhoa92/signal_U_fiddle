/*
int main(int argc, char *argv[])
{
  int shm_ID;
  struct monitored_value *my_shared_value;
  shm_ID = shmget(ftok("mysharedmem", 4), getpagesize(), IPC_CREAT | 0600);
  my_shared_value = (struct monitored_value *)
                        shmat(shm_ID, NULL, 0);  
  sem_init(&(my_shared_value->sem_read), 1, 0);
  sem_init(&(my_shared_value->sem_write), 1, 0);
  my_shared_value->value = count;
  while(count < limit)
  {  
    sem_wait(&(my_shared_value->sem_write));
    printf("Reader Process: %dth Loop\n", count+1);
    my_shared_value->value = count;
    sem_post(&(my_shared_value->sem_read));
    count++;
  }
  sem_destroy(&(my_shared_value->sem_write));
  sem_destroy(&(my_shared_value->sem_read));
  shmdt((void*) my_shared_value);
  shmctl(shm_ID, IPC_RMID, NULL);
  _exit(0);
}

*/
/*
 * Copied from https://www.softprayog.in/programming/interprocess-communication-using-posix-shared-memory-in-linux
 * Copyright © 2007-2017 SoftPrayog.in. All Rights Reserved.
 *      
 * Suggestion to use malloc_stats() from www.linuxjournal.com/article/6390
 * author: Luu Minh Khoa Ngo
 * Date created Dec/04/2017
 * Date updated Dec  5 2017
 * Description: This is an exercise. It should first start new processes clients
 * which are controlled by one program. The clients are classic consumer and 
 * producers which have are solved by having the controller, which is the same
 * as a monitor control them by sending signals.
 * The consumer and producers write and read messages (c-strings).
 * This part is the creator and the monitor of the consumer and producers.
 * It has to create everything, sends the order to continue a program and
 * receives the order to continue. To determine the sequence of execution of 
 * producer and consumers, it takes the order from commandline.
 * The loop is executed 10 times, so the command line is short, but the
 * execution long and repetitive.
 * usage: command <no>|reader <no>|reader <no>|reader <no>|reader 
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
//for signal constants, pause() and kill()
#include <signal.h>

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

#define IS_READER true
#define IS_WRITER false
#define COUNT_CHILDREN 10

#define S_CONST_READER "reader"
struct shared_memory {
    char buf [MAX_BUFFERS] [256];
    int buffer_index;
    int buffer_print_index;
};

//printing an error via perror to stderr
//AND exit with status 1 (what a waste of info)
void error (char *msg);

void arg_to_task(const char *arg, bool *is_read, int *writer_no);

void quit_handler(int signo);
void child_is_ready(int signo);

void execute_action(int argno);

int IND_OUTERLOOP = 0;
int IND_OUTERLOOP_MAX = 10;

int IND_IN_COMMAND_ARG = 1;
int IND_IN_COMMAND_ARG_MIN = 1;
int IND_IN_COMMAND_ARG_MAX;

int pid_children[COUNT_CHILDREN];
int counter = children_ready = 0; //counter for active readers, counter for initialized children, ready to compute
char **glob_argv;
sem_t *mutex_sem, *sem_mult_reader_start, *spool_signal_sem;
struct shared_memory *shared_mem_ptr;
int fd_shm;

int main (int argc, char **argv)
{
    IND_IN_COMMAND_ARG_MAX = argc;
    glob_argv = argv;
   
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

   //  mutual exclusion semaphore, mutex_sem with an initial value 0.
    if ((mutex_sem = sem_open (SEM_MUTEX_NAME, O_CREAT, 0660, 0)) == SEM_FAILED)
        error ("sem_open");
    
    // Initialize the shared memory, I do not need that in this code
    //shared_mem_ptr -> buffer_index = shared_mem_ptr -> buffer_print_index = 0;

    // counting semaphore, indicating the number of reader to start. Initial value = 0, so that no reader may start
    if ((sem_mult_reader_start = sem_open (SEM_BUFFER_COUNT_NAME, O_CREAT, 0660,
        0)) == SEM_FAILED)
        error ("sem_open");

    // counting semaphore, indicating the number of strings to be printed. Initial value = 0, so no strings to be printed
    if ((spool_signal_sem = sem_open (SEM_SPOOL_SIGNAL_NAME, O_CREAT, 0660, 
        0)) == SEM_FAILED)
        error ("sem_open");
    signal(SIGUSR1, child_is_ready);
    signal(SIGUSR2, quit_handler);

    while( IND_OUTERLOOP < COUNT_CHILDREN )
    {
        int id = fork();
        if(id > 0)
        {
            pid_children[IND_OUTERLOOP] = id;
            if(IND_OUTERLOOP % 2)
            {
                execve("./reader", "reader", "This is reader no", NULL);
            }
            else
            {
                execve("./writer", "writer", "This is writer no", NULL);
            }
        }
    }


    //this needs to wait for the children to be ready
    while(children_ready < COUNT_CHILDREN)
    {
        //waits for the next signal
        pause();
    }
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
    while(1)
    {
        //waits for the next signal
        pause();
    }
   

} //I realize there is not exit, sorry for that, dear Linux prof

// Print system error and exit
void error (char *msg)
{
    perror (msg);
    exit (1);
}

void clear_ressources()
{
    /*since I prefer to clean my system ressources*/
    if ( munmap ( shared_mem_ptr, sizeof (struct shared_memory) ) == -1 )
    error ("munmap");

    if ( close(fd_shm) == -1 )
        error ("close(shm)");
    if ( shm_unlink(SHARED_MEM_NAME) == -1 )
        error ("shm_unlink");

    if ( sem_close(mutex_sem) == -1 )
        error ("sem_close");
    if ( sem_close(sem_mult_reader_start) == -1 )
        error ("sem_close");
    if ( sem_close(spool_signal_sem) == -1 )
        error ("sem_close");
    if ( sem_unlink( SEM_MUTEX_NAME ) == -1 ) //"/sem-mutex"
        error ("sem_unlink");
    if ( sem_unlink( SEM_BUFFER_COUNT_NAME ) == -1 ) //"/sem-buffer-count"
        error ("sem_unlink");
    if ( sem_unlink( SEM_SPOOL_SIGNAL_NAME ) == -1 ) //"/sem-spool-signal"
        error ("sem_unlink");
 }

void quit_handler(int signo)
{
//critical section
    sem_wait(sem_mutex);
    if(counter > 0)
    {
        counter--;
        sem_post(sem_mutex),
        return;
    }
    //outerloop reaches limit
    if( IND_OUTERLOOP >= IND_OUTERLOOP_MAX )
    {
        for(
        IND_IN_COMMAND_ARG = 0;
        IND_IN_COMMAND_ARG < COUNT_CHILDREN; 
        IND_IN_COMMAND_ARG++) 
        {
            //SIGUSR2 is used to exit all children
            if ( kill(pid_children[IND_IN_COMMAND_ARG], SIGUSR2)  == -1 )
            {
                error("kill");
            }
        }
        clear_ressources();
        exit(0);
    }

    //else continue execution
    execute_action(IND_IN_COMMAND_ARG++);
    if(IND_IN_COMMAND_ARG >= IND_IN_COMMAND_ARG_MAX)
    {
        IND_IN_COMMAND_ARG = IND_IN_COMMAND_ARG_MIN; 
        IND_OUTERLOOP++;
    }
    //when execution is done return to main.
}

void execute_action(int argno)
{
    //taking the string of the arg value
    char *arg = glob_argv[argno];
    //comparing it, if it is an reader
    if(strcmp(arg, S_CONST_READER ) == 0)
    {
        //if it is a reader command
        int i;
        //wake all readers
        for(
        i = 1;
        i < COUNT_CHILDREN; 
        i += 2) 
        {
            //SIGUSR1 is used to signal a "continue task" message, so the child does something
            //kill returns 0 or -1 for success and failure
            if ( kill(pid_children[i], SIGUSR1)  == -1 )
            {
                error("kill");
            }
            //critical section, since it determines when a reader may start reading
            counter++;
        }
        //let readers read and send back answer
        for( i = 0; i < COUNT_READER / 2; ++i ) 
            sem_post (sem_mult_reader_start);
        //let readers decrease the counter
        sem_post(sem_mutex),
    }
    else
    {
        //if it is a writer command, it will be a number
        //get number
        //make the number a number between 0 and 4
        //multiply it by 2 to get an index in the children pid_list
        int childno = ( strtol(arg, NULL, 10) % 5 ) * 2;
        //kill returns 0 or -1 for success and failure
        //SIGUSR1 is used as specific signal to make the reader and writer do
        //something.
        if ( kill(pid_children[childno], SIGUSR1)  == -1 )
        {
            error("kill");
        }
    }
}
void child_is_ready(int signo)
{
    children_ready++;

}
