/*************************************************************************
	> File Name: IPCMonitorServer.cpp
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Mon 18 Jul 2016 10:54:48 PM CST
 ************************************************************************/

#include<iostream>
using namespace std;
#include"IPCManger.h"
#include"UnitMemory_Profile.h"


 string  myitoa(long pid)
{
	char str[20]={'\0'};
	char *cur=str;
    long  tmp =pid;
	long  i=pid;
	int power=1;
	for(;i>=10;i/=10)
	{
		power*=10;
	}

     for(;power>0;power/=10)
	{
	    *cur++ = tmp/power + '0';
		tmp%=power;
	 }
	 return string(str);
}


const char* SERVER_PIPE_NAME = "fifo_";

static string GetServerPipeName()
{
	string name = SERVER_PIPE_NAME;
	name += myitoa(getpid());
	return name;
}
// 启动IPC消息处理服务线程
void IPCMonitorServer::Start()
{}

IPCMonitorServer::~IPCMonitorServer()
{
  pthread_join(_onMsgThread,NULL);
}


static void* OnMessage(void *arg)
{
    IPCMonitorServer*p=(IPCMonitorServer*)arg;
     const int IPC_BUF_LEN = 1024;
	 char msg[IPC_BUF_LEN] = { 0 };
	 IPCServer server(GetServerPipeName().c_str());
	 
	while (1)
	{

		server.ReceiverMsg(msg, IPC_BUF_LEN);
		printf("Receiver Cmd Msg: %s\n", msg);

		string reply;
		string cmd = msg;
		CmdFuncMap::iterator it = p->_cmdFuncsMap.find(cmd);
		if (it != p->_cmdFuncsMap.end())
		{
			CmdFunc func = it->second;
			func(reply);
		}
		else
		{
			reply = "Invalid Command";
		}

		server.SendReplyMsg(reply.c_str(), reply.size());
	}
}
IPCMonitorServer::IPCMonitorServer()
{
	pthread_create(&_onMsgThread,NULL,OnMessage,this);
	_cmdFuncsMap["state"] = GetState;
	_cmdFuncsMap["save"] = Save;
	_cmdFuncsMap["disable"] = Disable;
	_cmdFuncsMap["enable"] = Enable;
}



void IPCMonitorServer::GetState(string& reply)
{
	reply += "State:";
	int flag = ConfigManager::GetInstance()->GetOptions();

	if (flag == PPCO_NONE)
	{
		reply += "None\n";
		return;
	}

	if (flag & PPCO_PROFILER)
	{
		reply += "Performance Profiler \n";
	}

	if (flag & PPCO_SAVE_TO_CONSOLE)
	{
		reply += "Save To Console\n";
	}

	if (flag & PPCO_SAVE_TO_FILE)
	{
		reply += "Save To File\n";
	}
}

void IPCMonitorServer::Enable(string& reply)
{
	ConfigManager::GetInstance()->SetOptions(PPCO_PROFILER | PPCO_SAVE_TO_FILE);

	reply += "Enable Success";
}

void IPCMonitorServer::Disable(string& reply)
{
	ConfigManager::GetInstance()->SetOptions(PPCO_NONE);

	reply += "Disable Success";
}

void IPCMonitorServer::Save(string& reply)
{
	ConfigManager::GetInstance()->SetOptions(
		ConfigManager::GetInstance()->GetOptions() | PPCO_SAVE_TO_FILE);
	PerformanceProfiler::GetInstance()->OutPut();

	reply += "Save Success";
}
