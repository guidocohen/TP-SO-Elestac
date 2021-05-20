#include "UMC.h"

int main(int argc, char *argv[]){
	int exitCode = EXIT_FAILURE ; //DEFAULT failure
	char *configurationFile = NULL;
	char *logFile = NULL;
	pthread_t serverThread;
	pthread_t consolaThread;

	assert(("ERROR - NOT arguments passed", argc > 1)); // Verifies if was passed at least 1 parameter, if DONT FAILS

	//get parameter
	int i;
	for( i = 0; i < argc; i++){
		//check Configuration file parameter
		if (strcmp(argv[i], "-c") == 0){
			configurationFile = argv[i+1];
			printf("Configuration File: '%s'\n",configurationFile);
		}
		//check Dump file parameter
		if (strcmp(argv[i], "-d") == 0){
			dumpFile = argv[i+1];
			printf("Dump File: '%s'\n",dumpFile);
		}
		//check log file parameter
		if (strcmp(argv[i], "-l") == 0){
			logFile = argv[i+1];
			printf("Log File: '%s'\n",logFile);
		}
	}

	//ERROR if not configuration parameter was passed
	assert(("ERROR - NOT configuration file was passed as argument", configurationFile != NULL));//Verifies if was passed the configuration file as parameter, if DONT FAILS

	//ERROR if not dump parameter was passed
	assert(("ERROR - NOT dump file was passed as argument", dumpFile != NULL));//Verifies if was passed the Dump file as parameter, if DONT FAILS

	//ERROR if not log parameter was passed
	assert(("ERROR - NOT log file was passed as argument", logFile != NULL));//Verifies if was passed the Dump file as parameter, if DONT FAILS

	//Creating log file
	UMCLog = log_create(logFile, "UMC", 0, LOG_LEVEL_TRACE);

	//get configuration from file
	getConfiguration(configurationFile);

	//created administration structures for UMC process
	createAdminStructs();

	//Connecting to SWAP before starting everything else
	exitCode = connectTo(SWAP,&socketSwap);

	if(exitCode == EXIT_SUCCESS){
		log_info(UMCLog, "UMC connected to SWAP successfully");

		//Initializing mutex
		pthread_mutex_init(&memoryAccessMutex, NULL);
		pthread_mutex_init(&activeProcessMutex, NULL);

		//Create thread for UMC console
		pthread_create(&consolaThread, NULL, (void*) startUMCConsole, NULL);

		//Create thread for server start
		pthread_create(&serverThread, NULL, (void*) startServer, NULL);

		pthread_join(consolaThread, NULL);
		pthread_join(serverThread, NULL);
	}

	return exitCode;
}

void startServer(){
	int exitCode = EXIT_FAILURE; //DEFAULT Failure
	int socketServer;

	exitCode = openServerConnection(configuration.port, &socketServer);
	log_info(UMCLog, "SocketServer: %d", socketServer);

	//If exitCode == 0 the server connection is opened and listening
	if (exitCode == 0) {
		log_info(UMCLog, "The UMC server is opened.");

		exitCode = listen(socketServer, SOMAXCONN);

		if (exitCode < 0 ){
			log_error(UMCLog,"Failed to listen server Port.");
			return;
		}

		while (1){
			newClients((void*) &socketServer);
		}
	}

}

void newClients (void *parameter){
	int exitCode = EXIT_FAILURE; //DEFAULT Failure
	int pid;

	t_serverData *serverData = malloc(sizeof(t_serverData));
	memcpy(&serverData->socketServer, parameter, sizeof(serverData->socketServer));

	// disparar un thread para acceptar cada cliente nuevo (debido a que el accept es bloqueante) y para hacer el handshake
	/**************************************/
	//Create thread attribute detached
	//			pthread_attr_t acceptClientThreadAttr;
	//			pthread_attr_init(&acceptClientThreadAttr);
	//			pthread_attr_setdetachstate(&acceptClientThreadAttr, PTHREAD_CREATE_DETACHED);
	//
	//			//Create thread for checking new connections in server socket
	//			pthread_t acceptClientThread;
	//			pthread_create(&acceptClientThread, &acceptClientThreadAttr, (void*) acceptClientConnection1, &serverData);
	//
	//			//Destroy thread attribute
	//			pthread_attr_destroy(&acceptClientThreadAttr);
	/************************************/

	exitCode = acceptClientConnection(&serverData->socketServer, &serverData->socketClient);

	if (exitCode == EXIT_FAILURE){
		log_warning(UMCLog,"There was detected an attempt of wrong connection.");
		close(serverData->socketClient);
		free(serverData);
	}else{
		log_info(UMCLog,"The was received a connection in socket: %d.",serverData->socketClient);
		//Create thread attribute detached
		pthread_attr_t handShakeThreadAttr;
		pthread_attr_init(&handShakeThreadAttr);
		pthread_attr_setdetachstate(&handShakeThreadAttr, PTHREAD_CREATE_DETACHED);

		//Create thread for checking new connections in server socket
		pthread_t handShakeThread;
		pthread_create(&handShakeThread, &handShakeThreadAttr, (void*) handShake, serverData);

		//Destroy thread attribute
		pthread_attr_destroy(&handShakeThreadAttr);

	}// END handshakes

}

void handShake (void *parameter){
	int exitCode = EXIT_FAILURE; //DEFAULT Failure

	t_serverData *serverData = (t_serverData*) parameter;

	//Receive message size
	int messageSize = 0;
	char *messageRcv = malloc(sizeof(messageSize));
	int receivedBytes = receiveMessage(&serverData->socketClient, messageRcv, sizeof(messageSize));

	//Receive message using the size read before
	memcpy(&messageSize, messageRcv, sizeof(int));
	messageRcv = realloc(messageRcv,messageSize);
	receivedBytes = receiveMessage(&serverData->socketClient, messageRcv, messageSize);

	//starting handshake with client connected
	t_MessageGenericHandshake *message = malloc(sizeof(t_MessageGenericHandshake));
	deserializeHandShake(message, messageRcv);

	//Now it's checked that the client is not down
	if ( receivedBytes == 0 ){
		log_error(UMCLog,"The client went down while handshaking! - Please check the client '%d' is down!", serverData->socketClient);
		close(serverData->socketClient);
		free(serverData);
	}else{
		switch ((int) message->process){
			case CPU:{
				log_info(UMCLog,"Message from '%s': %s", getProcessString(message->process), message->message);

				exitCode = sendClientAcceptation(&serverData->socketClient);

				if (exitCode == EXIT_SUCCESS){

					//After sending ACCEPTATION has to be sent the "Tamanio de pagina" information
					exitCode = sendMessage(&serverData->socketClient, &configuration.frames_size , sizeof(configuration.frames_size));

					//Create thread attribute detached
					pthread_attr_t processMessageThreadAttr;
					pthread_attr_init(&processMessageThreadAttr);
					pthread_attr_setdetachstate(&processMessageThreadAttr, PTHREAD_CREATE_DETACHED);

					//Create thread for checking new connections in server socket
					pthread_t processMessageThread;
					pthread_create(&processMessageThread, &processMessageThreadAttr, (void*) processMessageReceived, parameter);

					//Destroy thread attribute
					pthread_attr_destroy(&processMessageThreadAttr);
				}

				break;
			}
			case NUCLEO:{
				log_info(UMCLog,"Message from '%s': %s", getProcessString(message->process), message->message);

				exitCode = sendClientAcceptation(&serverData->socketClient);

				if (exitCode == EXIT_SUCCESS){

					//After sending ACCEPTATION has to be sent the "Tamanio de pagina" information
					exitCode = sendMessage(&serverData->socketClient, &configuration.frames_size , sizeof(configuration.frames_size));

					//processMessageReceived(parameter);

					//Create thread attribute detached
					pthread_attr_t processMessageThreadAttr;
					pthread_attr_init(&processMessageThreadAttr);
					pthread_attr_setdetachstate(&processMessageThreadAttr, PTHREAD_CREATE_DETACHED);

					//Create thread for checking new connections in server socket
					pthread_t processMessageThread;
					pthread_create(&processMessageThread, &processMessageThreadAttr, (void*) processMessageReceived, parameter);

					//Destroy thread attribute
					pthread_attr_destroy(&processMessageThreadAttr);
				}

				break;
			}
			default:{
				log_error(UMCLog,"Process not allowed to connect - Invalid process '%s' tried to connect to UMC", getProcessString(message->process));
				close(serverData->socketClient);
				free(serverData);
				break;
			}
		}
	}

	free(messageRcv);
	free(message->message);
	free(message);
}

