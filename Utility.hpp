#pragma once

#include <optional>

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
} // namespace tomolatoon
