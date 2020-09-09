#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include "cachelab.h"

void parseArgs(int argc, char *argv[], int* verbose, int* s, int* E, int* b, char** filename) {
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
                *verbose = 1;
                break;
            case 's':
                *s = atoi(optarg);
                break;
            case 'E':
                *E = atoi(optarg);
                break;
            case 'b':
                *b = atoi(optarg);
                break;
            case 't':
                // https://stackoverflow.com/questions/24333417/c-pass-by-reference-string
                // sprintf(filename, optarg);
                *filename = malloc(strlen(optarg) + 1);
                strcpy(*filename, optarg);
                break;
            default:
                fprintf(stderr, USAGE_INFO);
                exit(EXIT_FAILURE);
        }
    }
    if (*s == 0 || *E == 0 || *b == 0 || *filename == NULL) {
        fprintf(stderr, "%s: %s%s", argv[0], MISSING, USAGE_INFO);
        exit(EXIT_FAILURE);
    }
}

void openFile(char* filename, int verbose, int s, int E, int b) {
    FILE *pFile;
    pFile = fopen(filename, "r");
    if (!pFile) {
        fprintf(stderr, "Failed to open the file.");
        exit(EXIT_FAILURE);
    }

    char access_type;
    unsigned long address;
    int size;
    while (fscanf(pFile, " %c %lx,%d", &access_type, &address, &size) > 0) {
        if (access_type == 'I') continue;
        printf("%c %lx %d\n", access_type, address, size);
    }
    fclose(pFile);
}



int main(int argc, char *argv[]) {
    int verbose = 0, s = 0, E = 0, b = 0;
    char* filename;
    parseArgs(argc, argv, &verbose, &s, &E, &b, &filename);
    openFile(filename, verbose, s, E, b);
    free(filename);
    printSummary(0, 0, 0);
    return 0;
}
