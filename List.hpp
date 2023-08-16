#pragma once

#include <Siv3D.hpp>
#include <concepts>
#include <ranges>

#include "Utility.hpp"
#include "Units.hpp"
#include "LerpTransition.hpp"

namespace tomolatoon
{
	namespace List
	{
		struct IDrawer
		{
			virtual String name() const = 0;

			virtual void draw(double per, double stopTime) const {}

			virtual ~IDrawer() = default;
		};

		struct Id
		{
			bool operator==(const Id&) const = default;

			std::strong_ordering operator<=>(const Id&) const = default;

			const size_t id;
		};
	} // namespace List
} // namespace tomolatoon

template <>
struct std::hash<tomolatoon::List::Id>
{
	size_t operator()(const tomolatoon::List::Id& id) const noexcept
	{
		return id.id;
	}
};

namespace tomolatoon
{
	namespace List
	{
		// Drawables の辞書
		struct DrawablesDic
		{
			inline static struct Unavailable final : IDrawer
			{
				String name() const override
				{
					return U"Unavailable";
				}

				void draw(double per, double stopTime) const override {}
			} unavailable;

			size_t size() const noexcept
			{
				return m_drawablesDic.size();
			}

			Id add(std::unique_ptr<IDrawer>&& drawer)
			{
				Id id = {m_drawablesDic.size()};

				m_drawablesDic.emplace(id, std::move(drawer));

				return id;
			}

			template <class Lam>
			requires(not std::indirectly_readable<Lam>)
			Id add(Lam lam)
			{
				struct LambdaWrapper : IDrawer
				{
					LambdaWrapper(Lam lam) noexcept
						: m_lambda(std::move(lam)) {}

					String name() const override
					{
						return m_name;
					}

					void draw(double per, double stopTime) const override
					{
						m_lambda(per, stopTime);
					}

				private:
					Lam          m_lambda;
					const String m_name = U"LambdaWrapper[{}]"_fmt(Scene::Time());
				};

				return add(std::make_unique<LambdaWrapper>(std::move(lam)));
			}

			void erase(Id id)
			{
				m_drawablesDic.erase(id);
			}

			bool contains(Id id) const
			{
				return m_drawablesDic.contains(id);
			}

			IDrawer& getById(Id id) noexcept
			{
				if (contains(id))
				{
					return *m_drawablesDic.at(id);
				}
				else
				{
					return unavailable;
				}
			}

			const IDrawer& getById(Id id) const noexcept
			{
				if (contains(id))
				{
					return *m_drawablesDic.at(id);
				}
				else
				{
					return unavailable;
				}
			}

		private:
			HashTable<Id, std::unique_ptr<IDrawer>> m_drawablesDic = {};
		};

		// スライダーを掴んでいる状態であるかどうかを示す。isPressed が true なら加速度の計算を行う、のように使用する。
		struct HoldingState
		{
			// 特殊な条件で Rect の一部をドラッグ開始としては無視するときに true にする。
			bool update(Rect rect, bool isIgnoreWhenStart = false) noexcept
			{
				if (m_pressed)
				{
					// 前フレームで掴まれていれば、領域外に出ていても押し続けられていればよい
					m_pressed = MouseL.pressed();
				}
				else
				{
					// 掴みの開始は領域内から
					m_pressed = rect.leftClicked() && !isIgnoreWhenStart;
				}

				return isPressed();
			}

			bool isPressed() const noexcept
			{
				return m_pressed;
			}

		private:
			bool m_pressed = false;
		};

		// Update と Draw とで共有する変数
		struct Context
		{
			enum State : int16
			{
				//ButtonHandled, [[todo]]
				MouseHandled    = 0b00000010, // マウスで動かしてる時
				Coasting        = 0b00000100, // 惰性移動中
				ToStoppingFirst = 0b00001000, // 止まり始めた最初の1フレーム
				ToStopping      = 0b00010000, // 止まろうとしてる時
				StoppingFirst   = 0b00100000, // 完全に止まった最初の1フレーム
				Stopping        = 0b01000000, // 完全に止まった時
			};

			double cardHeight() const noexcept
			{
				return Units::sh(100.0 / 7);
			}

