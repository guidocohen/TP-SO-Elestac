#ifndef UMC_H_
#define UMC_H_

#include <errno.h>
#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <semaphore.h>
#include "sockets.h"
#include <commons/config.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <commons/log.h>
#include <ctype.h>

#define PAGE_PRESENT 1
#define PAGE_NOTPRESENT 0
#define PAGE_MODIFIED 1
#define PAGE_NOTMODIFIED 0

typedef struct {
	int port;
	char *ip_swap;
	int port_swap;
	int frames_max;
	int frames_size;
	int frames_max_proc;
	int TLB_entries; /* 0 = Disable */
	int delay;
	char *algorithm_replace;
} t_configFile;

typedef struct {
	int socketServer;
	int socketClient;
} t_serverData;

typedef struct{
	t_memoryLocation *virtualAddress;
	int frameNumber;
	int PID;
	unsigned dirtyBit : 1;//field of 1 bit
	unsigned presentBit : 1;//field of 1 bit
} t_memoryAdmin; /* It will be created as a fixed list at the beginning of the process using PID as element index */

typedef struct{
	int PID;
	int assignedFrames;
	t_list *ptrPageTable;//lista con registros del tipo t_memoryAdmin
} t_pageTablesxProc;

typedef enum{
	PUERTO = 0,
	IP_SWAP,
	PUERTO_SWAP,
	MARCOS,
	MARCO_SIZE,
	MARCO_X_PROC,
	ENTRADAS_TLB,
	RETARDO,
	ALGORITMO
} enum_configParameters;

typedef enum{
	TLB = 0,
	MAIN_MEMORY
} enum_memoryStructure;

typedef enum{
	READ = 0,
	WRITE
} enum_memoryOperations;

/***** Global variables *****/
t_configFile configuration;
char *dumpFile = NULL;
FILE *dumpf;
t_log* UMCLog;
void *memBlock = NULL;
int activePID = -1;
int socketSwap = 0;
bool TLBActivated = false; //TLB use FALSE by DEFAULT
t_list *freeFramesList = NULL;//lista con numeros de frames libres
t_list *TLBList = NULL;//lista con registros del tipo t_memoryAdmin
t_list *pageTablesListxProc = NULL;//lista con registros del tipo t_pageTablesxProc
pthread_mutex_t memoryAccessMutex;
pthread_mutex_t activeProcessMutex;
pthread_mutex_t delayMutex;

/***** Prototype functions *****/

//UMC operations
void procesCPUMessages(int messageSize, t_serverData* serverData);
void procesNucleoMessages(int messageSize, t_serverData* serverData);
void getConfiguration(char *configFile);
void createTLB();
void resetTLBAllEntries();
t_memoryAdmin *getLRUCandidate();
void resetTLBbyActivePID(t_memoryAdmin *listElement);
void deleteContentFromMemory(t_memoryAdmin *memoryElement);
void checkPageModification(t_memoryAdmin *memoryElement);
void destroyElementTLB(t_memoryAdmin *elementTLB);
void PageTable_Element_destroy(t_pageTablesxProc *self);
bool is_PIDPageTablePresent(t_pageTablesxProc* listElement);
bool isThereEmptyEntry(t_memoryAdmin* listElement);
bool isPageNOTPresent(t_memoryAdmin* listElement);
bool isPageNOTPresentNOTModified(t_memoryAdmin* listElement);
bool find_PIDEntry_PageTable(t_pageTablesxProc* listElement);
bool find_PIDEntry_TLB(t_memoryAdmin *listElement);
void iteratePageTablexProc(t_pageTablesxProc *pageTablexProc);
void dumpPageTablexProc(t_pageTablesxProc *pageTablexProc);
void showPageTableRows(t_memoryAdmin *pageTableElement);
void dumpMemoryxProc(t_pageTablesxProc *pageTablexProc);
void showMemoryRows(t_memoryAdmin *pageTableElement);
void markElementModified(t_memoryAdmin *pageTableElement);
void markElementNOTPResent(t_memoryAdmin *pageTableElement);
void consoleMessageUMC();
void createAdminStructs();
int getEnum(char *string);

//Communications
void startUMCConsole();
void startServer();
void newClients (void *parameter);
void processMessageReceived (void *parameter);
void handShake (void *parameter);

//UMC functions
void initializeProgram(int PID, int totalPagesRequired, char *programCode);
void *requestBytesFromPage(t_memoryLocation *virtualAddress);
int writeBytesToPage(t_memoryLocation *virtualAddress, void *buffer);
void endProgram(int PID);
t_memoryAdmin *getElementFrameNro(t_memoryLocation *virtualAddress, enum_memoryOperations operation);
t_memoryAdmin *searchFramebyPage(enum_memoryStructure deviceLocation, enum_memoryOperations operation, t_memoryLocation *virtualAddress);
t_memoryAdmin *updateMemoryStructure(t_pageTablesxProc *pageTablexProc, t_memoryLocation *virtualAddress, t_memoryAdmin *memoryElement);
void executeLRUAlgorithm(t_memoryAdmin *newElement, t_memoryLocation *virtualAddress);
void executeMainMemoryAlgorithm(t_pageTablesxProc *pageTablexProc, t_memoryAdmin *newElement, void *memoryContent);
void *requestPageToSwap(t_memoryLocation *virtualAddress, int PID);
void waitForResponse();
void changeActiveProcess(int PID);
char *getMemoryString (enum_memoryStructure memoryStructure);

#endif /* UMC_H_ */
