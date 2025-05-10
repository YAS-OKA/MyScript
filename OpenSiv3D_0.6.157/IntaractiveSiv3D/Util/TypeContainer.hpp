#pragma once
#include"../stdafx.h"

template <class T>
class WeakTypeContainer
{
public:
	HashTable<std::type_index, HashTable<String, T*>> arr{};

	HashTable<std::type_index, size_t> ids{};

	WeakTypeContainer() = default;
	WeakTypeContainer(WeakTypeContainer&& other) = default;
	WeakTypeContainer& operator = (WeakTypeContainer&&) = default;

	WeakTypeContainer(const WeakTypeContainer&) = delete;
	WeakTypeContainer& operator = (const WeakTypeContainer&) = delete;

	template<class A>
	A* addPtr(A* a)
	{
		addPtr(typeid(A), a);
		return a;
	}

	String addPtr(std::type_index ty, T* ptr)
	{
		if (not ids.contains(ty))ids.emplace(ty, 0);
		else ++ids[ty];
		auto name = Format(ids[ty]);
		addPtrIn(ty, name, ptr);
		return name;
	}

	template<class A>
	A* addPtrIn(StringView id,A* a)
	{
		return addPtrIn(typeid(A), id, a);
	}

	T* addPtrIn(std::type_index ty, StringView id, T* ptr)
	{
		if (arr[ty].contains(id))remove(ty,id);
		arr[ty].emplace(id, ptr);
		return ptr;
	}

	template<class A>
	A* setPtr(A* a)
	{
		return setPtrIn<A>(U"0", a);
	}

	template<class A, class... Args>
	A* setPtrIn(StringView id, A* a)
	{
		remove<A>();

		return addPtrIn<A>(id, a);
	}

	template<class A, std::enable_if_t<std::is_base_of_v<T, A>>* = nullptr>
	A* get()
	{
		if (not arr.contains(typeid(A)))return nullptr;
		if (arr.at(typeid(A)).empty())return nullptr;//これいる？

		return static_cast<A*>(arr[typeid(A)].begin()->second);
	}

	template<class A, std::enable_if_t<std::is_base_of_v<T, A>>* = nullptr>
	A* get(StringView id)
	{
		if (not arr.contains(typeid(A)))return nullptr;
		if (not arr.at(typeid(A)).contains(id))return nullptr;

		return static_cast<A*>(arr[typeid(A)][id]);
	}

	template<class A, std::enable_if_t<std::is_base_of_v<T, A>>* = nullptr>
	Array<A*> getArray()
	{
		if (not arr.contains(typeid(A)))return nullptr;
		if (arr.at(typeid(A)).empty())return nullptr;//これいる？
		Array<A*> res{};

		for (auto& [_, value] : arr[typeid(A)])
		{
			res << static_cast<A*>(value);
		}

		return res;
	}

	HashSet<T*> removeAll()
	{
		HashSet<T*> rem{};
		if (arr.empty())return rem;
		for (auto& [_,table] : arr)
		{
			for (auto& [id,elm] : table)
			{
				rem.emplace(elm);
			}
		}
		arr.clear();
		return rem;
	}

	template<class A>
	HashSet<T*> remove()
	{
		HashSet<T*> rem{};
		if (not arr.contains(typeid(A)))return rem;

		for (auto& elm : arr[typeid(A)])
		{
			rem.emplace(elm.second);
		}
		arr.erase(typeid(A));

		return rem;
	}

	template<class A>
	T* remove(StringView id)
	{
		return remove(typeid(A), id);
	}

	T* remove(const std::type_index& ind, StringView id)
	{
		if (not (arr.contains(ind) and arr[ind].contains(id)))return nullptr;

		auto tmp = arr[ind][id];
		arr[ind].erase(id);
		return tmp;
	}

	T* remove(std::type_index ind,T* ptr)
	{
		if (not arr.contains(ind))return nullptr;

		for (auto it = arr[ind].begin(), en = arr[ind].end(); it != en; ++it)
		{
			if (it->second == ptr)
			{
				arr[ind].erase(it);
				return ptr;
			}
		}
		return nullptr;
	}

