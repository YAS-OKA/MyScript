#pragma once
#include"../Util/Util.h"

//データセーバーをゲットする
#define GET(x,ds) auto x##Ds = ds.getDataSaver(U#x)

using DsTable = HashTable<String, class DataSaver>;

class DataSaver
{
	void setArgs()
	{
		bool isGroundFloar=true;
		size_t openBlacketCount = 0;
		for (const auto& s : data)
		{
			if (s == U'[')
			{
				isGroundFloar = false;
			}
			else if (not isGroundFloar)
			{
				if (s == U'{')
				{
					++openBlacketCount;
				}
				else if (s == U'}' and --openBlacketCount == 0)
				{
					isGroundFloar = true;
				}
			}
			else
			{
				arg << s;
			}
		}
	}

	HashSet<String> getOption(StringView key)
	{
		HashSet<String> ret{};
		for (const auto& elm : util::slice(String(key).split(U','), 1))
		{
			ret.emplace(elm);
		}
		return ret;
	}
	mutable size_t getArgCount = 0;
public:
	//データセーバ間の優先度
	size_t rank;
	DsTable children;
	String name, data;
	String arg;
	DataSaver() = default;

	DataSaver(StringView name, StringView data, bool resolveConfliction = true)
		:DataSaver(name, data, getOption(name), resolveConfliction)
	{}

	DataSaver(StringView name, StringView data, HashSet<String> option, bool resolveConfliction = true)
		:data(Escape(data, option))
	{
		this->name = name ? String(name).split(U',')[0] : name;
		setArgs();
		ConstractChild(this->data, resolveConfliction);
	}

	//変更または削除された要素にキーワードをつける
	//変更された内容->末尾に"@", 削除された内容->末尾に"-" をそれぞれ付与
	Optional<DataSaver> createDiff(const DataSaver& other)const;

	String getName()const;
	//データセーバーを補う
	void complement(const DataSaver& dataSaver)
	{
		//足りない引数を補う
		auto oppArg = dataSaver.getArray();
		for (size_t i = getArray().size(); i < oppArg.size(); i++)
		{
			addArg(oppArg[i]);
		}
		//足りない子DataSaverを補う
		for (const auto& oppChild : dataSaver.getDataSavers())
		{
			if (not children.contains(oppChild.name))
			{
				addChild(oppChild);
			}
			else
			{
				children[oppChild.name].complement(oppChild);
			}
		}
	}

	template<class T = StringView>
	void addArg(T elm)
	{
		arg += U"{}"_fmt(arg.isEmpty() ? Format(elm) : U"," + Format(elm));
	}

	void addChild(const DataSaver& ds)
	{
		addChild(ds.name, ds);
	}

	void addChild(StringView name,const DataSaver& ds)
	{
		String newData;
		newData.reserve(data.size());

		if (children.contains(name))
		{
			size_t dataSize = data.size();
			size_t nameSize = name.size();
			bool ignore = false;
			int32 depth = 0;
			//一致する名前のdsを削除
			for (size_t i = 0; i < dataSize; i++)
			{
				if (data[i] == U'{')
				{
					depth++;
				}

				if(data[i]==U'}')
				{
					depth--;

					if (depth == 0 and ignore)
					{
						ignore = false;
					}
				}
				//名前が一致したら無視する
				if (data[i] == U'[' and i + nameSize < dataSize and util::slice(data, i + 1, i + 1 + nameSize) == name)
				{
					ignore = true;
				}

				if(not ignore)newData[i] = data[i];
			}
		}
		children.emplace(name, ds);
		
		data += U"\n[{}]"_fmt(ds.name)+U"\n{" + ds.data + U"\n}";
	}

	static Array<String> GetOrderdList(const HashTable<String, DataSaver>& savers, bool removeSubscripts = false)
	{
		Array<String> list(savers.size());
		for (const auto& [key, v] : savers)
		{
			list[v.rank] = v.name;
			if (removeSubscripts)util::removeSubscripts(list[v.rank]);
		}
		return list;
	}

	static String preProcess(StringView data);

	void ConstractChild(String data,bool resolveConfliction = true)
	{
		children = LoadDataSaver(data,resolveConfliction);
	}

