#pragma once

#include <Siv3D.hpp>

#include <unicode/uchar.h>
#include <unicode/brkiter.h>
#include <unicode/errorcode.h>

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

} // namespace tomolatoon
