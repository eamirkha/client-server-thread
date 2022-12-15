/*
	UDP Server
*/

#include <stdio.h>
#include "UDPServer.h"

#ifndef _WIN32
using SOCKET = int
#define WSAGetLastError() 1
#else
#include <winsock2.h>
#pragma comment(lib,"ws2_32.lib") //Winsock Library
#endif

//#define SERVER "127.0.0.1"	//ip address of udp server
#define BUFLEN 	1024		    //Max length of buffer
#define PORT    8888			//The port on which to listen for incoming data

bool sign_in(UDPServer* serv, sockaddr_in* si_dest)
{
	int dest_len = sizeof(*si_dest);
	char buffer[BUFLEN];
	char msg_login[BUFLEN] = "Incorrect login";
	char msg_passwd[BUFLEN] = "Incorrect password";
	char msg_ok[BUFLEN] = "OK";

	while (1)
	{
		printf("\nWaiting for login...   ");
		fflush(stdout);
		//clear the buffer by filling null, it might have previously received data
		memset(buffer, '\0', BUFLEN);

		//try to receive some data, this is a blocking call
		serv->RecvDatagram(buffer, BUFLEN, (struct sockaddr*)&si_dest, &dest_len);

		//check the login
		if (strcmp(buffer, "client"))
		{
			//send error message
			serv->SendDatagram(msg_login, (int)strlen(msg_login), (struct sockaddr*)&si_dest, dest_len);
		}
		else
		{
			//send 'OK'
			serv->SendDatagram(msg_ok, (int)strlen(msg_ok), (struct sockaddr*)&si_dest, dest_len);
			printf("\nWaiting for password...   ");
			fflush(stdout);
			//clear the buffer by filling null, it might have previously received data
			memset(buffer, '\0', BUFLEN);

			//try to receive some data, this is a blocking call
			serv->RecvDatagram(buffer, BUFLEN, (struct sockaddr*)&si_dest, &dest_len);

			//check password
			if (strcmp(buffer, "CLIENT"))
			{
				//send error message
				serv->SendDatagram(msg_passwd, (int)strlen(msg_passwd), (struct sockaddr*)&si_dest, dest_len);
			}
			else
			{
				//send 'OK'
				serv->SendDatagram(msg_ok, (int)strlen(msg_ok), (struct sockaddr*)&si_dest, dest_len);
				return true;
			}
			break;
		}
		break;
	}
	return false;
}

int main(int argc, char* argv[])
{
	struct sockaddr_in si_other;
	unsigned short srvport;
	int slen;
	char buf[BUFLEN];
	char msg[BUFLEN];

	srvport = (1 == argc) ? PORT : atoi(argv[1]);

	UDPServer server(srvport);
	slen = sizeof(si_other);

	//sign-in call
	if (!sign_in(&server, &si_other))
	{
		puts("Failed authentication\n");
		exit(0);
	}

	//keep listening for data
	while (1)
	{
		printf("\nWaiting for data...   ");
		fflush(stdout);

		//clear the buffer by filling null, it might have previously received data
		memset(buf, '\0', BUFLEN);

		//try to receive some data, this is a blocking call
		server.RecvDatagram(buf, BUFLEN, (struct sockaddr*)&si_other, &slen);

		//print details of the client/peer and the data received
		printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));
		printf("Data: %s\n", buf);

		printf("\nAnswer : ");
		gets_s(msg, BUFLEN);
		//now reply the client with the same data

		server.SendDatagram(msg, (int)strlen(msg), (struct sockaddr*)&si_other, slen);

	}

	return 0;
}