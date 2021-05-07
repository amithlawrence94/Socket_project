#include <iostream>
#include <string.h>
#include <vector>
#include <sstream>
#include <string>

#define CHARLENGTH 16

bool consumeComma = false;
enum State {
	Free, Leader, InDHT //0: Free, 1:Leader, 2: InDHT
};

struct user {
	char userName[CHARLENGTH];
	char inAddress[CHARLENGTH];
	State state; //0: Free, 1:Leader, 2: InDHT
	unsigned short port[3];
	user() {
		memset(userName, 0, CHARLENGTH);
		memset(inAddress, 0, CHARLENGTH);
		state = Free;
		for (short i : port) {
			i = 0;
		}
	}

	user(char* name, char* address, unsigned short* sockets) {
		memcpy(userName, name, CHARLENGTH);
		memcpy(inAddress, address, CHARLENGTH);
		state = Free;
		for (int i = 0; i < 3; i++) {
			port[i] = sockets[i];
		}
	}
};

bool userExists(std::vector<struct user> vec, struct user newUser, unsigned short serverPort) {
	bool exists = false;
	for (int i = 0; i < 3; i++) {
		if (newUser.port[i] == serverPort)
			return true;
	}
	if (newUser.port[0] == newUser.port[1] || newUser.port[0] == newUser.port[2] || newUser.port[1] == newUser.port[2])
		return true;
	for (int i = 0; i < vec.size(); i++) {
		if (strcmp(newUser.userName, vec[i].userName) == 0) {
			exists = true;
			break;
		}
		for (int j = 0; j < 3; j++) {
			for (int k = 0; k < 3; k++) {
				if (newUser.port[j] == vec[i].port[k]) {
					exists = true;
					break;
				}
			}
		}
	}
	return exists;
}

bool userExists(std::vector<struct user> vec, char* userName) {
	bool exists = false;
	for (int i = 0; i < vec.size(); i++) {
		if (strcmp(vec[i].userName, userName) == 0) {
			exists = true;
			break;
		}
	}
	return exists;
}

int indexOfUser(std::vector<struct user> vec, char* userName) {
	int index = -1;
	for (unsigned int i = 0; i < vec.size(); i++) {
		if (strcmp(vec[i].userName, userName) == 0) {
			index = i;
		}
	}
	return index;
}

//Is only called if UserExists returns true, and therefore will be okay to return Free if user is not found because this scenario will not occur when this function is called
State stateOfUser(std::vector<struct user> vec, char* userName) {
	State result = Free;
	for (unsigned int i = 0; i < vec.size(); i++) {
		if (strcmp(vec[i].userName, userName) == 0)
			result = vec[i].state;
	}
	return result;
}

struct record {
	//std::string Country_Code, Short_Name, Table_Name, Long_Name, Alpha_Code, Currency, Region, WB_Code, Latest_Census;
	char Country_Code[4];
	char Short_Name[100];
	char Table_Name[100];
	char Long_Name[100];
	char Alpha_Code[3];
	char Currency[100];
	char Region[100];
	char WB_Code[3];
	char Latest_Census[100];
	record* next;

	record(std::istream& stream) {
		std::string S_Country_Code, S_Short_Name, S_Table_Name, S_Long_Name, S_Alpha_Code, S_Currency, S_Region, S_WB_Code, S_Latest_Census;
		if (stream.peek() == '\"'){
			stream.get();
			getline(stream, S_Country_Code, '\"');
			consumeComma = true;
		}
		else {
			if (consumeComma) {
				stream.get();
				consumeComma = false;
			}
			getline(stream, S_Country_Code, ',');
		}
		
		if (stream.peek() == '\"') {
			stream.get();
			getline(stream, S_Short_Name, '\"');
			consumeComma = true;
		}
		else {
			if (consumeComma) {
				stream.get();
				consumeComma = false;
			}
			getline(stream, S_Short_Name, ',');
		}
		
		if (stream.peek() == '\"') {
			stream.get();
			getline(stream, S_Table_Name, '\"');
			consumeComma = true;
		}
		else {
			if (consumeComma) {
				stream.get();
				consumeComma = false;
			}
			getline(stream, S_Table_Name, ',');
		}