	template<class T=String>
	Optional<T> getOpt(size_t index, bool eval = true)const
	{
		String result;

		auto elements = getArray();
		if (elements.size() <= index)return none;

		if constexpr (std::is_same_v<T, String>)return elements[index];

		getArgCount = index;

		auto splitElements = elements[index].split(U',');
		for (const auto& elm : splitElements)
		{
			bool hasBigBlacket = false;
			String strValue=elm;

			if (elm.front() == U'(' and elm.back() == U')')
			{
				hasBigBlacket = true;
				size_t depth = 0;
				strValue.each([&](const auto& c) {
					if (depth == 0)hasBigBlacket = false;
					if (c == U'(')depth++;
					else if (c == U')')depth--;
				});
				if(hasBigBlacket)strValue = util::slice(strValue, 1, strValue.size()-1);
			}

			if constexpr (!std::is_same_v<T,bool>)
			{
				if (not eval or strValue == U"none")
				{
				}
				else if (auto valueOpt = EvalOpt(strValue)) {
					strValue = Format(*valueOpt);
				}
			}

			if (hasBigBlacket)
			{
				strValue = U"(" + strValue + U")";
			}

			result += strValue;
			result += U",";
		}

		if ((not result.isEmpty()) and result.back() == U',')result.pop_back();

		return ParseOpt<T>(result);
	}

	template<class T=String>
	Optional<T> getPackOpt(size_t index, size_t size, bool eval = true)const
	{
		String result;

		auto elements = getArray();
		if (elements.size() <= index or elements.size() < index + size)return none;

		if constexpr (std::is_same_v<T, String>)
		{
			String result;
			for (const auto& s : util::slice(elements, index, index + size))
			{
				result += s + U",";
			}
			if (result)result.pop_back();
			return result;
		}

		getArgCount = index + size;

		for (; index < getArgCount; index++) {
			auto splitElements = elements[index].split(U',');

			for (const auto& elm : splitElements)
			{
				bool hasBigBlacket = false;
				String strValue = elm;

				if (elm.front() == U'(' and elm.back() == U')')
				{
					hasBigBlacket = true;
					size_t depth = 0;
					strValue.each([&](const auto& c) {
						if (depth == 0)hasBigBlacket = false;
						if (c == U'(')depth++;
						else if (c == U')')depth--;
					});
					if (hasBigBlacket)strValue = util::slice(strValue, 1, strValue.size() - 1);
				}

				if constexpr (!std::is_same_v<T, bool>)
				{
					if (not eval or strValue == U"none")
					{
					}
					else if (auto valueOpt = EvalOpt(strValue)) {
						strValue = Format(*valueOpt);
					}
				}

				if (hasBigBlacket)
				{
					strValue = U"(" + strValue + U")";
				}

				result += strValue + U",";
			}
		}

		if (not result.isEmpty())result.pop_back();

		if (result.front() != U'(')result = U"(" + result;
		if (result.back() != U')')result += U")";

		return ParseOpt<T>(result);
	}

	template<class T=String>
	T get(size_t index, T default_ = T(), bool eval = true) const
	{
		if (auto op = getOpt<T>(index,eval))
		{
			return *op;
		}
		else
		{
			return default_;
		}
	}

	template<class T = String>
	T getPack(size_t index,size_t size, T default_ = T(), bool eval = true) const
	{
		if (auto op = getPackOpt<T>(index, size, eval))
		{
			return *op;
		}
		else
		{
			return default_;
		}
	}

	template<class T = String>
	T each(T default_ = T())const
	{
		auto ret = get<T>(getArgCount, default_);
		getArgCount++;
		return ret;
	}

	//argをarrayとして返す
	Array<String> getArray()const
	{		
		Array<String> result{U""};
		size_t depth = 0;
		for (size_t i = 0; i < arg.size(); i++)
		{
			auto& elm = arg[i];

			if (depth == 0 and elm == U',')
			{
				result << U"";
			}
			else
			{
				result.back() += elm;
			}

			if (elm == U'(')
			{
				depth++;
			}
			else if (0 < depth and elm == U')')
			{
				depth--;
			}
		}
		if ((not result.isEmpty()) and result.back() == U"")result.pop_back();

		return result;
	}

