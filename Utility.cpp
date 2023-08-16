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

	namespace Cursor
	{
		namespace
		{
			Vec2 previousVelocity = {};
			Vec2 velocity         = {};
			Vec2 acceleration     = {};
		} // namespace

		void Update() noexcept
		{
			previousVelocity = velocity;
			velocity         = DeltaF() / Scene::DeltaTime();

			acceleration = (velocity - previousVelocity) / Scene::DeltaTime();
		}

		Point Delta() noexcept
		{
			return s3d::Cursor::Delta();
		}

		Vec2 DeltaF() noexcept
		{
			return s3d::Cursor::DeltaF();
		}

		Vec2 Velocity() noexcept
		{
			return velocity;
		}

		Vec2 PreviousVelocity() noexcept
		{
			return previousVelocity;
		}

		Vec2 Acceleration() noexcept
		{
			return acceleration;
		}
	} // namespace Cursor
}
