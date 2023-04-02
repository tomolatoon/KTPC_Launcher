﻿#include <Siv3D.hpp> // OpenSiv3D v0.6.5

#include <concepts>
#include <ranges>
#include <filesystem>

#include "Utility.hpp"
#include "Units.hpp"
// #include "CurryGenerator.hpp"
#include "ListDrawer.hpp"
#include "GraphemeView.hpp"
#include "DataTypes.hpp"
#include "Load.hpp"

#define DEBUGDRAW draw(Arg::top = HSV{0, 0.5, 0.5}, Arg::bottom = HSV{120, 0.5, 0.5})

namespace tomolatoon
{
#define USINGS                          \
 using namespace tomolatoon::Units;     \
 using namespace std::placeholders;     \
 using namespace tomolatoon::Operators; \
 using tomolatoon::Iframe, tomolatoon::ScopedIframe2D

	using App = SceneManager<String, Array<Game>>;

	struct Main : App::Scene
	{
		inline static constexpr double additionalHiddenTime = 1.0;

		Main(const InitData& init)
			: IScene{init}
		{
			USINGS;

			m_drawer.addAsArray(getData().map([&](const Game& e) {
				return [&](double per, double stopTime) {
					//Print << U"{}, {}"_fmt(per, stopTime);

					auto if_1 = Iframe::Rect();
					auto if_2 = Iframe::RectAtScene();

					// Background
					Iframe::Rect().draw(e.background);

					// Icon
					e.icon().resized(Iframe::Height() * 0.8).drawAt(10_vw, Iframe::Center().y);

					RectF{19_vw, 70_vh, 79.5_vw, 100_vh}.draw(Palette::Lightgrey);

					{
						ScopedIframe2D iframe{
							RectF{19_vw, 7.5_vh, 79.5_vw, 65_vh}
								.draw(Palette::Gray)
								.stretched(-1_vw, 0)
								.asRect()
                        };

						drawSingleline(e.title, per == 1.0, vh(titleHeight), 1.5_vw, vh(titleY));
						drawSingleline(e.author, per == 1.0, vh(authorHeight), 2.5_vw, vh(authorY));
					}

					//Iframe::Rect().DEBUGDRAW;
				};
			}));
		}

		/// @brief x軸にどの程度移動した所が文字列の先頭位置かを返す。
		/// なお、x軸は左上を0として、右方向へ軸を張り、右端まで来たら改行して lineHeight 下がったところに継続し、右下の方でこれ以上行を取れない所まで継続する。
		/// @param time スクロール開始時刻からの経過時間[s]
		/// @param additionalHiddenTime 更に隠し続ける時間[s]
		/// @param scrollVel スクロール速度[px/s]
		/// @param textWidth テキストを1行に描画した時の全長[px]
		/// @param regionWidth 表示領域の幅[px]
		/// @param lines 何行で描画するか
		double calDiff(const double time, const double additionalHiddenTime, const double scrollVel, const double textWidth, const double regionWidth, const size_t lines) const noexcept
		{
			const double loopTime    = (textWidth + regionWidth * lines) / scrollVel + additionalHiddenTime;
			const double virtualDiff = Fmod(time, loopTime) * scrollVel;
			return -virtualDiff < -textWidth ? -virtualDiff + additionalHiddenTime * scrollVel + textWidth + regionWidth * lines : -virtualDiff;
		}

		void drawSingleline(const String& string, const bool enableScrooll, const double fontSize, const double x, const double y)
		{
			if (enableScrooll)
			{
				const DrawableText text   = FontAsset(U"Black")(string);
				const RectF        region = text.region(fontSize, x, y);

				const double xDiff = region.w > Iframe::Width() ? calDiff(m_drawer.stopTime(), additionalHiddenTime, scrollVelocity, region.w, Iframe::Width(), 1) : 0;
				text.draw(fontSize, x + xDiff, y);
			}
			else
			{
				FontAsset(U"Black")(string).draw(fontSize, x, y);
			}
		}

