#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFFER_SIZE 10

int main(int argc, char *argv[]) {
    if(argc != 3){
        fprintf(stderr, "a.out <원본파일명> <복사본파일명>");
        return 1;
    }

    int fd_read = open(argv[1], O_RDONLY);
    if(fd_read <0){
        perror("원본 파일 오류");
        return 1;
    }
    //파일에대한 읽고 쓰기 가능 권한 : 0644
    int fd_write = open(argv[2], O_WRONLY|O_CREAT|O_TRUNC, 0644);
    if(fd_write < 0){
        perror("복사본 파일 오류");
        close(fd_read);
        return 1;
    }
    int r_byte, w_byte;
    char buf[BUFFER_SIZE];
    while ((r_byte = read(fd_read,buf,BUFFER_SIZE)) >0){
        w_byte = write(fd_write,buf,r_byte);
        if(w_byte != r_byte){
            perror("wrtie 오류");
            return 1;
        }
    }
    close(fd_read);
    close(fd_write);

    return 0;
}
