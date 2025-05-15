/*****************************************************************************/
/*									     */
/*				     tron5.c				     */
/*									     */
/*	   $ gcc -c winsuport2.c -o winsuport2.o			     	     */
/*	   $ gcc tron5.c winsuport2.o memoria.o missatge.o -o tron5 -lcurses -lpthread			 */
/*     $ gcc oponent5.c winsuport2.o memoria.o missatge.o -o oponent5 -lcurses                */
/*	   $ ./tron5 num_oponents variabilitat fitxer [retard_min retard_max]				     */
/*									     */
/*****************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include "winsuport2.h"
#include "memoria.h"
#include "missatge.h" // (Fase 5) Llibreria per enviar missatges
#include <pthread.h>
#include <time.h>


/* Variable global pel fitxer de sortida */
FILE *arxiuSortida = NULL;

/* Variable global para el nombre del archivo */
const char *nomArxiu;   // Añadir esta variable global

/* Variable global para el thread del usuario */
pthread_t threadUsuari;

/* Variables per memoria compartida */
int id_fi1, id_fi2;         /* IDs zona memoria compartida pels flags fi1/fi2 */
int *p_fi1, *p_fi2;         /* Punters a les zones de memoria compartida */
int id_pantalla;            /* ID zona memoria compartida per la pantalla */
void *p_pantalla;           /* Punter memoria compartida pantalla */
int id_vius;               /* ID zona memoria compartida pel contador d'oponents vius */
int *p_vius;               /* Punter a la zona de memoria compartida pel contador */

// (Fase 5) Variable per identificar els missatges
int id_missatges;                  /* cola de missatges */

// (Fase 5) Variable per identificar els mutex
pthread_mutex_t mutexTrajecte = PTHREAD_MUTEX_INITIALIZER;

// (Fase 5) struct de dades per les col·lisions
typedef struct {
    int f;          /* fila */
    int c;          /* columna */
    int oponent;    /* index de l'oponent que ha xocat */
} dadesColisions;

/* definir estructures d'informacio per usari*/
typedef struct {		/* per un tron (usuari) */
	int f;				/* posicio actual: fila */
	int c;				/* posicio actual: columna */
	int d;				/* direccio actual: [0..3] */
} tron;

typedef struct {		/* per una entrada de la taula de posicio */
	int f;
	int c;
} pos;

#define MAX_OPONENTS 9

int RET_MIN = 50;     /* valor mínim del retard */
int RET_MAX = 500;    /* valor màxim del retard */

int n_fil, n_col;		/* dimensions del camp de joc */

tron usu;   	   		/* informacio de l'usuari */

tron opo[MAX_OPONENTS];

int n_opo_actius = 0;

int df[] = {-1, 0, 1, 0};	/* moviments de les 4 direccions possibles */
int dc[] = {0, -1, 0, 1};	/* dalt, esquerra, baix, dreta */

int varia;		/* valor de variabilitat dels oponents [0..9] */
int retard;		/* valor del retard de moviment, en mil.lisegons */

pos *p_usu;			/* taula de posicions que va recorrent l'usuari */
int n_usu = 0;         /* numero d'entrades en la taula de pos. de l'usuari */

pos **p_opo;			/* vector de taules de posicions dels oponents */
int *n_opo;            /* vector del numero d'entrades per cada oponent */

int num_oponents = 1;      /* per defecte, un oponent */

/* funcio per esborrar totes les posicions anteriors de l'usuari */
void esborrar_posicions(pos p_pos[], int n_pos)
{
  int i;
  
  for (i=n_pos-1; i>=0; i--)
  {
    win_escricar(p_pos[i].f,p_pos[i].c,' ',NO_INV);
    win_update();  // Actualizar después de cada borrado
    win_retard(6);
  }
}

/* funcio per inicialitar les variables i visualitzar l'estat inicial del joc */
void inicialitza_joc(void)
{
  char strin[45];
  int i;
  int espaiat;

  /* inicialitza usuari (no canvia) */
  usu.f = (n_fil-1)/2;
  usu.c = (n_col)/4;
  usu.d = 3;
  win_escricar(usu.f,usu.c,'0',INVERS);
  p_usu[n_usu].f = usu.f;
  p_usu[n_usu].c = usu.c;
  n_usu++;

  /* inicialitza els oponents */
  espaiat = (n_fil - 1) / (num_oponents + 1);  /* calcul de l'espaiat */
  for (i = 0; i < num_oponents; i++) {
    opo[i].f = espaiat * (i + 1);              /* distribució equidistant */
    opo[i].c = (n_col*3)/4;                    /* mateixa columna */
    opo[i].d = 1;                              /* direcció inicial: esquerra */
    win_escricar(opo[i].f,opo[i].c,'1'+i,INVERS);
    
    /* Guardar también la posición inicial en el rastro */
    p_opo[i][0].f = opo[i].f;
    p_opo[i][0].c = opo[i].c;
    n_opo[i] = 1;  // Empezar desde 1 ya que hemos guardado la posición inicial
  }
  n_opo_actius = num_oponents;    /* actualitza comptador d'oponents */

  sprintf(strin,"Tecles: \'%c\', \'%c\', \'%c\', \'%c\', RETURN-> sortir\n",
          TEC_AMUNT, TEC_AVALL, TEC_DRETA, TEC_ESQUER);
  win_escristr(strin);
}

