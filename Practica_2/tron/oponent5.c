/*****************************************************************************/
/*									     */
/*				     oponent5.c				     */
/*									     */
/*	   $ gcc -c winsuport2.c -o winsuport2.o			     	     */
/*	   $ gcc tron5.c winsuport2.o memoria.o missatge.o -o tron5 -lcurses -lpthread			 */
/*     $ gcc oponent5.c winsuport2.o memoria.o missatge.o -o oponent5 -lcurses -lpthread                */
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
#include "missatge.h"     // (Fase 5) Afegim la llibreria dels missatges
#include <pthread.h>      // (Fase 5) També afegim la llibreria ja que algunes funcions seran threads
#include <time.h>

#define MAX_OPONENTS 9

/* Variable pel fitxer de sortida */
FILE *arxiuSortida = NULL;

/* Variables per memoria compartida */
int id_fi1, id_fi2;         /* IDs zona memoria compartida pels flags fi1/fi2 */
int *p_fi1, *p_fi2;         /* Punters a les zones de memoria compartida */
int id_pantalla;            /* ID zona memoria compartida per la pantalla */
void *p_pantalla;           /* Punter memoria compartida pantalla */
int id_vius;                /* ID zona memoria compartida pel contador d'oponents vius */
int *p_vius;                /* Punter a la zona de memoria compartida pel contador */
int id_missatges;           // (Fase 5) Zona de memòria pels missatges

/* definir estructures d'informacio per oponent */
typedef struct {
	int f;				    /* fila     */
	int c;				    /* columna  */
	int d;				    /* direccio */
} tron;

typedef struct {		/* per una entrada de la taula de posicio */
	int f;
	int c;
} pos;

tron opo[MAX_OPONENTS];

int RET_MIN;     /* valor mínim del retard */
int RET_MAX;    /* valor màxim del retard */

int n_fil, n_col;		/* dimensions del camp de joc */

int df[] = {-1, 0, 1, 0};	/* moviments de les 4 direccions possibles */
int dc[] = {0, -1, 0, 1};	/* dalt, esquerra, baix, dreta */

int varia;		/* valor de variabilitat dels oponents [0..9] */
int retard;		/* valor del retard de moviment, en mil.lisegons */

pos **p_opo;			/* vector de taules de posicions dels oponents */
int *n_opo;            /* vector del numero d'entrades per cada oponent */

/* Variables para la posición inicial */
int pos_ini_f;
int pos_ini_c;

// (Fase 5) Variable per guardar la mida actual del trajecte de cada oponent
int mida_trajecte = 0;

/* funcio per esborrar totes les posicions anteriors de l'oponent */
void esborrar_posicions(pos p_pos[], int n_pos)
{
  int i;
  
  for (i=n_pos-1; i>=0; i--)		/* de l'ultima cap a la primera */
  {
    win_escricar(p_pos[i].f,p_pos[i].c,' ',NO_INV);	/* esborra una pos. */
    win_retard(6);
    win_update();  // Actualitzar pantalla
  }
}

// (Fase 5) Definim els diferents threads que necesitarem

pthread_t thread_receptor;    // (Fase 5) Thread que fa la funcio *rebre_missatges()
pthread_t thread_tipus;       // (Fase 5) Thread per determinar el tipus d'oponent
pthread_t threadEnrere;    // (Fase 5) Thread per calcular el moviment

// (Fase 5) Creem el mutex (semàfor binari) per evitar que dos threads accedeixin a mem compartida

pthread_mutex_t mutex_trajecte = PTHREAD_MUTEX_INITIALIZER;

// (Fase 5) Definim la nova funció
void *modifica_trajecte(void *arg);

// (Fase 5) struct de dades per les col·lisions
typedef struct {
  int f;          /* fila */
  int c;          /* columna */
  int oponent;    /* index de l'oponent que ha xocat */
} dadesColisions;

// (Fase 5) struct de dades per fer les modificacions
typedef struct {
    int filaInicial;        /* fila inicial */
    int columnaInicial;     /* columna inicial */
    int tipus;              /* el thread 1 segueix avançant, el 0 retrocedeix */
    int index;              /* index oponent */
} dadesThreads;

