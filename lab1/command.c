#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <glib.h>
#include <string.h>
#include <assert.h>
#include "command.h"
#include "strextra.h"

// Definimos la estructura scommand_s
struct scommand_s{
    GList *command;
    char *redirectIn;
    char *redirectOut;
};

// Definimos la estructura pipeline_s
struct pipeline_s{
    GQueue *commands;
    bool wait;
};
 
//Invariante de representacion de scommand
static bool scommand_invrep(scommand s){
    return(s != NULL);
}

//Invariante de representacion de pipeline
static bool pipeline_invrep(pipeline p){
    return(p != NULL);
}

// Función para crear un nuevo comando
scommand scommand_new(void){

    scommand new_command = NULL;
    new_command = malloc(sizeof(struct scommand_s));
    if ( new_command == NULL){
        exit(EXIT_FAILURE);
    }
    new_command -> command = NULL;
    new_command -> redirectIn = NULL;
    new_command -> redirectOut = NULL;

    assert(scommand_invrep(new_command) && scommand_is_empty(new_command) && scommand_get_redir_in(new_command) == NULL && scommand_get_redir_out(new_command) == NULL);
    return new_command;
}

// Función para destruir un comando
scommand scommand_destroy(scommand self){
    assert(scommand_invrep(self));
    /*Destroy list*/
    g_list_free_full(self -> command, free);
    self -> command = NULL;
    free(self -> redirectIn);
    free(self ->redirectOut);
    self -> redirectIn = NULL;
    self -> redirectOut = NULL;
    free(self);
    self = NULL;

    assert(!scommand_invrep(self));
    return self;
}

// Función para destruir un comando (versión void)
static void scommand_destroy_void(gpointer self){
    scommand_destroy(self);
}

// Función para agregar un argumento al final de un comando
void scommand_push_back(scommand self, char * argument){
    assert(scommand_invrep(self) && (argument != NULL));
    self -> command = g_list_append(self -> command, argument);
    assert(!scommand_is_empty(self));
    
}

// Función para eliminar el primer argumento de un comando
void scommand_pop_front(scommand self){
    assert(scommand_invrep(self) && !scommand_is_empty(self));
    gpointer *data_first_elem = g_list_nth_data(self -> command, 0);
    self ->command = g_list_delete_link(self -> command, self -> command);
    free(data_first_elem); 

}

// Función para establecer la redirección de entrada de un comando
void scommand_set_redir_in(scommand self, char * filename){
    assert(scommand_invrep(self));
    if (self -> redirectIn  != NULL){
        free(self -> redirectIn );
    }
    self -> redirectIn = filename;
}

// Función para establecer la redirección de salida de un comando
void scommand_set_redir_out(scommand self, char * filename){
    assert(scommand_invrep(self));
    if (self -> redirectOut  != NULL){
        free(self -> redirectOut);
    }
    self -> redirectOut = filename;
}

// Función para verificar si un comando está vacío
bool scommand_is_empty(const scommand self){
    assert(scommand_invrep(self));
    gboolean is_empty  = self -> command == NULL;

    return is_empty;
}

// Función para obtener la longitud de un comando
unsigned int scommand_length(const scommand self){
    assert(scommand_invrep(self));
    unsigned int length = g_list_length( self -> command);
    assert( (length == 0) == scommand_is_empty(self) );
    return length;
}

// Función para obtener el primer argumento de un comando
char * scommand_front(const scommand self){
    assert(scommand_invrep(self) && !scommand_is_empty(self));
    char *front = g_list_nth_data(self -> command,0u); 
    assert(front!=NULL);
    return front;
}

// Función para obtener la redirección de entrada de un comando
char * scommand_get_redir_in(const scommand self){
    assert(scommand_invrep(self));
    char *redir_in = self -> redirectIn;
    return redir_in;
}

// Función para obtener la redirección de salida de un comando
char * scommand_get_redir_out(const scommand self){
    assert(scommand_invrep(self));
    char *redir_out = self -> redirectOut;
    return redir_out;
}

