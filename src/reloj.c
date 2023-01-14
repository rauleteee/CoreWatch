/* IMPLEMENTACIÓN DE RELOJ.C PARA PROYECTO SDGII 
@AUTHOR: Juan José Arlandis y Raúl Giménez
*/

#include "reloj.h"
#include "tmr.h"
#include "systemConfig.h"
#include "util.h"

fsm_trans_t g_fsmTransReloj[] = {{WAIT_TIC, CompruebaTic, WAIT_TIC, ActualizaReloj},{-1, NULL, -1, NULL}};
static TipoRelojShared g_relojSharedVars;
//------------------------------------------------------
// FUNCIONES DE INICIALIZACION DE LAS VARIABLES
//------------------------------------------------------

/* >>>>>>>>>>>>>>>>>>>>>>>> RESET RELOJ <<<<<<<<<<<<<<<<<<<<<<<<
*
*	Funcion que reinicia el reloj a valores por defecto (valores por defecto creados como etiquetas en reloj.h).
*/
void ResetReloj(TipoReloj *p_reloj) 
{
	TipoCalendario calendario1 = { 	.dd = DEFAULT_DAY ,
				   					.MM = DEFAULT_MONTH,
				   					.yyyy = DEFAULT_YEAR};
	TipoHora hora1 = {				.hh = DEFAULT_HOUR,
									.mm = DEFAULT_MIN,
									.ss = DEFAULT_SEC,
									.formato = DEFAULT_TIME_FORMAT};
	p_reloj -> timestamp = 0;
	p_reloj -> hora = hora1;
	p_reloj -> calendario = calendario1;

	piLock(RELOJ_KEY);
	g_relojSharedVars.flags = 0;
	piUnlock(RELOJ_KEY);

}
/* >>>>>>>>>>>>>>>>>>>>>>>> CONFIGURA INIZIALIZA RELOJ<<<<<<<<<<<<<<<<<<<<<<<<
*
*	Activa el reloj de forma que incluye un timer periodico en un reloj limpio. Aqui se comprueba si la hebra
*   de inicio del reloj se ha creado correctamente. 
*/

