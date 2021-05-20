/*
 * nucleo.h
 *
 */

#ifndef NUCLEO_H_
#define NUCLEO_H_
#define EOL_DELIMITER ";"

#include "sockets.h"
#include "commons/log.h"
#include "commons/temporal.h"
#include "commons/process.h"
#include "commons/txt.h"
#include "commons/collections/list.h"
#include "commons/collections/dictionary.h"
#include "commons/collections/queue.h"
#include "commons/config.h"
#include "commons/log.h"
#include <signal.h>
#include <sys/types.h>
#include <pthread.h>
#include <semaphore.h>
#include <math.h>
#include <ctype.h>

// Estructuras
typedef struct {
	int puerto_prog;
	int puerto_cpu;
	int puerto_umc;
	char* ip_umc;
	int quantum;
	int quantum_sleep;
	char** sem_ids;
	int* sem_ids_values;
	char** sem_init;
	char** io_ids;
	int* io_ids_values;
	char** io_sleep;
	char** shared_vars;
	int* shared_vars_values;
	int stack_size;
} t_configFile;

typedef struct {
	int socketServer;
	int socketClient;
} t_serverData;

//Estructura Procesos Bloqueados
typedef struct {
	int PID;
	int tiempo;
	t_nombre_dispositivo dispositivo;
} t_bloqueado;

//Estructura Procesos en cola
typedef struct {
	int PID;
	int ProgramCounter;
} t_proceso;

//Estructura datosCPU
typedef struct {
	int numSocket;
	int estadoCPU;
} t_datosCPU;

//Estructura datosConsola
typedef struct {
	int PID;
	int numSocket;
} t_datosConsola;

//Semaforos
pthread_mutex_t listadoCPU;
pthread_mutex_t listadoConsola;
pthread_mutex_t listadoProcesos;
pthread_mutex_t cListos;
pthread_mutex_t cBloqueados;
pthread_mutex_t cFinalizar;
pthread_mutex_t cSemaforos;
pthread_mutex_t globalMutex;
pthread_mutex_t activeProcessMutex;
pthread_mutex_t mutex_config;

//timer_t** timers;

//Configuracion
t_configFile configNucleo;

//Logger
t_log* logNucleo;

//Variables de Listas
t_list* listaCPU;
t_list* listaConsola;
t_list* listaProcesos;

//Variables de Colas
t_queue* colaListos;
t_queue* colaFinalizar;
t_queue** colas_IO;
t_queue** colas_semaforos;

//Variables Globales
int idProcesos = 1;
int activePID = 0;
int socketUMC = 0;
int socketConsola = 0;
int frameSize = 0;
int alertFlag = 0;
int z = 0;

//Encabezamientos de Funciones Principales

void runScript(char* codeScript, int socketConsola);
void planificarProceso();
void enviarMsjCPU(t_PCB* datosPCB,t_MessageNucleo_CPU* contextoProceso,t_serverData* serverData);
void finalizaProceso(int socket, int PID, int estado);
void atenderCorteQuantum(int socket, int PID);

//Encabezamiento de Funciones Secundarias

int buscarCPULibre();
int buscarPCB(int pid);
void cambiarEstadoProceso(int PID, int estado);
void liberarCPU(int socket);
int buscarCPU(int socket);
int buscarConsola(int socket);
int buscarPIDConsola(int socket);
int buscarSocketConsola(int PID);
int estaEjecutando(int PID, int* index);
void actualizarPC(int PID, int ProgramCounter);
void inicializarMutex();
void crearArchivoDeConfiguracion(char* configFile);
void *initialize(int tamanio);
void imprimirArray(char** array);
void imprimirValores(int* array, int i, int arrayLen);
void enviarUno(int PID);

//Encabezamiento de Funciones de Stack

void armarIndiceDeCodigo(t_PCB *unBloqueControl,t_metadata_program* miMetaData);
void armarIndiceDeEtiquetas(t_PCB *unBloqueControl,t_metadata_program* miMetaData);

//Privilegiadas

t_valor_variable obtenerValor(t_nombre_compartida variable);
void grabarValor(t_nombre_compartida variable, t_valor_variable valor);
void entradaSalida(t_nombre_dispositivo dispositivo, int tiempo);
void deserializarES(t_es* datos, char* bufferRecived);
void atenderIO(int io);

t_valor_variable pideSemaforo(t_nombre_semaforo semaforo);
void grabarSemaforo(t_nombre_semaforo semaforo,int valor);
void liberaSemaforo(t_nombre_semaforo semaforo);//SIGNAL
void bloqueoSemaforo(int processID, t_nombre_semaforo semaforo, t_list* listaIndiceDeStack);//WAIT

//Conexiones y Funciones para los mensajes

void startServerProg();
void startServerCPU();
void newClients(void *parameter);
void handShake(void *parameter);
void processMessageReceivedConsola(void *parameter);
int processMessageReceivedCPU(void *parameter);
//void processMessageReceived (void *parameter);
int processCPUMessages(int messageSize,int socketLibre);

int connectTo(enum_processes processToConnect, int *socketClient);
void startNucleoConsole();
void consoleMessageNucleo();

void iniciarPrograma(int PID, char* codeScript);
void finalizarPrograma(int PID);
void finalizarPid(int PID);
void procesarFinalizacion(int PID);

#endif /* NUCLEO_H_ */