/* Funció pel moviment de l'usuari com thread */
// (Fase 5) Hem de modificar la llògica ja que ara l'usuari només es mor quan xoca amb les parets
// (Fase 5) A més, si el tron usuari xoca amb un tron oponent li ha d’enviar un missatge a aquest oponent.
void *mou_usuari(void *arg)
{
  char cars;
  tron seg;
  int tecla;
  dadesColisions colisio; // (Fase 5) Creem una variable del tipus col·lisió
  
  while (!(*p_fi1)) /* (Fase 5) Abans feiem while (!(*p_fi1) && !(*p_fi2)) { }, però ara no fa falta controlar p_fi2 */ {
    tecla = win_gettec();
    if (tecla != 0)
    switch (tecla)
    {
      case TEC_AMUNT:	usu.d = 0; break;
      case TEC_ESQUER:	usu.d = 1; break;
      case TEC_AVALL:	usu.d = 2; break;
      case TEC_DRETA:	usu.d = 3; break;
      case TEC_RETURN:	*p_fi1 = -1; break;
    }
    
    seg.f = usu.f + df[usu.d];
    seg.c = usu.c + dc[usu.d];

    cars = win_quincar(seg.f,seg.c);
    if (cars == ' ')
    {
      usu.f = seg.f; usu.c = seg.c;
      win_escricar(usu.f,usu.c,'0',INVERS);
      p_usu[n_usu].f = usu.f;
      p_usu[n_usu].c = usu.c;
      n_usu++;
    }
    else if (cars == '+') { // (Fase 5) Només morim si col·lisionem contra una paret
      esborrar_posicions(p_usu, n_usu);
      *p_fi1 = 1;
      break;
    }
    else if (cars >= '1' && cars <= '9') { // (Fase 5) Si el caràcter amb el que hem xocat es un numero entre 1 i 9, és un oponent i li hem d'enviar un missatge amb sendM()
      colisio.f = seg.f;
      colisio.c = seg.c;
      colisio.oponent = cars - '1';
      sendM(id_missatges, &colisio, sizeof(colisio)); // (Fase 5) Afegim les dades a la variable de col·lisió i l'enviem
    }
    win_retard(65);
  }

  pthread_exit(NULL);
}

