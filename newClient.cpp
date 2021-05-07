#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket() and bind() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <iostream>
#include <vector>
#include <thread>
#include <fstream>
#include "record.cpp"

#define ECHOMAX 500     /* Longest string to echo */
#define CHARLENGTH 16
#define FILE_NAME "StatsCountry.csv"
#define LINES_OF_DATA 241
#define HASH_TABLE_SIZE 353

void DieWithError(const char* errorMessage) {
	perror(errorMessage);
	exit(1);
}

using namespace std;
using namespace std::literals::chrono_literals;

struct test {
	int first;
	char msg[CHARLENGTH];
};

int servSock;
int querySock, leftSock, rightSock;
struct sockaddr_in echoServAddr;
struct sockaddr_in fromAddr;
struct message myMessage, receivedMessage;
unsigned int fromSize;
int hostID, ringSize;
struct sockaddr_in queryAddr;
struct sockaddr_in leftAddr;
struct sockaddr_in rightAddr;
struct sockaddr_in outAddr;
struct sockaddr_in leftNeighbor;
struct sockaddr_in rightNeighbor;
unsigned short echoServPort;
char* servIP;
char* echoString;
char echoBuffer[ECHOMAX + 1];
int echoStringLen;
int respStringLen;
bool shouldJoin = false;
bool registered = false;
bool stopReregister = false;
char userName[CHARLENGTH];
char ipAddress[CHARLENGTH];
unsigned short myPorts[3];
record** hashTable = new record*[HASH_TABLE_SIZE];
bool leavingDHT = false;
char leftNeighborIPAddress[CHARLENGTH];
char rightNeighborIPAddress[CHARLENGTH];
unsigned short leftNeighborPort;
unsigned short rightNeighborPort;
char leftNeighborUserName[CHARLENGTH];
char rightNeighborUserName[CHARLENGTH];

