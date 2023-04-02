#pragma once

#include <Siv3D.hpp>
#include <concepts>
#include <type_traits>
#include <utility>

namespace tomolatoon
{
	inline namespace v1
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
			static Rect RectAtScene()
			{
				return Graphics2D::GetViewport().value_or(Scene::Rect());
			}

			static Point Size()
			{
				return RectAtScene().size;
			}

			static int32 Width()
			{
				return RectAtScene().w;
			}

			static int32 Height()
			{
				return RectAtScene().h;
			}

			static Point Center()
			{
				return Rect().center().asPoint();
			}

			static Vec2 CenterF()
			{
				return Rect().center();
			}

			static s3d::Rect Rect()
			{
				return s3d::Rect{0, 0, Size()};
			}
		};

		using PositionBasedIframe = YesNo<struct PositionBasedIframe_tag>;

		ScopedRenderStates2D CreateScissorRect(Rect rect, PositionBasedIframe b = PositionBasedIframe::No);
	} // namespace v1

} // namespace tomolatoon
