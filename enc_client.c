//#########################################################
// Author: Devon Miller
// Date: 3/6/23
// Course: CS_344
// Assignment: OTP
//#########################################################

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netdb.h>      

#define MAX_BUFFER 1000000    // max sending size

void error(const char *msg);

void setupAddressStruct(struct sockaddr_in* address, int portNumber, char* hostname);

int main(int argc, char *argv[]) {
  int socketFD, portNumber, charsWritten, charsRead;
  struct sockaddr_in serverAddress;
  char buffer[MAX_BUFFER];
  if (argc != 4) {         // Check usage & args
    fprintf(stderr,"USAGE: %s hostname, text, key, port\n", argv[0]); 
    exit(0); 
  } 

  // Create a socket
  socketFD = socket(AF_INET, SOCK_STREAM, 0); 
  if (socketFD < 0) error("CLIENT: ERROR opening socket");
  
  setupAddressStruct(&serverAddress, atoi(argv[3]), "localhost");      // set up socket on localhost
  if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to server
	error("CLIENT: ERROR connecting");

  // FILE ONE plain text file
  FILE *fp;
  char *line = NULL;
  size_t n = 0;

  // open file for reading
  fp = fopen(argv[1], "r");
  fseek(fp, 0L, SEEK_END);       // file size
  long int fileSize = ftell(fp);
  fclose(fp);

  // send file one size as first message
  memset(buffer, '\0', sizeof(buffer));
  sprintf(buffer, "%ld", fileSize);
  charsWritten = send(socketFD, buffer, strlen(buffer), 0); 
  if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
  if (charsWritten < strlen(buffer)) printf("CLIENT: WARNING: Not all data written to socket!\n");
  
  // receive confirmation
  memset(buffer, '\0', sizeof(buffer)); 
  charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); 
  if (charsRead < 0) error("ERROR recv()");
 // fprintf(stderr, "\nchars 1: %d", charsRead);

  // send first file content
  fp = fopen(argv[1], "r");
  if (fp == NULL) perror("open()");
  ssize_t line_length = getline(&line, &n, fp);
  memset(buffer, '\0', sizeof(buffer));
  strcpy(buffer, line);
  fclose(fp);

  charsWritten = send(socketFD, buffer, strlen(buffer), 0);    // send file one
  if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
  if (charsWritten < strlen(buffer)) printf("CLIENT: WARNING: Not all data written to socket!\n");
  memset(buffer, '\0', sizeof(buffer));

  // receive confirmation
  charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); 
  if (charsRead < 0) error("ERROR            ");
  //  fprintf(stderr, "\nchars 2: %d", charsRead);

  // file two key
  FILE *fp2;
  char *line2 = NULL;
  size_t n2 = 0;

  // open file for reading
  fp2 = fopen(argv[2], "r");
  if (fp2 == NULL) perror("fopen()");
  fseek(fp2, 0L, SEEK_END);      // get size of file
  long fileSize2 = ftell(fp2);
  fclose(fp2);

  // send size of file two
  memset(buffer, '\0', sizeof(buffer));
  sprintf(buffer, "%ld", fileSize2);
  charsWritten = send(socketFD, buffer, strlen(buffer), 0); 
  if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
  if (charsWritten < strlen(buffer)) printf("CLIENT: WARNING: Not all data written to socket!\n");

  // receive confirmation
  memset(buffer, '\0', sizeof(buffer)); 
  charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); 
  if (charsRead < 0) error("ERROR            ");
  //  fprintf(stderr, "\nchars 3: %d", charsRead);

  // send second file contents
  fp2 = fopen(argv[2], "r");
  if (fp2 == NULL) perror("open()");
  line_length = getline(&line2, &n2, fp2);    // get file content
  memset(buffer, '\0', sizeof(buffer));
  strcpy(buffer, line2);
  fclose(fp2);

  charsWritten = send(socketFD, buffer, strlen(buffer), 0); 
  if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
  if (charsWritten < strlen(buffer)) printf("CLIENT: WARNING: Not all data written to socket!\n");

  // receive confirmation
  memset(buffer, '\0', sizeof(buffer));
  charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); 
  if (charsRead < 0) error("ERROR ");
  //  fprintf(stderr, "\nchars 4: %d\nbuffer: %s\n", charsRead, buffer);
sleep(1);
  // receive encoded message
  memset(buffer, '\0', sizeof(buffer));
  charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); 
  if (charsRead < 0) error("ERROR ");
  fileSize -= strlen(buffer);
  printf("%s", buffer);     // print encoded message

  // if full file not received read from socket untill full file received
  while (fileSize > 0) {
	memset(buffer, '\0', sizeof(buffer)); 
	charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); // Read data from the socket, leaving \0 at end
	if (charsRead < 0) error("CLIENT: ERROR reading from socket\n");
	fileSize -= strlen(buffer);
  printf("%s", buffer);
  if (strlen(buffer) == 0) break;     // no more file left to receive
	}
  
  // Close the socket
  close(socketFD); 
  return EXIT_SUCCESS;
}

void setupAddressStruct(struct sockaddr_in* address, int portNumber, char* hostname){
  // fill address struct with information
  memset((char*) address, '\0', sizeof(*address)); 
  address->sin_family = AF_INET;
  address->sin_port = htons(portNumber);
  struct hostent* hostInfo = gethostbyname("localhost");     // socket on localhost
  if (hostInfo == NULL) fprintf(stderr, "CLIENT: ERROR, no such host\n"); 
  memcpy((char*) &address->sin_addr.s_addr, hostInfo->h_addr_list[0], hostInfo->h_length);
}

void error(const char *msg) { 
  perror(msg); 
  exit(0); 
} 
