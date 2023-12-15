#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
 #include <unistd.h>
#include "command.h"
#include "execute.h"
#include "parser.h"
#include "parsing.h"
#include "builtin.h"

static void show_prompt(void) {
    char *username = getenv("USER"); // Obtengo el nombre del usuario
    time_t t = time(NULL);
    struct tm tm = *localtime(&t); // Obtengo la fecha y hora actual
    char datetime[20];
    char buffer[4096];
    if (getcwd(buffer, 4096) == NULL){ // Obtengo el directorio actual
        exit(EXIT_FAILURE);
    }
    // Formateo la fecha y hora
    strftime(datetime, sizeof(datetime), "%Y-%m-%d - %H:%M", &tm);

    printf("\x1b[32m%s\x1b[0m in ", username); // Imprimo el nombre del usuario en verde
    printf("\x1b[36m\x1b[33m%s \x1b\x1b[0m~", buffer); // Imprimo el directorio actual en amarillo
    printf("\x1b[36m [%s]\n\x1b[0m", datetime); // Imprimo la fecha y hora en azul
    printf ("myWolash> "); // Imprimo el prompt
    
    fflush (stdout);
}

int main(int argc, char *argv[]) {
    pipeline pipe;
    Parser input;
    bool quit = false;

    input = parser_new(stdin);
    while (!quit) {
        show_prompt(); // Muestro el prompt
        pipe = parse_pipeline(input);

        quit = parser_at_eof(input); // Verifico si llegue al final del archivo
        
        if (pipe != NULL){
            execute_pipeline(pipe); // Ejecuto el pipeline
            pipe = pipeline_destroy(pipe);
        }
    }

    //Realizo un salto de linea
    printf("\n");
    parser_destroy(input);
    input = NULL;
    
    return EXIT_SUCCESS;
}

