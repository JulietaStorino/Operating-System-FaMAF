#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "command.h"
#include "builtin.h"
#include "execute.h"

#include "tests/syscall_mock.h"

// Función encargada de manejar errores
static void error(int flag, char *message){
    if (flag == -1) {
        perror(message);
        exit(EXIT_FAILURE);
    }
}

// Función que cambia el descriptor de archivo de entrada
static void change_file_descriptor_in(int fd_in) {
    int syscall_dup = dup2(fd_in, STDIN_FILENO);
    error(syscall_dup, "Dup2");

    syscall_dup = close(fd_in);
    error(syscall_dup, "Close");
}

// Función que cambia el descriptor de archivo de salida
static void change_file_descriptor_out(int fd_out) {
    int syscall_dup = dup2(fd_out, STDOUT_FILENO);
    error(syscall_dup, "Dup2");

    syscall_dup = close(fd_out);
    error(syscall_dup, "Close");
}

// Función que abre el descriptor de archivo de entrada
static void open_file_descriptor_in(char *redir_in) {
    if (redir_in != NULL) {
        int fd_in = open(redir_in, O_RDONLY, 0u);
        error(fd_in, "Open");

        change_file_descriptor_in(fd_in);
    }
}

// Función que abre el descriptor de archivo de salida
static void open_file_descriptor_out(char *redir_out) {
    if (redir_out != NULL) {
        int fd_out = open(redir_out, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
        error(fd_out, "Open");

        change_file_descriptor_out(fd_out);
    }
}

// Función que convierte un comando simple en un array de cadenas
static char **scommand_to_array(scommand command) {
    unsigned int length = scommand_length(command);
    char **array = (char **)malloc(sizeof(char *) * (length + 1));

    for (unsigned int i = 0; i < length; i++) {
        array[i] = strdup(scommand_front(command));
        scommand_pop_front(command);
    }
    array[length] = NULL;
    return array;
}

// Función que ejecuta un comando simple
static void execute_single(pipeline apipe, int fd_in, int fd_out, int fd_pipe_in) {
    scommand cmd = pipeline_front(apipe);

    if (builtin_alone(apipe)) {
        builtin_run(pipeline_front(apipe));
    } else {
        pid_t pid = fork();
        error(pid, "Fork");

        if (pid == 0) { 
            if (fd_pipe_in != -1) close(fd_pipe_in);
            if (fd_in != -1) change_file_descriptor_in(fd_in);
            if (fd_out != -1) change_file_descriptor_out(fd_out);

            open_file_descriptor_in(scommand_get_redir_in(cmd));
            open_file_descriptor_out(scommand_get_redir_out(cmd));

            char **argv = scommand_to_array(cmd);
            execvp(argv[0], argv);
            error(-1, argv[0]);
        }
    }
}

// Función que ejecuta un pipeline recursivamente
static void execute_pipeline_recursive(pipeline apipe, int fd_in) {
    assert(apipe != NULL);

    if (pipeline_length(apipe) == 1) {
        execute_single(apipe, fd_in, -1, -1);
        pipeline_pop_front(apipe);

    } else {
        int p[2];
        int syscall_pipe = pipe(p);
        error(syscall_pipe, "Pipe");

        execute_single(apipe, fd_in, p[1], p[0]);

        close(p[1]);
        pipeline_pop_front(apipe);

        execute_pipeline_recursive(apipe, p[0]);
        close(p[0]);
    }
}

// Función que llama al ejecutador del pipeline y espera a que termine si es que debe
static void execute_and_wait(pipeline apipe, int fd, int signal){
    int childrens = builtin_alone(apipe) ? 0 : pipeline_length(apipe);
  
    execute_pipeline_recursive(apipe, fd);

    while(childrens > 0){
        int child_signal = -1;
        wait((signal==-1) ? NULL : &child_signal);
        if((signal!=-1) && child_signal != signal) childrens --;
        if((signal==-1))childrens --;
    }
} 

// Función principal de execute
void execute_pipeline(pipeline apipe) {
    assert(apipe != NULL);

    if (!pipeline_is_empty(apipe)) {

        if (pipeline_get_wait(apipe)) {
            execute_and_wait(apipe, -1, 13);
        } else {
            int pid = fork();
            error(pid, "Fork");

            if (pid == 0) { 
                int p[2];
                int syscall_pipe = pipe(p);
                error(syscall_pipe, "Pipe");

                execute_and_wait(apipe, p[0], -1);

                close(p[0]);
                close(p[1]);

                exit(13);
            }
        }
    }
}