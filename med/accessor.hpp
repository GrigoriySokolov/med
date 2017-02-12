/**
@file
accessors for fields in container

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include <type_traits>

#include "field_proxy.hpp"

namespace med {

namespace detail {

template <class T>
struct access
{
	template <class R>
	static auto& as_mandatory(R&& ret)
	{
		static_assert(!std::is_same<field_proxy<T const>, R>(), "ACCESSING OPTIONAL NOT BY POINTER");
		return ret;
	}

	template <class R>
	static auto as_optional(R&& ret)
	{
		static_assert(std::is_same<field_proxy<T const>, R>(), "ACCESSING MANDATORY NOT BY REFERENCE");
		return ret;
	}
};

template <class CONTAINER>
struct accessor
{
	explicit accessor(CONTAINER& c) noexcept : m_that(c)  { }

	template <class T, class = std::enable_if_t<!std::is_pointer<T>::value>> operator T& ()
	{
		//static_assert(!std::is_pointer<T>(), "INVALID ACCESS BY NON-CONST POINTER");
		return m_that.template ref<T>();
	}
	template <class T, class = std::enable_if_t<std::is_const<T>::value>> operator T* () const
	{
		return access<T>::as_optional(m_that.template get<std::remove_const_t<T>>());
	}

private:
	CONTAINER& m_that;
};

template <class CONTAINER>
struct caccessor
{
	explicit caccessor(CONTAINER const& c) noexcept : m_that(c) { }
	template <class T> operator T const* () const
	{
		return access<T>::as_optional(m_that.template get<T>());
	}

	template <class T> operator T const& () const
	{
		return access<T>::as_mandatory(m_that.template get<T>());
	}

private:
	CONTAINER const& m_that;
};

template <class CONTAINER>
struct index_caccessor
{
	index_caccessor(CONTAINER const& c, std::size_t i) noexcept : m_that{c}, m_index{i}  { }
	template <class T> operator T const* () const
	{
		return m_that.template get<T>(m_index);
	}

private:
	CONTAINER const&  m_that;
	std::size_t const m_index;
};

}	//end: namespace detail

template <class CONTAINER>
inline auto make_accessor(CONTAINER& c)
{
	return detail::accessor<CONTAINER>{ c };
}

template <class CONTAINER>
inline auto make_accessor(CONTAINER const& c)
{
	return detail::caccessor<CONTAINER>{ c };
}

template <class CONTAINER>
inline auto make_accessor(CONTAINER const& c, std::size_t index)
{
	return detail::index_caccessor<CONTAINER>{ c, index };
}

}	//end: namespace med
