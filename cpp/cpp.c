#include<stdlib.h>
#include<stdio.h>
#include<ctype.h>
#include<getopt.h>
#include<string.h>
//#include<regex.h>

static enum state {
    INIT,
    MACRO_INIT,
    MACRO_NAME,
    MACRO_ARGS,
    MACRO_PRELINE,
    DEFINE,
    INCLUDE,
    ID,
    TOK,
    COMMENT
} stat;

char act[256] = {['0'...'9']=3,['A'...'Z']=2,['a'...'z']=2,['_']=2};

static inline int isprefix(char ch) {
    return isalnum(ch) || ch=='_';
}

static inline int ispart(char ch) {
    return isalpha(ch) || ch=='_';
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "%s <source> [-O <output>]", argv[0]);
        exit(EXIT_FAILURE);
    }

    FILE *f = fopen(argv[1], "r");
    char *out = NULL;
    if (argc == 4)
        out = argv[3];
    else {
        char *a = argv[1];
        char *k = strchr(a, '.');
        int len = k-a+3;
        out = (char*)malloc(len);
        strncpy(out, a, len-1);
        out[len-2] = 'c';
        out[len-1] = '\0';
    }
    FILE *wt = fopen(out, "w");
    char *tok = (char*)malloc(256);
    int tol = 0;
    while (!feof(f)) {
        char c = fgetc(f);
        switch (stat) {
            case INIT:
                if (c=='#') {
                    stat = MACRO_INIT;
                    break;
                }
                fputc(c, wt);
                break;
            case MACRO_INIT:
                if (isprefix(c)) {
                    stat = MACRO_NAME;
                    tok[tol++] = c;
                } else if (c=='\\') {
                    stat = MACRO_PRELINE;
                } else if (c == '\n') {
                    stat = INIT;
                }
                break;
            case MACRO_NAME:
                if (!ispart(c)) {
                    stat = MACRO_ARGS;
                }
                tok[tol++] = c;
            case MACRO_PRELINE:
                if (!isspace(c))
            case ID: break;
            default: break;
        }
    }
    if (out && argc < 4) free(out);
    free(tok);
    return 0;
}
