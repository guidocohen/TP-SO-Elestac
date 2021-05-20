#ifndef SOCKET_H_
#define SOCKET_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <commons/string.h>
#include "common-types.h"

typedef enum{
	ACCEPTED=0,
	CONSOLA,
	NUCLEO,
	CPU,
	UMC,
	SWAP
} enum_processes;

typedef enum{
	NEW = 0,
	READY,
	RUNNING,
	BLOCKED,
	SUSPENDED
} enum_processStatus;

typedef struct{
	enum_processes process;
	char *message;
} t_MessageGenericHandshake;

typedef struct{
	enum_operationsUMC_SWAP operation;
	t_memoryLocation *virtualAddress;
	int PID;
	int cantPages;
} t_MessageUMC_Swap;

typedef struct{
	int PID;
	enum_operationsUMC_SWAP operation;
	t_memoryLocation *virtualAddress;
} t_MessageCPU_UMC;

typedef struct{
	int processID;
	int operacion;
} t_MessageCPU_Nucleo;

typedef struct{
	int PID;
	int cantPages;
	enum_operationsUMC_SWAP operation;
} t_MessageNucleo_UMC;

typedef struct{
	int processID;
	int programCounter;
	int cantidadDePaginas;// Cantidad de paginas de codigo
	int stackPointer;
	int quantum_sleep;
	int quantum;
} t_MessageNucleo_CPU;

int openServerConnection(int newSocketServerPort, int *socketServer);
int acceptClientConnection(int *socketServer, int *socketClient);
int openClientConnection(char *IPServer, int PortServer, int *socketClient);
int sendClientHandShake(int *socketClient, enum_processes process);
int sendClientAcceptation(int *socketClient);
int sendMessage (int *socketClient, void *buffer, int bufferSize);
int receiveMessage(int *socketClient, void *messageRcv, int bufferSize);
void serializeHandShake(t_MessageGenericHandshake *value, char *buffer, int valueSize);
void deserializeHandShake(t_MessageGenericHandshake *value, char *bufferReceived);

char *serializarRegistroStack(t_registroStack* registroASerializar, char* registroSerializado, int *offset);
void deserializarRegistroStack(t_registroStack* registroRecibido, char* registroSerializado, int *offset);

int serializarListaStack(t_list* listaASerializar, char** listaSerializada);
void deserializarListaStack(t_list* listaARecibir, char* listaSerializada);

char *serializarVars(t_vars* miRegistro, char* value, int *offset);
void deserializarVars(t_vars* unaVariable, char* variablesRecibidas, int *offset);

char *serializeMemoryLocation(t_memoryLocation* unaPosicion, char* value, int *offset);
void deserializeMemoryLocation(t_memoryLocation* unaPosicion, char* posicionRecibida, int *offset);

char *serializarListasVars(t_list* listaASerializar, char* listaSerializada, int *offset);
void deserializarListasVars(t_list* listaVars,char* listaSerializada, int *offset);

char *serializarStack(t_registroStack* registroStack, char* registroSerializado, int *offset);
void deserializarStack(t_registroStack* estructuraARecibir, char* registroStack, int *offset);

char *serializeListaArgs(t_list* listaASerializar, char* listaSerializada, int *offset);
void deserializeListasArgs(t_list* listaArgs,char* listaSerializada, int *offset);

void serializarIndiceDeCodigo(t_registroIndiceCodigo* registroEnviar, char* registroSerializado);
void deserializarIndiceDeCodigo(t_registroIndiceCodigo* registroARecibir, char* registroSerializado);

int serializarListaIndiceDeCodigo(t_list* listaASerializar, char** listaSerializada);
void deserializarListaIndiceDeCodigo(t_list* listaARecibir, char* listaSerializada);

char *getProcessString (enum_processes process);

//IMPORTANTE!!! --> Nomeclatura de Serializadores y Deserealizadores
//1) serialize<FromProcess>_<ToProcess> ()
//2) deserialize<ToProcess>_<FromProcess> ()

void serializeUMC_Swap(t_MessageUMC_Swap *value, char *buffer, int valueSize);
void deserializeSwap_UMC(t_MessageUMC_Swap *value, char *bufferReceived);

void serializeNucleo_UMC(t_MessageNucleo_UMC *value, char *buffer, int valueSize);
void deserializeUMC_Nucleo(t_MessageNucleo_UMC *value, char *bufferReceived);

void serializeCPU_UMC(t_MessageCPU_UMC *value, char *buffer, int valueSize);
void deserializeUMC_CPU(t_MessageCPU_UMC *value, char *bufferReceived);

void serializeNucleo_CPU(t_MessageNucleo_CPU *value, char *buffer, int valueSize);
void deserializeCPU_Nucleo(t_MessageNucleo_CPU *value, char *bufferReceived);

void serializeCPU_Nucleo(t_MessageCPU_Nucleo *value, char *buffer, int valueSize);
void deserializeNucleo_CPU(t_MessageCPU_Nucleo *value, char *bufferReceived);

//Funciones para el PCB

void cleanIndiceDeStack(t_registroStack* registroS);

void destruirPCB(t_PCB* PCB);
void destruirRegistroIndiceDeCodigo(t_registroIndiceCodigo *registroIC);
void destruirRegistroIndiceDeStack(t_registroStack* registroS);
void destruirArgs(t_memoryLocation* args);
void destruirVars(t_vars* vars);

#endif /*SOCKET_H_*/
