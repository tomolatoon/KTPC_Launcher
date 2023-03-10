#pragma once

#include <Siv3D.hpp>

#include <filesystem>

namespace tomolatoon
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
} // namespace tomolatoon
