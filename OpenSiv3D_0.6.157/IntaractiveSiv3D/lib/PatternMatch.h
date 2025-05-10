#pragma once
#include<Siv3D.hpp>

namespace
{
	template<class T>
	class ExcludableIteratorsWrapper
	{
		size_t i = 0;
		Array<std::pair<ExcludableIteratorsWrapper, ExcludableIteratorsWrapper>> excludes;
	public:
		using iterator_t = typename String::const_iterator;

		ExcludableIteratorsWrapper() = default;
		ExcludableIteratorsWrapper(iterator_t itr) { (*this) = itr; }
		ExcludableIteratorsWrapper operator = (iterator_t itr) { this->itr = itr; return *this; }

		void exclude(ExcludableIteratorsWrapper start, ExcludableIteratorsWrapper end)
		{
			if (start <= (*this) and (*this) <= end)
			{
				itr = end.itr;
				return;
			}

			//除外範囲の結合を試みる
			if (not excludes.isEmpty())
			{
				auto& back = excludes.back();
				//除外範囲の結合
				if (back.first <= start and start <= back.second)
				{
					if (back.second < end)back.second = end;
					return;
				}
			}

			//結合できないのであれば追加
			excludes.push_back({ start,end });
		}

		ExcludableIteratorsWrapper operator ++()
		{
			++itr;

			if (excludes.size() <= i)return *this;

			auto& [start, end] = excludes[i];
			//除外箇所だったらスキップする
			if (start.itr == itr)
			{
				itr = end.itr;
				++i;
			}
			return *this;
		}

		std::strong_ordering operator<=>(const ExcludableIteratorsWrapper& other) const
		{
			return itr <=> other.itr;
		}
		bool operator ==(const ExcludableIteratorsWrapper& other) const
		{
			return itr == other.itr;
		}

		auto operator*() { return *itr; }

		iterator_t itr;
	};

	using StringIterator = ExcludableIteratorsWrapper<String>;

	String GetString(StringIterator start, StringIterator end)
	{
		String result;

		for (; start != end; ++start)
		{
			result += *start;
		}
		return result;
	}

	//using StringIterator = typename String::const_iterator;
}

namespace ys
{
	class Char
	{
		std::function<bool(const char32_t)> m_judgeFunction;
	public:
		Char(const std::function<bool(const char32_t)>& judgeFunction);

		Char(const char32_t c);

		Char();

		Char(const Char& other);

		std::function<bool(const char32_t)> getJudger()const;

		Char operator || (const Char& other)const;

		Char operator && (const Char& other)const;

		bool operator ()(const char32_t c) const;
	};
	//大文字　小文字
	const Char caps{ [](const char32_t c) {return U'A' <= c and c <= U'Z'; } };
	const Char lower{ [](const char32_t c) {return U'a' <= c and c <= U'z'; } };
	//アルファベット
	const Char alp{ caps || lower };
	//数字
	const Char num{ [](const char32_t c) {return U'0' <= c and c <= U'9'; } };
	//ワイルドカード　なんでもOK
	const Char wild = { [](const char32_t) {return true; } };

	class Pattern;
	using PatternList = std::list<Pattern>;

	enum class MetaTag
	{
		normal,
		ignore,//判定はされるがtreeに乗らないパターン
		expand,//自分を削除し子を展開する
	};

	class Holder;

	struct Pattern
	{
	public:
		class Tree
		{
		public:
			struct Node
			{
				size_t parent;
				size_t nodeId;
				String pattern = U"";
				Array<size_t> nexts;
				String str()const;
			};
		private:
			Array<Node> m_nodes;
		public:
			Tree(const Array<Node>& nodes);

			Array<Node> getNodes()const;
			String showChildren()const;
		};
	private:
		using PatternType = std::variant<
			std::unique_ptr<Pattern>,
			Char,
			std::function<bool(StringIterator&, const StringIterator&)>
		>;
		Array<PatternType> m_patterns;

		Tree childTree(const size_t parent)const;

