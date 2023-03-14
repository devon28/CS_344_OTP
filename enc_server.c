//#########################################################
// Author: Devon Miller
// Date: 3/6/23
// Course: CS_344
// Assignment: OTP
// Program: enc_server
//#########################################################

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>      
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_BUFFER 1000000       // max sending size

char *encryptMessage;          // global variable

void error(const char *msg);

void setupAddressStruct(struct sockaddr_in* address, int portNumber);

char* encryption(char* msg, char *key); 

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
      // initial handshake verify client is enc_client
      memset(buffer, '\0', MAX_BUFFER);   // clear buffer
      charsRead = recv(connectionSocket, buffer, MAX_BUFFER, 0); 
			if (charsRead < 0) fprintf(stderr, "socket error");
      if (strcmp(buffer, "enc_client") != 0) {
        fprintf(stderr,"ERROR: enc_server must only use enc_client\n");
        close(connectionSocket); 
        continue;
      }
      sleep(1);  // give client time to send next message

      long receivedSize;
      long keySize;

      // receive size of file one
      memset(buffer, '\0', MAX_BUFFER);   // clear buffer
			charsRead = recv(connectionSocket, buffer, MAX_BUFFER, 0); 
			if (charsRead < 0) fprintf(stderr, "socket error");
      // Send confirmation
      charsSent = send(connectionSocket, "received", 8, 0); 
			if (charsSent < 0) fprintf(stderr, "socket error");
			receivedSize = atoi(buffer);     // message containing size of file one

      // receive body of file one
      memset(buffer, '\0', MAX_BUFFER);
      charsRead = recv(connectionSocket, buffer, MAX_BUFFER, 0); 
      if (charsRead < 0) perror("error");
      receivedSize -= strlen(buffer);     // decriment total file size by bytes received
      receivedSize --;
      memset(plainTextMsg, '\0', MAX_BUFFER);
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
	  charsSent = send(connectionSocket, "Received", 8, 0); // Send confirmation
	  if (charsSent < 0) fprintf(stderr, "ERROR writing to socket");

    // receive size of file two, key
    memset(buffer, '\0', MAX_BUFFER);
	  charsRead = recv(connectionSocket, buffer, MAX_BUFFER, 0); 
	  if (charsRead < 0) fprintf(stderr, "socket error");

    //send confirmation
    charsSent = send(connectionSocket, "received", 8, 0); // Send success back
	  if (charsSent < 0) fprintf(stderr, "socket error");
	  keySize = atoi(buffer);     // message containing size of key

    // receive file two content
    memset(buffer, '\0', MAX_BUFFER);
    charsRead = recv(connectionSocket, buffer, MAX_BUFFER, 0); 
    if (charsRead < 0) perror("error");
    keySize -= strlen(buffer);     // when keySize == 0 full message received
     memset(key, '\0', MAX_BUFFER);
    strcat(key, buffer);

    // ensure full file recieved
    while (keySize > 0) {
      memset(buffer, '\0', MAX_BUFFER);
      charsRead = recv(connectionSocket, buffer, MAX_BUFFER, 0); 
      if (charsRead < 0) perror("error");
      keySize -= strlen(buffer);
      strcat(key, buffer);
      if (strlen(buffer) == 0) break;
      }

    // send confirmation
	  charsSent = send(connectionSocket, "received", 8, 0); // confirmation
	  if (charsSent < 0) fprintf(stderr, "ERROR writing to socket");
    sleep(2);  // give client time to read confirmation

    // send encoded message
	  charsSent = send(connectionSocket, encryption(plainTextMsg, key), strlen(plainTextMsg), 0); // Send success back
	  if (charsSent < 0) fprintf(stderr, "ERROR writing to socket");
    free(encryptMessage);     // assigned in encryption()
    }

    // Close socket to client
    close(connectionSocket); 
  }
  // Close listening socket
  close(listenSocket); 
  return EXIT_SUCCESS;
}

void error(const char *msg) {  // print errors
  perror(msg);
  exit(1);
} 

void setupAddressStruct(struct sockaddr_in* address, int portNumber){
  // full address struct 
  memset((char*) address, '\0', sizeof(*address)); 
  address->sin_family = AF_INET;
  address->sin_port = htons(portNumber);
  address->sin_addr.s_addr = INADDR_ANY;
}

char* encryption(char* plainText, char *key) {    // encrypt data
  // key must be longe than plain text
	if (strlen(key) < strlen(plainText)) {
		fprintf(stderr, "Error: key is too short!\n");
		exit(1);
	}
	encryptMessage = malloc(strlen(plainText) * sizeof(char));
  char encrypted[strlen(plainText)-1];
	const char valid_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

	for (int i = 0; i < strlen(plainText)-1; ++i) {
    // 65 == A 90 == Z
		if ((plainText[i] < 65 || plainText[i] > 90) && plainText[i] != ' '){  // uppercase alphabet
			fprintf(stderr, "error: input must only contain capital A-Z and space");
			exit(1);
		}
    // -6 for space -65 otherwise prevents overlap
		else if (plainText[i] == ' ' && (key[i] == ' '))
			encrypted[i] = (valid_chars[((plainText[i] - 6) + (key[i] - 6)) % 27]);
		else if (plainText[i] == ' ')
			encrypted[i] = (valid_chars[((plainText[i] - 6) + (key[i] - 65)) % 27]);
		else if (key[i] == ' ')
			encrypted[i] = (valid_chars[((plainText[i] - 65) + (key[i] - 6)) % 27]);
      
    // must differentiate if key or message has a space to prevent mapping issues
		else encrypted[i] = (valid_chars[((plainText[i] - 65) + (key[i] - 65)) % 27]);  // np spaces
	}
	encrypted[strlen(plainText)-1] = '\n';
	encrypted[strlen(plainText)] = '\0';
	strcpy(encryptMessage, encrypted);  // return pointer
	return encryptMessage;
}
