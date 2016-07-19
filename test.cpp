/*************************************************************************
    > File Name: test.cpp
    > Author: WK
    > Mail:18402927708@163.com 
    > Created Time: Mon 02 May 2016 06:00:02 PM CST
 ************************************************************************/

#include<iostream>
using namespace std;
#include"UnitMemory_Profile.h"

/*enum PP_CONFIG_OPTION
{
	PPCO_NONE = 0,					// 不做剖析
	PPCO_PROFILER = 2,				// 开启剖析
	PPCO_SAVE_TO_CONSOLE = 4,		// 保存到控制台
	PPCO_SAVE_TO_FILE = 8,			// 保存到文件
	PPCO_SAVE_BY_CALL_COUNT = 16,	// 按调用次数降序保存
	PPCO_SAVE_BY_COST_TIME = 32,	// 按调用花费时间降序保存
};
*/



// 1.测试基本功能
void Test1()
{
	PERFORMANCE_PROFILER_BEGIN(PP1, "PP1");
    
	sleep(30);
	PERFORMANCE_PROFILER_END(PP1);

	PERFORMANCE_PROFILER_BEGIN(PP2, "PP2");

	usleep(200);

	PERFORMANCE_PROFILER_END(PP2);
}
// 2.测试资源检测
void Test2()
{
	PERFORMANCE_PROFILER_AND_RS_BEGIN(PP1, "PP1");
    
	int *p = new int(10000);
	delete p;
	sleep(30);
	PERFORMANCE_PROFILER_AND_RS_END(PP1);

	PERFORMANCE_PROFILER_AND_RS_BEGIN(PP2, "PP2");

	usleep(200);

	PERFORMANCE_PROFILER_AND_RS_END(PP2);
}


int main()
{

  SET_OPTIONS(PPCO_PROFILER | PPCO_SAVE_TO_CONSOLE);
  Test1();
  //Test2();
return 0;
}