/* programa principal */
int main(int n_args, const char *ll_args[])
{

  int retwin;
  int i, id_proc[MAX_OPONENTS];
  
  srand(getpid());

  /* Verificar arguments */
  if ((n_args != 4) && (n_args != 6))
  {
    fprintf(stderr,"Comanda: ./tron5 num_oponents variabilitat fitxer [retard_min retard_max]\n");
    fprintf(stderr,"         on num_oponents indica el nombre d'oponents (1-9)\n");
    fprintf(stderr,"         variabilitat indica la frequencia de canvi de direccio\n");
    fprintf(stderr,"         de l'oponent: de 0 a 3 (0- gens variable, 3- molt variable),\n");
    fprintf(stderr,"         fitxer: nom del fitxer on es guardaran els logs\n");
    fprintf(stderr,"         retard_min i retard_max: valors opcionals pels retards\n");
    exit(1);
  }

  /* Processar num_oponents */
  num_oponents = atoi(ll_args[1]);
  if (num_oponents < 1) num_oponents = 1;
  if (num_oponents > MAX_OPONENTS) num_oponents = MAX_OPONENTS;

  /* Processar variabilidad */
  varia = atoi(ll_args[2]);
  if (varia < 0) varia = 0;
  if (varia > 3) varia = 3;

  retard = 100;  /* valor per defecte */

  /* Procesar retards (si hi ha) */
  if (n_args == 6) {
    RET_MIN = atoi(ll_args[4]);
    RET_MAX = atoi(ll_args[5]);
    if (RET_MIN < 10) RET_MIN = 10;
    if (RET_MAX > 1000) RET_MAX = 1000;
    if (RET_MIN > RET_MAX) RET_MAX = RET_MIN;
    retard = RET_MIN;  /* usar el retardo mínimo para el usuario */
  }

  /* Guardar nombre del archivo en variable global */
  nomArxiu = ll_args[3];

  /* Abrir archivo de salida */
  arxiuSortida = fopen(nomArxiu, "w");
  if (!arxiuSortida) {
    fprintf(stderr,"Error: no s'ha pogut obrir el fitxer %s\n", nomArxiu);
    exit(5);
  }
  setbuf(arxiuSortida, NULL);  // Deshabilitar el buffer
  fprintf(arxiuSortida, ">>> INICI DEL JOC (PID pare: %d) - %d oponents <<<\n", 
          getpid(), num_oponents);
  fflush(arxiuSortida);  // Això ho vam afegir perque vam trobar un problema amb l'escriptura  
  fclose(arxiuSortida);  // Cerrar archivo para que los hijos lo puedan abrir

  printf("Joc del Tron\n\tTecles: \'%c\', \'%c\', \'%c\', \'%c\', RETURN-> sortir\n",
		TEC_AMUNT, TEC_AVALL, TEC_DRETA, TEC_ESQUER);
  printf("prem una tecla per continuar:\n");
  getchar();

  n_fil = 0; n_col = 0;
  int mida_camp = win_ini(&n_fil, &n_col, '+', INVERS);  /* obtener tamaño necesario */
  if (mida_camp < 0) {
    fprintf(stderr,"Error en la creacio del taulell de joc:\t");
    switch (mida_camp)
    {	case -1: fprintf(stderr,"camp de joc ja creat!\n"); break;
	case -2: fprintf(stderr,"no s'ha pogut inicialitzar l'entorn de curses!\n"); break;
	case -3: fprintf(stderr,"les mides del camp demanades son massa grans!\n"); break;
	case -4: fprintf(stderr,"no s'ha pogut crear la finestra!\n"); break;
     }
     exit(2);
  }

  /* Crear zona de memoria compartida para el campo de juego */
  id_pantalla = ini_mem(mida_camp);
  p_pantalla = map_mem(id_pantalla);

  /* Inicializar acceso a la ventana */
  win_set(p_pantalla, n_fil, n_col);

  /* Hi había un bug, que no tenía a veure amb el programa
  Aquest feia que cada x execucions apareguessin valors aleatoris
  a la terminal. S'ha solucionat amb un bucle després d'iniciar que neteja
  tot el que es pugui haver creat. (Respectant els limits "+") */
  for(i = 1; i < n_fil-2; i++) {
    for(int j = 1; j < n_col-1; j++) {
      win_escricar(i, j, ' ', NO_INV);
    }
  }
  win_update();

  p_usu = calloc(n_fil*n_col/2, sizeof(pos));	/* demana memoria dinamica */
  /* Reservar memoria per les posicions dels oponents */
  p_opo = calloc(num_oponents, sizeof(pos*));     /* vector de punters */
  n_opo = calloc(num_oponents, sizeof(int));      /* vector de contadors */
  for(i = 0; i < num_oponents; i++) {
    p_opo[i] = calloc(n_fil*n_col/2, sizeof(pos)); /* espai per cada oponent */
    n_opo[i] = 0;                                  /* inicialitzar contador */
  }

  if (!p_usu || !p_opo || !n_opo)	/* si no hi ha prou memoria */
  { 
    win_fi();
    if (p_usu) free(p_usu);
    if (p_opo) {
      for(i = 0; i < num_oponents; i++) 
        if(p_opo[i]) free(p_opo[i]);
      free(p_opo);
    }
    if (n_opo) free(n_opo);
    fprintf(stderr,"Error en alocatacion de memoria dinamica.\n");
    exit(3);
  }
			/* Fins aqui tot ha anat be! */
  inicialitza_joc();

  /* Crear zones de memoria compartida */
  id_fi1 = ini_mem(sizeof(int));    /* Zona memoria fi1 */
  id_fi2 = ini_mem(sizeof(int));    /* Zona memoria fi2 */
  id_vius = ini_mem(sizeof(int));   /* Zona memoria comptador vius */
    
  /* Obtenir adreces de memoria compartida */
  p_fi1 = (int *)map_mem(id_fi1);  
  p_fi2 = (int *)map_mem(id_fi2);
  p_vius = (int *)map_mem(id_vius);
    
  /* Inicialitzar variables compartides */
  *p_fi1 = 0;
  *p_fi2 = 0;
  *p_vius = num_oponents;  /* Inicialitzem amb el nombre total d'oponents */

  /* (Fase 5) Hem de crear la zona (mailbox) on afegir els missatges (del tipus cua) */
  id_missatges = ini_mis();
  if (id_missatges < 0) {
    fprintf(stderr,"Error al crear la cua de missatges\n");
    exit(6);
  }

  /* Crear thread usuari */
  if (pthread_create(&threadUsuari, NULL, mou_usuari, NULL) != 0) {
    perror("Error en crear el thread");
    exit(1);
  }

  /* Creació dels processos fills (un per cada oponent) */
  for (i = 0; i < num_oponents; i++) {
    id_proc[i] = fork();
    if (id_proc[i] == (pid_t) 0) {      /* Codi del procés fill */
      char params[25][60];
      
      /* Convertir todos los parámetros a strings */
      sprintf(params[0], "oponent5");    /* nombre del programa */
      sprintf(params[1], "%s", ll_args[3]);     /* arxiuSortida path */
      sprintf(params[2], "%d", id_fi1);
      sprintf(params[3], "%d", id_fi2);
      sprintf(params[4], "%d", id_pantalla);    /* ID memoria compartida campo */
      sprintf(params[5], "%d", id_vius);
      sprintf(params[6], "%d", RET_MIN);
      sprintf(params[7], "%d", RET_MAX);
      sprintf(params[8], "%d", n_fil);     
      sprintf(params[9], "%d", n_col);     
      sprintf(params[10], "%d", varia);    
      sprintf(params[11], "%d", retard);
      sprintf(params[12], "%d", i);        /* índice del oponente */
      sprintf(params[13], "%d", opo[i].f); /* posición f inicial */
      sprintf(params[14], "%d", opo[i].c); /* posición c inicial */
      sprintf(params[15], "%d", opo[i].d); /* dirección inicial */
      sprintf(params[16], "%d", opo[i].f); /* posición inicial f */
      sprintf(params[17], "%d", opo[i].c); /* posición inicial c */
      sprintf(params[18], "%d", id_missatges);   /* ID del buzón de mensajes */

      execlp("./oponent5", params[0], params[1], params[2], params[3], 
             params[4], params[5], params[6], params[7], params[8], 
             params[9], params[10], params[11], params[12], params[13], 
             params[14], params[15], params[16], params[17], params[18], (char *)0);

      fprintf(stderr,"Error en la execució del proces fill %d.\n", i);
      exit(4);
    }
    else if (id_proc[i] < 0) {  /* Error en la creació del procés */
      win_fi();
      fprintf(stderr,"Error en la creació del procés fill %d.\n", i);
      exit(4);
    }
  }

  /* Bucle principal del joc */
  char strin[45];
  time_t momentInici = time(NULL);
  time_t ultimaActualitzacio = momentInici;
  
  while (!(*p_fi1) && !(*p_fi2)) {
    time_t tempsActual = time(NULL);
    
    // Actualitem el rellotge només quan passa un segón
    if (tempsActual > ultimaActualitzacio) {
      int tempsJugat = tempsActual - momentInici;
      sprintf(strin,"Temps: %02d:%02d", tempsJugat/60, tempsJugat%60);
      win_escristr(strin);
      win_update();
      ultimaActualitzacio = tempsActual;
    }
    
    usleep(10000);  // Pausa de 10 ms
    win_update();
  }

  /* Espera threads y procesos */
  pthread_join(threadUsuari, NULL);
  for (i = 0; i < num_oponents; i++) {
    waitpid(id_proc[i], NULL, 0);
  }

  win_fi();

  /* Mostrar temps total i missatge */
  int total = time(NULL) - momentInici;
  
  if (*p_fi1 == -1) {
    printf("\nS'ha aturat el joc amb tecla RETURN!\n");
  } else { 
    if (*p_fi2) {
      printf("\nHa guanyat l'usuari!\n");
    } else {
      printf("\nHa guanyat l'ordinador!\n");
    }
  }
  printf("Temps total: %02d:%02d\n\n", total/60, total%60);

  /* Liberar recursos */
  free(p_usu);
  for(i = 0; i < num_oponents; i++) {
    if(p_opo[i]) free(p_opo[i]);
  }
  free(p_opo);
  free(n_opo);

  elim_mem(id_fi1);
  elim_mem(id_fi2);
  elim_mem(id_vius);
  elim_mem(id_pantalla);

  /* Escribir mensaje final en archivo */
  arxiuSortida = fopen(nomArxiu, "a");
  if (*p_fi1 == -1) {
    fprintf(arxiuSortida, "Joc aturat manualment per l'usuari (PID: %d)\n", getpid());
  } else { 
    if (*p_fi2) {
      fprintf(arxiuSortida, "Fi del joc: ha guanyat l'usuari (PID: %d) - L'oponent ha quedat atrapat\n", getpid());
    } else {
      fprintf(arxiuSortida, "Fi del joc: ha guanyat l'ordinador (PID: %d) - L'usuari ha xocat\n", getpid());
    }
  }
  fprintf(arxiuSortida, ">>> FINAL DEL JOC <<<\n");
  fflush(arxiuSortida);
  fclose(arxiuSortida);

  /* (Fase 5) Només queda lliberar els recursos */
  pthread_mutex_destroy(&mutexTrajecte);
  elim_mis(id_missatges);

  return(0);
}