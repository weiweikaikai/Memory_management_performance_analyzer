/*************************************************************************
    > File Name: UnitMemory_Profile.h
    > Author: WK
    > Mail:18402927708@163.com 
    > Created Time: Mon 02 May 2016 05:52:13 PM CST
 ************************************************************************/


#include"Lock.h"
#include"SaveAdapter.h"
#include"TypeTraits.h"
#include<typeinfo>
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

	// 配置选项
enum ConfigOptions
{
	CO_NONE = 0,						// 不做剖析
	CO_ANALYSE_MEMORY_LEAK = 1,			// 剖析内存泄露
	CO_ANALYSE_MEMORY_HOT = 2,			// 剖析内存热点
	CO_ANALYSE_MEMORY_ALLOC_INFO = 4,	// 剖析内存块分配情况
	CO_SAVE_TO_CONSOLE = 8,				// 保存到控制台
	CO_SAVE_TO_FILE = 16,				// 保存到文件
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
     void SetOutputPath(const string& path)
     {
	   _outputPath = path;
     }

      const std::string& GetOutputPath()
     {
	    return _outputPath;
      }

	// 默认情况下不开启剖析
	ConfigManager()
		:_flag(PPCO_NONE)
		, _outputPath("MEMORY_ANALYSE_REPORT.txt")//默认的输出路径
	{}
private:
	int _flag;
   std::string _outputPath; // 输出路径
};

// IPC在线控制监听服务
typedef void(*CmdFunc) (string& reply);
typedef map<string, CmdFunc> CmdFuncMap;

class IPCMonitorServer : public Singleton<IPCMonitorServer>
{
	friend class Singleton<IPCMonitorServer>;

public:
	void Start();
	//
	// 以下均为观察者模式中，对应命令消息的的处理函数
	//
	static void GetState(string& reply);
	static void Enable(string& reply);
	static void Disable(string& reply);
	static void Save(string& reply);

	IPCMonitorServer();
	~IPCMonitorServer();

	pthread_t	_onMsgThread;		// 处理消息线程
	CmdFuncMap _cmdFuncsMap;		// 消息命令到执行函数的映射表
};


// 资源统计信息
struct ResourceInfo
{
	double  _peak;	  // 最大峰值
	double  _small;	  // 最小值
    int     _count;
	ResourceInfo()
		: _peak(0.0),
		_small(0.0),
		_count(0)
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
	long _pid;					        // 进程ID

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






// 内存块剖析信息
struct MemoryBlockInfo
{
	string	_type;			// 类型
	int		_num;			// 数据个数
	size_t	_size;			// 数据块大小
	void*	_ptr;			// 指向内存块的指针
	string	_filename;		// 文件名
	int		_fileline;		// 行号

	MemoryBlockInfo(const char* type = "", int num = 0, size_t size = 0,
		void * ptr = 0, const char* filename = "", int fileline = 0);

	// 序列化数据到适配器
     void Serialize(SaveAdapter& SA)
    {
	     SA.Save("Blcok->Ptr:0X%x , Type:%s , Num:%d, Size:%d\r\nFilename:%s , Fileline:%d\r\n",
		_ptr, _type.c_str(), _num, _size, _filename.c_str(), _fileline);
    }
};

// 统计计数信息
struct CountInfo
{
	int _addCount;		// 添加计数
	int _delCount;		// 删除计数

	long long _totalSize;	// 分配总大小

	long long _usedSize;	// 正在使用大小
	long long _maxPeakSize;	// 最大使用大小(最大峰值)

	CountInfo()
		: _addCount(0)
		, _delCount(0)
		, _totalSize(0)
		, _usedSize(0)
		, _maxPeakSize(0)
	{}

	// 增加计数
	void AddCount(size_t size)
	{
		_addCount++;
		_totalSize += size;
		_usedSize += size;
 
		if (_usedSize > _maxPeakSize)
		{
			_maxPeakSize = _usedSize;
		}
	}

	// 删除计数
	void DelCount(size_t size)
	{
		_delCount++;
		_usedSize -= size;
	}

	// 序列化统计计数信息到适配器
	void Serialize(SaveAdapter& SA)
	{
		SA.Save("Alloc Count:%d , Dealloc Count:%d, Alloc Total Size:%lld byte, NonRelease Size:%lld, Max Used Size:%lld\r\n",
			_addCount, _delCount, _totalSize, _usedSize, _maxPeakSize);
	}
};

// 内存剖析对象
class MemoryAnalyse : public Singleton<MemoryAnalyse>
{
	// unordered_map内部使用hash_table实现（时间复杂度为），map内部使用红黑树。
	// unordered_map操作的时间复杂度为O(1)，map为O(lgN)，so! unordered_map更高效。
	//
	typedef map<void*, MemoryBlockInfo> MemoryLeakMap;//内存块泄漏的地址==》泄漏的内存块信息
	typedef map<string, CountInfo> MemoryHostMap;//内存类型==》使用统计信息
	typedef vector<MemoryBlockInfo> MemoryBlcokInfos;//内存块剖析信息数组
public:
	// 更新相关的剖析记录
	void AddRecord(const MemoryBlockInfo& memBlockInfo);
	void DelRecord(const MemoryBlockInfo& memBlockInfo);

