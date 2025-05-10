#pragma once
#include"../stdafx.h"

namespace util
{
	/// @brief オブジェクトの親子関係のツリー
	/// @tparam T オブジェクトの型
	template<class T>
	class PCTree
	{
	private:
		PCTree() {};
		~PCTree() {};
		/// @brief 親->子
		static HashTable<T*, Array<T*>>PtoC_list;
		/// @brief 子->親
		static HashTable<T*, Array<T*>>CtoP_list;

	public:
		static void Register(T* owner)
		{
			PtoC_list.emplace(owner, Array<T*>{});
			CtoP_list.emplace(owner, Array<T*>{});
		}

		static void Erase(T* owner)
		{
			ClearChildren(owner);
			ClearParents(owner);
			PtoC_list.erase(owner);
			CtoP_list.erase(owner);
		}

		static Array<T*> GetParents(T* child)
		{
			if (CtoP_list.contains(child))
				return CtoP_list[child];
			else
				return Array<T*>{};
		};

		static Array<T*> GetChildren(T* parent)
		{
			if (PtoC_list.contains(parent))
				return PtoC_list[parent];
			else
				return Array<T*>{};
		};

		static T* GetParent(T* child) {
			const auto& parents = GetParents(child);
			return parents.isEmpty() ? nullptr : parents[0];
		};

		static T* GetChild(T* parent) {
			const auto& children = GetChildren(parent);
			return children.isEmpty() ? nullptr : children[0];
		};
		//親のセット
		static void SetParent(T* owner, T* parent) {
			RemoveParent(owner, GetParents(owner));
			AddParent(owner, parent);
		};
		//親の追加
		static void AddParent(T* owner, T* parent) {
			PtoC_list[parent] << owner;
			CtoP_list[owner] << parent;
		};
		//親のセット
		static void SetParent(T* owner, Array<T*> parents) {
			RemoveParent(owner, GetParents());
			AddParent(owner, parents);
		};
		//親の追加
		static void AddParent(T* owner, Array<T*> parents) {
			for (auto itr = parents.begin(), en = parents.end(); itr != en; ++itr) {
				AddParent(owner, *itr);
			}
		};
		//親の一掃
		static void ClearParents(T* owner) {
			RemoveParent(owner, GetParents(owner));
		};
		//親の削除
		static void RemoveParent(T* owner, Array<T*> parents) {
			for (auto itr = parents.begin(), en = parents.end(); itr != en; ++itr) {
				RemoveParent(owner, *itr);
			}
		};
		//親の削除
		static void RemoveParent(T* owner, T* parent) {
			PtoC_list[parent].remove(owner);
			CtoP_list[owner].remove(parent);
		};

		//子のセット
		static void SetChild(T* owner, T* child) {
			RemoveChild(owner,GetChildren(owner));
			AddChild(owner,child);
		};
		//子の追加
		static void AddChild(T* owner, T* child) {
			PtoC_list[owner] << child;
			CtoP_list[child] << owner;
		};
		//子のセット
		static void SetChild(T* owner, Array<T*> children) {
			RemoveChild(owner,GetChildren(owner));
			AddChild(owner, children);
		};
		//子の追加
		static void AddChild(T* owner, Array<T*> children) {
			for (auto itr = children.begin(), en = children.end(); itr != en; ++itr) {
				AddChild(owner, *itr);
			}
		};
		//子の一掃
		static void ClearChildren(T* owner) {
			RemoveChild(owner, GetChildren(owner));
		};
		//子の削除
		static void RemoveChild(T* owner, Array<T*> children) {
			for (auto itr = children.begin(), en = children.end(); itr != en; ++itr) {
				RemoveChild(owner, *itr);
			}
		};
		//子の削除
		static void RemoveChild(T* owner, T* child) {
			PtoC_list[owner].remove(child);
			CtoP_list[child].remove(owner);
		};
	};
	//定義
	template<class T> HashTable<T*, Array<T*>> PCTree<T>::PtoC_list = HashTable<T*, Array<T*>>();
	template<class T> HashTable<T*, Array<T*>> PCTree<T>::CtoP_list = HashTable<T*, Array<T*>>();

	/// @brief 親子関係クラス
	/// @tparam T 
	template <class T>
	class PCRelationship
	{
	private:
		T* owner;
	public:
		PCRelationship(T* owner)
			:owner(owner)
		{
			PCTree<T>::Register(owner);
		}

		~PCRelationship()
		{
			PCTree<T>::Erase(owner);
		}

		Array<T*> getParents()const {
			return PCTree<T>::GetParents(owner);
		}

		T* getParent() const {
			return PCTree<T>::GetParent(owner);
		}

		Array<T*> getChildren()const {
			return PCTree<T>::GetChildren(owner);
		}

		T* getChild()const {
			return PCTree<T>::GetChild(owner);
		}

		//親のセット
		void setParent(T* parent) {
			PCTree<T>::SetParent(owner, parent);
		};
		//親の追加
		void addParent(T* parent) {
			PCTree<T>::AddParent(owner, parent);
		};
		//親のセット
		void setParent(Array<T*> parents) {
			PCTree<T>::SetParent(owner, parents);
		};
		//親の追加
		void addParent(Array<T*> parents) {
			PCTree<T>::AddParent(owner, parents);
		};
		//親の一掃
		void clearParents() {
			PCTree<T>::ClearParents(owner);
		};
		//親の削除
		void removeParent(Array<T*> parent) {
			PCTree<T>::RemoveParent(owner, parent);
		};
		//親の削除
		void removeParent(T* parent) {
			PCTree<T>::RemoveParent(owner, parent);
		};

		//子のセット
		void setChild(T* child) {
			PCTree<T>::SetChild(owner, child);
		};
		//子の追加
		void addChild(T* child) {
			PCTree<T>::AddChild(owner, child);
		};
		//子のセット
		void setChild(Array<T*> children) {
			PCTree<T>::SetChild(owner, children);
		};
		//子の追加
		void addChild(Array<T*> children) {
			PCTree<T>::AddChild(owner, children);
		};
		//子の一掃
		void clearChildren() {
			PCTree<T>::ClearChildren(owner);
		};
		//子の削除
		void removeChild(Array<T*> children) {
			PCTree<T>::RemoveChild(owner, children);
		};
		//子の削除
		void removeChild(T* child) {
			PCTree<T>::RemoveChild(owner, child);
		};
	};
}
