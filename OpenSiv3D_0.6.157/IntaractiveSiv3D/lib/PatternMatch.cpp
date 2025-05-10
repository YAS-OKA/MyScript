#include "../../stdafx.h"
#include "PatternMatch.h"
#include"../Util/Util.h"

ys::Char::Char(const std::function<bool(const char32_t)>& judgeFunction)
	:m_judgeFunction(judgeFunction) {}

ys::Char::Char(const char32_t c)
	:m_judgeFunction([=](const auto& c2) {return c == c2; }) {}

ys::Char::Char() :Char([](const char) {return true; }) {}

ys::Char::Char(const Char& other) :m_judgeFunction(other.m_judgeFunction) {}

std::function<bool(const char32_t)> ys::Char::getJudger() const
{
	return m_judgeFunction;
}

ys::Char ys::Char::operator||(const Char& other) const
{
	const auto judge1 = m_judgeFunction;
	const auto judge2 = other.m_judgeFunction;
	return Char([judge1, judge2](const char c) {return judge1(c) or judge2(c); });
}

ys::Char ys::Char::operator&&(const Char& other) const
{
	const auto judge1 = m_judgeFunction;
	const auto judge2 = other.m_judgeFunction;
	return Char([judge1, judge2](const char c) {return judge1(c) and judge2(c); });
}

bool ys::Char::operator()(const char32_t c) const { return m_judgeFunction(c); }

String ys::Pattern::Tree::Node::str() const
{
	return U"[{}]{}[{}]"_fmt(parent != size_t(-1) ? Format(parent) : U"top",
							pattern,
							nodeId);
}

ys::Pattern::Tree::Tree(const Array<Node>& nodes)
	:m_nodes(nodes)
{}

Array<ys::Pattern::Tree::Node> ys::Pattern::Tree::getNodes() const
{
	return m_nodes;
}

String ys::Pattern::Tree::showChildren() const
{
	String ret;
	for (const auto& node : getNodes())
	{
		ret += U"{}\n"_fmt(node.str());
	}
	return ret;
}

ys::Pattern::Tree ys::Pattern::childTree(const size_t parent) const
{
	using Node = Tree::Node;

	Array<Node> nodes{ Node{.parent = parent - 1,.nodeId = parent, .pattern = U"(pat){}"_fmt(tag),.nexts = {}}};
	size_t index = parent + 1;

	m_patterns.reverse_each(
		[&](const auto& pattern)
		{
			switch (pattern.index())
			{
			case 0: {
				auto childTree = std::get<std::unique_ptr<Pattern>>(pattern)->childTree(index);
				auto childNodes = childTree.getNodes();
				if (not childNodes.isEmpty())
				{
					nodes.front().nexts << index;
					childNodes.front().parent = parent;
				}
				nodes.append(childNodes);
				index += childNodes.size();
			}break;
			case 1: {
				nodes.front().nexts << index;
				nodes << Node{ .parent = parent,.nodeId = index,.pattern = U"(char)",.nexts = {} };
				++index;
			}break;
			default: {
				nodes.front().nexts << index;
				nodes << Node{ .parent = parent,.nodeId = index,.pattern = U"(logic)",.nexts = {} };
				++index;
			}break;
			};
		}
	);
	return Tree(nodes);
}

ys::Pattern::Tree ys::Pattern::tree() const
{
	return childTree(0);
}

String ys::Pattern::getTag() const
{
	if (tag.isEmpty() and holder)
	{
		return holder->hold->getTag();
	}
	return tag;
}

void ys::Pattern::setTag(const String& tag)
{
	this->tag = tag;
}

ys::Pattern::Pattern(Holder* holder)
	:holder(holder)
{}

ys::Pattern::Pattern(const Char& c)
{
	add(c);
}

ys::Pattern::Pattern(const std::function<bool(StringIterator&, const StringIterator&)>& judgeFunction)
{
	add(judgeFunction);
}

ys::Pattern::Pattern(const Pattern& other)
{
	(*this) = other;
}

