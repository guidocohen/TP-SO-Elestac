/*
 * consola.h
 *
 */

#ifndef CONSOLA_H_
#define CONSOLA_H_

#define PROMPT "anSISOP> "
#define COMANDO "Comando> "

#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include "commons/log.h"
#include "commons/config.h"
#include "sockets.h"

typedef struct {
	int port_Nucleo;
	char* ip_Nucleo;
} t_configFile;

char nombreDelArchivo[100];
int num = 0;

//Logger
t_log* logConsola;

//Configuracion
t_configFile configConsola;

//Encabezamiento de Funciones

void* leerArchivoYGuardarEnCadena(int* tamanioDeArchivo);
int reconocerComando(char* comando);
void crearArchivoDeConfiguracion(char *configFile);
int connectTo(enum_processes processToConnect, int *socketClient);
void reconocerOperacion();
void startConsolaConsole();
void consoleMessageConsola();


#endif /* CONSOLA_H_ */
