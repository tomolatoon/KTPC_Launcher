#pragma once

#include <concepts>
#include <type_traits>
#include <utility>
#include <functional>

#include "ExpressionFunctor.hpp"
#include "Viewport.hpp"

namespace tomolatoon::Units
{
	double per(const double source, const double per) noexcept;

	double vw(const double p) noexcept;

	double vh(const double p) noexcept;

	struct vwf : ExpressionFunctor<decltype(std::bind(vw, std::declval<double>()))>
	{
		vwf(double per)
			: ExpressionFunctor(std::bind(vw, std::move(per))) {}
	};

	struct vhf : ExpressionFunctor<decltype(std::bind(vh, std::declval<double>()))>
	{
		vhf(double per)
			: ExpressionFunctor(std::bind(vh, std::move(per))) {}
	};

	double sw(const double p) noexcept;

	double sh(const double p) noexcept;

	struct swf : ExpressionFunctor<decltype(std::bind(sw, std::declval<double>()))>
	{
		swf(double per)
			: ExpressionFunctor(std::bind(sw, std::move(per))) {}
	};

	struct shf : ExpressionFunctor<decltype(std::bind(sh, std::declval<double>()))>
	{
		shf(double per)
			: ExpressionFunctor(std::bind(sh, std::move(per))) {}
	};

	inline namespace literals
	{
		double operator""_vw(const unsigned long long per) noexcept;

		double operator""_vw(const long double per) noexcept;

		double operator""_vh(const unsigned long long per) noexcept;

		double operator""_vh(const long double per) noexcept;

		vwf operator""_vwf(const unsigned long long per) noexcept;

		vwf operator""_vwf(const long double per) noexcept;

		vhf operator""_vhf(const unsigned long long per) noexcept;

		vhf operator""_vhf(const long double per) noexcept;

		double operator""_sw(const unsigned long long per) noexcept;

		double operator""_sw(const long double per) noexcept;

		double operator""_sh(const unsigned long long per) noexcept;

		double operator""_sh(const long double per) noexcept;

		swf operator""_swf(const unsigned long long per) noexcept;

		swf operator""_swf(const long double per) noexcept;

		shf operator""_shf(const unsigned long long per) noexcept;

		shf operator""_shf(const long double per) noexcept;
	} // namespace literals

} // namespace tomolatoon::Units
