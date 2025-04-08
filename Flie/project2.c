/*************************************************************
 *  ftlmgr.c
 *  
 *  Flash Device Driver를 활용하여
 *   (1) flash memory emulator (c)
 *   (2) 페이지 쓰기 (w)
 *   (3) 페이지 읽기 (r)
 *   (4) 블록 소거 (e)
 *   (5) In-place update (u)
 *  를 구현하는 예시 코드입니다.
 *
 *  - 파일 I/O는 C 표준 라이브러리 함수만 사용
 *  - fdevicedriver.c에서 제공하는 fdd_read/fdd_write/fdd_erase 사용
 *  - flash.h, fdevicedriver.c 수정 금지
 *
 *************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "flash.h"  // PAGE_NUM=8, SECTOR_SIZE=512, SPARE_SIZE=16, PAGE_SIZE=528, BLOCK_SIZE=4224

// fdevicedriver.c의 함수와 전역변수를 extern 선언
extern FILE *flashmemoryfp;
extern int fdd_read(int ppn, char *pagebuf);
extern int fdd_write(int ppn, char *pagebuf);
extern int fdd_erase(int pbn);

// -------------------------------------------------------
// 전역변수: fdevicedriver.c에서 사용하므로 flashmemoryfp로 통일
// -------------------------------------------------------
FILE *flashmemoryfp;  // 반드시 이 이름으로 정의 (flash.h에 나온 대로)

// 통계용 전역변수: 읽기/쓰기/소거 횟수
static int g_read_count  = 0;
static int g_write_count = 0;
static int g_erase_count = 0;

// -------------------------------------------------------
// 함수 선언 (명령어별)
// -------------------------------------------------------
void create_flash(const char *flashfile, int numBlocks);
void write_page(const char *flashfile, int ppn, const char *sectordata, const char *sparedata);
void read_page(const char *flashfile, int ppn);
void erase_block(const char *flashfile, int pbn);
void update_page(const char *flashfile, int ppn, const char *sectordata, const char *sparedata);

// -------------------------------------------------------
// 보조 함수 (In-place update 및 기타)
// -------------------------------------------------------
static int isPageEmpty(const char *pageBuf);
static int isBlockEmpty(int pbn);
static int findEmptyBlock(int oldBlock);  // oldBlock과 다른 빈 블록 찾기

int main(int argc, char *argv[])
{
    if(argc < 2) {
        fprintf(stderr, "Usage: a.out [c|w|r|e|u] ...\n");
        return 1;
    }

    char cmd = argv[1][0];  // 'c', 'w', 'r', 'e', 'u'
    switch(cmd) {

    case 'c':
        // a.out c <flashfile> <#blocks>
        if(argc != 4) {
            fprintf(stderr, "Usage: a.out c <flashfile> <#blocks>\n");
            return 1;
        }
        create_flash(argv[2], atoi(argv[3]));
        break;

    case 'w':
        // a.out w <flashfile> <ppn> "<sectordata>" "<sparedata>"
        if(argc != 6) {
            fprintf(stderr, "Usage: a.out w <flashfile> <ppn> \"<sector>\" \"<spare>\"\n");
            return 1;
        }
        write_page(argv[2], atoi(argv[3]), argv[4], argv[5]);
        break;

    case 'r':
        // a.out r <flashfile> <ppn>
        if(argc != 4) {
            fprintf(stderr, "Usage: a.out r <flashfile> <ppn>\n");
            return 1;
        }
        read_page(argv[2], atoi(argv[3]));
        break;

    case 'e':
        // a.out e <flashfile> <pbn>
        if(argc != 4) {
            fprintf(stderr, "Usage: a.out e <flashfile> <pbn>\n");
            return 1;
        }
        erase_block(argv[2], atoi(argv[3]));
        break;

    case 'u':
        // a.out u <flashfile> <ppn> "<sectordata>" "<sparedata>"
        if(argc != 6) {
            fprintf(stderr, "Usage: a.out u <flashfile> <ppn> \"<sector>\" \"<spare>\"\n");
            return 1;
        }
        update_page(argv[2], atoi(argv[3]), argv[4], argv[5]);
        break;

    default:
        fprintf(stderr, "Unknown command '%c'\n", cmd);
        return 1;
    }

    return 0;
}

// ---------------------------------------------------------------------
// (1) flash memory 생성: a.out c <flashfile> <#blocks>
// ---------------------------------------------------------------------
void create_flash(const char *flashfile, int numBlocks)
{
    flashmemoryfp = fopen(flashfile, "wb");
    if(!flashmemoryfp) {
        perror("fopen error");
        exit(1);
    }

    char blockBuf[BLOCK_SIZE];
    memset(blockBuf, 0xFF, BLOCK_SIZE);

    for(int i = 0; i < numBlocks; i++){
        fwrite(blockBuf, BLOCK_SIZE, 1, flashmemoryfp);
    }

    fclose(flashmemoryfp);
}

// ---------------------------------------------------------------------
// (2) 페이지 쓰기: a.out w <flashfile> <ppn> "<sectordata>" "<sparedata>"
// ---------------------------------------------------------------------
void write_page(const char *flashfile, int ppn, const char *sectordata, const char *sparedata)
{
    flashmemoryfp = fopen(flashfile, "r+b");
    if(!flashmemoryfp) {
        perror("fopen error");
        return;
    }

    char pageBuf[PAGE_SIZE];
    memset(pageBuf, 0xFF, PAGE_SIZE);

    if(fdd_read(ppn, pageBuf) == 1){
        g_read_count++;
    } else {
        fprintf(stderr, "fdd_read error on ppn=%d\n", ppn);
        fclose(flashmemoryfp);
        return;
    }

    if((unsigned char)pageBuf[0] != 0xFF){
        fprintf(stderr, "Error: page %d is already programmed. Use 'u' for update.\n", ppn);
        fclose(flashmemoryfp);
        return;
    }

    memset(pageBuf, 0xFF, PAGE_SIZE);
    size_t sLen = strlen(sectordata);
    if(sLen > SECTOR_SIZE) sLen = SECTOR_SIZE;
    memcpy(pageBuf, sectordata, sLen);

    int spareInt = atoi(sparedata);
    memcpy(pageBuf + SECTOR_SIZE, &spareInt, sizeof(int));

    if(fdd_write(ppn, pageBuf) == 1){
        g_write_count++;
    } else {
        fprintf(stderr, "fdd_write error on ppn=%d\n", ppn);
    }

    fclose(flashmemoryfp);
}

// ---------------------------------------------------------------------
// (3) 페이지 읽기: a.out r <flashfile> <ppn>
// ---------------------------------------------------------------------
void read_page(const char *flashfile, int ppn)
{
    flashmemoryfp = fopen(flashfile, "rb");
    if(!flashmemoryfp) {
        perror("fopen error");
        return;
    }

    char pageBuf[PAGE_SIZE];
    memset(pageBuf, 0xFF, PAGE_SIZE);

    if(fdd_read(ppn, pageBuf) == 1){
        g_read_count++;
    } else {
        fprintf(stderr, "fdd_read error on ppn=%d\n", ppn);
        fclose(flashmemoryfp);
        return;
    }

    int hasSector = 0;
    for(int i = 0; i < SECTOR_SIZE; i++){
        if((unsigned char)pageBuf[i] == 0xFF) break;
        putchar(pageBuf[i]);
        hasSector = 1;
    }
    if(hasSector) putchar(' ');

    int spareVal;
    memcpy(&spareVal, pageBuf + SECTOR_SIZE, sizeof(int));

    if(spareVal != -1 && (unsigned char)pageBuf[SECTOR_SIZE] != 0xFF){
        if(hasSector) putchar(' ');
        printf("%d\n", spareVal);
    } else {
        if(hasSector) putchar('\n');
    }

    fclose(flashmemoryfp);
}

// ---------------------------------------------------------------------
// (4) 블록 소거: a.out e <flashfile> <pbn>
// ---------------------------------------------------------------------
void erase_block(const char *flashfile, int pbn)
{
    flashmemoryfp = fopen(flashfile, "r+b");
    if(!flashmemoryfp) {
        perror("fopen error");
        return;
    }

    if(fdd_erase(pbn) == 1){
        g_erase_count++;
    } else {
        fprintf(stderr, "fdd_erase error on pbn=%d\n", pbn);
    }

    fclose(flashmemoryfp);
}

// ---------------------------------------------------------------------
// 보조 함수: 페이지가 전부 0xFF인지 검사
// ---------------------------------------------------------------------
static int isPageEmpty(const char *pageBuf)
{
    for(int i = 0; i < PAGE_SIZE; i++){
        if((unsigned char)pageBuf[i] != 0xFF){
            return 0;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------
// 보조 함수: 블록(8페이지) 전체가 0xFF인지 검사
// ---------------------------------------------------------------------
static int isBlockEmpty(int pbn)
{
    char buf[PAGE_SIZE];
    for(int i = 0; i < PAGE_NUM; i++){
        int curPPN = pbn * PAGE_NUM + i;
        if(fdd_read(curPPN, buf) == 1){
            g_read_count++;
        } else {
            return 0;
        }
        if(!isPageEmpty(buf)){
            return 0;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------
// 보조 함수: oldBlock과 다른 빈 블록 찾기
// ---------------------------------------------------------------------
static int findEmptyBlock(int oldBlock)
{
    fseek(flashmemoryfp, 0, SEEK_END);
    long fileSize = ftell(flashmemoryfp);
    fseek(flashmemoryfp, 0, SEEK_SET);

    int totalBlocks = fileSize / BLOCK_SIZE;
    for(int b = 0; b < totalBlocks; b++){
        if(b == oldBlock) continue;
        if(isBlockEmpty(b)){
            return b;
        }
    }
    return -1;
}

// ---------------------------------------------------------------------
// (5) In-place update: a.out u <flashfile> <ppn> "<sectordata>" "<sparedata>"
//     - oldBlock의 valid 페이지를 다른 빈 블록(newBlock)에 복사하고,
//       갱신 대상 페이지는 새 데이터로 작성 후 oldBlock erase,
//       newBlock 내용을 oldBlock으로 복원, newBlock erase
//     - 최종적으로 #reads, #writes, #erases 출력
// ---------------------------------------------------------------------
void update_page(const char *flashfile, int ppn, const char *sectordata, const char *sparedata)
{
    int start_r = g_read_count;
    int start_w = g_write_count;
    int start_e = g_erase_count;

    flashmemoryfp = fopen(flashfile, "r+b");
    if(!flashmemoryfp){
        perror("fopen error");
        return;
    }

    int oldBlock = ppn / PAGE_NUM;
    int offset   = ppn % PAGE_NUM;

    // 빈 블록 찾기 (oldBlock 제외)
    int newBlock = findEmptyBlock(oldBlock);
    if(newBlock < 0){
        fprintf(stderr, "Error: no empty block found.\n");
        fclose(flashmemoryfp);
        return;
    }
    if(newBlock == oldBlock){
        fprintf(stderr, "Error: oldBlock == newBlock. Cannot proceed.\n");
        fclose(flashmemoryfp);
        return;
    }

    // A) oldBlock → newBlock 복사 (갱신 대상 페이지는 새 데이터로 작성)
    char pageBuf[PAGE_SIZE];
    for(int i = 0; i < PAGE_NUM; i++){
        int curPPN_old = oldBlock * PAGE_NUM + i;
        int curPPN_new = newBlock * PAGE_NUM + i;

        if(i == offset){
            char updated[PAGE_SIZE];
            memset(updated, 0xFF, PAGE_SIZE);

            size_t sLen = strlen(sectordata);
            if(sLen > SECTOR_SIZE) sLen = SECTOR_SIZE;
            memcpy(updated, sectordata, sLen);

            int spareInt = atoi(sparedata);
            memcpy(updated + SECTOR_SIZE, &spareInt, sizeof(int));

            if(fdd_write(curPPN_new, updated) == 1){
                g_write_count++;
            }
        } else {
            if(fdd_read(curPPN_old, pageBuf) == 1){
                g_read_count++;
            } else {
                fprintf(stderr, "fdd_read error on ppn=%d\n", curPPN_old);
                continue;
            }
            if(!isPageEmpty(pageBuf)){
                if(fdd_write(curPPN_new, pageBuf) == 1){
                    g_write_count++;
                }
            }
        }
    }

    // B) oldBlock erase
    if(fdd_erase(oldBlock) == 1){
        g_erase_count++;
    }

    // C) newBlock → oldBlock 복원
    for(int i = 0; i < PAGE_NUM; i++){
        int curPPN_new = newBlock * PAGE_NUM + i;
        int curPPN_old = oldBlock * PAGE_NUM + i;

        if(fdd_read(curPPN_new, pageBuf) == 1){
            g_read_count++;
        }
        if(!isPageEmpty(pageBuf)){
            if(fdd_write(curPPN_old, pageBuf) == 1){
                g_write_count++;
            }
        }
    }

    // D) newBlock erase (빈 블록으로 되돌림)
    if(fdd_erase(newBlock) == 1){
        g_erase_count++;
    }

    fclose(flashmemoryfp);

    int r = g_read_count - start_r;
    int w = g_write_count - start_w;
    int e = g_erase_count - start_e;

    printf("#reads=%d #writes=%d #erases=%d\n", r, w, e);
}
