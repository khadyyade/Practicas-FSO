#include <stdio.h>
#include <unistd.h>

int main() {
    pid_t pid = fork();

    if (pid == 0) {
        // Codi executat pel fill
        printf("Soc el procés fill! PID: %d\n", getpid());
        execlp("/bin/ls", "ls", "-l", NULL);  
        // Substituïm la imatge del fill per 'ls -l'
        perror("execlp");   // Això s'executa només si hi ha un error en execlp
        return 1;
    } else if (pid > 0) {   // Codi executat pel pare
        printf("Soc el procés pare! PID: %d, Fill PID: %d\n", getpid(), pid);
    } else {               // Error en el fork
        perror("fork");
        return 1;
    }

    return 0;
}