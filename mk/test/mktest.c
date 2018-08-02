//
// mktest.c v2.0 // August 2, 2018.
//
// Login to the specified router and execute commands via the API.
//
// Enter {BLANK LINK} to send the Sentence
// Enter quit to end session.
//

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "../api.h"

int iPort = 8728;

/********************************************************************
 ********************************************************************/

int main(int argc, char *argv[])
{
   int fdSock;
   char cWordInput[256];        // limit user word input to 256 chars
   char *szNewline;             // used for word input from the user
   struct Sentence stSentence;
   struct Block stBlock;

   apiInitialize();

   if (argc != 4) {
      fprintf(stderr,"USAGE: %s ip user pass\n",argv[0]);
      exit(1);
   }

   printf("Connecting to API: %s:%d\n", argv[1], iPort);
   fdSock = apiConnect(argv[1], iPort);
   if (login(fdSock, argv[2], argv[3]) == 0) {
      apiDisconnect(fdSock);
      printf("Invalid username or password.\n");
      exit(1);
   }
   initializeSentence(&stSentence);

   while (1) {
      fputs("<<< ", stdout); fflush(stdout);
      
      if (fgets(cWordInput, sizeof cWordInput, stdin) != NULL) {
         szNewline = strchr(cWordInput, '\n');
         if (szNewline != NULL) *szNewline = '\0';
      }

      if (strcmp(cWordInput, "quit") == 0) { // check to see if we want to quit
         break;
      } else if (strcmp(cWordInput, "") == 0) { // check for end of sentence (\n)
         if (stSentence.iLength > 0) { // write sentence to the API
            writeSentence(fdSock, &stSentence);
            clearSentence(&stSentence); // clear the sentence
            readBlock(fdSock,&stBlock); // receive and print response block from the API
            printBlock(&stBlock);
            clearBlock(&stBlock);
         }
      } else { // if nothing else, simply add the word to the sentence
         addWordToSentence(&stSentence, cWordInput);
      }
   }
   if (stSentence.iLength > 0) clearSentence(&stSentence);
   apiDisconnect(fdSock);
   apiTerminate();
   exit(0);
} 
