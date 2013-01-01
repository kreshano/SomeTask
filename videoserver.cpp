#include <stdio.h>
#include "Socket.h"
#include <process.h>
#include <string>
#include <iostream>
using namespace std;

#define MAX_CONNECTIONS 100
#define NAME_SIZE 100
#define IP_SIZE 100

struct serverDescStruct
{
    char serverName[NAME_SIZE];
    char ip[IP_SIZE];
    int  port;
};

int totalServerCount = 0;

class ServerDescription
{
	/*//do it by struct
	char (*serverName)[NAME_SIZE];
	char (*ip)[IP_SIZE];
	int  *port;
	*/
	serverDescStruct *serverDesc;
public:
	ServerDescription()
	{
		serverDesc = 0;
		//nothing to do here.
	}
	void SetServerDescription(char *file)
	{
		FILE* fp = fopen(file,"r");

		if( !fp )
		{
			printf("Can not read description file.\n");
			exit(0);
		}

		while( !feof(fp) )
		{
			serverDesc = (serverDescStruct *)realloc(serverDesc,(totalServerCount+1)*sizeof(serverDescStruct));
			if( serverDesc )
			{
				fscanf(fp,"%s %s %d",(serverDesc+totalServerCount)->serverName,(serverDesc+totalServerCount)->ip,&(serverDesc+totalServerCount)->port);
				printf("data:%s %s %d\n",(serverDesc+totalServerCount)->serverName,(serverDesc+totalServerCount)->ip,&(serverDesc+totalServerCount)->port);
				totalServerCount++;
			}
		}
	}

	int SerachData(char* name)
	{
		int i;
		for(i=0;i<totalServerCount;i++)
		{
			if( !strcmp((serverDesc+i)->serverName,name) )
				return i;
		}
		return -1;
	}

	char* getIp(int index)
	{
		if( index<totalServerCount )
			return (serverDesc+index)->ip;
		else
			return "NULL";
	}

	int getPort(int index)
	{
		if( index<totalServerCount )
			return (serverDesc+index)->port;
		else
			return -1;
	}

};

ServerDescription serverDesc;//global server description var

class VideoServer
{
	int serverPort;
	SocketServer *myServer;
public:
	VideoServer()
	{
		serverPort = 0;
	}
	void SetVideoServer(char* port,char* file)
	{
		serverPort = atoi(port);
		serverDesc.SetServerDescription(file);
		myServer = 0;
	}

	void Error(char* errorMessage)
	{
		cout<<errorMessage<<endl;
		exit(0);
	}

	void createServer()
	{
		try
		{
			myServer=new SocketServer(serverPort,MAX_CONNECTIONS);
		}
		catch(char *err)
		{
			Error("Error in create server...\n");
			exit(-1);
		}
	}

	SocketServer* getServer()
	{
		return myServer;
	}

};

void sendToServerAndRecv(SocketClient sc,Socket* sockDesc)
{
	int flag = 1;
	string recvData;
	while(1)
	{
		recvData = sc.ReceiveLine();
		printf("%s\n",recvData.c_str());
		sockDesc->SendLine(recvData.c_str());
		if( flag && ( strstr(recvData.data(),"\r\n") || strstr(recvData.data(),"202 OK") || strstr(recvData.data(),"200 OK")) ) break;
		flag = 1;
	}
}

