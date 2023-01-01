#include <Siv3D.hpp> // OpenSiv3D v0.6.5

#include <concepts>
#include <ranges>
#include <format>
#include <filesystem>

#include "Viewport.hpp"
#include "CurryGenerator.hpp"
#include "ExpressionFunctor.hpp"
#include "LerpTransition.hpp"
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
			m_drawables.push_back(drawer);
			return m_drawables.size();
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

		template <class Head1, class Head2, class... Args>
		void addAsArgs(Head1&& head1, Head2&& head2, Args&&... args)
		{
			add(std::forward<Head1>(head1));
			add(std::forward<Head2>(head2));
			addAsArgs(std::forward<Args>(args)...);
		}

		template <class Head1>
		void addAsArgs(Head1&& head)
		{
			add(std::forward<Head1>(head));
		}

		IDrawer& getDrawer(int32 i)
		{
			return *m_drawables.at(i).get();
		}

		size_t getDrawersSize()
		{
			return m_drawables.size();
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

		State getState() const
		{
			return m_state;
		}

		int32 getSelected() const
		{
			return m_selected;
		}

		int32 draw()
		{
			// [0, m_drawables.size() * m_height) に丸める
			auto rounding = [&](auto fp) {
				if (fp < 0.0)
				{
					return m_drawables.size() * m_height + Fmod(fp, m_drawables.size() * m_height);
				}
				else
				{
					return Fmod(fp, m_drawables.size() * m_height);
				}
			};

			// state 更新
			if (Iframe::Rect().mouseOver() && !MouseL.down() && (MouseL.pressed() || MouseL.up()))
			{
				m_state = State::MouseHandled;
			}
			else
			{
				if (Abs(m_vel) < 100)
				{
					if (m_toStoppingDiff.isFinish())
					{
						m_state = State::Stopping;
					}
					else
					{
						if (m_state != State::ToStoppingFirst && m_state != State::ToStopping)
						{
							m_state = State::ToStoppingFirst;
						}
						else
						{
							m_state = State::ToStopping;
						}
					}
				}
				else
				{
					m_state = State::Coasting;
				}
			}

			// 更新
			switch (m_state)
			{
				using enum State;
			case MouseHandled:
			{
				m_vel = Clamp(Cursor::DeltaF().y / Scene::DeltaTime(), -5000.0, 5000.0);
			}
			break;
			case Coasting:
			{
				m_vel += rSing(m_vel) * 4000 * Scene::DeltaTime();
			}
			break;
			case ToStoppingFirst:
			{
				m_toStoppingDiff.setRange(m_diff, rounding((m_drawables.size() - 1 - m_selected) * m_height + (double)m_height / 2));
				m_vel = 0;
			}
				[[fallthrough]];
			case ToStopping:
			{
				m_toStoppingDiff.updateByDeltaSec(true);
				m_toStoppingHeight.updateByDeltaSec(true);
			}
			break;
			case Stopping:
				break;
			}

			if (m_state & ~(State::ToStoppingFirst | State::ToStopping | State::Stopping))
			{
				m_toStoppingDiff.setRange(0.0, 0.0);
				m_toStoppingDiff.updateByDeltaSec(false, 1.0);
				m_toStoppingHeight.updateByDeltaSec(false);
			}

			// 実際に真ん中に据えるカード分の座標差
			m_diff           = rounding(m_diff + m_vel * Scene::DeltaTime() + m_toStoppingDiff.deltaValue(EaseOutQuint));
			// 中央に存在するカードのインデックス
			m_selected       = m_drawables.size() - 1 - (int32)(m_diff / m_height);
			// startIndex の描画を開始する位置(y座標)
			double startY    = m_center() + m_diff - m_height * (m_drawables.size() - m_selected) - (m_toStoppingHeight.value() - m_height) / 2;
			// startIndex の描画位置(Rect)
			Rect   startRect = RectF{0.0, startY, (double)Iframe::Width(), m_toStoppingHeight.value()}.asRect();

			Print << U"m_vel: {}\ndiff:{}\nselected:{}\ncenterY:{}\ncenterRect:{}"_fmt(m_vel, m_diff, m_selected, startY, startRect);

			// 描画
			{
				// [0, m_drawables.size()) に丸めながら増減させる
				const auto inc = [&](int32 index) { return index + 1 >= m_drawables.size() ? 0 : index + 1; };
				const auto dec = [&](int32 index) { return index - 1 < 0 ? m_drawables.size() - 1 : index - 1; };

				// 中央のカード
				{
					ScopedIframe2D iframe(startRect);
					//Iframe::Rect().draw(Palette::Azure);
					m_drawables[m_selected].get()->draw(m_toStoppingDiff.transition(EaseOutQuint));
				}

				// 上半分のカードたち
				{
					int32 index = dec(m_selected);
					Rect  rect  = Rect{startRect.tl().movedBy(0, -m_height), Iframe::Width(), m_height};

					// bl は実際の領域よりも下方向に 1px はみ出していると考えられるので 0 は含まないでおく
					for (; rect.bl().y > 0; rect.moveBy(0, -m_height), index = dec(index))
					{
						ScopedIframe2D iframe(rect);
						m_drawables[index].get()->draw(0.0);
					}
				}

				// 下半分のカードたち
				{
					int32 index = inc(m_selected);
					Rect  rect  = Rect{startRect.bl(), Iframe::Width(), m_height};

					for (; rect.tl().y < Iframe::Height(); rect.moveBy(0, m_height), index = inc(index))
					{
						ScopedIframe2D iframe(rect);
						m_drawables[index].get()->draw(0.0);
					}
				}
			}

			return m_selected;
		}

	private:
		using Int32Func = int32 (*)();

		State                           m_state     = State::Coasting; // 始めから停止させているとズレちゃうので、滑っていることにして停止するところからやる
		int32                           m_selected  = 0;
		int32                           m_height    = 110;
		int32                           m_maxHeight = m_height * 1.3;
		Int32Func                       m_center    = []() { return Scene::Center().y; }; // 関数にするか普通の値にするかは検討
		double                          m_vel       = 0.0;
		double                          m_diff      = -(double)m_height / 2;
		LerpTransition                  m_toStoppingDiff{0.5s, 0.25s, 0.0, 0.0};
		LerpTransition                  m_toStoppingHeight{0.1s, 0.1s, m_height, m_maxHeight};
		Array<std::shared_ptr<IDrawer>> m_drawables = {};
	};
} // namespace tomolatoon

