/**
@file
decoding entry point

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include <utility>

#include "exception.hpp"
#include "optional.hpp"
#include "state.hpp"
#include "padding.hpp"
#include "length.hpp"
#include "name.hpp"
#include "value.hpp"
#include "meta/typelist.hpp"


namespace med {

//structure layer
namespace sl {

template <class META_INFO = meta::typelist<>, class EXPOSED = void, class EXP_LEN = void>
struct decode_type_context
{
	using meta_info_type = META_INFO;
	using exposed_type = EXPOSED;
	using explicit_length_type = EXP_LEN;
};


//Tag
//PEEK_SAVE - to preserve state in case of peek tag or not
template <class TAG_TYPE, bool PEEK_SAVE, class DECODER>
constexpr auto decode_tag(DECODER& decoder)
{
	TAG_TYPE ie;
	typename as_writable_t<TAG_TYPE>::value_type value{};
	if constexpr (PEEK_SAVE && is_peek_v<TAG_TYPE>)
	{
		CODEC_TRACE("PEEK TAG %s", class_name<TAG_TYPE>());
		if (decoder(PUSH_STATE{}, ie))
		{
			value = decoder(ie, IE_TAG{});
			CODEC_TRACE("%s=%zX [%s]", __FUNCTION__, std::size_t(value), class_name<TAG_TYPE>());
			decoder(POP_STATE{});
		}
	}
	else
	{
		value = decoder(ie, IE_TAG{});
		CODEC_TRACE("%s=%zX [%s]", __FUNCTION__, std::size_t(value), class_name<TAG_TYPE>());
	}
	return value;
}
//Length
template <class LEN_TYPE, class DECODER>
constexpr std::size_t decode_len(DECODER& decoder)
{
	LEN_TYPE ie;
	decoder(ie, IE_LEN{});
	return value_to_length(ie);
}

template <class TYPE_CTX, class DECODER, class IE, class... DEPS>
constexpr void ie_decode(DECODER& decoder, IE& ie, DEPS&... deps);


template <class LEN_TYPE, class TYPE_CTX, class DECODER, class IE, class... DEPS>
void apply_len(DECODER& decoder, IE& ie, std::size_t len, DEPS&... deps)
{
	using META_INFO = typename TYPE_CTX::meta_info_type;
	using EXPOSED = typename TYPE_CTX::exposed_type;
	using EXP_LEN = typename TYPE_CTX::explicit_length_type;

	using pad_traits = typename get_padding<LEN_TYPE>::type;
	using dependency_type = get_dependency_t<LEN_TYPE>;

	if constexpr (std::is_void_v<dependency_type>)
	{
		//TODO: may have fixed length like in BER:NULL/BOOL so need to check here
		auto end = decoder(PUSH_SIZE{len});
		if constexpr (std::is_void_v<pad_traits>)
		{
			ie_decode<decode_type_context<META_INFO, EXPOSED, EXP_LEN>>(decoder, ie, deps...);
			//TODO: ??? as warning not error
			if (0 != end.size()) { MED_THROW_EXCEPTION(overflow, name<IE>(), end.size()); }
		}
		else
		{
			CODEC_TRACE("padded %s...:", name<LEN_TYPE>());
			using pad_t = typename DECODER::template padder_type<pad_traits, DECODER>;
			pad_t pad{decoder};
			ie_decode<decode_type_context<META_INFO, EXPOSED, EXP_LEN>>(decoder, ie, deps...);
			CODEC_TRACE("before padding %s: end=%zu padding=%u", name<LEN_TYPE>(), end.size(), pad.padding_size());
			//if (end.size() != pad.padding_size()) { MED_THROW_EXCEPTION(overflow, name<IE>(), end.size()); }
			end.restore_end();
			pad.add_padding();
		}
	}
	else
	{
		CODEC_TRACE("'%s' depends on '%s'", name<LEN_TYPE>(), name<dependency_type>());

		using dep_decoder_t = typename DECODER::template make_dependent<LEN_TYPE, dependency_type>;
		dep_decoder_t dep_decoder{decoder.get_context()};

		auto end = dep_decoder(PUSH_SIZE{len, false});
		if constexpr (std::is_void_v<pad_traits>)
		{
			ie_decode<decode_type_context<META_INFO, EXPOSED, EXP_LEN>>(dep_decoder, ie, end, deps...);
			//TODO: ??? as warning not error
			CODEC_TRACE("decoded '%s' depends on '%s'", name<LEN_TYPE>(), name<dependency_type>());
			if (0 != end.size()) { MED_THROW_EXCEPTION(overflow, name<IE>(), end.size()); }
		}
		else
		{
			CODEC_TRACE("padded %s...:", name<LEN_TYPE>());
			using pad_t = typename DECODER::template padder_type<pad_traits, DECODER>;
			pad_t pad{dep_decoder};
			ie_decode<decode_type_context<META_INFO, EXPOSED, EXP_LEN>>(dep_decoder, ie, end, deps...);
			CODEC_TRACE("decoded '%s' depends on '%s'", name<LEN_TYPE>(), name<dependency_type>());
			//if (end.size() != pad.padding_size()) { MED_THROW_EXCEPTION(overflow, name<IE>(), end.size()); }
			end.restore_end();
			pad.add_padding();
		}
	}
}

template <class DECODER, class IE, class BUF_START>
constexpr std::size_t explicit_len(DECODER& decoder, IE& ie, BUF_START start)
{
	auto const delta = decoder(GET_STATE{}) - start;
	return value_to_length(ie) - delta;
}


template <class DEPENDENT, class DEPENDENCY, class DEPS>
constexpr void invoke_dependency(DEPENDENCY const& ie, DEPS& deps)
{
	deps.commit(DEPENDENT::dependency(ie));
}

template <class TYPE_CTX, class DECODER, class IE, class... DEPS>
constexpr void ie_decode(DECODER& decoder, IE& ie, DEPS&... deps)
{
	using META_INFO = typename TYPE_CTX::meta_info_type;
	using EXPOSED = typename TYPE_CTX::exposed_type;
	using EXP_LEN = typename TYPE_CTX::explicit_length_type;

	if constexpr (not meta::list_is_empty_v<META_INFO>)
	{
		using mi = meta::list_first_t<META_INFO>;
		using mi_rest = meta::list_rest_t<META_INFO>;
		CODEC_TRACE("%s[%s-%s](%zu) w/ %s", __FUNCTION__, name<IE>(), name<EXPOSED>(), sizeof... (deps), class_name<mi>());

		if constexpr (mi::kind == mik::LEN)
		{
			using len_t = typename mi::info_type;
			if constexpr (APresentIn<len_t, IE>)
			{
				static_assert(std::is_void_v<EXP_LEN>, "only single depth supported");
				static_assert(std::is_void_v<EXPOSED>, "overlapping dependency and explicit length");
				static_assert(sizeof...(deps) == 0);
				auto const start = decoder(GET_STATE{});
				CODEC_TRACE(">>> explicit LV[%s]", name<len_t>());
				ie_decode<decode_type_context<mi_rest, EXPOSED, len_t>>(decoder, ie, start);
			}
			else
			{
				auto const len = decode_len<len_t>(decoder);
				apply_len<len_t, decode_type_context<mi_rest, EXPOSED, EXP_LEN>>(decoder, ie, len, deps...);
			}
		}
		else
		{
			if constexpr (mi::kind == mik::TAG)
			{
				using tag_type = typename mi::info_type;
				auto const tag = decode_tag<tag_type, true>(decoder);
				if (not tag_type::match(tag))
				{
					//NOTE: this can only be called for mandatory field thus it's fail case (not unexpected)
					MED_THROW_EXCEPTION(unknown_tag, name<IE>(), tag)
				}
			}
			ie_decode<decode_type_context<mi_rest, EXPOSED, EXP_LEN>>(decoder, ie, deps...);
		}
	}
	else
	{
		using ie_type = typename IE::ie_type;
		if constexpr (std::is_base_of_v<CONTAINER, ie_type>)
		{
			CODEC_TRACE(">>> %s exposed=%s", name<IE>(), name<EXPOSED>());
			if constexpr (not std::is_void_v<EXPOSED>)
			{
				/* NOTE:
				1) exposed is valid for sequence only
				2) only 1st IE is expected to be exposed so now it s.b. skipped as was decoded in meta
				*/
				ie.template decode<meta::list_rest_t<typename IE::ies_types>>(decoder, deps...);
			}
			else
			{
				ie.decode(decoder, deps...);
			}
			CODEC_TRACE("<<< %s exposed=%s", name<IE>(), name<EXPOSED>());
		}
		else
		{
			CODEC_TRACE("%s[%.30s](%zu): %s", __FUNCTION__, class_name<IE>(), sizeof... (deps), class_name<ie_type>());
			if constexpr (is_peek_v<IE>)
			{
				CODEC_TRACE("PEEK %s[%s]", __FUNCTION__, name<IE>());
				if (decoder(PUSH_STATE{}, ie))
				{
					decoder(ie, ie_type{});
					decoder(POP_STATE{});
				}
			}
			else
			{
				using field_t = get_field_type_t<IE>;
				CODEC_TRACE("PRIMITIVE %s[%s]", __FUNCTION__, name<IE>());
				decoder(ie, ie_type{});

				if constexpr (std::is_same_v<field_t, EXP_LEN>)
				{
					auto const len = explicit_len(decoder, ie, deps...);
					CODEC_TRACE("<<< explicit LV[%s]=%#zX", name<EXP_LEN>(), len);
					apply_len<IE, decode_type_context<>>(decoder, ie, len);
				}
				else
				{
					using dependency_type = get_dependency_t<DECODER>;
					if constexpr (std::is_same_v<field_t, dependency_type>)
					{
						using dependent_type = typename DECODER::dependent_type;
						CODEC_TRACE("decoded dependency '%s' of dependent '%s' deps=%zu", name<IE>(), class_name<dependent_type>(), sizeof...(DEPS));
						invoke_dependency<dependent_type, dependency_type>(ie, deps...);
					}
				}
			}
		}
	}
}

}	//end: namespace sl

template <class TYPE_CTX, class DECODER, AHasIeType IE, class... DEPS>
constexpr void decode(DECODER&& decoder, IE& ie, DEPS&... deps)
{
	using EXPOSED = typename TYPE_CTX::exposed_type;
	using EXP_LEN = typename TYPE_CTX::explicit_length_type;
	using META_INFO = meta::produce_info_t<DECODER, IE>;
	sl::ie_decode<sl::decode_type_context<META_INFO, EXPOSED, EXP_LEN>>(decoder, ie, deps...);
}

template <class DECODER, AHasIeType IE, class... DEPS>
constexpr void decode(DECODER&& decoder, IE& ie, DEPS&... deps)
{
	using META_INFO = meta::produce_info_t<DECODER, IE>;
	sl::ie_decode<sl::decode_type_context<META_INFO>>(decoder, ie, deps...);
}

}	//end: namespace med
