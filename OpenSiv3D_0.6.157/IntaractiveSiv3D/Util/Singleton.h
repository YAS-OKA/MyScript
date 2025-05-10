#pragma once
#include"Borrow.h"

//これを継承して使う
template<class T>
class BorrowableSingleton:public Borrowable
{
	static T* instance;
protected:
	BorrowableSingleton() {};
	virtual ~BorrowableSingleton() {};

public:
	virtual void init() {};

	static void Create()
	{
		if (instance == nullptr)
		{
			instance = new T();
			instance->init();
		}
	}

	static void Destroy()
	{
		if (instance != nullptr)
		{
			delete instance;
			instance = nullptr;
		}
	}

	static Borrow<T> GetInstance()
	{
		if (instance == nullptr)
		{
			Create();
		}

		return instance->lend();
	}
};

template<class T>
T* BorrowableSingleton<T>::instance = nullptr;