		if (stream.peek() == '\"') {
			stream.get();
			getline(stream, S_Long_Name, '\"');
			consumeComma = true;
		}
		else {
			if (consumeComma) {
				stream.get();
				consumeComma = false;
			}
			getline(stream, S_Long_Name, ',');
		}

		if (stream.peek() == '\"') {
			stream.get();
			getline(stream, S_Alpha_Code, '\"');
			consumeComma = true;
		}
		else {
			if (consumeComma) {
				stream.get();
				consumeComma = false;
			}
			getline(stream, S_Alpha_Code, ',');
		}

		if (stream.peek() == '\"') {
			stream.get();
			getline(stream, S_Currency, '\"');
			consumeComma = true;
		}
		else {
			if (stream.peek() == ',')
				stream.get();
			getline(stream, S_Currency, ',');
		}

		if (stream.peek() == '\"') {
			stream.get();
			getline(stream, S_Region, '\"');
			consumeComma = true;
		}
		else {
			if (consumeComma) {
				stream.get();
				consumeComma = false;
			}
			getline(stream, S_Region, ',');
		}

		if (stream.peek() == '\"') {
			stream.get();
			getline(stream, S_WB_Code, '\"');
			consumeComma = true;
		}
		else {
			if (consumeComma) {
				stream.get();
				consumeComma = false;
			}
			getline(stream, S_WB_Code, ',');
		}

		/*if (stream.peek() == '\"') {
			stream.get();
			getline(stream, S_Latest_Census, '\"');
		}
		else {*/
			//if (stream.peek() == ',')
				//stream.get();
			getline(stream, S_Latest_Census, '\n');
		//}
		

		std::size_t length;
		length = S_Country_Code.copy(Country_Code, 4 - 1);
		Country_Code[length] = '\0';
		length = S_Short_Name.copy(Short_Name, 100 - 1);
		Short_Name[length] = '\0';
		length = S_Table_Name.copy(Table_Name, 100 - 1);
		Table_Name[length] = '\0';
		length = S_Long_Name.copy(Long_Name, 100 - 1);
		Long_Name[length] = '\0';
		length = S_Alpha_Code.copy(Alpha_Code, 3 - 1);
		Alpha_Code[length] = '\0';
		length = S_Currency.copy(Currency, 100 - 1);
		Currency[length] = '\0';
		length = S_Region.copy(Region, 100 - 1);
		Region[length] = '\0';
		length = S_WB_Code.copy(WB_Code, 3 - 1);
		WB_Code[length] = '\0';
		length = S_Latest_Census.copy(Latest_Census, 100 - 1);
		Latest_Census[length] = '\0';
		next = NULL;
	}
	record() {
		Country_Code[0] = '\0';
		Short_Name[0] = '\0';
		Table_Name[0] = '\0';
		Long_Name[0] = '\0';
		Alpha_Code[0] = '\0';
		Currency[0] = '\0';
		Region[0] = '\0';
		WB_Code[0] = '\0';
		Latest_Census[0] = '\0';
		next = NULL;
	}
	void copyTo(record& newRecord) {
		strcpy(newRecord.Alpha_Code, Alpha_Code);
		strcpy(newRecord.Country_Code, Country_Code);
		strcpy(newRecord.Currency, Currency);
		strcpy(newRecord.Latest_Census, Latest_Census);
		strcpy(newRecord.Long_Name, Long_Name);
		strcpy(newRecord.Region, Region);
		strcpy(newRecord.Short_Name, Short_Name);
		strcpy(newRecord.Table_Name, Table_Name);
		strcpy(newRecord.WB_Code, WB_Code);
		newRecord.next = next;
	}
	void print() {
		std::cout << "Country_Code "  << Country_Code << ",";
		std::cout << "Short_Name " << Short_Name << ",";
		std::cout << "Table_Name "<< Table_Name << ",";
		std::cout << "Long_Name " << Long_Name << ",";
		std::cout << "Alpha_Code " << Alpha_Code << ",";
		std::cout << "Currency " << Currency << ",";
		std::cout << "Region " << Region << ",";
		std::cout << "WB_Code " << WB_Code << ",";
		std::cout << "Latest_Census " << Latest_Census << "\n";

	}
};



