#pragma once
#include"Base.h"

class Lender;

namespace {
	//参照先のポインタを持っている
	template<class C>
	class BorrowImpl final :public BaseBorrow
	{
		C* borrow_obj = nullptr;
	public:
		Lender* cast_lender = nullptr;//キャストしたBorrowを作るときに使う

		BorrowImpl(Lender* lender) :BaseBorrow(lender), cast_lender(lender) {}

		void _init()
		{
			borrow_obj = static_cast<C*>(lender->owner);
		}

		C* get()const
		{
			//貸出者が生きてたら借りたものを返す
			if (lender)return borrow_obj;
			return nullptr;
		}
	};
}
//_Borrowのスマポ。sharedだからコピーも可
template<class C>
class Borrow final
{
	std::shared_ptr<BorrowImpl<C>> ptr = nullptr;
public:
	Borrow(BorrowImpl<C>* ptr)
		:ptr(std::forward<BorrowImpl<C>*>(ptr))
	{}
	Borrow() = default;
	////コピー可
	//Borrow(const Borrow<C>& other) = default;
	//Borrow<C>& operator = (const Borrow<C>&) = default;
	template<class T>
	Borrow<T> cast() const
	{
		if (auto p = ptr.get())
			if (auto pp = p->get())
				if (auto castpp = dynamic_cast<T*>(pp))
					return castpp->lend();

		return Borrow<T>();
	}

	//借用先が削除されてたらfalse
	operator bool()const
	{
		return ptr and ptr.get()->get();//nullptrかポインタ
	}

	C* get()const
	{
		if (auto p = ptr.get()) return p->get();
		else return nullptr;
	}

	operator C* ()const
	{
		if (auto p = ptr.get()) return p->get();
		else return nullptr;
	}

	C* operator -> ()const
	{
		if (auto p = ptr.get()) return p->get();
		else return nullptr;
	}

	template<class T, std::enable_if_t<std::is_base_of_v<T, C>>* = nullptr>
	operator Borrow<T>()const;
};

//貸出機能など
class Lender final :public BaseLender
{
public:
	using BaseLender::BaseLender;

	template<class C, std::enable_if_t<std::is_base_of_v<BaseBorrowable, C>>* = nullptr>
	Borrow<C> lend()
	{
		auto bor = new BorrowImpl<C>(this);//thisを使って貸し出す
		bor->_init();
		callbacks.emplace(bor, [=](BaseLender* next) {bor->lender = next; bor->_init(); });
		return Borrow<C>(bor);
	};


};

//これを継承するだけ
class Borrowable :public BaseBorrowable
{
public:
	using BaseBorrowable::BaseBorrowable;

	template<class Self>
	Borrow<Self> lend(this Self& self)
	{
		return self.lender->lend<Self>();
	};

	template<class T>
	Borrow<T> lend()
	{
		return this->lend().cast<T>();
	};

	template<class T> operator Borrow<T>()
	{
		return this->lend<T>();
	}
};

//キャスト
template<class C>
template<class T, std::enable_if_t<std::is_base_of_v<T,C>>*>
Borrow<C>::operator Borrow<T>()const
{
	if (ptr and ptr->get())return ptr->cast_lender->lend<T>();
	else return Borrow<T>(nullptr);
};

template<class _Ty1, class _Ty2>
bool operator == (const Borrow<_Ty1>& _Left, const Borrow<_Ty2>& _Right)
{
	return _Left.get() == _Right.get();
}
