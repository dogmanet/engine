
/***********************************************************
Copyright © Vitaliy Buturlin, Evgeny Danilovich, 2017, 2018
See the license in LICENSE
***********************************************************/

#include "concmd.h"

//#define WIN32_LEAN_AND_MEAN

//#include <Windows.h>
#include <Shellapi.h>

#include <winsock2.h>
#include <ws2tcpip.h>

#include <io.h>
#include <fcntl.h>

#include <process.h>

#include "common/ConcurrentQueue.h"

#include "cvars.h"

#pragma comment (lib, "Ws2_32.lib")

AssotiativeArray<String, ConCmd> g_mCmds;

SOCKET g_iSendSocket = INVALID_SOCKET;
SOCKET g_iRecvSocket = INVALID_SOCKET;

#define CONSOLE_PORT g_szServerPort /*!< Стандартный порт для подключения консоли */

bool g_bRunning = false;
bool g_bRunningCmd = false;

typedef CConcurrentQueue<char*> CommandBuffer;
typedef std::mutex Mutex;
CommandBuffer g_vCommandBuffer;
Stack<CommandBuffer> g_cbufStack;

char g_szServerPort[8] = "59705";
char g_szServerAddr[255] = "127.0.0.1";

void cmd_endalias();

SX_LIB_API XDEPRECATED void Core_0RegisterConcmd(const char * name, SXCONCMD cmd, const char * desc)
{
	ConCmd c;
	c.type = CMD_DEFAULT;
	c.cls = NULL;
	c.cmd.cmd = cmd;
	c.szDesc = NULL;
	if(desc)
	{
		c.szDesc = new char[strlen(desc) + 1];
		strcpy(c.szDesc, desc);
	}
	g_mCmds[name] = c;
}
SX_LIB_API XDEPRECATED void Core_0RegisterConcmdArg(const char * name, SXCONCMDARG cmd, const char * desc)
{
	ConCmd c;
	c.type = CMD_ARG;
	c.cls = NULL;
	c.cmd.cmd = cmd;
	c.szDesc = NULL;
	if(desc)
	{
		c.szDesc = new char[strlen(desc) + 1];
		strcpy(c.szDesc, desc);
	}
	g_mCmds[name] = c;
}
SX_LIB_API XDEPRECATED void Core_0RegisterConcmdCls(const char * name, void * pObject, const SXCONCMDCLS &cmd, const char * desc)
{
	ConCmd c;
	c.type = CMD_CLS;
	c.cls = (ConCmdStub*)pObject;
	c.cmd.clscmd = cmd;
	c.szDesc = NULL;
	if(desc)
	{
		c.szDesc = new char[strlen(desc) + 1];
		strcpy(c.szDesc, desc);
	}
	g_mCmds[name] = c;
}
SX_LIB_API XDEPRECATED void Core_0RegisterConcmdClsArg(const char * name, void * pObject, const SXCONCMDCLSARG &cmd, const char * desc)
{
	ConCmd c;
	c.type = CMD_CLS_ARG;
	c.cls = (ConCmdStub*)pObject;
	c.cmd.clscmdarg = cmd;
	c.szDesc = NULL;
	if(desc)
	{
		c.szDesc = new char[strlen(desc) + 1];
		strcpy(c.szDesc, desc);
	}
	g_mCmds[name] = c;
}

Mutex g_conUpdMtx;

class CAlias;
void Core_0RegisterConcmdAlias(const char *name, CAlias *pAlias, const char * desc);
void ConsoleExecInternal(char * cmd);
class CAlias final
{
public:
	CAlias(const char *szName, const char *szDesc):
		m_sName(szName),
		m_sDesc(szDesc ? szDesc : "")
	{
	}

	void execute()
	{
		for(UINT i = 0, l = m_aCommands.size(); i < l; ++i)
		{
			char *szStr = strdupa(m_aCommands[i].c_str());
			ConsoleExecInternal(szStr);
		}
	}
	void addCommand(const char *str)
	{
		m_aCommands.push_back(str);
	}

	void reg()
	{
		Core_0RegisterConcmdAlias(m_sName.c_str(), this, m_sDesc.c_str());
	}

	const char* getName()
	{
		return(m_sName.c_str());
	}
private:
	String m_sName;
	String m_sDesc;
	Array<String> m_aCommands;
};
CAlias *g_pNewAlias = NULL;

