/**
@file
base container class used in sequence and set

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include "debug.hpp"
#include "accessor.hpp"
#include "length.hpp"
#include "sl/field_copy.hpp"
#include "meta/typelist.hpp"
#include "meta/foreach.hpp"

namespace med {

namespace sl {

struct cont_clear
{
	template <class IE, class SEQ>
	static void apply(SEQ& s)           { static_cast<IE&>(s).clear(); }
	template <class SEQ>
	static constexpr void apply(SEQ&)   { }
};

struct cont_copy
{
	template <class IE, class TO, class FROM, class... ARGS>
	static void apply(TO& to, FROM const& from, ARGS&&... args)
	{
		using field_t = get_field_type_t<IE>;
		auto const& from_field = from.m_ies.template as<field_t>();
		if (from_field.is_set())
		{
			auto& to_field = to.m_ies.template as<field_t>();
			if constexpr (is_multi_field_v<IE>)
			{
				to_field.clear();
				for (auto const& rhs : from_field)
				{
					auto* p = to_field.push_back(std::forward<ARGS>(args)...);
					p->copy(rhs, std::forward<ARGS>(args)...);
				}
			}
			else
			{
				to_field.copy(from_field, std::forward<ARGS>(args)...);
			}
		}
	}

	template <class TO, class FROM, class... ARGS>
	static void apply(TO&, FROM const&, ARGS&&...)  { }
};

struct cont_len
{
	static constexpr std::size_t op(std::size_t r1, std::size_t r2) { return r1 + r2; }

	template <class IE, class SEQ, class ENCODER>
	static std::size_t apply(SEQ const& seq, ENCODER& encoder)
	{
		using mi = meta::produce_info_t<ENCODER, IE>;
		if constexpr (is_multi_field_v<IE>)
		{
			IE const& ie = seq;
			std::size_t len = 0;
			for (auto& v : ie)
			{
				len += sl::ie_length<mi>(v, encoder);
			}
			CODEC_TRACE("%s[%s]* len=%zu", __FUNCTION__, name<IE>(), len);
			return len;
		}
		else
		{
			IE const& ie = seq;
			std::size_t const len = has_setter_type_v<IE> || ie.is_set()
					? sl::ie_length<mi>(ie, encoder)
					: 0;
			CODEC_TRACE("%s[%s] len=%zu", __FUNCTION__, name<IE>(), len);
			return len;
		}
	}

	template <class SEQ, class ENCODER>
	static constexpr std::size_t apply(SEQ const&, ENCODER&)        { return 0; }
};

struct cont_is
{
	static constexpr bool op(bool r1, bool r2)  { return r1 || r2; }

	template <class IE, class SEQ>
	static constexpr bool apply(SEQ const& seq)
	{
		//optional or mandatory field w/ setter => can be set implicitly
		if constexpr (is_optional_v<IE> || has_setter_type_v<IE>)
		{
			//CODEC_TRACE("is_set[%s]=%d", name<IE>(), static_cast<IE const&>(seq).is_set());
			return static_cast<IE const&>(seq).is_set();
		}
		else //mandatory field w/o setter => s.b. set explicitly
		{
			if constexpr (is_predefined_v<IE>) //don't account predefined IEs like init/const/def
			{
				return false;
			}
			else
			{
				//CODEC_TRACE("is_set[%s]=%d", name<IE>(), static_cast<IE const&>(seq).is_set());
				return static_cast<IE const&>(seq).is_set();
			}
		}
	}

	template <class SEQ>
	static constexpr bool apply(SEQ const&)     { return false; }
};

struct cont_eq
{
	static constexpr bool op(bool r1, bool r2)  { return r1 && r2; }

	template <class IE, class SEQ>
	static constexpr bool apply(SEQ const& lhs, SEQ const& rhs)
	{
		return static_cast<IE const&>(lhs) == static_cast<IE const&>(rhs);
	}

	template <class SEQ>
	static constexpr bool apply(SEQ const&, SEQ const&)     { return true; }
};

//-----------------------------------------------------------------------
template <class IE>
constexpr std::size_t field_arity()
{
	if constexpr (is_multi_field_v<IE>)
	{
		return IE::max;
	}
	else
	{
		return 1;
	}
}

}	//end: namespace sl

template <class... IES>
class container : public IE<CONTAINER>
{
public:
	using container_t = container<IES...>;
	using ies_types = meta::typelist<IES...>;

	template <class T>
	static constexpr bool has()             { return not std::is_void_v<meta::find<ies_types, sl::field_at<T>>>; }

	template <class FIELD>
	decltype(auto) ref()
	{
		static_assert(!std::is_const<FIELD>(), "ATTEMPT TO COPY FROM CONST REF");
		auto& ie = m_ies.template as<FIELD>();
		using IE = remove_cref_t<decltype(ie)>;
		if constexpr (is_multi_field<IE>())
		{
			return static_cast<IE&>(ie);
		}
		else
		{
			return static_cast<FIELD&>(ie);
		}
	}

	template <class FIELD>
	decltype(auto) get() const
	{
		auto& ie = m_ies.template as<FIELD>();
		return get_field<FIELD>(ie);
	}

	auto field()                            { return make_accessor(*this); }
	auto field() const                      { return make_accessor(*this); }
	auto cfield() const                     { return make_accessor(*this); }

	template <class FIELD>
	std::size_t count() const               { return field_count(m_ies.template as<FIELD>()); }

	template <class FIELD>
	static constexpr std::size_t arity()    { return sl::field_arity<meta::find<ies_types, sl::field_at<FIELD>>>(); }

	template <class FIELD>
	void clear()                            { m_ies.template as<FIELD>().clear(); }
	void clear()                            { meta::foreach<ies_types>(sl::cont_clear{}, this->m_ies); }
	bool is_set() const                     { return meta::fold<ies_types>(sl::cont_is{}, this->m_ies); }
	template <class ENC>
	std::size_t calc_length(ENC& enc) const { return meta::fold<ies_types>(sl::cont_len{}, this->m_ies, enc); }

	template <class FROM, class... ARGS>
	void copy(FROM const& from, ARGS&&... args)
	{ meta::foreach<ies_types>(sl::cont_copy{}, *this, from, std::forward<ARGS>(args)...); }

	template <class TO, class... ARGS>
	void copy_to(TO& to, ARGS&&... args) const
	{ meta::foreach<ies_types>(sl::cont_copy{}, to, *this, std::forward<ARGS>(args)...); }

	bool operator==(container const& rhs) const { return meta::fold<ies_types>(sl::cont_eq{}, this->m_ies, rhs.m_ies); }
	bool operator!=(container const& rhs) const { return !this->operator==(rhs); }

protected:
	friend struct sl::cont_copy;

	struct ies_t : IES...
	{
		template <class FIELD>
		decltype(auto) as() const
		{
			using IE = meta::find<ies_types, sl::field_at<FIELD>>;
			static_assert(!std::is_void<IE>(), "NO SUCH FIELD");
			return static_cast<IE const&>(*this);
		}

		template <class FIELD>
		decltype(auto) as()
		{
			using IE = meta::find<ies_types, sl::field_at<FIELD>>;
			static_assert(!std::is_void<IE>(), "NO SUCH FIELD");
			return static_cast<IE&>(*this);
		}
	};

	ies_t m_ies;
};

} //end: namespace med
