#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stddef.h>
#include <signal.h>
#include <sys/wait.h>

int wait_child(pid_t id)
{
    int status;
    pid_t result;

    do {
        result = waitpid(id, &status, 0);
    } while (result == -1 && errno == EINTR);

    if (result == -1) {
        perror("waitpid");
        return -1;
    }

    if (!WIFEXITED(status)) {
        fprintf(stderr, "Child process terminated abnormally\n");
        return -1;
    }

    if (WEXITSTATUS(status) != 0) {
        fprintf(stderr, "Child process failed with code %d\n",
                WEXITSTATUS(status));
        return -1;
    }

    return 0;
}

int write_all(int fd, const void* data, size_t size){
    const unsigned char* ptr = data;
    size_t written = 0;

    while(written < size){
        ssize_t n = write(fd, ptr + written, size - written);

        if (n > 0){
            written += (size_t)n;
            continue;
        }
        if (n == -1 && errno == EINTR){
            continue;
        }

        if (n == 0){
            errno = EIO;
            return -1;
        }

        return -1;
    }
    return 0;
}

int main(){
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR){
        perror("signal");
        return 1;
    }

    char filename[256];
    if (fgets(filename, sizeof(filename), stdin) == NULL){
        perror("fgets");
        return 1;
    }
    filename[strcspn(filename, "\n")] = '\0';

    int pipefd[2];

    if(pipe(pipefd) == -1){
        perror("pipe");
        return 1;
    }

    pid_t id = fork();

    if (id < 0){
        perror("fork");
        if (close(pipefd[0]) == -1) {
            perror("close");
        }
        if (close(pipefd[1]) == -1) {
            perror("close");
        }
        return 1;
    }

    if (id == 0){
        if (close(pipefd[1]) == -1) {
            perror("close");
            _exit(1);;
        }

        if (dup2(pipefd[0], STDIN_FILENO) == -1){
            perror("dup2");
            _exit(1);
        }

        if (close(pipefd[0]) == -1) {
            perror("close");
            _exit(1);
        }

        execl("./child", "child", filename, (char *)NULL);
        perror("execl");
        _exit(1);
    }

    if (id > 0){

        printf(
            "Parent PID: %ld, child PID: %ld\n",
            (long)getpid(),
            (long)id
        );

        if (close(pipefd[0]) == -1) {
            perror("close");
            if (close(pipefd[1]) == -1) {
                perror("close");
            }
            wait_child(id);
            return 1;
        }

        char *line = NULL;
        size_t size = 0;
        ssize_t len;

        while ((len = getline(&line, &size, stdin)) != -1){
            if (write_all(pipefd[1], line, (size_t)len) == -1){
                perror("write");
                if (close(pipefd[1]) == -1) {
                    perror("close");
                }
                free(line);
                wait_child(id);
                return 1;
            }
        }

        if (ferror(stdin)){
            perror("getline");
            free(line);
            if (close(pipefd[1]) == -1) {
                perror("close");
            }
            wait_child(id);
            return 1;
        }

        free(line);

        if (close(pipefd[1]) == -1) {
            perror("close");
            wait_child(id);
            return 1;
        }

        if (wait_child(id) == -1) {
            return 1;
        }
    }
    return 0;
}