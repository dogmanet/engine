
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

#include <stdio.h>
#include <stdlib.h>

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <conio.h>
#include <memory>
#include <process.h>

#include <winsock2.h>
#include <ws2tcpip.h>

#include <common/Array.h>
#include <math.h>

#include "ColorPrint.h"

#include <mutex>


typedef std::mutex Mutex;
typedef std::unique_lock<Mutex> ScopedLock;

#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
//#define DEFAULT_PORT "59705"
//#define COMMAND_PORT "59706"

char * g_szUserInp = NULL;
HANDLE g_hStdOut = NULL;
COORD g_iOldOutPos;
ColorPrint * g_pColor;

bool g_bRunning = true;
SOCKET g_iListenSocket = INVALID_SOCKET;
SOCKET g_iRecvSocket = INVALID_SOCKET;
SOCKET g_iSendSocket = INVALID_SOCKET;

AnsiColor g_iCurColorFG;
AnsiColor g_iCurColorBG;

char g_szServerBind[64] = "127.0.0.1";
int g_iServerPort = 59705;
char g_szServerPort[8];

bool g_bExitOnDisconnect = false;

#define DEFAULT_PORT g_szServerPort

Mutex mx;

struct HistoryItem
{
	const char * szOrigCmd;
	char * szEditCmd;
};

Array<HistoryItem> g_vHistory;
int g_iHistoryPointer;

#pragma warning(disable:4996)

char * GetHistItemPtr()
{
	HistoryItem * hi = &g_vHistory[g_iHistoryPointer];
	if(!hi->szEditCmd)
	{
		hi->szEditCmd = new char[DEFAULT_BUFLEN];
		hi->szEditCmd[0] = 0;
		if(hi->szOrigCmd)
		{
			strcpy(hi->szEditCmd, hi->szOrigCmd);
		}
	}
	return(hi->szEditCmd);
}

void PutHistory()
{
	HistoryItem * hi = &g_vHistory[g_iHistoryPointer];
	char * cmd = hi->szEditCmd;
	hi->szEditCmd = NULL;
	if(!strlen(cmd))
	{
		delete[] cmd;
		g_iHistoryPointer = g_vHistory.size() - 1;
		return;
	}
	if(hi->szOrigCmd)
	{
		//put new item
		g_iHistoryPointer = g_vHistory.size() - 2;
		if(strcmp(g_vHistory[g_iHistoryPointer].szOrigCmd, cmd)) // last cmd not equals current
		{
			hi = &g_vHistory[g_vHistory.size() - 1];
			hi->szOrigCmd = cmd;
			cmd = hi->szEditCmd;
			hi->szEditCmd = NULL;
			g_vHistory.push_back({NULL, cmd});
			g_iHistoryPointer = g_vHistory.size() - 1;
		}
		else
		{
			g_iHistoryPointer = g_vHistory.size() - 1;
			delete[] g_vHistory[g_iHistoryPointer].szEditCmd;
			g_vHistory[g_iHistoryPointer].szEditCmd = cmd;
			cmd[0] = 0;
		}
	}
	else
	{
		g_iHistoryPointer = g_vHistory.size() - 2;
		if(g_iHistoryPointer < 0 || strcmp(g_vHistory[g_iHistoryPointer].szOrigCmd, cmd)) // last cmd not equals current
		{
			hi->szOrigCmd = cmd;
			++g_iHistoryPointer;
			g_vHistory.push_back({NULL, NULL});
		}
		else
		{
			hi->szEditCmd = cmd;
			cmd[0] = 0;
		}
		++g_iHistoryPointer;
	}
}

void SetColor(AnsiColor clr)
{
	g_iCurColorFG = clr;
	g_pColor->setColor(clr);
}

void SetColorBG(AnsiColor clr)
{
	g_iCurColorBG = clr;
	g_pColor->setBG(clr);
}