ys::Pattern& ys::Pattern::operator=(const Pattern& other)
{
	iteration = other.iteration;
	requierd = other.requierd;
	tag = other.tag;
	metaTag = other.metaTag;
	holder = other.holder;
	m_patterns.clear();

	for (const auto& pattern : other.m_patterns)
	{
		switch (pattern.index())
		{
		case 0: {
			auto& patternPtr = std::get<std::unique_ptr<Pattern>>(pattern);
			add(*patternPtr);
		}break;
		case 1: {
			m_patterns << std::get<Char>(pattern);
		}break;
		default:
			m_patterns << std::get<std::function<bool(StringIterator&, const StringIterator&) >>(pattern);
		};
	}

	return *this;
}

ys::Pattern::Pattern(PatternList&& list)
{
	(*this) = std::move(list);
}

ys::Pattern& ys::Pattern::operator=(PatternList&& list)
{
	for (auto itr = list.begin(), end = list.end(); itr != end; ++itr)
	{
		add(*itr);
	}
	setMetaTag(MetaTag::expand);
	return *this;
}

ys::Pattern& ys::Pattern::setMetaTag(const MetaTag tag)
{
	metaTag=tag;
	return *this;
}

namespace
{
	class ResultCollector
	{
	public:
		struct NodeInfo
		{
			String patternTag;
			StringIterator start;
			StringIterator end;
			ys::MetaTag tag;
		};

		struct Result
		{
			Result* parent = nullptr;
			bool isMatch;
			NodeInfo node;
			Array<Result*> children;
			size_t contIndex = 0;//m_resultContainerの中で何番目か
			size_t index = -1;//兄弟の中で何番目
		};
	private:
		Array<std::unique_ptr<Result>> m_resultContainer{};
	public:
		Result* result = nullptr;

		ResultCollector();
		~ResultCollector();

		bool enable();

		void removeChildren(size_t parentCondIndex);
		void next();
		void pre();
	};

	Array<ResultCollector*> resultCollectorsStack{};

	ResultCollector::ResultCollector()
	{
		next();
	}

	ResultCollector::~ResultCollector()
	{
		m_resultContainer.clear();
		result = nullptr;
	}

	bool ResultCollector::enable()
	{
		return not m_resultContainer.empty();
	}

	void ResultCollector::removeChildren(size_t parentContIndex)
	{
		m_resultContainer.erase(m_resultContainer.begin() + parentContIndex + 1, m_resultContainer.end());
		result = m_resultContainer[parentContIndex].get();
		result->children.clear();
	}

	void ResultCollector::next()
	{
		Result* parent = result;
		m_resultContainer.push_back(std::unique_ptr<Result>{new Result});
		result = m_resultContainer.back().get();
		result->contIndex = m_resultContainer.size() - 1;
		if (parent)
		{
			result->index = parent->children.size();
			parent->children << result;
		}
		result->parent = parent;
	}

	void ResultCollector::pre()
	{
		result = result->parent;
	}

	//begin
	// https://qiita.com/benikabocha/items/e943deb299d0f816f161#utf-32-%E3%81%8B%E3%82%89-utf-8
	int GetU8ByteCount(char ch) {
		if (0 <= uint8_t(ch) && uint8_t(ch) < 0x80) {
			return 1;
		}
		if (0xC2 <= uint8_t(ch) && uint8_t(ch) < 0xE0) {
			return 2;
		}
		if (0xE0 <= uint8_t(ch) && uint8_t(ch) < 0xF0) {
			return 3;
		}
		if (0xF0 <= uint8_t(ch) && uint8_t(ch) < 0xF8) {
			return 4;
		}
		return 0;
	}
	bool IsU8LaterByte(char ch) {
		return 0x80 <= uint8_t(ch) && uint8_t(ch) < 0xC0;
	}
	bool ConvChU8ToU32(const std::array<char, 4>& u8Ch, char32_t& u32Ch) {
		int numBytes = GetU8ByteCount(u8Ch[0]);
		if (numBytes == 0) {
			return false;
		}
		switch (numBytes) {
		case 1:
			u32Ch = char32_t(uint8_t(u8Ch[0]));
			break;
		case 2:
			if (!IsU8LaterByte(u8Ch[1])) {
				return false;
			}
			if ((uint8_t(u8Ch[0]) & 0x1E) == 0) {
				return false;
			}

			u32Ch = char32_t(u8Ch[0] & 0x1F) << 6;
			u32Ch |= char32_t(u8Ch[1] & 0x3F);
			break;
		case 3:
			if (!IsU8LaterByte(u8Ch[1]) || !IsU8LaterByte(u8Ch[2])) {
				return false;
			}
			if ((uint8_t(u8Ch[0]) & 0x0F) == 0 &&
				(uint8_t(u8Ch[1]) & 0x20) == 0) {
				return false;
			}

			u32Ch = char32_t(u8Ch[0] & 0x0F) << 12;
			u32Ch |= char32_t(u8Ch[1] & 0x3F) << 6;
			u32Ch |= char32_t(u8Ch[2] & 0x3F);
			break;
		case 4:
			if (!IsU8LaterByte(u8Ch[1]) || !IsU8LaterByte(u8Ch[2]) ||
				!IsU8LaterByte(u8Ch[3])) {
				return false;
			}
			if ((uint8_t(u8Ch[0]) & 0x07) == 0 &&
				(uint8_t(u8Ch[1]) & 0x30) == 0) {
				return false;
			}

			u32Ch = char32_t(u8Ch[0] & 0x07) << 18;
			u32Ch |= char32_t(u8Ch[1] & 0x3F) << 12;
			u32Ch |= char32_t(u8Ch[2] & 0x3F) << 6;
			u32Ch |= char32_t(u8Ch[3] & 0x3F);
			break;
		default:
			return false;
		}

		return true;
	}
	//end
}

