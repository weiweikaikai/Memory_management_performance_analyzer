/*************************************************************************
    > File Name: UnitMemory_Profile.h
    > Author: WK
    > Mail:18402927708@163.com 
    > Created Time: Mon 02 May 2016 05:52:13 PM CST
 ************************************************************************/


#include"Lock.h"
#include"SaveAdapter.h"
#include<map>

typedef long long LongType;


static int GetThreadId()
{
   return ::pthread_self();
}

// 单例基类
template<class T>
class Singleton
{
public:
	static T* GetInstance()
	{
		//
		// 1.双重检查保障线程安全和效率。
		//
		if (_sInstance == NULL)
		{
			Lock lock(_mutex);
			if (_sInstance == NULL)
			{
				_sInstance = new T();
			}
		}

		return _sInstance;
	}
protected:
	Singleton()
	{}

	static T* _sInstance;	// 单实例对象
	static pthread_mutex_t _mutex;	// 互斥锁对象
};
template<class T>
T* Singleton<T>::_sInstance = NULL;

template<class T>
pthread_mutex_t Singleton<T>::_mutex;


enum PP_CONFIG_OPTION
{
	PPCO_NONE = 0,					// 不做剖析
	PPCO_PROFILER = 2,				// 开启剖析
	PPCO_SAVE_TO_CONSOLE = 4,		// 保存到控制台
	PPCO_SAVE_TO_FILE = 8,			// 保存到文件
	PPCO_SAVE_BY_CALL_COUNT = 16,	// 按调用次数降序保存
	PPCO_SAVE_BY_COST_TIME = 32,	// 按调用花费时间降序保存
};

// 配置管理
class ConfigManager : public Singleton<ConfigManager>
{
public:
	void SetOptions(int flag)
	{
		_flag = flag;
	}
	int GetOptions()
	{
		return _flag;
	}

	ConfigManager()
		:_flag(PPCO_NONE)
	{}
private:
	int _flag;
};

// 资源统计信息
struct ResourceInfo
{
	LongType  _peak;	  // 最大峰值
	LongType  _small;	  // 最小值

	ResourceInfo()
		: _peak(0),
		_small(0)
	{}

	void Update(double  value);
	void Serialize(SaveAdapter& SA) const;
};

// 资源统计
class ResourceStatistics
{
public:
	ResourceStatistics();
	~ResourceStatistics();

	// 开始统计
	void StartStatistics();

	// 停止统计
	void StopStatistics();

	// 获取CPU/内存信息 
	const ResourceInfo& GetCpuInfo();
	const ResourceInfo& GetMemoryInfo();

    // Linux下实现资源统计
    void _UpdateStatistics();

public:
	int _pid;					        // 进程ID

	ResourceInfo _cpuInfo;				// CPU信息
	ResourceInfo _memoryInfo;			// 内存信息
	//ResourceInfo _IOInfo;			    // IO信息

	volatile int _refCount;				// 引用计数
	pthread_mutex_t _lockMutex;		    // 线程互斥锁
	pthread_cond_t _condVariable;	    // 控制是否进行统计的条件变量
	pthread_t _statisticsThread;		// 统计线程
};

// 性能剖析节点
struct  PerformanceNode
{
	string _fileName;	// 文件名
	string _function;	// 函数名
	int	   _line;		// 行号
	string _desc;		// 附加描述

	PerformanceNode(const char* fileName, const char* function,
		int line, const char* desc);

	//
	// 做map键值，所以需要重载operator<
	// 做unorder_map键值，所以需要重载operator==
	//
	bool operator<(const PerformanceNode& p) const;
	bool operator==(const PerformanceNode& p) const;

	// 序列化节点信息到容器
	void Serialize(SaveAdapter& SA) const;
};


// 性能剖析段
class  PerformanceProfilerSection
{
	typedef map<int,LongType> StatisMap;

	friend class PerformanceProfiler;
public:
	PerformanceProfilerSection()
		:_totalRef(0)
		, _rsStatistics(0)
		, _totalCallCount(0)
		, _totalCostTime(0)
	{}

