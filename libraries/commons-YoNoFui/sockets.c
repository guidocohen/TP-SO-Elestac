#include "sockets.h"

int openServerConnection(int newSocketServerPort, int *socketServer){
	int exitcode = EXIT_SUCCESS; //Normal completition

	struct sockaddr_in newSocketInfo;

	assert(("ERROR - Server port is the DEFAULT (=0)", newSocketServerPort != 0)); // FAILS if the Server port is equal to default value (0)

	newSocketInfo.sin_family = AF_INET; //AF_INET = IPv4
	newSocketInfo.sin_addr.s_addr = INADDR_ANY; //
	newSocketInfo.sin_port = htons(newSocketServerPort);

	*socketServer = socket(AF_INET, SOCK_STREAM, 0);

	int socketActivated = 1;

	//This line is to notify to the SO that the socket created is going to be reused
	setsockopt(*socketServer, SOL_SOCKET, SO_REUSEADDR, &socketActivated, sizeof(socketActivated));

	if (bind(*socketServer, (void*) &newSocketInfo, sizeof(newSocketInfo)) != 0){
		perror("Failed bind in openServerConnection()\n"); //TODO => Agregar logs con librerias
		printf("Please check whether another process is using port: %d \n",newSocketServerPort);

		//Free socket created
		close(*socketServer);

		exitcode = EXIT_FAILURE;
		return exitcode;
	}

	listen(*socketServer, SOMAXCONN); //SOMAXCONN = Constant with the maximum connections allowed by the SO

	return exitcode;
}

int openClientConnection(char *IPServer, int PortServer, int *socketClient){
	int exitcode = EXIT_SUCCESS; //Normal completition

	struct sockaddr_in serverSocketInfo;

	serverSocketInfo.sin_family = AF_INET; //AF_INET = IPv4
	serverSocketInfo.sin_addr.s_addr = inet_addr(IPServer); //
	serverSocketInfo.sin_port = htons(PortServer);

	*socketClient = socket(AF_INET, SOCK_STREAM, 0);

	if (connect(*socketClient, (void*) &serverSocketInfo, sizeof(serverSocketInfo)) != 0){
		perror("Failed connect to server in OpenClientConnection()"); //TODO => Agregar logs con librerias
		printf("Please check whether the server '%s' is up or the correct port is: %d \n",IPServer, PortServer);

		//Free socket created
		close(*socketClient);
		exitcode = EXIT_FAILURE;
		return exitcode;
	}

	return exitcode;
}

int acceptClientConnection(int *socketServer, int *socketClient){
	int exitcode = EXIT_SUCCESS; //Normal completition
	struct sockaddr_in clientConnection;
	unsigned int addressSize = sizeof(clientConnection); //The addressSize has to be initialized with the size of sockaddr_in before passing it to accept function

	*socketClient = accept(*socketServer, (void*) &clientConnection, &addressSize);

	if (*socketClient != -1){
		//printf("The was received a connection in socket: %d.\n", *socketClient); --> adding logs in programs
		exitcode = EXIT_SUCCESS;
	}else{
		perror("Failed to get a new connection"); //TODO => Agregar logs con librerias
		exitcode = EXIT_FAILURE;
	}

	return exitcode;
}

int sendClientHandShake(int *socketClient, enum_processes process){
	int exitcode = EXIT_SUCCESS; //Normal completition
	int bufferSize = 0;
	int messageLen = 0;
	int payloadSize = 0;
	char *processString;

	t_MessageGenericHandshake *messageACK = malloc(sizeof(t_MessageGenericHandshake));
	messageACK->process = process;

	processString = getProcessString(process);

	messageACK->message = string_new();
	string_append_with_format(&messageACK->message,"The process '%s' is trying to connect you!\0",processString);
	messageLen = strlen(messageACK->message);

	payloadSize = sizeof(messageACK->process) + sizeof(messageLen) + messageLen + 1; // process + length message + message + 1 (+1 because of '\0')
	bufferSize = sizeof(bufferSize) + payloadSize ;//+1 because of '\0'

	char *buffer = malloc(bufferSize);
	serializeHandShake(messageACK, buffer, payloadSize);//has to be sent the PAYLOAD size!!

	exitcode = send(*socketClient, (void*) buffer, bufferSize,0) == -1 ? EXIT_FAILURE : EXIT_SUCCESS ;

	free(buffer);
	free(messageACK->message);
	free(messageACK);

	return exitcode;
}

