#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef SMARTALLOC_H
#include "SmartAlloc.h"
#endif

#ifndef LZWCMP_H
#include "LZWCmp.h"
#endif

#define CHAR_BITS 9
#define UINT_BITS 32
 
static TreeNode *TreeRoot = NULL;
static TreeNode *FreeNodeList = NULL;

static void BuildInitialTree() {
   TreeNode *temp;
   int count = 0; 
   
   if (FreeNodeList) {
      TreeRoot = FreeNodeList;
      FreeNodeList = FreeNodeList->right;
      TreeRoot->right = NULL;
   }
   else {
      TreeRoot = calloc(1, sizeof(TreeNode));
   }
   TreeRoot->cNum = count;
   temp = TreeRoot;
   
   while (++count < NUM_SYMS) {
      if (FreeNodeList) {
         temp->right = FreeNodeList;
         FreeNodeList = FreeNodeList->right;
      }
      else {
         temp->right = calloc(1, sizeof(TreeNode));
      }
      temp = temp->right;
      temp->right = NULL;
      temp->left = NULL;
      temp->cNum = count;
   }
}

void LZWCmpInit(LZWCmp *cmp, CodeSink sink, void *sinkState, int recycleCode,
 int traceFlags) {
   int count = 0;
   
   BuildInitialTree();
   cmp->sink = sink;
   cmp->root = TreeRoot;
   cmp->sinkState = sinkState;
   cmp->recycleCode = recycleCode;
   
   cmp->traceFlags = traceFlags;
   cmp->numBits = CHAR_BITS;
   cmp->maxCode = 1 << cmp->numBits;
   cmp->curLoc = NULL;
   cmp->cst = CreateCodeSet(recycleCode + 1);
   cmp->bitsUsed = UINT_BITS;

   cmp->pCodeLimit = 0;
   cmp->pCode.data = calloc(SIZE_INCR, 1);
   cmp->pCode.size = 0;
   cmp->nextInt = 0;
   cmp->traceFlags = traceFlags;
   
   while (count <= NUM_SYMS) {
      NewCode(cmp->cst, count++);
   }  
}

static void PrintCodeEntry(Code in) {
   int count;
   
   printf("|");
   for (count = 0; count < in.size - 1; count++) {
      printf("%i ", in.data[count]);
   }
   printf("%i", in.data[count]);
}
  
static UInt PackNum(LZWCmp *cmp, int toCompress) {
   int count = 1;
   
   while (count <= cmp->numBits) {
      if (cmp->bitsUsed) {
         cmp->nextInt = cmp->nextInt | ((toCompress >> 
          (cmp->numBits - count)) & 1) << cmp->bitsUsed - 1;
         cmp->bitsUsed--;
         count++;
      }
      else {
         cmp->sink(cmp->sinkState, cmp->nextInt, 0);
         cmp->bitsUsed = UINT_BITS;
         cmp->nextInt = 0;
      }     
   }
} 

static void PrintCode(LZWCmp *cmp, int code) {
   if (cmp->traceFlags & TRACE_CODES) {
      printf("Sending code %d\n", code);
   }
}

static void DumpTree(LZWCmp *cmp, TreeNode *node) {
   if (node->left) {
      DumpTree(cmp, node->left);
   }
   PrintCodeEntry(GetCode(cmp->cst, node->cNum));
   FreeCode(cmp->cst, node->cNum);
   if (node->right) {
      DumpTree(cmp, node->right);
   }
}   

static void CheckForDump(LZWCmp *cmp) {
   if (cmp->traceFlags & TRACE_TREE) { 
      DumpTree(cmp, TreeRoot);
      printf("|\n\n");
   }
}

static void AddToFreeList(TreeNode *temp) {
   if (temp->left) {
      AddToFreeList(temp->left);
   }
   if (temp->right) {
      AddToFreeList(temp->right);
   }
   temp->left = NULL;
   temp->right = FreeNodeList;
   FreeNodeList = temp;
}

static int Minimum(int firstIn, int secondIn) {
   if (firstIn > secondIn) {
      firstIn = secondIn;
   }
   return firstIn;
}

static void RecycleCodes(LZWCmp *cmp) {
   int count = 0;
   
   if (cmp->traceFlags & TRACE_RECYCLES) {
      printf("Recycling dictionary...\n");
   }
   DestroyCodeSet(cmp->cst);
   cmp->cst = CreateCodeSet(cmp->recycleCode + 1);
   while (count <= NUM_SYMS) {
      NewCode(cmp->cst, count++);
   }  
   AddToFreeList(TreeRoot);
   TreeRoot = NULL;
   BuildInitialTree();
   cmp->curLoc = TreeRoot;
   cmp->numBits = CHAR_BITS;
   cmp->maxCode = 1 << cmp->numBits;
}

