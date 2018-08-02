//
// mkclone.c v2.0 // August 2, 2018.
//

//  This program connects to a hard coded MIKROTIK router and downloads:
//
//  /ip/firewall/filter
//  /ip/filewall/mangle
//  /ip/firewall/address-list
//
//  It then disconnects and connects to the specified TARGET router.
//  Disables all firewall filter DROP, REJECT and TARPIT rules before
//  proceeding to erase all firewall filter rules that do not have a
//  comment beginning with an '@'.  By disabling all the DROP, REJECT
//  and TARPIT rules first, we ensure that we don't lose connectivity
//  with the router when we go to remove all the rules.  This also makes
//  it possible to run this on a live router.  Existing connections will
//  not be disconnected.
//
//  WARNING:  Your router will have it's firewall disabled for
//            a few seconds.
//
//  Now it loads all the FILTER rules from the MASTER router.  Any rule
//  that has a comment beginning with an '@' will be ignored.  While
//  loading new rules, it will load any DROP, REJECT or TARPIT rules
//  "disabled" to prevent any accidental blocks until everything
//  is fully loaded.
//
//  Now we erase and load Mangle and Address-Lists's.  Any rule that
//  begins with an '@' will be ignored.  Once this is complete, go back
//  and turn on any "previously disabled" filter rules.
//
//  Lastly erase all firewall tracking connections.  Should someone try
//  to sneak through the firewall while it was disabled and establish
//  a connection, we want to make sure they get processed by the new
//  filter rules.  So we delete all existing/old connections.
//
//  h0rhay

// fill in your values here before compiling.

char *szIPaddr1  = "0.0.0.0";
char *szPort     = "8728";
char *szUsername = "admin";
char *szPassword = "password";

// don't touch below here.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../api.h"

// ********************************************************************
// ********************************************************************
// ********************************************************************
// ********************************************************************