void processMessageReceived (void *parameter){

	t_serverData *serverData = (t_serverData*) parameter;

	while(1){
		//Receive message size
		int messageSize = 0;

		//Get Payload size
		int receivedBytes = receiveMessage(&serverData->socketClient, &messageSize, sizeof(messageSize));

		if ( receivedBytes > 0 ){


			//Receive process from which the message is going to be interpreted
			enum_processes fromProcess;
			receivedBytes = receiveMessage(&serverData->socketClient, &fromProcess, sizeof(fromProcess));

			log_info(UMCLog, "\n\nMessage size received from process '%s' in socket cliente '%d': %d\n",getProcessString(fromProcess), serverData->socketClient, messageSize);

			switch (fromProcess){
			case CPU:{
				log_info(UMCLog, "Processing CPU message received");
				pthread_mutex_lock(&activeProcessMutex);
				procesCPUMessages(messageSize, serverData);
				pthread_mutex_unlock(&activeProcessMutex);
				break;
			}
			case NUCLEO:{
				log_info(UMCLog, "Processing NUCLEO message received");
				pthread_mutex_lock(&activeProcessMutex);
				procesNucleoMessages(messageSize, serverData);
				pthread_mutex_unlock(&activeProcessMutex);
				break;
			}
			default:{
				log_error(UMCLog,"Process not allowed to connect - Invalid process '%s' send a message to UMC", getProcessString(fromProcess));
				close(serverData->socketClient);
				free(serverData);
				break;
			}
			}

		}else if (receivedBytes == 0 ){
			//The client is down when bytes received are 0
			log_error(UMCLog,"The client went down while receiving! - Please check the client '%d' is down!", serverData->socketClient);
			close(serverData->socketClient);
			free(serverData);
			break;
		}else{
			log_error(UMCLog, "Error - No able to received - Error receiving from socket '%d', with error: %d",serverData->socketClient,errno);
			close(serverData->socketClient);
			free(serverData);
			break;
		}

	}

}

void procesCPUMessages(int messageSize, t_serverData* serverData){
	int exitcode = EXIT_SUCCESS;

	//Receive message using the size read before
	char *messageRcv = malloc(messageSize);
	int receivedBytes = receiveMessage(&serverData->socketClient, messageRcv, messageSize);

	t_MessageCPU_UMC *message = malloc(sizeof(t_MessageCPU_UMC));
	message->virtualAddress = malloc(sizeof(t_memoryLocation));

	//Deserialize messageRcv
	deserializeUMC_CPU(message, messageRcv);

	changeActiveProcess(message->PID);// rollbacking this because of GIT issue #312

	switch (message->operation){
		case lectura_pagina:{

			char *content = NULL;

			content = (char*) requestBytesFromPage(message->virtualAddress);

			if(content != NULL){
				exitcode = EXIT_SUCCESS;
				//After getting the memory content WITHOUT issues, inform status of the operation
				sendMessage(&serverData->socketClient, &exitcode, sizeof(exitcode));

				//Add \0 to the end of the message
				if(string_starts_with(content,"#!")){
					memset(content + message->virtualAddress->size,'\0',1 );
				}

				//has to be sent it to upstream - on the other side the process already know the size to received
				sendMessage(&serverData->socketClient, content, message->virtualAddress->size);
			}else{
				exitcode = EXIT_FAILURE;
				//The main memory hasn't any free frames - inform status of the operation
				sendMessage(&serverData->socketClient, &exitcode, sizeof(exitcode));
			}

			free(content);

			break;
		}
		case escritura_pagina:{

			//Receive value ALWAYS IS AN INT
			int value;
			receivedBytes = receiveMessage(&serverData->socketClient, &value, sizeof(value));

			exitcode = writeBytesToPage(message->virtualAddress, &value);

			//After writing the memory content inform status of the operation
			sendMessage(&serverData->socketClient, &exitcode, sizeof(exitcode));

			break;
		}
		default:{
			log_error(UMCLog,"Process not allowed to connect - Invalid operation for CPU messages '%d' requested to UMC",(int) message->operation);
			break;
		}
	}
}

void procesNucleoMessages(int messageSize, t_serverData* serverData){
	int exitcode = EXIT_SUCCESS;

	//Receive message using the size read before
	char *messageRcv = malloc(messageSize);
	int receivedBytes = receiveMessage(&serverData->socketClient, messageRcv, messageSize);

	t_MessageNucleo_UMC *message = malloc(sizeof(t_MessageNucleo_UMC));

	//Deserialize messageRcv
	deserializeUMC_Nucleo(message, messageRcv);

	changeActiveProcess(message->PID);// rollbacking this because of GIT issue #312

	switch (message->operation){
		case agregar_proceso:{

			//Receive content size
			messageSize = 0;//reseting size to get the content size
			receivedBytes = 0;//reseting receivedBytes to get the content size

			receivedBytes = receiveMessage(&serverData->socketClient, &messageSize, sizeof(messageSize));
			log_info(UMCLog, "Program size received: %d", messageSize);

			//Receive content using the size read before
			char *content = malloc(messageSize);
			receivedBytes = receiveMessage(&serverData->socketClient, content, messageSize);
			string_append(&content, "\0");
			log_info(UMCLog, "Program received:\n%s\n", content);

			initializeProgram(message->PID, message->cantPages, content);

			//received possible error from SWAP if is OK then send code
			int errorCode = EXIT_SUCCESS;
			int receivedBytes = receiveMessage(&socketSwap, &errorCode, sizeof(errorCode));

			if (errorCode == EXIT_SUCCESS){
				log_info(UMCLog, "Program added successfully in SWAP");
			}else{
				log_info(UMCLog, "Program NOT LOADED in SWAP... Informing to Nucleo");
				//sendMessage(&serverData->socketClient, &errorCode,sizeof(errorCode)); //TODO verificar y agregar receive in nucleo
			}

			free(content);
			break;
		}
		case finalizar_proceso:{

			endProgram(message->PID);

			break;
		}
		default:{
			log_error(UMCLog, "Process not allowed to connect - Invalid operation for CPU messages '%d' requested to UMC",(int) message->operation);
			close(serverData->socketClient);
			free(serverData);
			break;
		}
	}
}

