#ifndef SWAP_H_
#define SWAP_H_

/*
 * swap.h
 *
 *  Created on: 25/6/2016
 *      Author: utnso
 */

/*
#define agregar_proceso 1
#define finalizar_proceso 2
#define lectura_pagina 3
#define escritura_pagina 4
*/

#include <stdlib.h>
#include <stdbool.h>
#include "commons/string.h"
#include <string.h>
#include <sys/mman.h>
#include "commons/config.h"
#include "commons/log.h"
#include "stdbool.h"
#include <fcntl.h>

struct bloqueDeMemoria{
	int PID;
	int cantDePaginas;
	int ocupado;
	int tamanioDelBloque;
	int paginaInicial;
}typedef bloqueSwap;
/*
struct nuevoPrograma{
	int PID;
	int cantidadDePaginas;
	char* contenido;
}typedef nuevo_programa;

struct escribirPagina{
	int PID;
	int nroPagina;
	char* contenido;
}typedef escribir_pagina;

struct leerPagina{
	int PID;
	int nroPagina;
}typedef leer_pagina;

struct finPrograma{
	int PID;
}typedef fin_programa;*/

typedef struct {
	int socketServer;
	int socketClient;
} t_serverData;

typedef struct __attribute__((packed)){
	int ocupada;
	int idProcesoQueLoOcupa;
}t_pagina;

t_pagina *paginasSWAP;

void *discoVirtualMappeado;
int puertoEscucha;
int tamanioDePagina;
int cantidadDePaginas;
int retardoCompactacion;
int retardoAcceso;
char* nombre_swap;
char * nombreSwapFull;
t_log* logSwap;


void eliminarProceso(int PID);
void handShake (void *parameter);
void newClients (void *parameter);
void startServer();
void crearArchivoDeConfiguracion(char* configFile);
int verificarEspacioDisponible(int cantPagsNecesita);
char* leerPagina(int PID, int page);
void mapearArchivo();
void crearArchivoDeSwap();
void compactarArchivo();
int getLastFreePage();
int getNextLoadedPage();
int checkExistenceMoreLoadedPages();
void agregarProceso(int PID, int cantPags, int pagAPartir, char* codeScript);
void escribirPaginaPID(int idProceso, int page, void* data);
void escribirPagina(int page, void*dataPagina);
void borrarContenidoPagina(int page);
long int getPageStartOffset(int page);
int getFirstPagePID(int PID);
void processingMessages(int socketClient);


#endif /* SWAP_H_ */