int sendClientAcceptation(int *socketClient){
	int exitcode = EXIT_SUCCESS; //Normal completition
	int bufferSize = 0;
	int messageLen = 0;
	int payloadSize = 0;

	t_MessageGenericHandshake *messageACK = malloc(sizeof(t_MessageGenericHandshake));
	messageACK->process = ACCEPTED;
	messageACK->message = string_new();
	string_append(&messageACK->message,"The server has accepted your connection!\0");//ALWAYS put \0 for finishing the string
	messageLen = strlen(messageACK->message);

	payloadSize = sizeof(messageACK->process) + sizeof(messageLen) + messageLen + 1; // process + length message + message + 1 (+1 because of '\0')
	bufferSize = sizeof(bufferSize) + payloadSize ;//+1 because of '\0'

	char *buffer = malloc(bufferSize);
	serializeHandShake(messageACK, buffer, payloadSize);//has to be sent the PAYLOAD size!!

	exitcode = send(*socketClient, (void*) buffer, bufferSize,0) == -1 ? EXIT_FAILURE : EXIT_SUCCESS ;

	free(buffer);
	free(messageACK->message);
	free(messageACK);
	return exitcode;
}

int sendMessage (int *socketClient, void *buffer, int bufferSize){
	int exitcode = EXIT_SUCCESS; //Normal completition

	exitcode = send(*socketClient, buffer, bufferSize,0) == -1 ? EXIT_FAILURE : EXIT_SUCCESS ;

	return exitcode;
}

int receiveMessage(int *socketClient, void *messageRcv, int bufferSize){

	int receivedBytes = recv(*socketClient, messageRcv, bufferSize, 0);

	return receivedBytes;
}

void serializeHandShake(t_MessageGenericHandshake *value, char *buffer, int valueSize){
    int offset = 0;

    //0)valueSize
	memcpy(buffer, &valueSize, sizeof(valueSize));
	offset += sizeof(valueSize);

    //1)process
    memcpy(buffer + offset, &value->process, sizeof(value->process));
    offset += sizeof(value->process);

    //2)message length
    int messageLen = strlen(value->message) + 1;//+1 because of '\0'
	memcpy(buffer + offset, &messageLen, sizeof(messageLen));
	offset += sizeof(messageLen);

    //3)message
    memcpy(buffer + offset, value->message, messageLen);

}

void deserializeHandShake(t_MessageGenericHandshake *value, char *bufferReceived){
    int offset = 0;
    int messageLen = 0;

    //1)process
    memcpy(&value->process, bufferReceived, sizeof(value->process));
    offset += sizeof(value->process);

    //2)message length
	memcpy(&messageLen, bufferReceived + offset, sizeof(messageLen));
	offset += sizeof(messageLen);

    //3)message
	value->message = malloc(messageLen);
    memcpy(value->message, bufferReceived + offset, messageLen);

}

char* serializarRegistroStack(t_registroStack* registroASerializar, char* registroSerializado, int *offset){

	registroSerializado = serializarStack(registroASerializar, registroSerializado, offset);

	registroSerializado = serializeListaArgs(registroASerializar->args, registroSerializado, offset);

	registroSerializado = serializarListasVars(registroASerializar->vars, registroSerializado, offset);

	return registroSerializado;
}

void deserializarRegistroStack(t_registroStack* registroRecibido, char* registroSerializado, int *offset){

	deserializarStack(registroRecibido, registroSerializado, offset);

	deserializeListasArgs(registroRecibido->args, registroSerializado, offset);

	deserializarListasVars(registroRecibido->vars, registroSerializado, offset);

}

int serializarListaStack(t_list* listaASerializar, char** listaSerializada){
	int offset = 0;
	t_registroStack* registroStack;

	*listaSerializada = malloc(sizeof(listaASerializar->elements_count));

	memcpy(*listaSerializada, &listaASerializar->elements_count, sizeof(listaASerializar->elements_count));
	offset += sizeof(listaASerializar->elements_count);

	int i;
	for(i = 0 ; i < listaASerializar->elements_count; i++){
		//get the element from the list by index
		registroStack = list_get(listaASerializar,i);

		//serialize the element to the buffer
		*listaSerializada = serializarRegistroStack(registroStack,*listaSerializada, &offset);
	}

	return offset;
}