//Size 60 bytes
struct userInfo {
	char userName[CHARLENGTH];
	char inAddress[CHARLENGTH];
	unsigned short ports[3]; //0: query port, 1: left port, 2: right port

};
//Size 176
struct message {
	int code; 
	int header; //0: query , 1: SUCCESS, -1: FAILURE
	char msgType[CHARLENGTH];	
	int num;
	int id;
	char senderName[CHARLENGTH];
	char inAddress[CHARLENGTH];
	unsigned short port[3]; //0: query port, 1: left port, 2: right port
	userInfo neighbors[2];
	record currentRecord;
	userInfo random;
	char searchName[100];

	message() {
		code = 0;
		header = 0;
		memset(msgType, 0, CHARLENGTH);
		num = 0;
		id = 0;
		memset(senderName, 0, CHARLENGTH);
		memset(inAddress, 0, CHARLENGTH);
		memset(&port, 0, sizeof(short) * 3);
		memset(neighbors[0].userName, 0, CHARLENGTH);
		memset(neighbors[0].inAddress, 0, CHARLENGTH);
		memset(neighbors[0].ports, 0, sizeof(short) * 3);
		memset(neighbors[1].userName, 0, CHARLENGTH);
		memset(neighbors[1].inAddress, 0, CHARLENGTH);
		memset(neighbors[1].ports, 0, sizeof(short) * 3);
		memset(random.userName, 0, CHARLENGTH);
		memset(random.inAddress, 0, CHARLENGTH);
		memset(random.ports, 0, sizeof(short) * 3);
		memset(searchName, 0, 100);
	}

	void printMessage() {
		if (header == 0) {
			switch (code) {
			case 1:
				std::cout << "register " << senderName << " " << inAddress;
				for (unsigned short x : port) {
					std::cout << " " << x;
				}
				std::cout << std::endl;
				break;
			case 2:
				std::cout << "setup-dht " << num << " " << senderName << std::endl;
				break;
			case 3:
				std::cout << "dht-complete " << senderName << std::endl;
				break;
			case 4:
				std::cout << "query-dht " << senderName << std::endl;
					break;
			case 5:
				std::cout << "leave-dht " << senderName << std::endl;
				break;
			//case 6:
				//std::cout << "dht-rebuilt " << senderName
			}
		}
		/*std::cout << "code: " << code << std::endl;
		std::cout << "header: " << header << std::endl;
		std::cout << "msgType: " << msgType << std::endl;
		std::cout << "num: " << num << std::endl;
		std::cout << "senderName " << senderName << std::endl;
		std::cout << "inAddress: " << inAddress << std::endl;
		for (int i = 0; i < 3; i++) {//for each loop not working, starts printing from port[1]
			std::cout << "port[" << i << "]: " << port[i] << "\n";
		}*/
		//std::cout << "Print neighbors function" << std::endl;    
	}

	void printMessage(int numUsers) {
		std::cout << "code: " << code << std::endl;
		std::cout << "header: " << header << std::endl;
		std::cout << "msgType: " << msgType << std::endl;
		std::cout << "num: " << num << std::endl;
		std::cout << "senderName " << senderName << std::endl;
		std::cout << "inAddress: " << inAddress << std::endl;
		for (int i = 0; i < 3; i++) {//for each loop not working, starts printing from port[1]
			std::cout << "port[" << i << "]: " << port[i] << "\n";
		}
		//std::cout << "Print neighbors function" << std::endl;    
		/*for (int i = 0; i < numUsers; i++) {
			std::cout << "user[" << i << "]: ";
			std::cout << random[i].userName << "\t";
			std::cout << random[i].inAddress << "\t";
			for (int j = 0; j < 3; j++) {
				std::cout << "ports[" << j << "] " << random[i].ports[j] << "\t";
			}
			std::cout << "\n";
		}*/
	}
};

std::vector<std::string> removeSpace(std::string str) {
	std::vector<std::string> result;
	std::string token;
	std::stringstream ss(str);
	while (getline(ss, token, ' ')) {
		result.push_back(token);
	}
	return result;
}

