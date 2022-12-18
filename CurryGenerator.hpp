#pragma once

#include <functional>
#include <algorithm>
#include <tuple>

namespace tomolatoon
{
	/// @brief 標準ライブラリで用意されている (std::placeholders::_1, _2, ...) 以上のインデックスを持つプレースホルダが必要になった時に使用できる型
	/// @tparam I プレースホルダインデックス
	template <int I>
	struct _
	{
		static_assert(I > 0, "不正なプレースホルダーインデックスです。プレースホルダーインデックスには自然数が必要です。");
	};
} // namespace tomolatoon

namespace std
{
	template <int I>
	struct is_placeholder<::tomolatoon::_<I>> : integral_constant<int, I>
	{};
} // namespace std

namespace tomolatoon::Curry
{
	/// @brief   Curry::To<T>::With に渡す値が invocable である場合、 invoke を行わずに invocable なオブジェクトのまま引き渡すための指示子（クラス版）
	/// @details `Curry::To<T>::With(AsValue(f))` と使用する。 `Curry::To<T>::With(f | as_value)` とも書ける
	template <class T>
	struct AsValue
	{
		/// @brief  コンストラクタ
		/// @tparam T invocable なオブジェクトの型
		/// @param  t invocable オブジェクト
		template <class T>
		AsValue(T&& t)
			: t(std::forward<T>(t))
		{}

		/// @brief  保存してある invocable オブジェクトへの参照を返す
		/// @return 保存してある invocable オブジェクト
		decltype(auto) operator()()
		{
			return t;
		}

		decltype(auto) operator()() const
		{
			return t;
		}

	private:
		/// @brief 保存している invocalbe オブジェクト
		T t;
	};

	/// @brief AsValue を `f | as_value` とチェイン出来るようにするための型
	struct AsValueChainer
	{
		template <class F>
		friend auto operator|(F f, const AsValueChainer&)
		{
			return AsValue(f);
		}

		template <class F>
		friend auto operator|(const AsValueChainer&, F f)
		{
			return AsValue(f);
		}
	};

	/// @brief  Curry::To<T>::With に渡す値が invocable である場合、 invoke を行わずに invocable なオブジェクトのまま引き渡すための指示子（チェイン版）
	/// @details Curry::To<T>::With(f | as_value) と使用する
	constexpr inline AsValueChainer as_value = {};

	/// @brief  ある型のインスタンスを遅延生成するためのクラス
	/// @tparam R 生成するインスタンスの型
	template <class R>
	struct To
	{
		/// @brief  ある型のインスタンスを遅延生成するためのクラス
		/// @tparam ...Params R のインスタンスを生成する際に渡す値の並びを指定する仮引数の型
		template <class... Params>
		struct With
		{
		private:
			/// @brief  仮引数に登場するプレースホルダに応じて、必要な実引数の数を推定する型
			/// @tparam ...Params R のインスタンスを生成する際に渡す値の並びを指定する仮引数の型
			template <class... Params>
			struct args_requirements_size
			{
				static constexpr size_t value = std::max({std::is_placeholder_v<Params>...});
			};

			/// @brief  ある1つの仮引数の情報に応じて、 R のインスタンスを生成する時に使用する値を決定する関数
			/// @tparam Param ある1つの仮引数の型
			/// @tparam ...Args 実引数の型
			/// @param  param ある1つの仮引数
			/// @param  args 実引数から生成されたタプル
			/// @return ある1つの仮引数の情報から決定された値
			template <class Param, class... Args>
			constexpr decltype(auto) get_value(Param&& param, const std::tuple<Args...>& args) const
			{
				using ParamRaw = std::remove_cvref_t<Param>;

				if constexpr (std::is_placeholder_v<ParamRaw> >= 1)
				{
					return std::get<std::is_placeholder_v<ParamRaw> - 1>(args);
				}
				else if constexpr (std::invocable<Param>)
				{
					// as_value はここで解凍される
					return std::invoke(param);
				}
				else if constexpr (std::invocable<Param, Args...>)
				{
					return std::apply(param, args);
				}
				else
				{
					return param;
				}
			}

			/// @brief  operator() の実装
			/// @tparam ...Params 仮引数の型
			/// @tparam ...Args 実引数の型
			/// @tparam I 何番目の仮引数を処理しているか (0-indexed)
			/// @param  params 仮引数から生成されたタプル
			/// @param  args 実引数から生成されたタプル
			/// @return 仮引数と実引数から生成されたタプル
			template <size_t I, class... Params, class... Args>
			constexpr auto impl(const std::tuple<Params...>& params, const std::tuple<Args...>& args) const
			{
				auto tup = std::forward_as_tuple(get_value(std::get<I>(params), args));

				if constexpr (I + 1 < sizeof...(Params))
				{
					return std::tuple_cat(std::move(tup), impl<I + 1>(params, args));
				}
				else
				{
					return tup;
				}
			}

		public:
			/// @brief  コンストラクタ
			/// @tparam ...Params 仮引数の型
			/// @param  ...params 仮引数
			template <class... Params>
			constexpr With(Params&&... params)
				: params(std::forward<Params>(params)...)
			{}

			/// @brief  実引数を渡す関数
			/// @tparam ...Args 実引数の型
			/// @param  ...args 実引数
			/// @return 仮引数と実引数から生成された R のインスタンス
			template <class... Args>
			constexpr R operator()(Args&&... args) const
			{
				if constexpr (sizeof...(Args) >= args_requirements_size<Params...>::value)
				{
					return std::apply([]<class... T>(T&&... args) { return R(std::forward<T>(args)...); },
					                  impl<0>(params, std::forward_as_tuple(std::forward<Args>(args)...)));
				}
				else
				{
					static_assert([]() { return false; }(), "渡された実引数の個数が、指定された仮引数から導出された必要数を下回っています。");
				}
			}

			operator R()
			requires(args_requirements_size<Params...>::value == 0)
			{
				return this->operator();
			}

		private:
			/// @brief 保存している仮引数
			std::tuple<Params...> params;
		};

		/// @brief 推論ガイド
		template <class... Params>
		With(Params&&...) -> With<Params...>;
	};
} // namespace tomolatoon::Curry