void deserializarListaStack(t_list* listaARecibir, char* listaSerializada){
	int offset = 0;
	int cantidadDeElementos = 0;

	//Getting element count
	memcpy(&cantidadDeElementos, listaSerializada ,sizeof(listaARecibir->elements_count));
	offset += sizeof(listaARecibir->elements_count);

	int i;
	for(i=0; i< cantidadDeElementos; i++){
		t_registroStack* registroStack = malloc(sizeof(t_registroStack));
		registroStack->retVar = (t_memoryLocation*) malloc(sizeof(t_memoryLocation));
		registroStack->args = list_create();
		registroStack->vars = list_create();

		deserializarRegistroStack(registroStack, listaSerializada, &offset);
		list_add(listaARecibir, registroStack);

	}

}

char *serializarVars(t_vars* miRegistro, char* value, int *offset) {
	char *nuevoRegistroSerializado;

	//Request more memory for the new element to be serialized
	int tamanioRegistroNuevo = *offset + sizeof(miRegistro->identificador);
	nuevoRegistroSerializado = malloc(tamanioRegistroNuevo);
	memcpy(nuevoRegistroSerializado, value, tamanioRegistroNuevo);

	free(value);

	memcpy(nuevoRegistroSerializado + *offset, &miRegistro->identificador, sizeof(miRegistro->identificador));
	*offset += sizeof(miRegistro->identificador);

	nuevoRegistroSerializado = serializeMemoryLocation(miRegistro->direccionValorDeVariable, nuevoRegistroSerializado, offset);

	return nuevoRegistroSerializado;
}

void deserializarVars(t_vars* unaVariable, char* variablesRecibidas, int *offset) {

	memcpy(&unaVariable->identificador,variablesRecibidas + *offset, sizeof(unaVariable->identificador));
	*offset += sizeof(unaVariable->identificador);

	unaVariable->direccionValorDeVariable = (t_memoryLocation*) malloc(sizeof(t_memoryLocation));

	deserializeMemoryLocation(unaVariable->direccionValorDeVariable, variablesRecibidas, offset);

}

char *serializeMemoryLocation(t_memoryLocation* unaPosicion, char* value, int *offset) {

	char *nuevoRegistroSerializado;

	//Request more memory for the new element to be serialized
	int tamanioRegistroViejo = *offset + sizeof(unaPosicion->offset) + sizeof(unaPosicion->pag) + sizeof(unaPosicion->size);
	nuevoRegistroSerializado = malloc(tamanioRegistroViejo);
	memcpy(nuevoRegistroSerializado, value, tamanioRegistroViejo);

	free(value);

	memcpy(nuevoRegistroSerializado + *offset, &unaPosicion->offset, sizeof(unaPosicion->offset));
	*offset += sizeof(unaPosicion->offset);
	memcpy(nuevoRegistroSerializado + *offset, &unaPosicion->pag, sizeof(unaPosicion->pag));
	*offset += sizeof(unaPosicion->pag);
	memcpy(nuevoRegistroSerializado + *offset, &unaPosicion->size, sizeof(unaPosicion->size));
	*offset += sizeof(unaPosicion->size);

	return nuevoRegistroSerializado;
}

void deserializeMemoryLocation(t_memoryLocation* unaPosicion, char* posicionRecibida, int *offset) {

	memcpy(&unaPosicion->offset, posicionRecibida + *offset, sizeof(unaPosicion->offset));
	*offset += sizeof(unaPosicion->offset);
	memcpy(&unaPosicion->pag, posicionRecibida + *offset, sizeof(unaPosicion->pag));
	*offset += sizeof(unaPosicion->pag);
	memcpy(&unaPosicion->size, posicionRecibida + *offset, sizeof(unaPosicion->size));
	*offset += sizeof(unaPosicion->size);
}


