#pragma once

#include <Siv3D.hpp>

#include <ranges>
#include <filesystem>

#include "DataTypes.hpp"

namespace tomolatoon
{
	void InitialLoad(JSON& settings, Array<Game>& games) noexcept;
}
