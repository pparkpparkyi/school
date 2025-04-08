#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>


int main(int argc, char *argv[]) {
    if(argc != 4){
        fprintf(stderr, "a.out <오프셋> <데이터> <파일명>");
        return 1;
    }
    int offset = atoi(argv[1]);
    char* data = argv[2];
    int fd = open(argv[3], O_RDWR);
    if(fd <0){
        perror("file open error");
        return 1;
    }


    int data_len = strlen(data);
    int fileSize = lseek(fd, 0, SEEK_END);
    int buf_size = fileSize - offset;
    char * buf = (char *)malloc(buf_size);
    if(buf == NULL){
        perror("memory alloc error");
        close(fd);
        return 1;
    }
    if(lseek(fd, offset, SEEK_SET) < 0) {
        perror("lseek error");
        free(buf);
        close(fd);
        return 1;
    }
    if(read(fd, buf, buf_size)<0){
        perror("data read error");
        free(buf);
        close(fd);
        return 1;
    }
    lseek(fd, offset, SEEK_SET);
    write(fd, data, data_len);
    write(fd, buf, buf_size);

    free(buf);
    close(fd);

    return 0;
}
