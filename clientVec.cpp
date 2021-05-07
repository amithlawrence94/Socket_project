#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket() and bind() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <iostream>
#include <vector>
#include "record.cpp"

#define ECHOMAX 500     /* Longest string to echo */
#define CHARLENGTH 21

void DieWithError(const char* errorMessage) {
	perror(errorMessage);
	exit(1);
}

using namespace std;

struct test {
	int first;
	char msg[CHARLENGTH];
};

int main(int argc, char* argv[]) {
	int servSock, querySock, leftSock, rightSock;
	struct sockaddr_in echoServAddr;
	struct sockaddr_in fromAddr;
	unsigned short echoServPort;
	unsigned int fromSize;
	char* servIP;
	char* echoString;
	char echoBuffer[ECHOMAX + 1];
	int echoStringLen;
	int respStringLen;
	string command;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s <Server IP> [<Echo Port>]\n", argv[0]);
		exit(1);
	}

	servIP = argv[1];
	echoServPort = atoi(argv[2]);

	fromSize = sizeof(fromAddr);
	if ((servSock = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
		DieWithError("socket() failed\n");

	//Construct server address structure
	memset(&echoServAddr, 0, sizeof(echoServAddr));
	echoServAddr.sin_family = AF_INET;
	inet_pton(AF_INET, servIP, &(echoServAddr.sin_addr));
	echoServAddr.sin_port = htons(echoServPort);
	
	struct message myMessage, receivedMessage;
	for (;;) {
	  //getline(cin, command);
		int* size = new int;
		//	myMessage = createMessage(command);
		//myMessage.printMessage();
		
		vector<int> vec;
		vec.push_back(1);
		vec.push_back(2);
		vec.push_back(3);
		vec.push_back(4);
		
		/*for(unsigned int i = 0; i < vec.size(); i++){
		  cout << vec[i] << endl;
		  }*/
		int* arr;
		arr = new int[5];
		arr[0] = 1;
		arr[1] = 2;
		arr[2] = 3;
		arr[3] = 4;
		arr[4] = 5;
		int sendSize = 5;
		if (sendto(servSock, &sendSize, sizeof(int), 0, (struct sockaddr*) & echoServAddr, sizeof(echoServAddr)) != sizeof(int))
			DieWithError("sendto() senta different number of bytes than expected");

		for(int i =0; i < sendSize; i++){

		  sendto(servSock, &arr[i], sizeof(arr[i]), 0, (struct sockaddr*) &echoServAddr, sizeof(echoServAddr));
		}
		exit(0);
		//recvfrom(servSock, &receivedMessage, sizeof(receivedMessage), 0, (struct sockaddr*) & fromAddr, &fromSize);
		//receivedMessage.printMessage();
	}

	
	
	close(servSock);
}
