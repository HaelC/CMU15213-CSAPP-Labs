#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <stdbool.h>
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
    // Make sure all arguments are given.
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
        // In this lab, we don't need to worry about block_offset.
        // int block_offset = address & (~(~0 << b));
        if (verbose) {
            printf("%c %lx,%d", access_type, address, size);
        }
        // Go through the cache to check whether there is a cache hit
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
            // Go through the cache to see whether there is an empty line
            if (!insertEmptyLine(tag, set_index)) {
                // No empty line in cache, so we need to evict the oldest line
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
            // increment previous cache lines' lru_counters
            for (int update_line = 0; update_line < E; update_line++) {
                if ((cache + set_index * E + update_line)->valid 
                 && (cache + set_index * E + update_line)->lru_counter < cl->lru_counter) {
                    (cache + set_index * E + update_line)->lru_counter++;
                }
            }
            // update its own lru_counter to the latest
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
            // We don't need to copy the memory to cache in this lab.
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
        // The lru_counter should be in range [0, E-1].
        // If it reaches E, it means that it should be replaced.
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
