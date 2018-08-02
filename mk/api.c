//
// Mikrotik API 2.0 // August 2, 2018.
//


#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

#include "md5.h"
#include "api.h"


// ********************************************************************
// ********************************************************************

void apiInitialize(void) {
   debug_ram = 0;
}

// ********************************************************************
// ********************************************************************

void apiTerminate(void) {
   if (debug_ram) printf("ERROR: Still using %ld bytes of RAM.\n",debug_ram);
}


// ********************************************************************
// parse
// ********************************************************************
// PARSE PARAMETERS OF TYPE =name=data
//
// name does not contain any equal signs or spaces.
// data can contain equal signs and/or spaces so read until NULL.
//
// Strip off the first = sign.
// Copy to name until next = sign.
// Copy to data until NULL.

int parse(char *strptr, char *nptr, char *dptr) {

   if (strptr == NULL) return(0);

   while ((*strptr) && (*strptr != '=')) strptr++;
   strptr++; // skip over equal sign.
   while ((*strptr) && (*strptr != '='))
      if (nptr) *nptr++ = *strptr++;
      else strptr++;
      if (nptr) *nptr = 0;
      strptr++; // skip over equal sign.
   while (*strptr)
      if (dptr) *dptr++ = *strptr++;
      else strptr++;
      if (dptr) *dptr = 0;

   return (1);
}



// ********************************************************************
// apiConnect
// ********************************************************************
// OPEN A SOCKET AND CONNECT TO THE SUPPLIED IP AND PORT.
//
// Supply an IP address and PORT then this routine will open the
// socket and establish a TCP connection with the host router.
// Return the socket or 0 if error.

int apiConnect(char *szIPaddr, int iPort) {
   int fdSock;
   struct sockaddr_in address;
   int iLen;

   fdSock = socket(AF_INET, SOCK_STREAM, 0);
   address.sin_family = AF_INET;
   address.sin_addr.s_addr = inet_addr(szIPaddr);
   address.sin_port = htons(iPort);
   iLen = sizeof(address);

   if (connect(fdSock, (struct sockaddr *)&address, iLen) == -1) return (0);
   else return (fdSock);
}


// ********************************************************************
// apiDisconnect
// ********************************************************************
// CLOSE THE SOCKET.

void apiDisconnect(int fdSock) {
   close(fdSock);
}


// ********************************************************************
// hexStringToChar
// ********************************************************************
// CONVERT 2 HEX CHARACTERS TO 1 BYTE BINARY
//
// Convert the text string "A4" to integer 164... etc. Does not need
// to be NULL terminated. Textual representation of hexadecimal
// number to binary byte.  Works on uppper/lower/mixed case.

char hexStringToChar(char *cToConvert) {
   char *lookup = "0123456789ABCDEF";
   int retval = 0;

   retval = 16 * (int)(strchr(lookup,toupper(cToConvert[0]))-lookup); 
   retval += (int)(strchr(lookup,toupper(cToConvert[1]))-lookup); 
   return (char)retval;
}

// ********************************************************************
// md5ToBinary
// ********************************************************************
// CONVERT 32 HEX CHARACTERS TO 16 BYTES BINARY
//
// MD5 helper function to convert a string of characters representing
// hexadecimal values to their binary equivalent.
//
// IMPORTANT: Free md5ToBinary when finished with it.

char *md5ToBinary(char *szHex) {
   int di;
   char *szReturn;

   // allocate 16 bytes for our return string
   szReturn = malloc(16 * sizeof(char));
   debug_ram += (16 * sizeof(char));

   if (strlen(szHex) != 32) return NULL; // 32 bytes in szHex?

   for (di = 0; di < 32; di += 2) szReturn[di/2] = hexStringToChar(&szHex[di]);

   return (szReturn);
}


