#include "Viewport.hpp"

namespace tomolatoon
{
	ScopedRenderStates2D CreateScissorRect(Rect rect, PositionBasedIframe b)
	{
		Rect scissor = b ? rect.movedBy(Iframe::RectAtScene().tl()) : rect;

		Graphics2D::SetScissorRect(scissor);

		RasterizerState rs = RasterizerState::Default2D;
		rs.scissorEnable   = true;

		return ScopedRenderStates2D{rs};
	}
}