bool ys::Pattern::partiallyMatch(StringIterator& itr, const StringIterator& end) const
{
	size_t index = 0;

	ResultCollector* collector = nullptr;

	if (not resultCollectorsStack.empty())
	{
		collector = resultCollectorsStack.back();
	}

	while (index < m_patterns.size())
	{
		switch (m_patterns[index].index())
		{
		case 0: {
			auto pattern = std::get<std::unique_ptr<Pattern>>(m_patterns[index]).get();

			auto start = itr;
			bool isMatch = false;

			if (collector)collector->next();

			isMatch = (*pattern)(itr, end);

			if (collector)
			{
				auto result = collector->result;
				result->isMatch = isMatch;
				result->node.patternTag = pattern->getTag();
				result->node.tag = pattern->metaTag;
				result->node.start = start;
				result->node.end = itr;
				collector->pre();
			}

			if (not isMatch)return false;
		}break;
		case 1: {
			auto& pattern = std::get<Char>(m_patterns[index]);
			bool isMatch = itr != end and pattern(*itr);

			if (not isMatch)return false;

			++itr;
		}break;
		case 2:
			auto& pattern = std::get<std::function<bool(StringIterator&, const StringIterator&)>>(m_patterns[index]);

			auto start = itr;
			bool isMatch = pattern(itr, end);

			if (not isMatch)return false;
		}

		++index;
	}
	//パターンが含まれる場合この条件を満たす
	return m_patterns.size() <= index;
}

bool ys::Pattern::operator()(StringIterator& itr, const StringIterator& end) const
{
	//holderがある場合はholderがこのパターンの実体とする
	if (holder)
	{
		String preTag = holder->hold->tag;
		holder->hold->tag = getTag();
		auto result = (*holder->hold)(itr, end);
		holder->hold->tag = preTag;
		return result;
	}

	size_t count = 0;

	while (iteration(count + 1))
	{
		if (not partiallyMatch(itr, end))break;

		++count;

		if (itr == end)break;
	}

	return requierd(count);
}

ys::Pattern ys::Pattern::operator[](size_t i) const
{
	auto& pattern = m_patterns[i];
	switch (pattern.index())
	{
	case 0: return *std::get<0>(pattern).get();
	case 1: return Pattern{ std::get<1>(pattern) };
	case 2: return Pattern{ std::get<2>(pattern) };
	};
}

ys::Pattern ys::Pattern::get(const String& tag) const
{
	return Pattern();
}

ys::Pattern& ys::Pattern::add(const Pattern& other)
{
	m_patterns << std::make_unique<Pattern>(other);
	return *this;
}

ys::Pattern& ys::Pattern::add(const Char& c)
{
	m_patterns << c;
	return *this;
}

ys::Pattern& ys::Pattern::add(const std::function<bool(StringIterator&, const StringIterator&)>& judgeFunction)
{
	m_patterns << judgeFunction;
	return *this;
}