int connectTo(enum_processes processToConnect, int *socketClient){
	int exitcode = EXIT_FAILURE;//DEFAULT VALUE
	int port = 0;
	char *ip = string_new();

	switch (processToConnect){
		case SWAP:{
			 string_append(&ip,configuration.ip_swap);
			 port= configuration.port_swap;
			break;
		}
		default:{
			log_info(UMCLog,"Process '%s' NOT VALID to be connected by UMC.", getProcessString(processToConnect));
			break;
		}
	}

	exitcode = openClientConnection(ip, port, socketClient);

	//If exitCode == 0 the client could connect to the server
	if (exitcode == EXIT_SUCCESS){

		// ***1) Send handshake
		exitcode = sendClientHandShake(socketClient,UMC);

		if (exitcode == EXIT_SUCCESS){

			// ***2)Receive handshake response
			//Receive message size
			int messageSize = 0;
			char *messageRcv = malloc(sizeof(messageSize));
			int receivedBytes = receiveMessage(socketClient, messageRcv, sizeof(messageSize));

			if ( receivedBytes > 0 ){
				//Receive message using the size read before
				memcpy(&messageSize, messageRcv, sizeof(int));
				messageRcv = realloc(messageRcv,messageSize);
				receivedBytes = receiveMessage(socketClient, messageRcv, messageSize);

				//starting handshake with client connected
				t_MessageGenericHandshake *message = malloc(sizeof(t_MessageGenericHandshake));
				deserializeHandShake(message, messageRcv);

				free(messageRcv);

				switch (message->process){
					case ACCEPTED:{
						switch(processToConnect){
							case SWAP:{
								log_info(UMCLog, "Connected to SWAP - Message: %s",message->message);
								break;
							}
							default:{
								log_error(UMCLog, "Handshake not accepted when tried to connect your '%s' with '%s",getProcessString(processToConnect),getProcessString(message->process));
								close(*socketClient);
								exitcode = EXIT_FAILURE;
								break;
							}
						}

						break;
					}
					default:{
						log_error(UMCLog, "Process couldn't connect to SERVER - Not able to connect to server %s. Please check if it's down.",ip);
						close(*socketClient);
						break;
						break;
					}
				}

			}else if (receivedBytes == 0 ){
				//The client is down when bytes received are 0
				log_error(UMCLog,"The client went down while receiving! - Please check the client '%d' is down!", *socketClient);
				close(*socketClient);
			}else{
				log_error(UMCLog, "Error - No able to received - Error receiving from socket '%d', with error: %d",*socketClient,errno);
				close(*socketClient);
			}
		}

	}else{
		log_error(UMCLog, "I'm not able to connect to the server! - My socket is: '%d'", *socketClient);
		close(*socketClient);
	}

	return exitcode;
}

void getConfiguration(char *configFile){

	t_config* configurationFile;
	configurationFile = config_create(configFile);
	configuration.port = config_get_int_value(configurationFile,"PUERTO");
	configuration.ip_swap = config_get_string_value(configurationFile,"IP_SWAP");
	configuration.port_swap = config_get_int_value(configurationFile,"PUERTO_SWAP");
	configuration.frames_max = config_get_int_value(configurationFile,"MARCOS");
	configuration.frames_size = config_get_int_value(configurationFile,"MARCO_SIZE");
	configuration.frames_max_proc = config_get_int_value(configurationFile,"MARCO_X_PROC");
	configuration.algorithm_replace = config_get_string_value(configurationFile,"ALGORITMO");
	configuration.TLB_entries = config_get_int_value(configurationFile,"ENTRADAS_TLB");
	int delay = config_get_int_value(configurationFile,"RETARDO");
	configuration.delay = delay/1000; //changing value to milliseconds

}

void startUMCConsole(){
	char command[50];
	char option[50];
	char value[50];

	consoleMessageUMC();

	while (1){
		scanf("%s %s", command, option);

		//cleaning screen
		system("clear");

		int i;
		//lower case options
		for(i = 0; option[i] != '\0'; i++){
			option[i] = tolower(option[i]);
		}

		if (strcmp(command,"retardo") == 0 ){
			pthread_mutex_lock(&delayMutex);
			configuration.delay = atoi(option)/1000;
			pthread_mutex_unlock(&delayMutex);
			printf("The delay UMC was successfully changed to '%d' millisecond\n", atoi(option));
			log_info(UMCLog, "The delay UMC was successfully changed to '%d' millisecond", atoi(option));

		}else if (strcmp(command,"dump") == 0 ){
			int neededPID = 0; //DEFAULT value

			//Auxiliary function - Look for page table by PID
			bool is_PIDPageTable(t_pageTablesxProc* listElement){
				return (listElement->PID == neededPID);
			}

			printf("\nCommand entered: '%s %s'\n", command,option);
			printf("== [VALUE]\n");
			printf("all\t\t:: Todos los procesos\n");
			printf("<processPID>\t:: PID del proceso deseado\n\nPlease enter a value with the above format: ");
			scanf("%s", value);

			if(list_size(pageTablesListxProc) != 0){

				dumpf = fopen(dumpFile, "w");

				if (strcmp(value, "all") == 0){
					neededPID = -1; //ALL PIDs in memory
				}else{
					neededPID = atoi(value); //given PID in memory
				}

				if (strcmp(option, "estructuras") == 0){

					if(neededPID == -1){
						pthread_mutex_lock(&memoryAccessMutex);
						list_iterate(pageTablesListxProc,(void*) dumpPageTablexProc);
						pthread_mutex_unlock(&memoryAccessMutex);
					}else{
						//Look for table page by neededPID
						pthread_mutex_lock(&memoryAccessMutex);
						t_pageTablesxProc *pageTablexProc = list_find(pageTablesListxProc,(void*) is_PIDPageTable);
						list_iterate(pageTablexProc->ptrPageTable, (void*) showPageTableRows);
						pthread_mutex_unlock(&memoryAccessMutex);
					}

				}else if (strcmp(option, "contenido") == 0){

					if(neededPID == -1){
						pthread_mutex_lock(&memoryAccessMutex);
						list_iterate(pageTablesListxProc,(void*) dumpMemoryxProc);
						pthread_mutex_unlock(&memoryAccessMutex);
					}else{
						//Look for table page by neededPID
						pthread_mutex_lock(&memoryAccessMutex);
						t_pageTablesxProc *pageTablexProc = list_find(pageTablesListxProc,(void*) is_PIDPageTable);
						list_iterate(pageTablexProc->ptrPageTable, (void*) showMemoryRows);

						pthread_mutex_unlock(&memoryAccessMutex);
					}

				}
				//Closing file
				fclose(dumpf);
				printf("A copy of this dump was saved in: '%s'.\n", dumpFile);
				log_info(UMCLog, "A copy of this dump was saved in: '%s'.", dumpFile);
			}else{
				printf("Sorry! There is not information to show you now =(\n");
				log_info(UMCLog, "COMMAND: '%s %s %s' - Sorry! There is not information to show you now =(", command, option, value);
			}

		}else if (strcmp(command,"flush") == 0 ){

			if (strcmp(option, "tlb") == 0){

				if(list_all_satisfy(TLBList, (void*)isThereEmptyEntry) == false){
					//Locking TLB access for reading
					pthread_mutex_lock(&memoryAccessMutex);
					list_clean_and_destroy_elements(TLBList, (void*) destroyElementTLB);
					resetTLBAllEntries();
					pthread_mutex_unlock(&memoryAccessMutex);

					printf("The '%s' flush was completed successfully\n", option);
					log_info(UMCLog, "The '%s' flush was completed successfully", option);

				}else{
					printf("Sorry! There is not information in TLB for flushing =(\n");
					log_info(UMCLog, "COMMAND: '%s %s' - Sorry! There is not information in TLB for flushing =(", command, option);
				}

			}else if (strcmp(option, "memory") == 0){

				if(list_size(pageTablesListxProc) != 0){
					//Locking memory access for reading
					pthread_mutex_lock(&memoryAccessMutex);
					list_iterate(pageTablesListxProc,(void*) iteratePageTablexProc);
					pthread_mutex_unlock(&memoryAccessMutex);

					printf("The '%s' flush was completed successfully\n", option);
					log_info(UMCLog, "The '%s' flush was completed successfully", option);

				}else{
					printf("Sorry! There is not information in MEMORY for flushing =(\n");
					log_info(UMCLog, "COMMAND: '%s %s' - Sorry! There is not information in MEMORY for flushing =(", command, option);
				}
			}



		}else{

			printf("\nCommand entered NOT recognized: '%s %s'\n", command,option);
			printf("Please take a look to the correct commands!\n");
		}

		consoleMessageUMC();

	}

	//free(command);
	//free(value);

}

int getEnum(char *string){
	int parameter = -1;

	strcmp(string,"PUERTO") == 0 ? parameter = PUERTO : -1 ;
	strcmp(string,"IP_SWAP") == 0 ? parameter = IP_SWAP : -1 ;
	strcmp(string,"PUERTO_SWAP") == 0 ? parameter = PUERTO_SWAP : -1 ;
	strcmp(string,"MARCOS") == 0 ? parameter = MARCOS : -1 ;
	strcmp(string,"MARCO_SIZE") == 0 ? parameter = MARCO_SIZE : -1 ;
	strcmp(string,"MARCO_X_PROC") == 0 ? parameter = MARCO_X_PROC : -1 ;
	strcmp(string,"ENTRADAS_TLB") == 0 ? parameter = ENTRADAS_TLB : -1 ;
	strcmp(string,"RETARDO") == 0 ? parameter = RETARDO : -1 ;

	return parameter;
}