int main(int argc, char *argv[]) {
   int fdSock;
   int iPort;
   int iLoginResult;
   char cWordInput[256]; // limit user word input to 256 chars
   int i,j,k; // temporary loop and flag variables.
   char *ptr;
   struct Sentence stSentence;
   struct Block stBlockFILTER; // MASTER router FILTER rules.
   struct Block stBlockMANGLE; // MASTER router MANGLE rules.
   struct Block stBlockADDRESS; // MASTER router ADDRESS lists.
   struct Block stBlockTMP;
   struct Block stBlockRESULT; // TARGET router command response.


// 0. Check command line arguments.

   apiInitialize();

   stSentence.iLength=0;
   stSentence.iReturnValue = 0;

   if (argc!=2) {
      fprintf(stderr,"USAGE: %s ip_address\n",argv[0]);
      exit(1);
   }


// 1. Use apiConnect to connect to the MASTER router and login.

   printf("( 1/14): Connect to MASTER router: %s\n",szIPaddr1);

   iPort = atoi(szPort);
   fdSock = apiConnect(szIPaddr1, iPort);
   if (!(iLoginResult = login(fdSock, szUsername, szPassword))) {
       apiDisconnect(fdSock);
       printf("Invalid username or password.\n");
       exit(1);
   }


// 2. Read in all firewall settings from the MASTER router.

   printf("( 2/14): Download firewall configuration.\n");

   writeWord(fdSock,"/ip/firewall/filter/print"); // load all FILTER rules.
   writeWord(fdSock,"");
   readBlock(fdSock,&stBlockFILTER);

   writeWord(fdSock,"/ip/firewall/mangle/print"); // load all MANGLE rules.
   writeWord(fdSock,"");
   readBlock(fdSock,&stBlockMANGLE);

   writeWord(fdSock,"/ip/firewall/address-list/print"); // load all ADDRESS-LIST's.
   writeWord(fdSock,"");
   readBlock(fdSock,&stBlockADDRESS);


// 3. We have everything we need from the master router; time to disconnect.
 
   printf("( 3/14): Disconnect from MASTER router: %s\n",szIPaddr1);
   apiDisconnect(fdSock); 


// 4. Connect to the target router.

   printf("( 4/14): Connect to TARGET router: %s\n",argv[1]);

   iPort = atoi(szPort);
   fdSock = apiConnect(argv[1], iPort);
   if (!(iLoginResult = login(fdSock, szUsername, szPassword))) {
      apiDisconnect(fdSock);
      clearBlock(&stBlockADDRESS);
      clearBlock(&stBlockMANGLE);
      clearBlock(&stBlockFILTER);
      printf("Invalid username or password.\n");
      exit(1);
   }


// 5. Disable any DROP, REJECT or TARPIT rules in firewall. Does not
//    matter if it has a comment beginning with '@'.

   printf("( 5/14): Disable TARGET firewall DROP, REJECT and TARPIT filters.\n");

   writeWord(fdSock,"/ip/firewall/filter/print");
   writeWord(fdSock,"?=action=drop");
   writeWord(fdSock,"?=action=reject");
   writeWord(fdSock,"?#|");
   writeWord(fdSock,"?=action=tarpit");
   writeWord(fdSock,"?#|");
   writeWord(fdSock,"");
   readBlock(fdSock,&stBlockTMP);
   if (stBlockTMP.iLength > 1) { // If two or more results.  Remember one is the !done.
      for (i = 0; i < stBlockTMP.iLength - 1; i++) { // ignore !done at end of block.
         printf("%%%3.0f\r",100*(float)i/(float)stBlockTMP.iLength); fflush(stdout);
         addWordToSentence(&stSentence,"/ip/firewall/filter/set");
         ptr=findWord(stBlockTMP.stSentence[i],"=.id=");
         sprintf(cWordInput,"=.id=*%s",ptr+6);
         addWordToSentence(&stSentence, cWordInput);
         addWordToSentence(&stSentence,"=disabled=yes"); // disable drop rules.
         writeSentence(fdSock, &stSentence);
         clearSentence(&stSentence);
         readBlock(fdSock,&stBlockRESULT);
         clearBlock(&stBlockRESULT);
      }
   }
   clearBlock(&stBlockTMP); // clear either the filter list or the response block.


// 6. next we need to erase all firewall rules that don't have a comment beginning with '@'.

   printf("( 6/14): Remove TARGET firewall filter rules.\n");

   addWordToSentence(&stSentence,"/ip/firewall/filter/print");
   writeSentence(fdSock, &stSentence);
   clearSentence(&stSentence);
   readBlock(fdSock,&stBlockTMP);

   if (stBlockTMP.iLength > 2) { // If two or more results.  Remember one is the !done
      for (i = 0; i < stBlockTMP.iLength - 1; i++) { // ignore !done at end of block.
         printf("%%%3.0f\r",100*(float)i/(float)stBlockTMP.iLength); fflush(stdout);
         if (((ptr=findWord(stBlockTMP.stSentence[i],"=comment=")) == 0) || (*(ptr+9) != '@')) {
            addWordToSentence(&stSentence,"/ip/firewall/filter/remove");
            ptr=findWord(stBlockTMP.stSentence[i],"=.id=");
            sprintf(cWordInput,"=.id=*%s",ptr+6);
            addWordToSentence(&stSentence, cWordInput);
            writeSentence(fdSock, &stSentence);
            clearSentence(&stSentence);
            readBlock(fdSock,&stBlockRESULT); // read response to our command.
            clearBlock(&stBlockRESULT);
         }
      }
   }
   clearBlock(&stBlockTMP); // don't need list of entries to disable anymore.


// 7. Add the list of firewall filter rules from the master router.  Make sure
// you load drops disabled. Skip entries with a comment beginning with '@'.

   printf("( 7/14): Load new filter rules with DROP, REJECT and TARPIT disabled.\n");

   for (i = 0; i < stBlockFILTER.iLength - 1; i++) { // ignore !done at end of block.
      printf("%%%3.0f\r",100*(float)i/(float)stBlockFILTER.iLength); fflush(stdout);
      if (((ptr=findWord(stBlockFILTER.stSentence[i],"=comment=")) == 0) || (*(ptr+9) != '@')) {
         k=0;
         addWordToSentence(&stSentence,"/ip/firewall/filter/add");

         if (findWord(stBlockFILTER.stSentence[i],"=action=drop") != 0) k=1; // flag drop
         if (findWord(stBlockFILTER.stSentence[i],"=action=reject") != 0) k=1; // flag reject
         if (findWord(stBlockFILTER.stSentence[i],"=action=tarpit") != 0) k=1; // flag tarpit

         for (j=1; j<stBlockFILTER.stSentence[i]->iLength; j++) {
            // if this is a drop, reject or tarpit entry, strip off the disabled option and re-add later.
            if (k && (strncmp(stBlockFILTER.stSentence[i]->szWord[j],"=disabled=",10) == 0)) continue;

            if (strncmp(stBlockFILTER.stSentence[i]->szWord[j],"=.id=",5) == 0) continue;
            if (strncmp(stBlockFILTER.stSentence[i]->szWord[j],"=bytes=",7) == 0) continue;
            if (strncmp(stBlockFILTER.stSentence[i]->szWord[j],"=packets=",9) == 0) continue;
            if (strncmp(stBlockFILTER.stSentence[i]->szWord[j],"=invalid=",9) == 0) continue;
            if (strncmp(stBlockFILTER.stSentence[i]->szWord[j],"=dynamic=",9) == 0) continue;
            if (strncmp(stBlockFILTER.stSentence[i]->szWord[j],"=creation-time=",15) == 0) continue;

            addWordToSentence(&stSentence,stBlockFILTER.stSentence[i]->szWord[j]);
         }
         if (k) { // if this was a drop then add disabled back in.
            addWordToSentence(&stSentence,"=disabled=yes");
         }

         writeSentence(fdSock,&stSentence);
         clearSentence(&stSentence);

         readBlock(fdSock,&stBlockTMP); // read response to our command.
         clearBlock(&stBlockTMP);
      }
   }

   clearBlock(&stBlockFILTER);


// 8. Erase all mangle rules that don't have a comment beginning with '@'.

   printf("( 8/14): Remove TARGET firewall mangle rules.\n");

   addWordToSentence(&stSentence,"/ip/firewall/mangle/print");
   writeSentence(fdSock, &stSentence);
   clearSentence(&stSentence);
   readBlock(fdSock,&stBlockTMP);

   if (stBlockTMP.iLength > 2) { // If two or more results.  Remember one is the !done
      for (i = 0; i < stBlockTMP.iLength - 1; i++) { // ignore !done at end of block.
         printf("%%%3.0f\r",100*(float)i/(float)stBlockTMP.iLength); fflush(stdout);
         if (((ptr=findWord(stBlockTMP.stSentence[i],"=comment=")) == 0) || (*(ptr+9) != '@')) {
            addWordToSentence(&stSentence,"/ip/firewall/mangle/remove");
            ptr=findWord(stBlockTMP.stSentence[i],"=.id=");
            sprintf(cWordInput,"=.id=*%s",ptr+6);
            addWordToSentence(&stSentence, cWordInput);
            writeSentence(fdSock, &stSentence);
            clearSentence(&stSentence);
            readBlock(fdSock,&stBlockRESULT); // read response to our command.
            clearBlock(&stBlockRESULT);
         }
      }
   }
   clearBlock(&stBlockTMP); // don't need list of entries to disable anymore.


// 9. add the list of firewall mangle rules from the master router.

   printf("( 9/14): Load new mangle rules.\n");

   for (i = 0; i < stBlockMANGLE.iLength; i++) { // ignore !done at end of block.
      printf("%%%3.0f\r",100*(float)i/(float)stBlockMANGLE.iLength); fflush(stdout);

      if (stBlockMANGLE.stSentence[i]->iReturnValue != DATA) {
         continue; // skip response sentences.
      }

      addWordToSentence(&stSentence,"/ip/firewall/mangle/add");

      for (j=1; j<stBlockMANGLE.stSentence[i]->iLength; j++) {
         if (strncmp(stBlockMANGLE.stSentence[i]->szWord[j],"=.id=",5) == 0) continue;
         if (strncmp(stBlockMANGLE.stSentence[i]->szWord[j],"=bytes=",7) == 0) continue;
         if (strncmp(stBlockMANGLE.stSentence[i]->szWord[j],"=packets=",9) == 0) continue;
         if (strncmp(stBlockMANGLE.stSentence[i]->szWord[j],"=invalid=",9) == 0) continue;
         if (strncmp(stBlockMANGLE.stSentence[i]->szWord[j],"=dynamic=",9) == 0) continue;
         if (strncmp(stBlockMANGLE.stSentence[i]->szWord[j],"=creation-time=",15) == 0) continue;
         addWordToSentence(&stSentence,stBlockMANGLE.stSentence[i]->szWord[j]);
      }

      if ((ptr = findWord(stBlockMANGLE.stSentence[i],"=comment=")) != 0) {
         parse(ptr,cWordInput,cWordInput);
         if (cWordInput[0] != '@') {
            writeSentence(fdSock,&stSentence);
            clearSentence(&stSentence);
            readBlock(fdSock,&stBlockTMP); // read response to our command.
            clearBlock(&stBlockTMP);
         } else {
            clearSentence(&stSentence);
         }
      } else {
         writeSentence(fdSock,&stSentence);
         clearSentence(&stSentence);
         readBlock(fdSock,&stBlockTMP); // read response to our command.
         clearBlock(&stBlockTMP);
      }
   }

   clearBlock(&stBlockMANGLE);


// 10. next we need to erase all address-list entries that don't have a comment beginning with '@'.

   printf("(10/14): Remove TARGET firewall address-lists.\n");

   writeWord(fdSock,"/ip/firewall/address-list/print");
   writeWord(fdSock,"");
   readBlock(fdSock,&stBlockTMP);

   if (stBlockTMP.iLength > 2) { // If two or more results.  Remember one is the !done
      for (i = 0; i < stBlockTMP.iLength - 1; i++) { // ignore !done at end of block.
         printf("%%%3.0f\r",100*(float)i/(float)stBlockTMP.iLength); fflush(stdout);
         if (((ptr=findWord(stBlockTMP.stSentence[i],"=comment=")) == 0) || (*(ptr+9) != '@')) {
            addWordToSentence(&stSentence,"/ip/firewall/address-list/remove");
            ptr=findWord(stBlockTMP.stSentence[i],"=.id=");
            sprintf(cWordInput,"=.id=*%s",ptr+6);
            addWordToSentence(&stSentence, cWordInput);
            writeSentence(fdSock, &stSentence);
            clearSentence(&stSentence);
            readBlock(fdSock,&stBlockRESULT); // read response to our command.
            clearBlock(&stBlockRESULT);
         }
      }
   }
   clearBlock(&stBlockTMP); // don't need list of entries to disable anymore.


// 11. add the list of firewall address-list entries from the master router.

   printf("(11/14): Load new address-lists.\n");

   for (i = 0; i < stBlockADDRESS.iLength - 1; i++) { // ignore !done at end of block.
      printf("%%%3.0f\r",100*(float)i/(float)stBlockADDRESS.iLength); fflush(stdout);
      if (((ptr=findWord(stBlockADDRESS.stSentence[i],"=comment=")) == 0) || (*(ptr+9) != '@')) {
         addWordToSentence(&stSentence,"/ip/firewall/address-list/add");

         for (j = 0; j < stBlockADDRESS.stSentence[i]->iLength; j++) {
            if (strncmp(stBlockADDRESS.stSentence[i]->szWord[j],"=.id=",5) == 0) continue;
            if (strncmp(stBlockADDRESS.stSentence[i]->szWord[j],"=bytes=",7) == 0) continue;
            if (strncmp(stBlockADDRESS.stSentence[i]->szWord[j],"=packets=",9) == 0) continue;
            if (strncmp(stBlockADDRESS.stSentence[i]->szWord[j],"=invalid=",9) == 0) continue;
            if (strncmp(stBlockADDRESS.stSentence[i]->szWord[j],"=dynamic=",9) == 0) continue;
            if (strncmp(stBlockADDRESS.stSentence[i]->szWord[j],"=creation-time=",15) == 0) continue;
            addWordToSentence(&stSentence,stBlockADDRESS.stSentence[i]->szWord[j]);
         }

         writeSentence(fdSock,&stSentence);
         clearSentence(&stSentence);

         readBlock(fdSock,&stBlockTMP); // read response to our command.
         clearBlock(&stBlockTMP);
      }
   }

   clearBlock(&stBlockADDRESS);


// 12. Re-enable any disabled firewall rules.  Does not matter if they have
//     a comment beginning with an '@'.

   printf("(12/14): Enable TARGET firewall DROP, REJECT and TARPIT filters.\n");

   writeWord(fdSock,"/ip/firewall/filter/print");
   writeWord(fdSock,"?=action=drop");
   writeWord(fdSock,"?=action=reject");
   writeWord(fdSock,"?#|");
   writeWord(fdSock,"?=action=tarpit");
   writeWord(fdSock,"?#|");
   writeWord(fdSock,"");
   readBlock(fdSock,&stBlockTMP);
   if (stBlockTMP.iLength > 1) { // If two or more results.  Remember one is the !done
      for (i = 0; i < stBlockTMP.iLength - 1; i++) { // ignore !done at end of block.
         printf("%%%3.0f\r",100*(float)i/(float)stBlockTMP.iLength); fflush(stdout);
         addWordToSentence(&stSentence,"/ip/firewall/filter/set");
         ptr=findWord(stBlockTMP.stSentence[i],"=.id=");
         sprintf(cWordInput,"=.id=*%s",ptr+6);
         addWordToSentence(&stSentence, cWordInput);
         addWordToSentence(&stSentence,"=disabled=no"); // enable drop rules.
         writeSentence(fdSock, &stSentence);
         clearSentence(&stSentence);
         readBlock(fdSock,&stBlockRESULT);
         clearBlock(&stBlockRESULT);
      }
   }
   clearBlock(&stBlockTMP); // clear the filter list.


// 13. clear old firewall connections

   printf("(13/14): Reset TARGET firewall connection tracking.\n");

   addWordToSentence(&stSentence,"/ip/firewall/connection/print");
   writeSentence(fdSock, &stSentence);
   clearSentence(&stSentence);
   readBlock(fdSock,&stBlockTMP);

   if (stBlockTMP.iLength > 1) { // If two or more results.  Remember one is the !done
      for (i = 0; i < stBlockTMP.iLength - 1; i++) { // ignore !done at end of block.
         printf("%%%3.0f\r",100*(float)i/(float)stBlockTMP.iLength); fflush(stdout);
         addWordToSentence(&stSentence,"/ip/firewall/connection/remove");
         ptr=findWord(stBlockTMP.stSentence[i],"=.id=");
         sprintf(cWordInput,"=.id=*%s",ptr+6);
         addWordToSentence(&stSentence, cWordInput);
         writeSentence(fdSock, &stSentence);
         clearSentence(&stSentence);
         readBlock(fdSock,&stBlockRESULT);
         if (stBlockRESULT.iLength >1) {
            if  (strcmp(stBlockRESULT.stSentence[stBlockRESULT.iLength-2]->szWord[0], "!trap") == 0) {
               printSentence(stBlockRESULT.stSentence[stBlockRESULT.iLength-2]);
            }
         }
         clearBlock(&stBlockRESULT);
      }
   }
   clearBlock(&stBlockTMP); // clear the connection list.


// 14. disconnect from target router.

   printf("(14/14): Disconnect from TARGET router: %s\n",argv[1]);
   apiDisconnect(fdSock);
   apiTerminate();
   return (0);
}

// END OF PROGRAM mkclone.c