message createMessage(std::string str) {
	message msg;
	std::vector<std::string> segments = removeSpace(str);
	std::size_t length;
	if (segments[0] == "register") {
		msg.code = 1;
		msg.header = 0;
		length = segments[0].copy(msg.msgType, CHARLENGTH - 1);
		msg.msgType[length] = '\0';
		length = segments[1].copy(msg.senderName, CHARLENGTH - 1);
		msg.senderName[length] = '\0';
		length = segments[2].copy(msg.inAddress, CHARLENGTH - 1);
		msg.inAddress[length] = '\0';

		msg.port[0] = (short)std::stoi(segments[3]);
		// cout << "segments[3] = " << stoi(segments[3]) << endl;
		msg.port[1] = (short)std::stoi(segments[4]);
		msg.port[2] = (short)std::stoi(segments[5]);
	}
	else if (segments[0] == "setup-dht") {
		msg.code = 2;
		msg.header = 0;
		//length = segments[0].copy(msg.msgType, CHARLENGTH - 1);
		//msg.msgType[length] = '\0'; 
		msg.num = stoi(segments[1]);
		length = segments[2].copy(msg.senderName, CHARLENGTH - 1);
		msg.senderName[length] = '\0';
	}
	else if (segments[0] == "dht-complete") {
		msg.code = 3;
		msg.header = 0;
		//length = segments[0].copy(msg.msgType, CHARLENGTH - 1);
		//msg.msgType[length] = '\0'; 
		length = segments[1].copy(msg.senderName, CHARLENGTH - 1);
		msg.senderName[length] = '\0';
	}
	else if (segments[0] == "query-dht") {
		msg.code = 4;
		msg.header = 0;
		//length = segments[0].copy(msg.msgType, CHARLENGTH - 1);
		//msg.msgType[length] = '\0'; 
		length = segments[1].copy(msg.senderName, CHARLENGTH - 1);
		msg.senderName[length] = '\0';
	}
	else if (segments[0] == "leave-dht") {
		msg.code = 5;
		msg.header = 0;
		length = segments[1].copy(msg.senderName, CHARLENGTH - 1);
		msg.senderName[length] = '\0';
	}
	//---NOT FINISHED---
	else if (segments[0] == "dht-rebuilt") {
		msg.code = 6;
		msg.header = 0;
		length = segments[1].copy(msg.senderName, CHARLENGTH - 1);
		msg.senderName[length] = '\0';
	}
	return msg;
}
/*
char* encode(message msg, int* size) {
	char* arr;//= new char[176];

	int index = 0;
	switch (msg.code) {
	case -1://FAILURE
		arr = new char[4];
		*size = 4;
		memcpy(arr, std::to_string(msg.code).c_str(), sizeof(int));
		break;
	case 0://SUCCESS
		arr = new char[4];
		*size = 4;
		memcpy(arr, std::to_string(msg.code).c_str(), sizeof(int));
		break;
	case 1://register <user-name> <ip-Address> <query-port>  <left-port> <right-port>    
		arr = new char[64];
		*size = 64;
		memcpy(arr, std::to_string(msg.code).c_str(), sizeof(int));
		index += sizeof(int);
		//memcpy(arr, msg.msgType, CHARLENGTH);    
		memcpy(arr + index, msg.senderName, CHARLENGTH);
		index += CHARLENGTH;
		memcpy(arr + index, msg.inAddress, CHARLENGTH);
		index += CHARLENGTH;
		memcpy(arr + index, std::to_string(msg.port[0]).c_str(), 6);
		/* char pp[9];
		memcpy(pp, std::to_string(msg.port[0]).c_str(), 6);
		for(int i =0; i < 10; i++){
		  std::cout << "pp[" << i << "]: " <<  pp[i] << std::endl;
		}
	   {
		  unsigned short s = (short)atoi(pp);
		  std::cout << "Short value is: " << s << std::endl;
		  }
		index += 6;
		memcpy(arr + index, std::to_string(msg.port[1]).c_str(), 6);
		index += 6;
		memcpy(arr + index, std::to_string(msg.port[2]).c_str(), 6);
		index += 6;
		break;
	case 2://setup-dht <n> <user-name>
		arr = new char[29];
		*size = 29;
		memcpy(arr, std::to_string(msg.code).c_str(), sizeof(int));
		index += sizeof(int);
		memcpy(arr + index, std::to_string(msg.num).c_str(), sizeof(int));
		index += sizeof(int);
		memcpy(arr + index, msg.senderName, CHARLENGTH);
		break;
	case 3: //dht-complete <user-name>
		arr = new char[25];
		*size = 25;
		memcpy(arr, std::to_string(msg.code).c_str(), sizeof(int));
		index += sizeof(int);
		memcpy(arr + index, msg.senderName, CHARLENGTH);
		break;
	case 4: // query-dht <user-name>
		arr = new char[25];
		*size = 25;
		memcpy(arr, std::to_string(msg.code).c_str(), sizeof(int));
		index += sizeof(int);
		memcpy(arr + index, msg.senderName, CHARLENGTH);
		break;
	case 20:
		arr = new char[4 + 60 * (msg.num)];//60
		*size = 4 + 60 * (msg.num);
		memcpy(arr, std::to_string(msg.code).c_str(), sizeof(int));
		index += sizeof(int);
		memcpy(arr + index, std::to_string(msg.num).c_str(), sizeof(int));
		index += sizeof(int);
		for (int i = 0; i < msg.num; i++) {
			memcpy(arr + index, msg.random[i].userName, CHARLENGTH);
			index += CHARLENGTH;
			memcpy(arr + index, msg.random[i].inAddress, CHARLENGTH);
			index += CHARLENGTH;
			for (int j = 0; j < 3; j++) {
				memcpy(arr + index, std::to_string(msg.random[i].ports[j]).c_str(), 6);
				index += 6;
			}
		}
		break;
	}
	return arr;
}

message decode(char* arr) {
	message msg;
	char buffer[CHARLENGTH]; //Create buffer to use memcpy on
	memset(buffer, 0, CHARLENGTH);//Initialize to '\0'
	memcpy(buffer, arr, sizeof(int));
	//  std::cout << "buffer: " << buffer << "\n"; 

	int index = 0;
	msg.code = atoi(buffer);
	index += sizeof(int);
	switch (msg.code) {
	case -1:
		std::cout << "Code decoded corresponds to FAILURE\n";
		break;
	case 0:
		std::cout << "Code decoded corresponds to SUCCESS\n";
		break;
	case 1:
		std::cout << "Code decoded corresponds to register\n";
		memcpy(msg.senderName, arr + index, CHARLENGTH);
		index += CHARLENGTH;
		memcpy(msg.inAddress, arr + index, CHARLENGTH);
		index += CHARLENGTH;
		for (int i = 0; i < 3; i++) {
			memcpy(buffer, arr + index, 6);
			msg.port[i] = (unsigned short)atoi(buffer);
			index += 6;
		}
		break;
	case 2:
		std::cout << "Code decoded corresponds to setup-dht\n";
		memcpy(buffer, arr + index, sizeof(int));
		msg.num = atoi(buffer);
		index += sizeof(int);
		memcpy(msg.senderName, arr + index, CHARLENGTH);
		break;
	case 3:
		std::cout << "Code decoded corresponds to dht-complete\n";
		memcpy(msg.senderName, arr + index, CHARLENGTH);
		break;
	case 4:
		std::cout << "Code decoded corresponds to query-dht\n";
		memcpy(msg.senderName, arr + index, CHARLENGTH);
		break;
	case 20:
		std::cout << "Code corresponds to SUCCESS for setup-dht\n";
		memcpy(buffer, arr + index, sizeof(int));
		msg.num = atoi(buffer);
		index += sizeof(int);
		msg.random = new userInfo[msg.num];
		for (int i = 0; i < msg.num; i++) {
			memcpy(msg.random[i].userName, arr + index, CHARLENGTH);
			index += CHARLENGTH;
			memcpy(msg.random[i].inAddress, arr + index, CHARLENGTH);
			index += CHARLENGTH;
			for (int j = 0; j < 3; j++) {
				memcpy(buffer, arr + index, 6);
				msg.random[i].ports[j] = (unsigned short)atoi(buffer);
				index += 6;
			}
		}
		return msg;
	}
}
*/

/*
int main(){
  userInfo cat;
  std::cout << "Size of userInfo is: " << sizeof(struct userInfo) << std::endl;
  std::cout << "Size of message is: " << sizeof(message) << std::endl;
}
*/