char *serializarListasVars(t_list* listaASerializar, char* listaSerializada, int *offset) {
	t_vars* unaVariable;
	char *nuevoRegistroSerializado;

	//Request more memory for the new element to be serialized
	int tamanioRegistroNuevo = *offset + sizeof(listaASerializar->elements_count);
	nuevoRegistroSerializado = malloc(tamanioRegistroNuevo);
	memcpy(nuevoRegistroSerializado, listaSerializada, tamanioRegistroNuevo);

	free(listaSerializada);

	memcpy(nuevoRegistroSerializado + *offset, &listaASerializar->elements_count, sizeof(listaASerializar->elements_count));
	*offset += sizeof(listaASerializar->elements_count);

	int i;
	for (i = 0; i < listaASerializar->elements_count; i++) {

		//get the element from the list by index
		unaVariable = list_get(listaASerializar, i);

		//serialize the element to the buffer
		nuevoRegistroSerializado = serializarVars(unaVariable, nuevoRegistroSerializado, offset);
	}

	return nuevoRegistroSerializado;

}

void deserializarListasVars(t_list* listaVars,char* listaSerializada, int *offset){
	int cantidadDeElementos = 0;

	//Getting element count
	memcpy(&cantidadDeElementos, listaSerializada + *offset, sizeof(listaVars->elements_count));
	*offset += sizeof(listaVars->elements_count);

	int i;
	for(i=0; i < cantidadDeElementos; i++){
		t_vars* unaVariable = malloc(sizeof(t_vars));
		deserializarVars(unaVariable, listaSerializada, offset);
		list_add(listaVars, unaVariable);
	}
}

char* serializarStack(t_registroStack* registroStack, char* registroSerializado, int *offset) {
	char *nuevoRegistroSerializado;
	int retVarnullFlag;

	//Request more memory for the new element to be serialized
	int tamanioRegistroNuevo = *offset + sizeof(registroStack->pos) + sizeof(registroStack->retPos) + sizeof(retVarnullFlag);
	nuevoRegistroSerializado = malloc(tamanioRegistroNuevo);
	memcpy(nuevoRegistroSerializado, registroSerializado, tamanioRegistroNuevo);

	free(registroSerializado);

	memcpy(nuevoRegistroSerializado + *offset, &registroStack->pos, sizeof(registroStack->pos));
	*offset += sizeof(registroStack->pos);

	memcpy(nuevoRegistroSerializado + *offset, &registroStack->retPos, sizeof(registroStack->retPos));
	*offset += sizeof(registroStack->retPos);

	if (registroStack->retVar != NULL){
		retVarnullFlag = 0;// This means that the retVar has value to be serialized
		memcpy(nuevoRegistroSerializado + *offset, &retVarnullFlag, sizeof(retVarnullFlag));
		*offset += sizeof(retVarnullFlag);

		nuevoRegistroSerializado = serializeMemoryLocation(registroStack->retVar, nuevoRegistroSerializado, offset);
	}else{
		retVarnullFlag = -1;// This means that the retVar is NULL

		memcpy(nuevoRegistroSerializado + *offset, &retVarnullFlag, sizeof(retVarnullFlag));
		*offset += sizeof(retVarnullFlag);
	}


	return nuevoRegistroSerializado;
}

void deserializarStack(t_registroStack* estructuraARecibir, char* registroStack, int *offset) {

	memcpy(&estructuraARecibir->pos, registroStack + *offset, sizeof(estructuraARecibir->pos));
	*offset += sizeof(estructuraARecibir->pos);

	memcpy(&estructuraARecibir->retPos, registroStack + *offset, sizeof(estructuraARecibir->pos));
	*offset += sizeof(estructuraARecibir->retPos);

	int retVarnullFlag;
	memcpy(&retVarnullFlag, registroStack + *offset, sizeof(retVarnullFlag));
	*offset += sizeof(retVarnullFlag);

	if (retVarnullFlag == 0){// This means that the retVar has value to be deserialized
		deserializeMemoryLocation(estructuraARecibir->retVar, registroStack, offset);
	}else{
		estructuraARecibir->retVar->pag = -1;
		estructuraARecibir->retVar->offset = 0;
		estructuraARecibir->retVar->size = 0;
	}

}

