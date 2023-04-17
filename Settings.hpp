#include <Siv3D.hpp>

#define HSV_(h, s, v)     \
 HSV                      \
 {                        \
  h, s / 100.0, v / 100.0 \
 }

namespace tomolatoon::Settings
{
	inline constexpr auto CardBackgroundColor = HSV_(247, 41, 52);
}