ys::Pattern ys::operator||(const Pattern& left, const Pattern& right)
{
	Pattern ret{
		[left,right](StringIterator& itr,const StringIterator& end)
		{
			ResultCollector::Result* currentResult = nullptr;
			ResultCollector* collector=nullptr;
			if (not resultCollectorsStack.isEmpty())
			{
				collector = resultCollectorsStack.back();
				collector->next();
				currentResult = collector->result;
			}
			
			auto tmp = itr;
			auto result1 = left(itr, end);

			if (result1)
			{
				if (collector)
				{
					auto result = collector->result;
					result->isMatch = true;
					result->node.patternTag = left.getTag();
					result->node.tag = left.metaTag;
					result->node.start = tmp;
					result->node.end = itr;
					collector->pre();
				}
				return true;
			}

			//収集した情報を破棄
			if (currentResult)collector->removeChildren(currentResult->contIndex);

			itr = tmp;
			auto result2 = right(itr, end);

			if (collector)
			{
				auto result = collector->result;
				result->isMatch = result2;
				result->node.patternTag = right.getTag();
				result->node.tag = right.metaTag;
				result->node.start = tmp;
				result->node.end = itr;
				collector->pre();
			}

			if (result2)return true;
			return false;
		}
	};
	ret.setMetaTag(MetaTag::expand);
	return ret;
}

ys::Pattern ys::operator||(const Pattern& left, const Char& right)
{
	return left || Pattern{ right };
}

ys::Pattern ys::operator||(const Char& left, const Pattern& right)
{
	return Pattern{ left } || right;
}

ys::Pattern ys::operator||(PatternList&& left, const Pattern& right)
{
	return Pattern(std::move(left)) || right;
}

ys::Pattern ys::operator||(PatternList&& left, const Char& right)
{
	return std::move(left) || Pattern(right);
}

ys::Pattern ys::operator||(const Pattern& left, PatternList&& right)
{
	return left || Pattern(std::move(right));
}

ys::Pattern ys::operator||(const Char& left, PatternList&& right)
{
	return Pattern(left) || std::move(right);
}

ys::Pattern ys::operator||(PatternList&& left, PatternList&& right)
{
	return Pattern{ std::move(left) } || Pattern{ std::move(right) };
}

ys::Pattern ys::operator&&(PatternList&& left, const Pattern& right)
{
	return Pattern(std::move(left)) && right;
}

ys::Pattern ys::operator&&(PatternList&& left, const Char& right)
{
	return std::move(left) && Pattern(right);
}

ys::Pattern ys::operator&&(const Pattern& left, PatternList&& right)
{
	return left && Pattern(std::move(right));
}

ys::Pattern ys::operator&&(const Char& left, PatternList&& right)
{
	return Pattern(left) && std::move(right);
}

ys::Pattern ys::operator&&(const Pattern& left, const Pattern& right)
{
	//||と違い、leftの条件を軽視し、rightの方を重要な条件とみなす
	Pattern ret{
		[left,right](StringIterator& itr,const StringIterator& end) {
			ResultCollector::Result* currentResult = nullptr;
			ResultCollector* collector = nullptr;
			if (not resultCollectorsStack.isEmpty())
			{
				collector = resultCollectorsStack.back();
				collector->next();
				currentResult = collector->result;
			}
			auto tmp = itr;
			auto result1 = left(tmp, end);

			if (not result1)
			{
				if (collector)
				{
					auto result = collector->result;
					result->isMatch = false;
					result->node.patternTag = left.getTag();
					result->node.start = tmp;
					result->node.end = itr;
					collector->pre();
				}
				return false;
			}

			//収集した情報を破棄
			if (currentResult) collector->removeChildren(currentResult->contIndex);

			tmp = itr;
			auto result2 = right(tmp, end);

			if (collector)
			{
				auto result = collector->result;
				result->isMatch = result2;
				result->node.patternTag = left.getTag();
				result->node.start = tmp;
				result->node.end = itr;
				collector->pre();
			}

			if (not result2)return false;

			itr = tmp;
			return true;
		}
	};
	ret.setMetaTag(MetaTag::expand);
	return ret;
}

ys::Pattern ys::operator&&(const Pattern& left, const Char& right)
{
	return left && Pattern{ right };
}

ys::Pattern ys::operator&&(const Char& left, const Pattern& right)
{
	return Pattern{ left } && right;
}

