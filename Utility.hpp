#pragma once

#include <optional>
#include <compare>
#include <concepts>

#include "Settings.hpp"

#define HSV_(h, s, v)           \
	HSV                         \
	{                           \
		h, s / 100.0, v / 100.0 \
	}

namespace tomolatoon
{
	template <class T>
	using Optional = std::optional<T>;

	constexpr double Sign(double d) noexcept
	{
		return (int)(d > 0) - (int)(d < 0);
	}

	constexpr double rSign(double d) noexcept
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

	template <std::floating_point F>
	F modulo(F val, F mod)
	{
		if (mod)
		{
			return val > 0 ? Fmod(val, mod) : mod + Fmod(val, mod);
		}
		else
		{
			return val;
		}
	}

	namespace Cursor
	{
		void Update() noexcept;

		Point Delta() noexcept;

		Vec2 DeltaF() noexcept;

		Vec2 Velocity() noexcept;

		Vec2 PreviousVelocity() noexcept;

		Vec2 Acceleration() noexcept;
	} // namespace Cursor

} // namespace tomolatoon
