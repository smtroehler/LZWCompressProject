#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef SMARTALLOC_H
#include "SmartAlloc.h"
#endif

#ifndef LZWCMP_H
#include "LZWCmp.h"
#endif

#define LINE_LENGTH 8
#define RECYCLE_SIZE 4096
#define UINT_BITS 32
#define FILE_EXTENSION_SIZE 3

typedef struct FileInfo {
   int currentCode;
   FILE *toFile;
}FileInfo;

void PrintToFile(void *state, UInt code, int done) {
   FileInfo *temp = state;
   
   if (done) {
      fprintf(temp->toFile, "\n");
   }
   else if (temp->currentCode++ != LINE_LENGTH) {
      fprintf(temp->toFile, "%08X ", code);
   }
   else {
      temp->currentCode = 1;   
      fprintf(temp->toFile, "%08X\n", code);
   }
}

int GetFlags(int argc, char **argv, int *reportFlag) {
   int count = 1, flags = 0;
   UChar *temp;
   
   while (argc > count) {
      if (*argv[count] == '-') {
         temp = argv[count];
         while (*++temp) {
            if (*temp == 't') 
               flags = flags | TRACE_TREE;
            else if (*temp == 'c')
               flags = flags | TRACE_CODES;
            else if (*temp == 'b')
               flags = flags | TRACE_BUMPS;
            else if (*temp == 'r')
               flags = flags | TRACE_RECYCLES;
            else if (*temp == 's')
               *reportFlag = 1;
            else
               printf("Bad argument: %c\n", *temp);
         }
      }
      count++;
   }
   return flags;
}

int main(int argc, char **argv) {
   UInt flags = 0, reportSpace = 0, i = 1; 
   LZWCmp cmp;
   UChar *newFileName, toEncode;
   FileInfo fileInfo;
   FILE *infile, *outfile;
   
   flags = GetFlags(argc, argv, &reportSpace);
   while (argc > i) {
      if (*argv[i] != '-') {
         infile = fopen(argv[i], "r");
         if (infile == NULL) {
            printf("Cannot open %s", argv[i]);
            exit(EXIT_FAILURE);
         }
         newFileName = malloc(strlen(argv[i]) + FILE_EXTENSION_SIZE); 
         strcpy(newFileName, argv[i]);
         outfile = fopen(strcat(newFileName, ".Z"), "w"); 
         fileInfo.toFile = outfile;
         fileInfo.currentCode = 1;
         LZWCmpInit(&cmp, PrintToFile, &fileInfo, RECYCLE_SIZE, flags);
         while (fscanf(infile, "%c", &toEncode) != EOF) {
            LZWCmpEncode(&cmp, toEncode); 
         }
         LZWCmpStop(&cmp);
         if (reportSpace)
            printf("Space after LZWCmpStop for %s: %ld\n", argv[i],  
             report_space());     
         free(newFileName);
         fclose(infile);
         fclose(outfile);
         LZWCmpDestruct(&cmp);
      }
      i++;
      
   }
   LZWCmpClearFreelist();
   if (reportSpace) 
      printf("Final space: %ld\n", report_space());
   return 0;
}