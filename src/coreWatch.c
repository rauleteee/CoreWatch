/* IMPLEMENTACIÓN DE COREWATCH.C PARA PROYECTO SDGII 2021/22
@AUTHOR: Juan José Arlandis y Raúl Giménez
*/

#include "coreWatch.h"
#include "reloj.h"

TipoCoreWatch g_coreWatch;
static int g_flagsCoreWatch;
fsm_trans_t fsmTransCoreWatch[] = {
	{START, CompruebaSetupDone, STAND_BY, Start},
	{STAND_BY, CompruebaTimeActualizado, STAND_BY, ShowTime},
	{STAND_BY, CompruebaReset, STAND_BY, Reset},
	{STAND_BY, CompruebaSetCancelNewTime, SET_TIME, PrepareSetNewTime},
	{SET_TIME, CompruebaSetCancelNewTime, STAND_BY, CancelSetNewTime},
	{SET_TIME, CompruebaDigitoPulsado, SET_TIME, ProcesaDigitoTime},
	{SET_TIME, CompruebaNewTimeIsReady, STAND_BY, SetNewTime},
	{-1, NULL, -1, NULL},
	};


//------------------------------------------------------
// FUNCIONES PROPIAS
//------------------------------------------------------
// Wait until next_activation (absolute time)
// Necesita de la función "delay" de WiringPi.

void DelayUntil(unsigned int next) {
	unsigned int now = millis();
	if (next > now) {
		delay(next - now);
	}
}
/* >>>>>>>>>>>>>>>>>>>>>>>> PI_THREAD <<<<<<<<<<<<<<<<<<<<<<<<
*
*	Funcion que comprueba mediante un thread continuamente si se esta pulsando una tecla mediante kbhit() y kbread().
*	En el momento que se pulsa, reconoce la tecla pulsada asignadosela a una variable de teclaPulsada segun la posicion del teclado PC.
*	Hace uso de mutex para activar/desactivar los flags correspondientes a cada estado del diagrama de flujo de funcionamiento de coreWatch.
*/
PI_THREAD (ThreadExploraTecladoPC)
{
	char teclaPulsada;

	while(1)
	{
		delay(10);	
		if (kbhit())
		{
			teclaPulsada = kbread();
			//Logica Diagrama de Flujo

			if (teclaPulsada == TECLA_RESET)
			{
				piLock(SYSTEM_KEY);
				g_flagsCoreWatch |= FLAG_RESET;
				piUnlock(SYSTEM_KEY);
				teclaPulsada = TECLA_RESET;
			}
			else if(teclaPulsada == TECLA_SET_CANCEL_TIME)
			{
				piLock(SYSTEM_KEY);
				g_flagsCoreWatch |= FLAG_SET_CANCEL_NEW_TIME;
				piUnlock(SYSTEM_KEY);
				teclaPulsada = TECLA_SET_CANCEL_TIME;
			}
			else if (EsNumero(teclaPulsada))
			{
				g_coreWatch.digitoPulsado = (int)teclaPulsada;
				piLock(SYSTEM_KEY);
				g_flagsCoreWatch |= FLAG_DIGITO_PULSADO;
				piUnlock(SYSTEM_KEY);
			}
			else if (teclaPulsada == TECLA_EXIT)
			{
				piLock(SYSTEM_KEY);
				printf("----------------------------------------------\n");
				printf(" Acabas de salir del sistema.\n");
				printf(" Hasta luego ! ಥ_ಥ \n");
				printf("----------------------------------------------\n");
				piUnlock(SYSTEM_KEY);
				exit(0);
			}
		}
	}
}
/* >>>>>>>>>>>>>>>>>>>>>>>> ConfiguraInicializaSistema <<<<<<<<<<<<<<<<<<<<<<<<
*
*	Misma dinamica que configuraInicializaReloj para coreWatch.
*/
int ConfiguraInicializaSistema(TipoCoreWatch *p_sistema)
{

		g_flagsCoreWatch = 0;
		p_sistema->tempTime = 0;
		p_sistema->digitosGuardados = 0;
		p_sistema -> digitoPulsado = 0;


		int flagRelojIsIniciado = ConfiguraInicializaReloj(&p_sistema->reloj);

		if (flagRelojIsIniciado != 0)
			exit(0);

		piLock(SYSTEM_KEY);
		printf("----------------------\n");
		printf("...Creando la hebra...\n");
		printf("----------------------\n");
		piUnlock(SYSTEM_KEY);
		delay(2000);
		piLock(SYSTEM_KEY);int thread = piThreadCreate(ThreadExploraTecladoPC);piUnlock(SYSTEM_KEY);
		if (thread == 0)
		{
			piLock(SYSTEM_KEY);
			g_flagsCoreWatch |= FLAG_SETUP_DONE;
			piUnlock(SYSTEM_KEY);
			piLock(SYSTEM_KEY);
			printf("-----------------------------------\n");
			printf("SE HA CREADO CORRECTAMENTE LA HEBRA\n");
			printf("-----------------------------------\n");
			piUnlock(SYSTEM_KEY);
			delay(1000);
			return (SUCCEED);
		}
		else
			return (ERROR_VALUE);
}

