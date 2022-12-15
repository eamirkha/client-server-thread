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

#include "UDPSocket.h"
#include <cstring>
#include <stdio.h>
#include <cstdio>



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



bool sign_in(UDPSocket* sock, sockaddr_in* si_dest)
{
	int dest_len = sizeof(*si_dest);
	char buffer[BUFLEN];
	char msg[BUFLEN];

	while (1)
	{
		printf("\nPlease enter login : ");
		gets_s(msg, BUFLEN);

		//send the message
		sock->SendDatagram(msg, (int)strlen(msg), (struct sockaddr*)si_dest, dest_len);

		//receive a reply and print it
		//clear the buffer by filling null, it might have previously received data
		memset(buffer, '\0', BUFLEN);
		//try to receive some data, this is a blocking call

		sock->RecvDatagram(buffer, BUFLEN, (struct sockaddr*)si_dest, &dest_len);

		puts(buffer);

		//check response if login is ok
		if (!strcmp(buffer, "OK"))
		{
			printf("\nPlease enter password : ");
			gets_s(msg, BUFLEN);

			//send the message
			sock->SendDatagram(msg, (int)strlen(msg), (struct sockaddr*)si_dest, dest_len);
			//return true;

			//receive a reply and print it
			//clear the buffer by filling null, it might have previously received data
			memset(buffer, '\0', BUFLEN);
			//try to receive some data, this is a blocking call

			sock->RecvDatagram(buffer, BUFLEN, (struct sockaddr*)si_dest, &dest_len);

			puts(buffer);
			//check response if password is ok
			if (!strcmp(buffer, "OK"))
			{
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
	UDPSocket* socketarg;
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

	char buf[BUFLEN];
	char message[BUFLEN];

	unsigned short srv_port = 0;
	char srv_ip_addr[40];
	memset(srv_ip_addr, 0, 40);

	//create socket
	UDPSocket client_sock;

	//setup address structure
	memset((char*)&si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;

	//si_other.sin_port = htons(PORT);
	//si_other.sin_addr.S_un.S_addr = inet_addr(SERVER);

	if (1 == argc)
	{
		si_other.sin_port = htons(PORT);
		si_other.sin_addr.S_un.S_addr = inet_addr(SERVER);
		printf("1: Server - addr=%s , port=%d\n", SERVER, PORT);
	}
	else if (2 == argc)
	{
		//		si_other.sin_port = htons(PORT);
		//		si_other.sin_addr.S_un.S_addr = inet_addr(argv[1]);
		si_other.sin_port = htons(atoi(argv[1]));
		si_other.sin_addr.S_un.S_addr = inet_addr(SERVER);
		printf("2: argv[0]: Server - addr=%s , port=%d\n", SERVER, atoi(argv[1]));
	}
	else
	{
		si_other.sin_port = htons(atoi(argv[2]));
		si_other.sin_addr.S_un.S_addr = inet_addr(argv[1]);
		printf("3: Server - addr=%s , port=%d\n", argv[1], atoi(argv[2]));
	}

	//sign_in function call
	if (!sign_in(&client_sock, &si_other))
	{
		puts("Can`t login\n");
		exit(0);

	}


	// prepare parameter(s) for the async thread
//	DWORD_PTR* svRecvThrArgs = new DWORD_PTR[1];
	// pass the thread lock variable as parameter
	//svRecvThrArgs[0] = (DWORD_PTR)&recv_lock;


	threadargs svRecvThrArgs;
	svRecvThrArgs.lockarg = (PDWORD_PTR)&recv_lock;
	svRecvThrArgs.socketarg = &client_sock;


	// start the thread with parameters
	THREADVAR recvThrHandle = SpawnThread(MyAsyncThread, (THREADFUNCARGS)&svRecvThrArgs);

	// loop increment and check for exit
	while (true) {
		///*
		LockThread(recv_lock); // lock with the same var

		printf("\nEnter message : ");
		gets_s(message, BUFLEN);

		//send the message
		client_sock.SendDatagram(message, (int)strlen(message), (struct sockaddr*)&si_other, slen);	
		UnlockThread(recv_lock); // unlock with the same var
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
	UDPSocket* psocket = pthrargs->socketarg;

	THREAD_LOCK& ref_recv_lock = *((THREAD_LOCK*)(pthrargs->lockarg));


	sockaddr_in  si_dest;

	int dest_len = sizeof(si_dest);
	char buffer[BUFLEN];
	char msg1[BUFLEN];

	// loop increment, check for exit and print
	while (true) {
		
		psocket->RecvDatagram(buffer, BUFLEN, (struct sockaddr*)&si_dest, &dest_len);

		puts(buffer);
		
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

