#pragma once

#include <concepts>
#include <type_traits>
#include <utility>
#include <cmath>

namespace tomolatoon
{
	template <class Source>
	struct ExpressionFunctor;

	template <class T>
	struct is_ExpressionFunctor : std::false_type
	{};

	template <class T>
	struct is_ExpressionFunctor<ExpressionFunctor<T>> : std::true_type
	{};

	template <class T>
	concept as_ExpressionFunctor = is_ExpressionFunctor<std::remove_cvref_t<T>>::value;

	template <class Source>
	struct ExpressionFunctor
	{
		ExpressionFunctor(Source source)
			: source(source) {}

#define TOMOLATOON_EXPRESSION_FUNCTOR_OP(op)                                                                                             \
	template <class Rhs>                                                                                                                 \
	friend auto operator##op(ExpressionFunctor& functor, Rhs&& rhs)                                                                      \
	{                                                                                                                                    \
		const auto lambda = [&functor, rhs = std::forward<Rhs>(rhs)]() { return functor.source() op rhs; };                              \
		return ExpressionFunctor<decltype(lambda)>(std::move(lambda));                                                                   \
	}                                                                                                                                    \
	template <class Rhs>                                                                                                                 \
	friend auto operator##op(ExpressionFunctor&& functor, Rhs&& rhs)                                                                     \
	{                                                                                                                                    \
		const auto lambda = [functor = std::move(functor), rhs = std::forward<Rhs>(rhs)]() { return functor.source() op rhs; };          \
		return ExpressionFunctor<decltype(lambda)>(std::move(lambda));                                                                   \
	}                                                                                                                                    \
	template <as_ExpressionFunctor Rhs>                                                                                                  \
	friend auto operator##op(ExpressionFunctor& functor, Rhs&& rhs)                                                                      \
	{                                                                                                                                    \
		const auto lambda = [&functor, rhs = std::forward<Rhs>(rhs)]() { return functor.source() op rhs.source(); };                     \
		return ExpressionFunctor<decltype(lambda)>(std::move(lambda));                                                                   \
	}                                                                                                                                    \
	template <as_ExpressionFunctor Rhs>                                                                                                  \
	friend auto operator##op(ExpressionFunctor&& functor, Rhs&& rhs)                                                                     \
	{                                                                                                                                    \
		const auto lambda = [functor = std::move(functor), rhs = std::forward<Rhs>(rhs)]() { return functor.source() op rhs.source(); }; \
		return ExpressionFunctor<decltype(lambda)>(std::move(lambda));                                                                   \
	}

		TOMOLATOON_EXPRESSION_FUNCTOR_OP(+);
		TOMOLATOON_EXPRESSION_FUNCTOR_OP(-);
		TOMOLATOON_EXPRESSION_FUNCTOR_OP(*);
		TOMOLATOON_EXPRESSION_FUNCTOR_OP(/);
		TOMOLATOON_EXPRESSION_FUNCTOR_OP(%);

		auto operator()() const
		{
			return source();
		}

	private:
		Source source;
	};

	namespace Operators
	{
		namespace detail
		{
			// clang-format off
			template <class F>
			concept unary_arithmetic_chainer_function = requires(F f, double a)
			{
				{f(a)} -> std::convertible_to<double>;
			};
			// clang-format on

			// clang-format off
			template <class F>
			concept unary_arithmetic_chainer_source_function = requires(F f)
			{
				{f()} -> std::convertible_to<double>;
			};
			// clang-format on
		} // namespace detail

		template <detail::unary_arithmetic_chainer_function F>
		struct unary_arithmetic_chainer
		{
			unary_arithmetic_chainer(F f)
				: f(f) {}

			template <detail::unary_arithmetic_chainer_source_function F>
			friend auto operator|(F f, const unary_arithmetic_chainer& self)
			{
				return [f, &self]() { return self.f(f()); };
			}

			template <std::convertible_to<double> A>
			friend double operator|(A a, const unary_arithmetic_chainer& self)
			{
				return self.f(a);
			}

		private:
			F f;
		};

		const unary_arithmetic_chainer Floorf{[](double v) { return std::floor(v); }};
		const unary_arithmetic_chainer Roundf{[](double v) { return std::round(v); }};
		const unary_arithmetic_chainer Ceilf{[](double v) { return std::ceil(v); }};
	} // namespace Operators

} // namespace tomolatoon