	// 根据配置信息相应的剖析结果到对应的保存适配器
	static void OutPut();
	void _OutPut(SaveAdapter& SA);
	void _OutPutLeakBlockInfo(SaveAdapter& SA);
	void _OutPutHostObjectInfo(SaveAdapter& SA);
	void _OutPutAllBlockInfo(SaveAdapter& SA);
private:
	MemoryAnalyse();//私有化构造函数

	friend class Singleton<MemoryAnalyse>;
private:
	MemoryLeakMap	 _leakBlockMap;			// 内存泄露块记录
	MemoryHostMap	 _hostObjectMap;		// 热点对象(以对象类型type为key进行统计)
	MemoryBlcokInfos _memBlcokInfos;		// 内存块分配记录
	CountInfo		 _allocCountInfo;		// 分配计数记录
};

// 内存管理对象
class  MemoryManager : public Singleton<MemoryManager>
{
public:
	// 申请内存/更新剖析信息
	void* Alloc(const char* type, size_t size, int num,
		const char* filename, int fileline);
	// 释放内存/更新剖析信息
	void Dealloc(const char* type, void* ptr, size_t size);

private:
	// 定义为私有
	MemoryManager();

	// 定义为友元，否则无法创建单实例
	friend class Singleton<MemoryManager>;

	allocator<char> _allocPool;		// STL的内存池对象
};


template<typename T>
inline void _DELETE(T* ptr)
{
	if (ptr)
	{
		ptr->~T();
		MemoryManager::GetInstance()->Dealloc(typeid(T).name(), ptr, sizeof(T));
	}
}
template<typename T>
void construct(int num,T*retPtr,__FalseType)
{
  	for (size_t i = 0; i < num; ++i)
		{
			new(&retPtr[i])T();
		}
}
template<typename T>
void construct(int num,int retPtr,__TrueType)
{}


template<typename T>
inline T* _NEW_ARRAY(size_t num, const char* filename, int fileline)
{
	//
	// 多申请四个字节存储元素的个数
	//
	void* ptr = MemoryManager::GetInstance()->Alloc\
		(typeid(T).name(), sizeof(T)*num + 4, num, filename, fileline);

	// 内存分配失败，则直接返回
	if (ptr == NULL)
		return NULL;

	*(int*)ptr = num;
	T* retPtr = (T*)((int)ptr + 4);

	//
	// 使用【类型萃取】判断该类型是否需要调用析构函数
	//【未自定义构造函数析构函数的类型不需要调构造析构函数】
     construct(num,retPtr,__TypeTraits<T>::IsPODType());

	return retPtr;
}
template<typename T>
void destruct(int num,T*ptr,__FalseType)
{
     for (int i = 0; i < num; ++i)
		{
			ptr[i].~T();
		}
}
template<typename T>
void destruct(int num,T*ptr,__TrueType)
{}

template<typename T>
inline void _DELETE_ARRAY(T* ptr)
{
	// ptr == NULL，则直接返回
	if (ptr == NULL)
		return;

	T* selfPtr = (T*)((int)ptr - 4);
	int num = *(int*)(selfPtr);
	//
	// 使用【类型萃取】判断该类型是否需要调用析构函数
	//【未自定义构造函数析构函数的类型不需要调构造析构函数】
	destruct(num,ptr,__TypeTraits<T>::IsPODType());

	MemoryManager::GetInstance()->Dealloc(typeid(T).name(), selfPtr, sizeof(T)*num);
}


//
// 分配对象
// @parm type 申请类型
// ps:NEW直接使用宏替换，可支持 NEW(int)(0)的使用方式
#define NEW(type)							\
	new(MemoryManager::GetInstance()->Alloc \
	(typeid(type).name(), sizeof(type), 1, __FILE__, __LINE__)) type

//
// 释放对象
// @parm ptr 待释放对象的指针
//
#define DELETE(ptr)						\
	_DELETE(ptr)

//
// 分配对象数组
// @parm type 申请类型
// @parm num 申请对象个数
//
#define NEW_ARRAY(type, num)			\
	_NEW_ARRAY<type>(num, __FILE__, __LINE__)

// 释放数组对象
// @parm ptr 待释放对象的指针
//
#define DELETE_ARRAY(ptr)				\
	_DELETE_ARRAY(ptr)