void CheckCodeCount(int codeCount, LZWCmp *cmp) {
   if (codeCount == cmp->recycleCode) {
      RecycleCodes(cmp);
   }
   else if (codeCount >= cmp->maxCode) {
      cmp->maxCode = cmp->maxCode << 1;
      cmp->numBits++;
      if (cmp->traceFlags & TRACE_BUMPS) {
         printf("Bump numBits to %d\n", cmp->numBits);
      }
   }
}

TreeNode *TravereTree(LZWCmp *cmp, int *allocRight, TreeNode **toAlloc) {
   int compResult, notFound = 1;
   TreeNode *temp = cmp->curLoc;
   
   while (temp && notFound) {
      *toAlloc = temp;
      cmp->curCode = GetCode(cmp->cst, temp->cNum);
      compResult = memcmp(cmp->curCode.data, cmp->pCode.data, 
       Minimum(cmp->curCode.size, cmp->pCode.size) * sizeof(char));
      FreeCode(cmp->cst, temp->cNum);
      if ((compResult == 0 && cmp->curCode.size >  cmp->pCode.size)  
       || compResult > 0) {
         temp = temp->left;
         if (!temp) {
            *allocRight = 0;
         }
      }
      else if ((compResult == 0  && cmp->curCode.size <  cmp->pCode.size) 
       || compResult < 0) {
         temp = temp->right;
         if (!temp) {
            *allocRight = 1;
         }
      }
      else {
         notFound = 0;
         cmp->curLoc = temp;
      } 
   }
   return temp;
}
   
void LZWCmpEncode(LZWCmp *cmp, UChar sym) {
   int count = 0;
   int curCodeSize, right = -1, compare, codeCount;
   TreeNode *result, *toAlloc;
   
   if (cmp->curLoc == NULL) {
      cmp->curLoc = TreeRoot;
   }
   
   if (!cmp->cst) {
      while (count <= NUM_SYMS) {
         NewCode(cmp->cst, count++);
      }
   }
   cmp->pCode.data[cmp->pCode.size] = sym;
   cmp->pCode.size++;
   result = TravereTree(cmp, &right, &toAlloc);
   if (!result) {
      if (right) {
         if (FreeNodeList) {
            toAlloc->right = FreeNodeList;
            FreeNodeList = FreeNodeList->right;
         } 
         else {
            toAlloc->right = calloc(1, sizeof(TreeNode));
         }
         toAlloc = toAlloc->right;
         toAlloc->right = NULL;
         toAlloc->left = NULL;
      }
      else {
         if (FreeNodeList) {
            toAlloc->left = FreeNodeList;
            FreeNodeList = FreeNodeList->right;
         } 
         else {
            toAlloc->left = calloc(1, sizeof(TreeNode));
         }
         toAlloc = toAlloc->left;
         toAlloc->right = NULL;
         toAlloc->left = NULL;
      }
      PackNum(cmp, cmp->curLoc->cNum);
      codeCount = ExtendCode(cmp->cst, cmp->curLoc->cNum);
      SetSuffix(cmp->cst, codeCount, sym);
      toAlloc->cNum = codeCount;
      PrintCode(cmp, cmp->curLoc->cNum);
      CheckCodeCount(codeCount, cmp);    
      cmp->pCodeLimit = 0;
      cmp->curLoc = NULL;
      cmp->pCode.size = 0;
      LZWCmpEncode(cmp, sym);     
      CheckForDump(cmp);
   }
}

void LZWCmpStop(LZWCmp *cmp) {
   if (cmp->curLoc) {
      PrintCode(cmp, cmp->curLoc->cNum);
      CheckForDump(cmp);
      PackNum(cmp, cmp->curLoc->cNum);
   }
   
   PrintCode(cmp, NUM_SYMS);
   CheckForDump(cmp);
   PackNum(cmp, NUM_SYMS);
   cmp->sink(cmp->sinkState, cmp->nextInt, 0);
   cmp->sink(cmp->sinkState, cmp->nextInt, 1);
}

void DestroyTree(TreeNode *temp) {
   if(temp) {
      DestroyTree(temp->left);
      DestroyTree(temp->right);
      free(temp);
   }
}



void LZWCmpDestruct(LZWCmp *cmp) {
   free(cmp->pCode.data);
   DestroyTree(TreeRoot);
   TreeRoot = NULL;
   DestroyCodeSet(cmp->cst);
}

void LZWCmpClearFreelist() {
   TreeNode *toFree;
   
   while (FreeNodeList) {
      toFree = FreeNodeList;
      FreeNodeList = FreeNodeList->right;
      free(toFree);
   }
}