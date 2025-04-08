#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char *argv[]) {
    if(argc != 4){
        fprintf(stderr, "a.out <오프셋> <바이트 수> <파일명>");
        return 1;
    }

    int offset = atoi(argv[1]); 
    int byte = atoi(argv[2]);
    bool flag = true;

    int buf_size;

    int fd_read = open(argv[3], O_RDONLY);
    if(fd_read < 0){
        perror("file error");
        return 1;
    }

    if(lseek(fd_read, offset, SEEK_SET) < 0) {
        perror("lseek error");
        close(fd_read);
        return 1;
    }

    if(byte < 0){
        flag = false;
        buf_size = offset < -byte ? offset : -byte;
        lseek(fd_read, -buf_size, SEEK_CUR);
    }else{
        buf_size = byte;
    }

    char *buf = malloc(buf_size);
    if (buf == NULL) {
        fprintf(stderr, "Memory alloc error");
        close(fd_read);
        return 1;
    }

    int bytes_read = read(fd_read, buf, buf_size); 
    for (int i = 0; i < bytes_read; ++i) {
        printf("%c", buf[i]);
    }

    free(buf);
    close(fd_read);

    return 0;
}