			double cardHeightMax() const noexcept
			{
				return cardHeight() * 1.1;
			}

			double sliderHeight() const noexcept
			{
				return cardHeight() * ary.size();
			}

			friend struct CardProp;

			template <class Context>
			struct CardProp
			{
				CardProp(Context& context, double diff)
					: context(context)
					, m_diff(diff) {}

				// cur: 現在の Diff
				// prev: 前回に止まっていた時の Diff
				double diff() const noexcept
				{
					return m_diff;
				}

				// cur: 中央に存在するカードのインデックス
				// prev: 前回に止まっていた時の中央に存在するカードのインデックス
				size_t index() const noexcept
				{
					return context.ary.size() - 1 - static_cast<size_t>(m_diff / context.cardHeight());
				}

				// cur:　中央に存在するカードの登録Id
				// prev: 前回に止まっていた時の中央に存在するカードの登録Id
				Id id() const noexcept
				{
					return context.ary[index()];
				}

				// cur: 中央に存在するカードの描画クラス
				// prev: 前回に止まっていた時の中央に存在するカードの描画クラス
				IDrawer& drawer() noexcept
				requires(not std::is_const_v<Context>)
				{
					return context.dic.getById(id());
				}

				const IDrawer& drawer() const noexcept
				{
					return context.dic.getById(id());
				}

			private:
				Context& context;
				double   m_diff;
			};

			template <class Self>
			auto cur(this Self& self) noexcept
			{
				return CardProp(self, self.m_diff);
			}

			template <class Self>
			auto prev(this Self& self) noexcept
			{
				return CardProp(self, self.m_prevStoppedDiff);
			}

			// State 設定を先にやることを推奨
			Context& diff(double diff) noexcept
			{
				if (state & (State::StoppingFirst | State::ToStoppingFirst | State::ToStopping))
				{
					m_prevStoppedDiff = m_diff;
				}

				m_diff = modulo(diff, sliderHeight());

				return *this;
			}

			Context& vel(double vel) noexcept
			{
				m_vel = Clamp(vel, -VelMax, VelMax);
				return *this;
			}

			double vel() const noexcept
			{
				return m_vel;
			}

			double stoppingTime() const noexcept
			{
				return Scene::Time() - lastStopTime;
			}

			double velUnderThreshold() const noexcept
			{
				return 1'000;
			}

			double deVel() const noexcept
			{
				// 隣に移動するときなどに使いやすいように減速は急に
				// 遠くまで移動するときはある程度滑るように。
				if (Abs(vel()) > 2'000)
				{
					return -vel() * 0.05;
				}
				else
				{
					return -vel() * 0.6;
				}
			}

		public:
			DrawablesDic   dic              = {};
			Array<Id>      ary              = {};
			State          state            = State::ToStoppingFirst;
			bool           isMouseIgnore    = false;
			LerpTransition toStoppingDiff   = {0.5s, 0.25s, 0.0, 0.0};
			LerpTransition toStoppingHeight = {0.05s, 0.05s, cardHeight(), cardHeightMax()};
			double         lastStopTime     = 0.0;
			HoldingState   holdingState     = {};

		public:
			inline static constexpr double VelMax = 100'000;

		private:
			double m_diff            = modulo(-cardHeight() / 2, sliderHeight());
			double m_prevStoppedDiff = modulo(-cardHeight() / 2, sliderHeight());
			double m_vel             = 0;
		}; // namespace List