		/// @brief スクロールもする複数行に渡る文字列表示を行います。ScopedIframe を使って描画領域を制限のこと。
		void drawMultiline(const String& string, const size_t lines, const bool enableScrooll, const double fontSize, const Vec2 firstPos) const
		{
			// x軸は左上を0として、右方向へ軸を張り、右端まで来たら改行して lineHeight 下がったところに継続し、右下の方でこれ以上行を取れない所まで継続する。

			const double width            = Iframe::Width();
			const RectF  singleLineRegion = FontAsset(U"Semi")(string).region(fontSize);
			const double lineHeight       = singleLineRegion.h;
			const double stringAllWidth   = singleLineRegion.w;
			const double widthCapacity    = width * lines;

			const auto xMappingToVec2 = [&](const double x) {
				if (x < width)
				{
					return firstPos.movedBy(x, 0);
				}
				else
				{
					const size_t newlines = x / width;
					return firstPos.movedBy(x - width * newlines, lineHeight * newlines);
				}
			};

			const auto xToDrawPos = [&](const double x, const double w) -> std::tuple<Optional<Vec2>, Optional<Vec2>> {
				if (x + w < 0 || widthCapacity < x)
				{
					return {none, none};
				}
				// 端っこに来てなければ width で mod を取っても大小は変わらない
				else if (Fmod(x, width) <= Fmod(x + w, width) || widthCapacity < x + w)
				{
					return {xMappingToVec2(x), none};
				}
				else
				{
					return {xMappingToVec2(x), xMappingToVec2(x + w).movedBy(-w, 0)};
				}
			};

			const double startX    = enableScrooll && stringAllWidth > widthCapacity ? calDiff(m_drawer.stopTime(), additionalHiddenTime, scrollVelocity, stringAllWidth, width, lines) : 0;
			const auto   graphemes = string | tomolatoon::views::graphme;
			auto         it        = std::ranges::begin(graphemes);
			auto         end       = std::ranges::end(graphemes);
			for (double x = startX; it != end; ++it)
			{
				const String& grapheme = *it;

				DrawableText text   = FontAsset(U"Semi")(grapheme);
				RectF        region = text.region(fontSize);

				auto [f, s] = xToDrawPos(x, region.w);

				const auto draw = [&](const Optional<Vec2>& p) {
					if (p) text.draw(fontSize, p.value());
				};

				if (not enableScrooll)
				{
					const auto is = [&](Array<StringView> svs) {
						return svs.any([&](auto e) { return e == grapheme; });
					};
					if (is({U"、", U"。"}) && f && s)
					{
						draw(f);
						x = width;
						continue;
					}
				}

				draw(f), draw(s);

				x += region.w;
			}
		}

		void update() override
		{
			USINGS;

			// List
			{
				m_drawer.update(RectF{0, 90_vh, 100_vw, 100_vh}.mouseOver());
			}
		}

		void draw() const override
		{
			ClearPrint();
			//Print << Profiler::GetStat().drawCalls;

			USINGS;

			const auto sliderStart = 15_sw;

			// List
			{
				ScopedIframe2D iframe(RectF(sliderStart, 0, 44_sw, 100_sh).asRect());
				m_drawer.draw();
			}
			// 画像
			{
				getData()[m_drawer.selected()].icon().resized(31.5_sw).draw(62_sw, 15.5_sh);
			}
			// その他
			{
				Line{0, 50_sh, Scene::Width(), 50_sh}.draw(LineStyle::Default, 1, ColorF{Palette::White, 1});
				RectF{sliderStart, 90_sh, 85_sw, 10_sh}.asRect().draw(Palette::White);
			}
			// Description
			{
				Rect rect = RectF{vw(descriptionX), vh(descriptionY), vw(descriptionWidth), vh(descriptionHeight)}.asRect();
				rect.stretched(vw(1.0), 0).draw(Palette::Lightskyblue);

				ScopedIframe2D iframe(rect);
				drawMultiline(getData()[m_drawer.selected()].description, (size_t)descriptionLines, m_drawer.isStopped(), vh(descriptionFontSize), {0, vh(descriptionDiff)});
			}

//#define LISTDRAWER_DEBUG
#ifdef LISTDRAWER_DEBUG
# define EXPAND(macro)                macro()
# define STRINGIZE(s)                 #s
# define CAT(a, b)                    a##b
# define SLIDER(name, min, max, ybeg) SimpleGUI::Slider(U"{}: {:.2f})"_fmt(CAT(U, #name), name), name, min, max, Vec2{0, ybeg * 50}, 300, 400);
# define LISTDRAWER_VAR_DEFINE        double mutable

			int32 i = 2;
			SLIDER(titleHeight, 0, 100, i++);
			SLIDER(authorHeight, 0, 100, i++);
			SLIDER(titleY, 0, 100, i++);
			SLIDER(authorY, 0, 100, i++);
			SLIDER(stretchTop, 0, 20, i++);
			SLIDER(stretchRight, 0, 20, i++);
			SLIDER(stretchBottom, 0, 20, i++);
			SLIDER(stretchLeft, 0, 20, i++);
			SLIDER(scrollVelocity, 100, 500, i++);
			SLIDER(descriptionX, 0, 100, i++);
			SLIDER(descriptionY, 50, 100, i++);
			SLIDER(descriptionWidth, 0, 50, i++);
			SLIDER(descriptionHeight, 0, 30, i++);
			SLIDER(descriptionLines, 1, 10, i++);
			SLIDER(descriptionDiff, 0, 50, i++);
			SLIDER(descriptionFontSize, 10, 50, i++);
# undef EXPAND
# undef STRINGIZE
# undef CAT
# undef SLIDER
#else
# define LISTDRAWER_VAR_DEFINE const double
#endif
		}

