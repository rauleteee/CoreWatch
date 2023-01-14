/*IMPLEMENTACIÓN DE RELOJ.h PARA PROYECTO SDGII 
@AUTHOR: Juan José Arlandis y Raúl Giménez
*/


#ifndef RELOJ_H_
#define RELOJ_H_

#include "systemConfig.h"
#include "util.h"


//DEFINE, ENUM
#define MIN_DAY 1
#define MAX_MONTH 12
#define MIN_MONTH 1
#define MIN_YEAR 1970

#define MIN_HOUR_12 1
#define MAX_HOUR_12 12
#define MIN_HOUR_24 1
#define MAX_HOUR_24 24

#define DEFAULT_DAY MIN_DAY
#define DEFAULT_MONTH MIN_MONTH
#define DEFAULT_YEAR MIN_YEAR

#define DEFAULT_HOUR 00
#define DEFAULT_MIN 00
#define DEFAULT_SEC 00

#define DEFAULT_TIME_FORMAT 24

#define PRECISION_RELOJ_MS 1 //esta bien?

#define ERROR_VALUE -1
#define SUCCEED 0




enum FSM_ESTADOS_RELOJ
{
	WAIT_TIC
};
//FLAGS
#define FLAG_RESET 0x01
#define FLAG_ACTUALIZA_RELOJ 0x02
#define FLAG_TIME_ACTUALIZADO 0x04



//ESTRUCTURAS
typedef struct
{
	int dd;
	int MM;
	int yyyy;
} TipoCalendario;

typedef struct
{
	int hh;
	int mm;
	int ss;
	int formato;
} TipoHora;

typedef struct
{
	int timestamp;
	TipoHora hora;
	TipoCalendario calendario;
	tmr_t *tmrTic;
} TipoReloj;

typedef struct
{
	int flags;
} TipoRelojShared;

// DECLARACIÓN VARIABLES
extern fsm_trans_t g_fsmTransReloj[];

//------------------------------------------------------
// FUNCIONES DE INICIALIZACION DE LAS VARIABLES
//------------------------------------------------------
int ConfiguraInicializaReloj(TipoReloj *p_reloj);
void ResetReloj(TipoReloj *p_reloj);

//------------------------------------------------------
// FUNCIONES PROPIAS
//------------------------------------------------------
void ActualizaFecha(TipoCalendario *p_fecha);
void ActualizaHora(TipoHora *p_hora);
int CalculaDiaMes (int month, int year);
int EsBisiesto(int year);
TipoRelojShared GetRelojSharedVar();
int SetFecha(int nuevaFecha, TipoCalendario *p_fecha);
int SetFormato(int nuevoFormato, TipoHora *p_hora);
int SetHora(int nuevaHora, TipoHora *p_hora);
void SetRelojSharedVar(TipoRelojShared value);

//length de un int 
int ft_strlen(int str);

//------------------------------------------------------
// FUNCIONES DE ENTRADA O DE TRANSICION DE LA MAQUINA DE ESTADOS
//------------------------------------------------------
int CompruebaTic(fsm_t *p_this);

//------------------------------------------------------
// FUNCIONES DE SALIDA O DE ACCION DE LA MAQUINA DE ESTADOS
//------------------------------------------------------
void ActualizaReloj(fsm_t *p_this);

//------------------------------------------------------
// SUBRUTINAS DE ATENCION A LAS INTERRUPCIONES
//------------------------------------------------------
void tmr_actualiza_reloj_isr(union sigval value);

//------------------------------------------------------
// FUNCIONES LIGADAS A THREADS ADICIONALES
//------------------------------------------------------


#endif /*-------RELOJ_H-------*/