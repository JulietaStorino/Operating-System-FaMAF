#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void 
print_ping(int sem_name_f, int sem_name_s){
    sem_down(sem_name_f);
    printf("ping\n");
    sem_up(sem_name_s);
}

void 
print_pong(int sem_name_f, int sem_name_s){
    sem_down(sem_name_s);
    printf("\tpong\n");
    sem_up(sem_name_f);
}

int 
main(int argc, char *argv[]){
    int repetitions = atoi(argv[1]);

    if (repetitions < 1)
        printf("ERROR: El numero de rounds tiene que ser mayor a 0\n");

    int sem_name_f = 0;

    while (sem_open(sem_name_f, 1) == 0)
        sem_name_f++;  

    int sem_name_s = sem_name_f + 1;

    while (sem_open(sem_name_s, 0) == 0)
        sem_name_s++;   

    int result = fork();
    
    if (result < 0){
        printf("Failed to fork\n");

        return 0;
    } else if (result == 0){
        for (int i = 0; i < repetitions ; i++)
            print_ping(sem_name_f, sem_name_s);

    } else {
        for (int i = 0; i < repetitions ; i++)
            print_pong(sem_name_f, sem_name_s);
    }

    sem_close(sem_name_f);
    sem_close(sem_name_s);

    exit(0);
}