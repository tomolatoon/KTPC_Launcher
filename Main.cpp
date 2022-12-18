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
	double Sign(double d)
	{
		return (d > 0) - (d < 0);
	}

	double rSing(double d)
	{
		return -1 * Sign(d);
	}

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

		enum State : int16
		{
			//ButtonHandled, [[todo]]
			MouseHandled    = 0b0000'0010, // マウスで動かしてる時
			Coasting        = 0b0000'0100, // 惰性移動中
			ToStoppingFirst = 0b0000'1000, // 止まり始めた最初の1フレーム
			ToStopping      = 0b0001'0000, // 止まろうとしてる時
			Stopping        = 0b0010'0000, // 完全に止まった時
		};

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

			// state 更新
			if (Iframe::Rect().mouseOver() && !MouseL.down() && (MouseL.pressed() || MouseL.up()))
			{
				state = State::MouseHandled;
			}
			else
			{
				if (Abs(vel) < 50)
				{
					if (toStopping.isOne())
					{
						state = State::Stopping;
					}
					else
					{
						if (state != State::ToStoppingFirst && state != State::ToStopping)
						{
							state = State::ToStoppingFirst;
						}
						else
						{
							state = State::ToStopping;
						}
					}
				}
				else
				{
					state = State::Coasting;
				}
			}

			const auto toStoppingLerpCenterY = [&]() {
				return Math::Lerp(toStoppingStart, toStoppingFinish, EaseOutQuint(toStopping.value()));
			};

			// 更新
			switch (state)
			{
				using enum State;
			case MouseHandled:
			{
				vel = Clamp(Cursor::DeltaF().y / Scene::DeltaTime(), -5000.0, 5000.0);
			}
			break;
			case Coasting:
			{
				vel += rSing(vel) * 4000 * Scene::DeltaTime();
			}
			break;
			case ToStoppingFirst:
			{
				toStoppingStart  = diff;
				toStoppingFinish = rounding((drawables.size() - 1 - selected) * height + (double)height / 2);
				vel              = 0;
			}
				[[fallthrough]];
			case ToStopping:
			{
				toStoppingPrev = toStoppingLerpCenterY();
				toStopping.update(true);
			}
			break;
			case Stopping:
				break;
			}

			if (state & ~(State::ToStoppingFirst | State::ToStopping))
			{
				toStoppingStart  = 0.0;
				toStoppingPrev   = 0.0;
				toStoppingFinish = 0.0;
				toStopping.update(false, 1.0);
			}

			// 実際に真ん中に据えるカード分の座標差
			diff             = rounding(diff + vel * Scene::DeltaTime() + (toStoppingLerpCenterY() - toStoppingPrev));
			// 中央に存在するカードのインデックス
			selected         = drawables.size() - 1 - (int32)(diff / height);
			// startIndex の描画を開始する位置(y座標)
			double startY    = Iframe::Center().y + diff - height * (drawables.size() - selected);
			// startIndex の描画位置(Rect)
			Rect   startRect = RectF{0, startY, (double)Iframe::Width(), height}.asRect();

			Print << U"vel: {}\ndiff:{}\nselected:{}\ncenterY:{}\ntoStoppingLerpCenterY():{}\ncenterRect:{}\ntoStoppingStart:{}\ntoStoppingPrev:{}\ntoStoppingFinish:{}\n"_fmt(vel, diff, selected, startY, toStoppingLerpCenterY(), startRect, toStoppingStart, toStoppingPrev, toStoppingFinish);

			// 描画
			{
				// [0, drawables.size()) に丸めながら増減させる
				const auto inc = [&](int32 index) { return index + 1 >= drawables.size() ? 0 : index + 1; };
				const auto dec = [&](int32 index) { return index - 1 < 0 ? drawables.size() - 1 : index - 1; };

				// 中央のカード
				{
					ScopedIframe2D iframe(startRect);
					Iframe::Rect().draw(Palette::Azure);
					//drawables[selected].get()->draw(EaseOutQuint(toStopping.value()));
				}

				// 上半分のカードたち
				{
					int32 index = dec(selected);
					Rect  rect  = Rect{startRect.tl().movedBy(0, -height), Iframe::Width(), height};

					// bl は Rect の場合下方向に 1px はみ出していると考えられるので、 0 は含まないでおく
					for (; rect.bl().y > 0; rect.moveBy(0, -height), index = dec(index))
					{
						ScopedIframe2D iframe(rect);
						drawables[index].get()->draw(EaseOutQuint(toStopping.value()));
					}
				}

				// 下半分のカードたち
				{
					int32 index = inc(selected);
					Rect  rect  = Rect{startRect.bl(), Iframe::Width(), height};

					for (; rect.tl().y < Iframe::Height(); rect.moveBy(0, height), index = inc(index))
					{
						ScopedIframe2D iframe(rect);
						drawables[index].get()->draw(EaseOutQuint(toStopping.value()));
					}
				}
			}
		}

	private:
		Transition                      toStopping{0.5s, 0.25s};
		double                          toStoppingStart = 0, toStoppingPrev = 0, toStoppingFinish = 0;
		State                           state     = State::Coasting; // 始めから停止させているとズレちゃうので、滑っていることにして停止するところからやる
		int32                           selected  = 0;
		int32                           height    = 100;
		double                          vel       = 0.0;
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
