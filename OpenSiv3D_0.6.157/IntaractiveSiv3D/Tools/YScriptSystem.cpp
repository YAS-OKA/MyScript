#include "../stdafx.h"
#include "YScriptSystem.h"

#define REP(pattern,spliter)(pattern+many(spliter+pattern))

namespace ys
{
	namespace analysis
	{
		PlaceHolder holder;

		const Char ch{ alp or c('_') };
		const Pattern ChildIdentifier{ ch + many(ch or num) or s("..") };
		const Pattern Identifier{ (c('\\') or emp()) + REP(ChildIdentifier,c('\\')) };//識別子

		const Pattern Integer{ (c('-') or c('+') or emp()) / "sign" + (1 <= many(num)) };//整数
		const Pattern Decimal{ Integer + c('.') + (1 <= many(num)) };//小数点数
		const Pattern Rational{ Decimal / "dec" or Integer / "int" };//有理数

		const Pattern StringLiteral{ c('\"') + many(!c('\"') && wild) / "contents" + c('\"') };
		const Pattern Boolian{ s("true") or s("false") };
		const Pattern Value{ StringLiteral / "str" or Boolian / "bool" or Rational / "rat" or Identifier / "ide" };

		HashTable<String, size_t> OperatorNameToPriority
		{
			{U"^",100},
			{U"*",90},{U"/",90},{U"%",90},
			{U"-",80},{U"+",80},
			{U"<",70},{U">",70},{U"<=",70},{U">=",70},
			{U"==",60},{U"!=",60},
			{U"and",50},{U"or",50}
		};

		Pattern _createOperator()
		{
			Pattern result;

			Array<String> keys;
			for (auto [op, _] : OperatorNameToPriority)keys << op;

			size_t bigIdx, bigEqIdx, smallIdx, smallEqIdx;
			for (size_t i = 0; i < keys.size(); i++)
			{
				if (keys[i] == U">")bigIdx = i;
				else if (keys[i] == U">=")bigEqIdx = i;
				else if (keys[i] == U"<")smallIdx = i;
				else if (keys[i] == U"<=")smallEqIdx = i;
			}
			if (bigIdx < bigEqIdx)keys[bigIdx].swap(keys[bigEqIdx]);
			if (smallIdx < smallEqIdx)keys[smallIdx].swap(keys[smallEqIdx]);

			bool init = true;
			keys.each([&](const auto& op) {result = init ? s(op) : result or s(op); init = false; });

			return result;
		}

		const Char IgnoreChar{ c('\n') or c('\t') or c(' ') };
		const Pattern Ignore{ many(IgnoreChar) / MetaTag::ignore };

		const Pattern Callable{ Identifier / "name" + c('(') + (REP(holder(0),c(',')) / "args" or emp()) + c(')') };

		const Pattern Operator = _createOperator();
		const Pattern CompoundOperator
		{
			(Identifier / "left" + (s("+=") or s("-=") or s("*=") or s("/=") or s("%=")) / "operator" + holder(7) / "right") / "expr" or
			(s("++") or s("--")) / "operator" + Identifier / "ide" or
			Identifier / "ide" + (s("++") or s("--")) / "operator"
		};
		const Pattern ExprItem{ CompoundOperator / "compound" or Callable / "function" or Value / MetaTag::expand };
		const Pattern SimpleExpr{ ExprItem / "item" + (Ignore + Operator / "operator" + Ignore + holder(1) / "expr" or emp()) };
		const Pattern Expression
		{
			(s("not") / "not" or emp()) +
			(c('(') + Ignore + holder(1) / "expr" + Ignore + c(')') or SimpleExpr) +
			(Ignore + Operator / "operator" + Ignore + holder(1) / "expr" or emp())
		};

