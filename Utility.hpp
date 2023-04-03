#pragma once

#include <optional>

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
} // namespace tomolatoon