/* 
*	>>>>>>>>>>>>>>EsNumero<<<<<<<<<<<<<<
*
*	Devuelve 1 si es un caracter numerico, 0 si no lo es.
*/
int EsNumero(char c)
{
	if (c >= '0' && c <= '9')
		return (1);
	return (0);
}

/* 
*	>>>>>>>>>>>>>> COMPRUEBA_____________ <<<<<<<<<<<<<<
*
*	Funciones de comprobacion de FLAGS para pasar de un estado de la maquina a otro.
*/

int CompruebaDigitoPulsado(fsm_t* p_this)
{
	piLock(SYSTEM_KEY);
	int res = (FLAG_DIGITO_PULSADO & g_flagsCoreWatch);
	piUnlock(SYSTEM_KEY);
	return (res);
}

int CompruebaNewTimeIsReady(fsm_t* p_this)
{
	piLock(SYSTEM_KEY);
	int res = (FLAG_NEW_TIME_IS_READY & g_flagsCoreWatch);
	piUnlock(SYSTEM_KEY);
	return (res);
}

int CompruebaReset(fsm_t* p_this)
{
	piLock(SYSTEM_KEY);
	int res = (FLAG_RESET & g_flagsCoreWatch);
	piUnlock(SYSTEM_KEY);
	return (res);
}

int CompruebaSetCancelNewTime(fsm_t* p_this)
{
	piLock(SYSTEM_KEY);
	int res = (FLAG_SET_CANCEL_NEW_TIME & g_flagsCoreWatch);
	piUnlock(SYSTEM_KEY);
	return (res);
}

int CompruebaSetupDone(fsm_t* p_this)
{
	piLock(SYSTEM_KEY);
	int res = (FLAG_SETUP_DONE & g_flagsCoreWatch);
	piUnlock(SYSTEM_KEY);
	return (res);
}


