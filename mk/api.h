//
// Mikrotik API 2.0 // August 2, 2018.
//

#ifndef MK_API
#define MK_API

#define DONE 1
#define TRAP 2
#define FATAL 3
#define DATA 4

// struct Sentence
//
// A Sentence structure contains one pointer and two integers.  That is all.
// If you have multiple sentences, then you have multiple Sentence structures.
// The words in each sentence (pointers) are stored in an array that is
// dynamically allocated and reallocated as words are added.  The memory for
// the string data is also allocated as needed.  iLength is an integer that
// indicates how many words are in the sentence.  **szWord is a pointer to a
// block of memory that itself contains pointers to the individual word strings.

struct Sentence {
        char **szWord;    // array of pointers.  one for each word string
        int iLength;      // length of szWord (number of array elements)
        int iReturnValue; // return value of sentence reads from API
};

// struct Block
//
// A Block structure contains one pointer and one integer.
// iLength tells us how many Sentence structures the block holds.
// **stSentence is a pointer to a block of memory where you will
// find Sentence structures stored in an array.

struct Block {
        struct Sentence **stSentence; // pointer to array of Sentences.
        int iLength; // length of stSentence (number of pointers in array)
};

long debug_ram;

void apiInitialize(void);
void apiTerminate(void);
int parse(char *, char *, char *);
int apiConnect(char *szIPaddr, int iPort);
void apiDisconnect(int fdSock);
char hexStringToChar(char *cToConvert);
char *md5ToBinary(char *szHex);
char *md5DigestToHexString(unsigned char *binaryDigest);
void clearSentence(struct Sentence *stSentence);
void initializeSentence(struct Sentence *stSentence);
void printSentence(struct Sentence *stSentence);
void addWordToSentence(struct Sentence *stSentence, char *szWordToAdd);
void addPartWordToSentence(struct Sentence *stSentence, char *szWordToAdd);
void initializeBlock(struct Block *stBlock);
void clearBlock(struct Block *stBlock);
void printBlock(struct Block *stBlock);
char *findWord(struct Sentence *stSentence, char *szITEM);
void sortBlock(struct Block *stBlock, char *Field1, char *Field2);
void sortBlockID(struct Block *stBlock);
void addSentenceToBlock(struct Block *stBlock, struct Sentence *stSentence);
void writeLen(int fdSock, int iLen);
void writeWord(int fdSock, char *szWord);
void writeSentence(int fdSock, struct Sentence *stWriteSentence);
void writeBlock(int fdSock, struct Block *stBlock);
int readLen(int fdSock);
char *readWord(int fdSock);
void readSentence(int fdSock, struct Sentence *stReturnSentence);
void readBlock(int fdSock, struct Block *stBlock);
int login(int fdSock, char *username, char *password);

#endif // MK_API