// Función para convertir un comando a una cadena de caracteres
char * scommand_to_string(const scommand self){
    assert(scommand_invrep(self));
    
    char *result = strdup("");
    if (!scommand_is_empty(self) && self-> command != NULL){

        GList *listCommand = g_list_copy(self -> command); 
        char *redirIn = scommand_get_redir_in(self);
        char *redirOut = scommand_get_redir_out(self);

        //concatenamos el comando y los argumentos
        while (!(g_list_length(listCommand) == 0)){
            char *argument = g_list_nth_data( listCommand, 0);
            result = concatstr(result, argument);
            result = concatstr(result, " ");
            listCommand = g_list_remove(listCommand, argument);
        }

        //concatenamos el out
        if(redirOut != NULL){
            result = concatstr(result, "> ");
            result = concatstr(result, redirOut);
        }
    
        //concatenamos el in
        if(redirIn != NULL){
            result = concatstr(result, "< ");
            result = concatstr(result, redirIn);
        }
    }
    
    
    assert(scommand_is_empty(self) || scommand_get_redir_in(self)== NULL || scommand_get_redir_out(self)==NULL || strlen(result)>0);
    return result;
}

// Función para crear un nuevo pipeline
pipeline pipeline_new(void){
    pipeline result = (pipeline)malloc(sizeof(struct pipeline_s));

    GQueue *queue = g_queue_new();
    
    result->commands = queue;
    result->wait = true;
    
    assert(pipeline_invrep(result) && pipeline_is_empty(result) && pipeline_get_wait(result));
    return(result);
}

// Función para destruir un pipeline
pipeline pipeline_destroy(pipeline self){
    assert(pipeline_invrep(self));
    g_queue_clear_full(self->commands, scommand_destroy_void);
    free(self);
    self = NULL;
    
    assert(!pipeline_invrep(self));
    return(self);
}

// Función para agregar un comando al final de un pipeline
void pipeline_push_back(pipeline self, scommand sc){
    assert(pipeline_invrep(self) && scommand_invrep(sc));
    g_queue_push_tail(self->commands, sc);
    assert(!pipeline_is_empty(self));
}

// Función para eliminar el primer comando de un pipeline
void pipeline_pop_front(pipeline self){
    assert(pipeline_invrep(self) && !pipeline_is_empty(self));
    gpointer data = g_queue_pop_head(self->commands);
    free(data);
}

// Función para establecer si se debe esperar en un pipeline
void pipeline_set_wait(pipeline self, const bool w){
    assert(pipeline_invrep(self));

    self->wait = w;
}

// Función para verificar si un pipeline está vacío
bool pipeline_is_empty(const pipeline self){
    assert(pipeline_invrep(self));

    return g_queue_is_empty(self->commands);
}

// Función para obtener la longitud de un pipeline
unsigned int pipeline_length(const pipeline self){
    assert(pipeline_invrep(self));

    unsigned int length = g_queue_get_length(self->commands);


    assert((length == 0) == pipeline_is_empty(self));
    return length;
}

// Función para obtener el primer comando de un pipeline
scommand pipeline_front(const pipeline self){
    assert(pipeline_invrep(self) && !pipeline_is_empty(self));

    scommand result = g_queue_peek_head(self->commands);
    
    assert(scommand_invrep(result));
    return result;
}

// Función para obtener si se debe esperar en un pipeline
bool pipeline_get_wait(const pipeline self){
    assert(pipeline_invrep(self));

    return self->wait;
}

// Función para concatenar un comando a una cadena de caracteres
static char *concat_scommand(char *str, gpointer scm){
    char *arg = scommand_to_string(g_queue_peek_head(scm));
    str = concatstr(str, arg);
    free(arg);
    arg = NULL;
    g_queue_pop_head(scm);
    return str;
}

// Función para convertir un pipeline a una cadena de caracteres
char * pipeline_to_string(const pipeline self){
    assert(pipeline_invrep(self));

    char *result = strdup("");
    GQueue *commands = g_queue_copy(self -> commands);
    if( commands != NULL && pipeline_length(self) != 0){

        result = concat_scommand(result, commands);
        
        while( !g_queue_is_empty(commands)){
            result = concatstr(result, " | ");
            result = concat_scommand(result, commands);
            
        }
    
        if(!self->wait){
            char wait_str[2] = {'&', '\0'};
            result = concatstr(result, wait_str);
           
        }

    }
    assert(pipeline_is_empty(self) || pipeline_get_wait(self) || strlen(result)>0);
    return result;
}