	void Begin(int threadId);
	void End(int threadId);

	void Serialize(SaveAdapter& SA);
private:
	pthread_mutex_t _mutex;			// 互斥锁
	StatisMap _beginTimeMap;		// 线程id->开始时间统计
	StatisMap _costTimeMap;			// 线程id->花费时间统计
	LongType  _totalCostTime;		// 总花费时间

	StatisMap _refCountMap;			// 线程id->引用计数(解决剖析段首尾不匹配，递归函数内部段剖析)
	LongType _totalRef;				// 总的引用计数

	StatisMap _callCountMap;		// 线程id->调用次数统计
	LongType  _totalCallCount;		// 总的调用次数

	ResourceStatistics* _rsStatistics;	// 资源统计线程对象
};

class  PerformanceProfiler:public Singleton<PerformanceProfiler>
{
	friend class Singleton<PerformanceProfiler>;
public:
       typedef map<PerformanceNode, PerformanceProfilerSection*> PerformanceProfilerMap;
	// 创建剖析段
	PerformanceProfilerSection* CreateSection(const char* fileName,
		const char* funcName, int line, const char* desc, bool isStatistics);

	static void OutPut();
protected:
	static bool CompareByCallCount(PerformanceProfilerMap::iterator lhs,
		PerformanceProfilerMap::iterator rhs);
	static bool CompareByCostTime(PerformanceProfilerMap::iterator lhs,
		PerformanceProfilerMap::iterator rhs);
	PerformanceProfiler();
	// 输出序列化信息
	void _OutPut(SaveAdapter& SA);
private:
	time_t  _beginTime;
	pthread_mutex_t _mutex;
	PerformanceProfilerMap _ppMap;
};
// 添加性能剖析段开始
#define ADD_PERFORMANCE_PROFILE_SECTION_BEGIN(sign, desc, isStatistics) \
	PerformanceProfilerSection* PPS_##sign = NULL;						\
	if (ConfigManager::GetInstance()->GetOptions()&PPCO_PROFILER)		\
	{																	\
		PPS_##sign = PerformanceProfiler::GetInstance()->CreateSection(__FILE__, __FUNCTION__, __LINE__, desc, isStatistics);\
		PPS_##sign->Begin(GetThreadId());								\
	}

// 添加性能剖析段结束
#define ADD_PERFORMANCE_PROFILE_SECTION_END(sign)	\
	do{												\
		if(PPS_##sign)								\
			PPS_##sign->End(GetThreadId());			\
	}while(0);

//
// 剖析【效率】开始
// @sign是剖析段唯一标识，构造出唯一的剖析段变量
// @desc是剖析段描述
//
#define PERFORMANCE_PROFILER_BEGIN(sign, desc)	\
	ADD_PERFORMANCE_PROFILE_SECTION_BEGIN(sign, desc, false)

//
// 剖析【效率】结束
// @sign是剖析段唯一标识
//
#define PERFORMANCE_PROFILER_END(sign)	\
	ADD_PERFORMANCE_PROFILE_SECTION_END(sign)

//
// 剖析【效率&资源】开始。
// ps：非必须情况，尽量少用资源统计段，因为每个资源统计段都会开一个线程进行统计
// @sign是剖析段唯一标识，构造出唯一的剖析段变量
// @desc是剖析段描述
//
#define PERFORMANCE_PROFILER_AND_RS_BEGIN(sign, desc)	\
	ADD_PERFORMANCE_PROFILE_SECTION_BEGIN(sign, desc, true)

//
// 剖析【效率&资源】结束
// ps：非必须情况，尽量少用资源统计段，因为每个资源统计段都会开一个线程进行统计
// @sign是剖析段唯一标识
//
#define PERFORMANCE_PROFILER_AND_RS_END(sign)		\
	ADD_PERFORMANCE_PROFILE_SECTION_END(sign)

//
// 设置剖析选项
//
#define SET_OPTIONS(flag)		\
	ConfigManager::GetInstance()->SetOptions(flag)