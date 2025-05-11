#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main() {
    pid_t pid;
    int fd = open("example.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("open");
        return 1;
    }
    printf("Soc el procés pare! He creat el fitxer, ara faré un fork\n");
    write(fd, "Soc el pare abans del fork \n", 28);
    pid = fork();

    if (pid == 0) {
        // Codi executat pel fill
        printf("Soc el procés fill! PID: %d\n", getpid());
        write(fd, "Fill escriu això\n", 18);
        close(fd);  // El fill tanca el fitxer
    } else if (pid > 0) {
        // Codi executat pel pare
        printf("Soc el procés pare! PID: %d, Fill PID: %d\n", getpid(), pid);
        write(fd, "Pare escriu això\n", 18);
        close(fd);  // El pare tanca el fitxer
    } else {
        // Error en el fork
        perror("fork");
        return 1;
    }

    return 0;
}