void Main()
{
	JSON                   settings;
	tomolatoon::ListDrawer drawer;
	Array<Texture>         icons;

	const Array<String>& args = System::GetCommandLineArgs();

	if (args.size() > 1)
	{
		std::filesystem::path jsonPath = args[1].toUTF32();

		if (jsonPath.extension() == U".json")
		{
			settings = settings.Load(jsonPath.u32string());
		}

		for (const auto& games = settings[U"games"]; auto&& game : games.arrayView())
		{
			std::filesystem::path iconPath = (jsonPath.parent_path().u32string() + game[U"icon"].get<String>().toUTF32());

			icons.push_back(Texture{iconPath.u32string()});
		}
	}

	//Window::SetStyle(WindowStyle::Sizable);
	Window::Resize(1755, 810, Centering::Yes);

	using namespace tomolatoon::Units;
	using namespace std::placeholders;
	using namespace tomolatoon::Operators;
	using tomolatoon::Iframe, tomolatoon::Curry::To, tomolatoon::ScopedIframe2D;

	//const auto rect = To<Rect>::With(std::bind(sw, _2), _1, 40_swf, 100_shf / 7 | Ceilf);

	drawer.addAsArgs(
		[](double per) { Iframe::Rect().draw(Color(63, 203, 187)); },
		[](double per) { Iframe::Rect().draw(Color(255, 204, 17)); },
		[](double per) { Iframe::Rect().draw(Color(221, 68, 68)); });

	while (System::Update())
	{
		const auto sliderStart = 17.25_sw;

		ClearPrint();

		// List
		{
			ScopedIframe2D iframe(RectF(sliderStart, 0, 39_sw, 90_sh).asRect());
			drawer.draw();
		}
		// 画像
		{
			icons[drawer.getSelected()].resized(31.5_sw).draw(59.5_sw, 15.5_sh);
			//RectF{59.5_sw, 15.5_sh, 31.5_sw}.asRect().draw(Palette::Aqua);
		}
		// その他
		{
			Line{0, 50_sh, Scene::Width(), 50_sh}.draw(LineStyle::Default, 1, ColorF{Palette::White, 1});
			RectF{sliderStart, 90_sh, 85_sw, 10_sh}.asRect().draw(Palette::White);
		}
		drawer.getDrawer(drawer.getSelected());
	}
}