void consoleMessageUMC(){
	printf("\n***********************************\n");
	printf("* UMC Console ready for your use! *");
	printf("\n***********************************\n\n");
	printf("COMMANDS USAGE:\n\n");

	printf("===> COMMAND:\tretardo [OPTIONS]\n");
	printf("== [OPTIONS]\n");
	printf("<numericValue>\t::Cantidad de milisegundos que el sistema debe esperar antes de responder una solicitud\n\n");

	printf("===> COMMAND:\tdump [OPTIONS]\n");
	printf("== [OPTIONS]\n");
	printf("estructuras\t:: Tablas de paginas de todos los procesos o de un proceso en particular\n");
	printf("contenido\t:: Datos almacenados en la memoria de todos los procesos o de un proceso en particular\n\n");

	printf("===> COMMAND:\tflush [OPTIONS]\n");
	printf("== [OPTIONS]\n");
	printf("tlb\t\t:: Limpia completamente el contenido de la TLB\n");
	printf("memory\t\t:: Marca todas las paginas del proceso como modificadas\n\n");
	printf("UMC console >> $ ");
}

void createTLB(){
	TLBList = list_create();
	resetTLBAllEntries();
}

void resetTLBAllEntries(){
	int i;
	for(i=0; i < configuration.TLB_entries; i++){
		t_memoryAdmin *defaultTLBElement = malloc(sizeof(t_memoryAdmin));
		defaultTLBElement->virtualAddress = malloc(sizeof(t_memoryLocation));
		defaultTLBElement->PID = -1; //DEFAULT PID value in TLB
		defaultTLBElement->virtualAddress->pag = -1;
		defaultTLBElement->frameNumber = i;
		defaultTLBElement->virtualAddress->pag = -1;//DEFAULT non-used page
		defaultTLBElement->virtualAddress->offset = 0;//DEFAULT offset
		defaultTLBElement->virtualAddress->size = configuration.frames_size;// DEFAULT size equal to frame size
		list_add(TLBList, (void*) defaultTLBElement);
	}
}

void createAdminStructs(){
	//Creating memory block
	memBlock = calloc(configuration.frames_max, configuration.frames_size);

	//Creating list for checking free frames available
	freeFramesList = list_create();
	int i;
	for(i=0; i < configuration.frames_max; i++){
		int *frameNro = malloc(sizeof(int));
		*frameNro = i;
		list_add(freeFramesList, (void*)frameNro);
	}

	log_info(UMCLog, "'%d' frames created and ready for use.", list_size(freeFramesList));

	// Checking if TLB is enable
	if (configuration.TLB_entries != 0){
		//TLB enable
		createTLB();
		TLBActivated = true;
		log_info(UMCLog, "TLB enable. Size '%d'", list_size(TLBList));
	}else{
		log_info(UMCLog, "TLB disable!.");
	}

	//Creating page table list for Main Memory administration
	pageTablesListxProc = list_create();
}

/******************* Primary functions *******************/

void initializeProgram(int PID, int totalPagesRequired, char *programCode){

	//Creating new table page by PID - NOT needed to load anything in TLB
	t_pageTablesxProc *newPageTable = malloc(sizeof(t_pageTablesxProc));
	newPageTable->PID = PID;
	newPageTable->assignedFrames = 0;
	newPageTable->ptrPageTable = list_create();

	//Add new process to pageTablesListxProc
	list_add(pageTablesListxProc, (void *) newPageTable);

	// inform new program to swap and check if it could write it.
	int bufferSize = 0;
	int payloadSize = 0;

	//overwrite page content in swap (swap out)
	t_MessageUMC_Swap *message = malloc(sizeof(t_MessageUMC_Swap));
	message->virtualAddress = malloc(sizeof(t_memoryLocation));
	message->operation = agregar_proceso;
	message->PID = PID;
	message->cantPages = totalPagesRequired;
	message->virtualAddress->pag = -1;//DEFAULT value when the operation doesn't need it
	message->virtualAddress->offset = -1;//DEFAULT value when the operation doesn't need it
	message->virtualAddress->size = -1;//DEFAULT value when the operation doesn't need it

	payloadSize = sizeof(message->operation) + sizeof(message->PID) + sizeof(message->virtualAddress->pag) + sizeof(message->virtualAddress->offset) + sizeof(message->virtualAddress->size) + sizeof(message->cantPages);
	bufferSize = sizeof(bufferSize) + sizeof(enum_processes) + payloadSize ;

	char *buffer = malloc(bufferSize);
	//Serialize messageRcv
	serializeUMC_Swap(message, buffer, payloadSize);

	//Send information to swap - message serialized with virtualAddress information
	sendMessage(&socketSwap, buffer, bufferSize);

	//After sending information have to be sent the program code
	//1) Send program size to swap
	int programCodeLen = strlen(programCode) + 1;
	sendMessage(&socketSwap, &programCodeLen, sizeof(programCodeLen));

	//2) Send program to swap
	//string_append(&programCode,"\0");//ALWAYS put \0 for finishing the string
	sendMessage(&socketSwap, programCode, programCodeLen);

	free(buffer);
	free(message);

}

void endProgram(int PID){

	if(TLBActivated){//TLB is enable
		pthread_mutex_lock(&memoryAccessMutex);
		//Reseting to default entries from active PID in TLB
		list_iterate(TLBList, (void*)resetTLBbyActivePID);
		pthread_mutex_unlock(&memoryAccessMutex);
	}

	//Deleting elements from main memory
	pthread_mutex_lock(&memoryAccessMutex);
	list_remove_and_destroy_by_condition(pageTablesListxProc, (void*) find_PIDEntry_PageTable, (void*) PageTable_Element_destroy);
	pthread_mutex_unlock(&memoryAccessMutex);

	// Notify to Swap for freeing space
	// inform new program to swap and check if it could write it.
	int bufferSize = 0;
	int payloadSize = 0;

	//overwrite page content in swap (swap out)
	t_MessageUMC_Swap *message = malloc(sizeof(t_MessageUMC_Swap));
	message->virtualAddress = malloc(sizeof(t_memoryLocation));
	message->operation = finalizar_proceso;
	message->PID = PID;
	message->cantPages = -1; //DEFAULT value when the operation doesn't need it
	message->virtualAddress->pag = -1;//DEFAULT value when the operation doesn't need it
	message->virtualAddress->offset = -1;//DEFAULT value when the operation doesn't need it
	message->virtualAddress->size = -1;//DEFAULT value when the operation doesn't need it

	payloadSize = sizeof(message->operation) + sizeof(message->PID) + sizeof(message->virtualAddress->pag) + sizeof(message->virtualAddress->offset) + sizeof(message->virtualAddress->size) + sizeof(message->cantPages);
	bufferSize = sizeof(bufferSize) + sizeof(enum_processes) + payloadSize;

	char *buffer = malloc(bufferSize);
	//Serialize messageRcv
	serializeUMC_Swap(message, buffer, payloadSize);

	//Send information to swap - message serialized with virtualAddress information
	sendMessage(&socketSwap, buffer, bufferSize);

}

void *requestBytesFromPage(t_memoryLocation *virtualAddress){
	void *memoryContent;
	t_memoryAdmin *memoryElement;

	memoryElement = getElementFrameNro(virtualAddress, READ);

	if (memoryElement != NULL){ //PAGE HIT and NO errors from getFrame
		//delay memory access
		waitForResponse();

		int memoryBlockOffset = (memoryElement->frameNumber * configuration.frames_size) + virtualAddress->offset;

		memoryContent = malloc(virtualAddress->size);

		pthread_mutex_lock(&memoryAccessMutex);//Checking mutex before reading memory for copying request into return variable
		memcpy(memoryContent, memBlock + memoryBlockOffset , virtualAddress->size);
		pthread_mutex_unlock(&memoryAccessMutex);
	}else{
		//The main memory hasn't any free frames - inform this to upstream process
		memoryContent = NULL;
	}

	return memoryContent;
}