ys::Pattern ys::operator&&(PatternList&& left, PatternList&& right)
{
	return Pattern{ std::move(left) } && Pattern{ std::move(right) };
}

ys::Pattern ys::operator!(const Pattern& pattern)
{
	return Pattern{ [pattern](StringIterator& itr,const StringIterator& end) {return not pattern(itr,end); } };
}

ys::Char ys::operator!(const Char& ch)
{
	return Char{ [judger = ch.getJudger()](char c) {return not judger(c); } };
}

ys::Pattern ys::operator!(PatternList&& list)
{
	return !Pattern{ std::move(list) };
}

ys::PatternList ys::operator+(const Pattern& left, const Pattern& right)
{
	return PatternList{ {left,right} };
}

ys::PatternList ys::operator+(const Char& left, const Pattern& right)
{
	return Pattern{ left } + right;
}

ys::PatternList ys::operator+(const Pattern& left, const Char& right)
{
	return left + Pattern{ right };
}

ys::PatternList ys::operator+(const Char& left, const Char& right)
{
	return Pattern{ left } + Pattern{ right };
}

ys::PatternList&& ys::operator+(const Pattern& left, PatternList&& right)
{
	right.push_front(left);
	return std::move(right);
}

ys::PatternList&& ys::operator+(PatternList&& left, const Pattern& right)
{
	left.push_back(right);
	return std::move(left);
}

ys::PatternList&& ys::operator+(const Char& left, PatternList&& right)
{
	return Pattern{ left } + std::move(right);
}

ys::PatternList&& ys::operator+(PatternList&& left, const Char& right)
{
	return std::move(left) + Pattern{ right };
}

ys::PatternList&& ys::operator+(PatternList&& left, PatternList&& right)
{
	left.splice(left.end(), std::move(right));
	return std::move(left);
}

ys::PatternList ys::operator*(const Pattern& pattern, size_t time)
{
	PatternList result;
	for (size_t i = 0; i < time; ++i)result.push_back(pattern);
	return result;
}

ys::PatternList ys::operator*(const Char& pattern, size_t time)
{
	return Pattern{ pattern }*time;
}

ys::PatternList ys::operator*(size_t time, const Pattern& pattern)
{
	return pattern * time;
}

ys::PatternList ys::operator*(size_t time, const Char& pattern)
{
	return pattern * time;
}

ys::PatternList ys::operator*(const PatternList& pattern, size_t time)
{
	PatternList result;

	for (size_t i = 0; i < time; ++i)
	{
		for (auto itr = pattern.begin(), end = pattern.end(); itr != end; ++itr)
		{
			result.push_back(*itr);
		}
	}

	return result;
}

ys::PatternList ys::operator*(size_t time, const PatternList& pattern)
{
	return pattern * time;
}

ys::Pattern ys::operator<(const Pattern& pattern, size_t m)
{
	Pattern result = pattern;
	result.iteration = [m](size_t i) {return i < m; };
	return result;
}

ys::Pattern ys::operator<=(const Pattern& pattern, size_t m)
{
	Pattern result = pattern;
	result.iteration = [m](size_t i) {return i <= m; };
	return result;
}

ys::Pattern ys::operator<(size_t m, const Pattern& pattern)
{
	Pattern result = pattern;
	result.requierd = [m](size_t i) {return m < i; };
	return result;
}

ys::Pattern ys::operator<=(size_t m, const Pattern& pattern)
{
	Pattern result = pattern;
	result.requierd = [m](size_t i) {return m <= i; };
	return result;
}

ys::Pattern ys::operator<(PatternList&& pattern, size_t i)
{
	return Pattern(std::move(pattern)) < i;
}

ys::Pattern ys::operator<=(PatternList&& pattern, size_t i)
{
	return Pattern(std::move(pattern)) <= i;
}

ys::Pattern ys::operator<(size_t i, PatternList&& pattern)
{
	return i < Pattern(std::move(pattern));
}

ys::Pattern ys::operator<=(size_t i, PatternList&& pattern)
{
	return i < Pattern(std::move(pattern));
}

ys::Pattern ys::operator==(const Pattern& pattern, size_t n)
{
	Pattern result = pattern;
	result.iteration = [n](size_t i) {return i <= n; };
	result.requierd = [n](size_t i) {return i == n; };
	return result;
}

