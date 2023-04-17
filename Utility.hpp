#pragma once

#include <optional>

#include "Settings.hpp"

#define HSV_(h, s, v)     \
 HSV                      \
 {                        \
  h, s / 100.0, v / 100.0 \
 }

namespace tomolatoon
{
	template <class T>
	using Optional = std::optional<T>;

	constexpr double Sign(double d) noexcept
	{
		return (d > 0) - (d < 0);
	}

	constexpr double rSing(double d) noexcept
	{
		return -1 * Sign(d);
	}

	bool isURL(FilePathView fp) noexcept;

	template <class T, std::equality_comparable_with<T> U, std::convertible_to<T> V>
	T getOpt(T&& val, const U& error, V&& insted) noexcept(noexcept(val == error))
	{
		if (val == error)
			return std::forward<V>(insted);
		else
			return std::forward<T>(val);
	}
} // namespace tomolatoon