	template<class A>
	T* remove(A* rem)
	{
		return remove(typeid(A), rem);
	}
};

template <class T>
class TypeContainer
{
public:
	HashTable<std::type_index, HashTable<String, T*>> arr{};

	HashTable<std::type_index, size_t> ids{};

	Array<T*> garbages{};

	//メモリの解放
	void _del()
	{
		for (auto& multi_comp : arr)
		{
			for (auto& comp : multi_comp.second)
			{
				delete comp.second;
			}
		}
		arr.clear();
	};

	void deleteGarbages()
	{
		//ガーベージをクリア
		for (auto& garbage : garbages)
		{
			if (garbage != nullptr)delete garbage;
		}
		garbages.clear();
	};

	TypeContainer() = default;
	TypeContainer(TypeContainer&& other) = default;
	TypeContainer& operator = (TypeContainer&&) = default;

	TypeContainer(const TypeContainer&) = delete;
	TypeContainer& operator = (const TypeContainer&) = delete;

	~TypeContainer()
	{
		_del();
		deleteGarbages();
	}

	template<class A, class... Args>
	A* add(Args&& ...args)
	{
		if (not ids.contains(typeid(A)))ids.emplace(typeid(A), 0);
		else ++ids[typeid(A)];
		return addIn<A>(Format(ids[typeid(A)]), std::forward<Args>(args)...);
	}

	template<class A, class... Args>
	A* addIn(StringView id, Args&& ...args)
	{
		if (arr[typeid(A)].contains(id))remove<A>(id);

		auto a = new A(std::forward<Args>(args)...);

		arr[typeid(A)].emplace(id, a);

		return a;
	}

	template<class A, class... Args>
	A* set(Args&& ...args)
	{
		ids[typeid(A)] = 0;
		return setIn<A>(U"0", std::forward<Args>(args)...);
	}

	template<class A, class... Args>
	A* setIn(StringView id, Args&& ...args)
	{
		remove<A>();

		return addIn<A>(id, std::forward<Args>(args)...);
	}

	template<class A, std::enable_if_t<std::is_base_of_v<T, A>>* = nullptr>
	A* get()
	{
		if (not arr.contains(typeid(A)))return nullptr;
		if (arr.at(typeid(A)).empty())return nullptr;//これいる？

		return static_cast<A*>(arr[typeid(A)].begin()->second);
	}

	template<class A, std::enable_if_t<std::is_base_of_v<T, A>>* = nullptr>
	A* get(StringView id)
	{
		if (not arr.contains(typeid(A)))return nullptr;
		if (not arr.at(typeid(A)).contains(id))return nullptr;

		return static_cast<A*>(arr[typeid(A)][id]);
	}

	template<class A, std::enable_if_t<std::is_base_of_v<T, A>>* = nullptr>
	HashTable<String, A*> getAll()const
	{
		HashTable<String, A*> ret{};
		if (not arr.contains(typeid(A)))return ret;
		
		for (auto it = arr.at(typeid(A)).begin(), en = arr.at(typeid(A)).end(); it != en; ++it)
		{
			ret.emplace(it->first, static_cast<A*>(it->second));
		}
		return ret;
	}

	HashSet<T*> removeAll()
	{
		HashSet<T*> rem{};
		if (arr.empty())return rem;
		for (auto& [_, table] : arr)
		{
			for (auto& [id, elm] : table)
			{
				garbages << elm;
				rem.emplace(elm);
			}
		}
		arr.clear();
		return rem;
	}

	template<class A>
	HashSet<T*> remove()
	{
		HashSet<T*> rem{};
		if (not arr.contains(typeid(A)))return rem;

		for (auto& elm : arr[typeid(A)])
		{
			garbages << elm.second;
			rem.emplace(elm.second);
		}
		arr.erase(typeid(A));

		return rem;
	}

	template<class A>
	T* remove(StringView id)
	{
		return remove(typeid(A), id);
	}

	T* remove(const std::type_index& ind, StringView id)
	{
		if (not (arr.contains(ind) and arr[ind].contains(id)))return nullptr;

		auto tmp = arr[ind][id];
		garbages << tmp;
		arr[ind].erase(id);
		return tmp;
	}
};
