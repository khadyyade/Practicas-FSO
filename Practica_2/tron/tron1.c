/*****************************************************************************/
/*									     */
/*				     tron1.c				     */
/*									     */
/*	   $ gcc -c winsuport.c -o winsuport.o			     	     */
/*	   $ gcc tron1.c winsuport.o -o tron1 -lcurses			     */
/*	   $ $ ./tron1 num_oponents variabilitat fitxer [retard_min retard_max]				     */
/*									     */
/*  Codis de retorn:						  	     */
/*     El programa retorna algun dels seguents codis al SO:		     */
/*	0  ==>  funcionament normal					     */
/*	1  ==>  numero d'arguments incorrecte 				     */
/*	2  ==>  no s'ha pogut crear el camp de joc (no pot iniciar CURSES)   */
/*	3  ==>  no hi ha prou memoria per crear les estructures dinamiques   */
/*									     */
/*****************************************************************************/

#include <stdio.h>		/* incloure definicions de funcions estandard */
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/wait.h>
#include "winsuport.h"		/* incloure definicions de funcions propies */

/* Paso 1.8: variable global pel fitxer de sortida */
FILE *arxiuSortida = NULL;

				/* definir estructures d'informacio */
typedef struct {		/* per un tron (usuari o oponent) */
	int f;				/* posicio actual: fila */
	int c;				/* posicio actual: columna */
	int d;				/* direccio actual: [0..3] */
} tron;

typedef struct {		/* per una entrada de la taula de posicio */
	int f;
	int c;
} pos;

/* Paso 1.5: definició del límit màxim d'oponents */
#define MAX_OPONENTS 9

/* Paso 1.7: definició de constants pel retard aleatori */
int RET_MIN = 50;     /* valor mínim del retard */
int RET_MAX = 500;    /* valor màxim del retard */

/* variables globals */
int n_fil, n_col;		/* dimensions del camp de joc */

tron usu;   	   		/* informacio de l'usuari */
tron opo[MAX_OPONENTS];		/* Paso 1.5: array de tots els oponents */
int n_opo_actius = 0;        /* Paso 1.5: numero d'oponents actius */

int df[] = {-1, 0, 1, 0};	/* moviments de les 4 direccions possibles */
int dc[] = {0, -1, 0, 1};	/* dalt, esquerra, baix, dreta */

int varia;		/* valor de variabilitat dels oponents [0..9] */
int retard;		/* valor del retard de moviment, en mil.lisegons */
int retard_min, retard_max;	/*rang minim-maxim del retard de moviment d'un oponent*/

pos *p_usu;			/* taula de posicions que van recorrent */
pos *p_opo;			/* els jugadors */
int n_usu = 0, n_opo = 0;	/* numero d'entrades en les taules de pos. */

/* Variables globals pel control de finalització */
int fi1 = 0;    /* Variable pel proces pare (usuari): -1=RETURN, 1=xoc, 0=segueix */
int fi2 = 0;    /* Variable pels processos fill (oponents): 1=xoc, 0=segueix */

/* Paso 1.6: afegir variable global pel nombre d'oponents */
int num_oponents = 1;      /* per defecte, un oponent */

/* funcio per esborrar totes les posicions anteriors, sigui de l'usuari o */
/* de l'oponent */
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
    p_opo[n_opo].f = opo[i].f;
    p_opo[n_opo].c = opo[i].c;
    n_opo++;
  }
  n_opo_actius = num_oponents;    /* Paso 1.6: actualitza comptador d'oponents */

  sprintf(strin,"Tecles: \'%c\', \'%c\', \'%c\', \'%c\', RETURN-> sortir\n",
          TEC_AMUNT, TEC_AVALL, TEC_DRETA, TEC_ESQUER);
  win_escristr(strin);
}

/* Funció pel moviment dels oponents, executada com a procés independent.
 * Cada procés fill té la seva pròpia còpia de les variables globals,
 * cosa que pot causar problemes de sincronització */