void SyncOutput(bool begin)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(g_hStdOut, &csbi);
	if(csbi.dwSize.Y - csbi.dwCursorPosition.Y < 100)
	{
		csbi.dwSize.Y += 100;
		SetConsoleScreenBufferSize(g_hStdOut, csbi.dwSize);
	}
	if(begin)
	{
		//if(csbi.dwCursorPosition.X != 0)
		{
			char * str = (char*)alloca(sizeof(char)* (csbi.dwMaximumWindowSize.X + 1));
			memset(str, ' ', csbi.dwMaximumWindowSize.X);
			str[csbi.dwMaximumWindowSize.X] = 0;
			csbi.dwCursorPosition.X = 0;
			SetConsoleCursorPosition(g_hStdOut, csbi.dwCursorPosition);
			printf(str);
		}
		SetConsoleCursorPosition(g_hStdOut, g_iOldOutPos);
		g_pColor->setColor(g_iCurColorFG);
		g_pColor->setBG(g_iCurColorBG);
	}
	else
	{
		g_iOldOutPos = csbi.dwCursorPosition;

		if(csbi.dwCursorPosition.X != 0)
		{
			csbi.dwCursorPosition.X = 0;
			++csbi.dwCursorPosition.Y;
			SetConsoleCursorPosition(g_hStdOut, csbi.dwCursorPosition);
		}
		g_pColor->setColor(ANSI_LGREEN);
		if(g_szUserInp)
		{
			char * s;
			if((s = strstr(g_szUserInp, "\n")))
			{
				*s = 0;
			}
			printf("> %s", g_szUserInp);
		}
		else
		{
			printf("> ");
		}
		//g_pColor->setDefault();
	}
}

void ClearAll()
{
	mx.lock();
	SyncOutput(1);

	COORD tl = {0, 0};
	CONSOLE_SCREEN_BUFFER_INFO s;
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(console, &s);
	DWORD written, cells = s.dwSize.X * s.dwSize.Y;
	FillConsoleOutputCharacterA(console, ' ', cells, tl, &written);
	FillConsoleOutputAttribute(console, s.wAttributes, cells, tl, &written);
	SetConsoleCursorPosition(console, tl);

	SyncOutput(0);
	mx.unlock();
}

void WriteOutput(const char * buf, ...)
{
	mx.lock();
	va_list va;
	va_start(va, buf);
	SyncOutput(1);
	vprintf(buf, va);
	SyncOutput(0);
	va_end(va);
	mx.unlock();
}

