/*************************************************************************
    > File Name: SaveAdapter.h
    > Author: WK
    > Mail:18402927708@163.com 
    > Created Time: Mon 02 May 2016 05:49:49 PM CST
 ************************************************************************/

#include <stdarg.h>
#include <time.h>
#include <assert.h>
#include <string>
#include<vector>
#include<stdio.h>
#include <map>
#include <algorithm>
#include<iostream>
#include <sys/types.h>
#include <unistd.h>
#include<sys/time.h>>
using namespace std;
// 保存适配器抽象基类
class SaveAdapter
{
public:
	virtual int Save(char* format, ...) = 0;
};

// 控制台保存适配器
class ConsoleSaveAdapter : public SaveAdapter
{
public:
	virtual int Save(char* format, ...)
	{
		va_list argPtr;
		int cnt;

		va_start(argPtr, format);
		cnt = vfprintf(stdout, format, argPtr);
		va_end(argPtr);

		return cnt;
	}
};

// 文件保存适配器
class FileSaveAdapter : public SaveAdapter
{
public:
	FileSaveAdapter(const char* path)
		:_fOut(0)
	{
		_fOut = fopen(path, "w");
	}

	~FileSaveAdapter()
	{
		if (_fOut)
		{
			fclose(_fOut);
		}
	}

	virtual int Save(char* format, ...)
	{
		if (_fOut)
		{
			va_list argPtr;
			int cnt;

			va_start(argPtr, format);
			cnt = vfprintf(_fOut, format, argPtr);
			va_end(argPtr);

			return cnt;
		}

		return 0;
	}
private:
	FileSaveAdapter(const FileSaveAdapter&);
	FileSaveAdapter& operator==(const FileSaveAdapter&);

private:
	FILE* _fOut;
};
