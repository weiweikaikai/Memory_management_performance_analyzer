/*************************************************************************
	> File Name: Lock.h
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Thu 14 Jul 2016 07:58:49 PM CST
 ************************************************************************/

#ifndef _LOCK_H
#define _LOCK_H
#include<pthread.h>
using namespace std;

class Lock
{
	public:
		Lock(pthread_mutex_t cs):m_lock(cs)
	    {
		  pthread_mutex_lock(&m_lock);
		}
         ~Lock()
	     {
		   pthread_mutex_unlock(&m_lock);
		 }
	private:
		pthread_mutex_t m_lock;
};//锁类，实现多线程环境下高效工作

#endif