void sendCommand() {
	while (!registered)
	{		
		
		string command;
		getline(cin, command);
		int* size = new int;
		myMessage = createMessage(command);
		userInfo* ringNodes;

		if (sendto(servSock, &myMessage, sizeof(myMessage), 0, (struct sockaddr*) & echoServAddr, sizeof(echoServAddr)) != sizeof(struct message))
			DieWithError("sendto() senta different number of bytes than expected");
		
		recvfrom(servSock, &receivedMessage, sizeof(receivedMessage), 0, (struct sockaddr*) & fromAddr, &fromSize);

		if (receivedMessage.code == 1 && receivedMessage.header == 1) {
			if (stopReregister)
				cout << "Cannot change registration on host. Please terminate program if you wish to change your registration.\n";
			else {
				registered = true;

				strcpy(userName, myMessage.senderName);
				//cout << "setting userName to " << userName << endl;
				strcpy(ipAddress, myMessage.inAddress);;
				myPorts[0] = myMessage.port[0];
				myPorts[1] = myMessage.port[1];
				myPorts[2] = myMessage.port[2];

				memset(&queryAddr, 0, sizeof(queryAddr));
				queryAddr.sin_family = AF_INET;
				inet_pton(AF_INET, myMessage.inAddress, &(queryAddr.sin_addr));
				queryAddr.sin_port = htons(myMessage.port[0]);

				if (bind(querySock, (struct sockaddr*) & queryAddr, sizeof(queryAddr)) < 0)
					DieWithError("bind() for querySock failed\n");

				cout << "Setting up address of " << myMessage.inAddress << " to listen at port " << myMessage.port[0] << endl;
				memset(&leftAddr, 0, sizeof(leftAddr));
				leftAddr.sin_family = AF_INET;
				inet_pton(AF_INET, myMessage.inAddress, &(leftAddr.sin_addr));
				leftAddr.sin_port = htons(myMessage.port[1]);

				if (bind(leftSock, (struct sockaddr*) & leftAddr, sizeof(leftAddr)) < 0)
					DieWithError("bind() for leftSock failed\n");

				memset(&rightAddr, 0, sizeof(rightAddr));
				rightAddr.sin_family = AF_INET;
				inet_pton(AF_INET, myMessage.inAddress, &(rightAddr.sin_addr));
				rightAddr.sin_port = htons(myMessage.port[2]);

				if (bind(rightSock, (struct sockaddr*) & rightAddr, sizeof(rightAddr)) < 0)
					DieWithError("bind() for rightSock failed\n");
			}
		}
		else if (receivedMessage.code == 2 && receivedMessage.header == 1) {
			ringSize = receivedMessage.num;
			hostID = 0;//added
			leavingDHT = false;//added
			ringNodes = new userInfo[ringSize];
			for (int i = 0; i < ringSize; i++) {
				recvfrom(servSock, &ringNodes[i], sizeof(ringNodes[i]), 0, (struct sockaddr*) & fromAddr, &fromSize);
			}

			for (int i = 0; i < ringSize; i++) {
				cout << "user[" << i << "]:   " << ringNodes[i].userName << "   " << ringNodes[i].inAddress;
				for (unsigned short x : ringNodes[i].ports) {
					cout << "   " << x;
				}
				cout << endl;
			}

			//Set Leader's own neighbors
			memset(&leftNeighbor, 0, sizeof(leftNeighbor));
			leftNeighbor.sin_family = AF_INET;
			inet_pton(AF_INET, ringNodes[ringSize - 1].inAddress, &(leftNeighbor.sin_addr));
			leftNeighbor.sin_port = htons(ringNodes[ringSize - 1].ports[2]);

			strcpy(leftNeighborIPAddress, ringNodes[ringSize - 1].inAddress);
			leftNeighborPort = ringNodes[ringSize - 1].ports[2];
			strcpy(leftNeighborUserName, ringNodes[ringSize - 1].userName);

			memset(&rightNeighbor, 0, sizeof(rightNeighbor));
			rightNeighbor.sin_family = AF_INET;
			inet_pton(AF_INET, ringNodes[1].inAddress, &(rightNeighbor.sin_addr));
			rightNeighbor.sin_port = htons(ringNodes[1].ports[1]);

			strcpy(rightNeighborIPAddress, ringNodes[ringSize + 1].inAddress);
			rightNeighborPort = ringNodes[ringSize + 1].ports[1];
			strcpy(rightNeighborUserName, ringNodes[ringSize + 1].userName);

			//Send set-id to members of the DHT
			for (int i = 1; i < ringSize; i++) {
				memset(&outAddr, 0, sizeof(outAddr));
				outAddr.sin_family = AF_INET;
				inet_pton(AF_INET, ringNodes[i].inAddress, &(outAddr.sin_addr));
				outAddr.sin_port = htons(ringNodes[i].ports[0]);

				cout << "Sending message to " << ringNodes[i].inAddress << " at port " << ringNodes[i].ports[0] << endl;
				myMessage.code = 11;
				myMessage.header = 0;
				myMessage.num = ringSize;
				myMessage.id = i;
				int leftIndex, rightIndex;
				leftIndex = (i - 1) % ringSize;
				rightIndex = (i + 1) % ringSize;
				strcpy(myMessage.neighbors[0].inAddress, ringNodes[leftIndex].inAddress);				
				strcpy(myMessage.neighbors[0].userName, ringNodes[leftIndex].userName);

				for(int j = 0; j < 3; j++)
					myMessage.neighbors[0].ports[j] = ringNodes[leftIndex].ports[j];

				strcpy(myMessage.neighbors[1].inAddress, ringNodes[rightIndex].inAddress);
				strcpy(myMessage.neighbors[1].userName, ringNodes[rightIndex].userName);

				for (int j = 0; j < 3; j++)
					myMessage.neighbors[1].ports[j] = ringNodes[rightIndex].ports[j];
				int valChecker;
				if (valChecker = sendto(servSock, &myMessage, sizeof(myMessage), 0, (struct sockaddr*) & outAddr, sizeof(outAddr)) == -1)
					DieWithError("Error sending message\n");
			}

			//Construct the local DHT
			ifstream in;
			in.open(FILE_NAME);
			record currentRecord;
			string consume;
			int ASCII_Sum;
			getline(in, consume);
			for (int i = 0; i < LINES_OF_DATA; i++) {
				ASCII_Sum = 0;
				currentRecord = record(in);
				//currentRecord.print();
				int j = 0;
				while (currentRecord.Long_Name[j] != '\0') {
					ASCII_Sum += currentRecord.Long_Name[j];
					j++;
				}
				//cout << "ASCII Sum for record " << i << " is: " << ASCII_Sum << endl;
				int pos, id;
				pos = ASCII_Sum % HASH_TABLE_SIZE;
				id = pos % ringSize;
				
				
				//tempRecord->print();
				
				//Store in DHT of Leader
				if (hostID == id) {

					record* tempRecord = new record;
					currentRecord.copyTo(*tempRecord);

					if (hashTable[pos] == NULL) {
						hashTable[pos] = tempRecord;
					}
					else {
						tempRecord->next = hashTable[pos];
						hashTable[pos] = tempRecord;
					}
				}
				else {
					myMessage.code = 12;
					myMessage.num = pos;
					myMessage.id = id;
					myMessage.header = 0;
					currentRecord.copyTo(myMessage.currentRecord);
					
					int valChecker;
					if (valChecker = sendto(servSock, &myMessage, sizeof(myMessage), 0, (struct sockaddr*) & rightNeighbor, sizeof(rightNeighbor)) == -1)
						DieWithError("Error sending message to right neighbor\n");
				}
			}//End of for loop
			in.close();
			myMessage.code = 3;
			myMessage.header = 0;
			strcpy(myMessage.senderName, userName);
			int valChecker;
			if (valChecker = sendto(servSock, &myMessage, sizeof(myMessage), 0, (struct sockaddr*) & echoServAddr, sizeof(echoServAddr)) == -1)
				DieWithError("Error sending dht-complete message to server\n");

			recvfrom(servSock, &receivedMessage, sizeof(receivedMessage), 0, (struct sockaddr*)& fromAddr, &fromSize);
			
		}
		else if (receivedMessage.code == 4 && receivedMessage.header == -1) {
			cout << "Attempt to query-dht returned FAILURE\n";
		}
		else if (receivedMessage.code == 4 && receivedMessage.header == 1) {
			cout << "Attempt to query-dht returned SUCCESS\n";
			cout << "Enter the Long Name to search the DHT for: ";
			string searchString;
			getline(cin, searchString);
			//cin.clear();
			//cin.ignore(1000, '\n');
			//char searchArray[100];
			size_t length = searchString.copy(myMessage.searchName, 100 - 1);
			myMessage.searchName[length] = '\0';
			

			myMessage.code = 13;//query to peer
			myMessage.header = 0;
			strcpy(myMessage.random.userName, userName);
			strcpy(myMessage.random.inAddress, ipAddress);
			for (int i = 0; i < 3; i++) {
				myMessage.random.ports[i] = myPorts[i];
			}
			
			memset(&outAddr, 0, sizeof(outAddr));
			outAddr.sin_family = AF_INET;
			inet_pton(AF_INET, receivedMessage.random.inAddress, &(outAddr.sin_addr));
			outAddr.sin_port = htons(receivedMessage.random.ports[0]);

			cout << "Querying " << receivedMessage.random.inAddress << " at port " << receivedMessage.random.ports[0] << " to find " << myMessage.searchName << endl;
			sendto(servSock, &myMessage, sizeof(myMessage), 0, (struct sockaddr*)& outAddr, sizeof(outAddr));
			//cout << "Copied!\n";
			//exit(0);
			//Segfault??
		}

		else if (receivedMessage.code == 5 && receivedMessage.header == -1) {
			cout << "Server returned FAILURE to \"leave-dht command\"\n";
		}
		else if (receivedMessage.code == 5 && receivedMessage.header == 1) {
			cout << "Server returned SUCCESS to \"leave-dht command\"\n"; 
			
			myMessage.code = 99; //destroy DHT
			strcpy(myMessage.senderName, userName);

			sendto(servSock, &myMessage, sizeof(myMessage), 0, (struct sockaddr*)& rightNeighbor, sizeof(rightNeighbor));
						
		}
	}//End of while
}