// ********************************************************************
// md5DigestToHexString
// ********************************************************************
// CONVERT 16 BINARY BYTES TO 32 HEX CHARACTERS.
//
// MD5 helper function to calculate and return the hex string of
// an MD5 digest stored in binary.
//
// IMPORTANT:  Free md5DigestToHexString when finished with it.

char *md5DigestToHexString(unsigned char *binaryDigest) {
   int di;
   char *szReturn;

   // allocate 32 + 1 (for NULL) bytes for our return string
   szReturn = malloc((32 + 1) * sizeof(char));
   debug_ram += (33 * sizeof(char));

   for (di = 0; di < 16; ++di) {
      sprintf(szReturn + di * 2, "%02X", binaryDigest[di]);
   }

   return szReturn;
}


// ********************************************************************
// clearSentence
// ********************************************************************
// CLEAR AN EXISTING SENTENCE.
//
// Free memory used to hold words.
// Free memory used to hold an array of pointers to words.
// Set iLength to 0.

void clearSentence(struct Sentence *stSentence) {
   int i;
   
   if (stSentence->iLength == 0) return; // skip empty sentences.

   for (i = 0; i < stSentence->iLength; i++) { // free individual words
      debug_ram -= (strlen(stSentence->szWord[i]) + 1);
      free(stSentence->szWord[i]);
   }
   debug_ram -= (sizeof(char *) * stSentence->iLength);
   free(stSentence->szWord); // free pointer array

   stSentence->iLength = 0;
   stSentence->iReturnValue = 0;
}


// ********************************************************************
// initializeSentence
// ********************************************************************
// INITIALIZE A SENTENCE.
//
// Initialize a new sentence by setting iLength and iReturnValue = 0
// If I wanted to be a lazy programmer I would add the code here to
// check first to see if the sentence was already clear.  If NOT clear
// then I could run clearSentence to free the memory used by it.
// This would catch cases where I forgot to run clearSentence first.

void initializeSentence(struct Sentence *stSentence) {

   // if (stSentence->iLength > 0) clearSentence(stSentence);
   stSentence->iLength = 0;
   stSentence->iReturnValue = 0;
}


// ********************************************************************
// printSentence
// ********************************************************************
// PRINT A SENTENCE TO STDOUT.

void printSentence(struct Sentence *stSentence) {
   int i;

   for (i = 0; i < stSentence->iLength; i++) {
      printf("%s ",stSentence->szWord[i]);
   }
   printf("\n");
}


// ********************************************************************
// addWordToSentence
// ********************************************************************
// ADD A WORD TO A SENTENCE STRUCT.
//
// Allocate or reallocate memory to hold the array of pointers that
// point to the individual words.  Allocate memory for the word and
// copy the supplied string to the new word. Increase iLength.

void addWordToSentence(struct Sentence *stSentence, char *szWordToAdd) {

   if (stSentence->iLength == 0) { // allocate ram for array of word pointers or basically word[]
      stSentence->szWord = malloc(sizeof(char *));
      debug_ram += sizeof(char *);
   } else {
      stSentence->szWord = realloc(stSentence->szWord, (stSentence->iLength + 1) * sizeof(char *));
      debug_ram += sizeof(char *);
   }

   // allocate mem for the full word string incl NULL
   stSentence->szWord[stSentence->iLength] = malloc(strlen(szWordToAdd) + 1);
   debug_ram += (strlen(szWordToAdd) + 1);

   // copy word string to the sentence
   strcpy(stSentence->szWord[stSentence->iLength], szWordToAdd);

   // update iLength
   stSentence->iLength++;
}


// ********************************************************************
// addPartWordToSentence
// ********************************************************************
// APPEND WORD TO LAST WORD IN SENTENCE.
//
// Append the supplied string to the last word in the sentence.
// Useful for concatenation.