int ConfiguraInicializaReloj (TipoReloj *p_reloj)
{
	//reinicia reloj a 0	
	ResetReloj(p_reloj);
	//creamos un nuevo timer con interrupciones (isr)=tmr_tic
	tmr_t* tmr_tic = tmr_new(tmr_actualiza_reloj_isr);
	//asignamos a la estructura de p_reloj(tmrTic) nuestro nuevo temporizador
	p_reloj -> tmrTic = tmr_tic;
	//lanzamos el temporizador en milisegundos
	tmr_startms_periodic(tmr_tic, CLK_MS);
	if(tmr_tic != 0){
		printf("HE CREADO CORRECTAMENTE EL RELOJ DEL SISTEMA\n");
		delay(1000);
		return (SUCCEED);
	}
	return (ERROR_VALUE);
}
//------------------------------------------------------
// FUNCIONES PROPIAS
//------------------------------------------------------
/* >>>>>>>>>>>>>>>>>>>>>>>> COMPRUEBA TIC<<<<<<<<<<<<<<<<<<<<<<<<
*
*	Comprobacion del flag FLAG_ACTUALIZA_RELOJ en g_relojSharedVars definida como flag en reloj.h.
*	Se hace uso de mutex para proteger la variable global.
*/
int CompruebaTic(fsm_t *p_this)
{
	piLock(RELOJ_KEY);
	int res = (FLAG_ACTUALIZA_RELOJ & g_relojSharedVars.flags);
	piUnlock(RELOJ_KEY);
	return (res);

}
/* >>>>>>>>>>>>>>>>>>>>>>>> ACTUALIZA RELOJ <<<<<<<<<<<<<<<<<<<<<<<<
*
*	Asignacion continua de la nueva hora en el reloj en cada actualizacion. Se limpian/activan los flags que permiten
*	comprobar en todo momento el estado de la hora actualizada/desactualizada. Se hace uso de mutex para proteger la variable global.
*/
void ActualizaReloj(fsm_t *p_this)
{
	
	TipoReloj *p_miReloj = (TipoReloj*)(p_this -> user_data);
	(p_miReloj -> timestamp) += 1;
	ActualizaHora(&(p_miReloj -> hora));
	TipoHora mi_hora = p_miReloj -> hora;
	//OJO, NO TIENEN EN CUENTA QUE A LAS 12 HORAS SE ACTUALIZA LA FECHA IGUAL
	//DE FORMA ERRONEA, PERO NOS PIDEN NO DIFERENCIAR ENTRE AM Y PM.
	if (mi_hora.ss == 0 && mi_hora.mm == 0 && mi_hora.hh == 0 && mi_hora.formato==24)
		ActualizaFecha(&p_miReloj->calendario);
	if (mi_hora.ss == 0 && mi_hora.mm == 0 && mi_hora.hh == 1 && mi_hora.formato==12)
		ActualizaFecha(&p_miReloj->calendario);

	piLock(RELOJ_KEY);
	g_relojSharedVars.flags &= ~FLAG_ACTUALIZA_RELOJ;
	piUnlock(RELOJ_KEY);
	piLock(RELOJ_KEY);
	g_relojSharedVars.flags |= FLAG_TIME_ACTUALIZADO;
	piUnlock(RELOJ_KEY);

}
/* >>>>>>>>>>>>>>>>>>>>>>>> TMR_ACTUALIZA_RELOJ_ISR <<<<<<<<<<<<<<<<<<<<<<<<
*
*	Activa flag para actualizar el reloj (comprobacion del thread interno).
*/
void tmr_actualiza_reloj_isr(union sigval value)
{
	piLock (RELOJ_KEY);
	g_relojSharedVars.flags |= FLAG_ACTUALIZA_RELOJ;
	piUnlock (RELOJ_KEY);
}
/* >>>>>>>>>>>>>>>>>>>>>>>> ACTUALIZA FECHA <<<<<<<<<<<<<<<<<<<<<<<<
*
*	Misma dinamica que ActualizaReloj, en este caso con el calendario del struct Reloj.
*/
void ActualizaFecha(TipoCalendario *p_fecha)
{
	int day, month, year;

	int nDays, maxDay, resultDay;
	int maxMonth, nMonth;

	
	day = p_fecha->dd;
	month = p_fecha->MM;
	year = p_fecha->yyyy;


	nDays = CalculaDiaMes(month, year);

	day += 1;

	resultDay = day % (nDays + 1);
	maxDay = MAX(1,resultDay);
	p_fecha -> dd = maxDay;

	if(maxDay == 1)
	{
		month += 1;
		nMonth = month % (MAX_MONTH +1);
		maxMonth = MAX(1, nMonth);
		if(maxDay == 1 && maxMonth == 1)
		{
			year += 1;
			p_fecha->yyyy = year;
		}
		p_fecha->MM = maxMonth;
	}

}
/* >>>>>>>>>>>>>>>>>>>>>>>> ACTUALIZA HORA <<<<<<<<<<<<<<<<<<<<<<<<
*
*	Actualizacion de la hora del reloj en horas, minutos y segundos. Se tiene en cuenta la diferencia entre formato de hora de 12 y 24.
*/
void ActualizaHora(TipoHora *p_hora)
{
	int sec, min, hour, format;

	sec = p_hora -> ss;
	min = p_hora -> mm;
	hour = p_hora -> hh;
	format = p_hora -> formato;

	sec += 1;
	sec = sec % 60;
	p_hora -> ss = sec;

	if (sec == 0)
	{
		min += 1;
		min = min % 60;
		p_hora -> mm = min;
	}

	if (min == 0 && sec == 0)
	{
		hour += 1;

		if (format == MAX_HOUR_12)
			hour = (hour % (MAX_HOUR_12));
		if (format == MAX_HOUR_24)
			hour = hour % (MAX_HOUR_24);

		p_hora -> hh = hour;
		sec += 1;
	}
}
/* >>>>>>>>>>>>>>>>>>>>>>>> CALCULA DIA MES <<<<<<<<<<<<<<<<<<<<<<<<
*
*	Funcion que devuelve el numero de dias de cada mes segun si el ano es bisiesto o no es bisiesto.
* 	PUNTUALIZACION: Comenzamos array aumentando una posicion vacia para cuadrarlo con la actualizacion del calendario.
*/
int CalculaDiaMes(int month, int year)
{
	int	i;
	int DIAS_MESES_BISIESTOS[13] = {0 ,31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	int DIAS_MESES_NO_BISIESTOS[13] = {0 ,31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	i = 1;
	
	if (EsBisiesto(year) == 1)
	{
		while (DIAS_MESES_BISIESTOS[i])
		{
			if (i == month)
				return (DIAS_MESES_BISIESTOS[i]);
			i++;
		}	
	}
	i = 1;
	while (DIAS_MESES_NO_BISIESTOS[i])
	{
		if (i == month)
			return (DIAS_MESES_NO_BISIESTOS[i]);
		i++;
	}
	return (ERROR_VALUE);
}
/* >>>>>>>>>>>>>>>>>>>>>>>> SET HORA <<<<<<<<<<<<<<<<<<<<<<<<
*
*	Funcion para setear la hora en el reloj. La dinamica interna es un tanto especial, ya que solo admitira un numero de digitos
*	concreto para setear una hora correcta, teniendo en cuenta el formato de las horas para que lo haga correctamente.
*/
int SetHora(int horaInt, TipoHora *p_hora)
{
	int numDigitos;
	int horaExtraida;
	int format;
	int minsExtraidos;
	int specialCase;

	numDigitos = 0;
	horaExtraida = 0;
	minsExtraidos = 0;
	specialCase = horaInt;
	format = p_hora->formato;
	if (horaInt < 0)
		return (ERROR_VALUE);
	numDigitos = ft_strlen(horaInt);

	if (!(numDigitos >= 1 && numDigitos <= 4))
		return (ERROR_VALUE);

	if (numDigitos == 4 || numDigitos == 3)
		horaExtraida = horaInt / 100;
	if (numDigitos == 1 || numDigitos == 2)
		horaExtraida = 0;

	if (format == 12 && horaExtraida > MAX_HOUR_12)
		p_hora->hh = horaExtraida - MAX_HOUR_12;

	else if (format == 12 && horaExtraida == 0)
	{
		p_hora->hh = MAX_HOUR_12;
		p_hora->mm = specialCase;
		p_hora->ss = 0;
		return (SUCCEED);
	}

	
	else if (format == 24 && horaExtraida > MAX_HOUR_24)
		p_hora->hh = MAX_HOUR_24 - 1;
	
	else if (format == 24 && horaExtraida == MAX_HOUR_24)
		p_hora->hh = 0;
	
	else { 
		p_hora->hh = horaExtraida;
	}

	horaExtraida *= 100;
	minsExtraidos = horaInt - horaExtraida;
	if (minsExtraidos > 60)
		p_hora->mm = 59;
		
	p_hora->mm = minsExtraidos;
	p_hora->ss = 0;
	return (SUCCEED);	
}

int EsBisiesto(int year)
{
	if ((year % 4 == 0) && ( year%100 != 0 || year%400 == 0))
		return (1);
	return (SUCCEED);			
}
/* >>>>>>>>>>>>>>>>>>>>>>>> ft_strlen <<<<<<<<<<<<<<<<<<<<<<<<
*
*	Funcion que devuelve el numero de digitos de un entero.
*/
int ft_strlen(int str)
{
    int     i;

    i = 0;
    while (str != 0)
    {
        str /= 10;
        i++;
    }
    return (i);
}
/* >>>>>>>>>>>>>>>>>>>>>>>> GET/SET RELOJSHAREDVARS <<<<<<<<<<<<<<<<<<<<<<<<
*
*	Getters&Setters de la variable global compartida con el sistema coreWatch. Imprescindible para la conexion entre ambos.
*/
TipoRelojShared GetRelojSharedVar()
{

	TipoRelojShared relojSharedVars;
	piLock(RELOJ_KEY);
	relojSharedVars = g_relojSharedVars;
	piUnlock(RELOJ_KEY);
	return (relojSharedVars);
}

void SetRelojSharedVar(TipoRelojShared value)
{
	piLock(RELOJ_KEY);
	g_relojSharedVars = value;
	piUnlock(RELOJ_KEY);
}



