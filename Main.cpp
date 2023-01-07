#include <Siv3D.hpp> // OpenSiv3D v0.6.5

#include <concepts>
#include <ranges>
#include <filesystem>

#include "Utility.hpp"
#include "Units.hpp"
// #include "CurryGenerator.hpp"
#include "ListDrawer.hpp"
#include "GraphemeView.hpp"

#include <unicode/uchar.h>
#include <unicode/brkiter.h>

namespace tomolatoon
{
	namespace Unicode
	{
		namespace Property
		{
			template <class GetType, UProperty PropertyIndex>
			GetType GetProperty(char32 ch)
			{
				return static_cast<GetType>(u_getIntPropertyValue(ch, PropertyIndex));
			}

			template <class GetType>
			GetType GetProperty(char32 ch, UProperty propIndex)
			{
				return static_cast<GetType>(u_getIntPropertyValue(ch, propIndex));
			}

			ULineBreak GetLineBreak(char32 ch)
			{
				return GetProperty<ULineBreak, UCHAR_LINE_BREAK>(ch);
			}

			String GetUnicodeName(char32 c)
			{
				char           buffer[100];
				icu::ErrorCode errorCode;

				size_t size = u_charName(c, U_UNICODE_CHAR_NAME, buffer, std::ranges::size(buffer), errorCode);

				return s3d::Unicode::FromUTF8({buffer, size});
			}
		} // namespace Property

	} // namespace Unicode

	using PositionBasedIframe = YesNo<struct PositionBasedIframe_tag>;

	auto CreateScissorRect(Rect rect, PositionBasedIframe b = PositionBasedIframe::No)
	{
		Rect scissor = b ? rect.movedBy(Iframe::RectAtScene().tl()) : rect;

		Graphics2D::SetScissorRect(scissor);

		RasterizerState rs = RasterizerState::Default2D;
		rs.scissorEnable   = true;

		return ScopedRenderStates2D{rs};
	}
} // namespace tomolatoon