// (Fase 5) Funció (thread) que revisa fins que acabi el joc si hi ha nous missatges i crea els dos nous threads (un un segueix el rastre cap endevant, l'altre cap a enrere)
void *rebre_missatges(void *arg) {
    dadesColisions col_data;
    dadesThreads dadesThreadEnrere;     // Estructura pel thread que retrocedeix
    dadesThreads dadesThreadEndavant;   // Estructura pel thread que avança
    pthread_t threadEnrere;             // Thread per seguir el rastre cap a enrere ('O')
    pthread_t threadEndavant;           // Thread per seguir el rastre cap a endevant ('X')
    int index = *((int*)arg);
    
    while (!(*p_fi1) && !(*p_fi2)) {
        if (*p_fi1 || *p_fi2) {
            pthread_exit(NULL);  // (Fase 5) Hem de foprçar aquesta condició per a que el thread no sigui infinit
        }
        if (receiveM(id_missatges, &col_data) > 0) {
            if (*p_fi1 || *p_fi2) return NULL;  // (Fase 5) Afegim més controls per parar el thread quan el joc acabi (vam tenir problemes amb la finalització del thread)
            if (col_data.oponent == index) {
                // (Fase 5) Afegim dades pel thread que retrocedeix
                dadesThreadEnrere.filaInicial = col_data.f;
                dadesThreadEnrere.columnaInicial = col_data.c;
                dadesThreadEnrere.index = index;
                dadesThreadEnrere.tipus = 1;

                // (Fase 5) Afegim dades pel thread que avança
                dadesThreadEndavant.filaInicial = col_data.f;
                dadesThreadEndavant.columnaInicial = col_data.c;
                dadesThreadEndavant.index = index;
                dadesThreadEndavant.tipus = 0;

                // (Fase 5) Creem els threads
                pthread_create(&threadEndavant, NULL, modifica_trajecte, &dadesThreadEnrere);
                pthread_create(&threadEnrere, NULL, modifica_trajecte, &dadesThreadEndavant);
                
                // (Fase 5) Esperem a que acabin
                pthread_join(threadEndavant, NULL);
                pthread_join(threadEnrere, NULL);
            }
        }
    }
    return NULL;
}

// (Fase 5) Nova funció pels dos threads que seguirán el rastre (ja sigui endavant o enrere)
void *modifica_trajecte(void *arg) {
    dadesThreads *dades = (dadesThreads*)arg; // Recuperem les dades que es pasen per paràmetre
    int i;
    int punt_colisio = -1;  // Variable per buscar el index del vector p_opo on hi ha la col·lisió

    // Buscar el punto de colisión
    pthread_mutex_lock(&mutex_trajecte); // (Fase 5) S'ha de fer un lock per evitar que hi hagin conflictes
    
    // (Fase 5) Busquem entre tots els elements que te el vector p_opo si hi ha la coordenada de col·lisió

    for (i = 0; i < mida_trajecte; i++) {
        if (p_opo[0][i].f == dades->filaInicial && 
            p_opo[0][i].c == dades->columnaInicial) {
            punt_colisio = i;
            break;
        }
    }

    // (Fase 5) Si no hem trobat la col·lisió alliberem el lock i sortim
    if (punt_colisio == -1) {
        pthread_mutex_unlock(&mutex_trajecte);
        return NULL;
    }
    // (Fase 5) Ara mirem si estem en el thread que avança o en el thread que retrocedeix
    if (dades->tipus == 0) {
        // (Fase 5) Si estem en el thread que retrocedeix escribim 'O' fins l'inici del rastre
        for (i = punt_colisio; i >= 0; i--) {
            win_escricar(p_opo[0][i].f, p_opo[0][i].c, 'O', INVERS);
            win_retard(10);
            win_update();
        }
        // Pintar la posició inicial
        win_escricar(pos_ini_f, pos_ini_c, 'O', INVERS);
        win_update();
    } else {
        // (Fase 5) Si estem en el thread que avança escribim 'X' fins al final del rastre
        for (i = punt_colisio + 1; i < mida_trajecte; i++) {
            win_escricar(p_opo[0][i].f, p_opo[0][i].c, 'X', INVERS);
            win_retard(10);
            win_update();
        }
    }
    // (Fase 5) Alliberem el lock
    pthread_mutex_unlock(&mutex_trajecte);
    return NULL;
}

