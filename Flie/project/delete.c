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
    int fd = open(argv[3], O_RDWR);
    if(fd < 0){
        perror("file error");
        return 1;
    }

    int file_size = lseek(fd, 0, SEEK_END);

    if(offset > file_size){
        perror("offset is too big");
        return 1;
    }
    int move_byte;
    int start_read;
    
    if (byte >= 0) {
        if (offset + byte > file_size) { 
            move_byte = 0; 
        } else {
            move_byte = file_size - (offset + byte); 
        }
        start_read = offset + byte; 
        if (start_read > file_size) { 
            start_read = file_size; 
        }
        if (move_byte !=0){
            char* buf = (char *)malloc(move_byte);
            if(buf == NULL){
                perror("memory alloc error");
                close(fd);
                return 1;
            }
            lseek(fd, start_read, SEEK_SET);
            if (read(fd, buf, move_byte) != move_byte) {
                perror("data read error");
                free(buf);
                close(fd);
                return 1;
            }
            lseek(fd, offset, SEEK_SET);
            if (write(fd, buf, move_byte) != move_byte) {
                perror("data write error");
                free(buf);
                close(fd);
                return 1;
            }

            ftruncate(fd, file_size - abs(byte));
            free(buf);
        }
        else{
            ftruncate(fd, file_size - offset);
        }
    } else {
        move_byte = file_size - offset;
        move_byte = move_byte >=0 ? move_byte : 0;
        start_read = offset; 
        start_read = start_read <= file_size ? start_read : file_size;


        if (move_byte !=0){
            char* buf = (char *)malloc(move_byte);
            if(buf == NULL){
                perror("memory alloc error");
                close(fd);
                return 1;
            }
            lseek(fd, start_read, SEEK_SET);
            if (read(fd, buf, move_byte) != move_byte) {
                perror("data read error");
                free(buf);
                close(fd);
                return 1;
            }
            if(offset+byte <=0){
                lseek(fd, 0, SEEK_SET);
            }
            else{
                lseek(fd, offset+byte, SEEK_SET);
            }
            
            if (write(fd, buf, move_byte) != move_byte) {
                perror("data write error");
                free(buf);
                close(fd);
                return 1;
            }
            if(offset+byte >0){
                ftruncate(fd, file_size - abs(byte));
            }
            else{
                ftruncate(fd, file_size - abs(offset));
            }
            free(buf);
        }
        else{
            ftruncate(fd, file_size - offset);
        }
    }
    
    close(fd);
    return 0;
}