void WriteColored(char ** _buf)
{
	mx.lock();
	//mx.lock();
	SyncOutput(1);
	char * esc = NULL;
	char * buf = *_buf;

	int clr;
	bool done = false;
	while(!done)
	{
		esc = strstr(buf, "\033");
		if(esc)
		{
			*esc = 0;
			printf("%s", buf);
			*esc = '\033';
			++esc;

			if(esc[0] == ']' && esc[1] == '0' && esc[2] == ';')
			{
				char *szEnd = strstr(esc, "\a");
				if(!szEnd)
				{
					*_buf = esc - 1;
					goto end;
				}
				else
				{
					*szEnd = 0;
					buf = szEnd + 1;
					esc += 3;
					SetConsoleTitleA(esc);
					continue;
				}
			}

			if(!strstr(esc, "m") && strlen(esc) < 20)
			{
				*_buf = esc - 1;
				goto end;
			}
			if(*esc != '[')
			{
				buf = esc;
				continue;
			}
			++esc;
			if(!memcmp(esc, "38;5;", 5))
			{
				esc += 5;
				if(!sscanf(esc, "%d", &clr))
				{
					buf = esc;
					continue;
				}
				esc += (int)(log10(clr) + 1);
				if(*esc != 'm')
				{
					buf = esc;
					continue;
				}
				++esc;
				//swap 0 - 2 bits
				int r = clr & 1;
				int b = (clr >> 2) & 1;
				if(r)
				{
					clr |= 4;
				}
				else
				{
					clr &= ~4;
				}
				if(b)
				{
					clr |= 1;
				}
				else
				{
					clr &= ~1;
				}
				SetColor((AnsiColor)clr);
			}
			else if(!memcmp(esc, "48;5;", 5))
			{
				esc += 5;
				if(!sscanf(esc, "%d", &clr))
				{
					buf = esc;
					continue;
				}
				esc += (int)(floor(log10(clr)) + 1);
				if(*esc != 'm')
				{
					buf = esc;
					continue;
				}
				++esc;
				//swap 0 - 2 bits
				int r = clr & 1;
				int b = (clr >> 2) & 1;
				if(r)
				{
					clr |= 4;
				}
				else
				{
					clr &= ~4;
				}
				if(b)
				{
					clr |= 1;
				}
				else
				{
					clr &= ~1;
				}
				SetColorBG((AnsiColor)clr);
			}
			else if(!memcmp(esc, "0m", 2))
			{
				//reset color
				SetColor(g_pColor->getDefaultFG());
				SetColorBG(g_pColor->getDefaultBG());
				esc += 2;
			}
			buf = esc;
		}
		else
		{
			done = true;
		}
	}
	int len = strlen(buf);

#define IS_UTF8_HEAD0(chr) ((unsigned char)(chr) < 128)
#define IS_UTF8_TAIL(chr) ((unsigned char)(chr) >= 128 && (unsigned char)(chr) <= 191)
#define IS_UTF8_HEAD2(chr) ((unsigned char)(chr) >= 192 && (unsigned char)(chr) <= 223)
#define IS_UTF8_HEAD3(chr) ((unsigned char)(chr) >= 224 && (unsigned char)(chr) <= 239)
#define IS_UTF8_HEAD4(chr) ((unsigned char)(chr) >= 240 && (unsigned char)(chr) <= 247)
#define IS_UTF8_HEAD5(chr) ((unsigned char)(chr) >= 248 && (unsigned char)(chr) <= 251)


	int i = len - 1;
	while(len >= 0 && IS_UTF8_TAIL(buf[i]))
	{
		--i;
	}

	if(IS_UTF8_HEAD0(buf[i])
		|| (IS_UTF8_HEAD2(buf[i]) && i + 1 < len)
		|| (IS_UTF8_HEAD3(buf[i]) && i + 2 < len)
		|| (IS_UTF8_HEAD4(buf[i]) && i + 3 < len)
		|| (IS_UTF8_HEAD5(buf[i]) && i + 4 < len)
		)
	{
		goto good;
	}
	
	char chr = buf[i];
	buf[i] = 0;
	printf("%s", buf);
	buf[i] = chr;

	*_buf = &buf[i];

	goto end;

good:
	printf("%s", buf);
	*_buf = NULL;

end:
	SyncOutput(0);
	mx.unlock();
}

bool g_bConnected = false;

int iScreenWidth = 0, iScreenHeight = 0;

void threadCommander(void*)
{
	int iResult = 0;
	g_iSendSocket = accept(g_iListenSocket, NULL, NULL);
	if(g_iSendSocket == INVALID_SOCKET)
	{
		SetColor(ANSI_RED);
		WriteOutput("accept failed with error: %d\n", WSAGetLastError());
		SetColor(g_pColor->getDefaultFG());
		return;
	}
	closesocket(g_iListenSocket);

	while(g_bConnected)
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		GetConsoleScreenBufferInfo(g_hStdOut, &csbi);
		int iWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;
		int iHeight = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
		if(iWidth != iScreenWidth)
		{
			char buf[64];
			sprintf(buf, "con_width %d\n", iWidth);
			send(g_iSendSocket, buf, strlen(buf), 0);
			iScreenWidth = iWidth;
		}
		if(iHeight != iScreenHeight)
		{
			char buf[64];
			sprintf(buf, "con_height %d\n", iHeight);
			send(g_iSendSocket, buf, strlen(buf), 0);
			iScreenHeight = iHeight;
		}
		Sleep(10);
	}

	iResult = shutdown(g_iSendSocket, SD_SEND);
	if(iResult == SOCKET_ERROR)
	{
		SetColor(ANSI_RED);
		WriteOutput("shutdown failed with error: %d\n", WSAGetLastError());
		SetColor(g_pColor->getDefaultFG());
	}
	closesocket(g_iSendSocket);
	g_iSendSocket = INVALID_SOCKET;
}

