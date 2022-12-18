#pragma once

#include <concepts>
#include <type_traits>
#include <utility>
#include <functional>
#include "ExpressionFunctor.hpp"
#include "Viewport.hpp"

namespace tomolatoon
{
	namespace Units
	{
		double per(double source, double per)
		{
			return per * 0.01 * source;
		}

		double vw(double p)
		{
			return per(Iframe::Width(), p);
		}

		double vh(double p)
		{
			return per(Iframe::Height(), p);
		}

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

		double sw(double p)
		{
			return per(Scene::Width(), p);
		}

		double sh(double p)
		{
			return per(Scene::Height(), p);
		}

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
			auto operator""_vw(unsigned long long per)
			{
				return vw((double)per);
			}

			auto operator""_vw(long double per)
			{
				return vw((double)per);
			}

			auto operator""_vh(unsigned long long per)
			{
				return vh((double)per);
			}

			auto operator""_vh(long double per)
			{
				return vh((double)per);
			}

			auto operator""_vwf(unsigned long long per)
			{
				return vwf((double)per);
			}

			auto operator""_vwf(long double per)
			{
				return vwf((double)per);
			}

			auto operator""_vhf(unsigned long long per)
			{
				return vhf((double)per);
			}

			auto operator""_vhf(long double per)
			{
				return vhf((double)per);
			}

			auto operator""_sw(unsigned long long per)
			{
				return sw((double)per);
			}

			auto operator""_sw(long double per)
			{
				return sw((double)per);
			}

			auto operator""_sh(unsigned long long per)
			{
				return sh((double)per);
			}

			auto operator""_sh(long double per)
			{
				return sh((double)per);
			}

			auto operator""_swf(unsigned long long per)
			{
				return swf((double)per);
			}

			auto operator""_swf(long double per)
			{
				return swf((double)per);
			}

			auto operator""_shf(unsigned long long per)
			{
				return shf((double)per);
			}

			auto operator""_shf(long double per)
			{
				return shf((double)per);
			}
		} // namespace literals

	} // namespace Units

} // namespace tomolatoon