int CompruebaTimeActualizado(fsm_t* p_this)
{
	TipoRelojShared auxRelojShared;

	auxRelojShared = GetRelojSharedVar();
	return (auxRelojShared.flags & FLAG_TIME_ACTUALIZADO);
}
/* 
*	>>>>>>>>>>>>>> START <<<<<<<<<<<<<<
*	Funcion que da comienzo al primer estado del diagrama de estados de la maquina coreWatch. 
*	Hace uso de mutex para activar/desactivar los flags correspondientes a cada estado del diagrama de flujo de funcionamiento de coreWatch.
*/
void Start(fsm_t *p_this)
{
	piLock(SYSTEM_KEY);
	g_flagsCoreWatch &= (~FLAG_SETUP_DONE);
	piUnlock(SYSTEM_KEY);
	//printf("HE ENTRADO EN EL ESTADO START-----------\n");
}
/* 
*	>>>>>>>>>>>>>> SHOWTIME <<<<<<<<<<<<<<
*	Muestra por terminal la hora actual del reloj del sistema coreWatch, recuperando informacion de la variable global compartida.
*	Hace uso de mutex para evitar interbloqueos por pantalla.
*/
void ShowTime(fsm_t *p_this)
{
	TipoCoreWatch *p_sistema = (TipoCoreWatch*)(p_this -> user_data);
	TipoRelojShared aux = GetRelojSharedVar();

	piLock(SYSTEM_KEY);
	aux.flags = 0;
	piUnlock(SYSTEM_KEY);

	SetRelojSharedVar(aux);
	piLock(SYSTEM_KEY);
	printf("son las: %d:%d:%d del %d/%d/%d \n", p_sistema->reloj.hora.hh,p_sistema->reloj.hora.mm,
		p_sistema->reloj.hora.ss, p_sistema->reloj.calendario.dd,  p_sistema->reloj.calendario.MM,  p_sistema->reloj.calendario.yyyy);
	fflush(stdout);
	piUnlock(SYSTEM_KEY);
}
/* 
*	>>>>>>>>>>>>>> RESET<<<<<<<<<<<<<<
*	Estado de la maquina coreWatch que limpia el reloj del sistema y lo pone a los valores por defecto definidos en ResetReloj (reloj.c/.h).
*	Hace uso de mutex para evitar interbloqueos por pantalla.
*/
void Reset(fsm_t *p_this)
{
	TipoCoreWatch *p_sistema = (TipoCoreWatch*)(p_this-> user_data);

	ResetReloj(&(p_sistema->reloj));

	piLock(SYSTEM_KEY);
	g_flagsCoreWatch &= (~FLAG_RESET);
	piUnlock(SYSTEM_KEY);

	piLock(SYSTEM_KEY);
	printf("------------------------------------------------------\n");
	printf("HA REALIZADO UN RESET AL RELOJ EXITOSAMENTE ( ͡° ͜ʖ ͡°)\n");
	printf("------------------------------------------------------\n");
	piUnlock(SYSTEM_KEY);

}
/* 
*	>>>>>>>>>>>>>> PREPARE SET NEW TIME <<<<<<<<<<<<<<
*
*	Funcion intermedia para setear una nueva hora. Va moviendo flags segun necesario para guardar el nuevo tempTime.
*/
void PrepareSetNewTime(fsm_t *p_this)
{
	TipoCoreWatch *p_sistema = (TipoCoreWatch*)(p_this-> user_data);
	int formatoReloj = p_sistema->reloj.hora.formato;

	piLock(SYSTEM_KEY);
	g_flagsCoreWatch &= ~FLAG_DIGITO_PULSADO;
	g_flagsCoreWatch &= ~FLAG_SET_CANCEL_NEW_TIME;
	piUnlock(SYSTEM_KEY);
	//piLock(SYSTEM_KEY);
	printf("----------------------------------------------------------\n");
	printf("[SET_TIME] DEBE INTRODUCIR UNA NUEVA HORA EN FORMATO 0- %d\n", formatoReloj);
	printf("----------------------------------------------------------\n");
	//piUnlock(SYSTEM_KEY);

}
/* 
*	>>>>>>>>>>>>>> CANCEL SET NEW TIME <<<<<<<<<<<<<<
*
*	Funcion intermedia para setear una nueva hora. Cancela el proceso de setear la hora.
*/
void CancelSetNewTime(fsm_t *p_this)
{
	TipoCoreWatch *p_sistema = (TipoCoreWatch*)(p_this-> user_data);
	p_sistema->digitosGuardados = 0;
	p_sistema->tempTime = 0;

	piLock(SYSTEM_KEY);
	g_flagsCoreWatch &= ~FLAG_SET_CANCEL_NEW_TIME;
	piUnlock(SYSTEM_KEY);

	printf("[SET_TIME] OPERACION CANCELADA\n");
}
/* 
*	>>>>>>>>>>>>>> PROCESA DIGITO TIME <<<<<<<<<<<<<<
*
*	Funcion intermedia para setear una nueva hora. Va guardando en memoria cada digito guardado tecleado segun diagrama de flujo.
*/
void ProcesaDigitoTime(fsm_t* p_this)
{
	TipoCoreWatch *p_sistema = (TipoCoreWatch*)(p_this -> user_data);
	int tempTime = p_sistema->tempTime;
	int digitosGuardados = p_sistema->digitosGuardados;
	int ultimoDigito = (p_sistema->digitoPulsado) - 48;
	piLock(SYSTEM_KEY);
	g_flagsCoreWatch &= ~FLAG_DIGITO_PULSADO;
	piUnlock(SYSTEM_KEY);

	//Diagrama de flujo
	if (digitosGuardados == 0){
		ultimoDigito = MIN(2, ultimoDigito);
		tempTime = tempTime*10 + ultimoDigito;
		digitosGuardados++;

	}
	else if(digitosGuardados == 1){
			if(tempTime==2){
				ultimoDigito = MIN(3, ultimoDigito);
			}

			tempTime=tempTime*10 + ultimoDigito;
			digitosGuardados++;
	}

	else if(digitosGuardados == 2){
		tempTime = tempTime * 10 + MIN(5, ultimoDigito);
		digitosGuardados++;
	}
	else{
		tempTime = tempTime*10 + ultimoDigito;
		piLock(SYSTEM_KEY);g_flagsCoreWatch &= ~FLAG_DIGITO_PULSADO;piUnlock(SYSTEM_KEY);
		piLock(SYSTEM_KEY);g_flagsCoreWatch |= FLAG_NEW_TIME_IS_READY;piUnlock(SYSTEM_KEY);

	}
	printf("[SET_TIME] Nueva hora temporal = %d\n", tempTime);
	p_sistema->tempTime = tempTime;
	p_sistema->digitosGuardados = digitosGuardados;
	if (tempTime == 0)
		ResetReloj(&p_sistema->reloj);

}