void threadServer(void*)
{
	int iResult;

	struct addrinfo *result = NULL;
	struct addrinfo hints;

	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;
	
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(g_szServerBind, DEFAULT_PORT, &hints, &result);
	if(iResult != 0)
	{
		SetColor(ANSI_RED);
		WriteOutput("getaddrinfo failed with error: %d\n", iResult);
		SetColor(g_pColor->getDefaultFG());
		return;
	}

	// Create a SOCKET for connecting to server
	g_iListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if(g_iListenSocket == INVALID_SOCKET)
	{
		SetColor(ANSI_RED);
		WriteOutput("socket failed with error: %d\n", WSAGetLastError());
		SetColor(g_pColor->getDefaultFG());
		freeaddrinfo(result);
		return;
	}

	// Setup the TCP listening socket
	iResult = bind(g_iListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if(iResult == SOCKET_ERROR)
	{
		SetColor(ANSI_RED);
		WriteOutput("bind failed with error: %d\n", WSAGetLastError());
		SetColor(g_pColor->getDefaultFG());
		freeaddrinfo(result);
		closesocket(g_iListenSocket);
		return;
	}

	freeaddrinfo(result);

	iResult = listen(g_iListenSocket, SOMAXCONN);
	if(iResult == SOCKET_ERROR)
	{
		SetColor(ANSI_RED);
		WriteOutput("listen failed with error: %d\n", WSAGetLastError());
		SetColor(g_pColor->getDefaultFG());
		closesocket(g_iListenSocket);
		return;
	}
	SetColor(ANSI_LCYAN);
	WriteOutput("Waiting for connection...\n");
	SetColor(g_pColor->getDefaultFG());
	// Accept a client socket

	int offset = 0;
	while(g_bRunning)
	{
		g_iRecvSocket = accept(g_iListenSocket, NULL, NULL);
		if(g_iRecvSocket == INVALID_SOCKET)
		{
			SetColor(ANSI_RED);
			WriteOutput("accept failed with error: %d\n", WSAGetLastError());
			SetColor(g_pColor->getDefaultFG());
			continue;
		}

		g_bConnected = true;
		
		_beginthread(threadCommander, 0, 0);

		ClearAll();
		SetColor(ANSI_LGREEN);
		WriteOutput("Connected!\n");
		SetColor(g_pColor->getDefaultFG());

		iScreenWidth = iScreenHeight = 0;

		do
		{
			iResult = recv(g_iRecvSocket, recvbuf + offset, recvbuflen - 1 - offset, 0);
			if(iResult > 0)
			{
				recvbuf[iResult + offset] = 0;
				char * buf = recvbuf;
				WriteColored(&buf);
				offset = 0;
				if(buf) // not fully parsed
				{
					memmove(recvbuf, buf, (offset = strlen(buf)) + 1);
				}
			}
			else if(iResult == 0)
			{
				SetColor(ANSI_RED);
				WriteOutput("Connection closed.\n");
				SetColor(g_pColor->getDefaultFG());
			}
			else
			{
				SetColor(ANSI_RED);
				WriteOutput("recv failed with error: %d\n", WSAGetLastError());
				SetColor(g_pColor->getDefaultFG());
				closesocket(g_iRecvSocket);
				g_iRecvSocket = INVALID_SOCKET;
				continue;
			}

		}
		while(iResult > 0);

		g_bConnected = false;
		// shutdown the connection since we're done
		iResult = shutdown(g_iRecvSocket, SD_SEND);
		if(iResult == SOCKET_ERROR)
		{
			SetColor(ANSI_RED);
			WriteOutput("shutdown failed with error: %d\n", WSAGetLastError());
			SetColor(g_pColor->getDefaultFG());
		}
		break;
	}

	closesocket(g_iRecvSocket);
	g_iRecvSocket = INVALID_SOCKET;

	if(g_iListenSocket != INVALID_SOCKET)
	{
		closesocket(g_iListenSocket);
	}

	if(g_bExitOnDisconnect)
	{
		ExitProcess(0);
	}
	else
	{
		_beginthread(threadServer, 0, 0);
	}
}

BOOL WINAPI HandlerRoutine(
	_In_ DWORD dwCtrlType
	)
{
	if(CTRL_CLOSE_EVENT == dwCtrlType)
	{
		if(g_iSendSocket != INVALID_SOCKET)
		{
			send(g_iSendSocket, "exit\n", strlen("exit\n"), 0);
		}
	}
	return(FALSE);
}

int main(int argc, char ** argv)
{
	SetConsoleOutputCP(CP_UTF8);
	g_pColor = new ColorPrint();
	g_hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(g_hStdOut, &csbi);

	g_iCurColorBG = g_pColor->getDefaultBG();
	g_iCurColorFG = g_pColor->getDefaultFG();


	//csbi.dwMaximumWindowSize.X;
	//csbi.dwMaximumWindowSize.Y;
	SetColor(ANSI_LCYAN);
	WriteOutput("SkyXEngine Console\n");
	SetColor(g_pColor->getDefaultFG());


	WSADATA wsaData;
	int iResult;
	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if(iResult != 0)
	{
		SetColor(ANSI_RED);
		WriteOutput("WSAStartup failed with error: %d\n", iResult);
		SetColor(g_pColor->getDefaultFG());
		return(1);
	}


	g_iHistoryPointer = 0;
	g_vHistory.push_back({NULL, NULL});



	for(int i = 1; i < argc - 1; ++i)
	{
		if(!strcmp(argv[i], "-bind"))
		{
			strcpy(g_szServerBind, argv[i + 1]);
		}
		else if(!strcmp(argv[i], "-port"))
		{
			int iPort = 0;
			sscanf(argv[i + 1], "%d", &iPort);
			if(iPort > 0)
			{
				g_iServerPort = iPort;
			}
		}
		else if(!strcmp(argv[i], "-exit-on-disconnect"))
		{
			int iVal = 0;
			sscanf(argv[i + 1], "%d", &iVal);
			if(iVal > 0)
			{
				g_bExitOnDisconnect = true;
			}
		}
	}
	
	sprintf(g_szServerPort, "%d", g_iServerPort);
	
	SetConsoleCtrlHandler(HandlerRoutine, TRUE);

	_beginthread(threadServer, 0, 0);

	int i = 0;

	char ch;

	//char str[64];
	//strcpy(str, "Some " COLOR_RED "red " COLOR_RESET "text\n");
	//WriteColored(str);

	//SetConsoleTitle(L"sxconsole");

	while(1)
	{
		ch = (char)getch();
		if(ch == '\b')
		{
			if(!g_szUserInp)
			{
				g_szUserInp = GetHistItemPtr();
				i = strlen(g_szUserInp);
			}
			putch(g_szUserInp[i] = ch);
			i -= 2;
			if(i < 0)
			{
				i = -1;
			}
			putch(' ');
			putch('\b');
			g_szUserInp[++i] = 0;
		}
		else if(ch == '\r')
		{
			if(!g_szUserInp)
			{
				g_szUserInp = GetHistItemPtr();
			}
			putch(g_szUserInp[i] = ch);
			g_szUserInp[i] = '\n';
			g_szUserInp[++i] = 0;
			if(g_iSendSocket != INVALID_SOCKET)
			{
				send(g_iSendSocket, g_szUserInp, strlen(g_szUserInp), 0);
				WriteOutput("] %s", g_szUserInp);
				PutHistory();
			}
			g_szUserInp = NULL;
			i = 0;
			//g_szUserInp[i = 0] = 0;
			WriteOutput("");
		}
		else if(ch == -32) // Up
		{
			int c = getch();
			switch(c)
			{
			case 72: // up
				--g_iHistoryPointer;
				if(g_iHistoryPointer < 0)
				{
					g_iHistoryPointer = 0;
				}
				g_szUserInp = GetHistItemPtr();
				i = strlen(g_szUserInp);
				WriteOutput("");
				break;
			case 80: // down
				++g_iHistoryPointer;
				if(g_iHistoryPointer >= (int)g_vHistory.size())
				{
					g_iHistoryPointer = g_vHistory.size() - 1;
				}
				g_szUserInp = GetHistItemPtr();
				i = strlen(g_szUserInp);
				WriteOutput("");
				break;
			case 75: // left
			case 77: // right
			case 83: // del
				break;
			}
		}
		else
		{
			if(!g_szUserInp)
			{
				g_szUserInp = GetHistItemPtr();
			}
			putch(g_szUserInp[i] = ch);
			g_szUserInp[++i] = 0;
		}
		
		//g_szUserInp
	}
	
	WSACleanup();

	delete g_pColor;
	return(0);
}
