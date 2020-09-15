#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <stdbool.h>
// #include <math.h>
#include "cachelab.h"

struct cache_line {
    bool valid;
    unsigned long tag;
    int lru_counter;
    char* block;
};

struct cache_line *cache;

int verbose = 0, s = 0, E = 0, b = 0;
int hit_count = 0, miss_count = 0, eviction_count = 0;

void parseArgs(int argc, char *argv[], char** filename);
void openFile(char* filename);
void buildCache();
void freeCache();
int cacheHit(unsigned long tag, int set_index);
int insertEmptyLine(unsigned long tag, int set_index);
void replaceLine(unsigned long tag, int set_index);

void parseArgs(int argc, char *argv[], char** filename) {
    extern char *optarg;
    extern int optind, opterr, optopt;
    char opt;
    const char USAGE_INFO[] = "Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\nOptions:\n\
  -h         Print this help message.\n\
  -v         Optional verbose flag.\n\
  -s <num>   Number of set index bits.\n\
  -E <num>   Number of lines per set.\n\
  -b <num>   Number of block offset bits.\n\
  -t <file>  Trace file.\n\n\
Examples:\n\
  linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace\n\
  linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace\n";
  const char MISSING[] = "Missing required command line argument\n";

    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch (opt) {
            case 'h':
                printf("%s", USAGE_INFO);
                exit(0);
            case 'v':
                verbose = 1;
                break;
            case 's':
                s = atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 't':
                // https://stackoverflow.com/questions/24333417/c-pass-by-reference-string
                // sprintf(filename, optarg);
                *filename = malloc(strlen(optarg) + 1);
                if (!filename) {
                    exit(EXIT_FAILURE);
                }
                strcpy(*filename, optarg);
                break;
            default:
                fprintf(stderr, USAGE_INFO);
                exit(EXIT_FAILURE);
        }
    }
    if (s == 0 || E == 0 || b == 0 || *filename == NULL) {
        fprintf(stderr, "%s: %s%s", argv[0], MISSING, USAGE_INFO);
        exit(EXIT_FAILURE);
    }
}

void openFile(char* filename) {
    FILE *pFile;
    pFile = fopen(filename, "r");
    if (!pFile) {
        fprintf(stderr, "Failed to open the file.");
        exit(EXIT_FAILURE);
    }
    buildCache();

    char access_type;
    unsigned long address;
    int size;
    while (fscanf(pFile, " %c %lx,%d", &access_type, &address, &size) > 0) {
        if (access_type == 'I') {
            continue;
        }
        unsigned long tag = address >> (s + b);
        int set_index = (address >> b) & (~(~0 << s));
        // int block_offset = address & (~(~0 << b));
        if (verbose) {
            printf("%c %lx,%d", access_type, address, size);
        }
        if (cacheHit(tag, set_index)) {
            hit_count++;
            if (verbose) {
                printf(" hit");
            }
        }
        else {
            miss_count++;
            if (verbose) {
                printf(" miss");
            }
            if (!insertEmptyLine(tag, set_index)) {
                eviction_count++;
                replaceLine(tag, set_index);
                if (verbose) {
                    printf(" eviction");
                }
            }
        }
        if (access_type == 'M') {
            hit_count++;
            if (verbose) {
                printf(" hit");
            }
        }
        if (verbose) {
            printf("\n");
        }
        // printf("%c %lx %d\n", access_type, address, size);
        // printf("%ld %d %d\n\n", tag, set_index, block_offset);
    }
    fclose(pFile);
}

void buildCache() {
    int S = 1 << s;
    int B = 1 << b;
    cache = (struct cache_line *)malloc(S * E * sizeof(struct cache_line));
    for (int i = 0; i < S; i++) {
        for (int j = 0; j < E; j++) {
            (cache + i * E + j)->valid = false;
            (cache + i * E + j)->block = (char *)malloc(B * sizeof(char));
            if ((cache + i * E + j)->block == NULL) {
                exit(EXIT_FAILURE);
            }
            // printf("%d %ld\n", (*(cache + i * E + j)).valid, sizeof((*(cache + i * E + j)).block));
        }
    }
}

void freeCache() {
    int S = 1 << s;
    for (int i = 0; i < S; i++) {
        for (int j = 0; j < E; j++) {
            free((cache + i * E + j)->block);
        }
    }
    free(cache);
}

int cacheHit(unsigned long tag, int set_index) {
    for (int line = 0; line < E; line++) {
        struct cache_line* cl = cache + set_index * E + line;
        if (cl->valid && cl->tag == tag) {
            for (int update_line = 0; update_line < E; update_line++) {
                if ((cache + set_index * E + update_line)->valid 
                 && (cache + set_index * E + update_line)->lru_counter < cl->lru_counter) {
                    (cache + set_index * E + update_line)->lru_counter++;
                }
            }
            cl->lru_counter = 0;
            return 1;
        }
    }
    return 0;
}

int insertEmptyLine(unsigned long tag, int set_index) {
    for (int line = 0; line < E; line++) {
        struct cache_line* cl = cache + set_index * E + line;
        if (!cl->valid) {
            cl->valid = true;
            cl->lru_counter = 0;
            cl->tag = tag;
            /*
              char* address = (char*)((tag << (s + b)) | (set_index << b));
              memcpy(cl.block, address, 1 << b);
            */
            for (int update_line = 0; update_line < E; update_line++) {
                if ((cache + set_index * E + update_line)->valid && update_line != line) {
                    (cache + set_index * E + update_line)->lru_counter++;
                }
            }
            return 1;
        }
    }
    return 0;
}

void replaceLine(unsigned long tag, int set_index) {
    for (int line = 0; line < E; line++) {
        struct cache_line* cl = cache + set_index * E + line;
        cl->lru_counter++;
        if (cl->lru_counter == E) {
            cl->lru_counter = 0;
            cl->tag = tag;
        }
    }
}

int main(int argc, char *argv[]) {
    char* filename;
    parseArgs(argc, argv, &filename);
    openFile(filename);
    free(filename);
    freeCache();
    printSummary(hit_count, miss_count, eviction_count);
    return 0;
}
