#pragma once

/// <summary>
/// 一个用IOCP实现的线程安全的队列
/// </summary>
/// <typeparam name="T">模板类型</typeparam>
template<class T>
class CEdoyunQueue {
public:
	CEdoyunQueue();
	~CEdoyunQueue();
	bool void PushBack(const T& data);
	bool void PopFront(T& data);
	size_t size();
	void Clear();

private:
	static void threadEntry(void* arg);
	void threadMain();

private:
	std::list<T> m_lstData;
	HANDLE m_hCompletionPort;
	HANDLE m_hThread;

public:
	typedef struct iocpParam {
		int nOperator;//操作
		T strData;//数据
		HANDLE hEvent;//pop 操作需要

		iocpParam(int op, const char* sData, _beginthread_proc_type cb = NULL) {
			nOperator = op;
			strData = sData;
		}
		iocpParam() {
			nOperator = -1;
		}
	} PPARAM;//post Parameter 用于投递信息的结构体
	enum MyEnum {
		EQPush,
		EQPop,
		EQSize,
		EQClear
	};
};

