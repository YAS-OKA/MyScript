#pragma once
#include "../Util/Borrow.h"
#include "../lib/PatternMatch.h"
#include "../Util/Util.h"
#include "../lib/MemoryPool.h"

namespace ys
{
	namespace analysis
	{
		// 関数情報を抽出する構造体
		template <typename T>
		struct function_traits;

		template <typename Cty, typename Rty, typename... Args>
		struct function_traits<Rty(Cty::*)(Args...) const> {
			using A = std::tuple<Args...>;
			using Fty = std::function<Rty(Args...)>;
			using T = Rty;
		};

		extern HashTable<String, size_t> OperatorNameToPriority;

		extern const Char ch;
		extern const Pattern ChildIdentifier;
		extern const Pattern Identifier;//識別子

		extern const Char IgnoreChar;
		extern const Pattern Ignore;

		extern const Pattern Integer;//整数
		extern const Pattern Decimal;//小数点数
		extern const Pattern Rational;//有理数

		extern const Pattern StringLiteral;
		extern const Pattern Boolian;
		extern const Pattern Value;

		extern PlaceHolder holder;

		extern const Pattern Callable;

		extern const Pattern Operator;
		extern const Pattern CompoundOperator;

		extern const Pattern ExprItem;
		extern const Pattern SimpleExpr;
		extern const Pattern Expression;

		extern const Pattern Define;
		extern const Pattern Function;

		extern const Pattern Return;

		extern const Pattern Substitution;//代入式

		extern const Pattern Assine;
		extern const Pattern AssineGrammar;
		extern const Pattern AssineArea;

		extern const Pattern If;
		extern const Pattern For;
	}

	class YScript
	{
		static constexpr auto Extension = U"cpp";
	protected:
		struct CompileData
		{
			PatternTree tree;
			bool success()const;
		};
		struct RunTimeData
		{
			bool success()const;
		};

		struct Obj final
		{
			static constexpr auto ExceptionsHead = U"Exception:";

			CompileData* compile = nullptr;
			RunTimeData* runTime = nullptr;

			bool hasException()const;

			String exception;
		};

		Obj* createObj(const String& name);
	public:
		static String MargeException(const String& except1, const String& except2);

		struct LeftValue
		{
			std::any value;
			String type;
		};

		using FT = HashTable<String, HashTable<String, std::function<LeftValue(const Array<std::any>&)>>>;
		using VT = HashTable<String, LeftValue*>;

		struct State :public Borrowable
		{
			State(size_t stackSize);

			Array<LeftValue> returnValuesOfFunction;
			Array<Optional<bool>> exitForCondition;

			MemoryPool<LeftValue> dataStack;

			FT staticFunctionTable;
			HashTable<LeftValue*, size_t>referenceCounter;

			VT valueTable;

			HashTable<std::type_index, String> cppTypeToScriptTypeName;

			String getTypeName(std::type_index index)const;

			template<class F>
			void registerOperator(const String& operatorName, F&& f)
			{
				registerFunction(U"operator\\{}"_fmt(operatorName), std::forward<F>(f));
			}

			template<class F>
			void registerFunction(const String& name, F f)
			{
				using traits = analysis::function_traits<decltype(&F::operator())>;
				using Fty = traits::Fty;
				registerFunction(name, Fty{ f });
			}

			template<class F>
			void registerType(const String& name, F f)
			{
				using traits = analysis::function_traits<decltype(&F::operator())>;
				using Fty = traits::Fty;
				using T = traits::T;

				registerType(name, Fty{ f }, std::function<T(T&)>{[](T& original) {return T(original); }});
			}
			template<class F, class CloneF>
			void registerType(const String& name, F f, CloneF cloneFunction)
			{
				using traits = analysis::function_traits<decltype(&F::operator())>;
				using Fty = traits::Fty;
				using T = traits::T;

				std::function<T(T&)> clone = cloneFunction;
				registerType(name, Fty{ f }, clone);
			}
		private:
			template<class T, class ...Args>
			void registerFunction(const String& name, const std::function<T(Args...)>& f)
			{
				registerFunctionImpl(
					name,
					[=, returnType = getTypeName(typeid(T))](const Array<std::any>& arguments)
					{
							return std::apply
							(
								[=](Args&&... args)
								{
										LeftValue result{ {}, returnType };
										if constexpr (not std::is_void_v<T>)
										{
											result.value = f(std::forward<Args>(args)...);
										}
										else {
											f(std::forward<Args>(args)...);
										}
										return result;
								},
								util::makeTupleFromArray<Args...>(arguments)
							);
					},
					std::initializer_list<std::type_index>{ typeid(Args)... },
					typeid(T)
				);
			}
			//fはコンストラクタ
			template<class T, class ...Args>
			void registerType(const String& type, const std::function<T(Args...)>& f, const std::function<T(T&)>& clone)
			{
				registerTypeImpl
				(
					type,
					[=](const Array<std::any>& arguments)
					{
							return std::apply
							(
								[=](Args&&... args) {
										LeftValue result;
										result.value = f(std::forward<Args>(args)...);
										result.type = type;
										return result;
								},
								util::makeTupleFromArray<Args...>(arguments)
							);
					},
					std::initializer_list<std::type_index>{ typeid(Args)... },
					typeid(T)
				);

				if (clone) registerFunction(U"clone", clone);
			}

			void registerFunctionImpl(
				const String& name,
				const std::function<LeftValue(const Array<std::any>&)>& f,
				const std::initializer_list<std::type_index>& argsType,
				const std::type_index& returnType
			);

			void registerTypeImpl(
				const String& name,
				const std::function<LeftValue(const Array<std::any>&)>& f,
				const std::initializer_list<std::type_index>& argsType,
				const std::type_index& returnType
			);
		};

		struct Parameter final
		{
			String dir;
		};

		/// @brief CompileData,RunTimeDataの情報を取得するゲッタクラス
		struct ResultSummary final
		{
			String getException()const;

			void addResult(const String& name, Obj* result);

			bool isCompileSuccess(const String& name)const;
			bool isRunSuccess(const String& name)const;

			PatternTree getTree(const String& name)const;
		private:
			HashTable<String,std::shared_ptr<Obj>> results;
		};

		ResultSummary summary;

		Borrow<State> createState(size_t stackSize = 1 << 11);

		bool compile(const Array<FilePath>& srcPathList);

		bool compilePrj(const FilePath& dir);

		bool compileSrc(const FilePath& path);
		bool compileSrc(const String& name, const String& src);

		ResultSummary run(const String& name,const Parameter& param);
	private:
		std::unique_ptr<State> p_state;
		HashTable<String, Obj*> results;
	};
}
