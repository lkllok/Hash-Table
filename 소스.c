#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <time.h> // clock �Լ�

#define   MAX_CACHE_SIZE      8192

// ������ ���� ��Ʈ���� ����� ���� 
#define TRACE_FILE_NAME      "ref_stream.txt"

#define HASH_TABLE_SIZE 829 // ������� ���� �ؽ� ���̺� �й� ���� �Ҽ� 829

// hash key
int hash_function(unsigned long blkno) {
    return blkno % HASH_TABLE_SIZE;
}

// ���߿��Ḯ��Ʈ�� ��� Ÿ��
struct buffer {
    unsigned long blkno;
    struct buffer* next, * prev;
    struct buffer* hash_next, * hash_prev;
};

// �����͸� ������ ������ �� ���� �Ҵ� ����
// ���� �߰����� �޸� �Ҵ��� ����
// �Ҵ� ���� ������ �̿��Ͽ� LRU ����Ʈ �Ǵ� FIFO ����Ʈ�� �����ؾ� �� 
struct buffer cache_buffer[MAX_CACHE_SIZE];

// hash table
#define HASH_SIZE      6144 // 8192�� 75%
struct buffer hash_table[HASH_SIZE];

// LRU �ùķ����� �� ��� lrulist �ƴϸ� fifolist�� �����ϱ� ���� ��� ��� ����
#if 1
struct buffer lrulist;
#else
struct buffer fifolist;
#endif

unsigned long curtime, hit, miss;

void lruin(struct buffer* bp) {
    struct buffer* dp = &lrulist;

    bp->next = dp->next;
    bp->prev = dp;
    (dp->next)->prev = bp;
    dp->next = bp;
}

struct buffer* lruout() {
    struct buffer* bp;
    bp = lrulist.prev;

    //�ؽ� ���̺� ����

    if (bp->blkno == ~0) {
        (bp->prev)->next = bp->next;
        (bp->next)->prev = bp->prev;
    }
    else {
        (bp->prev)->next = bp->next;
        (bp->next)->prev = bp->prev;
        (bp->hash_prev)->hash_next = bp->hash_next;
        (bp->hash_next)->hash_prev = bp->hash_prev;
    }
    return bp;
}

void reorder(struct buffer* bp) {
    (bp->prev)->next = bp->next;
    (bp->next)->prev = bp->prev;

    lruin(bp);
}

void init() {
    int i;

    lrulist.next = lrulist.prev = &lrulist;

    for (i = 0; i < MAX_CACHE_SIZE; i++) {
        cache_buffer[i].blkno = ~0;
        cache_buffer[i].next = cache_buffer[i].prev = NULL;
        lruin(&cache_buffer[i]);
    }

    // �ؽ� ���̺� ���߿��Ḯ��Ʈ
    for (i = 0; i < HASH_TABLE_SIZE; i++) {
        hash_table[i].hash_next = &hash_table[i];
        hash_table[i].hash_prev = &hash_table[i];
    }
    return;
}

//O(n)
//hashtable
struct buffer* findblk(unsigned long blkno) {
    struct buffer* node;
    int hash = hash_function(blkno);
    //node ����
    node = &hash_table[hash]; // �ؽ� ���̺� ��� �ּ�
    while (node->hash_next != &hash_table[hash]) { // �ؽ� ���̺� �ѹ��� �������� �� Ž��
        if (node->hash_next->blkno == blkno) {
            return node->hash_next; // ã������ ���� �ּ� return
        }
        node = node->hash_next; 
    }
    return NULL;
}


void pgref(unsigned long blkno) {
    struct buffer* bufp;
    int hash = hash_function(blkno);

    //search blkno in lrulist
    bufp = findblk(blkno);

    if (bufp) {
        //hit
        hit++;

        reorder(bufp);
    }

    else {
        //miss
        miss++;
        struct buffer* node;

        bufp = lruout();
        bufp->blkno = blkno;
        lruin(bufp);

        node = &hash_table[hash];
        bufp->hash_next = node->hash_next;
        bufp->hash_prev = node;
        (node->hash_next)->hash_prev = bufp;
        node->hash_next = bufp;

    }
}


int main(int argc, char* argv[])
{
    clock_t start, end;
    start = clock();
    int   ret;
    unsigned long blkno;
    FILE* fp = NULL;

    init();

    curtime = miss = hit = 0;

    if ((fp = fopen(TRACE_FILE_NAME, "r")) == NULL) {
        printf("%s trace file open fail.\n", TRACE_FILE_NAME);

        return 0;
    }
    else {
        printf("start simulation!\n");
    }

    ////////////////////////////////////////////////
    // �ùķ��̼� ���� �� ��� ����� ���� �ڵ�
    ////////////////////////////////////////////////
    while ((ret = fscanf(fp, "%lu\n", &blkno)) != EOF) {
        curtime++; 
        pgref(blkno); 
    }

    fclose(fp);
    end = clock(); // timer ��
    double result = (end - start) / 1000; // �ҿ� �ð�

    printf("total access = %lu, hit = %lu, miss=%lu\n", curtime, hit, miss);
    printf("hit ratio = %f, miss ratio = %f, time = %lf\n", (float)hit / (float)curtime, (float)miss / (float)curtime, result);


    return 0;
}