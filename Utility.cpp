#include "Utility.hpp"

namespace tomolatoon
{
	bool isURL(FilePathView fp) noexcept
	{
		const auto protocol = Array{
			U"http://"_s,
			U"https://"_s,
		};

		for (auto&& e : protocol)
		{
			if (fp.starts_with(e))
			{
				return true;
			}
		}

		return false;
	}
}
