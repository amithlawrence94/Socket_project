#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket() and bind() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <iostream>
#include <vector>
#include <time.h>
#include "record.cpp"

#define ECHOMAX 500     /* Longest string to echo */
#define CHARLENGTH 16

void DieWithError(const char* errorMessage) {
	perror(errorMessage);
	exit(1);
}


struct test {
	int first;
	char msg[CHARLENGTH];
};
//int indexOfUser(vector<user>& vec, char* userName);
//void chooseNRandom(vector<user>& vec, int num, message& msg, char* leaderName);
//const char* findLeader(const vector<user>& vec);
//bool isFree(vector<user> vec, char* name);
using namespace std;

int main(int argc, char* argv[]) {
	int sock;
	struct sockaddr_in echoServAddr;
	struct sockaddr_in echoClntAddr;
	unsigned int cliAddrLen;
	char echoBuffer[ECHOMAX];
	unsigned short echoServPort;
	int recvMsgSize;
	struct message receivedMessage, replyMessage;
	vector<struct user> stateTable;
	int valueChecker;	
	int ringSize = 0;
	bool DHT_Exists = false;
	bool DHT_Busy = false;
	char departingUser[CHARLENGTH];
	//bool leaveRequested = false;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <PORT NUMBER>\n", argv[0]);
		exit(1);
	}

	echoServPort = atoi(argv[1]); //Set first arg as local port

	//Create socket 
	if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
		DieWithError("socket() failed");

	//Construct local address structure
	memset(&echoServAddr, 0, sizeof(echoServAddr));
	echoServAddr.sin_family = AF_INET; 
	echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	echoServAddr.sin_port = htons(echoServPort); 

	//Bind to the local address
	if (bind(sock, (struct sockaddr*) & echoServAddr, sizeof(echoServAddr)) < 0)
		DieWithError("bind() failed\n");
	for (;;) {
		//Set size of in-out parameter
		cliAddrLen = sizeof(echoClntAddr);

		//Block until receive message from a client
		if ((recvMsgSize = recvfrom(sock, &receivedMessage, sizeof(receivedMessage), 0, (struct sockaddr*) &echoClntAddr, &cliAddrLen)) < 0)
			DieWithError("recvfrom() failed\n");
		cout << "Received message\n--------------\n";
		receivedMessage.printMessage();

		if (receivedMessage.code == 1) {	
			struct user newUser(receivedMessage.senderName, receivedMessage.inAddress, receivedMessage.port);
			if (userExists(stateTable, newUser, echoServPort) || DHT_Busy) {
				cout << "FAILURE\n";
				replyMessage.code = 1;
				replyMessage.header = -1;
				if (valueChecker = sendto(sock, &replyMessage, sizeof(replyMessage), 0, (struct sockaddr*) & echoClntAddr, cliAddrLen) == -1)
					DieWithError("Error sending message\n");


			}
			else {
				stateTable.push_back(newUser);
				cout << "SUCCESS\n";
				replyMessage.code = 1;
				replyMessage.header = 1;
				if(valueChecker = sendto(sock, &replyMessage, sizeof(replyMessage), 0, (struct sockaddr*) & echoClntAddr, cliAddrLen) == -1)
					DieWithError("Error sending message\n");
			}
		}
		else if (receivedMessage.code == 2) {
			if (receivedMessage.num < 2 || !userExists(stateTable, receivedMessage.senderName) || receivedMessage.num > stateTable.size() || DHT_Exists || DHT_Busy) {
				cout << "FAILURE\n";
				replyMessage.code = 2;
				replyMessage.header = -1;
				sendto(sock, &replyMessage, sizeof(replyMessage), 0, (struct sockaddr*) & echoClntAddr, cliAddrLen);

			}
			else {
				//Set state of sender to Leader
				DHT_Busy = true;
				ringSize = receivedMessage.num;
				int indexOfLeader;
				userInfo* ringNodes = new userInfo[ringSize];				

				for (unsigned int i = 0; i < stateTable.size(); i++) {					
					if (strcmp(stateTable[i].userName, receivedMessage.senderName) == 0) {
						stateTable[i].state = Leader;
						indexOfLeader = i;
						break;
					}
				}
				int tempIndex = indexOfLeader;
				for (int i = 0; i < ringSize; i++) {					
					
					if (tempIndex != indexOfLeader)
						stateTable[tempIndex].state = InDHT;

					strcpy(ringNodes[i].userName, stateTable[tempIndex].userName);
					strcpy(ringNodes[i].inAddress, stateTable[tempIndex].inAddress);
					for (int j = 0; j < 3; j++) {
						ringNodes[i].ports[j] = stateTable[tempIndex].port[j];
					}
					tempIndex = (tempIndex + 1) % stateTable.size();
				}

				replyMessage.code = 2;
				replyMessage.header = 1;		
				replyMessage.num = receivedMessage.num;
				//Send success message to Leader
				sendto(sock, &replyMessage, sizeof(replyMessage), 0, (struct sockaddr*) &echoClntAddr, cliAddrLen);

				for (int i = 0; i < ringSize; i++) {
					sendto(sock, &ringNodes[i], sizeof(ringNodes[i]), 0, (struct sockaddr*) &echoClntAddr, cliAddrLen);
				}
				//delete[] ringNodes;

			}
		}
		else if (receivedMessage.code == 3) {
			unsigned int index = 0;
			for (; index < stateTable.size(); index++) {
				if (strcmp(stateTable[index].userName, receivedMessage.senderName) == 0)
					break;
			}
			
			//cout << "dht-complete " << receivedMessage.senderName << endl;

			if (stateTable[index].state == Leader) {
				replyMessage.code = 3;
				replyMessage.header = 1;
				DHT_Busy = false;
				DHT_Exists = true;
				
				
				sendto(sock, &replyMessage, sizeof(replyMessage), 0, (struct sockaddr*)& echoClntAddr, cliAddrLen);

			}
			else{
				replyMessage.code = 3;
				replyMessage.header = -1;								

				sendto(sock, &replyMessage, sizeof(replyMessage), 0, (struct sockaddr*)& echoClntAddr, cliAddrLen);
			}
		}
		else if (receivedMessage.code == 4) {
			//Return FAILURE if DHT is not complete, user is not registered, or state of user is not Free
			if (!DHT_Exists || !userExists(stateTable, receivedMessage.senderName) || (stateOfUser(stateTable, receivedMessage.senderName) != Free)) {
				replyMessage.code = 4;
				replyMessage.header = -1;

				sendto(sock, &replyMessage, sizeof(replyMessage), 0, (struct sockaddr*)& echoClntAddr, cliAddrLen);
			}
			else {
				//Initialize random seed
				srand(time(NULL));
				
				int randomIndex;
				randomIndex = rand() % stateTable.size();
				while (stateTable[randomIndex].state == Free) {
					randomIndex = (randomIndex + 1) % stateTable.size();
				}

				replyMessage.code = 4;
				replyMessage.header = 1;
				strcpy(replyMessage.random.userName, stateTable[randomIndex].userName);
				strcpy(replyMessage.random.inAddress, stateTable[randomIndex].inAddress);
				for (int i = 0; i < 3; i++) {
					replyMessage.random.ports[i] = stateTable[randomIndex].port[i];
				}
				
				sendto(sock, &replyMessage, sizeof(replyMessage), 0, (struct sockaddr*)& echoClntAddr, cliAddrLen);
			}

		}
		/*
		//leave-dht <user-name>
		*/
		else if (receivedMessage.code == 5) {
		if (!DHT_Exists || !userExists(stateTable, receivedMessage.senderName) || stateOfUser(stateTable, receivedMessage.senderName) == Free || DHT_Busy || ringSize <= 2) {
			replyMessage.code = 5;
			replyMessage.header = -1;

			sendto(sock, &replyMessage, sizeof(replyMessage), 0, (struct sockaddr*)& echoClntAddr, cliAddrLen);

			}
			//Store user-name and wait for dht-rebuilt
			else {
				DHT_Busy = true;
				strcpy(departingUser, receivedMessage.senderName);
				replyMessage.code = 5;
				replyMessage.header = 1;
	
				sendto(sock, &replyMessage, sizeof(replyMessage), 0, (struct sockaddr*)& echoClntAddr, cliAddrLen);
			}

		}

		else if (receivedMessage.code == 6) {
			if (strcmp(receivedMessage.senderName, departingUser) != 0) {
				
				replyMessage.code = 6;
				replyMessage.header = -1;
				sendto(sock, &replyMessage, sizeof(replyMessage), 0, (struct sockaddr*)& echoClntAddr, cliAddrLen);
			}
			else{
				replyMessage.code = 6;
				replyMessage.header = 1;
				sendto(sock, &replyMessage, sizeof(replyMessage), 0, (struct sockaddr*)& echoClntAddr, cliAddrLen);
				
				for (unsigned int i = 0; i < stateTable.size(); i++) {
					if (strcmp(stateTable[i].userName, receivedMessage.senderName) == 0)
						stateTable[i].state = Free;
				}
				for (unsigned int i = 0; i < stateTable.size(); i++) {
					if (stateTable[i].state == Leader && strcmp(stateTable[i].userName, receivedMessage.inAddress) != 0) {
						stateTable[i].state = InDHT;
						cout << "Changed " << stateTable[i].userName << " from Leader to InDHT\n";
					}
					if (strcmp(stateTable[i].userName, receivedMessage.inAddress) == 0) {
						if (stateTable[i].state == Leader)
							cout << stateTable[i].userName << " was already the previous Leader. No need to set new Leader\n";
						else {
							stateTable[i].state = Leader;
							cout << "Changed " << stateTable[i].userName << " to Leader\n";
						}
					}
				}
				ringSize--;
				cout << "Size of ring is now " << ringSize << endl;
			}
		}
	}//End of infinite for loop
}
