#include <Siv3D.hpp> // OpenSiv3D v0.6.5

#include <concepts>
#include <ranges>
#include <format>

#include "Viewport.hpp"
#include "CurryGenerator.hpp"
#include "ExpressionFunctor.hpp"
#include "Units.hpp"

namespace tomolatoon
{
	struct ListDrawer
	{
		struct IDrawer
		{
			virtual void draw(double per) {}

			virtual ~IDrawer() = default;
		};

		size_t add(std::shared_ptr<IDrawer>&& drawer)
		{
			drawables.push_back(drawer);
			return drawables.size();
		}

		template <std::derived_from<IDrawer> Drawable, class... Args>
		size_t add(Args&&... args)
		{
			return this->add(std::make_shared<Drawable>(std::forward<Args>(args)...));
		}

		template <std::invocable<double> Lam>
		size_t add(Lam lam)
		{
			struct LambdaWrapper : IDrawer
			{
				LambdaWrapper(Lam lam)
					: lambda(std::move(lam)) {}

				void draw(double per) override
				{
					lambda(per);
				}

			private:
				Lam lambda;
			};

			return this->add(std::make_shared<LambdaWrapper>(std::move(lam)));
		}

		void draw()
		{
			// [0, drawables.size() * height) に丸める
			auto rounding = [&](auto fp) {
				if (fp < 0.0)
				{
					return drawables.size() * height + Fmod(fp, drawables.size() * height);
				}
				else
				{
					return Fmod(fp, drawables.size() * height);
				}
			};

			// 更新
			if (Iframe::Rect().mouseOver() && !MouseL.down() && (MouseL.pressed() || MouseL.up()))
			{
				diff = rounding(diff + Cursor::DeltaF().y);
			}
			else
			{
				diff = rounding(diff);
			}

			// 中央に存在するカードのサイズ
			int32  startIndex = drawables.size() - 1 - (int32)(diff / height);
			// startIndex の描画を開始する位置(y座標)
			double startY     = Iframe::Center().y + diff + height * startIndex;
			// startIndex の描画位置(Rect)
			Rect   startRect  = RectF{0, startY, Iframe::Width(), height}.asRect();

			// [0, drawables.size()) に丸めながら増減させる
			const auto inc = [&](int32& index) { index = index + 1 >= drawables.size() ? 0 : index + 1; };
			const auto dec = [&](int32& index) { index = index - 1 < 0 ? drawables.size() - 1 : index - 1; };

			// 中央のカード
			{
				ScopedIframe2D iframe(startRect);
				drawables[startIndex].get()->draw(0.0);
			}

			// 上半分のカードたち
			{
				int32 index = startIndex;
				Rect  rect  = startRect;

				// 中央のカードは描かないので進めておく
				rect.moveBy(0, -height), dec(index);

				// 0 まで許容すると intersects の判定で false を食らうので 1 以上にしておく
				for (; rect.bl().y > 0; rect.moveBy(0, -height), dec(index))
				{
					ScopedIframe2D iframe(rect);
					drawables[index].get()->draw(0.0);
				}
			}

			// 下半分のカードたち
			{
				int32 index = startIndex;
				Rect  rect  = startRect;

				// 中央のカードは描かないので進めておく
				rect.moveBy(0, height), inc(index);

				for (; rect.tl().y < Iframe::Height(); rect.moveBy(0, height), inc(index))
				{
					ScopedIframe2D iframe(rect);
					drawables[index].get()->draw(0.0);
				}
			}
		}

	private:
		int32                           height    = 100;
		double                          diff      = -(double)height / 2;
		Array<std::shared_ptr<IDrawer>> drawables = {};
	};

} // namespace tomolatoon

void Main()
{
	Effect effect;

	//Window::SetStyle(WindowStyle::Sizable);
	Window::Resize(1280, 720, Centering::Yes);

	using namespace tomolatoon::Units;
	using namespace std::placeholders;
	using namespace tomolatoon::Operators;
	using tomolatoon::Iframe, tomolatoon::Curry::To, tomolatoon::ScopedIframe2D;

	//const auto rect = To<Rect>::With(std::bind(sw, _2), _1, 40_swf, 100_shf / 7 | Ceilf);

	tomolatoon::ListDrawer drawer;

	drawer.add([](double per) { Iframe::Rect().draw(Palette::Blue); });
	drawer.add([](double per) { Iframe::Rect().draw(Palette::Green); });
	drawer.add([](double per) { Iframe::Rect().draw(Palette::Yellow); });

	while (System::Update())
	{
		ClearPrint();

		{
			ScopedIframe2D iframe(RectF(10_sw, 0, 50_sw, 100_sh).asRect());
			drawer.draw();
		}
		Line{0, Scene::CenterF().y, Scene::Width(), Scene::CenterF().y}.draw(LineStyle::Default, 1, ColorF{Palette::White, 1});
	}
}
