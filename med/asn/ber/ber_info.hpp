#pragma once
#include <utility>

#include "value.hpp"
#include "field.hpp"
#include "length.hpp"
#include "ber_tag.hpp"

namespace med::asn::ber {

struct info
{
private:
	template <class T, class CONSTRUCTED = std::true_type>
	struct make_tag
	{
		template <class V>
		using tag_of = value< fixed< V::value, bytes<V::num_bytes> > >;
		using type = mi< mik::TAG, tag_of<tag_value<T, CONSTRUCTED::value>> >;
	};

	template <std::size_t I, std::size_t N>
	struct not_last
	{
		using type = std::bool_constant<I < (N-1)>;
	};


public:
	template <class IE>
	static constexpr auto produce_meta_info()
	{
		using asn_traits = get_meta_info_t<IE>;
/*
8.14 Encoding of a value of a prefixed type
8.14.2 Encoding of tagged value is derived from complete encoding of corresponding value of the type
appearing in "TaggedType" notation (called the base encoding) as specified in 8.14.3 and 8.14.4.
8.14.3 If implicit tagging (see X.680, 31.2.7) was not used, then:
the encoding is constructed and contents octets is the complete base encoding.
8.14.4 If implicit tagging was used, then:
a) the encoding is constructed if the base encoding is constructed, and primitive otherwise;
b) the contents octets shall be the same as the contents octets of the base encoding.
*/
		if constexpr (meta::list_is_empty_v<asn_traits>)
		{
			return meta::wrap<asn_traits>{};
		}
		else
		{
			constexpr auto get_tags = []
			{
				/* Rec. ITU-T X.690 (08/2015)
				8.9.1 The encoding of a sequence value shall be constructed.
				8.10.1 The encoding of a sequence-of value shall be constructed.
				8.11.1 The encoding of a set value shall be constructed.
				8.12.1 The encoding of a set-of value shall be constructed.
				*/
				constexpr bool is_constructed =
						std::is_base_of_v<CONTAINER, typename IE::ie_type> //sequence, set
						|| is_seqof_v<IE>; //sequence-of, set-of
				if constexpr (is_constructed)
				{
					return meta::wrap<meta::transform_t<asn_traits, make_tag>>{};
				}
				else
				{
					return meta::wrap<meta::transform_indexed_t<asn_traits, make_tag, not_last>>{};
				}
			};

			//!TODO: LENSIZE need to calc len to known its size (only 1 byte for now)
			using len_t = mi<mik::LEN, length_t<value<uint8_t>>>;
			using meta_info = meta::interleave_t< meta::unwrap_t<decltype(get_tags())>, len_t>;
			return meta::wrap<meta_info>{};
		}
	}
};

} //end: namespace med::asn::ber