int writeBytesToPage(t_memoryLocation *virtualAddress, void *buffer){
	int exitCode = EXIT_SUCCESS;
	t_memoryAdmin *memoryElement;

	memoryElement = getElementFrameNro(virtualAddress, WRITE);

	if (memoryElement != NULL){ //PAGE HIT and NO errors from getFrame
		//delay memory access
		waitForResponse();

		log_info(UMCLog, "PID '%d': Writing page '#%d' in Frame '%d' - Content '%d'", activePID, virtualAddress->pag, memoryElement->frameNumber, (int) buffer);

		int memoryBlockOffset = (memoryElement->frameNumber * configuration.frames_size) + virtualAddress->offset;

		pthread_mutex_lock(&memoryAccessMutex);//Locking mutex for writing memory
		memcpy(memBlock + memoryBlockOffset, buffer , virtualAddress->size); //here is overwriting the memory content - doesn't matter if it was wrote in updateMemoryStructure()
		pthread_mutex_unlock(&memoryAccessMutex);//unlocking mutex for writing memory

		//Marking page as modified after writing
		log_info(UMCLog, "PID '%d': Page '#%d' marked as MODFIED after writing", activePID, virtualAddress->pag);
		memoryElement->dirtyBit = PAGE_MODIFIED;

	}else{
		//The main memory hasn't any free frames
		//inform this to upstream process
		exitCode = EXIT_FAILURE;
	}

	return exitCode;
}

/******************* Auxiliary functions *******************/

void iteratePageTablexProc(t_pageTablesxProc *pageTablexProc){
	list_iterate(pageTablexProc->ptrPageTable, (void*) markElementModified);
}

//** Mark memory Element as MODIFIED**//
void markElementModified(t_memoryAdmin *pageTableElement){
	pageTableElement->dirtyBit = PAGE_MODIFIED;
}

//** Mark memory Element as NOTPRESENT**//
void markElementNOTPResent(t_memoryAdmin *pageTableElement){
	pageTableElement->presentBit = PAGE_NOTPRESENT;
}

//** Dumper Page Table Element **//
void dumpPageTablexProc(t_pageTablesxProc *pageTablexProc){
	printf("\nInformacion PID: '%d':\n", pageTablexProc->PID);
	fprintf(dumpf,"\nInformacion PID: '%d':\n", pageTablexProc->PID);

	if (pageTablexProc->ptrPageTable->elements_count > 0){
		list_iterate(pageTablexProc->ptrPageTable, (void*) showPageTableRows);
	}else{
		printf("No elements present in Memory\n");
		fprintf(dumpf,"No elements present in Memory\n");
	}
}

//** Show Page Table Element information **//
void showPageTableRows(t_memoryAdmin *pageTableElement){
	char *status = string_new();
	char *presence = string_new();

	if(pageTableElement->dirtyBit == PAGE_MODIFIED){
		string_append(&status,"MODIFIED");
	}else{
		string_append(&status,"NOT_MODIFIED");
	}

	if(pageTableElement->presentBit == PAGE_PRESENT){
		string_append(&presence,"PAGE_PRESENT");
	}else{
		string_append(&presence,"PAGE_NOT_PRESENT");
	}

	printf("\tEn Frame '%d'\t--> Pagina '%d'\t--> status: '%s' and '%s'.\n", pageTableElement->frameNumber,pageTableElement->virtualAddress->pag, presence, status);
	fprintf(dumpf,"\tEn Frame '%d'\t--> Pagina '%d'\t--> status: '%s' and '%s'.\n", pageTableElement->frameNumber,pageTableElement->virtualAddress->pag, presence, status);

	free(status);
	free(presence);

}

//** Dumper Memory content **//
void dumpMemoryxProc(t_pageTablesxProc *pageTablexProc){
	printf("Contenido de memoria de PID: '%d'.\n", pageTablexProc->PID);
	fprintf(dumpf,"Contenido de memoria de PID: '%d'.\n", pageTablexProc->PID);

	list_iterate(pageTablexProc->ptrPageTable, (void*) showMemoryRows);
}

//** Show Memory content **//
void showMemoryRows(t_memoryAdmin *pageTableElement){
	char *content = string_new();

	int memoryBlockOffset = (pageTableElement->frameNumber * configuration.frames_size) + pageTableElement->virtualAddress->offset;

	content = realloc(content, pageTableElement->virtualAddress->size);

	memcpy(content, memBlock + memoryBlockOffset, pageTableElement->virtualAddress->size);

	if (pageTableElement->virtualAddress->size > 4){
		memset(content + pageTableElement->virtualAddress->size,'\0',1 );
		printf("\nPagina:'%d'\nContenido: '%s'\n", pageTableElement->virtualAddress->pag, content);
		fprintf(dumpf,"\nPagina:'%d'\nContenido: '%s'\n", pageTableElement->virtualAddress->pag, content);
	}else{
		printf("\nPagina:'%d'\nContenido: '%d'\n", pageTableElement->virtualAddress->pag, (int)*content);
		fprintf(dumpf,"\nPagina:'%d'\nContenido: '%d'\n", pageTableElement->virtualAddress->pag, (int)*content);
	}

	free(content);
}

void deleteContentFromMemory(t_memoryAdmin *memoryElement){
	//check page status before deleting
	checkPageModification(memoryElement);

	//Deleting element from Main Memory
	int memOffset = memoryElement->frameNumber * configuration.frames_size + memoryElement->virtualAddress->offset;
	memset(memBlock + memOffset,'\0', memoryElement->virtualAddress->size);

	log_info(UMCLog, "PID '%d' - Content in page '#%d' DELETED from memory",memoryElement->PID, memoryElement->virtualAddress->pag);

	//adding frame again to free frames list
	int *frameNro = malloc(sizeof(int));
	*frameNro = memoryElement->frameNumber;
	list_add(freeFramesList, (void*)frameNro);

	log_info(UMCLog, "NEW free frame '#%d' available", memoryElement->frameNumber);

}

void checkPageModification(t_memoryAdmin *memoryElement){

	if (memoryElement->dirtyBit == PAGE_MODIFIED){
		int bufferSize = 0;
		int payloadSize = 0;
		char *content = string_new();

		//overwrite page content in swap (swap out)
		t_MessageUMC_Swap *message = malloc(sizeof(t_MessageUMC_Swap));
		message->virtualAddress = malloc(sizeof(t_memoryLocation));
		message->operation = escritura_pagina;
		message->PID = memoryElement->PID;
		message->cantPages = -1; //DEFAULT value when the operation doesn't need it
		message->virtualAddress->pag = memoryElement->virtualAddress->pag;
		message->virtualAddress->offset = memoryElement->virtualAddress->offset;
		message->virtualAddress->size = configuration.frames_size;//ALWAYS send the whole page to write into SWAP

		payloadSize = sizeof(message->operation) + sizeof(message->PID) + sizeof(message->virtualAddress->pag) + sizeof(message->virtualAddress->offset) + sizeof(message->virtualAddress->size) + sizeof(message->cantPages);
		bufferSize = sizeof(bufferSize) + sizeof(enum_processes) + payloadSize ;

		char *buffer = malloc(bufferSize);
		//Serialize messageRcv
		serializeUMC_Swap(message, buffer, payloadSize);

		//Send message serialized with virtualAddress information
		sendMessage(&socketSwap, buffer, bufferSize);

		//Send memory content to overwrite with the virtualAddress->size - On the other side is going to be waiting it with that size sent previously
		int memoryBlockOffset =  (memoryElement->frameNumber * message->virtualAddress->size) + memoryElement->virtualAddress->offset;
		content = realloc(content, message->virtualAddress->size);
		memcpy(content, memBlock + memoryBlockOffset, message->virtualAddress->size);//ALWAYS send the whole page to write into SWAP

		sendMessage(&socketSwap, content, message->virtualAddress->size);

		log_info(UMCLog, "PID '%d' - Content in page '#%d' SWAPPED OUT to SWAP",memoryElement->PID, memoryElement->virtualAddress->pag);

		free(message->virtualAddress);
		free(message);
		free(buffer);
		free(content);
	}
}