void ConsoleExecInternal(char * cmd, char * args)
{
	const AssotiativeArray<String, ConCmd>::Node * pNode;
	bool bCVarHandle = false;
	if(!g_mCmds.KeyExists(cmd, &pNode))
	{
		bCVarHandle = true;
		if(!g_mCmds.KeyExists("cvar_handle", &pNode))
		{
			return;
		}
	}
	ConCmd * _cmd = pNode->Val;
	char * arglist[64];
	int argc = 0;


	if(bCVarHandle)
	{
		arglist[argc++] = "cvar_handle";
		arglist[argc++] = cmd;
	}
	else
	{
		arglist[argc++] = cmd;
	}

	if(_cmd)
	{
		switch(_cmd->type)
		{
		case CMD_DEFAULT:
			((SXCONCMD)_cmd->cmd.cmd)();
			return;
		case CMD_CLS:
			(_cmd->cls->*(_cmd->cmd.clscmd))();
			return;
		}

		char * arg = args;

		bool IsInQuote = false;
		bool IsReading = false;
		for(; *args; ++args)
		{
			if(isspace((unsigned char)*args))
			{
				if(IsReading)
				{
					if(IsInQuote)
					{
						*arg++ = *args;
					}
					else
					{
						IsReading = false;
						*arg++ = 0;
					}
				}
			}
			else if(*args == '"')
			{
				if(!IsInQuote)
				{
					if(!IsReading)
					{
						arglist[argc++] = arg;
					}
					IsInQuote = true;
					IsReading = true;
				}
				else
				{
					IsInQuote = false;
					IsReading = false;
					*arg++ = 0;
				}
			}
			else if(*args == L'\\' && *(args + 1))
			{
				if(!IsReading)
				{
					arglist[argc++] = arg;
				}
				++args;
				switch(*args)
				{
				case L'\\':
					*arg++ = L'\\';
					break;
				case L'r':
					*arg++ = L'\r';
					break;
				case L'n':
					*arg++ = L'\n';
					break;
				case L'"':
					*arg++ = L'"';
					break;
				case L't':
					*arg++ = L'\t';
					break;
				case L' ':
					*arg++ = L' ';
					break;
				default:
					*arg++ = *args;
				}
				IsReading = true;
			}
			else
			{
				if(!IsReading)
				{
					arglist[argc++] = arg;
				}
				*arg++ = *args;
				IsReading = true;
			}
		}
		*arg++ = 0;
		switch(_cmd->type)
		{
		case CMD_ARG:
			((SXCONCMDARG)_cmd->cmd.cmd)(argc, (const char**)arglist);
			return;
		case CMD_CLS_ARG:
			(_cmd->cls->*(_cmd->cmd.clscmdarg))(argc, (const char**)arglist);
			return;
		}
	}
}
void ConsoleExecInternal(char * cmd)
{
	//printf("> %s\n", cmd);
	if(g_pNewAlias && fstrcmp(cmd, "endalias") && fstrcmp(cmd, "endalias\r"))
	{
		g_pNewAlias->addCommand(cmd);
		return;
	}

	char * space = cmd;
	while((*++space) && !isspace((unsigned char)*space));
	if(*space)
	{
		*space = 0;
		++space;
	}
	else
	{
		space = cmd;
		while(*++space);
	}
	ConsoleExecInternal(cmd, space);
}

void ConsolePushBuffer()
{
	g_conUpdMtx.lock();
	g_cbufStack.push(g_vCommandBuffer);
	g_vCommandBuffer.clear();
	g_conUpdMtx.unlock();
}

