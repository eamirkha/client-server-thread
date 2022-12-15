// SimpleThread.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

///////////////////////////////////////////////////////////////////////////////////////////////////////////
///                                                                                                      //
///                           cross platform multithreading made simple                                  //
///                                                                                                      //
///////////////////////////////////////////////////////////////////////////////////////////////////////////
///                                                                                                      //
/// compile on linux with g++ by adding  -D_COMPILE_LINUX  and also  -pthread  to the command line.      //
///                                                                                                      //
///////////////////////////////////////////////////////////////////////////////////////////////////////////
///                                                                                                      //
/// compile on windows with cl.exe by adding  /D_COMPILE_WINDOWS  to                                     //
/// the command line BEFORE the /LINK argument or it will not compile                                    //
///                                                                                                      //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <iostream>
using namespace std;

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
#define PORT    8888			//The port on which to listen for incoming dat



//crossplatform type definitions
#ifdef _COMPILE_WINDOWS
//#ifdef _WIN32
#include <Windows.h> //for Sleep and other time funcs, and thread funcs
typedef HANDLE THREADVAR;
typedef DWORD WINAPI THREADFUNCVAR;
typedef LPVOID THREADFUNCARGS;
typedef THREADFUNCVAR(*THREADFUNC)(THREADFUNCARGS);
typedef CRITICAL_SECTION THREAD_LOCK;
#endif
#ifdef _COMPILE_LINUX
#include <pthread.h>
#include <unistd.h>
typedef pthread_t THREADVAR;
typedef void* THREADFUNCVAR;
typedef void* THREADFUNCARGS;
typedef THREADFUNCVAR(*THREADFUNC)(THREADFUNCARGS);
typedef pthread_mutex_t THREAD_LOCK;
typedef unsigned long int DWORD_PTR;
typedef unsigned int DWORD;
typedef unsigned long long int uint64;
typedef long long int int64;
#endif



//cross platform funcs
THREADVAR SpawnThread(THREADFUNC f, THREADFUNCARGS arg);
void StopThread(THREADVAR t);
void InitThreadLock(THREAD_LOCK& t);
void LockThread(THREAD_LOCK& t);
void UnlockThread(THREAD_LOCK& t);
void sleep(int ms);

// the thread function declaration
THREADFUNCVAR MyAsyncThread(THREADFUNCARGS lpParam);

// globals
int globlint = 0;
bool quitnow = false;



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




struct  threadargs {
	DWORD_PTR* lockarg;
	UDPServer* serverarg;
};






///////////////////////////////////////////////////////////////////////////////////////////////////////////
///                                                                                                      //
/// entry point                                                                                          //
///                                                                                                      //
///////////////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[]) {
	THREAD_LOCK recv_lock;
	InitThreadLock(recv_lock);


	//	SOCKET s;
	struct sockaddr_in si_other;
	int slen = sizeof(si_other);
	unsigned short srvport;
	char buf[BUFLEN];
	char msg[BUFLEN];
	srvport = (1 == argc) ? PORT : atoi(argv[1]);


	//create socket
	UDPServer server(srvport);

	if (!sign_in(&server, &si_other))
	{
		puts("Failed authentication\n");
		exit(0);
	}

	threadargs svRecvThrArgs;
	svRecvThrArgs.lockarg = (PDWORD_PTR)&recv_lock;
	svRecvThrArgs.serverarg = &server;


	// start the thread with parameters
	THREADVAR recvThrHandle = SpawnThread(MyAsyncThread, (THREADFUNCARGS)&svRecvThrArgs);

	// loop increment and check for exit
	while (true) {
		///*
		LockThread(recv_lock); // lock with the same var

		printf("\nEnter message : ");
		gets_s(msg, BUFLEN);

		//send the message
		server.SendDatagram(msg, (int)strlen(msg), (struct sockaddr*)&si_other, slen);
//*/
		sleep(10);
		//		cout << "\nmain loop, after sleep\n";
	}

	StopThread(recvThrHandle);

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
///                                                                                                      //
/// thread                                                                                               //
///                                                                                                      //
///////////////////////////////////////////////////////////////////////////////////////////////////////////
THREADFUNCVAR MyAsyncThread(THREADFUNCARGS lpParam) {
	// get parameter array
//	DWORD_PTR* arg = (DWORD_PTR*)lpParam;
	// get 0th param, thats the thread lock variable used in main func

	threadargs* pthrargs = (threadargs*)lpParam;
	UDPServer* pserver = pthrargs->serverarg;

	THREAD_LOCK& ref_recv_lock = *((THREAD_LOCK*)(pthrargs->lockarg));


	sockaddr_in  si_other;

	int slen = sizeof(si_other);
	char buf[BUFLEN];
	char msg1[BUFLEN];

	// loop increment, check for exit and print
	while (true) {
		
		pserver->RecvDatagram(buf, BUFLEN, (struct sockaddr*)&si_other, &slen);

		puts(buf);
		
	}
	return NULL;
}






///////////////////////////////////////////////////////////////////////////////////////////////////////////
///                                                                                                      //
/// cross platform definitions                                                                           //
///                                                                                                      //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

THREADVAR SpawnThread(THREADFUNC f, THREADFUNCARGS arg) {
#ifdef _COMPILE_LINUX
	pthread_t out;
	pthread_create(&out, NULL, f, arg);
	return out;
#endif
#ifdef _COMPILE_WINDOWS
	DWORD thrId;
	THREADVAR out = CreateThread(
		NULL,          // default security attributes
		0,             // use default stack size  
		(LPTHREAD_START_ROUTINE)f,    			// thread function name
		arg,          // argument to thread function 
		0,             // use default creation flags 
		&thrId			// returns the thread identifier 
	);
	return out;
#endif
}

void StopThread(THREADVAR t) {
#ifdef _COMPILE_LINUX
	pthread_exit((void*)t);
#endif
#ifdef _COMPILE_WINDOWS
	TerminateThread(t, 0);
	CloseHandle(t);
#endif
}


void InitThreadLock(THREAD_LOCK& t) {
#ifdef _COMPILE_LINUX
	t = PTHREAD_MUTEX_INITIALIZER;
	//UnlockThread(t);
	//pthread_mutex_init(&t, NULL);
#endif
#ifdef _COMPILE_WINDOWS
	InitializeCriticalSection(&t);
#endif
}

void LockThread(THREAD_LOCK& t) {
#ifdef _COMPILE_LINUX
	pthread_mutex_lock(&t);
#endif
#ifdef _COMPILE_WINDOWS
	EnterCriticalSection(&t);
#endif
}

void UnlockThread(THREAD_LOCK& t) {
#ifdef _COMPILE_LINUX
	pthread_mutex_unlock(&t);
#endif
#ifdef _COMPILE_WINDOWS
	LeaveCriticalSection(&t);
#endif
}


void sleep(int ms) {
#ifdef _COMPILE_LINUX
	usleep(ms * 1000);   // usleep takes sleep time in us (1 millionth of a second)
#endif
#ifdef _COMPILE_WINDOWS
	Sleep(ms);
#endif
}