void addPartWordToSentence(struct Sentence *stSentence, char *szWordToAdd) {
   int i;
   i = stSentence->iLength - 1; // index of last word in sentence.

   // Reallocate memory for the new partial word.  size of both words plus a NULL.
   stSentence->szWord[i] = realloc(stSentence->szWord[i], strlen(stSentence->szWord[i]) + strlen(szWordToAdd) + 1);
   debug_ram += strlen(szWordToAdd); // already has a NULL

   // Concatenate the partial word to the existing sentence
   strcat (stSentence->szWord[i], szWordToAdd);

}


// ********************************************************************
// initializeBlock
// ********************************************************************
// INITIALIZE A BLOCK.
//
// Initialize block by setting iLength to 0.

void initializeBlock(struct Block *stBlock) {
   stBlock->iLength = 0;
}


// ********************************************************************
// clearBlock
// ********************************************************************
// CLEAR A BLOCK BY FREEING MEMORY.
//
// For each sentence, clear the words by calling clearSentence and
// then free the memory used by the Sentence structure itself.
// After that has completed, free the memory used to hold the
// pointers to the sentences.  Lastly set iLength = 0.

void clearBlock(struct Block *stBlock) {
   int i;

   if (stBlock->iLength == 0) return; // skip empty blocks.

   for (i = 0; i < stBlock->iLength; i++)  {
      clearSentence(stBlock->stSentence[i]); // free words.
      debug_ram -= sizeof(struct Sentence);
      free(stBlock->stSentence[i]); // sentence pointer from the array
   }
   debug_ram -= (sizeof(struct Sentence *) * stBlock->iLength);
   free(stBlock->stSentence); // pointer to array of sentence pointers
   initializeBlock(stBlock);
}


// ********************************************************************
// printBlock
// ********************************************************************
// PRINT A BLOCK TO STDOUT.

void printBlock(struct Block *stBlock) {
   int i;

   if (stBlock->iLength == 0) return; // skip empty blocks.

   for (i = 0; i < stBlock->iLength; i++) {
      printSentence(stBlock->stSentence[i]);
   }
}


// ********************************************************************
// findWord
// ********************************************************************
// SEARCH FOR AND RETURN A WORD FROM A SENTENCE.
//
// If the supplied sentence isn't of type DATA, return 0 meaning ERROR.
// Scan all words in the sentence for the supplied word.
// Once located, return a pointer to the word.
// NOTE: Partial matches work since we used strstr.

char *findWord(struct Sentence *stSentence, char *szITEM) {
   int i;

   if (stSentence->iReturnValue != DATA) return (0); // error not DATA.

   for (i = 0; i < stSentence->iLength; i++) {
      if (strstr(stSentence->szWord[i],szITEM)) return (stSentence->szWord[i]);
   }

   return (0); // error not found.
}


// ********************************************************************
// sortBlock
// ********************************************************************
// SORT A BLOCK.
//
// Bubble sort a block based on the specified word(s).  Ignore the
// last sentence in the block since it is normally = !done.  Alpha sort.

void sortBlock(struct Block *stBlock, char *Field1, char *Field2) {
   int i, j;
   char *ptr;
   struct Sentence *tmpSentence;
   char value1[256];
   char value2[256];

   for (i = 0; i < stBlock->iLength - 1; i++) { // bubble sort
      for (j = 0; j < (stBlock->iLength - i - 1); j++) { // -2 so we skip the !done entry
         if (!(ptr = findWord(stBlock->stSentence[j], Field1))) break;
         parse(ptr, NULL, value1);
         if (!(ptr = findWord(stBlock->stSentence[j + 1], Field1))) break;
         parse(ptr, NULL, value2);
         if (strcmp(value1, value2) > 0) { // exchange elements
            tmpSentence = stBlock->stSentence[j + 1];
            stBlock->stSentence[j + 1] = stBlock->stSentence[j];
            stBlock->stSentence[j] = tmpSentence;
         } else if (strcmp(value1, value2) == 0) {
            if (Field2 == NULL) continue;

            if (!(ptr = findWord(stBlock->stSentence[j], Field2))) break;
            parse(ptr, NULL, value1);
            if (!(ptr = findWord(stBlock->stSentence[j + 1], Field2))) break;
            parse(ptr, NULL, value2);

            if (strcmp(value1, value2) > 0) { // exchange elements
               tmpSentence = stBlock->stSentence[j + 1];
               stBlock->stSentence[j + 1] = stBlock->stSentence[j];
               stBlock->stSentence[j] = tmpSentence;
            }
         }
      }
   }
}


