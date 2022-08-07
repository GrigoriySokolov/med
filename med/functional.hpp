/**
@file
set of auxilary traits/functions

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include <utility>
#include <type_traits>

namespace med {

template <typename T> struct remove_cref : std::remove_reference<std::remove_const_t<T>> {};
template <typename T> using remove_cref_t = typename remove_cref<T>::type;

template <class, class = void> struct has_ie_type : std::false_type {};
template <class T> struct has_ie_type<T, std::void_t<typename T::ie_type>> : std::true_type {};
template <class T> constexpr bool has_ie_type_v = has_ie_type<T>::value;

template <class, class = void> struct has_field_type : std::false_type {};
template <class T> struct has_field_type<T, std::void_t<typename T::field_type>> : std::true_type {};
template <class T> constexpr bool has_field_type_v = has_field_type<T>::value;

template <class T, typename = void>
struct get_field_type
{
	using type = T;
};
template <class T>
struct get_field_type<T, std::enable_if_t<has_field_type<T>::value>>
{
	using type = typename get_field_type<typename T::field_type>::type;
};
template <class T> using get_field_type_t = typename get_field_type<T>::type;

//checks if T looks like a functor to test condition of a field presense
template <typename T, typename = void> struct is_condition : std::false_type {};
template <typename T> struct is_condition<T,
	std::enable_if_t<std::is_same_v<bool, decltype(std::declval<T>()(std::false_type{}))>>
> : std::true_type {};
template <class T> constexpr bool is_condition_v = is_condition<T>::value;


template <class, class = void> struct has_condition : std::false_type {};
template <class T> struct has_condition<T, std::void_t<typename T::condition>> : std::true_type {};
template <class T> constexpr bool has_condition_v = has_condition<T>::value;


template<class, class, class = void> struct is_setter : std::false_type {};
template<class IE, class FUNC> struct is_setter<IE, FUNC,
		std::void_t<decltype(std::declval<FUNC>()(std::declval<IE&>(), std::false_type{}))>
> : std::true_type {};
template <class IE, class FUNC> constexpr bool is_setter_v = is_setter<IE, FUNC>::value;

template <class, class = void> struct has_setter_type : std::false_type {};
template <class T> struct has_setter_type<T, std::void_t<typename T::setter_type>> : std::true_type {};
template <class T> constexpr bool has_setter_type_v = has_setter_type<T>::value;

template <class T, class = void> struct has_default_value : std::false_type {};
template <class T> struct has_default_value<T, std::void_t<decltype(T::traits::default_value)>> : std::true_type {};
template <class T> constexpr bool has_default_value_v = has_default_value<T>::value;

template<typename T, typename = void> struct is_predefined : std::false_type {};
template <class T> struct is_predefined<T, std::enable_if_t<T::is_const>> : std::true_type {};
template <class T> constexpr bool is_predefined_v = is_predefined<T>::value;

}	//end: namespace med