	Optional<DataSaver> getDataSaver(StringView name)const
	{
		if (children.contains(name))return children.at(name);
		else return none;
	}

	Optional<DataSaver> getDataSaver(StringView name, const DataSaver& defaultDataSaver, bool returnNoneResult = false)const
	{
		if (children.contains(name))
		{
			auto ret = getDataSaver(name);
			ret->complement(defaultDataSaver);
			return *ret;
		}
		else if(returnNoneResult)
		{
			return none;
		}
		else {
			return defaultDataSaver;
		}
	}

	DataSaver operator[](StringView name)const
	{
		if (children.contains(name))return children.at(name);
		else return DataSaver(name, U"");
	};

	Array<DataSaver> getDataSavers(Array<String> names = {}, bool removeSubscripts = false)const
	{
		Array<DataSaver> ret;
		//空だったらすべて取得
		if (not names)names = GetOrderdList(children, removeSubscripts);

		for (auto name : names)
		{
			if (auto opt = getDataSaver(name))
			{
				ret << *opt;
			}
		}

		return ret;
	}

	static String Escape(StringView text, HashSet<String> option = {})
	{
		String result;

		size_t depth = 0;

		bool inOneLineComentOut = false;

		for (size_t i = 0; i < text.size(); i++)
		{
			const auto& s = text[i];
			if (s == U'[')depth++;
			if (s == U'}' and depth > 0)depth--;

			if (i + 1 < text.size() and s == U'/' and text[i + 1]==U'/')
			{
				inOneLineComentOut = true;
				continue;
			}

			if (inOneLineComentOut and s != U'\n')continue;

			inOneLineComentOut = false;

			if (depth == 0)
			{
				if ((option.contains(U"l")) and s == U'\n')continue;
				if ((option.contains(U"t")) and s == U'\t')continue;
				if ((option.contains(U"s")) and s == U' ')continue;
			}

			result += s;
		}

		return result;
	}

	static DsTable LoadDataSaver(StringView originalText,bool resolveConfriction)
	{
		String text = preProcess(originalText);
		DsTable result;
		Array<String>nameList;
		size_t rank = 0;
		int32 start = -1;
		String name{};
		bool comp=false;
		int32 comentOutLayerCount = 0;
		int32 depth=0;	
		String cont{};		//中身　子に渡す
		size_t i = 0;

		while (i < text.size())
		{
			if (i + 1 < text.size())
			{
				if (comentOutLayerCount > 0)
				{
					if (text[i] == U'*' and text[i + 1] == U'/')
					{
						comentOutLayerCount--;
						++i;
						goto NEXT;
					}
				}
				else
				{
					if (text[i] == U'/' and text[i + 1] == U'*')
					{
						comentOutLayerCount++;
						++i;
					}
				}
			}
			//コメントアウトだったらここではじく
			if (comentOutLayerCount > 0)
			{
				goto NEXT;
			}

			//データセーバーの名前を取得
			if (not comp)
			{
				//start==-1のときはデータセーバーの{...}の中に入ってない
				if (start == -1 and text[i] == U'[')
				{
					start = i;
				}

				if (start != -1 and text[i] != U'[')
				{
					if (text[i] == U']')
					{
						start = -1;
						comp = true;
						depth = -1;
					}
					else {
						name << text[i];
					}
				}
			}
			else //データセーバーの内容を記録
			{
				//最初の'{'を見つけるまではここではじく
				if (depth == -1 and text[i] == U'{')
				{
					depth = 1;
					goto NEXT;
				}

				if (text[i] == U'{')
				{
					depth += 1;
				}
				else if (text[i] == U'}')
				{
					depth -= 1;
				}

				//内容の記録
				if (depth != 0)
				{
					cont << text[i];
					goto NEXT;
				}

				//depth==0なのでデータセーバーを作成
				auto ds = DataSaver(name, cont);
				if (resolveConfriction)
				{
					ds.name = util::resolveNameConflict(ds.name, nameList);
				}
				nameList << ds.name;
				ds.rank = rank;
				result.emplace(ds.name, ds);
				rank++;
				name = {};
				cont = {};
				comp = false;
			}

		NEXT:
			++i;
		}
		return result;
	}

	bool operator == (const DataSaver& other)const
	{
		return data == other.data;
	}
};
