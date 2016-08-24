#ifndef LZWCMP_H
#define LZWCMP_H

#include "MyLib.h"
#include "CodeSet.h"

#define NUM_SYMS 256
#define SIZE_INCR 1024

// Trace flags
#define TRACE_TREE  0x1    // Dump the tree at each code transmit
#define TRACE_BUMPS 0x2    // Announce each bump in bit count
#define TRACE_RECYCLES 0x4 // Announce each dictionary recycle
#define TRACE_CODES 0x8    // Announce each transmission of a code

/* Function pointer to method to call when a code is completed and ready for
 * transmission or whatever.  The |state| parameter can point to anything,
 * and gives hidden information to the function so that it can know what
 * file, socket, etc. the code is going to.  |code| is the next 32 bits
 * worth of compressed output.  Parameter |done| is false on all calls, except
 * a final call indicating end of data.  On this final call |code| has no valid data,
 * and |done| is true.
 */
typedef void (*CodeSink)(void *state, UInt code, int done);

/* One node in a BST representing the current dictionary.  Traverse the
 * tree in increments, with multiple LZWCmpEncode calls contibuting to a
 * single travers, until reaching a point where a new TreeNode should be
 * added.  At this point, generate a new LZW code, and a new dictionary
 * entry, and start over.  Do not expand cNum into a full symbol sequence
 * for more than one or two nodes at a time.
 */
typedef struct TreeNode {
   int cNum;
   struct TreeNode *right;
   struct TreeNode *left;
} TreeNode;


/* Current state of the LZW compressor. */
typedef struct LZWCmp {
   TreeNode *root;   /* Pointer to BST root */
   CodeSink sink;    /* Code sink to send bits to */
   void *sinkState;  /* Unknown object to send to sink for state */
   int numBits;      /* Number of bits per code currently */
   int maxCode;      /* Max code that fits in numBits bits */
   int recycleCode;  /* Code that triggers recycling of dictionary */
   UInt nextInt;     /* Partially-assembled next int of output */
   int bitsUsed;     /* Number of valid bits in top portion of nextInt */
   void *cst;        /* CodeSet holding codes thus far created */
   TreeNode *curLoc; /* Current position in tree */
   Code curCode;     /* Code held by *curLoc */
   Code pCode;       /* Code holding partially-done new code */
   int pCodeLimit;   /* Size of pCode data */
   int traceFlags;   /* Bit flags indicating trace actions to take */
} LZWCmp;

/* Initialize a LZWCmp given the CodeSink to which to send completed codes. */
void LZWCmpInit(LZWCmp *cmp, CodeSink sink, void *sinkState, int recycleCode,
 int traceFlags);

/* Encode "sym" using LZWCmp. Zero or more calls of the code sink
 * may result */
void LZWCmpEncode(LZWCmp *cmp, UChar sym);

/* Mark end of encoding (send next code value to code sink) */
void LZWCmpStop(LZWCmp *cmp);

/* Free all storage associated with LZWCmp (not the sinkState, though,
 * which is "owned" by the caller */
void LZWCmpDestruct(LZWCmp *cmp);

/* Clear freelist.  This method is static because the freelist is shared
 * among all LZWCmp objects, and should be cleared only when all LZWCmp
 * objects are destroyed. */
void LZWCmpClearFreelist();

#endif