void mou_oponent(int index)
{
  char cars;
  tron seg;
  int k, vk, nd, vd[3];
  int canvi = 0;
  int ret_aleatori;     /* Paso 1.7: variable pel retard aleatori */
 
  while (!fi1 && !fi2) /* Paso 1.2: bucle propi */
  {
    seg.f = opo[index].f + df[opo[index].d];	/* Paso 1.5: usa índex per accedir a l'oponent */
    seg.c = opo[index].c + dc[opo[index].d];
    cars = win_quincar(seg.f,seg.c);
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
          vk = (opo[index].d + k)%4;		/* Paso 1.5: usa índex */
          if (vk < 0) vk += 4;
          seg.f = opo[index].f + df[vk];
          seg.c = opo[index].c + dc[vk];
          cars = win_quincar(seg.f,seg.c);
          if (cars == ' ')
          { vd[nd] = vk;			/* memoritza com a direccio possible */
            nd++;				/* anota una direccio possible mes */
          }
      }
      if (nd == 0) {			/* si no pot continuar, */
          fi2 = 1;		/* Paso 1.2: actualitza variable global */
          fprintf(arxiuSortida, "L'oponent %d (PID: %d) ha xocat\n", 
                  index, getpid());
      }
      else
      { if (nd == 1)
          opo[index].d = vd[0];		/* Paso 1.5: usa índex */
        else
          opo[index].d = vd[rand() % nd];
      }
    }
    if (!fi2)		/* Paso 1.2: verifica si pot continuar */
    {
      opo[index].f = opo[index].f + df[opo[index].d];	/* Paso 1.5: usa índex */
      opo[index].c = opo[index].c + dc[opo[index].d];
      win_escricar(opo[index].f,opo[index].c,'1'+index,INVERS);
      p_opo[n_opo].f = opo[index].f;
      p_opo[n_opo].c = opo[index].c;
      n_opo++;
      /* Paso 1.8: registra moviment al fitxer */
      if (arxiuSortida)
        fprintf(arxiuSortida, "L'oponent %d (PID: %d) s'ha mogut a (%d,%d)\n",
                index, getpid(), opo[index].f, opo[index].c);
    }
    else esborrar_posicions(p_opo, n_opo);

    /* Paso 1.7: càlcul i aplicació del retard aleatori */
    ret_aleatori = RET_MIN + rand() % (RET_MAX - RET_MIN + 1);
    win_retard(ret_aleatori);
  }
}

/* Funció pel moviment de l'usuari, executada pel procés pare.
 * Al ser el procés principal, té accés directe a les variables globals
 * i pot modificar-les sense problemes de sincronització */