// ********************************************************************
// sortBlockID
// ********************************************************************
// SORT A BLOCK.
//
// Bubble sort a block based on the "=.id=" word.  Ignore the last
// sentence in the block since it is normally = !done.

void sortBlockID(struct Block *stBlock) {
   int i,j;
   char *ptr;
   struct Sentence *tmpSentence;
   char tmp[16];
   unsigned long value1, value2;

   for (i = 1; i < stBlock->iLength; ++i) { // bubble sort
      for (j = stBlock->iLength - 2; j >= i; --j) { // -2 so we skip the !done entry
         if (!(ptr = findWord(stBlock->stSentence[j-1],"=.id="))) break;
         sprintf(tmp,"0x%s",ptr+6);
         value1 = strtoul(tmp,0,16);
         if (!(ptr = findWord(stBlock->stSentence[j],"=.id="))) break;
         sprintf(tmp,"0x%s",ptr+6);
         value2 = strtoul(tmp,0,16);
         if (value1 > value2) { // exchange elements
            tmpSentence = stBlock->stSentence[j-1];
            stBlock->stSentence[j-1] = stBlock->stSentence[j];
            stBlock->stSentence[j] = tmpSentence;
         }
      }
   }
}


// ********************************************************************
// addSentenceToBlock
// ********************************************************************
// ADD A SENTENCE TO A BLOCK.
//
// Allocate or reallocate memory to hold a pointer to the sentence.
// Then allocate memory to hold the Sentence struct and copy the
// sentence to it.
//
// IMPORTANT: Use clearBlock when finished with it.

void addSentenceToBlock(struct Block *stBlock, struct Sentence *stSentence) {

   // stBlock only holds a pointer and an integer.  The pointer points to a block of
   // memory that is allocated and reallocated as needed to hold as many sentence
   // structures as we need.
   if (stBlock->iLength == 0) { // first sentence pointer.
      stBlock->stSentence = malloc(sizeof(struct Sentence *));
      debug_ram += sizeof(struct Sentence *);
   } else { // add one more sentence pointer.
      stBlock->stSentence = realloc(stBlock->stSentence, (stBlock->iLength + 1) * sizeof(struct Sentence *));
      debug_ram += sizeof(struct Sentence *);
   }

   // allocate memory for the new Sentence struct and copy the
   // supplied Sentence struct to it.
   stBlock->stSentence[stBlock->iLength] = malloc(sizeof(struct Sentence));
   debug_ram += sizeof(struct Sentence);
   memcpy(stBlock->stSentence[stBlock->iLength], stSentence,sizeof(struct Sentence));

   stBlock->iLength++;
}


// ********************************************************************
// writeLen
// ********************************************************************
// WRITE ENCODED MESSAGE LENGTH TO THE SOCKET.
//
// Encode message length and write it out to the socket.

