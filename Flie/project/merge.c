#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if(argc != 4){
        fprintf(stderr, "a.out <파일명1> <파일명2> <파일명3>");
        return 1;
    }

    int fd_1 = open(argv[1], O_RDONLY);
    if(fd_1 <0){
        perror("file 1 error");
        return 1;
    }

    int fd_2 = open(argv[2], O_RDONLY);
    if(fd_2 <0){
        perror("file 2 error");
        close(fd_1);
        return 1;
    }

    int fd_3 = open(argv[3], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(fd_3 <0){
        perror("file 3 error");
        close(fd_1);
        close(fd_2);
        return 1;
    }

    char buf[BUFFER_SIZE];
    int bytes;

    while((bytes = read(fd_1, buf, BUFFER_SIZE))>0){
        if(write(fd_3, buf, bytes) != bytes){
            perror("file 1 write error");
            close(fd_1);
            close(fd_2);
            close(fd_3);
            return 1;
        }
    }

    while((bytes = read(fd_2, buf, BUFFER_SIZE))>0){
        if(write(fd_3, buf, bytes) != bytes){
            perror("file 2 write error");
            close(fd_1);
            close(fd_2);
            close(fd_3);
            return 1;
        }
    }
    close(fd_1);
    close(fd_2);
    close(fd_3);

    return 0;
}
