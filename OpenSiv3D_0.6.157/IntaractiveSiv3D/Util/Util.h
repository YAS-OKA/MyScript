#pragma once
#include<Siv3D.hpp>
#include<fstream>
#include<string>
#include<any>

#define ENUM(x,...) \
enum class x {\
	__VA_ARGS__\
};\
inline StringView GetName_##x(x key){\
	static const auto EnumList=Array<String>{ String(U#__VA_ARGS__).replace(U" ",U"").split(U',').remove(U"") };\
	auto index=static_cast<int>(key);\
	if (EnumList.size()<=index)return U"";\
	return EnumList[static_cast<int>(key)];\
}\
inline x GetEnum_##x(StringView name=U"") {\
	size_t i=0;\
	while (auto s=GetName_##x((x)i)){if(name==s)return (x)i;++i;}\
	return (x)-1;\
}

namespace util
{
	template<class T> using uPtr = std::unique_ptr<T>;
	template<class T> using sPtr = std::shared_ptr<T>;
	template<class T> using wPtr = std::weak_ptr<T>;

	//極座標
	inline Vec2 polar(double rad, double len = 1)
	{
		return len * Vec2{ Cos(rad),Sin(rad) };
	}
	//ベクトル内積
	inline double dotProduct(const Vec2& vl, const Vec2& vr) {
		return vl.x * vr.x + vl.y * vr.y;
	}
	inline double dotProduct(const Vec3& vl, const Vec3& vr) {
		return vl.x * vr.x + vl.y * vr.y + vl.z * vr.z;
	}

	inline double angleOf2Vec(Vec2 A, Vec2 B)
	{
		//内積とベクトル長さを使ってcosθを求める
		double cos_sita = dotProduct(A, B) / (A.length() * B.length());

		//cosθからθを求める
		return Acos(cos_sita);
	}
	//x,yをひっくり返す
	inline Vec2 invXY(const Vec2& p)
	{
		return { p.y,p.x };
	}

	inline double getRad(const Vec2& p)
	{
		return p.getAngle() - 90_deg;
	}

	String str(const char* s);
	//名前被りの解決
	String resolveNameConflict(const String& name, Array<String> nameArray);
	//添え字の除去　例：name(1)->name, name(aaa)->name
	String removeSubscripts(const String& name);
	//添え字をゲット 例：name(1)->1, name(aaa)->aaa
	String getSubscripts(const String& name);

	//aのオーバーフローを利用し、循環させることも可能
	inline uint64 mod64(int64 a, uint64 b)
	{
		if (a >= 0)return a % b;
		else return (b - (-a) % b) % b;
	}
	//指定したインデックス以降の要素を返します
	template<class T>
	Array<T> slice(Array<T> arr, int64 a)
	{
		Array<T> ret{};
		size_t size = arr.size();
		if (size == 0 or size <= a)return ret;
		size_t ua = mod64(a, size);
		for (auto itr = arr.begin() + ua, en = arr.end(); itr != en; ++itr)ret << *itr;
		return ret;
	}
	//二つのインデックスで指定した区間の要素を返す。ただし終端の要素は含まない
	template<class T>
	Array<T> slice(Array<T> arr, int64 a, int64 b)
	{
		Array<T> ret{};
		size_t size = arr.size();
		if (size == 0 or size <= a)return ret;
		if (size <= b)return slice<T>(arr, a);
		size_t ua = mod64(a, size);
		size_t ub = mod64(b, size);
		if (ub < ua)return ret;
		for (auto itr = arr.begin() + ua, en = arr.begin() + ub; itr != en; ++itr)ret << *itr;
		return ret;
	}

	template<class T>
	size_t indexOf(const Array<T>& arr, std::function<bool(const T&)> f)
	{
		size_t ans = -1;

		for (size_t i = 0; i < arr.size(); ++i)
			if (f(arr[i])) {
				ans = i; break;
			}

		return ans;
	}
	template<class T>
	size_t indexOf_last(const Array<T>& arr, std::function<bool(const T&)> f)
	{
		size_t ans = -1;

		for (size_t i = arr.size(); 0 < i; --i)
			if (f(arr[i - 1])) {
				ans = i - 1; break;
			}

		return ans;
	}

	inline size_t indexOf_last(StringView s, StringView content)
	{
		size_t ret = s.indexOf(content);
		if (ret == String::npos)return ret;
		return ret + content.size();
	}

	inline String slice(StringView arr, int64 a)
	{
		String ret = U"";
		size_t size = arr.size();
		if (size == 0)return ret;
		size_t ua = mod64(a, size);
		for (auto itr = arr.begin() + ua, en = arr.end(); itr != en; ++itr)ret << *itr;
		return ret;
	}

	inline String slice(StringView arr, int64 a, int64 b)
	{
		String ret = U"";
		size_t size = arr.size();
		if (size == 0)return ret;
		if (size == b)return slice(arr, a);
		size_t ua = mod64(a, size);
		size_t ub = mod64(b, size);
		if (ub < ua)return ret;
		for (auto itr = arr.begin() + ua, en = arr.begin() + ub; itr != en; ++itr)ret << *itr;
		return ret;
	}

	//配列に配列を挿入する(i番目に挿入する)
	template<class A>
	void append(Array<A>& arr, Array<A> _ins, size_t i)
	{
		for (auto itr = _ins.rbegin(), en = _ins.rend(); itr != en; ++itr)
		{
			arr.insert(arr.begin() + i, *itr);
		}
	}
	//配列に配列を追加　同じ値は追加しない
	template<class A>
	void push(Array<A>& arr, Array<A> _comp)
	{
		_comp.remove_if([&](const A& c) {return arr.contains(c); });//かぶってるやつを消す
		arr.append(_comp);
	}

	inline bool between(StringView str, StringView key)
	{
		return str.indexOf(key) == 0 and str.lastIndexOf(key) == str.length() - key.length() - 1;
	}

	template<class T, class Fty, std::enable_if_t<std::is_invocable_r_v<double, Fty, T>>* = nullptr>
	inline T GetMax(Array<T> arr, Fty f, bool frontSidePrior = true)
	{
		if (arr.isEmpty())throw Error{ U"arrayが空でした。" };
		const auto comparing = frontSidePrior ? [](double v1, double v2) {return v1 < v2; } : [](double v1, double v2) {return v1 <= v2; };
		T* res = &arr[0];
		double value = f(*res);
		for (auto& elm : arr)
		{
			const auto elmValue = f(elm);
			if (comparing(value, elmValue))
			{
				res = &elm;
				value = elmValue;
			}
		}
		return *res;
	}
	//配列をもとに別の配列を生成
	template<class T, class U, class Fty, std::enable_if_t<std::is_invocable_r_v<T, Fty, U>>* = nullptr>
	inline Array<T> GenerateArray(Array<U> arr, Fty f)
	{
		Array<T> result;
		std::transform(arr.begin(), arr.end(), std::back_inserter(result), f);
		return result;
	}

	inline bool strEqual(StringView str, size_t from, size_t to, StringView cstr)
	{
		auto s = util::slice(str, from, to);

		if (not s.isEmpty() and s == cstr)return true;
		return false;
	}

	//上限で止まる
	struct StopMax
	{
		double value;
		double max;

		StopMax() {};

		StopMax(double max, double value = 0);

		void add(double value);

		bool additionable()const;
	};

	class EditFile
	{
	private:
		uPtr<std::fstream> file;
	public:

		EditFile() = default;

		EditFile(FilePathView path);
		//上書き
		void overwrite(StringView text);
		//fileの中身をすべて読みだす
		String readAll()const;
	};

	EditFile createFile(FilePath path);

	//メタ系　Optionalを判定する
	template < typename T >
	struct is_opt : std::false_type {};
	template < typename T >
	struct is_opt<Optional<T>> : std::true_type {};
	template < typename T >
	constexpr bool is_opt_v = is_opt<T>::value;

	//Stringを計算可能な型にパース
	template<class Type>
	inline typename std::enable_if<!is_opt_v<Type>, Type>
		::type convert(const String& value)
	{
		if (auto result = EvalOpt(value))
		{
			return static_cast<Type>(*result);
		}
		// 例外処理
		throw Error{ U"計算不能なValueが渡されました" };
	}

	//オプショナルな戻り値を許容する場合
	template<class T>
	inline typename std::enable_if<is_opt_v<T>, T>
		::type convert(const String& value)
	{
		if (value == U"none")return none;
		return convert<typename T::value_type>(value);
	}

	template<>
	inline bool convert<bool>(const String& value)
	{
		if (value == U"true")
		{
			return true;
		}
		else if (value == U"false")
		{
			return false;
		}
		throw Error{ U"bool 型に変換できない値が渡されました" };
	}

	template<>
	inline StringView convert<StringView>(const String& value) { return value; }

	template<>
	inline String convert<String>(const String& value) { return value; }

	template<>
	inline Vec2 convert<Vec2>(const String& value)
	{
		const auto& xy = value.replaced(U"(", U"").replaced(U")", U"").split(U',');
		return { convert<double>(xy[0]),convert<double>(xy[1]) };
	}

	template<>
	inline Vec3 convert<Vec3>(const String& value)
	{
		const auto& xyz = value.replaced(U"(", U"").replaced(U")", U"").split(U',');
		return { convert<double>(xyz[0]),convert<double>(xyz[1]),convert<double>(xyz[2]) };
	}


	template<class T>
	using Converter = std::function<T(StringView)>;

	template<class ...Types>
	struct Converters
	{
		std::tuple<Converter<Types>...> converters;

		Converters(Converter<Types>... converters) :converters(converters...) {};

		std::tuple<Types...> convert(const Array<String>& args)
		{
			return convert_impl(args, std::index_sequence_for<Types...>{});
		}
	private:
		template<std::size_t... Indices>
		std::tuple<Types...> convert_impl(const Array<String>& args, std::index_sequence<Indices...>)
		{
			return std::make_tuple(std::get<Indices>(converters)(args[Indices])...);
		}
	};

	//String配列をパラメータパックにしてTのコンストラクタに渡し、T*を生成して返す。
	template <typename T, class... Args>
	inline T* arrayPack(const Array<String>& arr, Converter<Args>... converters)
	{
		return new T(Converters<Args...>(converters...).convert(arr));
	}

	constexpr int MaxAnyCastDepth = 15;

	template <typename T>
	T castAnyElement(std::any& v, int depth)
	{
		if (depth <= 0)
		{
			throw std::runtime_error("std::any の再帰キャストの回数上限を超えました");
		}
		using BaseT = std::remove_reference_t<T>;

		if (v.type() == typeid(std::reference_wrapper<std::any>))
		{
			// 中身のstd::anyへの参照を取り出して再帰的に cast
			std::any& inner = std::any_cast<std::reference_wrapper<std::any>>(v).get();
			return castAnyElement<T>(inner, depth - 1);
		}
		return std::any_cast<T>(v);
	}

	template<class... Args, std::size_t... Indices>
	std::tuple<Args...> makeTupleFromArrayImpl(const Array<std::any>& arr, std::index_sequence<Indices...>)
	{
#if _DEBUG
		if (arr.size() != sizeof...(Args)) {
			throw std::invalid_argument("要素数が引数の数と一致しません。");
		}
#endif
		return std::tuple<Args...>(castAnyElement<Args>(const_cast<std::any&>(arr[Indices]), MaxAnyCastDepth)...);
	}

	template<class... Args>
	std::tuple<Args...> makeTupleFromArray(const Array<std::any>& arr)
	{
		return makeTupleFromArrayImpl<Args...>(arr, std::index_sequence_for<Args...>{});
	}

	template <class T, class... Args>
	T* arrayPack(const Array<std::any>& arr) {
		return std::apply(
			[](auto&&... args){return new T(std::forward<decltype(args)>(args)...);},
			makeTupleFromArray<Args...>(arr)
		);
	}

	// スタック変数を返す
	template <typename T, class... Args, std::size_t... Indices>
	T unpackHelper(Array<String> arr, std::index_sequence<Indices...>) {
		// パック展開を行って関数fに渡す
		return T(convert<Args>(arr[Indices])...);
	}
	template <typename T, class... Args>
	inline T unpack(Array<String> arr) {
		return unpackHelper<T, Args...>(arr, std::make_index_sequence<sizeof...(Args)>{});
	}

	template<size_t num, class T, std::enable_if_t<!std::is_convertible_v<T, String>>* = nullptr>
	constexpr auto repete(const T& elm)
	{
		std::array<T, num> result{};

		for (size_t i = 0; i < num; ++i)
		{
			result[i] = elm;
		}

		return result;
	}

	template<class T>
	std::enable_if_t<!std::is_convertible_v<T, String>, Array<T>>
		repete(const T& elm, const size_t num)
	{
		Array<T> result;
		for (auto _:step(num))
		{
			result << elm;
		}
		return result;
	}

	inline String repete(StringView elm, const size_t num)
	{
		String result;
		for (auto _ : step(num))
		{
			result += elm;
		}
		return result;
	}

	template<size_t num, size_t N>
	constexpr auto repete(const char32_t(&elm)[N])
	{
		constexpr size_t result_size = (N - 1) * num + 1;
		std::array<char32_t, result_size> result{};

		for (size_t i = 0; i < num; ++i)
		{
			for (size_t j = 0; j < N - 1; ++j)
			{
				result[i * (N - 1) + j] = elm[j];
			}
		}
		result[result_size - 1] = '\0';

		return result;
	}

	template<class Addable> class Section
	{
		Addable m_value;
		Addable m_min;
		Addable m_max;
	public:
		Section(Addable value = 0, Addable min = 0, Addable max = 0)
		{
			m_value = value;
			m_min = min;
			m_max = max;
		}

		//実際に足された分を返す
		Addable add(Addable other)
		{
			auto pre = m_value;

			m_value += other;

			Clamp<Addable>(m_value, m_min, m_max);

			return m_value - pre;
		}

		Addable get()const { return m_value; };

		void printParam()const { Print << m_value; };
	};

	class UnionFind
	{
	public:
		UnionFind(size_t n);

		size_t unite(size_t x1, size_t x2);

		size_t find(size_t x1);
	private:
		struct Impl;
		std::shared_ptr<Impl> p_impl;
	};
}
