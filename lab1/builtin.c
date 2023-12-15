#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "builtin.h"
#include "tests/syscall_mock.h" 

// Función que verifica si el comando ingresado es "cd"
static  bool command_is_cd(char* cmd){
    return (strcmp(cmd, "cd") == 0)  ;  
}

// Función que verifica si el comando ingresado es "exit"
static bool command_is_exit(char* cmd){
    return (strcmp(cmd, "exit") == 0);
}

// Función que verifica si el comando ingresado es "help"
static bool command_is_help(char* cmd){
    
    return (strcmp(cmd, "help") == 0);
    
}

// Función que verifica si el comando ingresado es interno
bool builtin_is_internal(scommand cmd){
    assert(cmd != NULL);
    char *command = scommand_front(cmd);
    bool cmd_is_internal = command_is_cd(command) || command_is_exit(command) || command_is_help(command);
    return cmd_is_internal;
} 

// Función que verifica si el pipeline ingresado tiene un solo comando interno
bool builtin_alone(pipeline p){
    assert(p != NULL);
    bool is_alone = false;
    is_alone = (pipeline_length(p) == 1) && (builtin_is_internal(pipeline_front(p)));

    assert(is_alone == (pipeline_length(p) == 1 && builtin_is_internal(pipeline_front(p))));
    return is_alone;
}

// Función que ejecuta el comando "cd"
static void builtin_run_cd(scommand cmd){
    scommand_pop_front(cmd);
    char *home =  getenv("HOME");

    if (home == NULL){
        exit(EXIT_FAILURE);
    }

    if (scommand_length(cmd) == 0 || ( (scommand_length(cmd) == 1) && strcmp(scommand_front(cmd), "~") == 0)){
        if (chdir(home) == -1) {
            printf("Error\n");
        }
    } else if (scommand_length(cmd) > 1){
        printf("Too many arguments\n");
    } else {
        char *new_path = scommand_front(cmd);
        if (chdir(new_path) == -1){
            if (errno == ENOENT ){
                printf("No such file or directory\n");
            }
        }
    }    
}

// Función que ejecuta el comando "exit"
static void builtin_run_exit(scommand cmd){
    exit(EXIT_SUCCESS);
}

// Función que ejecuta el comando interno correspondiente
void builtin_run(scommand cmd){
    assert(builtin_is_internal(cmd));
    char *command = scommand_front(cmd);

    if ( command_is_cd(command)){
        builtin_run_cd(cmd);
    } else if (command_is_exit(command)){
        builtin_run_exit(cmd);
    } else {
        printf(" MyWolash, version 0.0.1 \n - **Authors: Choque Esmeralda, Diaz Alejandro, Gomez Rocio, Storino Julieta \n" 
        "**Functions and Descriptions**\n"
        "- cd: Changes the current directory to the one specified in the given path\n"
        "- help: Prints information about the shell and available internal commands\n"
        "- exit: exits the shell\n"
        ); 
    }
}