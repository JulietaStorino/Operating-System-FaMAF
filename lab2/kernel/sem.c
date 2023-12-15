#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

typedef enum {CLOSE, OPEN} state_t;

typedef struct semaphore_s 
{
  struct spinlock lock;
  int value;
  state_t status; 
} semaphore;

semaphore semaphores[MAXSEM];

void 
seminit(void)
{
  for(unsigned int sem = 0; sem < MAXSEM; ++sem) {
    initlock(&semaphores[sem].lock, "sem");
    semaphores[sem].value = 0;
    semaphores[sem].status = CLOSE;
  }
}

//Searches for a closed semaphore
int 
get_closed_sem(void)
{
  int sem = 0, found = 0;

  while( !found && sem + 1 < MAXSEM){
    acquire(&semaphores[sem].lock);

    if (semaphores[sem].status == OPEN){
      sem++;
      release(&semaphores[sem - 1 ].lock);
    } else {
      found = 1;
      release(&semaphores[sem].lock);
    }
  }

  return sem;
}

//Open-initialize the semaphore "sem" with an arbitrary value "value"
int 
sem_open(int sem, int value)
{
  // If semaphore is already open or ID is incorrect or value is negative
  if(semaphores[sem].status == OPEN || sem < 0 || sem > MAXSEM || value < 0){
    return 0; // Return error
  }

  acquire(&semaphores[sem].lock); // Acquire semaphore lock

  semaphores[sem].value = value; // Set semaphore value to user input value
  semaphores[sem].status = OPEN; // Set semaphore state to open

  release(&semaphores[sem].lock); // Release semaphore lock

  return 1; // Return success
}

//Free the semaphore "sem"
int 
sem_close(int sem)
{
  acquire(&semaphores[sem].lock); // Acquire semaphore lock

  if(sem < 0 || sem >= MAXSEM){ // If semaphore still being use or ID is incorrect
    release(&semaphores[sem].lock); // Release semaphore lock
    return 0; // Return error
  } 
    
  semaphores[sem].status = CLOSE; // Close semaphore
  release(&semaphores[sem].lock); // Release semaphore lock

  return 1;
}

//Increase the semaphore "sem" unlocking the process when it value is 0.
int 
sem_up(int sem)
{
  acquire(&semaphores[sem].lock); //locked - start critical zone

  if (semaphores[sem].value == 0){
    wakeup(&semaphores[sem]);
  }

  semaphores[sem].value = semaphores[sem].value  + 1;
  release(&semaphores[sem].lock); //unlocked - end critical zone

  return 1;
}

//Decrease the semaphore "sem" unlocking the process when it value is 0.
int
sem_down(int sem)
{
  acquire(&semaphores[sem].lock); //locked - start critical zone
      
  while(semaphores[sem].value == 0)
    sleep(&semaphores[sem], &semaphores[sem].lock);
      
  semaphores[sem].value--;
  release(&semaphores[sem].lock); //unlocked - end critical zone

  return 1;
}