		const Pattern Define{ Identifier / "type" + (1 <= many(IgnoreChar)) + Identifier / "name" };
		const Pattern Function{ Identifier / "name" + c('(') + Ignore + (REP(Define,c(',')) / "args" or emp()) + Ignore + c(')') + Ignore + holder(3) / "body" };
		const Pattern Return{ s("return") + (((1 <= many(IgnoreChar)) + Expression / "expr") or emp()) };

		const Pattern Substitution{ Identifier / "left" + Ignore + (c('=') or s("->")) / "conector" + Ignore + Expression / "right" };//代入式

		const Pattern Assine
		{
			(s("//") + many(!c('\n')) + c('\n') or s("/*") + many(!s("*/")) + s("*/")) / MetaTag::ignore or 
			holder(4) / "if" or
			holder(5) / "for" or
			Return / "return" or
			s("break") / "break" or
			s("continue") / "continue" or
			Function / "def" or
			Substitution / "substitution" or
			Expression / "expr"
		};
		const Pattern AssineGrammar{ many(Ignore + Assine + Ignore + (c(';') or emp())) };
		const Pattern AssineArea{ c('{') + (AssineGrammar / "contents" or Ignore) + c('}') };

		const Pattern If
		{
			s("if") + Ignore +
			c('(') + Ignore + Expression / "condition" + Ignore + c(')')
			+ Ignore + (AssineArea / MetaTag::expand or Assine / "contents") + Ignore +
			((s("else") + Ignore + (AssineArea / MetaTag::expand or Assine / "contents")) / "else" or emp())
		};
		const Pattern For
		{
			s("for") + Ignore +
			c('(') + Ignore + (Substitution / "substitution" or Expression / "expr" or emp()) / "first" + Ignore +
			c(';') + Ignore + Expression / "condition" + Ignore +
			c(';') + Ignore + (Substitution / "substitution" or Expression / "expr" or emp()) / "second" + Ignore +
			c(')') + Ignore + (AssineArea / MetaTag::expand or Assine / "contents")
		};

		SetHolder _{ holder,{{0,Expression / "arg"},{1,Expression },{3,AssineArea},{4,If},{5,For},{7,Expression }} };
	}
}

using namespace ys;

