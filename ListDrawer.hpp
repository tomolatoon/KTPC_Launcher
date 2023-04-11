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
	static const auto defaultmaxheightf = [&](int32 m_height) -> int32 { return static_cast<int32>(m_height * 1.1); };
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
		using ID = int32;

		struct IDrawer
		{
			virtual void draw(double per, double stopTime) const {}

			virtual ~IDrawer() = default;
		};

		ID registerDrawable(bool isRefPushBack, std::unique_ptr<IDrawer>&& drawer)
		{
			ID id = m_drawablesDic.size();
			m_drawablesDic.emplace(id, std::move(drawer));

			if (isRefPushBack)
			{
				m_drablesRef.push_back(id);
			}

			return id;
		}

		template <std::derived_from<IDrawer> Drawable, class... Args>
		ID registerDrawable(bool isRefPushBack, Args&&... args)
		{
			return registerDrawable(isRefPushBack, std::make_unique<Drawable>(std::forward<Args>(args)...));
		}

		template <std::invocable<double, double> Lam>
		ID registerDrawable(bool isRefPushBack, Lam lam)
		{
			struct LambdaWrapper : IDrawer
			{
				LambdaWrapper(Lam lam) noexcept
					: lambda(std::move(lam)) {}

				void draw(double per, double stopTime) const override
				{
					lambda(per, stopTime);
				}

			private:
				Lam lambda;
			};

			return registerDrawable(isRefPushBack, std::make_unique<LambdaWrapper>(std::move(lam)));
		}

		template <std::invocable<double, double> Lam>
		Array<ID> registerDrawableAsArray(bool isRefPushBack, const Array<Lam>& lam)
		{
			return lam | std::views::transform([&](auto&& e) { return registerDrawable(isRefPushBack, std::move(e)); }) | std::ranges::to<Array<ID>>();
		}

		template <class... Args>
		Array<ID> registerDrawableAsArgs(bool isRefPushBack, Args&&... args)
		{
			return {add(isRefPushBack, std::forward<Args>(args))...};
		}

		bool unregister(ID id)
		{
			if (m_drawablesDic.contains(id))
			{
				m_drawablesDic.erase(m_drawablesDic.find(id));

				return true;
			}
			else
			{
				return false;
			}
		}

		void pushFrontRef(ID id)
		{
			selectedRefIndex(selectedRefIndex() + 1);

			m_drablesRef.push_front(id);
		}

		void pushBackRef(ID id)
		{
			m_drablesRef.push_back(id);
		}

		// 指定した index の直前に挿入
		void insertRef(size_t index, ID id)
		{
			if (index <= selectedRefIndex())
			{
				selectedRefIndex(selectedRefIndex() + 1);
			}

			m_drablesRef.insert(m_drablesRef.begin() + index, id);
		}

		void eraseRef(size_t index)
		{
			if (index <= selectedRefIndex())
			{
				selectedRefIndex(selectedRefIndex() - 1);
			}

			m_drablesRef.erase(m_drablesRef.begin() + index);
		}

		size_t sizeRefs() const noexcept
		{
			return m_drablesRef.size();
		}

		size_t selectedRefIndex() const noexcept
		{
			return m_selected;
		}

		void selectedRefIndex(int32 selectedRefIndex) noexcept
		{
			if (sizeRefs())
			{
				m_prevSelected = m_selected;
				m_selected = selectedRefIndex % sizeRefs();
			}
		}

		size_t prevSelectedRefIndex() const noexcept
		{
			return m_prevSelected;
		}

		bool isChangedSelectedRefIndex() const noexcept
		{
			return prevSelectedRefIndex() != selectedRefIndex();
		}

		ID selectedDrawableId() const noexcept
		{
			return m_drablesRef[selectedRefIndex()];
		}

		const IDrawer& drawable(ID i) const
		{
			return *(*m_drawablesDic.find(i)).second;
		}

		const IDrawer& selectedDrawable() const
		{
			return drawable(selectedDrawableId());
		}

		enum State : int16
		{
			//ButtonHandled, [[todo]]
			MouseHandled    = 0b0000'0010, // マウスで動かしてる時
			Coasting        = 0b0000'0100, // 惰性移動中
			ToStoppingFirst = 0b0000'1000, // 止まり始めた最初の1フレーム
			ToStopping      = 0b0001'0000, // 止まろうとしてる時
			StoppingFirst   = 0b0010'0000, // 完全に止まった最初の1フレーム
			Stopping        = 0b0100'0000, // 完全に止まった時
		};

		State getState() const noexcept
		{
			return m_state;
		}

		/// @brief 前回停止してからの時間[s]
		double stopTime() const noexcept
		{
			// あるフレームの中では同じ値が返されるので OK
			return Scene::Time() - m_lastStopTime;
		}

		/// @brief 中央の要素の拡大の割合[0.0, 1.0]
		double getTransition() const noexcept
		{
			return m_toStoppingDiff.transition(EaseOutQuint);
		}

		/// @brief 中央の要素が最大担っている時かどうか
		bool isMaximum() const noexcept
		{
			return m_toStoppingDiff.isFinish();
		}

		/// @brief 中央の要素が最大担っている時かどうか
		bool isMinimum() const noexcept
		{
			return m_toStoppingDiff.isStart();
		}

		bool isStopped() const noexcept
		{
			return getState() & (StoppingFirst | Stopping);
		}

		ID update(bool isMouseIgnore = false)
		{
			// [0, m_drawables.size() * m_heightf()) に丸める
			const auto rounding = [&](auto fp) {
				if (fp < 0.0)
				{
					return sizeRefs() * m_heightf() + Fmod(fp, sizeRefs() * m_heightf());
				}
				else
				{
					return Fmod(fp, sizeRefs() * m_heightf());
				}
			};

			// drawer の領域の大きさが途中で変わっても、真ん中の要素の高さを追従する
			m_toStoppingHeight.setRange(m_heightf(), m_maxHeightf(m_heightf()));

			// state 更新
			if (!isMouseIgnore && Iframe::Rect().mouseOver() && !MouseL.down() && (MouseL.pressed() || MouseL.up()))
			{
				m_state = State::MouseHandled;
			}
			else
			{
				if (Abs(m_vel) < 2000)
				{
					if (m_toStoppingDiff.isFinish())
					{
						if (m_state != State::StoppingFirst && m_state != State::Stopping)
						{
							m_state = State::StoppingFirst;
						}
						else
						{
							m_state = State::Stopping;
						}
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

			// state 依存の更新
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
				m_toStoppingDiff.setRange(m_diff, rounding((sizeRefs() - 1 - selectedRefIndex()) * m_heightf() + (double)(m_heightf()) / 2));
				m_vel = 0;
			}
				[[fallthrough]];
			case ToStopping:
			{
				m_toStoppingDiff.updateByDeltaSec(true);
				m_toStoppingHeight.updateByDeltaSec(true);
			}
			break;
			case StoppingFirst:
			case Stopping:
				break;
			}

			if (m_state & ~(State::ToStoppingFirst | State::ToStopping | State::StoppingFirst | State::Stopping))
			{
				m_toStoppingDiff.setRange(0.0, 0.0);
				m_toStoppingDiff.updateByDeltaSec(false, 1.0);
				m_toStoppingHeight.updateByDeltaSec(false);
			}

			if (m_state & ~(State::StoppingFirst | State::Stopping))
			{
				m_lastStopTime = Scene::Time();
			}

			// 実際に真ん中に据えるカード分の座標差
			m_diff = rounding(m_diff + m_vel * Scene::DeltaTime() + m_toStoppingDiff.deltaValue(EaseOutQuint));
			// 中央に存在するカードのインデックス
			selectedRefIndex(sizeRefs() - 1 - static_cast<ID>(m_diff / m_heightf()));

			return selectedDrawableId();
		}

		void draw() const
		{
			// startIndex の描画を開始する位置(y座標)
			double startY    = m_centerf() + m_diff - m_heightf() * (sizeRefs() - selectedRefIndex()) - (m_toStoppingHeight.value() - m_heightf()) / 2;
			// startIndex の描画位置(Rect)
			Rect   startRect = RectF{0.0, startY, static_cast<double>(Iframe::Width()), m_toStoppingHeight.value()}.asRect();

			// [0, sizeRefs()) に丸めながら増減させる
			const auto inc = [&](int32 index) { return index + 1 >= sizeRefs() ? 0 : index + 1; };
			const auto dec = [&](int32 index) { return index - 1 < 0 ? sizeRefs() - 1 : index - 1; };

			// 中央のカード
			{
				ScopedIframe2D iframe(startRect);
				selectedDrawable().draw(getTransition(), stopTime());
			}

			// 上半分のカードたち
			{
				int32 index = dec(selectedRefIndex());
				Rect  rect  = Rect{startRect.tl().movedBy(0, -m_heightf()), Iframe::Width(), m_heightf()};

				// bl は実際の領域よりも下方向に 1px はみ出していると考えられるので 0 は含まないでおく
				for (; rect.bl().y > 0; rect.moveBy(0, -m_heightf()), index = dec(index))
				{
					ScopedIframe2D iframe(rect, ScopedIframe2DCropped::No);
					drawable(index).draw(0.0, stopTime());
				}
			}

			// 下半分のカードたち
			{
				int32 index = inc(selectedRefIndex());
				Rect  rect  = Rect{startRect.bl(), Iframe::Width(), m_heightf()};

				for (; rect.tl().y < Iframe::Height(); rect.moveBy(0, m_heightf()), index = inc(index))
				{
					ScopedIframe2D iframe(rect, ScopedIframe2DCropped::No);
					drawable(index).draw(0.0, stopTime());
				}
			}
		}

		ListDrawer() = default;

		ListDrawer(HeightF heightf, MaxHeightF maxheightf, CenterF centerf)
			: m_heightf(heightf)
			, m_maxHeightf(maxheightf)
			, m_centerf(centerf)
		{}

	private:
		using Int32Func = int32 (*)();

		State                                   m_state            = State::Coasting; // 始めから停止させているとズレちゃうので、滑っていることにして停止するところからやる
		int32                                   m_selected         = 0;
		int32                                   m_prevSelected     = 0;
		HeightF                                 m_heightf          = defaultHeightf;
		MaxHeightF                              m_maxHeightf       = defaultmaxheightf;
		CenterF                                 m_centerf          = defaultcenterf;
		double                                  m_vel              = 0.0;
		double                                  m_diff             = -(double)m_heightf() / 2;
		double                                  m_lastStopTime     = Scene::Time();
		LerpTransition                          m_toStoppingDiff   = {0.5s, 0.25s, 0.0, 0.0};
		LerpTransition                          m_toStoppingHeight = {0.1s, 0.1s, m_heightf(), m_maxHeightf(m_heightf())};
		HashTable<ID, std::unique_ptr<IDrawer>> m_drawablesDic     = {};
		Array<ID>                               m_drablesRef       = {};
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