char *serializeListaArgs(t_list* listaASerializar, char* listaSerializada, int *offset) {
	t_memoryLocation* unaPosicion;
	char *nuevoRegistroSerializado;

	//Request more memory for the new element to be serialized
	int tamanioRegistroNuevo = *offset + sizeof(listaASerializar->elements_count);
	nuevoRegistroSerializado = malloc(tamanioRegistroNuevo);
	memcpy(nuevoRegistroSerializado, listaSerializada, tamanioRegistroNuevo);

	free(listaSerializada);

	memcpy(nuevoRegistroSerializado + *offset, &listaASerializar->elements_count, sizeof(listaASerializar->elements_count));
	*offset += sizeof(listaASerializar->elements_count);

	int i;
	for (i = 0; i < listaASerializar->elements_count; i++) {
		//get the element from the list by index
		unaPosicion = list_get(listaASerializar,i);

		//serialize the element to the buffer
		nuevoRegistroSerializado = serializeMemoryLocation(unaPosicion, nuevoRegistroSerializado, offset);
	}

	return nuevoRegistroSerializado;
}

void deserializeListasArgs(t_list* listaArgs, char* listaSerializada, int *offset){

	//Getting element count
	int cantidadDeElementos = 0;
	memcpy(&cantidadDeElementos, listaSerializada + *offset, sizeof(listaArgs->elements_count));
	*offset += sizeof(listaArgs->elements_count);

	int i;
	for(i=0; i < cantidadDeElementos; i++){
		t_memoryLocation* posicionDeMemoria = malloc(sizeof(t_memoryLocation));
		deserializeMemoryLocation(posicionDeMemoria, listaSerializada, offset);
		list_add(listaArgs, posicionDeMemoria);
	}

}

void serializarIndiceDeCodigo(t_registroIndiceCodigo* registroEnviar, char* registroSerializado){
	int offset = 0;

	memcpy(registroSerializado + offset, &registroEnviar->inicioDeInstruccion,sizeof(registroEnviar->inicioDeInstruccion));
	offset += sizeof(registroEnviar->inicioDeInstruccion);

	memcpy(registroSerializado + offset, &registroEnviar->longitudInstruccionEnBytes,sizeof(registroEnviar->longitudInstruccionEnBytes));

}

void deserializarIndiceDeCodigo(t_registroIndiceCodigo* registroARecibir, char* registroSerializado){
	int offset = 0;

	memcpy(&registroARecibir->inicioDeInstruccion,registroSerializado + offset, sizeof(registroARecibir->inicioDeInstruccion));
	offset += sizeof(registroARecibir->inicioDeInstruccion);

	memcpy(&registroARecibir->longitudInstruccionEnBytes,registroSerializado + offset, sizeof(registroARecibir->longitudInstruccionEnBytes));

}

int serializarListaIndiceDeCodigo(t_list* listaASerializar, char** listaSerializada){
	int offset = 0 ;
	t_registroIndiceCodigo* registroObtenido;

	memcpy(*listaSerializada + offset, &listaASerializar->elements_count, sizeof(listaASerializar->elements_count));
	offset += sizeof(listaASerializar->elements_count);

	int i;
	for (i = 0; i < listaASerializar->elements_count; i++) {
		//Request more memory for the new element to be serialized
		*listaSerializada = realloc(*listaSerializada, offset + sizeof(t_registroIndiceCodigo));

		//get the element from the list by index
		registroObtenido = list_get(listaASerializar,i);

		//serialize the element to the buffer
		serializarIndiceDeCodigo(registroObtenido, *listaSerializada + offset);

		offset += sizeof(t_registroIndiceCodigo);
	}

	return offset;
}

void deserializarListaIndiceDeCodigo(t_list* listaARecibir, char* listaSerializada){
	int offset = 0;
	int cantidadDeElementos = 0;

	//Getting element count
	memcpy(&cantidadDeElementos, listaSerializada ,sizeof(listaARecibir->elements_count));
	offset += sizeof(listaARecibir->elements_count);

	int i;
	for(i=0; i<cantidadDeElementos; i++){
		t_registroIndiceCodigo* registroIndiceDeCodigo = malloc(sizeof(t_registroIndiceCodigo));
		deserializarIndiceDeCodigo(registroIndiceDeCodigo, listaSerializada + offset);
		list_add(listaARecibir, registroIndiceDeCodigo);
		offset += sizeof(t_registroIndiceCodigo);
	}

}

/********************** PROTOCOL USAGE *****************************/


