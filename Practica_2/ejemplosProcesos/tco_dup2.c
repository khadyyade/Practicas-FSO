#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

int main() {
    int pipefd[2];  // Descriptors de la pipe
    pid_t pid1, pid2;

    // Creem la pipe
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // Creem el primer fill (executarà ls -l)
    pid1 = fork();
    if (pid1 == 0) {
        // Codi del primer fill
        close(pipefd[0]);  // Tanquem l'extrem de lectura de la pipe
        dup2(pipefd[1], STDOUT_FILENO);  
        // Redirigim la sortida estàndard a la pipe
        close(pipefd[1]);  
        // Tanquem l'extrem d'escriptura després de duplicar-lo
        execlp("ls", "ls", "-l", NULL);  // Executem ls -l
        perror("execlp ls");  // Si arribem aquí, hi ha hagut un error
        exit(EXIT_FAILURE);
    } else if (pid1 < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    // Creem el segon fill (executarà wc)
    pid2 = fork();
    if (pid2 == 0) {
        // Codi del segon fill
        close(pipefd[1]);  // Tanquem l'extrem d'escriptura de la pipe
        dup2(pipefd[0], STDIN_FILENO);  
        // Redirigim l'entrada estàndard a la pipe
        close(pipefd[0]);  // Tanquem l'extrem de lectura després de duplicar-lo

        execlp("wc", "wc", NULL);  // Executem wc
        perror("execlp wc");  // Si arribem aquí, hi ha hagut un error
        exit(EXIT_FAILURE);
    } else if (pid2 < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    // El pare tanca els dos extrems de la pipe
    close(pipefd[0]);
    close(pipefd[1]);
    // Esperem que els dos fills acabin
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    printf("Els dos fills han acabat.\n");
    return 0;
}