void Main()
{
	struct Game
	{
		String        title;
		String        author;
		URL           exe;
		Texture       icon;
		String        description;
		int32         year;
		Array<String> tags;
		Color         background;

		Game(const JSON& json, std::filesystem::path jsonPath)
			: title(json[U"title"].get<String>())
			, author(json[U"author"].get<String>())
			, exe(json[U"exe"].get<URL>())
			, icon([&](auto&& e) { std::filesystem::path iconPath = (jsonPath.parent_path().u32string() + e.toUTF32()); return Texture{iconPath.u32string()}; }(json[U"icon"].get<URL>()))
			, description(json[U"description"].get<String>())
			, year(json[U"year"].get<int32>())
			, tags([](auto&& e) { Array<String> ret; for (auto&& [key,value] : e) { ret.push_back(value.get<String>()); } return ret; }(json[U"tags"]))
			, background([](auto&& e) { if (e) { return Color{e[0].get<uint8>(), e[1].get<uint8>(), e[2].get<uint8>()}; } else { return Color{66,66,66}; } }(json.hasElement(U"background") ? json[U"background"] : JSON::Invalid()))
		{}
	};

	JSON                     settings;
	JSONSchema               schema;
	tomolatoon::ListDrawer<> drawer;
	Array<Texture>           icons;
	Array<Game>              games;
	const int32              baseFontSize = System::EnumerateMonitors()[System::GetCurrentMonitorIndex()].fullscreenResolution.y / 15;
	Font                     font         = {baseFontSize, Typeface::Mplus_Black};
	Font                     emoji        = {baseFontSize, Typeface::ColorEmoji};

	font.addFallback(emoji);

	{
		if ((schema = JSONSchema::Load(U"./data.schema.json")).isEmpty())
		{
			System::MessageBoxOK(U"KTPC Launcher Initialization Error", U"規定の JSON Schema ファイルが実行ファイルと同じディレクトリに data.schema.json という名前で配置されていませんでした。");
			return;
		}

		const Array<String>& args = System::GetCommandLineArgs();

		if (args.size() <= 1)
		{
			System::MessageBoxOK(U"KTPC Launcher Initialization Error", U"ゲームについてのデータを提供して下さい。ゲームについてのデータは JSON 形式で、拡張子は .json とし、規定の JSON Schema に従う形で作成し、その JSON へのパスをコマンドライン引数に指定してください。");
			return;
		}

		std::filesystem::path jsonPath = args[1].toUTF32();

		if (!(jsonPath.extension() == U".json"))
		{
			System::MessageBoxOK(U"KTPC Launcher Initialization Error", U"コマンドライン引数にパス（{}）が渡されましたが、拡張子が JSON ではありませんでした。ゲームについてのデータは JSON 形式で、拡張子は .json とし、規定の JSON Schema に従う形で作成してください。"_fmt(jsonPath.u32string()));
			return;
		}
		else if (!std::filesystem::exists(jsonPath))
		{
			System::MessageBoxOK(U"KTPC Launcher Initialization Error", U"コマンドライン引数にパス（{}）が渡されましたが、そのパスに該当するファイルが存在しませんでした。ゲームについてのデータは JSON 形式で、拡張子は .json とし、規定の JSON Schema に従う形で作成してください。"_fmt(jsonPath.u32string()));
			return;
		}
		else
		{
			settings = JSON::Load(jsonPath.u32string());
		}

		if (auto&& details = schema.validateWithDetails(settings); details.isError())
		{
			System::MessageBoxOK(U"KTPC Launcher Initialization Error", U"ファイルは存在しましたが、規定の JSON Schema に沿った JSON データではありませんでした。規定の JSON Schema に従う形で作成してください。エラーメッセージは次に示す通りです。\n{}"_fmt(Format(details)));
			return;
		}
		else
		{
			std::ranges::for_each(settings[U"games"], [&](auto&& item) { auto&& [key, game] = item; games.push_back(Game{game, jsonPath}); });
		}
	}

	Window::Resize(1755, 810, Centering::Yes);
	Scene::SetResizeMode(ResizeMode::Actual);

	using namespace tomolatoon::Units;
	using namespace std::placeholders;
	using namespace tomolatoon::Operators;
	using tomolatoon::Iframe, tomolatoon::Curry::To, tomolatoon::ScopedIframe2D;

	//const auto rect = To<Rect>::With(std::bind(sw, _2), _1, 40_swf, 100_shf / 7 | Ceilf);

#define LISTDRAWER_DEBUG

#ifdef LISTDRAWER_DEBUG
#	define LISTDRAWER_VAR_DEFINE double
#else
#	define LISTDRAWER_VAR_DEFINE const double
#endif

	LISTDRAWER_VAR_DEFINE titleHeight       = 25;
	LISTDRAWER_VAR_DEFINE authorHeight      = 20;
	LISTDRAWER_VAR_DEFINE descriptionHeight = 13.5;
	LISTDRAWER_VAR_DEFINE titleY            = 4;
	LISTDRAWER_VAR_DEFINE authorY           = 31;
	LISTDRAWER_VAR_DEFINE descriptionY      = 57.5;
	LISTDRAWER_VAR_DEFINE descriptionChars  = 75;
	LISTDRAWER_VAR_DEFINE stretchTop        = 0;
	LISTDRAWER_VAR_DEFINE stretchRight      = 0;
	LISTDRAWER_VAR_DEFINE stretchBottom     = 0;
	LISTDRAWER_VAR_DEFINE stretchLeft       = 0;
	LISTDRAWER_VAR_DEFINE scrollPerS        = 5;

	drawer.addAsArray(games.map([&](const Game& e) {
		return [&](double per, double stopTime) {
			Print << U"{}, {}"_fmt(per, stopTime);

			Iframe::Rect().draw(e.background);
			e.icon.resized(Iframe::Height() * 0.8).drawAt(10_vwf(), Iframe::Center().y);

			{
				const auto oneLineScroller = [&](const String& s, double fontSize, double x, double y, double width) {
					if (per == 1.0)
					{
						DrawableText text   = font(s);
						RectF        region = text.region(fontSize, x, y);

						if (region.w <= width)
						{
							text.draw(fontSize, x, y);
						}
						else
						{
							const double scrollVel           = width / scrollPerS;
							const double halfLoopTime        = region.w / scrollVel;
							const double per                 = (Fmod(stopTime + halfLoopTime, halfLoopTime * 2) - halfLoopTime) / halfLoopTime;
							const double addtionalHiddenTime = 1.0;
							const double baseDiff            = per * (per >= 0.0 ? region.w : width + scrollVel * addtionalHiddenTime);
							text.draw(fontSize, x - baseDiff, y);
						}
					}
					else
					{
						font(s).draw(fontSize, x, y);
					}
				};

				RectF region        = {19_vwf(), 7.5_vhf(), 79.5_vwf(), 50_vhf()};
				RectF scissorRegion = region.stretched(-1_vwf(), 0);
				region.draw(Palette::Gray);

				ScopedRenderStates2D scissor = tomolatoon::CreateScissorRect(scissorRegion.asRect(), tomolatoon::PositionBasedIframe::Yes);
				oneLineScroller(e.title, vhf(titleHeight)(), 20_vwf(), vhf(titleY)(), scissorRegion.w);
				//font(e.title).draw(vhf(titleHeight)(), 20_vwf(), vhf(titleY)());
				font(e.author).draw(vhf(authorHeight)(), 20.5_vwf(), vhf(authorY)());
			}

			const auto textRegion = [&](const String& s, const double fontSize, const Vec2 firstPos, const double maxX) {
				int32 break_    = 0;
				Vec2  pos       = firstPos;
				auto  graphemes = s | tomolatoon::views::graphme;
				auto  end       = std::ranges::end(graphemes);
				for (auto it = std::ranges::begin(graphemes); it != end; ++it)
				{
					const String& grapheme = *it;

					DrawableText text   = font(grapheme);
					RectF        region = text.region(fontSize, pos);

					const auto draw = [&]() {
						//region.draw(Palette::Coral);
						text.draw(fontSize, pos);

						pos = region.tr();
					};

					const auto isPreventBreak = [](const String& s) {
						return s == U"、" || s == U"。";
					};

					if (pos.x + region.w > maxX)
					{
						if (isPreventBreak(grapheme))
						{
							draw();
						}
						else
						{
							pos    = Vec2{firstPos.x, pos.y + region.h};
							region = text.region(fontSize, pos);

							if (++break_ == 2)
							{
								break;
							}

							draw();
						}
					}
					else
					{
						draw();
					}
				}
			};

			{
				textRegion(e.description, vhf(descriptionHeight)(), {20.5_vwf(), vhf(descriptionY)()}, 19_vwf() + 79.5_vwf());
			}

			//font(e.description).draw(vhf(descriptionHeight)(), 20.5_vwf(), vhf(descriptionY)());
			//font(e.description).draw(vhf(descriptionHeight)(), RectF{20.5_vwf(), vhf(descriptionY)(), 78_vwf(), vhf(90 - descriptionY)()}.stretched(vhf(stretchTop)(), vwf(stretchRight)(), vhf(stretchBottom)(), vwf(stretchLeft)()));
		};
	}));

	while (System::Update())
	{
		const auto sliderStart = 15_sw;

		ClearPrint();

		// List
		{
			ScopedIframe2D iframe(RectF(sliderStart, 0, 44_sw, 100_sh).asRect());
			drawer.update(RectF{0, 90_vh, 100_vw, 100_vh}.mouseOver());
			drawer.draw();
		}
		// 画像
		{
			games[drawer.getSelected()].icon.resized(31.5_sw).draw(62_sw, 15.5_sh);
		}
		// その他
		{
			Line{0, 50_sh, Scene::Width(), 50_sh}.draw(LineStyle::Default, 1, ColorF{Palette::White, 1});
			RectF{sliderStart, 90_sh, 85_sw, 10_sh}.asRect().draw(Palette::White);
		}
		drawer.getDrawer(drawer.getSelected());

#ifdef LISTDRAWER_DEBUG
#	define EXPAND(macro)                macro()
#	define STRINGIZE(s)                 #s
#	define CAT(a, b)                    a##b
#	define SLIDER(name, min, max, ybeg) SimpleGUI::Slider(U"{}: {:.2f})"_fmt(CAT(U, #name), name), name, min, max, Vec2{0, ybeg * 50}, 250);

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
}
