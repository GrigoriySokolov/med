/**
@file
choice class as union

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include <new>

#include "exception.hpp"
#include "tag.hpp"
#include "length.hpp"
#include "encode.hpp"
#include "decode.hpp"
#include "meta/unique.hpp"
#include "meta/typelist.hpp"
#include "meta/foreach.hpp"

namespace med {

namespace sl {

template <class FIELD>
struct choice_at
{
	template <class T>
	using type = std::is_same<FIELD, typename T::option_type>;
};

struct choice_if
{
	template <class IE, class T, class... Ts>
	static bool check(T const& v, Ts&&...)
	{
		auto& hdr = v.get_header();
		return hdr.is_set() && IE::option_value_type::match( med::get_tag(hdr) );
	}
};

struct choice_len : choice_if
{
	template <class IE, class TO, class ENCODER>
	static std::size_t apply(TO const& to, ENCODER& encoder)
	{
		using type = typename IE::option_type;
		void const* store_p = &to.m_storage;
		return field_length(to.get_header(), encoder)
			+ field_length(*static_cast<type const*>(store_p), encoder);
	}

	template <class TO, class ENCODER>
	static constexpr std::size_t apply(TO const&, ENCODER&)
	{
		return 0;
	}
};

struct choice_copy : choice_if
{
	template <class IE, class TO, class FROM, class... ARGS>
	static void apply(TO& to, FROM const& from, ARGS&&... args)
	{
		using value_type = typename IE::option_type;
		void const* store_p = &from.m_storage;
		to.template ref<value_type>().copy(*static_cast<value_type const*>(store_p), std::forward<ARGS>(args)...);
		to.header().copy(from.get_header(), std::forward<ARGS>(args)...);
	}

	template <class TO, class FROM, class... ARGS>
	static constexpr void apply(TO&, FROM const&, ARGS&&...) { }
};

struct choice_name
{
	template <class IE, typename TAG>
	static constexpr bool check(TAG const& tag)
	{
		return IE::option_value_type::match( tag );
	}

	template <class IE, typename TAG>
	static constexpr char const* apply(TAG const&)
	{
		return name<typename IE::option_type>();
	}

	template <typename TAG>
	static constexpr char const* apply(TAG const&)
	{
		return nullptr;
	}
};

struct choice_enc : choice_if
{
	template <class IE, class TO, class ENCODER>
	static void apply(TO const& to, ENCODER& encoder)
	{
		using type = typename IE::option_type;
		CODEC_TRACE("CASE[%s]", name<type>());
		void const* store_p = &to.m_storage;

		med::encode(encoder, to.get_header());
		med::encode(encoder, *static_cast<type const*>(store_p));
	}

	template <class TO, class ENCODER>
	static void apply(TO const& to, ENCODER&)
	{
		MED_THROW_EXCEPTION(unknown_tag, name<TO>(), get_tag(to.get_header()))
	}
};

struct choice_dec : choice_if
{
	template <class IE, class TO, class DECODER, class UNEXP>
	static void apply(TO& to, DECODER& decoder, UNEXP& unexp)
	{
		using type = typename IE::option_type;
		CODEC_TRACE("->CASE[%s]", name<type>());
		auto* p = new (&to.m_storage) type{};
		med::decode(decoder, *p, unexp);
	}

	template <class TO, class DECODER, class UNEXP>
	static void apply(TO& to, DECODER& decoder, UNEXP& unexp)
	{
		CODEC_TRACE("unexp CASE[%s] tag=%zu", name<TO>(), std::size_t(get_tag(to.get_header())));
		unexp(decoder, to, to.get_header());
	}
};

}	//end: namespace sl


template <class HEADER, class ...CASES>
class choice : public IE<CONTAINER>
{
private:
	template <class T>
	struct selector
	{
		explicit selector(T* that) noexcept
			: m_this(that)
		{}

		template <class U> operator U&()
		{
			static_assert(!std::is_const<T>(), "CONST CHOICE RETURNS A POINTER, NOT REFERENCE!");
			return m_this->template ref<U>();
		}

		template <class U> operator U const* ()
		{
			return m_this->template get<U>();
		}

		T* m_this;
	};
	template <class T>
	static auto make_selector(T* that) { return selector<T>{that}; }

public:
	using header_type = HEADER;
	using ies_types = meta::typelist<CASES...>;

	template <typename TAG>
	static constexpr char const* name_tag(TAG const& tag)
	{
		return meta::for_if<ies_types>(sl::choice_name{}, tag);
	}

	header_type const& header() const       { return m_header; }
	header_type const& get_header() const   { return m_header; }
	header_type& header()                   { return m_header; }

	void clear()                            { header().clear(); }
	bool is_set() const                     { return header().is_set(); }
	template <class ENC>
	std::size_t calc_length(ENC& enc) const { return meta::for_if<ies_types>(sl::choice_len{}, *this, enc); }

	auto select()                           { return make_selector(this); }
	auto select() const                     { return make_selector(this); }
	auto cselect() const                    { return make_selector(this); }

	template <class CASE>
	static constexpr bool has()             { return not std::is_void_v<meta::find<ies_types, sl::choice_at<CASE>>>; }

	template <class CASE>
	CASE& ref()
	{
		//TODO: how to prevent a copy when callee-side re-uses reference by mistake?
		static_assert(!std::is_const<CASE>(), "REFERENCE IS NOT FOR ACCESSING AS CONST");
		using IE = meta::find<ies_types, sl::choice_at<CASE>>;
		static_assert(!std::is_void<IE>(), "NO SUCH CASE IN CHOICE");
		void* store_p = &m_storage;
		return *(set_tag(header(), IE::option_value_type::get_encoded())
			? new (store_p) CASE{}
			: static_cast<CASE*>(store_p));
	}

	template <class CASE>
	CASE const* get() const
	{
		using IE = meta::find<ies_types, sl::choice_at<CASE>>;
		static_assert(!std::is_void<IE>(), "NO SUCH CASE IN CHOICE");
		void const* store_p = &m_storage;
		return IE::option_value_type::match( get_tag(header()) ) ? static_cast<CASE const*>(store_p) : nullptr;
	}

	template <class FROM, class... ARGS>
	void copy(FROM const& from, ARGS&&... args)
	{ meta::for_if<ies_types>(sl::choice_copy{}, *this, from, std::forward<ARGS>(args)...); }

	template <class TO, class... ARGS>
	void copy_to(TO& to, ARGS&&... args) const
	{ meta::for_if<ies_types>(sl::choice_copy{}, to, *this, std::forward<ARGS>(args)...); }

	template <class ENCODER>
	void encode(ENCODER& encoder) const    { meta::for_if<ies_types>(sl::choice_enc{}, *this, encoder); }

	template <class DECODER, class UNEXP>
	void decode(DECODER& decoder, UNEXP& unexp)
	{
		static_assert(std::is_void_v<meta::unique_t<option_getter, ies_types>>
			, "SEE ERROR ON INCOMPLETE TYPE/UNDEFINED TEMPLATE HOLDING IEs WITH CLASHED TAGS");
		med::decode(decoder, header(), unexp);
		meta::for_if<ies_types>(sl::choice_dec{}, *this, decoder, unexp);
	}

private:
	friend struct sl::choice_len;
	friend struct sl::choice_copy;
	friend struct sl::choice_enc;
	friend struct sl::choice_dec;

	using storage_type = std::aligned_union_t<0, typename CASES::option_type...>;

	header_type  m_header;
	storage_type m_storage;
};

} //end: namespace med
