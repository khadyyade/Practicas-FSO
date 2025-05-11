#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    int pipefd[2];  // Descriptors de la pipe
    pid_t pid1, pid2;

    // Crear la pipe
    if (pipe(pipefd)) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // Crear el primer fill (executarà ls -l)
    pid1 = fork();
    if (pid1 == 0) {       // Codi del primer fill
        close(pipefd[0]);  // Tancar l'extrem de lectura de la pipe

        // Redirigir el stdout a l'extrem d'escriptura de la pipe
        close(1);          // Tancar stdout
        dup(pipefd[1]);    
        // Duplicar l'extrem d'escriptura de la pipe a stdout (descriptor 1)
        close(pipefd[1]);  
        // Tancar l'extrem d'escriptura de la pipe (ja està duplicat)

        // Executar ls -l
        execlp("ls", "ls", "-l", NULL);
        perror("execlp ls");  // Si execlp falla
        exit(EXIT_FAILURE);
    }

    // Crear el segon fill (executarà wc)
    pid2 = fork();
    if (pid2 == 0) {        // Codi del segon fill
        close(pipefd[1]);  // Tancar l'extrem d'escriptura de la pipe

        // Redirigir el (stdin) a l'extrem de lectura de la pipe
        close(0);          // Tancar stdin
        dup(pipefd[0]);    
        // Duplicar l'extrem de lectura de la pipe a stdin (descriptor 0)
        close(pipefd[0]);  
        // Tancar l'extrem de lectura de la pipe (ja està duplicat)

        // Executar wc
        execlp("wc", "wc", NULL);
        perror("execlp wc");  // Si execlp falla
        exit(EXIT_FAILURE);
    }

    // Procés pare: tancar els dos extrems de la pipe
    close(pipefd[0]);
    close(pipefd[1]);

    // Esperar que els dos fills acabin
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    printf("Els dos fills han acabat.\n");
    return 0;
}
