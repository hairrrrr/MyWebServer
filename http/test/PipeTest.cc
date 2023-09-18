#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <sys/types.h>

int main() {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t child_pid = fork();
    if (child_pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (child_pid == 0) { // 子进程
        close(pipefd[1]); // 关闭写入端

        char buffer[100];
        ssize_t num_bytes;
        while( (num_bytes = read(pipefd[0], buffer, sizeof(buffer))) > 0 )
        {
            buffer[num_bytes] = '\0';
            printf("Child received: %s\n", buffer);
        }

        printf("child finish\n");

        close(pipefd[0]); // 关闭读取端
    } else { // 父进程
        close(pipefd[0]); // 关闭读取端

        const char *message = "Hello, child!";
        ssize_t num_bytes = write(pipefd[1], message, strlen(message));
        if (num_bytes == -1) {
            perror("write");
            exit(EXIT_FAILURE);
        }
        getchar();

        close(pipefd[1]); // 关闭写入端

        sleep(10);
    }

    return 0;
}
