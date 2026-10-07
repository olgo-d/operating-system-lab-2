#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stddef.h>

int main(int argc, char *argv[]){

    printf(
        "Child PID: %ld, parent PID: %ld\n",
        (long)getpid(),
        (long)getppid()
    );
    
    if (argc < 2){
        return 1;
    }

    FILE *file = fopen(argv[1], "w");

    if (file == NULL){
        perror("fopen");
        return 1;
    }

    char *line = NULL;
    size_t size = 0;

    while (getline(&line, &size, stdin) != -1){

        float sum = 0;
        char *ptr = line;

        while (*ptr != '\0'){
            char *end;
            errno = 0;
            float value = strtof(ptr, &end);
            
            if(ptr == end){
                break;
            }

            if (errno == ERANGE){
                perror("strtof");
                free(line);
                fclose(file);
                return 1;
            }

            sum += value;
            ptr = end;
        }

        if (fprintf(file, "%f\n", sum) < 0){
            perror("fprintf");
            free(line);
            fclose(file);
            return 1;
        }
    }

    if (ferror(stdin)){
        perror("getline");
        free(line);
        fclose(file);
        return 1;
    }

    free(line);
    if (fclose(file) == EOF){
        perror("fclose");
        return 1;
    }
    return 0;
}