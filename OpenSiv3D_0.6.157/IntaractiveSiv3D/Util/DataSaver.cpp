#include "../stdafx.h"
#include "DataSaver.h"

Optional<DataSaver> DataSaver::createDiff(const DataSaver& other) const
{
	struct AppendKeyWord
	{
		bool hasDifference=false;
		void operator() (String& str, StringView keyWord) { str += keyWord; hasDifference = true; }
		String operator()(const String& str, StringView keyWord) { hasDifference = true; return (str + keyWord); }
		void operator() (Array<String> list, StringView keyWord) { list << String(keyWord); hasDifference = true; }
		operator bool()const { return hasDifference; }
	};

	DataSaver ret;
	AppendKeyWord appender;
	//引数の差分
	{
		auto myArg = arg.split(U',');
		auto otherArg = other.arg.split(U',');
		size_t argSize = myArg.size();
		size_t otherArgSize = otherArg.size();
		bool isLonger = otherArgSize < argSize;
		auto min = isLonger ? otherArgSize : argSize;
		auto max = isLonger ? argSize : otherArgSize;

		size_t i = 0;
		//変更を検知
		for (; i < min; i++)if (myArg[i] != otherArg[i]) appender(otherArg[i],U"@");
		//削除されたか追加されたか
		if (isLonger)	for (; i < max; i++)appender(otherArg, myArg[i] + U"-");
		else			for (; i < max; i++)appender(otherArg[i],U"@");

		//データセーバーを更新
		for (const auto& newArg : otherArg)
		{
			if (newArg.back() == U'@' or newArg.back() == U'-')ret.arg += newArg + U",";
		}
		if (not ret.arg.empty())ret.arg.pop_back();//末尾の,を削除
	}
	//子の差分
	{
		auto myChildren = children;
		DsTable newChildren;
		for (const auto& [name, child] : myChildren)
		{
			//子の削除を検知
			if (not other.children.contains(name))
			{
				newChildren.emplace(appender(name, U"-"), child);
				continue;
			}
			//子の変更を検知
			if (auto diff = children.at(name).createDiff(other.children.at(name)))
			{
				newChildren.emplace(appender(name, U"@"),child);
			}
			else //変更がない
			{
			}
		}
		ret.children = newChildren;
	}

	if ((bool)appender)	return ret;
	else				return none;
}

String DataSaver::getName() const
{
	String ret=name;
	if ((not ret.empty()) and (ret.back() == U'-' or ret.back() == U'@'))ret.pop_back();
	return ret;
}

//コメントアウトと波カッコを補う
String DataSaver::preProcess(StringView text)
{
	bool bracesFlag = false;//波かっこ
	size_t closeBracketsInd = 0;
	String res(text);
	size_t insertNum = 0;
	for (size_t i = 0; i < text.size(); i++)
	{
		if (text[i] == U']') {
			bracesFlag = true;
			closeBracketsInd = i+1;
		}
		else if (bracesFlag) {
			if (text[i] == U'{') {
				bracesFlag = false;
			}
			else if (text[i] == U'[') {
				//省略された波かっこを補充
				res.insert(closeBracketsInd + insertNum, U"{"); insertNum++;
				res.insert(i + insertNum, U"}"); insertNum++;
				bracesFlag = false;
			}
		}
	}
	//最後のパックで波かっこが省略されていたらここで補充
	if (bracesFlag)
	{
		res.insert(closeBracketsInd + insertNum, U"{");
		res.append(U"}");
	}

	return res;
}
