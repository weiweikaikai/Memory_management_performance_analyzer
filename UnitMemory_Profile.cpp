/*************************************************************************
    > File Name: UnitMemory_Profile.cpp
    > Author: WK
    > Mail:18402927708@163.com 
    > Created Time: Mon 02 May 2016 05:52:36 PM CST
 ************************************************************************/

#include"UnitMemory_Profile.h"

//获取路径中最后的文件名。
static string GetFileName(const string& path)
{
	char ch = '/';
	size_t pos = path.rfind(ch);
	if (pos == string::npos)
	{
		return path;
	}
	else
	{
		return path.substr(pos + 1);
	}
}


void ResourceInfo::Update(double value)
{
	// value < 0时则直接返回不再更新。
	if (value < 0)
		return;

	if (value > _peak)
		_peak = value;
    if(value < _small)
		_small = value;
	// 计算不准确，平均值受变化影响不明显
	// 需优化，可参考网络滑动窗口反馈调节计算。
	//_total += value;
	//_avg = _total / (++_count);
}


static void* Statistics(void *arg)
{
    ResourceStatistics*p=(ResourceStatistics*)arg;
	while (1)
	{
		//
		// 未开始统计时，则使用条件变量阻塞。
		if (p->_refCount == 0)
		{
			Lock lock(p->_lockMutex);
			pthread_cond_wait(&p->_condVariable,&p->_lockMutex);//解锁并等待cond改变
		}

		// 更新统计信息
		p->_UpdateStatistics();
		// 1ms更新一次
		usleep(1000);
	}
	return NULL;
}


ResourceStatistics::ResourceStatistics()
	:_refCount(0)
{
	// 初始化统计资源信息
	pthread_create(&_statisticsThread,NULL,Statistics,this);
	pthread_cond_init(&_condVariable,NULL);/*初始化条件变量*/  
	_pid = getpid();
}

ResourceStatistics::~ResourceStatistics()
{
      pthread_cond_destroy(&_condVariable);
}

void ResourceStatistics::StartStatistics()
{
	// 多个线程并行剖析一个代码段的场景下使用引用计数进行统计。
	// 第一个线程进入剖析段时开始统计，最后一个线程出剖析段时
	// 停止统计。
	if (_refCount++ == 0)
	{
		Lock lock(_lockMutex);
		pthread_cond_signal(&_condVariable);
	}
}
void ResourceStatistics::StopStatistics()
{
	if (_refCount > 0)
	{
		--_refCount;
	}
}
const ResourceInfo& ResourceStatistics::GetCpuInfo()
{
	return _cpuInfo;
}

const ResourceInfo& ResourceStatistics::GetMemoryInfo()
{
	return _memoryInfo;
}

//const ResourceInfo& ResourceStatistics::GetIOInfo()
//{
//	return _IOInfo;
//}


void ResourceStatistics::_UpdateStatistics()
{
	char buf[1024] = { 0 };
	char cmd[256] = { 0 };
	sprintf(cmd, "ps -o pcpu,pmem -p %d | sed 1,1d", _pid);

	// 将 "ps" 命令的输出 通过管道读取 ("pid" 参数) 到 FILE* stream
	// 将刚刚 stream 的数据流读取到buf中
	// http://www.cnblogs.com/caosiyang/archive/2012/06/25/2560976.html
	FILE *stream = ::popen(cmd, "r");
	::fread(buf, sizeof (char), 1024, stream);
	::pclose(stream);

	double cpu = 0.0;
	double mem = 0.0;
	sscanf(buf, "%lf %lf", &cpu, &mem);

	_cpuInfo.Update(cpu);
	_memoryInfo.Update(mem);
}


PerformanceNode::PerformanceNode(const char* fileName, const char* function,
	int line, const char* desc)
	:_fileName(GetFileName(fileName))
	,_function(function)
	,_line(line)
	,_desc(desc)
{}
bool PerformanceNode::operator<(const PerformanceNode& p) const
{
	if (_line > p._line)
		return false;

	if (_line < p._line)
		return true;

	if (_fileName > p._fileName)
		return false;
	
	if (_fileName < p._fileName)
		return true;

	if (_function > p._function)
		return false;

	if (_function < p._function)
		return true;

	return false;
}

bool PerformanceNode::operator == (const PerformanceNode& p) const
{
	return _fileName == p._fileName
		&&_function == p._function
		&&_line == p._line;
}
void PerformanceNode::Serialize(SaveAdapter& SA) const
{
	SA.Save("FileName:%s, Fuction:%s, Line:%d\n",
		_fileName.c_str(), _function.c_str(), _line);
}

//PerformanceProfilerSection
void PerformanceProfilerSection::Serialize(SaveAdapter& SA)
{
	// 若总的引用计数不等于0，则表示剖析段不匹配
	if (_totalRef)
		SA.Save("Performance Profiler Not Match!\n");

	// 序列化效率统计信息
	StatisMap::iterator costTimeIt = _costTimeMap.begin();
	for (; costTimeIt != _costTimeMap.end(); ++costTimeIt)
	{
		LongType callCount = _callCountMap[costTimeIt->first];
		SA.Save("Thread Id:%d, Cost Time:%d us, Call Count:%d\n",
			costTimeIt->first, 
			costTimeIt->second ,
			callCount);
	}

	SA.Save("Total Cost Time:%d us, Total Call Count:%d\n",
		_totalCostTime ,_totalCallCount);

	// 序列化资源统计信息
	if (_rsStatistics)
	{
		ResourceInfo cpuInfo = _rsStatistics->GetCpuInfo();
		SA.Save("[Cpu] Peak:%lf%%, Small:%lf%%\n", cpuInfo._peak, cpuInfo._small);

		ResourceInfo memoryInfo = _rsStatistics->GetMemoryInfo();
		SA.Save("[Memory] Peak:%lf%%, Small:%lf%%\n", memoryInfo._peak, memoryInfo._small);
	}
}

