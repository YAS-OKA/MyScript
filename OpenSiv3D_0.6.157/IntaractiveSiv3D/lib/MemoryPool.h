#pragma once
#include<vector>

namespace ys
{
	template<class T>
	class MemoryPool
	{
		//領域確保のための型
		class Dummy { class Empty {}empty[sizeof(T)]; };
		union Chunk { size_t link; Dummy _; };

		Chunk* memorys;
		size_t allocPos;

		const size_t size;
	public:
		//コンストラクタ
		MemoryPool(size_t poolSize) :size(poolSize)
		{
			memorys = new Chunk[size];
			//emptyListとallocPosの初期化
			for (size_t i = 0; i < size; i++)
			{
				memorys[i].link = i + 1;
			}
			allocPos = 0;
		}

		~MemoryPool()
		{
			//メモリを所有していなければ解放を行わない
			if (not memorys)return;

			//確保されている場所はtrue、されていない場所はfalseを入れる配列を作成
			bool* isAllocated = new bool[size];
			//すべてtrue
			for (size_t i = 0; i < size; i++)
			{
				isAllocated[i] = true;
			}
			while (allocPos < size)
			{
				//空いている場所にfalseを入れる
				isAllocated[allocPos] = false;
				//次の空いている場所へ移動
				allocPos = memorys[allocPos].link;
			}

			for (size_t i = 0; i < size; i++)
			{
				//確保されている場所だったら
				if (isAllocated[i])
				{
					//DummyのアドレスをT型ポインターとして再解釈
					T* reinterpretPointer = (T*)&memorys[i];
					//デストラクタ呼び出し
					reinterpretPointer->~T();
				}
			}
			delete[]isAllocated;
			delete[]memorys;
		}

		
		MemoryPool(const MemoryPool&) = delete;
		MemoryPool& operator=(const MemoryPool&) = delete;
		MemoryPool(MemoryPool&& other) noexcept
			: memorys(other.memorys), allocPos(other.allocPos), size(other.size)
		{
			other.memorys = nullptr;
			other.allocPos = 0;
		}
		MemoryPool& operator=(MemoryPool&& other) noexcept
		{
			if (this != &other)
			{
				this->~MemoryPool(); // 現在のリソースを解放

				memorys = other.memorys;
				allocPos = other.allocPos;
				const_cast<size_t&>(size) = other.size;

				other.memorys = nullptr;
				other.allocPos = 0;
			}
			return *this;
		}

		//メモリの確保
		template<class ...Args>
		T* alloc(Args&&... args)
		{
			if (size <= allocPos)return nullptr;
			//移動先の取得
			size_t nextAllocPos = memorys[allocPos].link;
			//オブジェクト生成
			T* ret = new (&memorys[allocPos]) T(args...);
			//allocPosを移動
			allocPos = nextAllocPos;

			return ret;
		}

		//メモリを解放
		void free(T* ptr)
		{
			Chunk* reinterpretPtr = (Chunk*)ptr;
			//未割り当てのポインタだったら
			if (reinterpretPtr < memorys || &memorys[size - 1] < reinterpretPtr)
			{
				//早期リターンでメモリの解放を行わずに関数を終了
				return;
			}

			ptr->~T();
			//現在のallocPosを取得
			size_t currentAllocPos = allocPos;
			//削除されたメモリへ移動
			allocPos = (Chunk*)ptr - memorys;
			//移動前の場所を移動先に設定
			memorys[allocPos].link = currentAllocPos;
		}
	};
}
