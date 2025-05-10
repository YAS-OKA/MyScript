#pragma once

namespace ys
{
	//strの長さを返す（半角なら1文字、全角なら2文字として扱う）
	size_t getStringLength(const String& str);

	template<class T>
	typename std::enable_if_t<not std::is_pointer_v<T>, String>
		VisualizeTree(
		T* obj,
		std::function<String(T*)> id,
		std::function<Array<T*>(T*)> children)
	{
		auto result = id(obj);

		auto stack = children(obj);
		if (stack.isEmpty())return result;

		enum State { branch, bottom, };

		using StateInfo = std::pair<Vector2D<size_t>, State>;//その世帯の状態を記録
		using ProgressInfo = std::pair<const T*, size_t>;//親に対し、何人目の子まで処理し終えたか
		using HouseholdInfo = std::pair<ProgressInfo, StateInfo>;

		HouseholdInfo top{ {obj,0}, {{result.size(),0}, branch} };
		Array<HouseholdInfo> branchState{ top };

		//子ー＞{親,兄弟数}
		HashTable<T*, std::pair<T*, size_t>> childToParentWithBraNum{ {obj,{nullptr,1}} };
		stack.each(
			[&](const auto& t)
			{
				childToParentWithBraNum[t] = { obj,stack.size() };
			}
		);

		auto lastLineLength = [&]
			{
				size_t index = result.lastIndexOf(U"\n");
				if (index == String::npos)return result.size();
				index = getStringLength(result.substr(0, index));
				auto length = getStringLength(result);
				return length - index;
			};

		auto spacePadding = [&](size_t x)
			{
				auto length = lastLineLength();
				bool ret = length < x;
				for (auto _ : step((ret ? x - length : 0)))result += U" ";
				return ret;
			};

		do {
			auto child = stack.back();
			auto [parent, braNum] = childToParentWithBraNum[child];

			auto name = id(child);
			if (not name)name = U"empty";

			if (braNum - 1 != 0) //自分以外の兄弟がいる
			{
				size_t rowDiff = 0;
				auto currentRow = result.count(U"\n");
				for (size_t i = 0; i < branchState.size(); ++i)
				{
					const auto [upperParent, progress] = branchState[i].first;
					const auto [branchPos, state] = branchState[i].second;

					if (currentRow == 0 and parent != upperParent)continue;

					auto hasPadding = spacePadding(branchPos.x);
					//親の行からいくつずれてるか
					rowDiff = currentRow - branchPos.y;
					//今処理している子の親でない、もしく奇数行もしくはまだ描いてないなら
					if ((parent != upperParent) or currentRow % 2)
					{
						if (hasPadding)result += U" |";
						continue;
					}
					//自分が兄弟の中で何番目に描かれたか
					auto order = progress + 1;
					++branchState[i].first.second;
					//描かれたのが一番最後なら
					if (order == braNum)
					{
						result += U" +- " + name;
						branchState.pop_back();
						break;
					}
					//一番最初なら「+-」そうでなければ「|-」
					result += (order == 1 ? U" +- " : U" |- ") + name;
				}
				//奇数行だったらここで次の行へ
				if (currentRow % 2) { result += U"\n"; continue; }
			}
			else //兄弟がいない
			{
				result += U" -- " + name;
				branchState.pop_back();
			}
			stack.pop_back();
			if (auto next = children(child))
			{
				next.each(
					[&](const auto& v)
					{
						childToParentWithBraNum[v]
							= std::pair<T*, size_t>{ child,next.size() };
					}
				);
				stack.append(next);
				branchState.push_back({ { child, 0 }, { { lastLineLength(),result.count(U"\n")}, branch} });
			}
			else
			{
				result += U"\n";
			}
		} while (stack);

		return result;
	}

