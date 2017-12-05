#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>

int count = 0;
int limit = 5;

struct monitored_value
{
  sem_t sem_read, sem_write;
  int value;
};

int main(int argc, char *argv[])
{
  int shm_ID;
  struct monitored_value *my_shared_value;
  shm_ID = shmget(ftok("mysharedmem",4), getpagesize(), 0600);
  my_shared_value = (struct monitored_value *)
                        shmat(shm_ID, NULL, 0);
  
  sem_post(&(my_shared_value->sem_write));                    
  while(count < limit)
  {  
    sem_wait(&(my_shared_value->sem_read));
    printf("Reader Process: %dth Loop\n", (my_shared_value->value)+1);
    sem_post(&(my_shared_value->sem_write));
    count++;
  }
  
  shmdt((void*) my_shared_value);
  _exit(0);
}