ys::Pattern ys::operator==(PatternList&& pattern, size_t i)
{
	return Pattern(std::move(pattern)) == i;
}

ys::Pattern ys::operator/(Pattern pattern, const MetaTag tag)
{
	return pattern.setMetaTag(tag);
}

ys::Pattern ys::operator/(const Char& c, const MetaTag tag)
{
	return Pattern{ c } / tag;
}

ys::Pattern ys::operator/(PatternList&& pattern, const MetaTag tag)
{
	return Pattern{ std::move(pattern) } / tag;
}

ys::Pattern ys::operator/(const Pattern& pattern, const String& tag)
{
	Pattern result = pattern;
	result.setTag(tag);
	return result;
}

ys::Pattern ys::operator/(const Char& c, const String& tag)
{
	return Pattern{ c } / tag;
}

ys::Pattern ys::operator/(PatternList&& pattern, const String& tag)
{
	return Pattern(std::move(pattern)) / tag;
}

ys::Pattern ys::s(const std::string& str)
{
	return s(String{ str.begin(),str.end() });
}

ys::Pattern ys::s(const String& str)
{
	Pattern result;
	for (size_t i = 0; i < str.size(); ++i)
	{
		result.add(Char{ str[i] });
	}
	return result;
}
ys::Char ys::c(const char c)
{
	char32_t converted;
	ConvChU8ToU32({ c }, converted);
	return c;
}

ys::Char ys::c(const char32_t c)
{
	return Char([c](const char32_t inp) {return inp == c; });
}

ys::Pattern ys::many(const Pattern& pattern)
{
	Pattern result{ pattern };
	//反復は何回でも可能
	result.iteration = [](size_t) {return true; };
	//反復条件
	result.requierd = [](size_t) {return true; };
	//展開タグを追加
	result.metaTag = MetaTag::expand;
	return result;
}

ys::Pattern ys::many(PatternList&& pattern)
{
	return many(Pattern{ std::move(pattern) });
}

ys::Pattern ys::emp()
{
	Pattern result;
	//一度も反復を行わない
	result.iteration = [](size_t) {return false; };
	//無条件でtrue
	result.requierd = [](size_t) {return true; };
	//空タグを代入
	result.metaTag = MetaTag::ignore;
	return result;
}

template<enum ys::MetaTag _metaTag>
void proc(std::shared_ptr<String> str, ResultCollector::Result* ptr)
{
	Array<ResultCollector::Result*> stack{ ptr };

	while (stack)
	{
		auto& back = stack.back();
		stack.pop_back();

		if constexpr (_metaTag == ys::MetaTag::expand)
		{
			//expandならその場に子を展開する
			if (back->node.tag == ys::MetaTag::expand and back->node.patternTag.empty())
			{
				//親がいないなら圧縮を試みる
				if (not back->parent)
				{
					//圧縮する
					if (back->children.size() == 1)
					{
						auto child = back->children[0];
						back->children.clear();
						child->parent = back->parent;
						*back = *child;
						stack << back;
						continue;
					}
				}
				else {
					auto& parent = back->parent;
					size_t index = back->index;
					size_t childrenNum = back->children.size();

					if (childrenNum == 0)continue;

					back->children.each([&](auto& child) {child->parent = parent; });

					parent->children.erase(parent->children.begin() + index);
					
					for (size_t i = 0; i < childrenNum; ++i)
					{
						ResultCollector::Result* ptr = back->children[i];
						ptr->index = index++;
						parent->children.insert(parent->children.begin() + ptr->index, ptr);
					}
					//insertした分後ろの要素の.indexに列長の増加分を加算
					for (; index < parent->children.size(); ++index)
					{
						//childrenNumから消えた元の親の分を引くことで列長の増加分を足す
						parent->children[index]->index += childrenNum - 1;
					}
				}
			}
		}
		else if constexpr (_metaTag == ys::MetaTag::ignore)
		{
			//このパターンを削除
			if (back->node.tag == ys::MetaTag::ignore)
			{
				//自分の文字列を除外する（自分を含む親からも除外）
				{
					auto parent = back->parent;
					//親をさかのぼる
					while (parent)
					{
						parent->node.start.exclude(back->node.start, back->node.end);
						parent = parent->parent;
					}
				}
				//親がいるならその親から削除する
				if (back->parent)
				{
					auto& parent = back->parent;
					auto pos = parent->children.begin() + back->index;
					pos = parent->children.erase(pos);
					//後続がひとつ前にずれるので、indexをデクリメントする
					while (pos < parent->children.end())
					{
						--(*pos)->index;
						++pos;
					}
				}
				else
				{
					str->clear();
				}
			}
		}

		stack.append(back->children.reversed());
	}
}