// PerformanceProfiler
PerformanceProfilerSection* PerformanceProfiler::CreateSection(const char* fileName,
	const char* function, int line, const char* extraDesc, bool isStatistics)
{
	PerformanceProfilerSection* section = NULL;
	PerformanceNode node(fileName, function, line, extraDesc);

	Lock lock(_mutex);
	PerformanceProfilerMap::iterator it = _ppMap.find(node);
	if (it != _ppMap.end())
	{
		section = it->second;
	}
	else
	{
		section = new PerformanceProfilerSection;
		if (isStatistics)//如果要进行资源的剖析
		{
			section->_rsStatistics = new ResourceStatistics();
		}
		_ppMap[node] = section;
	}

	return section;
}

static long getCurrentTime()
{
   struct timeval tv;
   gettimeofday(&tv,NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec; 
  }
void PerformanceProfilerSection::Begin(int threadId)
{
	Lock lock(_mutex);

	// 更新调用次数统计
	++_callCountMap[threadId];

	// 引用计数 == 0 时更新段开始时间统计，解决剖析递归程序的问题。
	LongType  refCount = _refCountMap[threadId];
	if (refCount == 0)
	{
		_beginTimeMap[threadId] = getCurrentTime();

		// 开始资源统计
		if (_rsStatistics)
		{
			_rsStatistics->StartStatistics();
		}
	}

	// 更新剖析段开始结束的引用计数统计
	++_refCountMap[threadId]; 
	++_totalRef;
	++_totalCallCount;
}
void PerformanceProfilerSection::End(int threadId)
{
	Lock lock(_mutex);

	// 更新引用计数
	LongType refCount = --_refCountMap[threadId];
	--_totalRef;

	// 引用计数 <= 0 时更新剖析段花费时间。
	// 解决剖析递归程序的问题和剖析段不匹配的问题
	if (refCount <= 0)
	{
	StatisMap::iterator it= _beginTimeMap.find(threadId);
		if (it != _beginTimeMap.end())
		{
			LongType costTime = getCurrentTime() - it->second;
			if (refCount == 0)
				_costTimeMap[threadId] += costTime;
			else
				_costTimeMap[threadId] = costTime;

			_totalCostTime += costTime;
		}

		// 停止资源统计
		if (_rsStatistics)
		{
			_rsStatistics->StopStatistics();
		}
	}
}

PerformanceProfiler::PerformanceProfiler()
{
	// 程序结束时输出剖析结果
	atexit(OutPut);

	time(&_beginTime);

	//IPCMonitorServer::GetInstance()->Start();
}

void PerformanceProfiler::OutPut()
{
	int flag = ConfigManager::GetInstance()->GetOptions();
	if (flag & PPCO_SAVE_TO_CONSOLE)
	{
		ConsoleSaveAdapter CSA;
		PerformanceProfiler::GetInstance()->_OutPut(CSA);
	}

	if (flag & PPCO_SAVE_TO_FILE)
	{
		FileSaveAdapter FSA("PerformanceProfilerReport.txt");
		PerformanceProfiler::GetInstance()->_OutPut(FSA);
	}
}

bool PerformanceProfiler::CompareByCallCount(PerformanceProfilerMap::iterator lhs,
	PerformanceProfilerMap::iterator rhs)
{
	return lhs->second->_totalCallCount > rhs->second->_totalCallCount;
}
bool PerformanceProfiler::CompareByCostTime(PerformanceProfilerMap::iterator lhs,
	PerformanceProfilerMap::iterator rhs)
{
	return lhs->second->_totalCostTime > rhs->second->_totalCostTime;
}

void PerformanceProfiler::_OutPut(SaveAdapter& SA)
{
	SA.Save("=============Performance Profiler Report==============\n\n");
	SA.Save("Profiler Begin Time: %s\n", ctime(&_beginTime));

	Lock lock(_mutex);

	vector<PerformanceProfilerMap::iterator> vInfos;
	PerformanceProfilerMap::iterator it = _ppMap.begin();
	for (; it != _ppMap.end(); ++it)
	{
		vInfos.push_back(it);
	}

	// 按配置条件对剖析结果进行排序输出
	int flag = ConfigManager::GetInstance()->GetOptions();
	if (flag & PPCO_SAVE_BY_COST_TIME)
		sort(vInfos.begin(), vInfos.end(), CompareByCostTime); //所有剖析的代码段花费的总时间进行排序
	else if (flag & PPCO_SAVE_BY_CALL_COUNT)
		sort(vInfos.begin(), vInfos.end(), CompareByCallCount);//所有剖析的代码段执行的的总次数进行排序

	for (int index = 0; index < vInfos.size(); ++index)
	{
		SA.Save("NO%d. Description:%s\n", index + 1, vInfos[index]->first._desc.c_str());
		vInfos[index]->first.Serialize(SA);
		vInfos[index]->second->Serialize(SA);
		SA.Save("\n");
	}

	SA.Save("==========================end========================\n\n");
}

