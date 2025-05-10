#pragma once

//借用クラス
class BaseBorrow
{
public:
	class BaseLender* lender = nullptr;

	BaseBorrow(BaseLender* lender);

	virtual ~BaseBorrow();

	void removeBorrower();
};
//BaseBorrowableの貸し出しクラス
class BaseLender
{
public:
	class BaseBorrowable* owner = nullptr;
	//借用先を変えるためのコールバック
	HashTable<BaseBorrow*, std::function<void(BaseLender*)>>callbacks;

	BaseLender(BaseBorrowable* owner);

	virtual ~BaseLender();

	void setOwner(BaseBorrowable* owner);

	void _remove(BaseBorrow* bor);
};

class Lender;

//借用可能オブジェクトの基底
class BaseBorrowable
{
public:
	std::unique_ptr<Lender> lender;

	BaseBorrowable();

	BaseBorrowable(BaseBorrowable&& other);

	BaseBorrowable& operator =(BaseBorrowable&& other);

	BaseBorrowable(const BaseBorrowable&) = delete;

	BaseBorrowable& operator =(const BaseBorrowable&) = delete;

	virtual ~BaseBorrowable() = default;
};
