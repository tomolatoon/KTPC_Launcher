#pragma once

namespace tomolatoon
{
	constexpr double Sign(double d) noexcept
	{
		return (d > 0) - (d < 0);
	}

	constexpr double rSing(double d) noexcept
	{
		return -1 * Sign(d);
	}
} // namespace tomolatoon