SX_LIB_API void Core_0ConsoleUpdate()
{
	XPROFILE_FUNCTION();

	char * buf;
	while(g_vCommandBuffer.tryPop(buf))
	{
		ConsoleExecInternal(buf);
		mem_delete_a(buf);
	}
	if(!g_cbufStack.IsEmpty())
	{
		if(g_pNewAlias)
		{
			LogWarning("Forcebly closing alias '%s' as end-of-input received\n", g_pNewAlias->getName());
			cmd_endalias();
		}
		g_cbufStack.pop(&g_vCommandBuffer);
	}

	//execute command buffer
}
SX_LIB_API XDEPRECATED void Core_0ConsoleExecCmd(const char * format, ...)
{
	va_list va;
	va_start(va, format);
	size_t len = _vscprintf(format, va) + 1;
	char * buf, *cbuf = NULL;
	if(len < 4096)
	{
		buf = (char*)alloca(len * sizeof(char));
	}
	else
	{
		cbuf = buf = new char[len];
	}
	vsprintf(buf, format, va);
	va_end(va);

	//g_vCommandBuffer
	
	char * nl;
	do
	{
		nl = strstr(buf, "\n");
		if(nl)
		{
			*nl = 0;
			++nl;

			while(isspace((unsigned char)*buf))
			{
				++buf;
			}

			if(!(*buf == '/' && *(buf + 1) == '/') && (len = strlen(buf)))
			{
				char * str = new char[len + 1];
				memcpy(str, buf, len + 1);
				g_conUpdMtx.lock();
				g_vCommandBuffer.push(str);
				g_conUpdMtx.unlock();
			}
			buf = nl;
		}
	}
	while(nl);

	while(isspace((unsigned char)*buf))
	{
		++buf;
	}

	if(!(*buf == '/' && *(buf + 1) == '/') && (len = strlen(buf)))
	{
		char * str = new char[len + 1];
		memcpy(str, buf, len + 1);
		g_conUpdMtx.lock();
		g_vCommandBuffer.push(str);
		g_conUpdMtx.unlock();
	}

	mem_delete_a(cbuf);
}


void Core_0RegisterConcmdAlias(const char *name, CAlias *pAlias, const char * desc)
{
	String sName(name);

	ConCmd c;
	c.type = CMD_CLS;
	c.cls = (ConCmdStub*)pAlias;
	c.cmd.clscmd = (SXCONCMDCLS)&CAlias::execute;
	c.szDesc = NULL;
	c.isAlias = true;
	if(desc)
	{
		c.szDesc = new char[strlen(desc) + 1];
		strcpy(c.szDesc, desc);
	}

	const AssotiativeArray<String, ConCmd>::Node *pNode;
	if(g_mCmds.KeyExists(sName, &pNode))
	{
		if(pNode->Val->isAlias)
		{
			CAlias *ptr = (CAlias*)pNode->Val->cls;
			mem_delete(ptr);
		}
		if(pNode->Val->szDesc)
		{
			mem_delete_a(pNode->Val->szDesc);
		}
	}

	g_mCmds[sName] = c;
}


void echo(int argc, const char ** argv)
{
	for(int i = 1; i < argc; ++i)
	{
		printf("%s ", argv[i]);
	}
	printf("\n");
}

void exec(int argc, const char **argv)
{
	if(argc != 2)
	{
		puts(COLOR_LRED "[exec]: Expected filename" COLOR_RESET);
		return;
	}
	
	IFile *f = Core_GetIXCore()->getFileSystem()->openFile(argv[1]);
	if(!f)
	{
		printf(COLOR_LRED "Couldn't exec '%s'\n" COLOR_RESET, argv[1]);
		return;
	}
	size_t len = f->getSize() + 1;
	char *buf, *cbuf = NULL;
	if(len <= 4096)
	{
		buf = (char*)alloca(sizeof(char) * len);
	}
	else
	{
		buf = cbuf = new char[len];
	}

	f->readBin(buf, len - 1);
	buf[len - 1] = 0;

	ConsolePushBuffer();
	Core_0ConsoleExecCmd("%s", buf);
	mem_release(f); 

	mem_delete_a(cbuf);
}

void cmd_exit()
{
	PostQuitMessage(0);
}

void cvar_handle(int argc, const char ** argv)
{
	if(argc == 2)
	{
		PrintCVar(argv[1]);
	}
	else if(argc > 2)
	{
		SetCVar(argv[1], argv[2]);
	}
}

void cmd_help(int argc, const char ** argv)
{
	if(argc == 2)
	{
		const AssotiativeArray<String, ConCmd>::Node * pNode;
		if(!g_mCmds.KeyExists(argv[1], &pNode))
		{
			printf(COLOR_LRED "Unknown command '%s'\n" COLOR_RESET, argv[1]);
			return;
		}
		if(pNode->Val->szDesc)
		{
			printf(COLOR_GREEN "%s\n" COLOR_RESET, pNode->Val->szDesc);
		}
		else
		{
			printf(COLOR_RED "No description for '%s'\n" COLOR_RESET, argv[1]);
		}
	}
	else
	{
		printf(COLOR_GREEN "Usage:\n    " COLOR_LGREEN "help <command>" COLOR_GREEN " - Show command description\n" COLOR_RESET);
	}
}

void DumpCVars();
void cmd_cvarlist(int argc, const char ** argv)
{
	DumpCVars();
}

