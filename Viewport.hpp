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
			const auto ScopedIframe2DRect = [](Rect rect, bool isStrict) {
				const auto viewport = Graphics2D::GetViewport().value_or(Scene::Rect());
				auto       newRect  = rect;

				if (isStrict)
				{
					const auto left   = (rect.tl().x < 0 ? 0 - rect.tl().x : 0);
					const auto right  = (rect.br().x >= viewport.w ? rect.br().x - viewport.w : 0);
					const auto top    = (rect.tl().y < 0 ? 0 - rect.tl().y : 0);
					const auto bottom = (rect.br().y >= viewport.h ? rect.br().y - viewport.h : 0);

					if (!viewport.intersects(rect.movedBy(viewport.tl())))
					{
						throw Error{U"現在の Iframe 領域:Global{} と新しく作ろうとしている Iframe 領域:Local{}, Global{} とが重なっていません。新しい Iframe 領域は作成されません。"_fmt(viewport, rect, rect.movedBy(viewport.tl()))};
					}
					else if (left || right || top || bottom)
					{
						//Print << U"現在の Iframe 領域の中に、新しく作ろうとしている Iframe 領域が収まっていません。新しく作ろうとしている Iframe 領域は現在の Iframe 領域に収まるように丸められます。";
						newRect = rect.stretched(-top, -right, -bottom, -left);
					}
				}

				return newRect.movedBy(viewport.tl());
			};
		} // namespace detail

		/// @brief ScopedIframe2D の Iframe 領域の設定の仕方と、エラー報告についてを設定するフラグ
		/// No に設定すると、現在の Iframe 領域をはみ出した領域に新たな Iframe 領域を設定することが出来るようになります。
		/// Yes (default) に設定すると、現在の Iframe 領域をはみ出すように新たな Iframe 領域を設定しようとすると、現在の領域の中に収まるように丸められます。
		using ScopedIframe2DError = YesNo<struct ScopedIframe2DErrorTag>;

		struct ScopedIframe2D
			: s3d::ScopedViewport2D
			, s3d::Transformer2D
		{
			ScopedIframe2D(Rect rect, ScopedIframe2DError isError = ScopedIframe2DError::Yes)
				: s3d::ScopedViewport2D(detail::ScopedIframe2DRect(rect, isError.getBool()))
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

	namespace v2
	{
		using v1::PositionBasedIframe;
		using v1::ScopedIframe2DError;

		struct Iframe
		{
			static Rect RectAtScene()
			{
				auto rect = Graphics2D::GetScissorRect();
				return (rect.w || rect.h) ? rect : Scene::Rect();
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
				return RectAtScene().movedBy(-RectAtScene().tl());
			}

			friend struct ScopedIframe2D;
		};

		ScopedRenderStates2D CreateScissorRect(Rect rect, PositionBasedIframe b = PositionBasedIframe::No);

		namespace detail
		{
			const auto ScopedIframe2DRect = [](Rect rect, bool isStrict) {
				const auto viewport = Iframe::RectAtScene();
				auto       newRect  = rect;

				if (isStrict)
				{
					const auto left   = (rect.tl().x < 0 ? 0 - rect.tl().x : 0);
					const auto right  = (rect.br().x >= viewport.w ? rect.br().x - viewport.w : 0);
					const auto top    = (rect.tl().y < 0 ? 0 - rect.tl().y : 0);
					const auto bottom = (rect.br().y >= viewport.h ? rect.br().y - viewport.h : 0);

					if (!viewport.intersects(rect.movedBy(viewport.tl())))
					{
						throw Error{U"現在の Iframe 領域:Global{} と新しく作ろうとしている Iframe 領域:Local{}, Global{} とが重なっていません。新しい Iframe 領域は作成されません。"_fmt(viewport, rect, rect.movedBy(viewport.tl()))};
					}
					else if (left || right || top || bottom)
					{
						//Print << U"現在の Iframe 領域の中に、新しく作ろうとしている Iframe 領域が収まっていません。新しく作ろうとしている Iframe 領域は現在の Iframe 領域に収まるように丸められます。";
						newRect = rect.stretched(-top, -right, -bottom, -left);
					}
				}

				return newRect.movedBy(viewport.tl());
			};
		} // namespace detail

		struct ScopedIframe2D
			: s3d::ScopedRenderStates2D
			, s3d::Transformer2D
		{
			ScopedIframe2D(Rect rect, ScopedIframe2DError isError = ScopedIframe2DError::Yes)
				: s3d::ScopedRenderStates2D(
					CreateScissorRect(
						detail::ScopedIframe2DRect(rect, isError.getBool()),
						PositionBasedIframe::No))
				, s3d::Transformer2D(Mat3x2::Translate(rect.tl()), TransformCursor::Yes)
			{
			}
		};

	} // namespace v2
} // namespace tomolatoon
