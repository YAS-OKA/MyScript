#include"../stdafx.h"
#include"Util.h"
#include<Windows.h>

namespace util
{
	String str(const char* s)
	{
		std::string st{ s };
		return String(st.begin(), st.end());
	}

	String resolveNameConflict(const String& name,Array<String> nameArray)
	{
		HashSet<int32> dNums;
		const auto& len = name.length();
		//かぶってる名前があったらdNumsに0を追加
		//「かぶってる名前＋(n)」ですでに使われてるnをdNumsに追加。
		for (const auto& str : nameArray)
		{ 
			const auto& slen = str.length();
			//strが「name+U"..."」の形になっているか
			if (slen >= len and util::strEqual(str, 0, len, name))
			{
				//strとnameの長さが一致してたらstr=「name」のはず
				if (slen == len)
				{
					dNums.emplace(0);
				}
				//strが「name+U"(..."」の形になっているか
				else if (str[len] == U'(' and slen >= len + 3)
				{
					auto closePos = str.indexOf(U")", len + 2);
					//strが「name+U"(...)"」の形になっているか
					if (not (closePos == String::npos))
					{
						//strが「name+U"(数字)"」の形になっているか
						if (auto num = ParseOpt<size_t>(util::slice(str, len + 1, closePos)))
						{
							//数字を追加
							dNums.emplace(*num);
						}
					}
				}
			}
		}
		size_t num = 0;
		//かぶらない数字を探す
		while (dNums.contains(num))num++;
		//オリジナルのままでもかぶらない
		if (num == 0)return name;
		//オリジナルに数字を添える
		else return name + U"({})"_fmt(num);
	}

	String removeSubscripts(const String& name)
	{
		if (name.back() == U')')
		{
			auto ind = name.lastIndexOf(U'(');
			if(ind!=String::npos) return util::slice(name, 0, ind);
		}

		return name;
	}

	String getSubscripts(const String& name)
	{
		String ret;
		if (name.back() == U')')
		{
			auto ind = name.lastIndexOf(U'(');
			if (ind != String::npos)return util::slice(name, ind, name.size() - 1);
		}
		return ret;
	}

	StopMax::StopMax(double max, double value)
		:value(value), max(max) {}

	void StopMax::add(double additional)
	{
		value += additional;
		value = value > max ? max : value;
	}

	bool StopMax::additionable()const
	{
		return value < max;
	}

	EditFile::EditFile(FilePathView path)
	{
		//auto sPath = str(path);
		////存在しなかったら作成
		//if (not FileSystem::IsFile(path)) {
		//	std::ofstream(sPath);
		//}
		//auto f = std::fstream(sPath);
		////オープンできたらfileにセット
		//if (f)
		//{
		//	file = uPtr<std::fstream>(new std::fstream(std::move(f)));
		//}
	}

	void EditFile::overwrite(StringView text)
	{
		if (not file)return;

		file->clear();
	}

	String EditFile::readAll()const
	{
		if (not file)return String{};//fileがセットされてなかったら

		std::string contents;
		std::string line;
		while (std::getline(*file, line)) {
			contents += line + "\n";
		}

		return String(contents.begin(), contents.end());
	}

	EditFile createFile(FilePath path)
	{
		return EditFile(path);
	}

	struct UnionFind::Impl
	{
		Array<size_t> parents;
		Array<size_t> size;

		Impl(size_t n)
			:parents(n),size(n,1)
		{
			for (size_t i = 0; i < parents.size(); ++i)
			{
				parents[i] = i;
			}
		}

		size_t unite(size_t x1, size_t x2)
		{
			x1 = find(x1), x2 = find(x2);

			if (x1 == size_t(-1) or x2 == size_t(-1))return -1;

			if (x1 != x2)
			{
				if (size[x1] < size[x2]) std::swap(x1, x2);

				parents[x2] = x1;//x2の親をx1にする
				size[x1] += size[x2];//x1にx2のサイズを加える
			}
			return x1;
		}

		size_t find(size_t x)
		{
			if (parents.size() <= x)return -1;
			if (parents[x] == x) return x;
			return (parents[x] = find(parents[x]));
		}
	};

	UnionFind::UnionFind(size_t n)
		:p_impl(std::make_shared<Impl>(n))
	{}

	size_t UnionFind::unite(size_t x1, size_t x2)
	{
		return p_impl->unite(x1, x2);
	}

	size_t UnionFind::find(size_t x)
	{
		return p_impl->find(x);
	}
}