		struct Update
		{
			Id update(Context& context)
			{
				using State = Context::State;

				// [0, m_drawables.size() * context.cardHeight()) に丸める
				const auto rounding = [&](auto fp) {
					return modulo(fp, context.sliderHeight());
				};

				context.toStoppingHeight.setRange(context.cardHeight(), context.cardHeightMax());

				// state 更新
				if (context.holdingState.update(Iframe::Rect(), context.isMouseIgnore))
				{
					context.state = Context::State::MouseHandled;
				}
				else
				{
					if (Abs(context.vel()) < context.velUnderThreshold())
					{
						if (context.toStoppingHeight.isFinish())
						{
							if (context.state != State::StoppingFirst && context.state != State::Stopping)
							{
								context.state = State::StoppingFirst;
							}
							else
							{
								context.state = State::Stopping;
							}
						}
						else
						{
							if (context.state != State::ToStoppingFirst && context.state != State::ToStopping)
							{
								context.state = State::ToStoppingFirst;
							}
							else
							{
								context.state = State::ToStopping;
							}
						}
					}
					else
					{
						context.state = State::Coasting;
					}
				}

				// state 依存の更新
				switch (context.state)
				{
					using enum State;
				case MouseHandled:
				{
					context.vel(Cursor::DeltaF().y / Scene::DeltaTime());
				}
				break;
				case Coasting:
				{
					context.vel(context.vel() + context.deVel());
				}
				break;
				case ToStoppingFirst:
				{
					context.toStoppingDiff.setRange(context.cur().diff(), rounding((context.ary.size() - 1 - context.cur().index()) * context.cardHeight() + (double)(context.cardHeight()) / 2));
					context.lastStopTime = Scene::Time();
					context.vel(0);
				}
					[[fallthrough]];
				case ToStopping:
				{
					context.toStoppingDiff.updateByDeltaSec(true);
					context.toStoppingHeight.updateByDeltaSec(true);
				}
				break;
				case StoppingFirst:
				case Stopping:
					break;
				}

				if (context.state & ~(State::ToStoppingFirst | State::ToStopping))
				{
					context.toStoppingDiff.setRange(0.0, 0.0);
					context.toStoppingDiff.updateByDeltaSec(false, 1.0);
				}

				if (context.state & ~(State::ToStoppingFirst | State::ToStopping | State::StoppingFirst | State::Stopping))
				{
					context.toStoppingHeight.updateByDeltaSec(false);
				}

				// 実際に真ん中に据えるカード分の座標差
				context.diff(rounding(context.cur().diff() + context.vel() * Scene::DeltaTime() + context.toStoppingDiff.deltaValue(EaseOutQuint)));

				return context.cur().id();
			}
		};

		struct Draw
		{
			void draw(const Context& context) const
			{
				// startIndex の描画を開始する位置(y座標)
				const double startY = Scene::CenterF().y + Fmod(context.cur().diff() - 1, context.cardHeight()) - context.cardHeight() - (context.toStoppingHeight.value() - context.cardHeight()) / 2;

				// startIndex の描画位置(Rect)
				const Rect startRect = RectF{0.0, startY, Iframe::Width(), context.toStoppingHeight.value()}.asRect();

				// [0, context.ary.size()) に丸めながら増減させる
				const auto inc = [&](int32 index) { return index + 1 >= (int32)(context.ary.size()) ? 0 : index + 1; };
				const auto dec = [&](int32 index) { return index - 1 < 0 ? (int32)(context.ary.size() - 1) : index - 1; };

				// 中央のカード
				{
					ScopedIframe2D iframe(startRect);
					context.cur().drawer().draw(context.toStoppingDiff.transition(EaseOutQuint), context.stoppingTime());
				}

				// 上半分のカードたち
				{
					int32 index = dec(context.cur().index());
					Rect  rect  = RectF{startRect.tl().movedBy(0, -context.cardHeight()), Iframe::Width(), context.cardHeight()}.asRect();

					// bl は実際の領域よりも下方向に 1px はみ出していると考えられるので 0 は含まないでおく
					for (; rect.bottomY() > 0; rect.moveBy(0, -context.cardHeight()), index = dec(index))
					{
						ScopedIframe2D iframe(rect, ScopedIframe2DCropped::No);
						context.dic.getById(context.ary[index]).draw(0.0, context.stoppingTime());
					}
				}

				// 下半分のカードたち
				{
					int32 index = inc(context.cur().index());
					Rect  rect  = RectF{startRect.bl(), Iframe::Width(), context.cardHeight()}.asRect();

					for (; rect.topY() < Iframe::Height(); rect.moveBy(0, context.cardHeight()), index = inc(index))
					{
						ScopedIframe2D iframe(rect, ScopedIframe2DCropped::No);
						context.dic.getById(context.ary[index]).draw(0.0, context.stoppingTime());
					}
				}
			}
		};

	} // namespace List
} // namespace tomolatoon