unsigned __stdcall doMyTask(void* sock)
{
	Socket* sockDesc = (Socket*) sock;
	int priority=0;
	int time=0;

	while (1) {

		string recvUserCommand = sockDesc->ReceiveLine();

		/*command parsing start*/
		char *buf = (char *)recvUserCommand.data();
		char *token[10];
		printf("%s\n",buf);

		//RECORD priority server filename time
		int i=0;
		token[i++] = strtok (buf," ");
		while(1)
		{
			token[i++] = strtok (NULL, " ");
			if( i > 4 )//5 part
				break;
		}
		printf("%s\n",token[0]);
		if( i<4 || strcmp(token[0],"RECORD") )
		{
			sockDesc->SendLine("Erorr.Syntex RECORD priority server filename time.\n");
			delete sockDesc;
			return -1;
		}
		int priority = atoi(token[2]);
		int serverID = serverDesc.SerachData(token[2]);

		if( serverID>=0 )
		{
			char *server = serverDesc.getIp(serverID);
			int port = serverDesc.getPort(serverID);
			string sendToVideoServer="";

			try {
				SocketClient sc(server, port);
				string recvData;
				while(1)
				{
					recvData = sc.ReceiveLine();
					if( strstr(recvData.data(),"\r\n") ) break;
				}

				sendToVideoServer="UADD 10101";
				sc.SendLine(sendToVideoServer);
				int flag = 0;
				while(1)
				{
					recvData = sc.ReceiveLine();
					printf("%s\n",recvData.c_str());
					sockDesc->SendLine(recvData.c_str());
					if( flag && ( strstr(recvData.data(),"\r\n") || strstr(recvData.data(),"202 OK")|| strstr(recvData.data(),"200 OK") ) ) break;
					flag = 1;
				}
				string unit=recvData.substr(0,recvData.find('\r'));
				sendToVideoServer = "LOAD " + unit + " " +token[3];
				printf("sendToVideoServer === %s\n",sendToVideoServer.data());
				sc.SendLine(sendToVideoServer);
				flag = 0;
				while(1)
				{
					recvData = sc.ReceiveLine();
					printf("%s\n",recvData.c_str());
					sockDesc->SendLine(recvData.c_str());
					if( flag && ( strstr(recvData.data(),"\r\n") || strstr(recvData.data(),"202 OK")|| strstr(recvData.data(),"200 OK") ) ) break;
					flag = 1;
				}


				sendToVideoServer = "CUER " + unit;
				printf("sendToVideoServer === %s\n",sendToVideoServer.data());
				sc.SendLine(sendToVideoServer);//minified the one line reply

				sendToVideoServer = "REC " + unit;
				printf("sendToVideoServer === %s\n",sendToVideoServer.data());
				sc.SendLine(sendToVideoServer);
				sendToServerAndRecv(sc,sockDesc);//minified the one line reply

				Sleep(atoi(token[4])*1000);

				sendToVideoServer = "STOP " + unit;
				printf("sendToVideoServer === %s\n",sendToVideoServer.data());
				sc.SendLine(sendToVideoServer);
				sendToServerAndRecv(sc,sockDesc);//minified the one line reply


				sendToVideoServer = "UNLD " + unit;
				printf("sendToVideoServer === %s\n",sendToVideoServer.data());
				sc.SendLine(sendToVideoServer);
				sendToServerAndRecv(sc,sockDesc);//minified the one line reply

				sendToVideoServer = "UCLS " + unit;
				printf("sendToVideoServer === %s\n",sendToVideoServer.data());
				sc.SendLine(sendToVideoServer);
				sendToServerAndRecv(sc,sockDesc);//minified the one line reply
			}
			catch(char *err)
			{
				sockDesc->SendLine("Error while connecting...\n");
				exit(-1);
			}
		}
		else
		{
			sockDesc->SendLine("Server not found...\n");
		}

	}
	return 0;
}

//run a for infinitly....
void runServer(SocketServer *myServer)
{
	while (1)
	{
		Socket* mySocket=myServer->Accept();
		unsigned ret;
		_beginthreadex(0,0,doMyTask,(void*)mySocket,0,&ret);
	}
}


int main(int argc, char* argv[]) {

	VideoServer myServer;
	if( argc<3 || argc>3 )
	{
		myServer.Error("now correct argument.Syntax:Assignment DescFile Port\n");
	}

	myServer.SetVideoServer(argv[1],argv[2]);
	myServer.createServer();

	runServer(myServer.getServer());

	return 0;
}
