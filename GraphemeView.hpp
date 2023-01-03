#pragma once

#include <Siv3D.hpp>

#include <ranges>

#include <unicode/utypes.h>
#include <unicode/ubrk.h>
#include <unicode/errorcode.h>

namespace tomolatoon
{
	template <std::ranges::input_range View>
	requires std::ranges::view<View> && requires { requires sizeof std::ranges::range_value_t<View> == 4; }
	struct GraphemeView : std::ranges::view_interface<GraphemeView<View>>
	{
		struct iterator
		{
			using value_type       = String;
			using difference_type  = ptrdiff_t;
			using iterator_concept = std::input_iterator_tag;

			iterator(const iterator&)            = delete;
			iterator(iterator&&)                 = default;
			iterator& operator=(const iterator&) = delete;
			iterator& operator=(iterator&&)      = default;

			iterator(View& view)
				: m_parent(std::addressof(view))
				, m_it(std::ranges::begin(view))
			{
				++(*this);
			}

			const String& operator*()
			{
				return m_grapehme;
			}

			iterator& operator++()
			{
				if (m_isEnd)
				{
					throw Error{U"[GraphemeView::Iterator::operator++] Out of range. Don't advance iterator over sentinel(end)."};
				}

				if (m_it == std::ranges::end(*m_parent))
				{
					m_isEnd = true;
				}

				std::u16string u16buffer;

				for (bool isFirst = true; m_it != std::ranges::end(*m_parent); ++m_it)
				{
					// コードポイント1つが多いことを考慮して始めに2つ読み込む
					if (isFirst)
					{
						const char32 first = *m_it;
						m_grapehme         = String{first};

						if (++m_it != std::ranges::end(*m_parent))
						{
							const char32 second = *m_it;
							u16buffer           = Unicode::ToUTF16(String{first, second});
						}
						else
						{
							break;
						}

						isFirst = false;
					}
					else
					{
						u16buffer += Unicode::ToUTF16(String{*m_it});
					}

					// ICU 初期化
					{
						utext_openUChars(m_utext, u16buffer.c_str(), u16buffer.size(), m_errorCode);

						if (m_errorCode.isFailure())
						{
							throw Error{U"[GraphemeView::Iterator::operator++] {}"_fmt(Unicode::FromUTF8(m_errorCode.errorName()))};
						}

						ubrk_setUText(m_brkit, m_utext, m_errorCode);

						if (m_errorCode.isFailure())
						{
							throw Error{U"[GraphemeView::Iterator::operator++] {}"_fmt(Unicode::FromUTF8(m_errorCode.errorName()))};
						}
					}

					// 書記素境界を見つけたら break
					// ex) u16buffer: ["a", "b"], ubrk_next: 1, u16buffer.size: 2
					if (ubrk_next(m_brkit) != u16buffer.size())
					{
						break;
					}

					// 無駄な push_back と pop_back を避ける
					m_grapehme.push_back(*m_it);
				}

				return *this;
			}

			iterator& operator++(int)
			{
				return (*this)++;
			}

			friend bool operator==(const iterator& it, const std::default_sentinel_t& sen)
			{
				return it.m_isEnd;
			}

		private:
			View*                         m_parent;
			std::ranges::iterator_t<View> m_it;
			bool                          m_isEnd     = false;
			String                        m_grapehme  = U"";
			icu::ErrorCode                m_errorCode = {};
			UText*                        m_utext     = utext_openUChars(NULL, u"", 0, m_errorCode);
			UBreakIterator*               m_brkit     = ubrk_open(UBRK_CHARACTER, uloc_getDefault(), NULL, 0, m_errorCode);
		};

		iterator begin()
		{
			return iterator{m_view};
		}

		auto end()
		{
			return std::default_sentinel;
		}

		// clang-format off
		GraphemeView() requires std::default_initializable<View> = default;
		// clang-format on

		template <std::ranges::input_range Range>
		requires std::ranges::viewable_range<Range>
		GraphemeView(Range&& range)
			: m_view(std::views::all(std::forward<Range>(range)))
		{}

		GraphemeView(View view)
			: m_view(std::move(view))
		{}

	private:
		View m_view;
	};

	template <std::ranges::input_range Range>
	requires std::ranges::viewable_range<Range>
	GraphemeView(Range&&) -> GraphemeView<std::views::all_t<Range>>;

	template <std::ranges::view View>
	GraphemeView(View) -> GraphemeView<View>;

	namespace views
	{
		namespace detail
		{
			struct GraphemeViewAdoptorClosure : std::ranges::range_adaptor_closure<GraphemeViewAdoptorClosure>
			{
				template <std::ranges::viewable_range Range>
				constexpr auto operator()(Range&& range) const
				{
					return GraphemeView(std::forward<Range>(range));
				}
			};
		} // namespace detail

		inline constexpr detail::GraphemeViewAdoptorClosure graphme{};
	} // namespace views

} // namespace tomolatoon