void *requestPageToSwap(t_memoryLocation *virtualAddress, int PID){
	void *memoryContent = NULL;
	int bufferSize = 0;
	int payloadSize = 0;

	// request page content to swap
	t_MessageUMC_Swap *message = malloc(sizeof(t_MessageUMC_Swap));
	message->virtualAddress = malloc(sizeof(t_memoryLocation));
	message->operation = lectura_pagina;
	message->PID = PID;
	message->cantPages = -1; //DEFAULT value when the operation doesnt need it
	message->virtualAddress->pag = virtualAddress->pag;
	message->virtualAddress->offset = virtualAddress->offset;
	message->virtualAddress->size = configuration.frames_size;//virtualAddress->size;

	payloadSize = sizeof(message->operation) + sizeof(message->PID) + sizeof(message->virtualAddress->pag) + sizeof(message->virtualAddress->offset) + sizeof(message->virtualAddress->size) + sizeof(message->cantPages);
	bufferSize = sizeof(bufferSize) + sizeof(enum_processes) + payloadSize ;

	//Serialize messageRcv
	char *buffer = malloc(bufferSize);
	serializeUMC_Swap(message, buffer, payloadSize);

	//Send message serialized with virtualAddress information
	sendMessage(&socketSwap, buffer, bufferSize);

	//Receive memory content from SWAP with the virtualAddress->size - On the other side is going to be sending it with that size requested previously
	char *messageRcv = malloc(configuration.frames_size);// corregido!
	int receivedBytes = receiveMessage(&socketSwap, messageRcv, configuration.frames_size);
	memoryContent = malloc(configuration.frames_size);
	memcpy(memoryContent, messageRcv, configuration.frames_size);

	//TODO Se puede agregar una validacion despues del receive para que no pinche despues de hacer el memcpy
	log_info(UMCLog, "PID '%d' - Content in page '#%d' SWAPPED IN to memory",PID, virtualAddress->pag);

	free(message->virtualAddress);
	free(message);
	free(buffer);
	free(messageRcv);

	return memoryContent;
}

//** TLB and PageTable Element Destroyer **//
void destroyElementTLB(t_memoryAdmin *elementTLB){
	//Erasing memory content
	deleteContentFromMemory(elementTLB);

	//Deleting all elements from structure
	free(elementTLB->virtualAddress);
	free(elementTLB);
}

//** Page Table Element Destroyer **//
void PageTable_Element_destroy(t_pageTablesxProc *self){
	list_destroy_and_destroy_elements(self->ptrPageTable, (void*) destroyElementTLB);
	free(self);
}

//** Find function by process**//
bool is_PIDPageTablePresent(t_pageTablesxProc* listElement){
	return (listElement->PID == activePID);
}

//** Find function by PID**//
bool find_PIDEntry_PageTable(t_pageTablesxProc* listElement){
	return (listElement->PID == activePID);
}

//** Find function by PID**//
bool find_PIDEntry_TLB(t_memoryAdmin *listElement){
	return(listElement->PID == activePID);
}

//** Find function by Page**//
bool isThereEmptyEntry(t_memoryAdmin* listElement){
	return (listElement->virtualAddress->pag == -1);
}

//** Find Page with presence bit disable**//
bool isPageNOTPresent(t_memoryAdmin* listElement){
	bool returnValue;

	if (listElement->presentBit == PAGE_NOTPRESENT){
		returnValue = true;
	}else{
		listElement->presentBit = PAGE_NOTPRESENT; //This is used for First seek in CLOCK ALGORITHM
		returnValue = false;
	}

	return returnValue;
}

//** Find Page with presence bit disable and not modified**//
bool isPageNOTPresentNOTModified(t_memoryAdmin* listElement){
	return ((listElement->presentBit == PAGE_NOTPRESENT) && (listElement->presentBit == PAGE_NOTMODIFIED));
}

//** Find Page with presence bit disable and  modified**//
bool isPageNOTPresentModified(t_memoryAdmin* listElement){
	bool exitCode;

	exitCode = ((listElement->presentBit == PAGE_NOTPRESENT) && (listElement->presentBit == PAGE_MODIFIED));

	if (exitCode == false){
		listElement->presentBit = PAGE_NOTPRESENT;
	}else{
		log_info(UMCLog, "PID: '%d' - CLOCK candidate found in SECOND CHANCE page '#%d' in Frame '#%d'", activePID, listElement->virtualAddress->pag, listElement->frameNumber);
	}

	return exitCode;
}


t_memoryAdmin *getLRUCandidate(){
	t_memoryAdmin *elementCandidate = NULL;

	//** Find function by PID and page**//
	bool is_PIDPagePresent(t_memoryAdmin* listElement){
		return ((listElement->PID == elementCandidate->PID) && (listElement->virtualAddress->pag == elementCandidate->virtualAddress->pag));
	}

	pthread_mutex_lock(&memoryAccessMutex);
	//Filter TLB by PID
	t_list *TLBfilteredByPID = list_filter(TLBList, (void*) find_PIDEntry_TLB);

	//If elements by PID were found
	if (TLBfilteredByPID->elements_count == 0){
		//Filter by empty entries
		TLBfilteredByPID = list_filter(TLBList,(void*) isThereEmptyEntry);
	}

	//If any element was found in previous filters
	if (TLBfilteredByPID->elements_count > 0){
		//Get memory element from last index position from active PID
		elementCandidate = (t_memoryAdmin*) list_remove(TLBfilteredByPID, TLBfilteredByPID->elements_count - 1);

		//Remove element found from TLB
		elementCandidate = (t_memoryAdmin*) list_remove_by_condition(TLBList, (void*) is_PIDPagePresent);

	}else{
		//Get memory element from last index position in TLB (NO MATTER WHICH PID IS) if active PID has no entries in TLB
		elementCandidate = (t_memoryAdmin*) list_remove(TLBList, list_size(TLBList) - 1);
	}

	//Destroy temporary list created
	list_destroy(TLBfilteredByPID);
	pthread_mutex_unlock(&memoryAccessMutex);

	return elementCandidate;
}

void updateTLBPositionsbyLRU(t_memoryAdmin *elementCandidate){
	pthread_mutex_lock(&memoryAccessMutex);
	//add element to first position for keeping Least Recently used access updated
	list_add_in_index(TLBList, 0, (void*)elementCandidate);
	pthread_mutex_unlock(&memoryAccessMutex);
}

void resetTLBbyActivePID(t_memoryAdmin *listElement){

	if(listElement->PID == activePID){
		//adding frame again to free frames list
		int *frameNro = malloc(sizeof(int));
		*frameNro = listElement->frameNumber;
		list_add(freeFramesList, (void*)frameNro);

		//Additionally to restore PID default value, restoring all values to default just in case..
		listElement->PID = -1;
		listElement->dirtyBit = PAGE_NOTMODIFIED;
		listElement->presentBit = PAGE_NOTPRESENT;
		listElement->frameNumber = -1;
		free(listElement->virtualAddress);
		listElement->virtualAddress = malloc(sizeof(listElement->virtualAddress));
		listElement->virtualAddress->pag = -1;//DEFAULT non-used page
		listElement->virtualAddress->offset = 0;//DEFAULT offset
		listElement->virtualAddress->size = configuration.frames_size;// DEFAULT size equal to frame size
	}
}

