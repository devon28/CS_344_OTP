//#########################################################
// Author: Devon Miller
// Date: 3/6/23
// Course: CS_344
// Assignment: OTP
// Program: dec_server
//#########################################################

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_BUFFER 1000000    // max sending size

char * decryptMessage;

void error(const char *msg);

void setupAddressStruct(struct sockaddr_in* address, int portNumber);

char* decryption(char* msg, char *key);

int main(int argc, char *argv[]){
  int connectionSocket, charsRead, charsSent;
  char buffer[MAX_BUFFER];
  struct sockaddr_in serverAddress, clientAddress;
  socklen_t sizeOfClientInfo = sizeof(clientAddress);
  pid_t pid;
  if (argc != 2) {     // check correct usage
    fprintf(stderr,"USAGE: %s port\n", argv[0]); 
    exit(1);
  } 
  
  // Create the socket that will listen for connections
  int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (listenSocket < 0) error("ERROR opening socket");
  
  // Set up the address struct use localhost
  setupAddressStruct(&serverAddress, atoi(argv[1]));

  // bind socket to port
  if (bind(listenSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0){
    error("ERROR on binding");
  }

  // allow 5 connections to queue
  listen(listenSocket, 5); 
  
  while(1){
    char plainTextMsg[MAX_BUFFER];
    char key[MAX_BUFFER];

    // accept connections
    connectionSocket = accept(listenSocket, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); 
    if (connectionSocket < 0) error("ERROR on accept");

    pid = fork();          // branch connection into child process
		if (pid < 0) {
			perror("fork()");
			exit(1);
		}

    else if (pid == 0) {       // child process
      // initial handshake dec_server may only connect with dec_client
      memset(buffer, '\0', MAX_BUFFER);  
      charsRead = recv(connectionSocket, buffer, MAX_BUFFER, 0); 
			if (charsRead < 0) fprintf(stderr, "socket error");
      if (strcmp(buffer, "dec_client") != 0) {  // make sure dec_client only connects to dec_server
        close(connectionSocket); 
        continue;
        }

      sleep(1);          // give client time to send new message
      long receivedSize;
      long keySize;

      // receive size of file one
      memset(buffer, '\0', MAX_BUFFER);   // clear buffer
			charsRead = recv(connectionSocket, buffer, MAX_BUFFER, 0); 
			if (charsRead < 0) fprintf(stderr, "socket error");
      //send confirmation
      charsSent = send(connectionSocket, "received", 8, 0); 
			if (charsSent < 0) fprintf(stderr, "socket error");
			receivedSize = atoi(buffer);     // message containing size of file one

      // receive body of file one
      memset(buffer, '\0', MAX_BUFFER);
      charsRead = recv(connectionSocket, buffer, MAX_BUFFER, 0); 
      if (charsRead < 0) perror("error");
      receivedSize -= strlen(buffer);     // decriment total file size by bytes received
      memset(plainTextMsg, '\0', MAX_BUFFER);     // clear plainTextMsg
      strcat(plainTextMsg, buffer);

      // ensure full message recieved
      while (receivedSize > 0) {
        memset(buffer, '\0', MAX_BUFFER);
        charsRead = recv(connectionSocket, buffer, MAX_BUFFER, 0);
        if (charsRead < 0) perror("error");
        receivedSize -= strlen(buffer);
        strcat(plainTextMsg, buffer);
        if (strlen(buffer) == 0) break;   // no more data to be read from socket
      }

    // Send confirmation to client
	  charsSent = send(connectionSocket, "Received", 8, 0); 
	  if (charsSent < 0) fprintf(stderr, "ERROR writing to socket");

    // receive size of file two, key
    memset(buffer, '\0', MAX_BUFFER);
	  charsRead = recv(connectionSocket, buffer, MAX_BUFFER, 0); 
	  if (charsRead < 0) fprintf(stderr, "socket error");

    //send confirmation
    charsSent = send(connectionSocket, "received", 8, 0); 
	  if (charsSent < 0) fprintf(stderr, "socket error");
	  keySize = atoi(buffer);     // message containing size of key

    // receive file two content
    memset(buffer, '\0', MAX_BUFFER);
    charsRead = recv(connectionSocket, buffer, MAX_BUFFER, 0); 
    if (charsRead < 0) perror("error");
    keySize -= strlen(buffer);     // when keySize == 0 full message received
    memset(key, '\0', MAX_BUFFER);     // clear key
    strcat(key, buffer);

    // ensure full file recieved
    while (keySize > 0) {
      memset(buffer, '\0', MAX_BUFFER);
      charsRead = recv(connectionSocket, buffer, MAX_BUFFER, 0); 
      if (charsRead < 0) perror("error");
      keySize -= strlen(buffer);
      strcat(key, buffer);
      if (strlen(buffer) == 0) break;    // no more data to receive 
      }

    // send confirmation
	  charsSent = send(connectionSocket, "received", 8, 0); // confirmation
	  if (charsSent < 0) fprintf(stderr, "ERROR writing to socket");
    sleep(2);   // give client time to recieve confrimation before decrypted message

    // send un-encoded message
	  charsSent = send(connectionSocket, decryption(plainTextMsg, key), strlen(plainTextMsg), 0); // Send success back
	  if (charsSent < 0) fprintf(stderr, "ERROR writing to socket");

    free(decryptMessage);      // assigned in decryption()
    }

    // Close socket to client
    close(connectionSocket); 
  }
  // Close listening socket
  close(listenSocket); 
  return EXIT_SUCCESS;
}

void error(const char *msg) {       // report errors
  perror(msg);
  exit(1);
} 

void setupAddressStruct(struct sockaddr_in* address, int portNumber){
  // fill address struct using localhost
  memset((char*) address, '\0', sizeof(*address)); 
  address->sin_family = AF_INET;
  address->sin_port = htons(portNumber);  // assign port number
  address->sin_addr.s_addr = INADDR_ANY;
}

char* decryption(char* encMsg, char *key) {     // decrypt message
  // key must be at least as long as encrypted message
	if (strlen(key) < strlen(encMsg)) {
		fprintf(stderr, "Error: key is too short!\n");
		exit(1);
	}

	char *decryptMessage = malloc (sizeof (char) * strlen(encMsg));
  char decMsg[strlen(encMsg)-1];
	const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";   // valid characters

	for (int i = 0; i < strlen(encMsg)-1; ++i) {
    // 65 == A 90 == Z
		if ((encMsg[i] < 65 || encMsg[i] > 90) && encMsg[i] != ' '){
			fprintf(stderr, "error: input contains bad characters");
			exit(1);
		}
    // -6 if space present -65 if no space present to avoid colisions, +27 to ensure value is posotive
		else if (encMsg[i] == ' ' && (key[i] == ' '))
		  decMsg[i] = (chars[((((encMsg[i] - 6) - (key[i] - 6)) + 27) % 27)]);
		else if (encMsg[i] == ' ' && key[i] != ' ') 
		  decMsg[i] = (chars[((((encMsg[i] - 6) - (key[i] - 65)) + 27) % 27)]);
		else if (key[i] == ' ' && encMsg[i] != ' ') 
		  decMsg[i] = (chars[((((encMsg[i] - 65) - (key[i] - 6) + 27)) % 27)]);

    // treat key or text with spaces differently to avoid mapping issues
		else decMsg[i] = (chars[((((encMsg[i] - 65) - (key[i] - 65) + 27)) % 27)]);
	}
	decMsg[strlen(encMsg)-1] = '\n';
	decMsg[strlen(encMsg)] = '\0';
	strcpy(decryptMessage, decMsg);  // return pointer

	return decryptMessage;
}