void serializeCPU_UMC(t_MessageCPU_UMC *value, char *buffer, int valueSize){
	int offset = 0;
	enum_processes process = CPU;

	//0)valueSize
	memcpy(buffer, &valueSize, sizeof(valueSize));
	offset += sizeof(valueSize);

	//1)From process
	memcpy(buffer + offset, &process, sizeof(process));
	offset += sizeof(process);

	//2)operation
	memcpy(buffer + offset, &value->operation, sizeof(value->operation));
	offset += sizeof(value->operation);

	//3)processID
	memcpy(buffer + offset, &value->PID, sizeof(value->PID));
	offset += sizeof(value->PID);

	//4)pageNro
	memcpy(buffer + offset, &value->virtualAddress->pag, sizeof(value->virtualAddress->pag));
	offset += sizeof(value->virtualAddress->pag);

	//5)offset
	memcpy(buffer + offset, &value->virtualAddress->offset, sizeof(value->virtualAddress->offset));
	offset += sizeof(value->virtualAddress->offset);

	//6)size
	memcpy(buffer + offset, &value->virtualAddress->size, sizeof(value->virtualAddress->size));

}

void deserializeUMC_CPU(t_MessageCPU_UMC *value, char *bufferReceived){
	int offset = 0;

	//2)operation
	memcpy(&value->operation, bufferReceived, sizeof(value->operation));
	offset += sizeof(value->operation);

	//3)processID
	memcpy(&value->PID, bufferReceived + offset, sizeof(value->PID));
	offset += sizeof(value->PID);

	//4)pageNro
	memcpy(&value->virtualAddress->pag, bufferReceived + offset, sizeof(value->virtualAddress->pag));
	offset += sizeof(value->virtualAddress->pag);

	//5)offset
	memcpy(&value->virtualAddress->offset, bufferReceived + offset, sizeof(value->virtualAddress->offset));
	offset += sizeof(value->virtualAddress->offset);

	//6)size
	memcpy(&value->virtualAddress->size, bufferReceived + offset, sizeof(value->virtualAddress->size));
}


void serializeUMC_Swap(t_MessageUMC_Swap *value, char *buffer, int valueSize){
	int offset = 0;
	enum_processes process = UMC;

	//0)valueSize
	memcpy(buffer, &valueSize, sizeof(valueSize));
	offset += sizeof(valueSize);

	//1)From process
	memcpy(buffer + offset, &process, sizeof(process));
	offset += sizeof(process);

	//2)operation
	memcpy(buffer + offset, &value->operation, sizeof(value->operation));
	offset += sizeof(value->operation);

	//3)PID
	memcpy(buffer + offset, &value->PID, sizeof(value->PID));
	offset += sizeof(value->PID);

	//4)pageNro
	memcpy(buffer + offset, &value->virtualAddress->pag, sizeof(value->virtualAddress->pag));
	offset += sizeof(value->virtualAddress->pag);

	//5)offset
	memcpy(buffer + offset, &value->virtualAddress->offset, sizeof(value->virtualAddress->offset));
	offset += sizeof(value->virtualAddress->offset);

	//6)size
	memcpy(buffer + offset, &value->virtualAddress->size, sizeof(value->virtualAddress->size));
	offset += sizeof(value->virtualAddress->size);

	//7)page quantity
	memcpy(buffer + offset, &value->cantPages, sizeof(value->cantPages));

}

void deserializeSwap_UMC(t_MessageUMC_Swap *value, char *bufferReceived){
	int offset = 0;

	//2)operation
	memcpy(&value->operation, bufferReceived, sizeof(value->operation));
	offset += sizeof(value->operation);

	//3)PID
	memcpy(&value->PID, bufferReceived + offset, sizeof(value->PID));
	offset += sizeof(value->PID);

	//4)pageNro
	memcpy(&value->virtualAddress->pag, bufferReceived + offset, sizeof(value->virtualAddress->pag));
	offset += sizeof(value->virtualAddress->pag);

	//5)offset
	memcpy(&value->virtualAddress->offset, bufferReceived + offset, sizeof(value->virtualAddress->offset));
	offset += sizeof(value->virtualAddress->offset);

	//6)size
	memcpy(&value->virtualAddress->size, bufferReceived + offset, sizeof(value->virtualAddress->size));
	offset += sizeof(value->virtualAddress->size);

	//7)page quantity
	memcpy(&value->cantPages, bufferReceived + offset, sizeof(value->cantPages));

}


