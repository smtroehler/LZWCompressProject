#include <string.h>
#include <stdio.h>

#ifndef CODESET_H
#include "CodeSet.h"
#endif

#ifndef SMARTALLOC_H
#include "SmartAlloc.h"
#endif
typedef struct CodeEntry {
   char code;
   struct CodeEntry *prev;
   int count;
   char *codePtr;
} CodeEntry;

typedef struct CodeSet {
   CodeEntry *arr;
   int size;
   int maxSize;
}CodeSet;

void *CreateCodeSet(int numCodes) {
   CodeSet *set = malloc(sizeof(CodeSet));
   CodeEntry *arr = calloc(numCodes, sizeof(CodeEntry));
   void *out;
   
   set->arr = arr;
   set->size = 0;
   set->maxSize = numCodes;
   return  set;
}

int NewCode(void *codeSet, char val) {
   CodeSet *temp = codeSet;
   CodeEntry *entryPtr = temp->arr;  
   int count = 0;
   
   while (count != temp->size) {
      ++entryPtr;
      count++;
   }
   temp->size++;
   entryPtr->code = val;
   entryPtr->prev = NULL;
   return count;
}

int ExtendCode(void *codeSet, int oldCode)
{
   CodeSet *temp = codeSet;
   CodeEntry *entryPtr = temp->arr;   
   CodeEntry *prevEntry = &entryPtr[oldCode];  
   int count = 0;
   
   while (count < temp->size) {
      ++entryPtr;
      count++;
   } 
   temp->size++;
   entryPtr->code = 0; 
   entryPtr->prev = prevEntry;
   return count;
}

void SetSuffix(void *codeSet, int code, char suffix) {
   CodeSet *tempSet = codeSet;
   CodeEntry *entryPtr = tempSet->arr;   
   int count = 0;
   
   while (count < code) {
      ++entryPtr;
      count++;
   } 
   entryPtr->code = suffix;
}

Code GetCode(void *codeSet, int code) {
   CodeSet *tempSet = codeSet;
   CodeEntry *curr, *counter, *fetch;
   Code outCode;
   int count = 1, size;
   
   curr = counter = fetch = tempSet->arr + code;
   while (counter->prev && (counter = counter->prev)) {
      ++count;
   }
   size = count;
   if (!curr->count) {
      curr->codePtr = malloc(count);
   }
   curr->count++;
   do {
      (curr->codePtr)[--count] = fetch->code;
   } while (fetch = fetch->prev);
   outCode.data = curr->codePtr;
   outCode.size = size;
   return outCode;
}

void FreeCode(void *codeSet, int code) {
   CodeSet *temp = codeSet;
   CodeEntry *entryPtr = &(temp->arr[code]); 
   
   if (entryPtr->count-- == 1) {
      free(entryPtr->codePtr); 
      entryPtr->codePtr = 0;      
   }
}

void DestroyCodeSet(void *codeSet) {
   CodeSet *codeSetPtr = codeSet;
   CodeEntry *arr = codeSetPtr->arr;   
   int count = 0;
   
   while (count++ < codeSetPtr->size) {
      if (arr->codePtr != 0) {
         free(arr->codePtr);
      }
      arr++;
   }
   free(codeSetPtr->arr);
   free(codeSetPtr);
}