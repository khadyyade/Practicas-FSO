/*****************************************************************************/
/*									     */
/*				     tron3.c				     */
/*									     */
/*	   $ gcc -c winsuport2.c -o winsuport2.o			     	     */
/*	   $ gcc tron3.c winsuport2.o memoria.o -o tron3 -lcurses -lpthread			 */
/*     $ gcc oponent3.c winsuport2.o memoria.o -o oponent3 -lcurses                */
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
#include <time.h>

/* Funciones per la taula compartida */
// (Fase3) Hay que definirlas tambien en oponent3
// char win_quincar(int f, int c);
// void win_escricar(int f, int c, char car);

/* Variable global pel fitxer de sortida */
// (Fase3) Hay que pasar el puntero a oponent3
FILE *arxiuSortida = NULL;

/* Variable global para el nombre del archivo */
const char *nomArxiu;   // Añadir esta variable global

/* Variables per memoria compartida */
// (Fase3) Hay que pasar todos los punteros a oponent3
int id_fi1, id_fi2;         /* IDs zona memoria compartida pels flags fi1/fi2 */
int *p_fi1, *p_fi2;         /* Punters a les zones de memoria compartida */
int id_pantalla;            /* ID zona memoria compartida per la pantalla */
void *p_pantalla;           /* Punter memoria compartida pantalla */
int id_vius;               /* ID zona memoria compartida pel contador d'oponents vius */
int *p_vius;               /* Punter a la zona de memoria compartida pel contador */

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

/* Constant limit només en tron3 */
#define MAX_OPONENTS 9

// (Fase3) Tot i que s'utilitzen en els oponents les controla el programa principal 
// Poden ser modificades per paràmetre al executar tron3
// Les pasarem com paràmetres a oponent3 (ja que no canvien)

int RET_MIN = 50;     /* valor mínim del retard */
int RET_MAX = 500;    /* valor màxim del retard */

/* Variables per semàfors - ELIMINAR */
/* int semPantalla;
int semArxiu; 
int semFinal; */

// (Fase3) Definits a tron3 però s'utilitzen a oponent3 (no canvien)
// Les pasem com a paràmetre
int n_fil, n_col;		/* dimensions del camp de joc */

// (Fase3) Només en tron3
tron usu;   	   		/* informacio de l'usuari */

// (Fase3) Es crea a tron3 i es passa a oponent3
// Cada oponente accede al suyo

tron opo[MAX_OPONENTS];		/* Paso 1.5: array de tots els oponents */

// (Fase3) Només en tron3

int n_opo_actius = 0;        /* Paso 1.5: numero d'oponents actius */

// (Fase3) En tron3 i en oponent3
int df[] = {-1, 0, 1, 0};	/* moviments de les 4 direccions possibles */
int dc[] = {0, -1, 0, 1};	/* dalt, esquerra, baix, dreta */

// (Fase 3) Les rebem per paràmetre en tron3
int varia;		/* valor de variabilitat dels oponents [0..9] */
int retard;		/* valor del retard de moviment, en mil.lisegons */

// (Fase3) Per l'usuari només la necesitem a tron3 

pos *p_usu;			/* taula de posicions que va recorrent l'usuari */
int n_usu = 0;         /* numero d'entrades en la taula de pos. de l'usuari */

// (Fase3) Per l'oponent les necesitem per inicialitzar i oponent3 editar

pos **p_opo;			/* vector de taules de posicions dels oponents */
int *n_opo;            /* vector del numero d'entrades per cada oponent */

// (Fase 3) Només a tron3
int num_oponents = 1;      /* per defecte, un oponent */

// (Fase 3) A tron3 i oponent3 es necessaria
/* funcio per esborrar totes les posicions anteriors de l'usuari */
void esborrar_posicions(pos p_pos[], int n_pos)
{
  int i;
  
  for (i=n_pos-1; i>=0; i--)		/* de l'ultima cap a la primera */
  {
    win_escricar(p_pos[i].f,p_pos[i].c,' ',NO_INV);	/* esborra una pos. */
    win_retard(10);		/* un petit retard per simular el joc real */
  }
}

/* funcio per inicialitar les variables i visualitzar l'estat inicial del joc */
// (Fase3) Només l'utilitzem a tron3
void inicialitza_joc(void)
{
  char strin[45];
  int i;
  int espaiat;  /* Paso 1.6: espai entre oponents */

  /* inicialitza usuari (no canvia) */
  usu.f = (n_fil-1)/2;
  usu.c = (n_col)/4;
  usu.d = 3;
  win_escricar(usu.f,usu.c,'0',INVERS);
  p_usu[n_usu].f = usu.f;
  p_usu[n_usu].c = usu.c;
  n_usu++;

  /* Paso 1.6: inicialitza els oponents */
  espaiat = (n_fil - 1) / (num_oponents + 1);  /* calcul de l'espaiat */
  for (i = 0; i < num_oponents; i++) {
    opo[i].f = espaiat * (i + 1);              /* distribució equidistant */
    opo[i].c = (n_col*3)/4;                    /* mateixa columna */
    opo[i].d = 1;                              /* direcció inicial: esquerra */
    win_escricar(opo[i].f,opo[i].c,'1'+i,INVERS);
    p_opo[i][n_opo[i]].f = opo[i].f;
    p_opo[i][n_opo[i]].c = opo[i].c;
    n_opo[i]++;
  }
  n_opo_actius = num_oponents;    /* Paso 1.6: actualitza comptador d'oponents */

  sprintf(strin,"Tecles: \'%c\', \'%c\', \'%c\', \'%c\', RETURN-> sortir\n",
          TEC_AMUNT, TEC_AVALL, TEC_DRETA, TEC_ESQUER);
  win_escristr(strin);
}