		String tag = U"";
	public:
		Holder* holder = nullptr;

		Tree tree()const;

		MetaTag metaTag;

		String getTag()const;
		void setTag(const String& tag);

		/// @brief 反復可能条件
		std::function<bool(size_t)> iteration = [](size_t i) {return i <= 1; };
		/// @brief 必要な反復回数条件
		std::function<bool(size_t)> requierd = [](size_t i) {return 0 < i; };

		Pattern() = default;
		Pattern(Holder* holder);
		Pattern(const Char& c);
		Pattern(const std::function<bool(StringIterator&, const StringIterator&)>& judgeFunction);

		Pattern(const Pattern& other);
		Pattern& operator=(const Pattern& other);

		explicit Pattern(PatternList&& list);
		Pattern& operator=(PatternList&& list);

		Pattern& setMetaTag(const MetaTag tag);

		/// @brief 文字列の始端からパターンが一致するか判定 後ろは完全一致じゃなくて良い
		/// @param itr 判定する文章のイテレータ
		/// @param end 判定する文章のイテレータ
		/// @return マッチする場合true
		bool partiallyMatch(StringIterator& itr, const StringIterator& end) const;

		/// @brief 文章の終端までパターンがマッチするか判定
		/// @param itr 判定する文章のイテレータ
		/// @param end 判定する文章のイテレータ
		/// @return マッチする場合true
		bool operator ()(StringIterator& itr, const StringIterator& end) const;

		Pattern operator[](size_t i)const;
		Pattern get(const String& tag)const;

		Pattern& add(const Pattern& other);
		Pattern& add(const Char& c);
		Pattern& add(const std::function<bool(StringIterator&, const StringIterator&)>& judgeFunction);
	};

	struct Holder
	{
		Pattern* hold = nullptr;
	};

	class PlaceHolder
	{
		Array<Pattern*> patternStore;
		HashTable<size_t, Holder*> holderTable;
	public:
		~PlaceHolder();
		void replace(size_t id, const Pattern& pattern);
		void replace(size_t id, Pattern* pattern);
		Pattern operator()(size_t id);
	};

	struct SetHolder
	{
		SetHolder(
			PlaceHolder& holder,
			Array<std::pair<size_t, std::variant<Pattern,Pattern*>>>
		);
	};

	Pattern operator || (const Pattern& left, const Pattern& right);
	Pattern operator || (const Pattern& left, const Char& right);
	Pattern operator || (const Char& left, const Pattern& right);
	Pattern operator || (PatternList&& left, const Pattern& right);
	Pattern operator || (PatternList&& left, const Char& right);
	Pattern operator || (const Pattern& left, PatternList&& right);
	Pattern operator || (const Char& left, PatternList&& right);
	Pattern operator || (PatternList&& left, PatternList&& right);

	Pattern operator && (const Pattern& left, const Pattern& right);
	Pattern operator && (const Pattern& left, const Char& right);
	Pattern operator && (const Char& left, const Pattern& right);
	Pattern operator && (PatternList&& left, const Pattern& right);
	Pattern operator && (PatternList&& left, const Char& right);
	Pattern operator && (const Pattern& left, PatternList&& right);
	Pattern operator && (const Char& left, PatternList&& right);
	Pattern operator && (PatternList&& left, PatternList&& right);

	Pattern operator! (const Pattern& pattern);
	Char operator! (const Char& ch);
	Pattern operator! (PatternList&& list);

	PatternList operator + (const Pattern& left, const Pattern& right);
	PatternList operator + (const Char& left, const Pattern& right);
	PatternList operator + (const Pattern& left, const Char& right);
	PatternList operator + (const Char& left, const Char& right);

	PatternList&& operator + (const Pattern& left, PatternList&& right);
	PatternList&& operator + (PatternList&& left, const Pattern& right);
	PatternList&& operator + (const Char& left, PatternList&& right);
	PatternList&& operator + (PatternList&& left, const Char& right);
	PatternList&& operator + (PatternList&& left, PatternList&& right);

