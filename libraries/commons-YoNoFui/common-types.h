/****************************************************
* Common Types for all the processes
***************************************************/
#include <commons/collections/list.h>
#include <stdio.h>
#include <parser/metadata_program.h>
#include <parser/parser.h>

typedef enum{
	DISCO = 0//TODO definir enum_dispositivos
} enum_dispositivos;

typedef enum{
	agregar_proceso = 1,
	finalizar_proceso,
	lectura_pagina,
	escritura_pagina
} enum_operationsUMC_SWAP;

typedef struct{
	int pag;
	int offset;
	int size;
} t_memoryLocation;

typedef struct{
	char identificador;
	t_memoryLocation *direccionValorDeVariable; //
} t_vars;

struct indiceDeCodigo{
	int inicioDeInstruccion;//Se carga con un contador cada vez que se deserealiza una instruccion desde "t_metadata_program.instrucciones_serializado"
	int longitudInstruccionEnBytes; // strlen de la instruccion
}typedef t_registroIndiceCodigo;

typedef struct{
	int pos; // Position in stack
	t_list *args; // List of address memory position for function arguments with format t_memoryLocation
	t_list *vars; // List of vars with format t_vars
	int retPos; // si hay una funcion se deberia cargar con el valor "t_registroIndiceCodigo.inicioDeInstruccion" de esa instruccion
	t_memoryLocation* retVar;
} t_registroStack;

struct indiceDeEtiqueta{
	char* funcion;
	int posicionDeLaEtiqueta;
}typedef t_registroIndiceEtiqueta;

struct bloqueDeControl{
	int PID;
	int ProgramCounter;
	int cantidadDePaginas;// Cantidad de paginas de codigo
	int StackPointer;
	int estado; //0: New, 1: Ready, 2: Exec, 3: Block, 4:5: Exit
	int finalizar;
	int indiceDeEtiquetasTamanio;
	t_list *indiceDeCodigo;//cola o pila con registros del tipo t_registroIndiceCodigo
	char *indiceDeEtiquetas;
	t_list *indiceDeStack;//cola o pila con registros del tipo t_registroStack
	int finDePrograma;//index from list indiceDeCodigo where is the "end" from main
}typedef t_PCB;

typedef struct datosEntradaSalida {
	int tiempo;
	int ProgramCounter;
	t_nombre_dispositivo dispositivo;
} t_es;

void setPageSize (int pageSize);
int getLogicalAddress (int page);