void writeLen(int fdSock, int iLen) {
   char *cEncodedLength;  // encoded length to send to the api socket
   char *cLength;         // exactly what is in memory at &iLen integer

   cEncodedLength = calloc(sizeof(int), 1);
   cLength = (char *)&iLen; // set cLength address to be same as iLen

   if (iLen < 0x80) { // write 1 byte

      cEncodedLength[0] = (char)iLen;
      write(fdSock, cEncodedLength, 1);

   } else if (iLen < 0x4000) { // write 2 bytes

      cEncodedLength[0] = cLength[1] | 0x80;
      cEncodedLength[1] = cLength[0];
      write(fdSock, cEncodedLength, 2);

   } else if (iLen < 0x200000) { // write 3 bytes

      cEncodedLength[0] = cLength[2] | 0xc0;
      cEncodedLength[1] = cLength[1];
      cEncodedLength[2] = cLength[0];
      write(fdSock, cEncodedLength, 3);

   } else if (iLen < 0x10000000) { // write 4 bytes this code SHOULD work, but is untested

      cEncodedLength[0] = cLength[3] | 0xe0;
      cEncodedLength[1] = cLength[2];
      cEncodedLength[2] = cLength[1];
      cEncodedLength[3] = cLength[0];
      write(fdSock, cEncodedLength, 4);

   } else  { // this should never happen

      fprintf(stderr,"writeLen(): length of word is %d which is too long.\n", iLen);
      exit(1);

   }

   free(cEncodedLength);

}


// ********************************************************************
// writeWord
// ********************************************************************
// WRITE A WORD TO THE SOCKET.

void writeWord(int fdSock, char *szWord) {

   writeLen(fdSock, strlen(szWord));
   write(fdSock, szWord, strlen(szWord));
}


// ********************************************************************
// writeSentence
// ********************************************************************
// WITE A SENTENCE TO THE SOCKET.
//
// Write a sentence (multiple words) to the socket.  Follow the
// sentence with a blank word in accordance with the API.

void writeSentence(int fdSock, struct Sentence *stWriteSentence) {
   int i;
   
   if (stWriteSentence->iLength == 0) return; // nothing to write
   for (i = 0; i < stWriteSentence->iLength; i++) writeWord(fdSock, stWriteSentence->szWord[i]);
   writeWord(fdSock, "");
}

// ********************************************************************
// writeBlock
// ********************************************************************
// WRITE A BLOCK TO THE SOCKET.
//
// Write a block of sentences to the socket.  Follow the last sentence
// with a blank word in accordance with the API.

void writeBlock(int fdSock, struct Block *stBlock) {
   int i,j;

   if (stBlock->iLength == 0) return; // skip empty blocks.

   for (i = 0; i < stBlock->iLength; i++) {
      if (stBlock->stSentence[i]->iLength == 0) continue; // skip empty sentences
      for (j=0; j<stBlock->stSentence[i]->iLength; j++) writeWord(fdSock, stBlock->stSentence[i]->szWord[j]);
   }
   writeWord(fdSock, "");
}


// ********************************************************************
// readLen
// ********************************************************************
// READ THE MESSAGE LENGTH FROM THE SOCKET.
//
// A message length is itself between 1 and 4 bytes in length.
// The first byte returned determines how many bytes need to be read
// in order to receive the full message length.
//
// 80 = 10000000 (2 character encoded length)
// C0 = 11000000 (3 character encoded length)
// E0 = 11100000 (4 character encoded length)
//
// Message length is returned.  No memory is allocated.

int readLen(int fdSock) {
   char cFirstChar; // first character read from socket
   char cLength[sizeof(int)]; // length of next message to read...will be cast to int at the end
   int *iLen;       // calculated length of next message (Cast to int)

   read(fdSock, &cFirstChar, 1);

   // this code SHOULD work, but is untested

   if ((cFirstChar & 0xE0) == 0xE0) { // read 4 bytes
      cLength[3] = cFirstChar;
      cLength[3] &= 0x1f;        // mask out the 1st 3 bits
      read(fdSock, &cLength[2], 1);
      read(fdSock, &cLength[1], 1);
      read(fdSock, &cLength[0], 1);
   } else if ((cFirstChar & 0xC0) == 0xC0) { // read 3 bytes
      cLength[3] = 0;
      cLength[2] = cFirstChar;
      cLength[2] &= 0x3f;        // mask out the 1st 2 bits
      read(fdSock, &cLength[1], 1);
      read(fdSock, &cLength[0], 1);
   } else if ((cFirstChar & 0x80) == 0x80) { // read 2 bytes
      cLength[3] = 0;
      cLength[2] = 0;
      cLength[1] = cFirstChar;
      cLength[1] &= 0x7f;        // mask out the 1st bit
      read(fdSock, &cLength[0], 1);
   } else { // assume 1-byte encoded length.
      cLength[3] = 0;
      cLength[2] = 0;
      cLength[1] = 0;
      cLength[0] = cFirstChar;
   }

   iLen = (int *)cLength;

   return *iLen;
}