/* 
*	>>>>>>>>>>>>>>  SET NEW TIME <<<<<<<<<<<<<<
*
*	Funcion intermedia para setear una nueva hora. Guarda la nueva hora en el reloj del sistema.
*/
void SetNewTime(fsm_t* p_this)
{
	TipoCoreWatch *p_sistema = (TipoCoreWatch*)(p_this->user_data);
	piLock(SYSTEM_KEY);
	g_flagsCoreWatch &= (~FLAG_NEW_TIME_IS_READY);
	piUnlock(SYSTEM_KEY);
	printf("\n...SETEANDO LA NUEVA...\n\n");
	delay(1000);
	SetHora(p_sistema->tempTime, &(p_sistema->reloj.hora));
	printf("---¡HORA GUARDADA CORRECTAMENTE!---\n");
	p_sistema->tempTime = 0;
	p_sistema->digitosGuardados = 0;
}

//------------------------------------------------------
// MAIN
//------------------------------------------------------
// LANZAMIENTO DEL SISTEMA DE COREWATCH EN CONJUNTO CON EL RELOJ DEL MISMO
int main(void) {

	unsigned int next;
	
	fsm_t* fsmReloj = fsm_new(WAIT_TIC, g_fsmTransReloj, &(g_coreWatch.reloj));
	fsm_t* fsmCoreWatch = fsm_new(START,fsmTransCoreWatch, &(g_coreWatch));

	ConfiguraInicializaSistema(&g_coreWatch);

 	next = millis();
	while(1)
	{
		next += 1000;
		DelayUntil(next);

		fsm_fire(fsmCoreWatch);
		fsm_fire(fsmReloj);
		

	}
	tmr_destroy(g_coreWatch.reloj.tmrTic);
	fsm_destroy(fsmReloj);
	fsm_destroy(fsmCoreWatch);
}
