#ifndef COREWATCH_H_
#define COREWATCH_H_

// INCLUDES
// Propios:
#include "systemConfig.h"     
#include "reloj.h"
#include "tmr.h"
// Sistema: includes, entrenadora (GPIOs, MUTEXes y entorno), setup de perifericos y otros otros.


// DEFINES Y ENUMS
enum FSM_ESTADOS_SISTEMA
{
	START,
	STAND_BY,
	SET_TIME
};



// FLAGS FSM DEL SISTEMA CORE WATCH

#define FLAG_SETUP_DONE 0x08
#define FLAG_SET_CANCEL_NEW_TIME 0x10
#define FLAG_NEW_TIME_IS_READY 0x20
#define FLAG_DIGITO_PULSADO 0x40

#define TECLA_SET_CANCEL_TIME  101
#define TECLA_RESET 102
#define TECLA_EXIT 98


// DECLARACIÓN ESTRUCTURAS

typedef struct
{
	TipoReloj reloj;
	int tempTime;
	int digitosGuardados;
	int digitoPulsado;
}TipoCoreWatch;



// DECLARACIÓN VARIABLES



// DEFINICIÓN VARIABLES


//------------------------------------------------------
// FUNCIONES DE INICIALIZACION DE LAS VARIABLES
//------------------------------------------------------
int ConfiguraInicializaSistema(TipoCoreWatch *p_sistema);
//------------------------------------------------------
// FUNCIONES PROPIAS
//------------------------------------------------------
void DelayUntil(unsigned int next);

//------------------------------------------------------
// FUNCIONES DE ENTRADA O DE TRANSICION DE LA MAQUINA DE ESTADOS
//------------------------------------------------------
void Start(fsm_t *p_this);
int CompruebaDigitoPulsado(fsm_t* p_this);
int CompruebaNewTimeIsReady(fsm_t* p_this);
int CompruebaReset(fsm_t* p_this);
int CompruebaSetCancelNewTime(fsm_t* p_this);
int CompruebaSetupDone(fsm_t* p_this);
int CompruebaTimeActualizado(fsm_t* p_this);
int EsNumero(char c);
void Reset(fsm_t *p_this);
void PrepareSetNewTime(fsm_t *p_this);
void CancelSetNewTime(fsm_t *p_this);
void SetNewTime(fsm_t* p_this);
void ProcesaDigitoTime(fsm_t* p_this);

//------------------------------------------------------
// FUNCIONES DE SALIDA O DE ACCION DE LA MAQUINA DE ESTADOS
//------------------------------------------------------
void ShowTime(fsm_t *p_this);

//------------------------------------------------------
// SUBRUTINAS DE ATENCION A LAS INTERRUPCIONES
//------------------------------------------------------


//------------------------------------------------------
// FUNCIONES LIGADAS A THREADS ADICIONALES
//------------------------------------------------------

PI_THREAD(ThreadExploraTecladoPC);

#endif /* EAGENDA_H */
