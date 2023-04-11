#pragma once

#include "Utility.hpp"

#include <Siv3D.hpp>

#include <filesystem>

namespace tomolatoon
{
	struct Game
	{
		uint32        id;
		String        title;
		String        author;
		URL           exe;
		uint32        year;
		String        description;
		ColorF        background;
		Array<String> tags;

		String getIdString() const
		{
			return String(reinterpret_cast<const char32_t*>(&id), 1);
		}

		TextureAsset icon() const
		{
			return TextureAsset(getIdString() + U"_icon");
		}

#define FILEPATH ((jsonPath.parent_path() / json[U"icon"].get<URL>().toUTF32()).u32string())

		Game(const JSON& json, std::filesystem::path jsonPath)
			: id(TextureAsset::Enumerate().size())
			, title(json[U"title"].get<String>())
			, author(json[U"author"].get<String>())
			, exe(json[U"exe"].get<URL>())
			, year(json[U"year"].get<int32>())
			, description(json[U"description"].get<String>())
			, background([](auto&& e) -> ColorF {
				if (e) return Color{e[0].get<uint8>(), e[1].get<uint8>(), e[2].get<uint8>()};
				else return HSV_(204, 100, 80);
			}(json.hasElement(U"background") ? json[U"background"] : JSON::Invalid()))
			, tags([](auto&& e) { Array<String> ret; for (auto&& [key,value] : e) { ret.push_back(value.get<String>()); } return ret; }(json[U"tags"]))
		{
			TextureAsset::Register(getIdString() + U"_icon", FILEPATH);
		}

#undef FILEPATH
	};
} // namespace tomolatoon
