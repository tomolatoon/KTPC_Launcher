#include <Siv3D.hpp> // OpenSiv3D v0.6.5

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
		Main(const InitData& init)
			: IScene{init}
		{
			USINGS;

			drawer.addAsArray(getData().map([&](const Game& e) {
				return [&](double per, double stopTime) {
					// Print << U"{}, {}"_fmt(per, stopTime);

					Iframe::Rect().draw(e.background);
					e.icon().resized(Iframe::Height() * 0.8).drawAt(10_vw, Iframe::Center().y);

					const double addtionalHiddenTime = 1.0;

					const auto calDiff = [&](const double time, const double scrollVel, const double textWidth, const double regionWidth, const size_t lines) {
						const double loopTime    = (textWidth + regionWidth * lines) / scrollVel + addtionalHiddenTime;
						const double virtualDiff = Fmod(time, loopTime) * scrollVel;
						return -virtualDiff < -textWidth ? -virtualDiff + addtionalHiddenTime * scrollVel + textWidth + regionWidth * lines : -virtualDiff;
					};

					{
						const auto oneLineScroller = [&](const String& s, double fontSize, double x, double y, double width) {
							if (per == 1.0)
							{
								DrawableText text   = FontAsset(U"Black")(s);
								RectF        region = text.region(fontSize, x, y);

								if (region.w <= width)
								{
									text.draw(fontSize, x, y);
								}
								else
								{
									text.draw(fontSize, x + calDiff(stopTime, width / scrollPerS, region.w, width, 1), y);
								}
							}
							else
							{
								FontAsset(U"Black")(s).draw(fontSize, x, y);
							}
						};

						RectF region        = {19_vw, 7.5_vh, 79.5_vw, 50_vh};
						RectF scissorRegion = region.stretched(-1_vw, 0);
						region.draw(Palette::Gray);

						ScopedRenderStates2D scissor = tomolatoon::CreateScissorRect(scissorRegion.asRect(), tomolatoon::PositionBasedIframe::Yes);
						oneLineScroller(e.title, vh(titleHeight), 20_vw, vh(titleY), scissorRegion.w);
						oneLineScroller(e.author, vh(authorHeight), 20.5_vw, vh(authorY), scissorRegion.w);
					}

					const auto textRegion = [&](const String& s, const double fontSize, const Vec2 firstPos, const double width) {
						const RectF  oneLineRegion = FontAsset(U"Semi")(s).region(fontSize);
						const double lineHeight    = oneLineRegion.h;
						const double allLength     = oneLineRegion.w;

						const auto xMappingToVec2 = [&](const double x) {
							if (x < width)
							{
								return firstPos.movedBy(x, 0);
							}
							else
							{
								return firstPos.movedBy(x - width, lineHeight);
							}
						};

						const auto xToDrawPos = [&](const double x, const double w) -> std::tuple<Optional<Vec2>, Optional<Vec2>> {
							if (x + w < 0 || width * 2 < x)
							{
								return {none, none};
							}
							else if ((x < width && x + w < width) || (x >= width))
							{
								return {xMappingToVec2(x), none};
							}
							else
							{
								return {xMappingToVec2(x), xMappingToVec2(x + w).movedBy(-w, 0)};
							}
						};

						int32  break_    = 0;
						double x         = per == 1.0 && allLength > width * 2 ? calDiff(stopTime, width / scrollPerS, allLength, width, 2) : 0;
						auto   graphemes = s | tomolatoon::views::graphme;
						auto   end       = std::ranges::end(graphemes);
						for (auto it = std::ranges::begin(graphemes); it != end; ++it)
						{
							const String& grapheme = *it;

							DrawableText text   = FontAsset(U"Semi")(grapheme);
							RectF        region = text.region(fontSize);

							auto [f, s] = xToDrawPos(x, region.w);

							const auto draw = [&](const Optional<Vec2>& p) {
								if (p)
								{
									text.draw(fontSize, p.value());
								}
							};

							if (per != 1.0)
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
					};

					{
						RectF scissorRegion = RectF{20.5_vw, vh(descriptionY), 78_vw, vh(descriptionHeight) * 2 + 10_vh}.stretched(vh(stretchTop), vw(stretchRight), vh(stretchBottom), vw(stretchLeft));
						//scissorRegion.draw(Palette::Aqua);

						ScopedRenderStates2D scissor = tomolatoon::CreateScissorRect(scissorRegion.asRect(), tomolatoon::PositionBasedIframe::Yes);
						textRegion(e.description, vh(descriptionHeight), {20.5_vw, vh(descriptionY)}, scissorRegion.w);
					}
				};
			}));
		}

		void update() override
		{
			USINGS;

			// List
			{
				drawer.update(RectF{0, 90_vh, 100_vw, 100_vh}.mouseOver());
			}
		}

#define LISTDRAWER_DEBUG

#ifdef LISTDRAWER_DEBUG
# define LISTDRAWER_VAR_DEFINE double mutable
#else
# define LISTDRAWER_VAR_DEFINE const double
#endif

		void draw() const override
		{
			USINGS;

			const auto sliderStart = 15_sw;

			ClearPrint();

			// List
			{
				ScopedIframe2D iframe(RectF(sliderStart, 0, 44_sw, 100_sh).asRect());
				drawer.draw();
			}
			// 画像
			{
				getData()[drawer.getSelected()].icon().resized(31.5_sw).draw(62_sw, 15.5_sh);
			}
			// その他
			{
				Line{0, 50_sh, Scene::Width(), 50_sh}.draw(LineStyle::Default, 1, ColorF{Palette::White, 1});
				RectF{sliderStart, 90_sh, 85_sw, 10_sh}.asRect().draw(Palette::White);
			}
			drawer.getDrawer(drawer.getSelected());

#ifdef LISTDRAWER_DEBUG
# define EXPAND(macro)                macro()
# define STRINGIZE(s)                 #s
# define CAT(a, b)                    a##b
# define SLIDER(name, min, max, ybeg) SimpleGUI::Slider(U"{}: {:.2f})"_fmt(CAT(U, #name), name), name, min, max, Vec2{0, ybeg * 50}, 250);

			int32 i = 2;
			SLIDER(titleHeight, 10, 50, i++);
			SLIDER(authorHeight, 10, 50, i++);
			SLIDER(descriptionHeight, 0, 100, i++);
			SLIDER(titleY, 0, 30, i++);
			SLIDER(authorY, 20, 50, i++);
			SLIDER(descriptionY, 0, 100, i++);
			SLIDER(descriptionChars, 75, 200, i++);
			SLIDER(stretchTop, 0, 20, i++);
			SLIDER(stretchRight, 0, 20, i++);
			SLIDER(stretchBottom, 0, 20, i++);
			SLIDER(stretchLeft, 0, 20, i++);
			SLIDER(scrollPerS, 0.1, 15, i++);
#endif
		}

	private:
		tomolatoon::ListDrawer<> drawer;

		LISTDRAWER_VAR_DEFINE titleHeight       = 25;
		LISTDRAWER_VAR_DEFINE authorHeight      = 20;
		LISTDRAWER_VAR_DEFINE descriptionHeight = 13.5;
		LISTDRAWER_VAR_DEFINE titleY            = 4;
		LISTDRAWER_VAR_DEFINE authorY           = 31;
		LISTDRAWER_VAR_DEFINE descriptionY      = 60;
		LISTDRAWER_VAR_DEFINE descriptionChars  = 75;
		LISTDRAWER_VAR_DEFINE stretchTop        = 0;
		LISTDRAWER_VAR_DEFINE stretchRight      = 0;
		LISTDRAWER_VAR_DEFINE stretchBottom     = 0;
		LISTDRAWER_VAR_DEFINE stretchLeft       = 0;
		LISTDRAWER_VAR_DEFINE scrollPerS        = 5;
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
				// 非同期処理の結果を取得する
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
