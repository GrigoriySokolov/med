/**
@file
set of templates to define mandatory fields within containers

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include "field.hpp"
#include "count.hpp"
#include "tag.hpp"
#include "length.hpp"

namespace med {

template<
	class TAG_FLD_LEN,
	class FLD_LEN_NUM = void,
	class FLD_NUM     = void,
	class MIN_MAX     = min<1>,
	class MAX_MIN     = max<1>,
	typename Enable   = void >
struct mandatory
{
	static_assert(std::is_void<TAG_FLD_LEN>(), "MALFORMED MANDATORY");
};

//M<FIELD>
template <class FIELD>
struct mandatory<
	FIELD,
	void,
	void,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD>>
> : field_t<FIELD>
{
	using field_type = FIELD;
	//NOTE: since FIELD may have meta_info already we have to override it directly
	// w/o inheritance to avoid ambiguity
	using meta_info = make_meta_info_t<FIELD>;
};

//M<FIELD, SETTER>
template <class FIELD, class SETTER>
struct mandatory<
	FIELD,
	SETTER,
	void,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_setter_v<FIELD, SETTER>>
> : field_t<FIELD>
{
	using setter_type = SETTER;
	using meta_info = make_meta_info_t<FIELD>;
};


//M<FIELD, arity<NUM>>
template <class FIELD, std::size_t NUM>
struct mandatory<
	FIELD,
	arity<NUM>,
	void,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD>>
> : multi_field<FIELD, NUM, max<NUM>>
{
	static_assert(NUM > 1, "ARITY SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	using meta_info = make_meta_info_t<FIELD>;
};

//M<FIELD, min<MIN>, max<MAX>, CNT_GETTER>
template <class FIELD, std::size_t MIN, std::size_t MAX, class COUNT_GETTER>
struct mandatory<
	FIELD,
	min<MIN>,
	max<MAX>,
	COUNT_GETTER,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_count_getter_v<COUNT_GETTER>>
> : multi_field<FIELD, MIN, max<MAX>>
{
	using count_getter = COUNT_GETTER;
	using meta_info = make_meta_info_t<FIELD>;
	static_assert(MIN > 1, "MIN SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN");
};

//M<FIELD, min<MIN>, Pmax<MAX>, CNT_GETTER>
template <class FIELD, std::size_t MIN, std::size_t MAX, class COUNT_GETTER>
struct mandatory<
	FIELD,
	min<MIN>,
	pmax<MAX>,
	COUNT_GETTER,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_count_getter_v<COUNT_GETTER>>
> : multi_field<FIELD, MIN, pmax<MAX>>
{
	using meta_info = make_meta_info_t<FIELD>;
	using count_getter = COUNT_GETTER;
	static_assert(MIN > 1, "MIN SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN");
};

//M<CNT, FIELD, min<MIN>, max<MAX>>
template <class COUNTER, class FIELD, std::size_t MIN, std::size_t MAX>
struct mandatory<
	COUNTER,
	FIELD,
	min<MIN>,
	max<MAX>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_counter_v<COUNTER>>
> : multi_field<FIELD, MIN, max<MAX>>, COUNTER
{
	using meta_info = make_meta_info_t<FIELD>;
	static_assert(MIN > 1, "MIN SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN");
};

//M<CNT, FIELD, min<MIN>, Pmax<MAX>>
template <class COUNTER, class FIELD, std::size_t MIN, std::size_t MAX>
struct mandatory<
	COUNTER,
	FIELD,
	min<MIN>,
	pmax<MAX>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_counter_v<COUNTER>>
> : multi_field<FIELD, MIN, pmax<MAX>>, COUNTER
{
	using meta_info = make_meta_info_t<FIELD>;
	static_assert(MIN > 1, "MIN SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN");
};

//M<FIELD, min<MIN>, max<MAX>>
template <class FIELD, std::size_t MIN, std::size_t MAX>
struct mandatory<
	FIELD,
	min<MIN>,
	max<MAX>,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD>>
> : multi_field<FIELD, MIN, max<MAX>>
{
	using meta_info = make_meta_info_t<FIELD>;
	static_assert(MIN > 1, "MIN SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN");
};

//M<FIELD, min<MIN>, Pmax<MAX>>
template <class FIELD, std::size_t MIN, std::size_t MAX>
struct mandatory<
	FIELD,
	min<MIN>,
	pmax<MAX>,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD>>
> : multi_field<FIELD, MIN, pmax<MAX>>
{
	using meta_info = make_meta_info_t<FIELD>;
	static_assert(MIN > 1, "MIN SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN");
};

//M<FIELD, max<MAX>>
template <class FIELD, std::size_t MAX>
struct mandatory<
	FIELD,
	max<MAX>,
	void,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD>>
> : multi_field<FIELD, 1, max<MAX>>
{
	using meta_info = make_meta_info_t<FIELD>;
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

//M<FIELD, Pmax<MAX>>
template <class FIELD, std::size_t MAX>
struct mandatory<
	FIELD,
	pmax<MAX>,
	void,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD>>
> : multi_field<FIELD, 1, pmax<MAX>>
{
	using meta_info = make_meta_info_t<FIELD>;
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

//M<FIELD, max<MAX>, COUNT_GETTER>
template <class FIELD, std::size_t MAX, class COUNT_GETTER>
struct mandatory<
	FIELD,
	max<MAX>,
	COUNT_GETTER,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_count_getter_v<COUNT_GETTER>>
> : multi_field<FIELD, 1, max<MAX>>
{
	using meta_info = make_meta_info_t<FIELD>;
	using count_getter = COUNT_GETTER;
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

//M<FIELD, Pmax<MAX>, COUNT_GETTER>
template <class FIELD, std::size_t MAX, class COUNT_GETTER>
struct mandatory<
	FIELD,
	pmax<MAX>,
	COUNT_GETTER,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_count_getter_v<COUNT_GETTER>>
> : multi_field<FIELD, 1, pmax<MAX>>
{
	using meta_info = make_meta_info_t<FIELD>;
	using count_getter = COUNT_GETTER;
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

//M<CNT, FIELD, max<MAX>>
template <class COUNTER, class FIELD, std::size_t MAX>
struct mandatory<
	COUNTER,
	FIELD,
	max<MAX>,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_counter_v<COUNTER>>
> : multi_field<FIELD, 1, max<MAX>>, COUNTER
{
	using meta_info = make_meta_info_t<FIELD>;
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

//M<CNT, FIELD, Pmax<MAX>>
template <class COUNTER, class FIELD, std::size_t MAX>
struct mandatory<
	COUNTER,
	FIELD,
	pmax<MAX>,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_counter_v<COUNTER>>
> : multi_field<FIELD, 1, pmax<MAX>>, COUNTER
{
	using meta_info = make_meta_info_t<FIELD>;
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

//M<TAG, FIELD>
template <class TAG, class FIELD>
struct mandatory<
	TAG,
	FIELD,
	void,
	min<1>,
	max<1>,
	std::enable_if_t<is_tag_v<TAG> && is_field_v<FIELD>>
> : field_t<FIELD>
{
	using meta_info = make_meta_info_t<mi<mik::TAG, TAG>, FIELD>;
};

//M<TAG, FIELD, arity<NUM>
template <class TAG, class FIELD, std::size_t NUM>
struct mandatory<
	TAG,
	FIELD,
	arity<NUM>,
	min<1>,
	max<1>,
	std::enable_if_t<is_tag_v<TAG> && is_field_v<FIELD>>
> : multi_field<FIELD, NUM, max<NUM>>
{
	using meta_info = make_meta_info_t<mi<mik::TAG, TAG>, FIELD>;
	static_assert(NUM > 1, "ARITY SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

//M<TAG, FIELD, max<MAX>>
template <class TAG, class FIELD, std::size_t MAX>
struct mandatory<
	TAG,
	FIELD,
	max<MAX>,
	min<1>,
	max<1>,
	std::enable_if_t<is_tag_v<TAG> && is_field_v<FIELD>>
> : multi_field<FIELD, 1, max<MAX>>
{
	using meta_info = make_meta_info_t<mi<mik::TAG, TAG>, FIELD>;
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

//M<TAG, FIELD, Pmax<MAX>>
template <class TAG, class FIELD, std::size_t MAX>
struct mandatory<
	TAG,
	FIELD,
	pmax<MAX>,
	min<1>,
	max<1>,
	std::enable_if_t<is_tag_v<TAG> && is_field_v<FIELD>>
> : multi_field<FIELD, 1, pmax<MAX>>
{
	using meta_info = make_meta_info_t<mi<mik::TAG, TAG>, FIELD>;
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

//M<TAG, FIELD, min<MIN>, max<MAX>>
template <class TAG, class FIELD, std::size_t MIN, std::size_t MAX>
struct mandatory<
	TAG,
	FIELD,
	min<MIN>,
	max<MAX>,
	max<1>,
	std::enable_if_t<is_tag_v<TAG> && is_field_v<FIELD>>
> : multi_field<FIELD, MIN, max<MAX>>
{
	using meta_info = make_meta_info_t<mi<mik::TAG, TAG>, FIELD>;
	static_assert(MIN > 1, "MIN SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN");
};

//M<TAG, FIELD, min<MIN>, Pmax<MAX>>
template <class TAG, class FIELD, std::size_t MIN, std::size_t MAX>
struct mandatory<
	TAG,
	FIELD,
	min<MIN>,
	pmax<MAX>,
	max<1>,
	std::enable_if_t<is_tag_v<TAG> && is_field_v<FIELD>>
> : multi_field<FIELD, MIN, pmax<MAX>>
{
	using meta_info = make_meta_info_t<mi<mik::TAG, TAG>, FIELD>;
	static_assert(MIN > 1, "MIN SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN");
};

//M<LEN, FIELD>
template <class LEN, class FIELD>
struct mandatory<
	LEN,
	FIELD,
	void,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_length_v<LEN>>
> : field_t<FIELD>
{
	using meta_info = make_meta_info_t<mi<mik::LEN, LEN>, FIELD>;
};

//M<LEN, FIELD, arity<NUM>>
template <class LEN, class FIELD, std::size_t NUM>
struct mandatory<
	LEN,
	FIELD,
	arity<NUM>,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_length_v<LEN>>
> : multi_field<FIELD, NUM, max<NUM>>
{
	using meta_info = make_meta_info_t<mi<mik::LEN, LEN>, FIELD>;
	static_assert(NUM > 1, "ARITY SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

//M<TAG, LEN, FIELD>
template <class TAG, class LEN, class FIELD>
struct mandatory<
	TAG,
	LEN,
	FIELD,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_tag_v<TAG> && is_length_v<LEN>>
> : field_t<FIELD>
{
	using meta_info = make_meta_info_t<mi<mik::TAG, TAG>, mi<mik::LEN, LEN>, FIELD>;
};

//M<TAG, LEN, FIELD, arity<NUM>>
template <class TAG, class LEN, class FIELD, std::size_t NUM>
struct mandatory<
	TAG,
	LEN,
	FIELD,
	arity<NUM>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_tag_v<TAG> && is_length_v<LEN>>
> : multi_field<FIELD, NUM, max<NUM>>
{
	using meta_info = make_meta_info_t<mi<mik::TAG, TAG>, mi<mik::LEN, LEN>, FIELD>;
	static_assert(NUM > 1, "ARITY SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

//M<LEN, FIELD, max<MAX>>
template <class LEN, class FIELD, std::size_t MAX>
struct mandatory<
	LEN,
	FIELD,
	max<MAX>,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_length_v<LEN>>
> : multi_field<FIELD, 1, max<MAX>>
{
	using meta_info = make_meta_info_t<mi<mik::LEN, LEN>, FIELD>;
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

//M<LEN, FIELD, Pmax<MAX>>
template <class LEN, class FIELD, std::size_t MAX>
struct mandatory<
	LEN,
	FIELD,
	pmax<MAX>,
	min<1>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_length_v<LEN>>
> : multi_field<FIELD, 1, pmax<MAX>>
{
	using meta_info = make_meta_info_t<mi<mik::LEN, LEN>, FIELD>;
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

//M<TAG, LEN, FIELD, max<MAX>>
template <class TAG, class LEN, class FIELD, std::size_t MAX>
struct mandatory<
	TAG,
	LEN,
	FIELD,
	max<MAX>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_tag_v<TAG> && is_length_v<LEN>>
> : multi_field<FIELD, 1, max<MAX>>
{
	using meta_info = make_meta_info_t<mi<mik::TAG, TAG>, mi<mik::LEN, LEN>, FIELD>;
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

//M<TAG, LEN, FIELD, Pmax<MAX>>
template <class TAG, class LEN, class FIELD, std::size_t MAX>
struct mandatory<
	TAG,
	LEN,
	FIELD,
	pmax<MAX>,
	max<1>,
	std::enable_if_t<is_field_v<FIELD> && is_tag_v<TAG> && is_length_v<LEN>>
> : multi_field<FIELD, 1, pmax<MAX>>
{
	using meta_info = make_meta_info_t<mi<mik::TAG, TAG>, mi<mik::LEN, LEN>, FIELD>;
	static_assert(MAX > 1, "MAX SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
};

//M<TAG, LEN, FIELD, min<MIN>, max<MAX>>
template <class TAG, class LEN, class FIELD, std::size_t MIN, std::size_t MAX>
struct mandatory<
	TAG,
	LEN,
	FIELD,
	min<MIN>,
	max<MAX>,
	std::enable_if_t<is_field_v<FIELD> && is_tag_v<TAG> && is_length_v<LEN>>
> : multi_field<FIELD, MIN, max<MAX>>
{
	using meta_info = make_meta_info_t<mi<mik::TAG, TAG>, mi<mik::LEN, LEN>, FIELD>;
	static_assert(MIN > 1, "MIN SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN");
};

//M<TAG, LEN, FIELD, min<MIN>, Pmax<MAX>>
template <class TAG, class LEN, class FIELD, std::size_t MIN, std::size_t MAX>
struct mandatory<
	TAG,
	LEN,
	FIELD,
	min<MIN>,
	pmax<MAX>,
	std::enable_if_t<is_field_v<FIELD> && is_tag_v<TAG> && is_length_v<LEN>>
> : multi_field<FIELD, MIN, pmax<MAX>>
{
	using meta_info = make_meta_info_t<mi<mik::TAG, TAG>, mi<mik::LEN, LEN>, FIELD>;
	static_assert(MIN > 1, "MIN SHOULD BE MORE THAN 1 OR NOT SPECIFIED");
	static_assert(MAX > MIN, "MAX SHOULD BE MORE THAN MIN");
};

} //namespace med