int main(int n_args, char *ll_args[])
{
  if (n_args != 19) {
    exit(1);
  }

  /* Obrir arxiu */
  arxiuSortida = fopen(ll_args[1], "a");  // Abrir en modo append
  if (!arxiuSortida) exit(2);
  setbuf(arxiuSortida, NULL);  // Deshabilitar el buffer

  /* Recuperar IDs de memoria compartida */
  id_fi1 = atoi(ll_args[2]);
  id_fi2 = atoi(ll_args[3]);
  id_pantalla = atoi(ll_args[4]);
  id_vius = atoi(ll_args[5]);
  id_missatges = atoi(ll_args[18]); // (Fase 5) Nou paràmetre per la cua de missatges

  /* Mapear la memoria compartida */
  p_fi1 = (int *)map_mem(id_fi1);
  p_fi2 = (int *)map_mem(id_fi2);
  p_pantalla = map_mem(id_pantalla);
  p_vius = (int *)map_mem(id_vius);

  /* Recuperar dimensiones y variables */
  n_fil = atoi(ll_args[8]);
  n_col = atoi(ll_args[9]);

  /* Inicializar acceso a la ventana compartida */
  win_set(p_pantalla, n_fil, n_col);

  /* Otras variables */
  RET_MIN = atoi(ll_args[6]);
  RET_MAX = atoi(ll_args[7]);
  varia = atoi(ll_args[10]);
  retard = atoi(ll_args[11]);
  int index = atoi(ll_args[12]);

  /* Posición inicial del oponente */
  opo[index].f = atoi(ll_args[13]);
  opo[index].c = atoi(ll_args[14]);
  opo[index].d = atoi(ll_args[15]);

  /* Guardar posición inicial */
  pos_ini_f = atoi(ll_args[16]);
  pos_ini_c = atoi(ll_args[17]);

  /* Reservar memoria para el rastro */
  p_opo = calloc(MAX_OPONENTS, sizeof(pos *));
  n_opo = calloc(MAX_OPONENTS, sizeof(int));
  p_opo[0] = calloc(n_fil*n_col/2, sizeof(pos));
  n_opo[index] = 0;  // Inicializamos el contador del oponente actual

  if (!p_opo || !n_opo || !p_opo[0]) {
    fprintf(stderr,"Error: no s'ha pogut reservar memoria.\n");
    exit(3);
  }

  // (Fase 5) Aquest thread l'hem de crear sempre, es la funció que rep el missatges
  pthread_create(&thread_receptor, NULL, rebre_missatges, &index);

  /* Ejecutar movimiento del oponente */
  char cars;
  tron seg;
  int k, vk, nd, vd[3];
  int canvi = 0;
  int ret_aleatori;
 
  srand(getpid() + time(NULL) + index); /* Diferent llavor per cada procés */

  while (!(*p_fi1) && !(*p_fi2))
  {
    seg.f = opo[index].f + df[opo[index].d];
    seg.c = opo[index].c + dc[opo[index].d];

    cars = win_quincar(seg.f,seg.c);  /* win_quincar ya está sincronizado */

    if (cars != ' ')
       canvi = 1;
    else if (varia > 0)
    { k = rand() % 10;
      if (k < varia) canvi = 1;
    }
    
    if (canvi){
      nd = 0;
      for (k=-1; k<=1	; k++)	/* provar direccio actual i dir. veines */
      {
          vk = (opo[index].d + k)%4;
          if (vk < 0) vk += 4;
          seg.f = opo[index].f + df[vk];
          seg.c = opo[index].c + dc[vk];

          cars = win_quincar(seg.f,seg.c); /* win_quincar ya está sincronizado */

          if (cars == ' ')
          { vd[nd] = vk;			/* memoritza com a direccio possible */
            nd++;				/* anota una direccio possible mes */
          }
      }
      if (nd == 0) {			/* si no pot continuar, */
          
          esborrar_posicions(p_opo[0], mida_trajecte);  /* esborrar aquest oponent */
          win_escricar(pos_ini_f, pos_ini_c, ' ', NO_INV); /* borrar posición inicial */
          win_update();

          (*p_vius)--;   /* Decrementem el comptador d'oponents vius */
          if (*p_vius <= 0) {  /* Si no queden oponents vius */
              *p_fi2 = 1;
              win_update();
          }

          fprintf(arxiuSortida, "L'oponent %d (PID: %d) ha xocat. Queden %d oponents vius\n", 
                  index, getpid(), *p_vius);
          fflush(arxiuSortida);  // Forzar escritura
          
          break;  /* Sortim del bucle per acabar aquest oponent */
      }
      else
      { if (nd == 1)
          opo[index].d = vd[0];
        else
          opo[index].d = vd[rand() % nd];
      }
    }
    if (*p_fi1 || *p_fi2) {
        break;  // (Fase 5) Mes controls, potser inncesaris, per controlar la correcta finalització del joc
    }
    if (!(*p_fi2))
    {
      opo[index].f = opo[index].f + df[opo[index].d];
      opo[index].c = opo[index].c + dc[opo[index].d];

      win_escricar(opo[index].f,opo[index].c,'1'+index,INVERS);

      // (Fase 5) Controlem la nova variable que controla la mida del trajecte
      p_opo[0][mida_trajecte].f = opo[index].f;
      p_opo[0][mida_trajecte].c = opo[index].c;
      mida_trajecte++;  // Incrementem
      n_opo[index] = mida_trajecte;

      if (arxiuSortida)
        fprintf(arxiuSortida, "L'oponent %d (PID: %d) s'ha mogut a (%d,%d)\n",
                index, getpid(), opo[index].f, opo[index].c);
        fflush(arxiuSortida);  // Forzar escritura
    }
    else {
      esborrar_posicions(p_opo[0], mida_trajecte);  // Usamos mida_trajecte en lugar de n_opo[index]
      mida_trajecte = 0;
    }

    /* Retard aleatori */
    ret_aleatori = RET_MIN + rand() % (RET_MAX - RET_MIN + 1);
    win_retard(ret_aleatori);
  }

  /* Asegurarse de que la pantalla está actualizada antes de salir */
  win_update();

  // (Fase 5) Esperem que acabi la funció de rebre missatges
  if (thread_receptor) {
      pthread_cancel(thread_receptor);  // (Fase 5) Forçar que no quedi cap thread en execució
      pthread_join(thread_receptor, NULL);
  }

  /* Liberar recursos */
  free(p_opo[0]);
  free(p_opo);
  free(n_opo);
  if (arxiuSortida) {
      fclose(arxiuSortida);
  }

  // (Fase 5) Destruïm el mutex (semàfor binari)
  pthread_mutex_destroy(&mutex_trajecte);

  exit(0);  // (Fase 5) Forçar sortida
}