	PatternList operator * (const Pattern& pattern, size_t time);
	PatternList operator * (const Char& c, size_t time);
	PatternList operator * (size_t time, const Pattern& pattern);
	PatternList operator * (size_t time, const Char& c);

	PatternList operator * (const PatternList& pattern, size_t time);
	PatternList operator * (size_t time, const PatternList& pattern);

	Pattern operator < (const Pattern& pattern, size_t i);
	Pattern operator <= (const Pattern& pattern, size_t i);
	Pattern operator < (size_t i, const Pattern& pattern);
	Pattern operator <= (size_t i, const Pattern& pattern);

	Pattern operator < (PatternList&& pattern, size_t i);
	Pattern operator <= (PatternList&& pattern, size_t i);
	Pattern operator < (size_t i, PatternList&& pattern);
	Pattern operator <= (size_t i, PatternList&& pattern);

	Pattern operator == (const Pattern& pattern, size_t i);
	Pattern operator == (PatternList&& pattern, size_t i);

	Pattern operator / (Pattern pattern, const MetaTag tag);
	Pattern operator / (const Char& c, const MetaTag tag);
	Pattern operator / (PatternList&& pattern, const MetaTag tag);

	Pattern operator / (const Pattern& pattern, const String& tag);
	Pattern operator / (const Char& c, const String& tag);
	Pattern operator / (PatternList&& pattern, const String& tag);

	template<class Pat>
	Pattern operator/ (Pat&& pattern, const std::string& tag)
	{
		auto converted = String{ tag.begin(),tag.end() };
		return std::forward<Pat>(pattern) / converted;
	}
	template<class Pat>
	Pattern operator/ (Pat&& pattern, const char* tag)
	{
		return std::forward<Pat>(pattern) / std::string(tag);
	}

	Pattern s(const std::string& str);
	Pattern s(const String& str);

	Char c(const char c);
	Char c(const char32_t c);

	Pattern many(const Pattern& pattern);
	Pattern many(PatternList&& pattern);

	Pattern emp();

	class PatternTree
	{
		StringIterator m_itr;
		StringIterator m_end;

		std::shared_ptr<String> m_str = nullptr;

		Array<PatternTree> m_children;

		HashTable<String, Array<size_t>> m_groups;

		Optional<bool> isEnable = none;

		void buildTree(PatternTree& tree);
	public:
		String tag=U"";

		PatternTree() = default;
		PatternTree(const std::string& str);
		PatternTree(const String& str);
		PatternTree(std::shared_ptr<String> str, const StringIterator& itr, const StringIterator& end);

		PatternTree(const PatternTree& other);
		PatternTree& operator = (const PatternTree& other);

		PatternTree search(PatternList&& pattern);
		PatternTree search(const Pattern& pattern);
		
		Array<PatternTree> children()const;
		Array<size_t> indexOf(const std::string& tag)const;
		Array<size_t> indexOf(const String& tag)const;

		operator bool()const;

		PatternTree get(const std::string& tag, size_t i = 0)const;
		PatternTree get(const String& tag, size_t i = 0)const;
		PatternTree get(const char* str, size_t i = 0)const;
		PatternTree get(const char32_t* str, size_t i = 0)const;

		Array<PatternTree> group(const std::string& tag)const;
		Array<PatternTree> group(const String& tag)const;
		Array<PatternTree> group(const char* str)const;
		Array<PatternTree> group(const char32_t* str)const;
		
		PatternTree operator [](size_t i)const;

		PatternTree& resetAt(size_t i, const PatternTree& tree);
		PatternTree& reset(const std::string& tag, const PatternTree& tree, size_t i = 0);
		PatternTree& reset(const String& tag, const PatternTree& tree, size_t i = 0);
		
		size_t size()const;

		void setIterator(const StringIterator& itr, const StringIterator& end);

		String str()const;

		PatternTree match(PatternList&& pattern);
		PatternTree match(const Pattern& pattern);
	};
}

namespace ys
{
	String visualize(const PatternTree& tree);
}