void DumpCCommands()
{
	int iLenName = 0;

	for(AssotiativeArray<String, ConCmd>::Iterator i = g_mCmds.begin(); i; ++i)
	{
		ConCmd * pCmd = i.second;
		int len = (int)i.first->length();
		if(iLenName < len)
		{
			iLenName = len;
		}
	}
	char szRow[128];
	sprintf(szRow, COLOR_LGREEN " %%%ds " COLOR_GRAY "| " COLOR_GREEN " %%s" COLOR_RESET "\n", iLenName);
	printf(szRow, "Name", "Description");
	char tmpName[64];
	for(AssotiativeArray<String, ConCmd>::Iterator i = g_mCmds.begin(); i; ++i)
	{
		ConCmd * pCmd = i.second;
		sprintf(tmpName, "%s", i.first->c_str());
		int j;
		for(j = i.first->length(); j < iLenName; ++j)
		{
			tmpName[j] = ' ';
		}
		tmpName[j] = 0;
		printf(szRow, tmpName, pCmd->szDesc ? pCmd->szDesc : "");
	}
}

void cmd_cmdlist(int argc, const char ** argv)
{
	DumpCCommands();
}

void cmd_alias(int argc, const char **argv)
{
	if(argc == 2 || argc == 3)
	{
		if(g_pNewAlias)
		{
			printf(COLOR_RED "Cannot create nested alias! Removing previous!\n" COLOR_RESET);
			mem_delete(g_pNewAlias);
		}
		g_pNewAlias = new CAlias(argv[1], argc == 3 ? argv[2] : NULL);
	}
	else
	{
		printf(COLOR_GREEN "Usage:\n    " COLOR_LGREEN "alias <name> [<description>]" COLOR_GREEN " - Create new alias\n" COLOR_RESET);
	}
}
void cmd_endalias()
{
	if(!g_pNewAlias)
	{
		printf(COLOR_RED "No alias was started!\n" COLOR_RESET);
		return;
	}
	g_pNewAlias->reg();
	g_pNewAlias = NULL;
}

void ConsoleRegisterCmds()
{
	Core_0RegisterConcmdArg("echo", echo, "Echoes all parameters to console");
	Core_0RegisterConcmdArg("exec", exec, "Executes given cfg file");
	Core_0RegisterConcmd("exit", cmd_exit, "Shuts down the engine");
	Core_0RegisterConcmd("quit", cmd_exit, "Shuts down the engine");
	Core_0RegisterConcmdArg("cvar_handle", cvar_handle, "Handle to show or set cvar value");
	Core_0RegisterConcmdArg("help", cmd_help, "Handle to show command description");
	Core_0RegisterConcmdArg("cvarlist", cmd_cvarlist, "List all CVars");
	Core_0RegisterConcmdArg("cmdlist", cmd_cmdlist, "List all Commands");
	Core_0RegisterConcmdArg("alias", cmd_alias, "Begin alias creation");
	Core_0RegisterConcmd("endalias", cmd_endalias, "End alias creation");

	Core_0RegisterCVarInt("con_width", 80, "Ширина окна консоли");
	Core_0RegisterCVarInt("con_height", 25, "Высота окна консоли");
}

SX_LIB_API UINT_PTR Core_ConsoleGetOutHandler()
{
	return(g_iSendSocket);
}

