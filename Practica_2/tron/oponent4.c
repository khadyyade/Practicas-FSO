/*****************************************************************************/
/*									     */
/*				     oponent3.c				     */
/*									     */
/*	   $ gcc -c winsuport2.c -o winsuport2.o			     	     */
/*	   $ gcc tron3.c winsuport2.o memoria.o semafor.o -o tron3 -lcurses -lpthread			 */
/*     $ gcc oponent3.c winsuport2.o memoria.o semafor.o -o oponent3 -lcurses                */
/*	   $ ./tron3 num_oponents variabilitat fitxer [retard_min retard_max]				     */
/*									     */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include "winsuport2.h"
#include "memoria.h"
#include "semafor.h"
#include <time.h>

#define MAX_OPONENTS 9

/* Funciones per la taula compartida */
// (Fase3) Hay que definirlas tambien en oponent3
/* char win_quincar(int f, int c);
void win_escricar(int f, int c, char car); */

/* Variable global pel fitxer de sortida */
// (Fase3) Rebem el punter de tron3
FILE *arxiuSortida = NULL;

/* Variables per memoria compartida */
// (Fase3) Les rebem totes de tron3
int id_fi1, id_fi2;         /* IDs zona memoria compartida pels flags fi1/fi2 */
int *p_fi1, *p_fi2;         /* Punters a les zones de memoria compartida */
int id_pantalla;            /* ID zona memoria compartida per la pantalla */
void *p_pantalla;           /* Punter memoria compartida pantalla */
int id_vius;               /* ID zona memoria compartida pel contador d'oponents vius */
int *p_vius;               /* Punter a la zona de memoria compartida pel contador */

/* definir estructures d'informacio per oponent*/
typedef struct {		/* per un tron (oponent) */
	int f;				/* posicio actual: fila */
	int c;				/* posicio actual: columna */
	int d;				/* direccio actual: [0..3] */
} tron;

typedef struct {		/* per una entrada de la taula de posicio */
	int f;
	int c;
} pos;

// (Fase3)

tron opo[MAX_OPONENTS];


// (Fase3) Tot i que s'utilitzen en els oponents les controla el programa principal 
// Poden ser modificades per paràmetre al executar tron3
// Les rebrem com paràmetres (ja que no canvien)

int RET_MIN;     /* valor mínim del retard */
int RET_MAX;    /* valor màxim del retard */

// (Fase3) Definits a tron3 però les utilitzem a oponent3 (no canvien)
// Les rebem com a paràmetre
int n_fil, n_col;		/* dimensions del camp de joc */

// (Fase3) En tron3.c i en oponent3.c
int df[] = {-1, 0, 1, 0};	/* moviments de les 4 direccions possibles */
int dc[] = {0, -1, 0, 1};	/* dalt, esquerra, baix, dreta */

// (Fase3) Ens les passen desde tron3
int varia;		/* valor de variabilitat dels oponents [0..9] */
int retard;		/* valor del retard de moviment, en mil.lisegons */

// (Fase3) Només necessitem guardar el rastre d'un sol oponent (recuperm el funcionament de tron0)

pos **p_opo;			/* vector de taules de posicions dels oponents */
int *n_opo;            /* vector del numero d'entrades per cada oponent */


// (Fase 3) A tron3 i oponent3 es necessaria
/* funcio per esborrar totes les posicions anteriors de l'oponent */
void esborrar_posicions(pos p_pos[], int n_pos)
{
  int i;
  
  for (i=n_pos-1; i>=0; i--)		/* de l'ultima cap a la primera */
  {
    win_escricar(p_pos[i].f,p_pos[i].c,' ',NO_INV);	/* esborra una pos. */
    win_retard(6);
    win_update();  // Actualizar después de cada borrado
  }
}

// (Fase3) Aquesta funció la necesitem als dos costats
/* char win_quincar(int f, int c)
{
  char caracter = win_quincar(f,c); // Primer llegim el valor que hauria d'estar escrit en la pantalla
  if (caracter == ' '){ // Si la posicio està buida no estem 100 % segurs de que la pantalla hagi retornat el resutat correcte
    if (f >= 0 && f < n_fil && c >= 0 && c < n_col) { // Si la posicio està dins dels limits del tablero
      return ((char *)p_pantalla)[f * n_col + c]; // Llegim el valor real de la pantalla que s'ha guardat desde memòria compartida
    }
  }
  return caracter; // Si el valor de terminal es diferent de un espai segur que no s'equivoca
} */

// (Fase3) Aquesta funció la necesitem als dos costats
/* void win_escricar(int f, int c, char car)
{
  if (f >= 0 && f < n_fil && c >= 0 && c < n_col) { 
    ((char *)p_pantalla)[f * n_col + c] = car; // Escribim en memòria compartida
    // Si vamos a escribir un espacio (solo pasará al borrar el rastro de un oponente muerto cuando aun quedan mas)
    if (car == ' ') {
      win_escricar(f, c, car, NO_INV);  // Espais sense atribut INVERS (color del fons)
    } else {
      win_escricar(f, c, car, INVERS);  // Resta de caracters amb INVERS (color del fons)
    }
  }
} */

// (Fase3) No fa falta la funció d'inicialitza_joc

// (Fase3) La funció mou_usuari ara només es troba a tron3.c

/////////////////////////////////

// (Fase3) La funció mou_oponent ara s'executa al main d'oponent3.c

int main(int n_args, char *ll_args[])
{
  if (n_args != 16) {
    fprintf(stderr,"Error: numero de parametres incorrecte.\n");
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

  /* Reservar memoria para el rastro */
  p_opo = calloc(MAX_OPONENTS, sizeof(pos *));
  n_opo = calloc(MAX_OPONENTS, sizeof(int));
  p_opo[0] = calloc(n_fil*n_col/2, sizeof(pos));
  n_opo[index] = 0;  // Inicializamos el contador del oponente actual

  if (!p_opo || !n_opo || !p_opo[0]) {
    fprintf(stderr,"Error: no s'ha pogut reservar memoria.\n");
    exit(3);
  }

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
          
          esborrar_posicions(p_opo[0], n_opo[index]);  /* esborrar aquest oponent */

          (*p_vius)--;   /* Decrementem el comptador d'oponents vius */
          if (*p_vius <= 0) {  /* Si no queden oponents vius */
              *p_fi2 = 1;
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
    if (!(*p_fi2))
    {
      opo[index].f = opo[index].f + df[opo[index].d];
      opo[index].c = opo[index].c + dc[opo[index].d];

      win_escricar(opo[index].f,opo[index].c,'1'+index,INVERS);  /* win_escricar ya está sincronizado */

      p_opo[0][n_opo[index]].f = opo[index].f;  // Usar [0] en lugar de [index]
      p_opo[0][n_opo[index]].c = opo[index].c;
      n_opo[index]++;

      if (arxiuSortida)
        fprintf(arxiuSortida, "L'oponent %d (PID: %d) s'ha mogut a (%d,%d)\n",
                index, getpid(), opo[index].f, opo[index].c);
        fflush(arxiuSortida);  // Forzar escritura
    }
    else esborrar_posicions(p_opo[0], n_opo[index]);  // Usar [0] en lugar de [index]

    /* Retard aleatori */
    ret_aleatori = RET_MIN + rand() % (RET_MAX - RET_MIN + 1);
    win_retard(ret_aleatori);
  }

  /* Liberar recursos */
  free(p_opo[0]);
  free(p_opo);
  free(n_opo);
  fclose(arxiuSortida);

  return 0;
}