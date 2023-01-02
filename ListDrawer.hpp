#pragma once

#include <Siv3D.hpp>
#include <concepts>
#include <ranges>

#include "Utility.hpp"
#include "Units.hpp"
#include "LerpTransition.hpp"

namespace tomolatoon
{
	static const auto defaultHeightf    = [&]() -> int32 { using namespace tomolatoon::Units; return static_cast<int32>(100_shf() / 7); };
	static const auto defaultmaxheightf = [&](int32 m_height) -> int32 { return static_cast<int32>(m_height * 1.3); };
	static const auto defaultcenterf    = [&]() -> int32 { return static_cast<int32>(Scene::Center().y); };

	template <
		std::invocable<>      HeightF    = decltype(defaultHeightf),
		std::invocable<int32> MaxHeightF = decltype(defaultmaxheightf),
		std::invocable<>      CenterF    = decltype(defaultcenterf)>
	// clang-format off
	requires requires(HeightF heightf, MaxHeightF maxheightf, CenterF centerf) {
		 {heightf()} -> std::same_as<int32>;
		 {maxheightf(heightf())} -> std::same_as<int32>;
		 {centerf()} -> std::same_as<int32>;
	}
	// clang-format on
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
				LambdaWrapper(Lam lam) noexcept
					: lambda(std::move(lam)) {}

				void draw(double per) override
				{
					lambda(per);
				}

			private:
				Lam lambda;
			};

			return this->add(std::make_unique<LambdaWrapper>(std::move(lam)));
		}

		template <std::invocable<double> Lam>
		void addAsArray(const Array<Lam>& lam)
		{
			std::ranges::for_each(lam, [&](Lam e) { this->add(std::move(e)); });
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

		size_t getDrawersSize() noexcept
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

		State getState() const noexcept
		{
			return m_state;
		}

		int32 getSelected() const noexcept
		{
			return m_selected;
		}

		int32 draw(bool isMouseIgnore = false)
		{
			m_toStoppingHeight.setRange(m_heightf(), m_maxHeightf(m_heightf()));

			// [0, m_drawables.size() * m_heightf()) に丸める
			auto rounding = [&](auto fp) {
				if (fp < 0.0)
				{
					return m_drawables.size() * m_heightf() + Fmod(fp, m_drawables.size() * m_heightf());
				}
				else
				{
					return Fmod(fp, m_drawables.size() * m_heightf());
				}
			};

			// state 更新
			if (!isMouseIgnore && Iframe::Rect().mouseOver() && !MouseL.down() && (MouseL.pressed() || MouseL.up()))
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
				m_toStoppingDiff.setRange(m_diff, rounding((m_drawables.size() - 1 - m_selected) * m_heightf() + (double)(m_heightf()) / 2));
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
			m_selected       = m_drawables.size() - 1 - static_cast<int32>(m_diff / m_heightf());
			// startIndex の描画を開始する位置(y座標)
			double startY    = m_centerf() + m_diff - m_heightf() * (m_drawables.size() - m_selected) - (m_toStoppingHeight.value() - m_heightf()) / 2;
			// startIndex の描画位置(Rect)
			Rect   startRect = RectF{0.0, startY, static_cast<double>(Iframe::Width()), m_toStoppingHeight.value()}.asRect();

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
					Rect  rect  = Rect{startRect.tl().movedBy(0, -m_heightf()), Iframe::Width(), m_heightf()};

					// bl は実際の領域よりも下方向に 1px はみ出していると考えられるので 0 は含まないでおく
					for (; rect.bl().y > 0; rect.moveBy(0, -m_heightf()), index = dec(index))
					{
						ScopedIframe2D iframe(rect);
						m_drawables[index].get()->draw(0.0);
					}
				}

				// 下半分のカードたち
				{
					int32 index = inc(m_selected);
					Rect  rect  = Rect{startRect.bl(), Iframe::Width(), m_heightf()};

					for (; rect.tl().y < Iframe::Height(); rect.moveBy(0, m_heightf()), index = inc(index))
					{
						ScopedIframe2D iframe(rect);
						m_drawables[index].get()->draw(0.0);
					}
				}
			}

			return m_selected;
		}

		ListDrawer() = default;

		ListDrawer(HeightF heightf, MaxHeightF maxheightf, CenterF centerf)
			: m_heightf(heightf)
			, m_maxHeightf(maxheightf)
			, m_centerf(centerf)
		{}

	private:
		using Int32Func = int32 (*)();

		State                           m_state            = State::Coasting; // 始めから停止させているとズレちゃうので、滑っていることにして停止するところからやる
		int32                           m_selected         = 0;
		HeightF                         m_heightf          = defaultHeightf;
		MaxHeightF                      m_maxHeightf       = defaultmaxheightf;
		CenterF                         m_centerf          = defaultcenterf;
		double                          m_vel              = 0.0;
		double                          m_diff             = -(double)m_heightf() / 2;
		LerpTransition                  m_toStoppingDiff   = {0.5s, 0.25s, 0.0, 0.0};
		LerpTransition                  m_toStoppingHeight = {0.1s, 0.1s, m_heightf(), m_maxHeightf(m_heightf())};
		Array<std::shared_ptr<IDrawer>> m_drawables        = {};
	};

	template <std::invocable<> HeightF, std::invocable<int32> MaxHeightF, std::invocable<> CenterF>
	// clang-format off
	requires requires(HeightF heightf, MaxHeightF maxheightf, CenterF centerf) {
		 {heightf()} -> std::same_as<int32>;
		 {maxheightf(heightf())} -> std::same_as<int32>;
		 {centerf()} -> std::same_as<int32>;
	}
	// clang-format on
	ListDrawer(HeightF, MaxHeightF, CenterF)->ListDrawer<HeightF, MaxHeightF, CenterF>;
} // namespace tomolatoon