SOCKET Connect(const char *szAddr, const char *szPort)
{
	int iResult;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	iResult = getaddrinfo(szAddr, szPort, &hints, &result);
	if(iResult != 0)
	{
		printf("getaddrinfo failed with error: %d\n", iResult);
		return(INVALID_SOCKET);
	}

	SOCKET iSocket = INVALID_SOCKET;

	// Attempt to connect to an address until one succeeds
	for(ptr = result; ptr != NULL; ptr = ptr->ai_next)
	{

		// Create a SOCKET for connecting to server
		iSocket = WSASocketW(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol, NULL, 0, 0);
		if(iSocket == INVALID_SOCKET)
		{
			printf("socket failed with error: %ld\n", WSAGetLastError());
			return(iSocket);
		}

		// Connect to server.
		iResult = connect(iSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if(iResult == SOCKET_ERROR)
		{
			closesocket(iSocket);
			iSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	return(iSocket);
}

void ConsoleRecv(void*)
{
	int iResult;
	g_iRecvSocket = Connect(g_szServerAddr, CONSOLE_PORT);

	if(g_iRecvSocket == INVALID_SOCKET)
	{
		printf("Unable to connect to console command channel!\n");
		return;
	}

	char recvbuf[2048];
	int recvbuflen = sizeof(recvbuf);

	while(g_bRunning)
	{
		iResult = recv(g_iRecvSocket, recvbuf, recvbuflen - 1, 0);
		if(iResult > 0)
		{
			recvbuf[iResult] = 0;
			Core_0ConsoleExecCmd("%s", recvbuf);
		}
		else if(iResult == 0)
		{
			break;
		}
	}

	iResult = shutdown(g_iRecvSocket, SD_SEND);
	if(iResult == SOCKET_ERROR)
	{
		printf("shutdown failed with error: %d\n", WSAGetLastError());
	}
	closesocket(g_iRecvSocket);
	g_iRecvSocket = INVALID_SOCKET;
}

bool ConsoleConnect(const char *szName, bool bNewInstance)
{
	{
		const char *szConsole = Core_0GetCommandLineArg("console", "127.0.0.1:59705");

		char *str = strdupa(szConsole);
		char *parts[2];
		if(parse_str(str, parts, 2, ':') == 2)
		{
			strcpy(CONSOLE_PORT, parts[1]);
		}
		strcpy(g_szServerAddr, parts[0]);
		if(!strcmp(parts[0], "localhost"))
		{
			strcpy(g_szServerAddr, "127.0.0.1");
		}
	}

	char szConsoleArgs[64];
	szConsoleArgs[0] = 0;

	bool bRunNew = !Core_0IsProcessRun("sxconsole.exe");
	if(bNewInstance)
	{
		bRunNew = true;
		srand((UINT)time(NULL));
		int port = (rand() % (65536 - 2048) | 1) + 2048;
		sprintf(g_szServerPort, "%d", port);

		sprintf(szConsoleArgs, "-port %s -exit-on-disconnect 1", g_szServerPort);
	}
	else if(!strcmp(g_szServerAddr, "127.0.0.1"))
	{
		if(strcmp(g_szServerPort, "59705"))
		{
			sprintf(szConsoleArgs, "-port %s -exit-on-disconnect 1", g_szServerPort);
		}
	}
	else
	{
		bRunNew = false;
	}

	if(bRunNew)
	{
		char str[MAX_PATH];
		GetModuleFileNameA(NULL, str, MAX_PATH);
		ShellExecuteA(0, "open", "sxconsole.exe", szConsoleArgs, dirname(str), SW_SHOWNORMAL);
	}

	WSADATA wsaData;

	char *sendbuf = "Console initialized...\n";
	//char recvbuf[2048];
	int iResult;
	//int recvbuflen = sizeof(recvbuf);

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if(iResult != 0)
	{
		printf("WSAStartup failed with error: %d\n", iResult);
		return(false);
	}

	g_iSendSocket = Connect(g_szServerAddr, CONSOLE_PORT);
	if(g_iSendSocket == INVALID_SOCKET)
	{
		Sleep(1000);
		g_iSendSocket = Connect(g_szServerAddr, CONSOLE_PORT);
	}

	if(g_iSendSocket == INVALID_SOCKET)
	{
		printf("Unable to connect to console!\n");
		WSACleanup();
		const char *szConsoleAddr = Core_0GetCommandLineArg("console", NULL);
		if(szConsoleAddr)
		{
			AllocConsole();
			freopen("CONOUT$", "wt", stdout);
			freopen("CONOUT$", "wt", stderr);
			printf("Unable to connect to %s!\n", szConsoleAddr);
			return(false);
		}
		return(ConsoleConnect(szName, true));
		//return(false);
	}

	// Send an initial buffer
	iResult = send(g_iSendSocket, sendbuf, (int)strlen(sendbuf), 0);
	if(iResult == SOCKET_ERROR)
	{
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(g_iSendSocket);
		g_iSendSocket = INVALID_SOCKET;
		goto end;
	}

	//connected? reopen streams

	// unsigned long block = 1;//+
	// ioctlsocket(g_iSendSocket, FIONBIO, &block);//+
	
	FreeConsole();

	Core_SetOutPtr();

	g_bRunning = true;

	_beginthread(ConsoleRecv, 0, 0);

	return(true);

end:
	WSACleanup();
	return(false);
}
void ConsoleDisconnect()
{

	g_bRunning = false; 

	int iResult = shutdown(g_iSendSocket, SD_SEND);
	if(iResult == SOCKET_ERROR)
	{
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		goto end;
	}
end:
	closesocket(g_iSendSocket);
	WSACleanup();
	//Sleep(1000);
	//CommandDisconnect();
}