int main(int argc, char* argv[]) {
	//int servSock;
	//int querySock, leftSock, rightSock;
	//struct sockaddr_in echoServAddr;
	//struct sockaddr_in fromAddr;
	/*struct sockaddr_in queryAddr;
	struct sockaddr_in leftAddr;
	struct sockaddr_in rightAddr;
	unsigned short echoServPort;
	//unsigned int fromSize;
	char* servIP;
	char* echoString;
	char echoBuffer[ECHOMAX + 1];
	int echoStringLen;
	int respStringLen;
	*/
	if (argc != 3) {
		fprintf(stderr, "Usage: %s <Server IP> [<Echo Port>]\n", argv[0]);
		exit(1);
	}

	servIP = argv[1];
	echoServPort = atoi(argv[2]);

	for (int i = 0; i < HASH_TABLE_SIZE; i++)
		hashTable[i] = NULL;

	fromSize = sizeof(fromAddr);

	if ((servSock = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
		DieWithError("servSock socket() failed\n");

	if ((querySock = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
		DieWithError("querySock socket() failed\n");
	
	if ((leftSock = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
		DieWithError("leftSock socket() failed\n");
	
	if ((rightSock = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
		DieWithError("rightSock socket() failed\n");
	
	//Construct server address structure
	memset(&echoServAddr, 0, sizeof(echoServAddr));
	echoServAddr.sin_family = AF_INET;
	inet_pton(AF_INET, servIP, &(echoServAddr.sin_addr));
	echoServAddr.sin_port = htons(echoServPort);
	
	//struct message myMessage, receivedMessage;
	
	sendCommand();
	//cout << "Finished registering\nCreating thread\n";
	registered = false;
	stopReregister = true;
	thread worker(sendCommand);
	for (;;) {
		/*getline(cin, command);
		int* size = new int;
		myMessage = createMessage(command);
		//myMessage.printMessage();

		if (sendto(servSock, &myMessage, sizeof(myMessage), 0, (struct sockaddr*) & echoServAddr, sizeof(echoServAddr)) != sizeof(struct message))
			DieWithError("sendto() senta different number of bytes than expected");

		recvfrom(servSock, &receivedMessage, sizeof(receivedMessage), 0, (struct sockaddr*) & fromAddr, &fromSize);
		//receivedMessage.printMessage();
		*/
		//Set up the sockets 

		
		int queryLen;
		queryLen = recvfrom(querySock, &receivedMessage, sizeof(receivedMessage), MSG_DONTWAIT, (struct sockaddr*) & fromAddr, &fromSize);
		if (queryLen > -1) {
			if (receivedMessage.code == 11) {
				ringSize = receivedMessage.num;
				cout << "Set ringSize to " << ringSize << endl;
				hostID = receivedMessage.id;
				cout << "Set id to " << hostID << endl;

				leavingDHT = false;
				memset(&leftNeighbor, 0, sizeof(leftNeighbor));
				leftNeighbor.sin_family = AF_INET;
				inet_pton(AF_INET, receivedMessage.neighbors[0].inAddress, &(leftNeighbor.sin_addr));
				leftNeighbor.sin_port = htons(receivedMessage.neighbors[0].ports[2]);

				strcpy(leftNeighborIPAddress, receivedMessage.neighbors[0].inAddress);
				leftNeighborPort = receivedMessage.neighbors[0].ports[2];
				strcpy(leftNeighborUserName, receivedMessage.neighbors[0].userName);

				memset(&rightNeighbor, 0, sizeof(rightNeighbor));
				rightNeighbor.sin_family = AF_INET;
				inet_pton(AF_INET, receivedMessage.neighbors[1].inAddress, &(rightNeighbor.sin_addr));
				rightNeighbor.sin_port = htons(receivedMessage.neighbors[1].ports[1]);

				strcpy(rightNeighborIPAddress, receivedMessage.neighbors[1].inAddress);
				rightNeighborPort = receivedMessage.neighbors[1].ports[1];
				strcpy(rightNeighborUserName, receivedMessage.neighbors[1].userName);
				cout << "Neighbors setup\n";
			}
			else if (receivedMessage.code == 13 && receivedMessage.header == 0) {
				cout << "Processing query for Long Name " << receivedMessage.searchName << endl;
				int ASCII_sum = 0, pos, id;
				int j = 0;
				while (receivedMessage.searchName[j] != '\0') {
					ASCII_sum += receivedMessage.searchName[j];
					j++;
				}
				cout << "ASCII value is " << ASCII_sum << endl;
				pos = ASCII_sum % HASH_TABLE_SIZE;
				id = pos % ringSize;
				if (id == hostID) {
					cout << "Same id\n";
					record* tempRecord = hashTable[pos];					
					while (tempRecord != NULL) {
						if (strcmp(tempRecord->Long_Name, receivedMessage.searchName) == 0) {
							
							//SUCCESS FINDING LONG NAME
							myMessage.code = 13;
							myMessage.header = 1;

							tempRecord->copyTo(myMessage.currentRecord);
							memset(&outAddr, 0, sizeof(outAddr));
							outAddr.sin_family = AF_INET;
							inet_pton(AF_INET, receivedMessage.random.inAddress, &(outAddr.sin_addr));
							outAddr.sin_port = htons(receivedMessage.random.ports[0]);

							sendto(servSock, &myMessage, sizeof(myMessage), 0, (struct sockaddr*)& outAddr, sizeof(outAddr));
							break;
						}
						else{
							tempRecord = tempRecord->next;
						}
					}
					if (tempRecord == NULL) {
						
						//FAILURE FINDING LONG NAME
						myMessage.code = 13;
						myMessage.header = -1;

						strcpy(myMessage.searchName, receivedMessage.searchName);

						memset(&outAddr, 0, sizeof(outAddr));
						outAddr.sin_family = AF_INET;
						inet_pton(AF_INET, receivedMessage.random.inAddress, &(outAddr.sin_addr));
						outAddr.sin_port = htons(receivedMessage.random.ports[0]);

						sendto(servSock, &myMessage, sizeof(myMessage), 0, (struct sockaddr*)& outAddr, sizeof(outAddr));
					}
					
				}
				//Not present in this node, forward to right neighbor
				else {
					//Query for finding Long Name
					myMessage.code = 13;
					myMessage.header = 0;

					strcpy(myMessage.searchName, receivedMessage.searchName);

					strcpy(myMessage.random.userName, receivedMessage.random.userName);
					strcpy(myMessage.random.inAddress, receivedMessage.random.inAddress);
					for (int i = 0; i < 3; i++) {
						myMessage.random.ports[i] = receivedMessage.random.ports[i];
					}
					sendto(servSock, &myMessage, sizeof(myMessage), 0, (struct sockaddr*)& rightNeighbor, sizeof(rightNeighbor));
				}
			}
			else if (receivedMessage.code == 13 && receivedMessage.header == 1) {
				cout << receivedMessage.currentRecord.Country_Code << ", " << receivedMessage.currentRecord.Short_Name << ", " << receivedMessage.currentRecord.Table_Name << ", " << receivedMessage.currentRecord.Long_Name << ", " << receivedMessage.currentRecord.Alpha_Code <<
					", " << receivedMessage.currentRecord.Currency << ", " << receivedMessage.currentRecord.Region << ", " << receivedMessage.currentRecord.WB_Code << ", " << receivedMessage.currentRecord.Latest_Census << endl << endl;
			}
			else if (receivedMessage.code == 13 && receivedMessage.header == -1) {
				cout << "The record associated with " << receivedMessage.searchName << " is not present in the DHT.\n\n";
			}
		}
		//Incoming message from left neighbor
		queryLen = recvfrom(leftSock, &receivedMessage, sizeof(receivedMessage), MSG_DONTWAIT, (struct sockaddr*) & fromAddr, &fromSize);
		if (queryLen > -1) {
			if (receivedMessage.code == 12) {
				int id, pos;
				id = receivedMessage.id;
				pos = receivedMessage.num;

				if (hostID == id) {
					int ASCII_Sum = 0;
					int j = 0;
					while (receivedMessage.currentRecord.Long_Name[j] != '\0') {
						ASCII_Sum += receivedMessage.currentRecord.Long_Name[j];
						j++;
					}
					//cout << "Record received. Long Name: " << receivedMessage.currentRecord.Long_Name << " with ASCII value of: " << ASCII_Sum << endl;
					record* tempRecord = new record;
					receivedMessage.currentRecord.copyTo(*tempRecord);

					if (hashTable[pos] == NULL) {
						hashTable[pos] = tempRecord;
					}
					else {
						tempRecord->next = hashTable[pos];
						hashTable[pos] = tempRecord;
					}
				}
				else {
					myMessage.code = 12;
					myMessage.num = pos;
					myMessage.id = id;
					myMessage.header = 0;
					receivedMessage.currentRecord.copyTo(myMessage.currentRecord);

					int valChecker;
					if (valChecker = sendto(servSock, &myMessage, sizeof(myMessage), 0, (struct sockaddr*) & rightNeighbor, sizeof(rightNeighbor)) == -1)
						DieWithError("Error sending message to right neighbor\n");
				}
			}
			else if (receivedMessage.code == 13 && receivedMessage.header == 0) {
				cout << "Processing query for Long Name " << receivedMessage.searchName << endl;
				int ASCII_sum = 0, pos, id;
				int j = 0;
				while (receivedMessage.searchName[j] != '\0') {
					ASCII_sum += receivedMessage.searchName[j];
					j++;
				}
				cout << "ASCII value is " << ASCII_sum << endl;
				pos = ASCII_sum % HASH_TABLE_SIZE;
				id = pos % ringSize;
				if (id == hostID) {
					cout << "Same id\n";
					record* tempRecord = hashTable[pos];
					while (tempRecord != NULL) {
						if (strcmp(tempRecord->Long_Name, receivedMessage.searchName) == 0) {

							//SUCCESS FINDING LONG NAME
							myMessage.code = 13;
							myMessage.header = 1;

							tempRecord->copyTo(myMessage.currentRecord);
							memset(&outAddr, 0, sizeof(outAddr));
							outAddr.sin_family = AF_INET;
							inet_pton(AF_INET, receivedMessage.random.inAddress, &(outAddr.sin_addr));
							outAddr.sin_port = htons(receivedMessage.random.ports[0]);

							sendto(servSock, &myMessage, sizeof(myMessage), 0, (struct sockaddr*) & outAddr, sizeof(outAddr));
							break;
						}
						else {
							tempRecord = tempRecord->next;
						}
					}
					if (tempRecord == NULL) {

						//FAILURE FINDING LONG NAME
						myMessage.code = 13;
						myMessage.header = -1;

						strcpy(myMessage.searchName, receivedMessage.searchName);

						memset(&outAddr, 0, sizeof(outAddr));
						outAddr.sin_family = AF_INET;
						inet_pton(AF_INET, receivedMessage.random.inAddress, &(outAddr.sin_addr));
						outAddr.sin_port = htons(receivedMessage.random.ports[0]);

						sendto(servSock, &myMessage, sizeof(myMessage), 0, (struct sockaddr*) & outAddr, sizeof(outAddr));
					}

				}
				//Not present in this node, forward to right neighbor
				else {
					//Query for finding Long Name
					myMessage.code = 13;
					myMessage.header = 0;

					strcpy(myMessage.searchName, receivedMessage.searchName);

					strcpy(myMessage.random.userName, receivedMessage.random.userName);
					strcpy(myMessage.random.inAddress, receivedMessage.random.inAddress);
					for (int i = 0; i < 3; i++) {
						myMessage.random.ports[i] = receivedMessage.random.ports[i];
					}
					sendto(servSock, &myMessage, sizeof(myMessage), 0, (struct sockaddr*) & rightNeighbor, sizeof(rightNeighbor));
				}
			}
			else if (receivedMessage.code == 99) {
				cout << "Received Message: teardown DHT\n";
				for (int i = 0; i < HASH_TABLE_SIZE; i++) {
					delete hashTable[i];
					hashTable[i] = NULL;
				}

				//If this is not the user which initiated the leave-dht command, propagate the teardown to the right neighbor
				if (strcmp(receivedMessage.senderName, userName) != 0) {
					myMessage.code = 99;
					strcpy(myMessage.senderName, receivedMessage.senderName);

					cout << "Sending teardownto right neighbor\n";
					sendto(servSock, &myMessage, sizeof(myMessage), 0, (struct sockaddr*) & rightNeighbor, sizeof(rightNeighbor));
				}
				//At the user which initiated the leave-dht command
				else {
					myMessage.code = 51;//reset-id
					myMessage.id = 0;
					strcpy(myMessage.senderName, userName);

					cout << "Teardown has propagated back. Sending reset-id to right neighbor.\n";
					sendto(servSock, &myMessage, sizeof(myMessage), 0, (struct sockaddr*) & rightNeighbor, sizeof(rightNeighbor));
				}
			}
			else if (receivedMessage.code == 51) {
				cout << "Received Message: reset-id\n";
				if (strcmp(receivedMessage.senderName, userName) != 0) {
					hostID = receivedMessage.id;
					myMessage.code = 51;
					myMessage.id = hostID + 1;
					ringSize--;
					strcpy(myMessage.senderName, receivedMessage.senderName);

					cout << "ID changed to " << hostID << ". Ring Size changed to " << ringSize << ".\n";

					sendto(servSock, &myMessage, sizeof(myMessage), 0, (struct sockaddr*) & rightNeighbor, sizeof(rightNeighbor));
				}
				//At the user which intiated the leave-dht command
				else {
					myMessage.code = 55;//reset-left
					strcpy(myMessage.random.inAddress, leftNeighborIPAddress);
					myMessage.port[0] = leftNeighborPort;

					cout << "Reset-id has propagated back. Sending reset-left to right neighbor.\n";
					sendto(servSock, &myMessage, sizeof(myMessage), 0, (struct sockaddr*) & rightNeighbor, sizeof(rightNeighbor));

					myMessage.code = 59;//reset-right
					strcpy(myMessage.random.inAddress, rightNeighborIPAddress);
					myMessage.port[0] = rightNeighborPort;

					cout << "Sending reset-right to left neighbor.\n";
					sendto(servSock, &myMessage, sizeof(myMessage), 0, (struct sockaddr*) & leftNeighbor, sizeof(leftNeighbor));

					myMessage.code = 60;//rebuild-dht

					sendto(servSock, &myMessage, sizeof(myMessage), 0, (struct sockaddr*) & rightNeighbor, sizeof(rightNeighbor));
					
					//Message server that DHT is rebuilt
					myMessage.code = 6;
					strcpy(myMessage.senderName, userName);
					//Use inAddress to hold value of newLeader's userName
					strcpy(myMessage.inAddress, rightNeighborUserName);

					sendto(servSock, &myMessage, sizeof(myMessage), 0, (struct sockaddr*)& echoServAddr, sizeof(echoServAddr));

				}
			}
			else if (receivedMessage.code == 55) {
				cout << "Recieved Message: reset-left\n";
				strcpy(leftNeighborIPAddress, receivedMessage.random.inAddress);
				leftNeighborPort = receivedMessage.port[0];

				memset(&leftNeighbor, 0, sizeof(leftNeighbor));
				leftNeighbor.sin_family = AF_INET;
				inet_pton(AF_INET, leftNeighborIPAddress, &(leftNeighbor.sin_addr));
				leftNeighbor.sin_port = htons(leftNeighborPort);
			}

			else if (receivedMessage.code == 60) {
				cout << "Received Message: rebuild-dht\n";

				//Construct the local DHT
				ifstream in;
				in.open(FILE_NAME);
				record currentRecord;
				string consume;
				int ASCII_Sum;
				getline(in, consume);
				for (int i = 0; i < LINES_OF_DATA; i++) {
					ASCII_Sum = 0;
					currentRecord = record(in);
					//currentRecord.print();
					int j = 0;
					while (currentRecord.Long_Name[j] != '\0') {
						ASCII_Sum += currentRecord.Long_Name[j];
						j++;
					}
					//cout << "ASCII Sum for record " << i << " is: " << ASCII_Sum << endl;
					int pos, id;
					pos = ASCII_Sum % HASH_TABLE_SIZE;
					id = pos % ringSize;


					//tempRecord->print();

					//Store in DHT of Leader
					if (hostID == id) {

						record* tempRecord = new record;
						currentRecord.copyTo(*tempRecord);

						if (hashTable[pos] == NULL) {
							hashTable[pos] = tempRecord;
						}
						else {
							tempRecord->next = hashTable[pos];
							hashTable[pos] = tempRecord;
						}
					}
					else {
						myMessage.code = 12;
						myMessage.num = pos;
						myMessage.id = id;
						myMessage.header = 0;
						currentRecord.copyTo(myMessage.currentRecord);

						int valChecker;
						if (valChecker = sendto(servSock, &myMessage, sizeof(myMessage), 0, (struct sockaddr*) & rightNeighbor, sizeof(rightNeighbor)) == -1)
							DieWithError("Error sending message to right neighbor\n");
					}
				}//End of for loop
				in.close();
				/*myMessage.code = 61;
				myMessage.header = 0;
				strcpy(myMessage.senderName, userName);
				int valChecker;
				if (valChecker = sendto(servSock, &myMessage, sizeof(myMessage), 0, (struct sockaddr*) &fromAddr, sizeof(fromAddr)) == -1)
					DieWithError("Error sending dht-complete to leaving host\n");			
					*/
			}
		}
		queryLen = recvfrom(rightSock, &receivedMessage, sizeof(receivedMessage), MSG_DONTWAIT, (struct sockaddr*) & fromAddr, &fromSize);
		if (queryLen > -1) {
			if (receivedMessage.code == 59) {
				cout << "Recieved Message: reset-right\n";	
				strcpy(rightNeighborIPAddress, receivedMessage.random.inAddress);
				rightNeighborPort = receivedMessage.port[0];

				memset(&rightNeighbor, 0, sizeof(rightNeighbor));
				rightNeighbor.sin_family = AF_INET;
				inet_pton(AF_INET, rightNeighborIPAddress, &(rightNeighbor.sin_addr));
				rightNeighbor.sin_port = htons(rightNeighborPort);
			}
		}
	}//End of for loop
	cout << "Detaching BAD\n";
	worker.detach();

	close(servSock);
}