void serializeNucleo_UMC(t_MessageNucleo_UMC *value, char *buffer, int valueSize){
	int offset = 0;
	enum_processes process = NUCLEO;

	//0)valueSize
	memcpy(buffer, &valueSize, sizeof(valueSize));
	offset += sizeof(valueSize);

	//1)From process
	memcpy(buffer + offset, &process, sizeof(process));
	offset += sizeof(process);

	//2)operation
	memcpy(buffer + offset, &value->operation, sizeof(value->operation));
	offset += sizeof(value->operation);

	//3)PID
	memcpy(buffer + offset, &value->PID, sizeof(value->PID));
	offset += sizeof(value->PID);

	//4)page quantity
	memcpy(buffer + offset, &value->cantPages, sizeof(value->cantPages));

}

void deserializeUMC_Nucleo(t_MessageNucleo_UMC *value, char *bufferReceived){
	int offset = 0;

	//2)operation
	memcpy(&value->operation, bufferReceived, sizeof(value->operation));
	offset += sizeof(value->operation);

	//3)PID
	memcpy(&value->PID, bufferReceived + offset, sizeof(value->PID));
	offset += sizeof(value->PID);

	//4)page quantity
	memcpy(&value->cantPages, bufferReceived + offset, sizeof(value->cantPages));

}

void serializeNucleo_CPU(t_MessageNucleo_CPU *value, char *buffer, int valueSize) {
	int offset = 0;
	enum_processes process = NUCLEO;

	//0)valueSize
	memcpy(buffer, &valueSize, sizeof(valueSize));
	offset += sizeof(valueSize);

	//1)From process
	memcpy(buffer + offset, &process, sizeof(process));
	offset += sizeof(process);

	//2)processID
	memcpy(buffer + offset, &value->processID, sizeof(value->processID));
	offset += sizeof(value->processID);

	//3)ProgramCounter
	memcpy(buffer + offset, &value->programCounter, sizeof(value->programCounter));
	offset += sizeof(value->programCounter);

    //4)cantidadDePaginas de codigo
    memcpy(buffer + offset, &value->cantidadDePaginas, sizeof(value->cantidadDePaginas));
	offset += sizeof(value->cantidadDePaginas);

	//5)StackPointer
	memcpy(buffer + offset, &value->stackPointer, sizeof(value->stackPointer));
	offset += sizeof(value->stackPointer);

	//6)quantum_sleep
	memcpy(buffer + offset, &value->quantum_sleep, sizeof(value->quantum_sleep));
	offset += sizeof(value->quantum_sleep);

	//7)quantum
	memcpy(buffer + offset, &value->quantum, sizeof(value->quantum));
	offset += sizeof(value->quantum);

}

void deserializeCPU_Nucleo(t_MessageNucleo_CPU *value, char * bufferReceived) {
	int offset = 0;

	//2)processID
	memcpy(&value->processID, bufferReceived + offset, sizeof(value->processID));
	offset += sizeof(value->processID);

	//3)ProgramCounter
	memcpy(&value->programCounter, bufferReceived + offset, sizeof(value->programCounter));
	offset += sizeof(value->programCounter);

	//4)cantidadDePaginas de codigo
	memcpy(&value->cantidadDePaginas, bufferReceived + offset, sizeof(value->cantidadDePaginas));
	offset += sizeof(value->cantidadDePaginas);

	//5)StackPointer
	memcpy(&value->stackPointer, bufferReceived + offset, sizeof(value->stackPointer));
	offset += sizeof(value->stackPointer);

	//6)quantum_sleep
	memcpy(&value->quantum_sleep, bufferReceived + offset, sizeof(value->quantum_sleep));
	offset += sizeof(value->quantum_sleep);

	//7)quantum
	memcpy(&value->quantum, bufferReceived + offset, sizeof(value->quantum));
	offset += sizeof(value->quantum);

}

void serializeCPU_Nucleo(t_MessageCPU_Nucleo *value, char *buffer, int valueSize) {
	int offset = 0;
	enum_processes process = CPU;

	//0)valueSize
	memcpy(buffer, &valueSize, sizeof(valueSize));
	offset += sizeof(valueSize);

	//1)From process
	memcpy(buffer + offset, &process, sizeof(process));
	offset += sizeof(process);

	//2)processID
	memcpy(buffer + offset, &value->processID, sizeof(value->processID));
	offset += sizeof(value->processID);

	//3)operacion
	memcpy(buffer + offset, &value->operacion, sizeof(value->operacion));

}