void ys::PatternTree::buildTree(PatternTree& tree)
{
	ResultCollector* collector = nullptr;
	if (resultCollectorsStack.isEmpty())return;

	collector = resultCollectorsStack.back();

	auto ptr = collector->result;

	//metaTagによる操作
	proc<MetaTag::expand>(m_str, ptr);
	proc<MetaTag::ignore>(m_str, ptr);
	
	//ツリーの構築
	{
		Array<ResultCollector::Result*> stack{ ptr };
		Array<PatternTree*> treeStack{ &tree };
		tree.tag = ptr->node.patternTag;

		while (stack)
		{
			auto& back = stack.back();
			stack.pop_back();
			auto& treeBack = treeStack.back();
			treeStack.pop_back();

			treeBack->setIterator(back->node.start, back->node.end);
			if (not back->isMatch)treeBack->setIterator(back->node.start, back->node.start);

			back->children.each(
				[&](auto& child)mutable
				{
					if (not child->isMatch or child->node.start == child->node.end)return;

					//タグがつけられていたらグループに登録
					if (not child->node.patternTag.empty())
					{
						treeBack->m_groups[child->node.patternTag] << treeBack->m_children.size();
					}
					PatternTree childTree{ m_str,child->node.start, child->node.end };
					childTree.tag = child->node.patternTag;
					treeBack->m_children.push_back(childTree);
					stack << child;
				});

			treeBack->m_children.each([&](auto& child) {treeStack << &child; });
		};
	}
}

ys::PatternTree::PatternTree(const std::string& str)
	:PatternTree(String{ str.begin(),str.end() })
{
}

ys::PatternTree::PatternTree(const String& str)
	:m_str(std::make_shared<String>(str))
{
	m_itr = m_str->begin();
	m_end = m_str->end();
}

ys::PatternTree::PatternTree(std::shared_ptr<String> str, const StringIterator& itr, const StringIterator& end)
	:m_itr(itr), m_end(end), m_str(str)
{}

ys::PatternTree::PatternTree(const PatternTree& other)
{
	*this = other;
}

ys::PatternTree& ys::PatternTree::operator=(const PatternTree& other)
{
	m_str = other.m_str;
	m_itr = other.m_itr;
	m_end = other.m_end;
	m_children = other.m_children;
	m_groups = other.m_groups;
	tag = other.tag;
	isEnable = other.isEnable;
	//other.m_str; これはムーブしない
	return *this;
}

ys::PatternTree ys::PatternTree::search(PatternList&& pattern)
{
	return search(Pattern{ std::move(pattern) });
}

ys::PatternTree ys::PatternTree::search(const Pattern& pattern)
{
	ResultCollector collector;
	resultCollectorsStack << &collector;

	auto itr = m_itr;

	auto isMatch = pattern(itr, m_end);

	collector.result->isMatch = isMatch;
	collector.result->node = ResultCollector::NodeInfo{ .patternTag = pattern.getTag()};
	collector.result->node.tag = pattern.metaTag;
	collector.result->node.start = m_itr;
	collector.result->node.end = itr;

	PatternTree tree;

	m_itr = itr;
	tree.m_str = m_str;
	buildTree(tree);

	resultCollectorsStack.pop_back();

	return tree;
}

Array<ys::PatternTree> ys::PatternTree::children() const
{
	return m_children;
}

Array<size_t> ys::PatternTree::indexOf(const std::string& tag) const
{
	return indexOf(String{ tag.begin(),tag.end() });
}

Array<size_t> ys::PatternTree::indexOf(const String& tag) const
{
	if (m_groups.contains(tag)) return m_groups.at(tag);
	return {};
}

ys::PatternTree::operator bool() const
{
	return isEnable ? *isEnable : m_itr != m_end;
}

size_t ys::PatternTree::size() const
{
	return m_children.size();
}