// ********************************************************************
// readWord
// ********************************************************************
// READ A WORD FROM THE SOCKET.
//
// Returns a pointer to the memory that was allocated in order
// to hold the word.
//
// IMPORTANT:  You must free readWord() when finished with it.
//             Free words added to sentences with clearSentence.

char *readWord(int fdSock) {
   int iBytesToRead = 0;
   int iBytesRead = 0;
   int iLen;
   char *szRetWord;
   char *szTmpWord;

   if ((iLen = readLen(fdSock)) <= 0) return (NULL); // how many bytes to read.

   // allocate memory for strings
   szRetWord = calloc(sizeof(char), iLen + 1); // allocate memory for read data plus NULL
   debug_ram += (sizeof(char) * (iLen + 1));
   szTmpWord = calloc(sizeof(char), 1024 + 1); // allocate memory for temp data plus a NULL

   while (iLen > 0) {
      iBytesToRead = iLen > 1024 ? 1024 : iLen; // limit to 1024 bytes max.
      iBytesRead = read(fdSock, szTmpWord, iBytesToRead); // read iBytesToRead from the socket
      szTmpWord[iBytesRead] = 0; // terminate szTmpWord
      strcat(szRetWord, szTmpWord); // concatenate szTmpWord to szRetWord
      iLen -= iBytesRead;
   }

   free(szTmpWord); // deallocate szTmpWord
   return (szRetWord);
}


// ********************************************************************
// readSentence
// ********************************************************************
// READ A SENTENCE FROM THE SOCKET.
//
// Read a sentence from the socket, allocate memory for the words and
// copy them to stReturnSentence.
//
// IMPORTANT:  You must free the sentence when finished with it.
//             Free sentences added to Blocks with clearBlock.

void readSentence(int fdSock, struct Sentence *stReturnSentence) {
   char *szWord;

   initializeSentence(stReturnSentence);

   while ((szWord = readWord(fdSock)) != NULL) { // must free szWord
      addWordToSentence(stReturnSentence, szWord);

      if (strstr(szWord, "!done") != NULL) stReturnSentence->iReturnValue = DONE;
      else if (strstr(szWord, "!re") != NULL) stReturnSentence->iReturnValue = DATA;
      else if (strstr(szWord, "!trap") != NULL) stReturnSentence->iReturnValue = TRAP;
      else if (strstr(szWord, "!fatal") != NULL) stReturnSentence->iReturnValue = FATAL;

      debug_ram -= (strlen(szWord) + 1);
      free(szWord);
   }
}


// ********************************************************************
// readBlock
// ********************************************************************
// READ A BLOCK FROM THE SOCKET.
//
// Read a block of sentences from the socket.  Keep reading sentences
// until you get !done or !fatal.  !trap can occur one or more times.
//
// Reply word (from Wiki)
//
// It is sent only by the router. It is only sent in response to full
// sentence sent by the client.
//
//    * Each sentence sent generates at least one reply
//      (if connection does not get terminated);
//    * First word of reply begins with '!';
//    * Errors and exceptional conditions begin with !trap;
//    * Data replies begin with !re
//    * If API connection is closed, RouterOS sends !fatal with reason
//      as reply and then closes the connection;
//    * Last reply for every sentence is reply that has first word !done;
//
// IMPORTANT:  Must free the block returned when done with it.