// (Fase3) La funció mou_oponent ara només es troba a oponent3.c

/////////////////////////////////

/* Funció pel moviment de l'usuari */
void mou_usuari(void)
{
  char cars;
  tron seg;
  int tecla;
  
  tecla = win_gettec();
  if (tecla != 0)
  switch (tecla)	/* modificar direccio usuari segons tecla */
  {
    case TEC_AMUNT:	usu.d = 0; break;
    case TEC_ESQUER:	usu.d = 1; break;
    case TEC_AVALL:	usu.d = 2; break;
    case TEC_DRETA:	usu.d = 3; break;
    case TEC_RETURN:	*p_fi1 = -1;
                      break;
  }
  
  seg.f = usu.f + df[usu.d];	/* calcular seguent posicio */
  seg.c = usu.c + dc[usu.d];

  cars = win_quincar(seg.f,seg.c);
  if (cars == ' ')			/* si seguent posicio lliure */
  {
    usu.f = seg.f; usu.c = seg.c;		/* actualitza posicio */
    win_escricar(usu.f,usu.c,'0',INVERS);	/* dibuixa bloc usuari */
    p_usu[n_usu].f = usu.f;		/* memoritza posicio actual */
    p_usu[n_usu].c = usu.c;
    n_usu++;
  }

  if (cars != ' ') {
    esborrar_posicions(p_usu, n_usu);
    *p_fi1 = 1;
    arxiuSortida = fopen(nomArxiu, "a");  // Usar la variable global
    fprintf(arxiuSortida, "L'usuari (PID: %d) ha xocat a: (%d,%d)\n",
            getpid(), seg.f, seg.c);
    fflush(arxiuSortida);  // Forzar escritura
    fclose(arxiuSortida);  // Cerrar inmediatamente
  }
}

/* programa principal */
int main(int n_args, const char *ll_args[])
{
  
  // (Fase3) Aquesta part del main es igual

  int retwin;
  int i, id_proc[MAX_OPONENTS];
  
  srand(getpid());

  /* Verificar arguments */
  if ((n_args != 4) && (n_args != 6))
  {
    fprintf(stderr,"Comanda: ./tron2 num_oponents variabilitat fitxer [retard_min retard_max]\n");
    fprintf(stderr,"         on num_oponents indica el nombre d'oponents (1-9)\n");
    fprintf(stderr,"         variabilitat indica la frequencia de canvi de direccio\n");
    fprintf(stderr,"         de l'oponent: de 0 a 3 (0- gens variable, 3- molt variable),\n");
    fprintf(stderr,"         fitxer: nom del fitxer on es guardaran els logs\n");
    fprintf(stderr,"         retard_min i retard_max: valors opcionals pels retards\n");
    exit(1);
  }

  /* Procesar número de oponentes */
  num_oponents = atoi(ll_args[1]);
  if (num_oponents < 1) num_oponents = 1;
  if (num_oponents > MAX_OPONENTS) num_oponents = MAX_OPONENTS;

  /* Procesar variabilidad */
  varia = atoi(ll_args[2]);
  if (varia < 0) varia = 0;
  if (varia > 3) varia = 3;

  /* Establecer retardo base antes de nada */
  retard = 100;  /* valor por defecto */

  /* Procesar retardos si se especifican */
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
  fflush(arxiuSortida);  // Forzar escritura inmediata
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

  /* Eliminar creación de semáforos */
  /* semPantalla = ini_sem(1);
  semArxiu = ini_sem(1);
  semFinal = ini_sem(1); */

  // (Fase3) Fins aquí tot igual
  //////////////////////////////

  /* Creació dels processos fills (un per cada oponent) */
  for (i = 0; i < num_oponents; i++) {
    id_proc[i] = fork();
    if (id_proc[i] == (pid_t) 0) {      /* Codi del procés fill */
      char params[15][30];  /* Array para convertir parámetros a strings */
      
      /* Convertir todos los parámetros a strings */
      sprintf(params[0], "oponent3");    /* nombre del programa */
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

      execlp("./oponent3", params[0], params[1], params[2], params[3], 
             params[4], params[5], params[6], params[7], params[8], 
             params[9], params[10], params[11], params[12], params[13], 
             params[14], params[15], (char *)0);

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
  do {
    mou_usuari();  // Moure l'usuari una posició
    sprintf(strin,"Vius: %d", *p_vius);  // Mostrar info en última línia
    win_escristr(strin);
    win_update();  // Actualitzar pantalla
    win_retard(retard);  // Usar el retard configurat
  } while (!(*p_fi1) && !(*p_fi2));

  /* Espera a que tots els fills acabin abans de finalitzar */
  for (i = 0; i < num_oponents; i++) {
    waitpid(id_proc[i], NULL, 0);
  }

  /////////////////////////

  // (Fase3) A partir d'aquí en teoría tampoc s'ha de modificar res mes

  win_fi();				/* tanca les curses */
  free(p_usu);
  free(p_opo);	  	 /* allibera la memoria dinamica obtinguda */

  /* Alliberar recursos */
  elim_mem(id_fi1);
  elim_mem(id_fi2);
  elim_mem(id_vius);    /* Alliberem la memoria del comptador */
  elim_mem(id_pantalla); /* Liberar la memoria del tablero */

  /* Eliminar liberación de semáforos */
  /* elim_sem(semPantalla);
  elim_sem(semArxiu);
  elim_sem(semFinal); */

  arxiuSortida = fopen(nomArxiu, "a");  // Reabrir para mensaje final
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

  return(0);
}