void ys::PatternTree::setIterator(const StringIterator& itr, const StringIterator& end)
{
	m_itr = itr;
	m_end = end;
}

String ys::PatternTree::str() const
{
	return GetString(m_itr, m_end);
}

ys::PatternTree ys::PatternTree::match(PatternList&& pattern)
{
	return match(Pattern{ std::move(pattern) });
}

ys::PatternTree ys::PatternTree::match(const Pattern& pattern)
{
	auto ret = search(pattern);
	ret.isEnable = m_itr == m_end;
	return ret;
}

ys::PlaceHolder::~PlaceHolder()
{
	for (auto& [id, holder] : holderTable) delete holder;
	patternStore.each([](auto& pattern) {delete pattern; });
}

void ys::PlaceHolder::replace(size_t id, const Pattern& pattern)
{
	if (not holderTable.contains(id))return;
	patternStore.push_back(new Pattern{ pattern });
	holderTable[id]->hold = patternStore.back();
}

void ys::PlaceHolder::replace(size_t id, Pattern* pattern)
{
	if (not holderTable.contains(id))return;
	holderTable[id]->hold = pattern;
}

ys::Pattern ys::PlaceHolder::operator()(size_t id)
{
	if (not holderTable.contains(id)) {
		holderTable[id] = new Holder();
	};
	return holderTable[id];
}

ys::SetHolder::SetHolder(PlaceHolder& holder, Array<std::pair<size_t, std::variant<Pattern,Pattern*>>> patterns)
{
	for (auto [id, pattern] : patterns)
	{
		pattern.index() == 0 ?
			holder.replace(id, std::get<Pattern>(pattern)) :
			holder.replace(id, std::get<Pattern*>(pattern));
	}
}

#include"../Util/Visualizer.h"

String ys::visualize(const PatternTree& tree)
{
	return ys::VisualizeTree<PatternTree>(
		tree,
		[](auto tr) {return U"[{}]{}"_fmt(tr.tag, tr.str()); },
		[](auto tr) {return tr.children(); }
	);
}

ys::PatternTree ys::PatternTree::get(const std::string& tag, size_t i) const
{
	return get(String(tag.begin(), tag.end()), i);
}

ys::PatternTree ys::PatternTree::get(const String& tag, size_t i) const
{
	auto trees = group(tag);
	if (trees.size() <= i)return PatternTree{};
	return trees[i];
}

ys::PatternTree ys::PatternTree::get(const char* str, size_t i) const
{
	return get(std::string(str), i);
}

ys::PatternTree ys::PatternTree::get(const char32_t* str, size_t i) const
{
	return get(String(str), i);
}

Array<ys::PatternTree> ys::PatternTree::group(const std::string& tag) const
{
	return group(String(tag.begin(), tag.end()));
}

Array<ys::PatternTree> ys::PatternTree::group(const String& tag) const
{
	Array<PatternTree> result;

	if (m_groups.contains(tag))
	{
		m_groups.at(tag).each([&](const auto& index) {result << m_children[index]; });
	}

	return result;
}

Array<ys::PatternTree> ys::PatternTree::group(const char* str) const
{
	return group(std::string(str));
}

Array<ys::PatternTree> ys::PatternTree::group(const char32_t* str) const
{
	return group(String(str));
}

ys::PatternTree ys::PatternTree::operator[](size_t i) const
{
	return m_children[i];
}

ys::PatternTree& ys::PatternTree::resetAt(size_t i, const PatternTree& tree)
{
	if (m_children.size() <= i)return *this;

	if (m_groups.contains(m_children[i].tag))
	{
		m_groups[m_children[i].tag].remove(i);
	};
	if (not tree.tag.isEmpty())m_groups[tree.tag] << i;
	m_children[i] = tree;

	return *this;
}

ys::PatternTree& ys::PatternTree::reset(const std::string& tag, const PatternTree& tree, size_t i)
{
	return reset(String{ tag.begin(),tag.end() }, tree, i);
}

ys::PatternTree& ys::PatternTree::reset(const String& tag, const PatternTree& tree, size_t i)
{
	size_t count = 0;
	size_t idx;
	for (idx = 0; idx < m_children.size(); idx++)
	{
		if (m_children[idx].tag == tag and count++ == i)
		{
			return resetAt(idx, tree);
		}
	}
	return *this;
}
