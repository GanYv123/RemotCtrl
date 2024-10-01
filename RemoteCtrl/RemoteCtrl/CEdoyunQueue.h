#pragma once

/// <summary>
/// һ����IOCPʵ�ֵ��̰߳�ȫ�Ķ���
/// </summary>
/// <typeparam name="T">ģ������</typeparam>
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
		int nOperator;//����
		T strData;//����
		HANDLE hEvent;//pop ������Ҫ

		iocpParam(int op, const char* sData, _beginthread_proc_type cb = NULL) {
			nOperator = op;
			strData = sData;
		}
		iocpParam() {
			nOperator = -1;
		}
	} PPARAM;//post Parameter ����Ͷ����Ϣ�Ľṹ��
	enum MyEnum {
		EQPush,
		EQPop,
		EQSize,
		EQClear
	};
};