void readBlock(int fdSock, struct Block *stBlock) {
   struct Sentence stSentence;

   initializeBlock(stBlock);
   initializeSentence(&stSentence);

   do {
      readSentence(fdSock,&stSentence);
      addSentenceToBlock(stBlock,&stSentence);
      // We don't free &stSentence here since we're loading the block.
   } while ((stSentence.iReturnValue == DATA) || (stSentence.iReturnValue == TRAP));
}


// ********************************************************************
// login
// ********************************************************************
// LOGIN TO THE API.
//
// 1 is returned on successful login
// 0 is returned on error or user/password incorrect.

int login(int fdSock, char *username, char *password) {
   struct Sentence stReadSentence;
   struct Sentence stWriteSentence;
   char *szMD5ChallengeBinary;
   char *szMD5PasswordToSend;
   unsigned char digest[64];
   char tmp[256];
   MD5_CTX md5hash;

   writeWord(fdSock, "/login");
   writeWord(fdSock, "");

   readSentence(fdSock,&stReadSentence);

   if (stReadSentence.iReturnValue != DONE) {
      fprintf(stderr,"login(): error logging in.\n");
      clearSentence(&stReadSentence);
      exit(0);
   }
   // extract md5 string from the challenge sentence
   parse(stReadSentence.szWord[1],tmp,tmp);
   szMD5ChallengeBinary = md5ToBinary((char *)tmp);
   clearSentence(&stReadSentence);

   // get md5 of the 0x00 + password + challenge concatenation
   tmp[0]=0;
   memcpy(&tmp[1],password,strlen(password));
   memcpy(&tmp[1+strlen(password)],szMD5ChallengeBinary,16);

   MD5_Init(&md5hash);
   MD5_Update(&md5hash,tmp,1+strlen(password)+16);
   MD5_Final(digest,&md5hash);
   debug_ram -= (16 * sizeof(char));
   free(szMD5ChallengeBinary);

   // convert this digest to a string representation of the hex values
   // digest is the binary format of what we want to send
   // szMD5PasswordToSend is the "string" hex format
   szMD5PasswordToSend = md5DigestToHexString(digest); // must be free'd


   // put together the login sentence
   initializeSentence(&stWriteSentence);

   addWordToSentence(&stWriteSentence, "/login");
   addWordToSentence(&stWriteSentence, "=name=");
   addPartWordToSentence(&stWriteSentence, username);
   addWordToSentence(&stWriteSentence, "=response=00");
   addPartWordToSentence(&stWriteSentence, szMD5PasswordToSend);
   writeSentence(fdSock, &stWriteSentence);
   clearSentence(&stWriteSentence);
   debug_ram -= (33 * sizeof(char));
   free(szMD5PasswordToSend);

   readSentence(fdSock,&stReadSentence);

   if (stReadSentence.iReturnValue == DONE) {
      clearSentence(&stReadSentence);
      return 1;
   } else {
      clearSentence(&stReadSentence);
      return 0;
   }
}


// ********************************************************************
// login
// ********************************************************************
// LOGIN TO THE API.
//
// 1 is returned on successful login
// 0 is returned on error or user/password incorrect.

int login_643(int fdSock, char *username, char *password) {
   struct Sentence stReadSentence;
   struct Sentence stWriteSentence;

   initializeSentence(&stWriteSentence);
   addWordToSentence(&stWriteSentence,"/login");
   addWordToSentence(&stWriteSentence,"=name=");
   addPartWordToSentence(&stWriteSentence,username);
   addWordToSentence(&stWriteSentence,"=password=");
   addPartWordToSentence(&stWriteSentence,password);
   writeSentence(fdSock, &stWriteSentence);
   clearSentence(&stWriteSentence);

   readSentence(fdSock,&stReadSentence);

   if (stReadSentence.iReturnValue != DONE) {
      fprintf(stderr,"login(): error logging in.\n");
      clearSentence(&stReadSentence);
      return(0);
   }

   clearSentence(&stReadSentence);
   return 1;
}


// ********************************************************************
// ********************************************************************
// ********************************************************************
// ********************************************************************
