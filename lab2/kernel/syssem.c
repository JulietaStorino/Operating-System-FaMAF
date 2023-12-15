// Semaphore system call.
#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64 
sys_sem_open(void)
{
  int sem, value;
  argint(0, &sem); // Get semaphore ID from user input
  argint(1, &value); // Get semaphore value from user input
  
  return sem_open(sem, value); // Call sem_open function with semaphore ID and value
}

uint64 
sys_sem_close(void)
{
  int sem;
  argint(0, &sem);

  return sem_close(sem);
}

uint64 
sys_sem_up(void)
{
  int sem;
  argint(0, &sem);

  return sem_up(sem);
}

uint64 
sys_sem_down(void)
{
  int sem;
  argint(0, &sem);
  
  return sem_down(sem);
}

uint64
sys_get_closed_sem(void)
{
  return get_closed_sem();
}