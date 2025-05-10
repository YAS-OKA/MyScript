#include"../../stdafx.h"
#include"Base.h"
#include"Borrowable.hpp"

BaseBorrow::BaseBorrow(BaseLender* lender)
	:lender(lender) {}

BaseBorrow::~BaseBorrow()
{
	//自分のコールバックを削除する。
	if (lender)lender->_remove(this);
}
//貸出者をnullに
void BaseBorrow::removeBorrower()
{
	lender = nullptr;
}

BaseLender::BaseLender(BaseBorrowable* owner)
{
	setOwner(owner);
}

void BaseLender::setOwner(BaseBorrowable* owner)
{
	this->owner = owner;
	for (auto& [key, value] : callbacks)value(this);
}

void BaseLender::_remove(BaseBorrow* bor)
{
	callbacks.erase(bor);
}

BaseLender::~BaseLender()
{
	//borrowのborrowerを無効にする.
	for (auto& [key, value] : callbacks)key->removeBorrower();
}

BaseBorrowable::BaseBorrowable()
	:lender(new Lender(this))
{
}

BaseBorrowable::BaseBorrowable(BaseBorrowable&& other)
	:lender(std::move(other.lender))
{
	//ownerをリセット
	lender->setOwner(this);
}

BaseBorrowable& BaseBorrowable::operator=(BaseBorrowable&& other)
{
	lender = std::move(other.lender);
	lender->setOwner(this);
	return *this;
}
