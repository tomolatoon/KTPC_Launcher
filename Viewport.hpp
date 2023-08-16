#pragma once

#include <Siv3D.hpp>
#include <concepts>
#include <type_traits>
#include <utility>

namespace tomolatoon
{
	namespace detail
	{
		const auto ScopedIframe2DRect = [](const Rect newLocal, const bool isCropped = true, const bool isForceIntersects = true) {
			const auto viewportGlobal  = Graphics2D::GetViewport().value_or(Scene::Rect());
			const auto viewportLocal   = viewportGlobal.movedBy(-viewportGlobal.tl());
			const auto newLocalAjusted = newLocal.stretched(newLocal.w == 0, newLocal.h == 0);
			auto       resultNewLocal  = newLocalAjusted;

			if (isForceIntersects)
			{
				if (auto newGlobal = newLocalAjusted.movedBy(viewportGlobal.tl()); not viewportGlobal.intersects(newGlobal))
				{
					throw Error{U"現在の Iframe 領域:Local{}, Global{} と新しく作ろうとしている Iframe 領域:Local{}, Global{} とが重なっていません。新しい Iframe 領域は作成されません。"_fmt(viewportLocal, viewportGlobal, newLocalAjusted, newGlobal)};
				}

				if (isCropped)
				{
					// クロップする用
					const auto left   = (newLocalAjusted.tl().x < 0 ? 0 - newLocalAjusted.tl().x : 0);
					const auto right  = (newLocalAjusted.br().x >= viewportLocal.w ? newLocalAjusted.rightX() - viewportLocal.rightX() : 0);
					const auto top    = (newLocalAjusted.tl().y < 0 ? 0 - newLocalAjusted.tl().y : 0);
					const auto bottom = (newLocalAjusted.br().y >= viewportLocal.h ? newLocalAjusted.bottomY() - viewportLocal.bottomY() : 0);

					if (left || right || top || bottom)
					{
						//Print << U"現在の Iframe 領域の中に、新しく作ろうとしている Iframe 領域が収まっていません。新しく作ろうとしている Iframe 領域は現在の Iframe 領域に収まるように丸められます。";
						resultNewLocal = newLocalAjusted.stretched(-top, -right, -bottom, -left);
					}
				}
			}

			return resultNewLocal.movedBy(viewportGlobal.tl());
		};
	} // namespace detail

	/// @brief ScopedIframe2D の Iframe 領域の設定の仕方と、エラー報告についてを設定するフラグ
	/// No に設定すると、現在の Iframe 領域をはみ出した領域に新たな Iframe 領域を設定することが出来るようになります。
	/// Yes (default) に設定すると、現在の Iframe 領域をはみ出すように新たな Iframe 領域を設定しようとすると、現在の領域の中に収まるように丸められます。
	using ScopedIframe2DForceIntersects = YesNo<struct ScopedIframe2DForceIntersectsTag>;

	using ScopedIframe2DCropped = YesNo<struct ScopedIframe2DCroppedTag>;

	struct ScopedIframe2D
		: s3d::ScopedViewport2D
		, s3d::Transformer2D
	{
		ScopedIframe2D(Rect rect, ScopedIframe2DCropped isCropped = ScopedIframe2DCropped::Yes, ScopedIframe2DForceIntersects isForceIntersects = ScopedIframe2DForceIntersects::Yes)
			: s3d::ScopedViewport2D(detail::ScopedIframe2DRect(rect, isCropped.getBool(), isForceIntersects.getBool()))
			, s3d::Transformer2D(Mat3x2::Identity(), Mat3x2::Translate(rect.tl()))
		{}
	};

	struct Iframe
	{
		static Rect RectAtScene() noexcept
		{
			return Graphics2D::GetViewport().value_or(Scene::Rect());
		}

		static Point Size() noexcept
		{
			return RectAtScene().size;
		}

		static int32 Width() noexcept
		{
			return RectAtScene().w;
		}

		static int32 Height() noexcept
		{
			return RectAtScene().h;
		}

		static Point Center() noexcept
		{
			return Rect().center().asPoint();
		}

		static Vec2 CenterF() noexcept
		{
			return Rect().center();
		}

		static s3d::Rect Rect() noexcept
		{
			return s3d::Rect{0, 0, Size()};
		}
	};

	using PositionBasedIframe = YesNo<struct PositionBasedIframe_tag>;

	ScopedRenderStates2D CreateScissorRect(Rect rect, PositionBasedIframe b = PositionBasedIframe::No);

	template <std::floating_point T = double>
	struct PercentFloat
	{
		using Func = T (*)() noexcept;

		PercentFloat(T per, Func func) noexcept
			: per(per)
			, func(func)
		{}

		PercentFloat(const PercentFloat&)            = default;
		PercentFloat(PercentFloat&&)                 = default;
		PercentFloat& operator=(const PercentFloat&) = default;
		PercentFloat& operator=(PercentFloat&&)      = default;

		PercentFloat operator=(T px) noexcept
		{
			return this->px(px);
		}

		operator T() const noexcept
		{
			return px();
		}

#define OP(op)                                \
 friend T operator##op(PercentFloat vf, T px) \
 {                                            \
  return vf.px()##op##px;                     \
 }
		OP(+)
		OP(-)
		OP(*)
		OP(/)
#undef OP

#define OP(op)                                \
 friend T operator##op(T px, PercentFloat vf) \
 {                                            \
  return vf.px()##op##px;                     \
 }
		OP(+)
		OP(-)
		OP(*)
		OP(/)
#undef OP

#define OP(op)                                                        \
 friend PercentFloat operator##op(PercentFloat lhs, PercentFloat rhs) \
 {                                                                    \
  return PercentFloat(lhs.per##op##rhs.per, lhs.func);                \
 }
		OP(+)
		OP(-)
		OP(*)
		OP(/)
#undef OP

		friend auto operator<=>(PercentFloat vf, T px)
		{
			return vf.px() <=> px;
		}

		friend auto operator==(PercentFloat vf, T px)
		{
			return vf.px() == px;
		}

		friend auto operator<=>(PercentFloat lhs, PercentFloat rhs)
		{
			return lhs.per <=> rhs.per;
		}

		friend auto operator==(PercentFloat lhs, PercentFloat rhs)
		{
			return lhs.per == rhs.per;
		}

		T px() const noexcept
		{
			return per * func();
		}

		PercentFloat& px(T px)
		{
			per = px / func();
			return *this;
		}

		PercentFloat clone() const noexcept
		{
			return *this;
		}

		T    per;
		Func func;
	};

	template <std::floating_point T = double>
	PercentFloat(T per, T (*)()) -> PercentFloat<T>;

} // namespace tomolatoon
