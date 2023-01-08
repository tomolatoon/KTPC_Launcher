﻿#include "Load.hpp"

namespace tomolatoon
{
	void InitialLoad(JSON& settings, Array<Game>& games) noexcept
	{
		JSONSchema schema;

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
} // namespace tomolatoon
