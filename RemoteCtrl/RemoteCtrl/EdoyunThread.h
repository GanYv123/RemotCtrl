#pragma once

#include "pch.h"
#include <atomic>
#include <vector>
#include <mutex>
#include <Windows.h>

class ThreadFuncBase {};

typedef int(ThreadFuncBase::* FUNCTYPE)();

class ThreadWorker {
	//封装了类要执行的任务
public:
	ThreadWorker() :thiz(NULL), func(NULL) {

	}
	ThreadWorker(ThreadFuncBase* obj, FUNCTYPE f) :thiz(obj), func(f) {

	}
	ThreadWorker(const ThreadWorker& worker) {
		thiz = worker.thiz;
		func = worker.func;
	}
	ThreadWorker& operator=(const ThreadWorker& worker) {
		if (this != &worker) {
			thiz = worker.thiz;
			func = worker.func;
		}
		return *this;
	}
	int operator()() {
		if (IsValid()) {
			return (thiz->*func)();
		}
		return -1;
	}
	bool IsValid() const{
		return (thiz != NULL) && (func != NULL);
	}

private:
	ThreadFuncBase* thiz;
	FUNCTYPE func;//ThreadFuncBase成员函数指针
};

class EdoyunThread {
	//管理一个线程，负责 创建 销毁 启动
public:
	EdoyunThread() : m_hThread(NULL), m_bStatus(false) {}
	~EdoyunThread() { Stop(); }
	//true 成功  false 失败
	bool Start() {
		m_bStatus = true;
		m_hThread = (HANDLE)_beginthread(&EdoyunThread::ThreadEntry, 0, this);
		if (!IsValid()) {
			m_bStatus = false;
		}
		return m_bStatus;
	}

	//线程是否有效
	bool IsValid() {
		if (m_hThread == NULL || (m_hThread == INVALID_HANDLE_VALUE)) return false;
		return (WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT);
	}

	bool Stop() {
		if (m_bStatus == false) return true;
		m_bStatus = false;
		bool ret = (WaitForSingleObject(m_hThread, INFINITE) == WAIT_OBJECT_0);
		UpdateWorker();
		return ret;
	}

	void UpdateWorker(const ::ThreadWorker& worker = ::ThreadWorker()) {
		if (!worker.IsValid()) {
			m_worker.store(NULL);
			return;
		}
		if (m_worker.load() != NULL) {
			::ThreadWorker* pWorker = m_worker.load();
			m_worker.store(NULL);
			delete pWorker;
		}
		m_worker.store(new ::ThreadWorker(worker));
	}

	//true 空闲； false 已经分配工作;
	bool IsIdle() {
		if (m_worker.load() == NULL) return true;
		return !m_worker.load()->IsValid();

	}

private:
	void ThreadWorker() {
		while (m_bStatus) {
			if (m_worker.load() == NULL) {
				Sleep(1);
				continue;
			}
			::ThreadWorker worker = *m_worker.load();
			if (worker.IsValid()) {
				if (WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT) {
					int ret = worker();
					if (ret != 0) {
						CString str;
						str.Format(_T("thread found warning code %d\r\n"), ret);
						OutputDebugString(str);
					}
					if (ret < 0) {
						m_worker.store(NULL);
					}
				}	
			}
			else {
				Sleep(1);
			}
		}
	}

	static void ThreadEntry(void* arg) {
		EdoyunThread* thiz = (EdoyunThread*)arg;
		if (thiz) {
			thiz->ThreadWorker();
		}
		_endthread();
	}

private:
	HANDLE m_hThread;
	bool m_bStatus;//false -> 线程将要关闭 true -> 表示线程正在运行
	std::atomic<::ThreadWorker*> m_worker;
};

class EdoyunThreadPool {
	//线程池
public:
	EdoyunThreadPool(size_t size) {
		m_threads.resize(size);
		for (size_t i = 0; i < size; i++) {
			m_threads[i] = new EdoyunThread();
		}
	}
	EdoyunThreadPool() {

	}
	~EdoyunThreadPool() {
		Stop();
		m_threads.clear();
	}
	bool Invoke() {
		bool ret = true;
		for (size_t i = 0; i < m_threads.size(); ++i) {
			if (m_threads[i]->Start() == false) {
				ret = false;
				break;
			}
		}
		if (ret == false) {
			for (size_t i = 0; i < m_threads.size(); ++i) {
				m_threads[i]->Stop();
			}
		}
		return ret;
	}

	void Stop() {
		for (size_t i = 0; i < m_threads.size(); ++i) {
			m_threads[i]->Stop();
		}
	}

	//-1 表示分配失败（无空闲线程） >=0表示第n个线程来拿到该任务
	int DispatchWorker(const ThreadWorker& worker) {
		m_lock.lock();
		int index = -1;
		for (size_t i = 0; i < m_threads.size(); ++i) {
			if (m_threads[i]->IsIdle()) {
				m_threads[i]->UpdateWorker(worker);
				index = i;
				break;
			}
		}
		m_lock.unlock();
		return index;
	}

	bool CheckThreadValid(size_t index) {
		if (index < m_threads.size()) {
			return m_threads[index]->IsValid();
		}
		return false;
	}
private:
	std::mutex m_lock;
	std::vector<EdoyunThread*> m_threads;
};