	template<class T>
	typename std::enable_if_t<not std::is_pointer_v<T>, String>
		VisualizeTree(
			T objValue,
			std::function<String(T)> id,
			std::function<Array<T>(T)> children
		)
	{
		//一時的に展開されたオブジェクトを格納しておく
		Array<std::shared_ptr<T>> objectContainer{};

		auto pushToContainer = [&](const Array<T>& objs)
			{
				Array<size_t> ret;
				objs.each(
					[&](const auto& obj) {
						auto ptr = std::shared_ptr<T>{ new T(obj) };
						objectContainer << ptr;
						ret << objectContainer.size();
					}
				);
				return ret;
			};
		auto getObj = [&](size_t i) {
			auto index = i - 1;
			if (objectContainer.size() < index)
				throw Error{ U"index out of range {}/{}"_fmt(index,objectContainer.size()) };
			return objectContainer[index].get();
		};

		auto obj = pushToContainer({ objValue })[0];

		//子ー＞{親,兄弟数}
		HashTable<size_t, std::pair<size_t, size_t>> childToParentWithBraNum{ {obj ,{-1,1}} };

		String result = id(*getObj(obj));

		if (not result)result = U"empty";

		auto stack = pushToContainer(children(*getObj(obj)));
		stack.each(
			[&](const auto& t)
			{
				childToParentWithBraNum[t] = { obj,stack.size() };
			}
		);

		if (stack.isEmpty())return result;

		enum State { branch, bottom, };

		using StateInfo = std::pair<Vector2D<size_t>, State>;//その世帯の状態を記録
		using ProgressInfo = std::pair<size_t, size_t>;//親に対し、何人目の子まで処理し終えたか
		using HouseholdInfo = std::pair<ProgressInfo, StateInfo>;

		HouseholdInfo top{ {obj,0}, {{result.size(),0}, branch} };
		Array<HouseholdInfo> branchState{ top };

		auto lastLineLength = [&]
			{
				size_t index = result.lastIndexOf(U"\n");
				if (index == String::npos)return result.size();
				index = getStringLength(result.substr(0, index));
				auto length = getStringLength(result);
				return length - index;
			};

		auto spacePadding = [&](size_t x)
			{
				auto length = lastLineLength();
				bool ret = length < x;
				for (auto _ : step((ret ? x - length : 0)))result += U" ";
				return ret;
			};

		do {
			auto child = stack.back();
			auto [parent, braNum] = childToParentWithBraNum[child];

			auto name = id(*getObj(child));
			if (not name)name = U"empty";

			if (braNum - 1 != 0) //自分以外の兄弟がいる
			{
				size_t rowDiff = 0;
				auto currentRow = result.count(U"\n");
				for (size_t i = 0; i < branchState.size(); ++i)
				{
					const auto [upperParent, progress] = branchState[i].first;
					const auto [branchPos, state] = branchState[i].second;

					if (currentRow == 0 and parent != upperParent)continue;

					auto hasPadding = spacePadding(branchPos.x);
					//親の行からいくつずれてるか
					rowDiff = currentRow - branchPos.y;
					//今処理している子の親でない、奇数行もしくはまだ描いてないなら
					if ((parent != upperParent) or currentRow % 2)
					{
						if (hasPadding)result += U" |";
						continue;
					}
					//自分が兄弟の中で何番目に描かれたか
					auto order = progress + 1;
					++branchState[i].first.second;
					//描かれたのが一番最後なら
					if (order == braNum)
					{
						result += U" +- " + name;
						branchState.pop_back();
						break;
					}
					//一番最初なら「+-」そうでなければ「|-」
					result += (order == 1 ? U" +- " : U" |- ") + name;
				}
				//奇数行だったらここで次の行へ
				if (currentRow % 2) { result += U"\n"; continue; }
			}
			else //兄弟がいない
			{
				result += U" -- " + name;
				branchState.pop_back();
			}
			stack.pop_back();
			if (auto next = pushToContainer(children(*getObj(child))))
			{
				next.each(
					[&](const auto& v)
					{
						childToParentWithBraNum[v] = std::pair<size_t, size_t>{ child,next.size() };
					}
				);
				stack.append(next);
				branchState.push_back({ { child, 0 }, { { lastLineLength(),result.count(U"\n")}, branch} });
			}
			else
			{
				result += U"\n";
			}
		} while (stack);

		return result;
	}
}