	private:
		tomolatoon::ListDrawer<> m_drawer;

		LISTDRAWER_VAR_DEFINE titleHeight         = 40;
		LISTDRAWER_VAR_DEFINE authorHeight        = 35;
		LISTDRAWER_VAR_DEFINE titleY              = 2.45;
		LISTDRAWER_VAR_DEFINE authorY             = 47;
		LISTDRAWER_VAR_DEFINE stretchTop          = 0;
		LISTDRAWER_VAR_DEFINE stretchRight        = 0;
		LISTDRAWER_VAR_DEFINE stretchBottom       = 0;
		LISTDRAWER_VAR_DEFINE stretchLeft         = 0;
		LISTDRAWER_VAR_DEFINE scrollVelocity      = 250;
		LISTDRAWER_VAR_DEFINE descriptionX        = 15;
		LISTDRAWER_VAR_DEFINE descriptionY        = 90;
		LISTDRAWER_VAR_DEFINE descriptionWidth    = 50;
		LISTDRAWER_VAR_DEFINE descriptionHeight   = 10;
		LISTDRAWER_VAR_DEFINE descriptionLines    = 2;
		LISTDRAWER_VAR_DEFINE descriptionDiff     = 15.5;
		LISTDRAWER_VAR_DEFINE descriptionFontSize = 25;

#undef LISTDRAWER_VAR_DEFINE
	};

	struct Load : App::Scene
	{
		Load(const InitData& init)
			: IScene{init}
			, gamesLoad{[]() { return InitialLoad(); }}
		{}

		void update() override
		{
			if (gamesLoad.isReady())
			{
				getData() = gamesLoad.get();

				changeScene(U"Main");
			}
		}

		void draw() const override
		{
			USINGS;

			Circle{Scene::Center(), 40_vh}.drawArc(Scene::Time() * 120_deg, 300_deg, 4, 4);
		}

	private:
		AsyncTask<Array<Game>> gamesLoad;
	};

#undef USINGS

} // namespace tomolatoon

decltype(auto) as_clref(auto&& t)
{
	return static_cast<const std::remove_reference_t<decltype(t)>&>(t);
}

void Main()
{
	const int32 baseFontSize = System::EnumerateMonitors()[System::GetCurrentMonitorIndex()].fullscreenResolution.y / 15;

	FontAsset::Register(U"Black", baseFontSize, Typeface::Mplus_Black);
	FontAsset::Register(U"Semi", baseFontSize, Typeface::Mplus_Bold);
	FontAsset::Register(U"Emoji", baseFontSize, Typeface::ColorEmoji);

	FontAsset(U"Black").addFallback(as_clref(FontAsset(U"Emoji")));
	FontAsset(U"Semi").addFallback(as_clref(FontAsset(U"Emoji")));

	Window::Resize(1755, 810, Centering::Yes);
	Scene::SetResizeMode(ResizeMode::Actual);

	tomolatoon::App manager;
	manager.add<tomolatoon::Load>(U"Load");
	manager.add<tomolatoon::Main>(U"Main");

	while (System::Update())
	{
		if (not manager.update())
		{
			break;
		}
	}
}
