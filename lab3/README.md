# Primera parte: Estudiando el planificador de xv6-riscv

## 1. ¿Qué política de planificación utiliza xv6-riscv para elegir el próximo proceso a ejecutarse?

La política de planificación utilizada por **xv6-riscv**, es de tipo **[Round Robin (RR)](https://es.wikipedia.org/wiki/Planificaci%C3%B3n_Round-robin#:~:text=Round%2DRobin%20es%20un%20algoritmo,procesos%20con%20la%20misma%20prioridad.)**, donde se elige el primer proceso de la tabla de proc table que pueda ser ejecutado y se ejecuta un tiempo determinado o hasta que el proceso le transfiera control (lo que ocurra primero). 

> proc.c - scheduler()

```c
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  
  c->proc = 0;
  for(;;){
    // Avoid deadlock by ensuring that devices can interrupt.
    intr_on();

    for(p = proc; p < &proc[NPROC]; p++) {
      acquire(&p->lock);
      if(p->state == RUNNABLE) {
        // Switch to chosen process.  It is the process's job
        // to release its lock and then reacquire it
        // before jumping back to us.
        p->schedpick = p->schedpick + 1;
        acquire(&tickslock);
        p -> lastexec = ticks;
        release(&tickslock);
        p->state = RUNNING;
        c->proc = p;
        swtch(&c->context, &p->context);

        // Process is done running for now.
        // It should have changed its p->state before coming back.
        c->proc = 0;
      }
      release(&p->lock);
    }
  }
}
```

## 2. ¿Cuánto dura un quantum en xv6-riscv?

Un quantum tiene duración de **1000000 ciclos de clock** (cerca de **1/10th second en qemu**). 

> start.c - timerinit()

```c
// arrange to receive timer interrupts.
// they will arrive in machine mode at
// at timervec in kernelvec.S,
// which turns them into software interrupts for
// devintr() in trap.c.
void
timerinit()
{
// each CPU has a separate source of timer interrupts.
int id = r_mhartid();

// ask the CLINT for a timer interrupt.
int interval = 1000000; // cycles; about 1/10th second in qemu.
*(uint64*)CLINT_MTIMECMP(id) = *(uint64*)CLINT_MTIME + interval;

// prepare information in scratch[] for timervec.
// scratch[0..2] : space for timervec to save registers.
// scratch[3] : address of CLINT MTIMECMP register.
// scratch[4] : desired interval (in cycles) between timer interrupts.
uint64 *scratch = &timer_scratch[id][0];
scratch[3] = CLINT_MTIMECMP(id);
scratch[4] = interval;
w_mscratch((uint64)scratch);

// set the machine-mode trap handler.
w_mtvec((uint64)timervec);

// enable machine-mode interrupts.
w_mstatus(r_mstatus() | MSTATUS_MIE);

// enable machine-mode timer interrupts.
w_mie(r_mie() | MIE_MTIE);
}
```

## 3. ¿Cuánto dura un cambio de contexto en xv6-riscv?

Un cambio de contexto en xv6-riscv **dura 28 ciclos de reloj**. 

> swtch.S - scheduler()

```c
# Context switch
#
#   void swtch(struct context *old, struct context *new);
# 
# Save current registers in old. Load from new.	

.globl swtch
swtch:
        sd ra, 0(a0)
        sd sp, 8(a0)
        sd s0, 16(a0)
        sd s1, 24(a0)
        sd s2, 32(a0)
        sd s3, 40(a0)
        sd s4, 48(a0)
        sd s5, 56(a0)
        sd s6, 64(a0)
        sd s7, 72(a0)
        sd s8, 80(a0)
        sd s9, 88(a0)
        sd s10, 96(a0)
        sd s11, 104(a0)

        ld ra, 0(a1)
        ld sp, 8(a1)
        ld s0, 16(a1)
        ld s1, 24(a1)
        ld s2, 32(a1)
        ld s3, 40(a1)
        ld s4, 48(a1)
        ld s5, 56(a1)
        ld s6, 64(a1)
        ld s7, 72(a1)
        ld s8, 80(a1)
        ld s9, 88(a1)
        ld s10, 96(a1)
        ld s11, 104(a1)
        
        ret
```
**Idea para medir el cambio de contexto**

Dado que un cambio de contexto consume tiempo de un quantum, si disminuímos la duración del mismo hasta lograr que ningún proceso pueda ser ejecutado sabremos que todo el tiempo de quanto se está consumiendo al realizar esta acción. Conociendo este valor de intervalo, podemos hacer la equivalencia a una métrica de tiempo y obtener el resultado deseado.

Se listan los intervalos en el orden en que se llevaron a cabo los experimentos.

|  Intervalo  |  Se llega a ejecutar los procesos ?  |
| ------------ | ------------ |
|  1000000  |  Si  |
|  1000  |  Si  |
|  100  |  No  |
|  500  |  Si  |
|  400  |  No  |
|  450  |  No  |
|  470  |  Si  |
|  460  |  No  |



## 4. ¿El cambio de contexto consume tiempo de un quantum?

El cambio de contexto si consume tiempo. Al momento en que se produce una interrupción de timer, se determina el momento de la próxima antes de pasar al trap que maneja la interrupción. El tiempo del quántum ya está siendo utilizado y todavía debemos pasar por yield, que es en donde llamamos a sched  para realizar el primer cambio de contexto y regresar al planificador. Es en este último que cambiamos al nuevo proceso y para ese entonces ya hemos utilizado una (pequeña) parte del quantum.

> proc.c - scheduler()

```c
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  
  c->proc = 0;
  for(;;){
    // Avoid deadlock by ensuring that devices can interrupt.
    intr_on();

    for(p = proc; p < &proc[NPROC]; p++) {
      acquire(&p->lock);
      if(p->state == RUNNABLE) {
        // Switch to chosen process.  It is the process's job
        // to release its lock and then reacquire it
        // before jumping back to us.
        p->schedpick = p->schedpick + 1;
        acquire(&tickslock);
        p -> lastexec = ticks;
        release(&tickslock);
        p->state = RUNNING;
        c->proc = p;
        swtch(&c->context, &p->context);

        // Process is done running for now.
        // It should have changed its p->state before coming back.
        c->proc = 0;
      }
      release(&p->lock);
    }
  }
}
```

## 5. ¿Hay alguna forma de que a un proceso se le asigne menos tiempo?

Si, si hay forma de que a un proceso se le asigne menos tiempo.
El **SO** asigna quantum de **tiempo globalmente** a procesos a través de un temporizador, en lugar de asignar a cada proceso **individualmente**. Esto puede llevar a que un **proceso termine antes** de que termine su quantum, resultando en que el próximo proceso elegido por el programador tenga menos tiempo asignado, ya que **comienza con el tiempo no utilizado del** proceso **anterior**.

## 6. ¿Cúales son los estados en los que un proceso pueden permanecer en xv6- riscv y que los hace cambiar de estado?

Estados posibles: UNUSED, USED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE.

| Estado previo | Función     | Estado Actual | Descripción |
| ------------ | ------------ | ------------ | ------------ |
| UNUSED        | allocproc() | USED          | Al inicializar la tabla de procesos |
|              |             |               | en procinit, establecemos el estado |
|             |             |               | de cada uno de ellos como UNUSED. |
|             |             |               | Cuando necesitamos un nuevo proceso y |
|              |             |               | usamos allocproc, recorremos la tabla |
|             |             |               | de procesos y tomamos el primero con |
|             |             |               | UNUSED. Modificamos al mismo para |
|              |             |               | indicar que está en uso (USED) . |
|   |   |   |   |
| USED          | userinit()  | RUNNABLE      | Se establece el primer proceso de |
|              |             |               | usuario. Al marcarlo como RUNNABLE, |
|              |             |               | el scheduler lo toma en cuenta al |
|             |             |               | momento de elegir qué proceso |
|              |             |               | ejecutar en un quantum dado. |
|   |   |   |   |
| USED          | fork()      | RUNNABLE      | Buscamos un proceso que no esté en |
|               |             |               | uso con allocproc y establecemos su |
|               |             |               | estado como RUNNABLE para que sea  |
|               |             |               | ejecutado por el planificador. |
|   |   |   |   |
|               | yield()     | RUNNABLE      | Como se va a llamar al planificador |
|               |             |               | cambio el estado del proceso a RUNNABLE |
|   |   |   |   |
| RUNNABLE      | scheduler() | RUNNING       | Al elegir un proceso para ejecutar, |
|               |             |               | nos aseguramos que su estado |
|               |             |               | represente el hecho de que se está  |
|               |             |               | ejecutando en un momento dado. |
|   |   |   |   |
| RUNNING       | sleep       | SLEEPING      |  |
|               |             |               |   |
|               |             |               |   |
|               |             |               |   |
|   |   |   |   |
| SLEEPING      | wakeup()    | RUNNABLE      | Cambiamos el estado a RUNNABLE para |
|               |             |               | que pueda ser elegible por el |
|               |             |               | planificador. |
|   |   |   |   |
| SLEEPING      |kill()       | RUNNABLE      |  |
|               |             |               |   |
|               |             |               |   |
|               |             |               |   |
|   |   |   |   |
| RUNNING       |exit         | ZOMBIE        |  |
|               |             |               |   |
|               |             |               |   |
|               |             |               |   |
|   |   |   |   |
| USED , ZOMBIE |freeproc     | UNUSED        | Ya sea que pedimos un proceso y no se  |
|               |             |               |  pudo asignar memoria para el mismo o  |
|               |             |               |si alguno de los procesos hijos (en caso  | 
|               |             |               | que tuviera) ha finalizado de ejecutar |
|               |             |               |, freeproc marca a tales procesos como |
|               |             |               | UNUSED para que puedan ser adquiridos  |
|               |             |               | nuevamente con allocproc.  

# Segunda parte

Se incorpora en proc.h a struct proc tres nuevos miembros: schedpick, lastexec y priority.

> proc.h
```c
struct proc {
  struct spinlock lock;

  // p->lock must be held when using these:
  enum procstate state;        // Process state
  void *chan;                  // If non-zero, sleeping on chan
  int killed;                  // If non-zero, have been killed
  int xstate;                  // Exit status to be returned to parent's wait
  int pid;
  int schedpick;               // Number of times the process was picked by the scheduler
  int lastexec;                // Last time it was executed
  int priority;                // Priority of a process
  ...
}
```
Estos son utilizados tanto en la implementación de una nueva llamada al sistema pstat como en la modificación de la función  procdump:

> proc.c - procdump
```c
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [USED]      "used",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  struct proc *p;
  char *state;

  printf("\n");
  for(p = proc; p < &proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    printf("%d %s %s %d %d", p->pid, state, p->name, p->schedpick, p->priority);
    printf("\n");
  }
}
```

> proc.c - pstat
```c
int 
pstat(int pid)
{
  struct proc *p;

  printf("\nPID\tPRIORITY\tN_TIMES_CHOSEN\tLAST_EXECUTION\n");

  for(p = proc; p < &proc[NPROC]; p++){
    if(p->pid == pid){
      printf("%d\t%d\t\t%d\t\t%d", p->pid,p->priority, p->schedpick, p->lastexec);
      printf("\n");
    } 
  }

  return 0;
}
```
## Prioridad 

Se busca tener tres niveles distintos de prioridad: 0, 1 y 2 (máximo). En primer lugar, se define en  proc.h a NPRIO  como una constante de valor 3. Como se han establecido que cuando un proceso se inicia, su prioridad debe ser máxima, en allocproc le asignamos a cada proceso la prioridad NPRIO-1.
A su vez se indica que la prioridad debe descender cada vez que el proceso pasa todo un quatum relizando cómputo, y ascender cuando se bloquea antes de terminar su quantum.
Interpretamos que como lo que ocurre luego que un proceso use todo su quantum es un timer interrupt que produce que el proceso ceda el CPU, en la función yield() nos podemos encargar de disminuir la prioridad de la siguiente forma.

> proc.c - yield
```c
void
yield(void)
{
  struct proc *p = myproc();
  acquire(&p->lock);
  if (p->priority > 0)
    p->priority--;
  p->state = RUNNABLE;
  sched();
  release(&p->lock);
}
```

A su vez, el hecho de que un proceso se bloquee antes de terminar su quantum sucede en xv6 cuando se produce una llamada a sleep(). Por lo tanto se realizan las siguientes modificaciones a la misma:

> proc.c - sleep
```c
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();


  acquire(&p->lock);  //DOC: sleeplock1
  release(lk);

  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  if (p->priority < NPRIO-1)
    p->priority++;
  
  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  release(&p->lock);
  acquire(lk);
}
```

## Implementando MLFQ

Teniendo en cuenta que lo qque buscamos es que nuestro planificador siempre elija al proceso con mayor prioridad que se ha ejecutado menos veces, nuestra idea se basó en recorrer la tabla de procesos mientras realizamos comparaciones para dar con tal proceso.
El algortimo que se puede ver en la función scheduler() en proc.c de la rama mlfq no es la mejor implementación de esta política y tiene ciertos problemas cuando disminuímos el valor del quantum(*). Por una cuestión de tiempo avanzamos haciendo las mediciones.


## Experimento con quantum por defecto

**Hardware**: Intel® Core™ i3-1005G1  
**Política**: Round Robin

### Caso 1: 1 iobench solo
**Instrucción**: iobench

Característica | Primer ejecución | Segunda ejecución | Tercera ejecución | Cuarta ejecución | Quinta ejecución
 :------------: | :------------: | :------------: | :------------: | :------------: | :------------:
Promedio OPW/100T | 7,339.79 | 3,377.79 | 2,651.16 | 3,309.44 | 7,093.89
Promedio OPR/100T | 7,339.79 | 3,377.79 | 2,651.16 | 3,309.44 | 7,093.89
Veces elegidas | 500288 | 492106 | 502283 | 506852 | 505041

### Caso 2: 1 cpubench solo
**Instrucción**: cpubench

Característica | Primer ejecución | Segunda ejecución | Tercera ejecución | Cuarta ejecución | Quinta ejecución
 :------------: | :------------: | :------------: | :------------: | :------------: | :------------:
Promedio MFLOPS/100T | 830.59 | 812.39 | 829.06 | 866.29 | 861.59
Veces elegidas | 2118 | 2112 | 2113 | 2115 | 2116

### Caso 3: 1 iobench con 1 cpubench
**Instrucción**: iobench &; cpubench

Característica | Primer ejecución | Segunda ejecución | Tercera ejecución | Cuarta ejecución | Quinta ejecución
 :------------: | :------------: | :------------: | :------------: | :------------: | :------------:
Promedio MFLOPS/100T | 860.82 | 859.41 | 855.87 | 853.12 | 859.52
Promedio OPW/100T | 36.22 | 36.22 | 36.22 | 36.00 | 36.22
Promedio OPR/100T | 36.22 | 36.22 | 36.22 | 36.00 | 36.22
Veces elegidas (iobench) | 2225 | 2224 | 2223 | 2224 | 2222
Veces elegidas (cpubench) | 2119 | 2111 | 2102 | 2121 | 2114

### Caso 4: 1 cpubench con 1 cpubench
**Instrucción**: cpubench &; cpubench

Característica | Primer ejecución | Segunda ejecución | Tercera ejecución | Cuarta ejecución | Quinta ejecución
 :------------: | :------------: | :------------: | :------------: | :------------: | :------------:
Promedio MFLOPS/100T (primer cpubench) | 964.28 | 965.33 | 997.58 | 996.57 | 989.32
Promedio MFLOPS/100T (segundo cpubench) | 965.33 | 998.63 | 998.63 | 997.59 | 990.42
Veces elegidas (primer cpubench) | 1058 | 1056 | 1056 | 1055 | 1057
Veces elegidas (segundo cpubench) | 1056 | 1056 | 1057 | 1058 | 1055

### Caso 5: 1 cpubench con 1 cpubench y 1 iobench.
**Instrucción**: cpubench &; cpubench &; iobench

Característica | Primer ejecución | Segunda ejecución | Tercera ejecución | Cuarta ejecución | Quinta ejecución
 :------------: | :------------: | :------------: | :------------: | :------------: | :------------:
Promedio MFLOPS/100T (primer cpubench) | 984.79 | 989.26 | 984.68 | 992.32 | 990.74
Promedio MFLOPS/100T (segundo cpubench) | 990.95 | 990.89 | 992.89 | 992.89 | 993.89
Promedio OPW/100T | 18.8 | 18.8 | 20.66 | 18.8 | 18.8
Promedio OPR/100T | 18.8 | 18.8 | 20.66 | 18.8 | 18.8
Veces elegidas (primer cpubench) | 1051 | 1052 | 1055 | 1053 | 1056
Veces elegidas (segundo cpubench) | 1057 | 1050 | 1062 | 1060 | 1052
Veces elegidas (iobench) | 1227 | 1227 | 1227 | 1227 | 1227

## Experimento con quantum 10 veces más chico

### Caso 1: 1 iobench solo
**Instrucción**: iobench

Característica | Primer ejecución | Segunda ejecución | Tercera ejecución | Cuarta ejecución | Quinta ejecución
 :------------: | :------------: | :------------: | :------------: | :------------: | :------------:
Promedio OPW/100T | 7,265.47 | 6,888.42 | 7,050.11 | 5,993.26 | 5,450.32
Promedio OPR/100T | 7,265.47 | 6,888.42 | 7,050.11 | 5,993.26 | 5,450.32
Veces elegidas | 467207 | 444868 | 454248 | 387498 | 355682

### Caso 2: 1 cpubench solo
**Instrucción**: cpubench

Característica | Primer ejecución | Segunda ejecución | Tercera ejecución | Cuarta ejecución | Quinta ejecución
 :------------: | :------------: | :------------: | :------------: | :------------: | :------------:
Promedio MFLOPS/100T | 589.67 | 491.75 | 444.36 | 424.07 | 207.18
Veces elegidas | 21068 | 21077 | 21476 | 21385 | 21262

### Caso 3: 1 iobench con 1 cpubench
**Instrucción**: iobench &; cpubench

Característica | Primer ejecución | Segunda ejecución | Tercera ejecución | Cuarta ejecución | Quinta ejecución
 :------------: | :------------: | :------------: | :------------: | :------------: | :------------:
Promedio MFLOPS/100T | 173.88 | 173.52 | 176.94 | 307.00 | 602.12
Promedio OPW/100T | 328.38 | 328.38 | 327.24 | 325.69 | 335.47
Promedio OPR/100T | 328.38 | 328.38 | 327.24 | 325.69 | 335.47
Veces elegidas (iobench) | 20720 | 20715 | 20769 | 20865 | 20894
Veces elegidas (cpubench) | 21453 | 21466 | 21002 | 20892 | 21230

### Caso 4: 1 cpubench con 1 cpubench
**Instrucción**: cpubench &; cpubench

Característica | Primer ejecución | Segunda ejecución | Tercera ejecución | Cuarta ejecución | Quinta ejecución
 :------------: | :------------: | :------------: | :------------: | :------------: | :------------:
Promedio MFLOPS/100T (primer cpubench) | 783.67 | 767.33 | 787.33 | 797.33 | 780.41
Promedio MFLOPS/100T (segundo cpubench) | 789.44 | 770.78 | 743.56 | 759.32 | 776.33
Veces elegidas (primer cpubench) | 10641 | 10613 | 10522 | 10553 | 10573
Veces elegidas (segundo cpubench) | 10547 | 10551 | 10592 | 10553 | 10502

### Caso 5: 1 cpubench con 1 cpubench y 1 iobench.
**Instrucción**: cpubench &; cpubench &; iobench

Característica | Primer ejecución | Segunda ejecución | Tercera ejecución | Cuarta ejecución | Quinta ejecución
 :------------: | :------------: | :------------: | :------------: | :------------: | :------------:
Promedio MFLOPS/100T (primer cpubench) | 872.33 | 838.67 | 867.4 | 869.22 | 857.08
Promedio MFLOPS/100T (segundo cpubench) | 911.09 | 918.18 | 906.1 | 911.27 | 901.33
Promedio OPW/100T (iobench) | 167.2 | 167.4 | 164.78 | 167.3 | 167.17
Promedio OPR/100T (iobench) | 167.2 | 167.4 | 164.78 | 167.3 | 167.17
Veces elegidas (primer cpubench) | 10572 | 10496 | 10568 | 10516 | 10487
Veces elegidas (segundo cpubench) | 10496 | 10586 | 10579 | 10486 | 10554
Veces elegidas (iobench) | 10649 | 10650 | 10648 | 10651 | 10651

## Conclusiones
En la segunda parte del análisis, se está observando cómo el planificador de procesos selecciona y afecta a los procesos en diferentes escenarios. Estos escenarios se dividen en varios casos (del 1 al 5) en los que se ejecutan programas de prueba, ya sea de E/S (iobench), de cálculos de punto flotante (cpubench) o una combinación de ambos.

**Caso 1: 1 iobench solo**
En este caso, se nota que el número promedio de operaciones de escritura disminuye en cada ejecución hasta la cuarta, cuando aumenta nuevamente, pero nunca supera el promedio inicial. En cuanto a las operaciones de lectura, su promedio es igual al de escritura en cada ejecución.

**Caso 2: 1 cpubench solo**
En esta situación, se observa que el promedio de MFLOPS/100T disminuye en cada ejecución hasta la tercera, momento en el que comienza a aumentar, superando el promedio inicial en la cuarta y quinta ejecución.

**Caso 3: 1 iobench con 1 cpubench**
En este caso, se destaca una drástica disminución en el promedio de OPW/100T y OPR/100T. En contrapartida, el promedio de MFLOPS/100T aumenta ligeramente.

**Caso 4: 1 cpubench con 1 cpubench**
En esta configuración, el promedio de MFLOPS/100T aumenta significativamente en más de 100 puntos en comparación con los otros casos. Además, hay poca variación en el promedio entre ambos procesos.

**Caso 5: 1 cpubench con 1 cpubench y 1 iobench**
En este escenario, el promedio de MFLOPS/100T aumenta aún más en comparación con el caso anterior, mientras que los promedios de OPW/100T y OPR/100T disminuyen en casi un tercio en comparación con el caso 3. También, hay mucha menos variación entre cada ejecución. En resumen, se concluye que los MFLOPS/100T son mayores a medida que aumenta la cantidad de procesos en ejecución en paralelo, lo cual es lo opuesto a lo que ocurre con los promedios de OPW/100T y OPR/100T, que disminuyen y se vuelven más estables a medida que aumenta la cantidad de procesos en paralelo.

Luego se llevó a cabo un experimento con quantums (intervalos de tiempo de planificación) 10 veces más cortos en los siguientes escenarios:

**Caso 1: 1 iobench solo**
En esta situación, los promedios de operaciones de escritura y lectura varían mucho menos, pero en todas las ejecuciones (excepto la primera y la última) son casi el doble en comparación con el promedio con el quantum por defecto. 

**Caso 2: 1 cpubench solo**
Se observa que el promedio de MFLOPS/100T es aproximadamente la mitad en comparación con el promedio con el quantum por defecto.

**Caso 3: 1 iobench con 1 cpubench**
En este caso, tanto los promedios de operaciones de escritura y lectura como el promedio de MFLOPS/100T disminuyen. Sin embargo, la disminución de los promedios de operaciones de escritura y lectura no es tan pronunciada como en el caso con el quantum por defecto.

**Caso 4: 1 cpubench con 1 cpubench**
En esta configuración, el promedio de MFLOPS/100T aumenta significativamente en más de 100 puntos en comparación con los otros casos. Además, hay poca variación en el promedio entre ambos procesos.

**Caso 5: 1 cpubench con 1 cpubench y 1 iobench**
En este caso, se observa una disminución en el promedio de MFLOPS en ambos cpubench y al mismo tiempo un aumento en los promedios de operaciones de escritura y lectura, a diferencia del mismo caso con el quantum por defecto. Por lo tanto, se puede concluir que hacer que el quantum sea 10 veces más corto tiene un impacto mixto en el rendimiento del procesador. En general, no es ni mejor ni peor, ya que su efecto depende del enfoque deseado.




## Experimento en otro hardware

**Hardware**: Intel® Celeron(R) N4020 CPU @ 1.10GHz × 2
**Quantum**: 1000000
**Política** Scheduler: Round Robin


### Caso 1: 1 iobench solo
**Instrucción**: iobench


Característica | Primer ejecución | Segunda ejecución | Tercera ejecución | Cuarta ejecución
:------------: | :------------: | :------------: | :------------: | :------------:
Promedio OPW/100T | 3111.26 | 2869.68 | 2857.84| 2716.52
Promedio OPR/100T | 3111.26 | 2869.68 | 2857.84 | 2716.52
Total de operaciones | 118228u | 109048u | 108598u | 103228u
Veces elegidas | 193449 | 181206 | 180236 | 171455


### Caso 2: 1 cpubench solo
**Instrucción**: cpubench


Característica | Primer ejecución | Segunda ejecución | Tercera ejecución | Cuarta ejecución
:------------: | :------------: | :------------: | :------------: | :------------:
Promedio MFLOPS /100T 1er cpubench | 305.13 | 257.85 | 295.5
| 296.86
Total de operaciones | 3087007744u | 1073741824u | 4160749568u | 3959422976u
Veces elegidas | 21068 | 21077 | 21476 | 21385


### Caso 3: 1 iobench con 1 cpubench
**Instrucción**: iobench &; cpubench


Característica | Primer ejecución | Segunda ejecución | Tercera ejecución | Cuarta ejecución
:------------: | :------------: | :------------: | :------------: | :------------:
Promedio MFLOPS /100T 1er cpubench | 349.64 | 305.86 | 297.07 | 335.88
Promedio OPW/100T | 36.1 | 36.2 | 36.1 | 36.2
Promedio OPR/100T | 36.1 | 36.2 | 36.1 | 36.2
Total de operaciones (iobench) | 722u | 724u | 722u | 724u
Veces elegidas (iobench) | 2218 | 2208 | 2218 | 2208
Veces elegidas (cpubench) | 2119 | 2102 | 2142 | 2138

### Caso 4: 1 cpubench con 1 cpubench
**Instrucción**: cpubench &; cpubench


Característica | Primer ejecución | Segunda ejecución | Tercera ejecución | Cuarta ejecución
:------------: | :------------: | :------------: | :------------: | :------------:
Promedio MFLOPS /100T (primer cpubench) | 393 | 407.64 | 396.88 | 383.41
Veces elegidas (primer cpubench) | 1066 | 1061 | 1061 | 1072
Promedio MFLOPS /100T (segundo cpubench) | 393.07 | 407.28 | 397.88 | 382.88
Veces elegidas (segundo cpubench) | 1064 | 1062 | 1058 | 1074


### Caso 5: 1 cpubench con 1 cpubench y 1 iobench.
**Instrucción**: cpubench &; cpubench &; iobench


Característica | Primer ejecución | Segunda ejecución | Tercera ejecución | Cuarta ejecución
:------------: | :------------: | :------------: | :------------: | :------------:
Promedio MFLOPS /100T (primer cpubench) | 387.05 | 379.55 | 411.38 | 406.69
Veces elegidas (primer cpubench) | 1064 | 1058 | 1052 | 1063
Promedio MFLOPS /100T (segundo cpubench) | 382.44 | 386.88 | 416.92| 401.53
Veces elegidas (segundo cpubench) | 1050 | 1064 | 1064 | 1055
Promedio OPW/100T (iobench) | 18.6 | 18.8 | 18.6| 19.2
Promedio OPR/100T (iobench) | 18.6 | 18.8 | 18.6 | 19.2
Veces elegidas (iobench) | 1228 | 1227 | 1232 | 1220


**Quantum**: 100000

### Caso 1: 1 iobench solo
**Instrucción**: iobench


Característica | Primer ejecución | Segunda ejecución | Tercera ejecución | Cuarta ejecución
:------------: | :------------: | :------------: | :------------: | :------------:
Promedio OPW/100T | 3235.11 | 3216.53 | 3231.58 | 3213.21
Promedio OPR/100T | 3235.10 | 3216.53 | 3231.58| 3213.21
Veces elegidas | 215917 | 214690 | 215717 | 214456


### Caso 2: 1 cpubench solo
**Instrucción**: cpubench


Característica | Primer ejecución | Segunda ejecución | Tercera ejecución | Cuarta ejecución
:------------: | :------------: | :------------: | :------------: | :------------:
Promedio MFLOPS /100T 1er cpubench | 313.2 | 318 | 319.07
| 322.81
Veces elegidas | 21527 | 21216 | 21150 | 21553


### Caso 3: 1 iobench con 1 cpubench
**Instrucción**: iobench &; cpubench


Característica | Primer ejecución | Segunda ejecución | Tercera ejecución
:------------: | :------------: | :------------: | :------------:
Promedio MFLOPS /100T 1er cpubench | 289.21 | 287.64 | 290.21
Promedio OPW/100T | 333.94 | 333.53 | 333.88
Promedio OPR/100T | 333.94 | 333.53 | 333.88
Veces elegidas (iobench) | 20963 | 20964 | 20965
Veces elegidas (cpubench) | 20984 | 21092 | 20914


### Caso 4: 1 cpubench con 1 cpubench
**Instrucción**: cpubench &; cpubench


Característica | Primer ejecución | Segunda ejecución | Tercera ejecución
:------------: | :------------: | :------------: | :------------:
Promedio MFLOPS /100T (primer cpubench) | 357.44 | 369.83 | 367.55
Veces elegidas (primer cpubench) | 10631 | 10546 | 10619
Promedio MFLOPS /100T (segundo cpubench) | 355.39 | 366.5 | 365.33
Veces elegidas (segundo cpubench) | 10688 | 10663 | 10685


### Caso 5: 1 cpubench con 1 cpubench y 1 iobench.
**Instrucción**: cpubench &; cpubench &; iobench


Característica | Primer ejecución | Segunda ejecución | Tercera ejecución
:------------: | :------------: | :------------: | :------------:
Promedio MFLOPS /100T (primer cpubench) | 366.61 | 173.68 | 312.61
Veces elegidas (primer cpubench) | 10600 | 10696 | 10460
Promedio MFLOPS /100T (segundo cpubench) | 330.63 | 153.64 | 347.93
Veces elegidas (segundo cpubench) | 10523 | 10600 | 10523
Promedio OPW/100T (iobench) | 167.76 | 168.35 | 167.12
Promedio OPR/100T (iobench) | 167.76 | 168.35 | 167.12
Veces elegidas (iobench) | 10523 | 10500 | 10555


# Cuarta parte

## Experimento con quantum por defecto

**Hardware**: Intel® Core™ i3-1005G1  
**Política**: Scheduler: MLFQ

### Caso 1: 1 iobench solo
**Instrucción**: iobench

Característica | Primer ejecución | Segunda ejecución | Tercera ejecución | Cuarta ejecución | Quinta ejecución
 :------------: | :------------: | :------------: | :------------: | :------------: | :------------:
Promedio OPW/100T | 8000 | 8000 | 8000 | 8000 | 8000
Promedio OPR/100T | 8000 | 8000 | 8000 | 8000 | 8000
Veces elegidas | 486903 | 501469 | 501067 | 500878 | 500888

### Caso 2: 1 cpubench solo
**Instrucción**: cpubench

Característica | Primer ejecución | Segunda ejecución | Tercera ejecución | Cuarta ejecución | Quinta ejecución
 :------------: | :------------: | :------------: | :------------: | :------------: | :------------:
Promedio MFLOPS/100T | 1916.29 | 1898.06 | 1921.82 | 1932.67 | 1954.67
Veces elegidas | 2105 | 2106 | 2109 | 2099 | 2101

### Caso 3: 1 iobench con 1 cpubench
**Instrucción**: iobench &; cpubench

Característica | Primer ejecución | Segunda ejecución | Tercera ejecución | Cuarta ejecución | Quinta ejecución
 :------------: | :------------: | :------------: | :------------: | :------------: | :------------:
Promedio MFLOPS/100T | 1891.67 | 1862.25 | 1924.92 | 1910.67 | 1939.08
Promedio OPW/100T | 45.25 | 41.5 | 45.75 | 43.08 | 42.33
Promedio OPR/100T | 45.25 | 41.5 | 45.75 | 43.08 | 42.33
Veces elegidas (iobench) | 2224 | 2225 | 2225 | 2224 | 2222
Veces elegidas (cpubench) | 2106 | 2104 | 2110 | 2101 | 2106

### Caso 4: 1 cpubench con 1 cpubench
**Instrucción**: cpubench &; cpubench

Característica | Primer ejecución | Segunda ejecución | Tercera ejecución | Cuarta ejecución | Quinta ejecución
 :------------: | :------------: | :------------: | :------------: | :------------: | :------------:
Promedio MFLOPS/100T (primer cpubench) | 966.25 | 993.17 | 996.31 | 995.77 | 996.23
Promedio MFLOPS/100T (segundo cpubench) | 968.17  | 998.25 | 997.54 | 996.15 | 987.85
Veces elegidas (primer cpubench) | 1059 | 1051 | 1052 | 1060 | 1054
Veces elegidas (segundo cpubench) | 1054 | 1058 | 1051 | 1051 | 1054

### Caso 5: 1 cpubench con 1 cpubench y 1 iobench.
**Instrucción**: cpubench &; cpubench &; iobench

Característica | Primer ejecución | Segunda ejecución | Tercera ejecución | Cuarta ejecución | Quinta ejecución
 :------------: | :------------: | :------------: | :------------: | :------------: | :------------:
Promedio MFLOPS/100T (primer cpubench) | 975.67 | 881.39 | 898.88 | 987.93 | 940.67
Promedio MFLOPS/100T (segundo cpubench) | 988.5 | 909.68 | 899.76 | 996.56 | 941.56
Promedio OPW/100T | 21.0 | 19.5 | 19.5 | 19.5 | 19.5
Promedio OPR/100T | 21.0 | 19.5 | 19.5 | 19.5 | 19.5
Veces elegidas (primer cpubench) | 1069 | 1055 | 1055 | 1053 | 1058
Veces elegidas (segundo cpubench) | 1063 | 1059 | 1060 | 1052 | 1056
Veces elegidas (iobench) | 1221 | 1227 | 1227 | 1227 | 1227

## Experimento con quantum 10 veces más chico

### Caso 1: 1 iobench solo
**Instrucción**: iobench

Característica | Primer ejecución | Segunda ejecución | Tercera ejecución | Cuarta ejecución | Quinta ejecución
 :------------: | :------------: | :------------: | :------------: | :------------: | :------------:
Promedio OPW/100T | 8640 | 8640 | 8640 | 8601.6 | 8601.6
Promedio OPR/100T | 8640 | 8640 | 8640 | 8601.6 | 8601.6
Veces elegidas |  |  |  |  | 

### Caso 2: 1 cpubench solo
**Instrucción**: cpubench

Característica | Primer ejecución | Segunda ejecución | Tercera ejecución | Cuarta ejecución | Quinta ejecución
 :------------: | :------------: | :------------: | :------------: | :------------: | :------------:
Promedio MFLOPS/100T | 860 | 859.63 | 859.25 | 859.38 | 859.13
Veces elegidas | 21092 | 21067 | 21131 | 21062 | 21089

### Caso 3: 1 iobench con 1 cpubench
**Instrucción**: iobench &; cpubench

Característica | Primer ejecución | Segunda ejecución | Tercera ejecución | Cuarta ejecución | Quinta ejecución
 :------------: | :------------: | :------------: | :------------: | :------------: | :------------:
Promedio MFLOPS/100T | 828.4 | 789.5 | 781.5 | 792.25 | 807.5
Promedio OPW/100T | 344.5 | 349 | 403.75 | 395.75 | 349.75
Promedio OPR/100T | 344.5 | 349 | 403.75 | 395.75 | 349.75
Veces elegidas (iobench) | 21598 | 21028 | 22463 | 21267 | 21457
Veces elegidas (cpubench) | 21161 | 21047 | 21175 | 20899 | 21057

### Caso 4: 1 cpubench con 1 cpubench
**Instrucción**: cpubench &; cpubench

Característica | Primer ejecución | Segunda ejecución | Tercera ejecución | Cuarta ejecución | Quinta ejecución
 :------------: | :------------: | :------------: | :------------: | :------------: | :------------:
Promedio MFLOPS/100T (primer cpubench) | 963.25 | 981.7 | 977 | 978.03 | 970.6
Promedio MFLOPS/100T (segundo cpubench) | 963.625 | 984.6 | 981.2  | 985.5 | 972.6
Veces elegidas (primer cpubench) | 10567 | 10589 | 10592 | 10574 | 10550
Veces elegidas (segundo cpubench) | 10496 | 10517 | 10521 | 10505 | 10516

### Caso 5: 1 cpubench con 1 cpubench y 1 iobench.
**Instrucción**: cpubench &; cpubench &; iobench

Característica | Primer ejecución | Segunda ejecución | Tercera ejecución | Cuarta ejecución | Quinta ejecución
 :------------: | :------------: | :------------: | :------------: | :------------: | :------------:
Promedio MFLOPS/100T (primer cpubench) | 896.125 | 712.75 | 394.4 | 357.85 | 375
Promedio MFLOPS/100T (segundo cpubench) | 891.125 | 869.5 | 399.5 | 367.65 | 362.5
Promedio OPW/100T | 336 | 345 | 180.75 | 178.95 | 168.5
Promedio OPR/100T | 336 | 345 | 180.75 | 178.95 | 168.5
Veces elegidas (primer cpubench) | 10526 | 10540 | 10443 | 10655 | 10552
Veces elegidas (segundo cpubench) | 10561 | 10638 | 10532 | 10430 | 10689
Veces elegidas (iobench) | 21055 | 15866 | 10723 | 10539 | 10540


## Experimento en otro hardware

**Política** Scheduler: MLFQ
**Quantum**: 1000000


### Caso 1: 1 iobench solo
**Instrucción**: iobench


Característica | Primer ejecución | Segunda ejecución | Tercera ejecución | Cuarta ejecución
:------------: | :------------: | :------------: | :------------: | :------------:
Promedio OPW/100T | 3625.63 | 3356.68 | 3301.05 | 3250.52
Promedio OPR/100T | 3625.63 | 3356.68 | 3301.05 | 3250.52
Veces elegidas | 241022 | 221385 | 217633 | 214962


### Caso 2: 1 cpubench solo
**Instrucción**: cpubench


Característica | Primer ejecución | Segunda ejecución | Tercera ejecución
:------------: | :------------: | :------------: | :------------:
Promedio MFLOPS /100T 1er cpubench | 334.75 | 340.31 | 341
Veces elegidas | 21454 | 21065 | 21150 | 21032


### Caso 3: 1 iobench con 1 cpubench
**Instrucción**: iobench &; cpubench


Característica | Primer ejecución | Segunda ejecución | Tercera ejecución
:------------: | :------------: | :------------: | :------------:
Promedio MFLOPS /100T 1er cpubench | 306.53 | 324.2 | 310
Promedio OPW/100T | 324.29 | 328.94 | 510.06
Promedio OPR/100T | 324.29 | 328.94 | 510.06
Veces elegidas (iobench) | 20620 | 20736 | 20968
Veces elegidas (cpubench) | 20836 | 21033 | 20944


### Caso 4: 1 cpubench con 1 cpubench
**Instrucción**: cpubench &; cpubench


Característica | Primer ejecución | Segunda ejecución | Tercera ejecución
:------------: | :------------: | :------------: | :------------:
Promedio MFLOPS /100T (primer cpubench) | 389.81 | 391.66 | 389.4
Veces elegidas (primer cpubench) | 10739 | 10759 | 10735
Promedio MFLOPS /100T (segundo cpubench) | 394.86 | 389.56 | 387.5
Veces elegidas (segundo cpubench) | 10686 | 10572 | 10543


### Caso 5: 1 cpubench con 1 cpubench y 1 iobench.
**Instrucción**: cpubench &; cpubench &; iobench


Característica | Primer ejecución | Segunda ejecución | Tercera ejecución
:------------: | :------------: | :------------: | :------------:
Promedio MFLOPS /100T (primer cpubench) | 366.61 | 386.89 | 380.78
Veces elegidas (primer cpubench) | 10600 | 10593 | 10525
Promedio MFLOPS /100T (segundo cpubench) | 330.63 | 381.39 | 384.63
Veces elegidas (segundo cpubench) | 10522 | 10491 | 10672
Promedio OPW/100T (iobench) | 178.24 | 175.29 | 173
Promedio OPR/100T (iobench) | 178.24 | 175.29 | 173
Veces elegidas (iobench) | 10914 | 10882 | 10887


**Quantum**: 100000


### Caso 1: 1 iobench solo
**Instrucción**: iobench


Característica | Primer ejecución | Segunda ejecución | Tercera ejecución | Cuarta ejecución
:------------: | :------------: | :------------: | :------------: | :------------:
Promedio OPW/100T | 3075.79 | 2964.59 | 2856.53 | 2876.89
Promedio OPR/100T | 3075.79 | 2964.59 | 2856.53 | 2876.89
Veces elegidas | 206536 | 198867 | 192660 | 193564


### Caso 2: 1 cpubench solo
**Instrucción**: cpubench


Característica | Primer ejecución | Segunda ejecución | Tercera ejecución | Cuarta ejecución
:------------: | :------------: | :------------: | :------------: | :------------:
Promedio MFLOPS /100T 1er cpubench | 320.06 | 317.07 | 310.2 | 305.6
Veces elegidas | 21100 | 21268 | 21135 | 21419


### Caso 3: 1 iobench con 1 cpubench
**Instrucción**: iobench &; cpubench


Característica | Primer ejecución | Segunda ejecución | Tercera ejecución
:------------: | :------------: | :------------: | :------------:
Promedio MFLOPS /100T 1er cpubench | 579.687 | 571 | 580,27
Promedio OPW/100T | 303,71 | 303,61 | 303,5
Promedio OPR/100T | 303,71 | 303,61 | 303,5
Veces elegidas (iobench) | 19790 | 20008 | 19951
Veces elegidas (cpubench) | 19834 | 19888 | 19847


### Caso 4: 1 cpubench con 1 cpubench
**Instrucción**: cpubench &; cpubench


Característica | Primer ejecución | Segunda ejecución | Tercera ejecución
:------------: | :------------: | :------------: | :------------:
Promedio MFLOPS /100T (primer cpubench) | 324.93 | 325.12 | 332,68
Veces elegidas (primer cpubench) | 10811 | 10725 | 10799
Promedio MFLOPS /100T (segundo cpubench) | 307.67 | 325.81 | 334,56
Veces elegidas (segundo cpubench) | 10873 | 10692 | 10743


### Caso 5: 1 cpubench con 1 cpubench y 1 iobench.
**Instrucción**: cpubench &; cpubench &; iobench


Característica | Primer ejecución | Segunda ejecución | Tercera ejecución
:------------: | :------------: | :------------: | :------------:
Promedio MFLOPS /100T (primer cpubench) | 296,06 | 305.33 | 302.2
Veces elegidas (primer cpubench) | 10453 | 10496 | 10586
Promedio MFLOPS /100T (segundo cpubench) | 292,13 | 281,66 | 297,86
Veces elegidas (segundo cpubench) | 10579 | 10327 | 10401
Promedio OPW/100T (iobench) | 173,71 | 163.13 | 161,21
Promedio OPR/100T (iobench) | 173,71 | 163.13 | 161,21
Veces elegidas (iobench) | 10407 | 10758 | 10375


## Conclusiones 

### Round Robin

En el planificador original de xv6 se utiliza la política RR, la cual le otorga a cada proceso que está listo para ejecutarse una misma cantidad de tiempo para poder hacerlo.
Esto se puede ver reflejado en la manera en que se comportan los programas cpubench e iobench al momento de medir operaciones que ocupan el CPU, en el primer caso, y de entrada/salida en el segundo.
Notamos ,por ejemplo, que al ejecutar cpubench con un iobench este último no opera tanto como el primero. Recordemos que ambos procesos tienen un medidor global el cual dicta por cuanto tiempo van a ejecutar sus operaciones. Como iobench no usa todo su quanto, al irse a dormir, sucede que el planificador pone  en marcha a cpubench el cual ejecuta por todo el intervalo que se le otorga. Parte de este tiempo global de iobench es, entonces, ocupado por la ejecución del otro proceso. 
Distinto es lo que sucede al ejecutar dos cpubench al mismo tiempo. Cada uno ocupa todo su quantum y si bien su tiempo global se ve afectado por las operaciones que realiza el otro (lo cual se refleja en las medidas) la diferencia no es tan pronunciada.
Combinando estos dos casos tenemos nuestro último escenario de prueba, en donde notamos poca diferencia entre las medidas de los dos cpus pero una gran dismunición en la cantidad de operaciones de entrada y salida.

Al disminuír el quantum podemos ver una pequeña disminución en la cantidad de operaciones realizadas por un cpubench. Suponemos que esto se debe a que parte del tiempo global del programa es consumido cuando se realizan cambios de contextos, los cuales son más frecuentes con este quanto.
Estas interrupciones también generan que los procesos de cpubench al ser ejecutados con iobench no consuman todo el tiempo y este último tenga la oportunidad de realizar más operaciones(Se puede notar la diferencia en las mediciones).

### MLFQ

Al elegir primeramente a aquellos procesos que no consuman todo su quantum, MLFQ siempre favorece a iobench en comparación con RR y le permite realizar más operaciones.

(*)Nuestros problemas con la implementación surgen al disminuir el quantum ya que sucede que al ejecutar un iobench mientras se realizan las operaciones de lectura y escritura este se mantiene en el nivel más alto de prioridad pero una vez que terminan y pasa a llamar a pstat su prioridad disminuye. Suponemos que se debe a la cantidad de veces que se está llamando a yield() (mayor que con la de un quanto normal) que ocasiona tales problemas.
Como solución para implementar a futuro pensamos en implementar MLFQ con distintas colas de prioridad, cada una con un quantum apropiado.

Con nuestra implementación si puede existir starvation ya que si tenemos un proceso de baja prioridad esperando a ser ejecutado y llega otro proceso de mayor prioridad que realice operaciones I/O entonces este se elige a este último y no al que se encontraba esperando. A la larga, si esto sucediera repetidamente ese proceso seguiría en estado RUNNABLE pero no será elejido.