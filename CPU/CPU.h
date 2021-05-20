#ifndef CPU_H_
#define CPU_H_

#include "sockets.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <parser/metadata_program.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <commons/log.h>
#include <signal.h>

typedef struct {
	int port_Nucleo;
	char *ip_Nucleo;
	int port_UMC;
	char *ip_UMC;
} t_configFile;

t_configFile configuration;

//Encabezamientos primitivas
void finalizar(void);
t_puntero definirVariable(t_nombre_variable nombreVariable);
t_puntero obtenerPosicionVariable(t_nombre_variable identificador_variable);
t_valor_variable dereferenciar(t_puntero direccion_variable);
void asignar(t_puntero direccion_variable, t_valor_variable valor);
t_valor_variable obtenerValorCompartida(t_nombre_compartida variable);
t_valor_variable asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor);
void irAlLabel(t_nombre_etiqueta etiqueta);
void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar);
void llamarSinRetorno(t_nombre_etiqueta etiqueta);
void retornar(t_valor_variable retorno);
void imprimir(t_valor_variable valor_mostrar);
void imprimirTexto(char *texto);
void entradaSalida(t_nombre_dispositivo dispositivo, int tiempo);
void waitPrimitive(t_nombre_semaforo identificador_semaforo);
void signalPrimitive(t_nombre_semaforo identificador_semaforo);

t_puntero convertirDireccionAPuntero(t_memoryLocation* direccion);
void convertirPunteroADireccion(t_puntero puntero, t_memoryLocation* direccion);


/* PROTOTIPOS FUNCIONES PRIMITIVAS en parser.h
t_puntero (*AnSISOP_definirVariable)(t_nombre_variable identificador_variable);
t_puntero (*AnSISOP_obtenerPosicionVariable)(t_nombre_variable identificador_variable);
t_valor_variable (*AnSISOP_dereferenciar)(t_puntero direccion_variable);
void (*AnSISOP_asignar)(t_puntero direccion_variable, t_valor_variable valor);
t_valor_variable (*AnSISOP_obtenerValorCompartida)(t_nombre_compartida variable);
t_valor_variable (*AnSISOP_asignarValorCompartida)(t_nombre_compartida variable, t_valor_variable valor);
void (*AnSISOP_irAlLabel)(t_nombre_etiqueta t_nombre_etiqueta);
void (*AnSISOP_llamarSinRetorno)(t_nombre_etiqueta etiqueta);
void (*AnSISOP_llamarConRetorno)(t_nombre_etiqueta etiqueta, t_puntero donde_retornar);
void (*AnSISOP_finalizar)(void);
void (*AnSISOP_retornar)(t_valor_variable retorno);
void (*AnSISOP_imprimir)(t_valor_variable valor_mostrar);
void (*AnSISOP_imprimirTexto)(char* texto);
void (*AnSISOP_entradaSalida)(t_nombre_dispositivo dispositivo, int tiempo);
*/

AnSISOP_funciones funciones = {
		.AnSISOP_definirVariable			= definirVariable,
		.AnSISOP_obtenerPosicionVariable	= obtenerPosicionVariable,
		.AnSISOP_dereferenciar				= dereferenciar,
		.AnSISOP_asignar					= asignar,
		.AnSISOP_obtenerValorCompartida		= obtenerValorCompartida,
		.AnSISOP_asignarValorCompartida		= asignarValorCompartida,
		.AnSISOP_irAlLabel					= irAlLabel,
		.AnSISOP_llamarSinRetorno			= llamarSinRetorno,
		.AnSISOP_llamarConRetorno			= llamarConRetorno,
		.AnSISOP_finalizar					= finalizar,
		.AnSISOP_retornar					= retornar,
		.AnSISOP_imprimir					= imprimir,
		.AnSISOP_imprimirTexto				= imprimirTexto,
		.AnSISOP_entradaSalida				= entradaSalida,
};

/* PROTOTIPOS FUNCIONES KERNEL en parser.h
void (*AnSISOP_wait)(t_nombre_semaforo identificador_semaforo);
void (*AnSISOP_signal)(t_nombre_semaforo identificador_semaforo);
*/

AnSISOP_kernel funciones_kernel = {
		.AnSISOP_wait	= waitPrimitive,
		.AnSISOP_signal	= signalPrimitive,
};

int frameSize = 0;
int stackSize = 0;
int socketUMC = 0;
int socketNucleo = 0;
int QUANTUM_SLEEP = 0 ;
int QUANTUM = 0;
int ultimoPosicionPC;
t_log* logCPU;
t_PCB* PCBRecibido = NULL;
t_list *listaIndiceEtiquetas = NULL;
bool SignalActivated = false; //Signal use FALSE by DEFAULT
bool waitSemActivated = false; //wait sem use FALSE by DEFAULT
bool functionCall = false;

int ejecutarPrograma();
int connectTo(enum_processes processToConnect, int *socketClient);
void crearArchivoDeConfiguracion(char *configFile);
void sighandler(int signum);
void waitRequestFromNucleo(int *socketClient, char **messageRcv);
void deserializarListaIndiceDeEtiquetas(char* charEtiquetas, int listaSize);
void destruirRegistroIndiceEtiquetas(t_registroIndiceEtiqueta* registroEtiqueta);
void destroyIndiceEtiquetas();
void manejarES(int PID, int pcActualizado, int* banderaFinQuantum, int tiempoBloqueo);
void serializarES(t_es *value, char* buffer, int valueSize);
void cargarValoresNuevaPosicion(t_memoryLocation* ultimaPosicionOcupada, t_memoryLocation* variableAAgregar);
void getReturnStackRegister(t_registroStack* registroARegresar);
t_memoryLocation* buscarEnElStackPosicionPagina(t_PCB* pcb);
t_memoryLocation* buscarUltimaPosicionOcupada(t_PCB* pcbEjecutando);

#endif /* CPU_H_ */