#define RegisterOperator3(state,x1,x2,ty1,ty2)\
state.registerOperator(util::str(#x1), [](ty1 l, ty1 r) {return x2; });\
state.registerOperator(util::str(#x1), [](ty2 l, ty1 r) {return x2; });\
state.registerOperator(util::str(#x1), [](ty1 l, ty2 r) {return x2; });\
state.registerOperator(util::str(#x1), [](ty2 l, ty2 r) {return x2; });

//2組の組み合わせ
#define RegisterOperator2(state,x,ty1,ty2)\
state.registerOperator(util::str(#x), [](ty1 l, ty1 r) {return l x r; });\
state.registerOperator(util::str(#x), [](ty2 l, ty1 r) {return l x r; });\
state.registerOperator(util::str(#x), [](ty1 l, ty2 r) {return l x r; });\
state.registerOperator(util::str(#x), [](ty2 l, ty2 r) {return l x r; });

#define RegisterOperator1(state,x,ty1) state.registerOperator(util::str(#x), [](ty1 l, ty1 r) {return l x r; });

#define RegisterOperator(state,x,ty1,ty2) state.registerOperator(util::str(#x), [](ty1 l, ty2 r) {return l x r; });

class Executor
{
public:
	String exception;

	using LeftValue = YScript::LeftValue;
	using VT = typename YScript::VT;
	using FT = typename YScript::FT;

	LeftValue null{ .value = nullptr,.type = U"null" };
	LeftValue Void{ .type = U"void" };

	YScript::State& state;

	String cd(const String& dir, const String& dest)
	{
		String result = dir;
		bool ignore = false;

		if (dest.size() and dest[0] == U'\\')
		{
			result.clear();
			ignore = true;
		}

		dest.split('\\').each(
			[&](const auto& elm)
			{
				if (ignore)
				{
					ignore = false;
					return;
				}
				if (elm == U"..")
				{
					result.pop_back_N(result.size() - result.lastIndexOf('\\'));
				}
				else
				{
					result += result.isEmpty() ? elm : U"\\" + elm;
				}
			}
		);
		return result;
	}
	//idは1から始まるように割り当てる 0がnullptrの役割
	LeftValue* createId()
	{
		return state.dataStack.alloc(null);
	};

	LeftValue popData(LeftValue* id)
	{
		auto ret = *id;
		state.referenceCounter.erase(id);
		state.dataStack.free(id);
		return ret;
	}

	String createArguments(const String& dir, const Array<PatternTree>& argTrees, Array<std::any>& args, const VT& table)
	{
		String result;
		argTrees.each
		(
			[&](const auto& val)
			{
				auto id = createRightValue(dir, val, table);
				auto value = id.value;
				auto type = id.type;
				if (type == U"ref")
				{
					auto data = std::any_cast<LeftValue*>(value);
					value = std::ref(data->value);
					type = data->type;
				}
				args << value;
				result += U"{},"_fmt(type);
			}
		);
		//後ろの,を削除
		if (not result.isEmpty())result.pop_back();

		return result;
	}

	LeftValue callFunction(const String& dir, const String& name, const Array<PatternTree>& argTrees, const VT& table)
	{
		LeftValue result;
		auto dirPos = cd(dir, name);
		if (not state.staticFunctionTable.contains(dirPos))return result;

		//引数の作成
		Array<std::any> args;
		String argsType = createArguments(dir, argTrees, args, table);
		//該当する関数が登録されているか判定
		if (state.staticFunctionTable[dirPos].contains(argsType))
		{
			result = state.staticFunctionTable[dirPos][argsType](args);
		}
		return result;
	}

	LeftValue callFunction(const String& dir, const PatternTree& callable, const VT& table)
	{
		auto name = callable.get("name").str();
		auto argTrees = callable.get("args").group("arg");
		return callFunction(dir, name, argTrees, table);
	}

	//式の項から右辺値を作る
	LeftValue createValueFromItem(const String& dir, const PatternTree& itemTree, const VT& table)
	{
		LeftValue result;

		if (auto item = itemTree.get("compound"))
		{
			if (auto expr = item.get("expr"))
			{
				auto op = expr.get("operator").str();
				auto left = expr.get("left");
				auto right = expr.get("right");
				left.resetAt(0, PatternTree{ left.str() }.search(analysis::Value / "item"));
				result = callFunction(dir, U"\\operator\\{}"_fmt(op), { left,right }, table);
			}
			else
			{
				auto op = item.get("operator").str();
				auto ide = item.get("ide");
				Array<PatternTree> args = { ide.resetAt(0,PatternTree{ ide.str() }.search(analysis::ExprItem / "item")) };
				//後置の場合はダミーを追加
				if (item[1].tag == U"operator")
				{
					args << PatternTree{ U"0" }.search(Pattern{}.add(analysis::ExprItem / "item") / "dummy");
				}
				result = callFunction(dir, U"\\operator\\{}"_fmt(op), args, table);
			}
		}
		else if (auto item = itemTree.get("function"))
		{
			result = callFunction(dir, item, table);
		}
		else if (auto item = itemTree.get("str"))
		{
			auto value = item.get("contents").str();
			value.replace(U"\\n", U"\n");
			value.replace(U"\\t", U"\t");
			result.value = value;
			result.type = U"str";
		}
		else if (auto item = itemTree.get("bool"))
		{
			result.value = Parse<bool>(item.str());
			result.type = U"bool";
		}
		else if (auto item = itemTree.get("rat"))
		{
			if (auto dec = item.get("dec"))
			{
				result.value = Parse<double>(dec.str());
				result.type = U"dec";
			}
			else if (auto integer = item.get("int"))
			{
				result.value = Parse<int32>(integer.str());
				result.type = U"int";
			}
		}
		else if (auto item = itemTree.get("ide"))
		{
			auto itemDir = cd(dir, item.str());
			if (table.contains(itemDir))
			{
				auto ref = table.at(itemDir);
				result.value = ref;
				result.type = U"ref";
			}
		}
#if _DEBUG
		else {
			exception = YScript::MargeException(exception, U"右辺値エラー");
		}
#endif
		return result;
	}

	LeftValue calculateValue(const String& currentDir, const String& operatorName, LeftValue left, LeftValue right)
	{
		while (left.type == U"ref")
		{
			auto data = std::any_cast<LeftValue*>(left.value);
			left.value = data->value;
			left.type = data->type;
		}
		while (left.type == U"ref")
		{
			auto data = std::any_cast<LeftValue*>(right.value);
			right.value = data->value;
			right.type = data->type;
		}
		String argType = U"{},{}"_fmt(left.type, right.type);
		auto operatorDirectory = cd(currentDir, U"\\operator\\{}"_fmt(operatorName));

		//演算子による関数実行
		auto f = state.staticFunctionTable.at(operatorDirectory).at(argType);
		return f({ left.value,right.value });
	}

	//代入式の右辺を受け取る
	LeftValue createRightValue(const String& currentDir, const PatternTree& rightExpressionTree, const VT& table, Optional<std::pair<LeftValue, String>> pre = none)
	{
		LeftValue value = null;
		PatternTree left, right;
		if (auto item = rightExpressionTree.get("item"))
		{
			value = createValueFromItem(currentDir, item, table);
			right = rightExpressionTree.get("expr");
		}
		else
		{
			left = rightExpressionTree.get("expr", 0);
			right = rightExpressionTree.get("expr", 1);
		}

		//どちらもない
		if (not (left or value.type != U"null") and not right) return null;

		bool hasNot = rightExpressionTree.get("not");
		//どちらか一方
		if (not ((left or value.type != U"null") and right))
		{
			if (left)value = createRightValue(currentDir, left, table);
			if (hasNot)value.value = not std::any_cast<bool>(value.value);

			if (right) value = createRightValue(currentDir, right, table);

			if (pre) value = calculateValue(currentDir, pre->second, pre->first, value);

			return value;
		}
		//itemもexprも両方ある
		if (value.type == U"null")value = createRightValue(currentDir, left, table);
		if (hasNot)value.value = not std::any_cast<bool>(value.value);

		auto operatorName = rightExpressionTree.get("operator").str();
		//ex. pre + item * expr
		if (pre and (analysis::OperatorNameToPriority.at(pre->second) < analysis::OperatorNameToPriority.at(operatorName)))
		{
			value = createRightValue(currentDir, right, table, std::pair<LeftValue, String>{value, operatorName});
			return calculateValue(currentDir, pre->second, pre->first, value);
		}

		//ex. true: pre * item + expr  false: item operator expr
		if (pre)value = calculateValue(currentDir, pre->second, pre->first, value);

		return createRightValue(currentDir, right, table, std::pair<LeftValue, String>{value, operatorName});
	}

	LeftValue* stackData(const String& leftIdentifier, LeftValue&& value, VT& table)
	{
		auto id = createId();
		table[leftIdentifier] = id;
		*id = std::move(value);
		return id;
	}

	void substitution(LeftValue right, const String& leftIdentifier, VT& table,bool referable)
	{
		LeftValue* preId = nullptr;
		if (table.contains(leftIdentifier))preId = table[leftIdentifier];

		//参照更新を行うか　（参照指し直し、または初期化時ならtrue）
		if (not preId)referable = true;
		//右辺の参照の取得
		LeftValue* id;
		if (referable)
		{
			id = right.type == U"ref" ? std::any_cast<LeftValue*>(right.value) : stackData(leftIdentifier, std::move(right), table);
			table[leftIdentifier] = id;
		}
		else 
		{
			id = table[leftIdentifier];
			*id = std::move(right);
		}
		//参照カウンタを更新
		if (not state.referenceCounter.contains(id))state.referenceCounter[id] = 0;
		++state.referenceCounter[id];
		if (preId)
		{
			--state.referenceCounter[preId];
			if (state.referenceCounter[preId] == 0)popData(preId);
		}
	}

	void updateValueTable(const String& dir, const Array<PatternTree>& trees, VT& table, bool& exit)
	{
		trees.each(
			[this, &table, &dir, &exit](const auto& child)
			{
				if (exit)return;

				if (child.tag == U"break")
				{
					if (state.exitForCondition) state.exitForCondition.back() = true;
					exit = true;
				}
				else if (child.tag == U"continue")
				{
					if (state.exitForCondition) state.exitForCondition.back() = false;
					exit = true;
				}
				else if (child.tag == U"return")
				{
					if (auto expr = child.get("expr"))
					{
						if (not state.returnValuesOfFunction.isEmpty())state.returnValuesOfFunction.back() = createRightValue(dir, expr, table);
					}
					exit = true;
				}
				else if (child.tag == U"substitution")
				{
					auto value = createRightValue(dir, child.get("right"), table);
					auto leftIdentifier = cd(dir, child.get("left").str());
					bool referable= child.get("conector").str() == U"->";
					substitution(value, leftIdentifier, table, referable);
				}
				else if (child.tag == U"def")
				{
					auto name = cd(dir, child.get("name").str());
					auto argsTree = child.get("args");
					auto types = util::GenerateArray<String>(argsTree.group("type"), [](const auto& type) { return type.str(); });
					auto argNames = util::GenerateArray<String>(argsTree.group("name"), [&](const auto& arg) { return cd(dir, arg.str()); });
					auto assineArea = child.get("body").get("contents");

					String argTypeName;
					types.each([&](const auto& type) {argTypeName += U"{},"_fmt(type); });
					if (not argTypeName.isEmpty())argTypeName.pop_back();

					state.staticFunctionTable[name][argTypeName]
						= [=](const Array<std::any>& arguments)mutable
						{
							VT localValue;
							for (size_t i = 0; i < arguments.size(); i++)
							{
								auto id = createId();
								localValue[argNames[i]] = id;
								id->value = arguments[i];
								id->type = types[i];
							}
							state.returnValuesOfFunction.push_back(Void);
							bool exitFunction = false;
							updateValueTable(dir, assineArea.children(), localValue, exitFunction);
							auto ret = std::move(state.returnValuesOfFunction.back());
							state.returnValuesOfFunction.pop_back();
							//参照であれば一つ上にあがる（もしローカル参照の参照を返すようにしていたらバグるけど、それはバグって然るべき）
							if (ret.type == U"ref")ret = std::ref(*std::any_cast<LeftValue*>(ret.value));
							//ローカル変数の削除　
							for (auto& [key, id] : localValue)
							{
								if (not state.referenceCounter.contains(id))continue;
								--state.referenceCounter.at(id);
								if (state.referenceCounter.at(id) == 0)popData(id);
							}
							return ret;
						};
				}
				else if (child.tag == U"expr")
				{
					createRightValue(dir, child, table);
				}
				else if (child.tag == U"if")
				{
					auto condition = child.get("condition");
					if (std::any_cast<bool>(createRightValue(dir, condition, table).value))
					{
						updateValueTable(dir, child.get("contents").children(), table, exit);
					}
					else if (auto elseTree = child.get("else"))
					{
						updateValueTable(dir, elseTree.get("contents").children(), table, exit);
					}
				}
				else if (child.tag == U"for")
				{
					auto first = child.get("first");
					auto condition = child.get("condition");
					auto second = child.get("second");
					bool hasEmptyFirst = first.str().isEmpty(),
						hasEmptySecond = second.str().isEmpty();

					if (not hasEmptyFirst)updateValueTable(dir, first.children(), table, exit);

					state.exitForCondition.push_back(none);
					while (std::any_cast<bool>(createRightValue(dir, condition, table).value)
						and not (state.exitForCondition and state.exitForCondition.back() and *state.exitForCondition.back()))
					{
						updateValueTable(dir, child.get("contents").children(), table, exit);
						if (not hasEmptySecond)updateValueTable(dir, second.children(), table, exit);
						if (exit and not state.exitForCondition.back())break;
						else exit = false;
					}
					state.exitForCondition.pop_back();
				}
			}
		);
	}
};

bool YScript::CompileData::success() const
{
	return tree;
}

String ys::YScript::ResultSummary::getException() const
{
	String exception = U"";
	for (const auto& [name,obj] : results)
	{
		if (not obj->hasException())continue;
		exception = MargeException(name, obj->exception);
	}
	return exception;
}

void YScript::ResultSummary::addResult(const String& name, Obj* result)
{
	results[name] = std::shared_ptr<Obj>(result);
}

PatternTree YScript::ResultSummary::getTree(const String& name) const
{
	PatternTree result;
	if (results.contains(name))result = results.at(name)->compile->tree;
	return result;
}

bool YScript::ResultSummary::isCompileSuccess(const String& name) const
{
	if (not results.contains(name))return false;
	return results.at(name)->compile->success();
}

bool YScript::ResultSummary::isRunSuccess(const String& name) const
{
	if (not results.contains(name))return false;
	if (not results.at(name)->runTime)return false;
	return results.at(name)->runTime->success();
}

YScript::Obj* YScript::createObj(const String& name)
{
	Obj* result = new Obj;
	summary.addResult(name, result);
	results[name] = result;
	return result;
}

String YScript::MargeException(const String& except1, const String& except2)
{
	return U"{}\n{}\n{}"_fmt(except1, Obj::ExceptionsHead, except2);
}

Borrow<YScript::State> ys::YScript::createState(size_t stackSize)
{
	p_state = std::make_unique<State>(stackSize);
	auto& state = *p_state;

	state.cppTypeToScriptTypeName[typeid(void)] = U"void";

	state.registerType(U"int", [](int v) {return v; });
	state.registerType(U"str", [](const String& v) {return v; });
	state.registerType(U"bool", [](bool v) {return v; });
	state.registerType(U"dec", [](double v) {return v; });

	state.registerFunction(U"fmt", [](int i) {return Format(i); });
	state.registerFunction(U"fmt", [](double i) {return Format(i); });

	RegisterOperator1(state, %, int);
	RegisterOperator1(state, +, const String&);
	RegisterOperator1(state, and, bool);
	RegisterOperator1(state, or , bool);
	RegisterOperator1(state, == , bool);
	RegisterOperator1(state, != , bool);
	RegisterOperator1(state, == , const String&);
	RegisterOperator1(state, != , const String&);

	RegisterOperator2(state, +, int, double);
	RegisterOperator2(state, -, int, double);
	RegisterOperator2(state, *, int, double);
	RegisterOperator2(state, / , int, double);
	RegisterOperator2(state, < , int, double);
	RegisterOperator2(state, <= , int, double);
	RegisterOperator2(state, > , int, double);
	RegisterOperator2(state, >= , int, double);
	RegisterOperator2(state, == , int, double);
	RegisterOperator2(state, != , int, double);

	RegisterOperator3(state, ^, std::pow(l, r), int, double);

	RegisterOperator(state, +=, int&, int);
	RegisterOperator(state, -=, int&, int);
	RegisterOperator(state, *=, int&, int);
	RegisterOperator(state, /=, int&, int);
	RegisterOperator(state, %=, int&, int);
	RegisterOperator(state, +=, int&, double);
	RegisterOperator(state, -=, int&, double);
	RegisterOperator(state, *=, int&, double);
	RegisterOperator(state, /=, int&, double);
	RegisterOperator(state, +=, double&, double);
	RegisterOperator(state, -=, double&, double);
	RegisterOperator(state, *=, double&, double);
	RegisterOperator(state, /=, double&, double);
	RegisterOperator(state, +=, double&, int);
	RegisterOperator(state, -=, double&, int);
	RegisterOperator(state, *=, double&, int);
	RegisterOperator(state, /=, double&, int);
	RegisterOperator(state, +=, String&, const String&);

	state.registerOperator(U"++", [](int& n, int) {return n++; });
	state.registerOperator(U"++", [](int& n) {return ++n; });
	state.registerOperator(U"--", [](int& n, int) {return n--; });
	state.registerOperator(U"--", [](int& n) {return --n; });

	return state.lend();
}

bool YScript::compilePrj(const FilePath& directoryPath)
{
	return compile
	(
		FileSystem::DirectoryContents(directoryPath)
		.filter([&](const auto& path) {return FileSystem::Extension(path) == Extension; })
	);
}

bool YScript::compile(const Array<FilePath>& srcPathList)
{
	bool result = true;
	srcPathList.each([&](const auto& path) { result = result and compileSrc(path); });
	return result;
}

bool ys::YScript::compileSrc(const FilePath& path)
{
	return compileSrc(path, TextReader{ path }.readAll());
}

bool YScript::compileSrc(const String& name, const String& src)
{
	Obj* obj = createObj(name);

	obj->compile = new CompileData();
	obj->compile->tree = PatternTree{ src }.match(analysis::AssineGrammar);

	return obj->compile->success();
}

YScript::ResultSummary YScript::run(const String& name, const Parameter& param)
{
	if (not summary.isCompileSuccess(name))
	{
		String exception;
		if (not results.contains(name))
			exception = U"未コンパイルのオブジェクト:{}"_fmt(name);
		else
			exception = U"コンパイル失敗のオブジェクト:{}"_fmt(name);

		results.at(name)->exception = MargeException(results.at(name)->exception, exception);
		return summary;
	}

	//treeの取得
	const auto& tree = summary.getTree(name);

	Executor executor{ .state = *p_state };

	bool _exit = false;
	executor.updateValueTable(param.dir, tree.children(), p_state->valueTable, _exit);

	return summary;
}

ys::YScript::State::State(size_t stackSize) :dataStack(stackSize) {}

String ys::YScript::State::getTypeName(std::type_index index) const
{
#if _DEBUG
	if (not cppTypeToScriptTypeName.contains(index))
	{
		auto typeName = util::str(index.name());
		throw Error{ U"スクリプトに登録されていない型です. \ntypeName={}"_fmt(typeName) };
	}
#endif
	return cppTypeToScriptTypeName.at(index);
}

void YScript::State::registerFunctionImpl(
	const String& name,
	const std::function<YScript::LeftValue(const Array<std::any>&)>& f,
	const std::initializer_list<std::type_index>& argsType,
	const std::type_index& returnType
) {
	String argsTypeString;
	for (auto index : argsType)
	{
		argsTypeString += U"{},"_fmt(getTypeName(index));
	}
	if (not argsTypeString.isEmpty())argsTypeString.pop_back();

	const String returnTypeString = getTypeName(returnType);

	staticFunctionTable[name][argsTypeString] = f;
}

void YScript::State::registerTypeImpl(const String& name, const std::function<YScript::LeftValue(const Array<std::any>&)>& f, const std::initializer_list<std::type_index>& argsType, const std::type_index& returnType)
{
	cppTypeToScriptTypeName[returnType] = name;
	registerFunctionImpl(name, f, argsType, returnType);
}

bool ys::YScript::Obj::hasException() const
{
	return not exception.isEmpty();
}

bool ys::YScript::RunTimeData::success() const
{
	return false;
}