t_memoryAdmin *getElementFrameNro(t_memoryLocation *virtualAddress, enum_memoryOperations operation){
	enum_memoryStructure memoryStructure;
	t_memoryAdmin *memoryElement = NULL;

	if(TLBActivated){//TLB is enable
		memoryStructure = TLB;
		memoryElement = searchFramebyPage(memoryStructure, operation, virtualAddress);
	}

	if (memoryElement == NULL){//TLB is disable or PAGE FAULT in TLB

		memoryStructure = MAIN_MEMORY;
		memoryElement = searchFramebyPage(memoryStructure, operation, virtualAddress);

		//Due to TLB miss has to be done a delay for memory access because here it was needed to read page table in MAIN MEMORY (despite it's present or not in that table)
		waitForResponse();

		t_pageTablesxProc *pageTablexProc = NULL;

		//Look for table page by active Process - (at least the list is going to have an empty entry due to its creation)
		pthread_mutex_lock(&memoryAccessMutex);
		pageTablexProc = list_find(pageTablesListxProc,(void*) is_PIDPageTablePresent);
		pthread_mutex_unlock(&memoryAccessMutex);

		t_memoryAdmin *newMemoryElement = updateMemoryStructure(pageTablexProc, virtualAddress, memoryElement);

		//Assign the memory value returned by the function into the return value
		memoryElement = newMemoryElement;

	}else{//TLB HIT
		//** Find function by PID and page**//
		bool is_PIDPagePresent(t_memoryAdmin* listElement){
			return ((listElement->PID == memoryElement->PID) && (listElement->virtualAddress->pag == memoryElement->virtualAddress->pag));
		}

		//Remove element found from TLB
		memoryElement = (t_memoryAdmin*) list_remove_by_condition(TLBList, (void*) is_PIDPagePresent);

		//updated TLB by LRU algorithm
		updateTLBPositionsbyLRU(memoryElement);

	}

	return memoryElement;
}

t_memoryAdmin *searchFramebyPage(enum_memoryStructure deviceLocation, enum_memoryOperations operation, t_memoryLocation *virtualAddress){
	t_pageTablesxProc *pageTablexProc = NULL;
	t_list *pageTableList = NULL;//lista con registros del tipo t_memoryAdmin
	t_memoryAdmin* pageNeeded = NULL;

	//** Find function by page**//
	bool is_PagePresent(t_memoryAdmin* listElement){
		return (listElement->virtualAddress->pag == virtualAddress->pag);
	}

	log_info(UMCLog, "PID '%d': Looking for page '#%d' in %s", activePID, virtualAddress->pag, getMemoryString(deviceLocation));

	switch(deviceLocation){
		case(TLB):{
			//Locking TLB access for reading
			pthread_mutex_lock(&memoryAccessMutex);
			pageNeeded = list_find(TLBList,(void*) is_PagePresent);
			pthread_mutex_unlock(&memoryAccessMutex);
			break;
		}
		case(MAIN_MEMORY):{
			//Look for table page by active Process
			pthread_mutex_lock(&memoryAccessMutex);
			pageTablexProc = list_find(pageTablesListxProc,(void*) is_PIDPageTablePresent);
			pthread_mutex_unlock(&memoryAccessMutex);

			//By default in program initialization ptrPageTable is going to be NULL
			pageTableList = pageTablexProc->ptrPageTable;

			//Locking memory access for reading
			pthread_mutex_lock(&memoryAccessMutex);
			pageNeeded = list_find(pageTableList,(void*) is_PagePresent);
			pthread_mutex_unlock(&memoryAccessMutex);
			break;
		}
		default:{
			log_error(UMCLog, "Error Device Location not recognized for searching: '%d'",deviceLocation);
		}
	}

	if (pageNeeded != NULL){
		//Page found
		log_info(UMCLog, "PID '%d': PAGE FOUND in %s - Page '#%d'", activePID, getMemoryString(deviceLocation), virtualAddress->pag);
		switch(operation){
			case(READ):{
				//After getting the frame needed for reading, mark memory element as present (overwrite it no matter if it was marked as present before)
				pageNeeded->presentBit = PAGE_PRESENT;
				log_info(UMCLog, "PID '%d': Page '#%d' marked as PRESENT in %s", activePID, virtualAddress->pag, getMemoryString(deviceLocation));
				break;
			}
			case (WRITE):{
				//After getting the frame needed for writing, mark memory element as modified and as present as well
				pageNeeded->presentBit = PAGE_PRESENT;
				pageNeeded->dirtyBit = PAGE_MODIFIED;
				log_info(UMCLog, "PID '%d': Page '#%d' marked as MODFIED in %s", activePID, virtualAddress->pag, getMemoryString(deviceLocation));
				break;
			}
		}
	}

	return pageNeeded;
}

t_memoryAdmin *updateMemoryStructure(t_pageTablesxProc *pageTablexProc, t_memoryLocation *virtualAddress, t_memoryAdmin *memoryElement){
	/*
	 * 1)busco frame libre(ver algoritmo de ubicacion - puede ser cualquiera es irrelevante para paginacion pura)
	 * 2)agrego info en TLB (si corresponde)
	 * 3)agrego info en estructura de Main Memory
	 * 4)pageTablexProc->assignedFrames++;
	 */

	t_memoryAdmin *newMemoryElement = NULL;

	if (memoryElement != NULL){//PAGE HIT in Main Memory

		log_info(UMCLog, "PID '%d': PAGE HIT in Main Memory - Page #%d", pageTablexProc->PID, virtualAddress->pag);

		if(TLBActivated){//TLB is enable
			log_info(UMCLog, "PID '%d' - After page hit in Main memory update TLB for page '#%d'", activePID, memoryElement->virtualAddress->pag);

			//Update TLB --> memory element is not present in TLB at this point
			executeLRUAlgorithm(memoryElement, virtualAddress);//---> This will check previous PID entries, empty TLB elements or taking last TLB element used in last case
		}

		newMemoryElement = memoryElement;

	}else{//PAGE Fault in Main Memory

		log_info(UMCLog, "PID '%d': PAGE FAULT in Main Memory - Page #%d", pageTablexProc->PID, virtualAddress->pag);

		if (list_size(freeFramesList) > 0){

			//The main memory still has free frames available
			log_info(UMCLog, "The MAIN MEMORY still has free frames to assign to process '%d'", activePID);

			//Request memory content to Swap
			void *memoryContent = requestPageToSwap(virtualAddress, pageTablexProc->PID);

			//request memory for new element
			newMemoryElement = malloc(sizeof(t_memoryAdmin));
			newMemoryElement->virtualAddress = malloc(sizeof(t_memoryLocation));
			newMemoryElement->PID = pageTablexProc->PID;
			newMemoryElement->presentBit = PAGE_PRESENT;
			newMemoryElement->dirtyBit = PAGE_NOTMODIFIED;
			newMemoryElement->virtualAddress->pag = virtualAddress->pag;
			newMemoryElement->virtualAddress->offset = virtualAddress->offset;
			newMemoryElement->virtualAddress->size = virtualAddress->size;

			//check if process has not reached the max of frames by process
			if (pageTablexProc->assignedFrames < configuration.frames_max_proc){

				//The active process has free frames available
				log_info(UMCLog, "The process '%d' still has free frames to assigned", activePID);

				//By DEFAULT always take the first free frame
				pthread_mutex_lock(&memoryAccessMutex);
				newMemoryElement->frameNumber = *(int*) list_remove(freeFramesList,0);
				pthread_mutex_unlock(&memoryAccessMutex);

				//Adding new Page table entry by process
				pthread_mutex_lock(&memoryAccessMutex);
				list_add(pageTablexProc->ptrPageTable,(void *)newMemoryElement);
				pthread_mutex_unlock(&memoryAccessMutex);

				log_info(UMCLog, "PID '%d' - Content in page '#%d' assigned to Frame '#%d'", activePID,newMemoryElement->virtualAddress->pag, newMemoryElement->frameNumber);

				//Update TLB
				if(TLBActivated){//TLB is enable
					//This will check previous PID entries, empty TLB elements or taking last TLB element used in last case
					executeLRUAlgorithm(newMemoryElement, newMemoryElement->virtualAddress); //---> newMemoryElement->virtualAddress ALREADY HAS THE VIRTUAL ADDRESS updated above
				}

				int memoryBlockOffset = (newMemoryElement->frameNumber * configuration.frames_size) + virtualAddress->offset;

				pthread_mutex_lock(&memoryAccessMutex);//Locking mutex for writing memory
				memcpy(memBlock + memoryBlockOffset, memoryContent , configuration.frames_size);
				pthread_mutex_unlock(&memoryAccessMutex);//unlocking mutex for writing memory

				//increasing frames assigned to active process ONLY
				pageTablexProc->assignedFrames++;

			}else{
				//The active process hasn't free frames available, replacement algorithm has to be executed
				log_info(UMCLog, "The process '%d' hasn't more free frames available", activePID);

				//execute main memory algorithm
				executeMainMemoryAlgorithm(pageTablexProc, newMemoryElement, memoryContent);

			}
		}else{
			//The main memory still hasn't any free frames
			log_warning(UMCLog, "The main memory is FULL!");
			//inform this to upstream process
			newMemoryElement = NULL;
			//exit function immediately
			return newMemoryElement;
		}

	}

	return newMemoryElement;
}