void deserializeNucleo_CPU(t_MessageCPU_Nucleo *value, char *bufferReceived){
	int offset = 0;

	//2)processID
	memcpy(&value->processID, bufferReceived + offset, sizeof(value->processID));
	offset += sizeof(value->processID);

	//3)operacion
	memcpy(&value->operacion, bufferReceived + offset, sizeof(value->operacion));

}


char *getProcessString (enum_processes process){

	char *processString;
	switch (process){
		case CONSOLA:{
			processString = "CONSOLA";
			break;
		}
		case NUCLEO:{
			processString = "NUCLEO";
			break;
		}
		case UMC:{
			processString = "UMC";
			break;
		}
		case SWAP:{
			processString = "SWAP";
			break;
		}
		case CPU:{
			processString = "CPU";
			break;
		}
		default:{
			perror("Process not recognized");//TODO => Agregar logs con librerias
			printf("Invalid process '%d' tried to send a message\n",(int) process);
			processString = NULL;
		}
	}
	return processString;
}

/* EJEMPLO DE COMO CREAR UN CLIENTE Y MANDAR MENSAJES AL SERVER
int socketClient;

	char *ip= "127.0.0.1";
	int exitcode = openClientConnection(ip, 8080, &socketClient);

	//If exitCode == 0 the client could connect to the server
	if (exitcode == 0){

		// ***1) Send handshake
		sendClientHandShake(&socketClient,SWAP);

		// ***2)Receive handshake response
		//Receive message size
		int messageSize = 0;
		char *messageRcv = malloc(sizeof(messageSize));
		int receivedBytes = receiveMessage(&socketClient, messageRcv, sizeof(messageSize));

		//Receive message using the size read before
		memcpy(&messageSize, messageRcv, sizeof(int));
		printf("messageRcv received: %s\n",messageRcv);
		printf("messageSize received: %d\n",messageSize);
		messageRcv = realloc(messageRcv,messageSize);
		receivedBytes = receiveMessage(&socketClient, messageRcv, messageSize);

		printf("bytes received: %d\n",receivedBytes);

		//starting handshake with client connected
		t_MessageGenericHandshake *message = malloc(sizeof(t_MessageGenericHandshake));
		deserializeHandShake(message, messageRcv);

		switch (message->process){
			case ACCEPTED:{
				printf("%s\n",message->message);
				break;
			}
			default:{
				perror("Process couldn't connect to SERVER");//TODO => Agregar logs con librerias
				printf("Not able to connect to server %s. Please check if it's down.\n",ip);
				break;
			}
		}

		while(1){
			//aca tiene que ir una validacion para ver si el server sigue arriba
			//send(socketClient, msg, sizeof(msg),0);
		}

	}else{
		perror("no me pude conectar al server!"); //
		printf("mi socket es: %d\n", socketClient);
	}

	return exitcode;

 */

void cleanIndiceDeStack(t_registroStack* registroS){
	list_destroy_and_destroy_elements(registroS->args,(void*)destruirArgs);
	list_destroy_and_destroy_elements(registroS->vars,(void*)destruirVars);
	free(registroS->retVar);
	free(registroS);
}

void destruirPCB(t_PCB* PCB){
	list_destroy_and_destroy_elements(PCB->indiceDeCodigo,(void*)destruirRegistroIndiceDeCodigo);
	list_destroy_and_destroy_elements(PCB->indiceDeStack,(void*)destruirRegistroIndiceDeStack);
	free(PCB->indiceDeEtiquetas);
	free(PCB);
}

void destruirRegistroIndiceDeCodigo(t_registroIndiceCodigo *registroIC){
	free(registroIC);
}

void destruirRegistroIndiceDeStack(t_registroStack* registroS){
	list_destroy_and_destroy_elements(registroS->args,(void*)destruirArgs);
	list_destroy_and_destroy_elements(registroS->vars,(void*)destruirVars);
	free(registroS->retVar);
	free(registroS);
}

void destruirArgs(t_memoryLocation* args){
	free(args);
}

void destruirVars(t_vars* vars){
	free(vars->direccionValorDeVariable);
	free(vars);
}
