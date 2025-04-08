#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#define BUFFER_SIZE 1024


int main(int argc, char *argv[]) {
    if(argc != 4){
        fprintf(stderr, "a.out <오프셋> <데이터> <파일명>");
        return 1;
    }
    int offset = atoi(argv[1]);
    char* data = argv[2];
    int fd = open(argv[3], O_WRONLY | O_CREAT , 0644);
    if(fd <0){
        perror("file open error");
        return 1;
    }

    char buf[BUFFER_SIZE];

    if(lseek(fd, offset, SEEK_SET) < 0) {
        perror("lseek error");
        close(fd);
        return 1;
    }
    int bytes = write(fd, data, strlen(data));
    if(bytes < 0){
        perror("file write error");
        close(fd);
        return 1;
    }
        
    close(fd);

    return 0;
}
