#include "Units.hpp"

namespace tomolatoon::Units
{
	double per(const double source, const double per) noexcept
	{
		return per * 0.01 * source;
	}

	double vw(const double p) noexcept
	{
		return per(Iframe::Width(), p);
	}

	double vh(const double p) noexcept
	{
		return per(Iframe::Height(), p);
	}

	double sw(const double p) noexcept
	{
		return per(Scene::Width(), p);
	}

	double sh(const double p) noexcept
	{
		return per(Scene::Height(), p);
	}

	inline namespace literals
	{
		double operator""_vw(const unsigned long long per) noexcept
		{
			return vw((double)per);
		}

		double operator""_vw(const long double per) noexcept
		{
			return vw((double)per);
		}

		double operator""_vh(const unsigned long long per) noexcept
		{
			return vh((double)per);
		}

		double operator""_vh(const long double per) noexcept
		{
			return vh((double)per);
		}

		vwf operator""_vwf(const unsigned long long per) noexcept
		{
			return vwf((double)per);
		}

		vwf operator""_vwf(const long double per) noexcept
		{
			return vwf((double)per);
		}

		vhf operator""_vhf(const unsigned long long per) noexcept
		{
			return vhf((double)per);
		}

		vhf operator""_vhf(const long double per) noexcept
		{
			return vhf((double)per);
		}

		double operator""_sw(const unsigned long long per) noexcept
		{
			return sw((double)per);
		}

		double operator""_sw(const long double per) noexcept
		{
			return sw((double)per);
		}

		double operator""_sh(const unsigned long long per) noexcept
		{
			return sh((double)per);
		}

		double operator""_sh(const long double per) noexcept
		{
			return sh((double)per);
		}

		swf operator""_swf(const unsigned long long per) noexcept
		{
			return swf((double)per);
		}

		swf operator""_swf(const long double per) noexcept
		{
			return swf((double)per);
		}

		shf operator""_shf(const unsigned long long per) noexcept
		{
			return shf((double)per);
		}

		shf operator""_shf(const long double per) noexcept
		{
			return shf((double)per);
		}
	} // namespace literals
}
