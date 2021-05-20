#include "common-types.h"

/******************* PRIMITIVAS *******************/

/*
 * Listado
	definirVariable
	obtenerPosicionVariable
	dereferenciar
	asignar
	obtenerValorCompartida
	asignarValorCompartida
	irAlLabel
	llamarConRetorno
	retornar
	imprimir
	imprimirTexto
	entradaSalida
	wait
	signal
 *
 */

int tamanioDePagina = -1; //TODO ver como se lo paso desde la UMC

void setPageSize (int pageSize){
	tamanioDePagina = pageSize;
}

int getLogicalAddress (int page){
	return (page * tamanioDePagina);
}
