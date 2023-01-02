#include <Siv3D.hpp> // OpenSiv3D v0.6.5

#include <concepts>
#include <ranges>
#include <filesystem>

#include "Utility.hpp"
#include "Units.hpp"
// #include "CurryGenerator.hpp"
#include "ListDrawer.hpp"

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
			, background([](auto&& e) { if (e) { return Color{e[0].get<uint8>(), e[1].get<uint8>(), e[2].get<uint8>()}; } else { return Color{66,66,66}; } }(json[U"background"]))
		{}
	};

	JSON                     settings;
	JSONSchema               schema;
	tomolatoon::ListDrawer<> drawer;
	Array<Texture>           icons;
	Array<Game>              games;
	Font                     font{
        System::EnumerateMonitors()[System::GetCurrentMonitorIndex()].fullscreenResolution.y / 15,
        U"./Mplus1-Black.otf"};

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

// #define LISTDRAWER_DEBUG

#ifdef LISTDRAWER_DEBUG
#	define LISTDRAWER_VAR_DEFINE double
#else
#	define LISTDRAWER_VAR_DEFINE const double
#endif

	LISTDRAWER_VAR_DEFINE titleHeight       = 25;
	LISTDRAWER_VAR_DEFINE authorHeight      = 20;
	LISTDRAWER_VAR_DEFINE descriptionHeight = 13.5;
	LISTDRAWER_VAR_DEFINE titleY            = 4;
	LISTDRAWER_VAR_DEFINE authorY           = 30;
	LISTDRAWER_VAR_DEFINE descriptionY      = 57.5;

	drawer.addAsArray(games.map([&](const Game& e) {
		return [&](double per) {
			Print << U"{}, {}"_fmt(per, 40_vhf());
			Iframe::Rect().draw(e.background);
			e.icon.resized(Iframe::Height() * 0.8).drawAt(10_vwf(), Iframe::Center().y);

			RectF{19_vwf(), 7.5_vhf(), 79.5_vwf(), 50_vhf()}.draw(Palette::Gray);
			font(e.title).draw(vhf(titleHeight)(), 20_vwf(), vhf(titleY)());
			font(e.author).draw(vhf(authorHeight)(), 20.5_vwf(), vhf(authorY)());
			font(e.description).draw(vhf(descriptionHeight)(), 20.5_vwf(), vhf(descriptionY)());
		};
	}));

	while (System::Update())
	{
		const auto sliderStart = 15_sw;

		ClearPrint();

		// List
		{
			ScopedIframe2D iframe(RectF(sliderStart, 0, 44_sw, 100_sh).asRect());
			drawer.draw(RectF{0, 90_vh, 100_vw, 100_vh}.mouseOver());
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
		SimpleGUI::Slider(U"titleHeight: {:.2f}"_fmt(titleHeight), titleHeight, 10, 50, Vec2{0, 150}, 250);
		SimpleGUI::Slider(U"authorHeight: {:.2f}"_fmt(authorHeight), authorHeight, 10, 50, Vec2{0, 200}, 250);
		SimpleGUI::Slider(U"descriptionHeight: {:.2f}"_fmt(descriptionHeight), descriptionHeight, 0, 100, Vec2{0, 250}, 250);
		SimpleGUI::Slider(U"titleY: {:.2f}"_fmt(titleY), titleY, 0, 100, Vec2{0, 300}, 250);
		SimpleGUI::Slider(U"authorY: {:.2f}"_fmt(authorY), authorY, 0, 100, Vec2{0, 350}, 250);
		SimpleGUI::Slider(U"descriptionY: {:.2f}"_fmt(descriptionY), descriptionY, 0, 100, Vec2{0, 400}, 250);
#endif
	}
}