void mou_usuari(void)
{

  char cars;
  tron seg;
  int tecla;
  
  while (!fi1 && !fi2) /* Paso 1.3: bucle propi */
  {
    tecla = win_gettec();
    if (tecla != 0)
    switch (tecla)	/* modificar direccio usuari segons tecla */
    {
      case TEC_AMUNT:	usu.d = 0; break;
      case TEC_ESQUER:	usu.d = 1; break;
      case TEC_AVALL:	usu.d = 2; break;
      case TEC_DRETA:	usu.d = 3; break;
      case TEC_RETURN:	fi1 = -1; break; /* Paso 1.3: actualitza variable global */
    }
    
    seg.f = usu.f + df[usu.d];	/* calcular seguent posicio */
    seg.c = usu.c + dc[usu.d];
    cars = win_quincar(seg.f,seg.c);	/* calcular caracter seguent posicio */
    if (cars == ' ')			/* si seguent posicio lliure */
    {
      usu.f = seg.f; usu.c = seg.c;		/* actualitza posicio */
      win_escricar(usu.f,usu.c,'0',INVERS);	/* dibuixa bloc usuari */
      p_usu[n_usu].f = usu.f;		/* memoritza posicio actual */
      p_usu[n_usu].c = usu.c;
      n_usu++;
    }
    else
    { 
      esborrar_posicions(p_usu, n_usu);
      fi1 = 1; /* Paso 1.3: actualitza variable global */
      fprintf(arxiuSortida, "L'usuari (PID: %d) ha xocat a: (%d,%d)\n",
              getpid(), seg.f, seg.c);
    }
    win_retard(retard); /* Paso 1.3: afegeix retardo en el bucle */
  }
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
    fprintf(stderr,"Comanda: ./tron1 num_oponents variabilitat fitxer [retard_min retard_max]\n");
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

  /* Abrir archivo de salida */
  arxiuSortida = fopen(ll_args[3], "w");
  if (!arxiuSortida) {
    fprintf(stderr,"Error: no s'ha pogut obrir el fitxer %s\n", ll_args[3]);
    exit(5);
  }
  setbuf(arxiuSortida, NULL);
  fprintf(arxiuSortida, ">>> INICI DEL JOC (PID pare: %d) - %d oponents <<<\n", 
          getpid(), num_oponents);

  printf("Joc del Tron\n\tTecles: \'%c\', \'%c\', \'%c\', \'%c\', RETURN-> sortir\n",
		TEC_AMUNT, TEC_AVALL, TEC_DRETA, TEC_ESQUER);
  printf("prem una tecla per continuar:\n");
  getchar();

  n_fil = 0; n_col = 0;		/* demanarem dimensions de taulell maximes */
  retwin = win_ini(&n_fil,&n_col,'+',INVERS);	/* intenta crear taulell */

  if (retwin < 0)	/* si no pot crear l'entorn de joc amb les curses */
  { fprintf(stderr,"Error en la creacio del taulell de joc:\t");
    switch (retwin)
    {	case -1: fprintf(stderr,"camp de joc ja creat!\n"); break;
	case -2: fprintf(stderr,"no s'ha pogut inicialitzar l'entorn de curses!\n"); break;
	case -3: fprintf(stderr,"les mides del camp demanades son massa grans!\n"); break;
	case -4: fprintf(stderr,"no s'ha pogut crear la finestra!\n"); break;
     }
     exit(2);
  }

  p_usu = calloc(n_fil*n_col/2, sizeof(pos));	/* demana memoria dinamica */
  p_opo = calloc(n_fil*n_col/2, sizeof(pos));	/* per a les posicions ant. */
  if (!p_usu || !p_opo)	/* si no hi ha prou memoria per als vectors de pos. */
  { win_fi();				/* tanca les curses */
    if (p_usu) free(p_usu);
    if (p_opo) free(p_opo);	   /* allibera el que hagi pogut obtenir */
    fprintf(stderr,"Error en alocatacion de memoria dinamica.\n");
    exit(3);
  }
			/* Fins aqui tot ha anat be! */
  inicialitza_joc();

  /* Creació dels processos fills (un per cada oponent)
   * Cada fill executarà la funció mou_oponent() de forma concurrent */
  for (i = 0; i < num_oponents; i++) {
    id_proc[i] = fork();
    if (id_proc[i] == 0) {      /* Codi del procés fill */
      fprintf(arxiuSortida, "Inicialitzat el procés oponent %d (PID: %d)\n", i, getpid());
      mou_oponent(i);          /* El fill només executa el seu oponent */
      fprintf(arxiuSortida, "Fi del procés oponent %d (PID: %d)\n", i, getpid());
      fclose(arxiuSortida);
      exit(0);                 /* El fill acaba quan mor */
    }
    else if (id_proc[i] < 0) {  /* Error en la creació del procés */
      win_fi();
      fprintf(stderr,"Error en la creació del procés fill %d.\n", i);
      exit(4);
    }
  }

  /* El procés pare executa el moviment de l'usuari
   * mentre els fills executen els oponents en paral·lel */
  mou_usuari();

  /* Espera a que tots els fills acabin abans de finalitzar
   * Això assegura que no quedin processos zombie */
  for (i = 0; i < num_oponents; i++) {
    waitpid(id_proc[i], NULL, 0);
  }

  win_fi();				/* tanca les curses */
  free(p_usu);
  free(p_opo);	  	 /* allibera la memoria dinamica obtinguda */

  if (fi1 == -1) {
    printf("S'ha aturat el joc amb tecla RETURN!\n\n");
    fprintf(arxiuSortida, "Joc aturat manualment per l'usuari (PID: %d)\n", getpid());
  }
  else { 
    if (fi2) {
      printf("Ha guanyat l'usuari!\n\n");
      fprintf(arxiuSortida, "Fi del joc: ha guanyat l'usuari (PID: %d) - L'oponent ha quedat atrapat\n", getpid());
    }
    else {
      printf("Ha guanyat l'ordinador!\n\n");
      fprintf(arxiuSortida, "Fi del joc: ha guanyat l'ordinador (PID: %d) - L'usuari ha xocat\n", getpid());
    }
  }

  fprintf(arxiuSortida, ">>> FINAL DEL JOC <<<\n");
  if (arxiuSortida) fclose(arxiuSortida);

  return(0);
}