void executeLRUAlgorithm(t_memoryAdmin *newElement, t_memoryLocation *virtualAddress){
	t_memoryAdmin *LRUCandidate = NULL;

	//Replacement algorithm LRU
	LRUCandidate = getLRUCandidate();
	log_info(UMCLog, "PID: '%d' - New page needed '%d' in Memory -> LRU candidate page '#%d'", newElement->PID, newElement->virtualAddress->pag, LRUCandidate->virtualAddress->pag);

	pthread_mutex_lock(&memoryAccessMutex);
	//check page status before replacing
	checkPageModification(LRUCandidate);
	pthread_mutex_unlock(&memoryAccessMutex);

	//Load new program structure into candidate TLB entry
	LRUCandidate->PID = newElement->PID;
	LRUCandidate->dirtyBit = newElement->dirtyBit;
	LRUCandidate->presentBit = newElement->presentBit;
	LRUCandidate->virtualAddress->pag = virtualAddress->pag;
	LRUCandidate->virtualAddress->offset = virtualAddress->offset;
	LRUCandidate->virtualAddress->size = virtualAddress->size;
	LRUCandidate->frameNumber = newElement->frameNumber;

	//updated TLB by LRU algorithm
	updateTLBPositionsbyLRU(LRUCandidate);

}

void executeMainMemoryAlgorithm(t_pageTablesxProc *pageTablexProc, t_memoryAdmin *newElement, void *memoryContent){
	t_memoryAdmin *clockCandidate = NULL;

	bool find_ClockCandidateToRemove(t_memoryAdmin* listElement){
		return (listElement->virtualAddress->pag == clockCandidate->virtualAddress->pag);
	}

	bool find_freeFrameToRemove (int freeFrame){
		return (freeFrame == clockCandidate->frameNumber);
	}

	pthread_mutex_lock(&memoryAccessMutex);

	if (string_equals_ignore_case(configuration.algorithm_replace,"CLOCK")){
		//*** Algorithm CLOCK ***//

		//First seek
		clockCandidate = list_find(pageTablexProc->ptrPageTable, (void*) isPageNOTPresent);

		if(clockCandidate == NULL){
			//Second seek for candidate after first pass - THIS ENSURES A CANDIDATE
			clockCandidate = list_find(pageTablexProc->ptrPageTable, (void*) isPageNOTPresent);
			log_info(UMCLog, "PID: '%d' - CLOCK candidate found in SECOND CHANCE page '#%d' in Frame '#%d'", pageTablexProc->PID, clockCandidate->virtualAddress->pag, clockCandidate->frameNumber);
		}else{
			log_info(UMCLog, "PID: '%d' - CLOCK candidate found in FIRST CHANCE page '#%d' in Frame '#%d'", pageTablexProc->PID, clockCandidate->virtualAddress->pag, clockCandidate->frameNumber);
		}

		log_info(UMCLog, "PID: '%d' - New page needed '%d' -> CLOCK candidate page '#%d' in Frame '#%d'", pageTablexProc->PID, newElement->virtualAddress->pag, clockCandidate->virtualAddress->pag, clockCandidate->frameNumber);

		//Candidate found
		newElement->frameNumber = clockCandidate->frameNumber;
		newElement->presentBit = PAGE_PRESENT;

	}else{
		//*** Algorithm ENHANCED CLOCK***//

		while (clockCandidate == NULL){
			//First seek
			clockCandidate = list_find(pageTablexProc->ptrPageTable, (void*) isPageNOTPresentNOTModified);

			if(clockCandidate == NULL){
				//Second seek for candidate after first pass
				clockCandidate = list_find(pageTablexProc->ptrPageTable, (void*) isPageNOTPresentModified);
			}else{
				log_info(UMCLog, "PID: '%d' - ENCAHNCED CLOCK candidate found in FIRST or THIRD CHANCE page '#%d' in Frame '#%d'", pageTablexProc->PID, clockCandidate->virtualAddress->pag, clockCandidate->frameNumber);
			}
		}

		log_info(UMCLog, "PID: '%d' - New page needed '%d' -> ENCAHNCED CLOCK candidate page '#%d' in Frame '#%d'", pageTablexProc->PID, newElement->virtualAddress->pag, clockCandidate->virtualAddress->pag, clockCandidate->frameNumber);

		//Candidate found
		newElement->frameNumber = clockCandidate->frameNumber;
		newElement->presentBit = PAGE_PRESENT;

	}

	pthread_mutex_unlock(&memoryAccessMutex);

	//Update TLB before replacing memory content (out of the mutex because the function locks the same mutex inside)
	if(TLBActivated){//TLB is enable
		//Execute LRU algorithm for modifying TLB after updating table page frame
		executeLRUAlgorithm(newElement, newElement->virtualAddress); // ---> newElement->virtualAddress ALREADY HAS THE VIRTUAL ADDRESS updated from the calling function
	}

	pthread_mutex_lock(&memoryAccessMutex);

	//Removing the frame added to free frames list when the candidate was destroyed
	list_remove_and_destroy_by_condition(freeFramesList, (void*) find_freeFrameToRemove, (void*) free);

	//Removing candidate from page table list
	list_remove_and_destroy_by_condition(pageTablexProc->ptrPageTable, (void*) find_ClockCandidateToRemove, (void*) destroyElementTLB); //---> detroyElement checks pages modification before deleting

	//Add new element to the end of the list
	list_add_in_index(pageTablexProc->ptrPageTable, pageTablexProc->ptrPageTable->elements_count - 1, newElement); //--> This ensures the CLOCK pointer always in head position for next seek

	log_info(UMCLog, "PID: '%d' - New page '#%d' in Frame '#%d' added to Table page", pageTablexProc->PID, newElement->virtualAddress->pag, newElement->frameNumber);

	int memoryBlockOffset = (newElement->frameNumber * configuration.frames_size) + newElement->virtualAddress->offset;

	//writing memory
	memcpy(memBlock + memoryBlockOffset, memoryContent , newElement->virtualAddress->size);

	pthread_mutex_unlock(&memoryAccessMutex);

}

void waitForResponse(){
	pthread_mutex_lock(&delayMutex);
	sleep(configuration.delay);
	pthread_mutex_unlock(&delayMutex);
}

void changeActiveProcess(int PID){

	//Ante un cambio de proceso, se realizará una limpieza (flush) en las entradas que correspondan.
	if(TLBActivated){//TLB is enable
		pthread_mutex_lock(&memoryAccessMutex);
		//Reseting to default entries from previous active PID
		list_iterate(TLBList, (void*)resetTLBbyActivePID);
		pthread_mutex_unlock(&memoryAccessMutex);
	}

	//after flushing entries from old process change active process to the one needed
	log_info(UMCLog, "Changing to active PID: '#%d'", PID);
	activePID = PID;
}

char *getMemoryString (enum_memoryStructure memoryStructure){

	char *memoryString = string_new();
	switch (memoryStructure){
		case TLB:{
			string_append(&memoryString, "TLB");
			break;
		}
		case MAIN_MEMORY:{
			string_append(&memoryString, "MAIN_MEMORY");
			break;
		}
		default:{
			log_info(UMCLog, "Memory Structure not recognized '%d'",(int) memoryStructure);
			memoryString = NULL;
		}
	}
	return memoryString;
}
