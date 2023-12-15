#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include "parsing.h"
#include "parser.h"
#include "command.h"

static scommand parse_scommand(Parser p) {
    assert(p != NULL);

    arg_kind_t type;
    scommand command = scommand_new(); 
    char *arg = parser_next_argument(p, &type); //obtenemos el primer argumento

    while (arg != NULL) { 
        if (type == ARG_NORMAL){
            scommand_push_back(command, arg); //agregamos el argumento a la estructura scommand
        } else if (type == ARG_INPUT){
            scommand_set_redir_in(command, arg); //establecemos la redireccion de entrada
        } else if (type == ARG_OUTPUT){
            scommand_set_redir_out(command, arg); //establecemos la redireccion de salida
        }
        arg = parser_next_argument(p, &type); //obtenemos el siguiente argumento
    }
    
    // Verifico si el comando dado está vacío
    if (scommand_is_empty(command)) {
        // Si el comando está vacío lo destruye y establece su valor en NULL
        scommand_destroy(command);
        command = NULL;
    }

    return command;
}

pipeline parse_pipeline(Parser p) {
    assert(p != NULL && !parser_at_eof(p));

    pipeline result = pipeline_new(); //creamos un nuevo pipeline
    scommand cmd = NULL;

    bool error, another_pipe = true;
    bool garbage, is_backsground;

    cmd = parse_scommand(p); //parseamos el primer comando

    error = (cmd == NULL); /* Comando inválido al empezar */

    while (another_pipe && !error) {
        pipeline_push_back(result,cmd); //agregamos el comando al pipeline
        parser_op_background(p, &is_backsground); //verificamos si hay un background
        parser_op_pipe(p, &another_pipe); //verificamos si hay un pipe
        if(another_pipe){
            error = is_backsground; //si hay un pipe, verificamos si hay un background
        }
        cmd = parse_scommand(p); //parseamos el siguiente comando
    }

    parser_skip_blanks(p); //consumimos espacios
    pipeline_set_wait(result, !is_backsground); //establecemos si se espera o no al pipeline
    parser_garbage(p, &garbage); //consumimos el salto de linea

    if(error){ 
        pipeline_destroy(result); //destruimos el pipeline
        result = NULL;
        printf("Syntax error.\n"); //imprimimos un mensaje de error
    }   

     // Devuelve el pipeline o NULL (en caso de un error de sintaxis)
    return result; 
}