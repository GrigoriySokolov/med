#pragma once
#include "med.hpp"

template <typename... T>
using M = med::mandatory<T...>;
template <typename... T>
using O = med::optional<T...>;

using L = med::length_t<med::value<uint8_t>>;
using CNT = med::counter_t<med::value<uint16_t>>;
template <std::size_t TAG>
using T = med::value<med::fixed<TAG, uint8_t>>;
template <std::size_t TAG>
using T16 = med::value<med::fixed<TAG, uint16_t>>;
template <std::size_t TAG>
using C = med::value<med::fixed<TAG, uint8_t>>;

struct FLD_UC : med::value<uint8_t>
{
	static constexpr char const* name() { return "UC"; }
};
struct FLD_U8 : med::value<uint8_t>
{
	static constexpr char const* name() { return "U8"; }
};

struct FLD_U16 : med::value<uint16_t>
{
	static constexpr char const* name() { return "U16"; }
};
struct FLD_W : med::value<uint16_t>
{
	static constexpr char const* name() { return "Word"; }
};

struct FLD_U24 : med::value<med::bytes<3>>
{
	static constexpr char const* name() { return "U24"; }
};

struct FLD_IP : med::value<uint32_t>
{
	static constexpr char const* name() { return "IP-Address"; }
	template <std::size_t N>
	void print(char (&sz)[N]) const
	{
		uint32_t ip = get();
		std::snprintf(sz, sizeof(sz), "%u.%u.%u.%u", uint8_t(ip >> 24), uint8_t(ip >> 16), uint8_t(ip >> 8), uint8_t(ip));
	}
};

struct FLD_DW : med::value<uint32_t>
{
	static constexpr char const* name() { return "Double-Word"; }
};

//struct VFLD1 : med::octet_string<med::min<5>, med::max<10>, std::vector<uint8_t>>
//struct VFLD1 : med::octet_string<med::min<5>, med::max<10>, boost::container::static_vector<uint8_t, 10>>
struct VFLD1 : med::ascii_string<med::min<5>, med::max<10>>
{
	static constexpr char const* name() { return "url"; }
};

struct custom_length : med::value<uint8_t>
{
	std::size_t get_length() const noexcept
	{
		return 2 * (get_encoded() + 1);
	}

	bool set_length(std::size_t v)
	{
		if (0 == (v & 1)) //should be even
		{
			set_encoded((v - 2) / 2);
			return true;
		}
		return false;
	}
};
using CLEN = med::length_t<custom_length>;

struct MSG_SEQ : med::sequence<
	M< FLD_UC >,              //<V>
	M< T<0x21>, FLD_U16 >,    //<TV>
	M< L, FLD_U24 >,          //<LV>(fixed)
	M< T<0x42>, L, FLD_IP >, //<TLV>(fixed)
	O< T<0x51>, FLD_DW >,     //[TV]
	O< T<0x12>, CLEN, VFLD1 > //TLV(var)
>
{
	static constexpr char const* name() { return "Msg-Seq"; }
};

struct OOO_SEQ : med::sequence<
	M<T<1>, FLD_U8>,
	O<T<2>, FLD_U16>,
	M<T<3>, FLD_U24>,
	O<T<4>, FLD_DW>
>
{};

struct NO_THING : med::empty<>
{
	static constexpr char const* name() { return "Nothing"; }
};

struct FLD_CHO : med::choice<
	M<C<0x00>, FLD_U8>,
	M<C<0x02>, FLD_U16>,
	M<C<0x04>, FLD_IP>,
	M<C<0x06>, NO_THING>
>
{};

struct SEQOF_1 : med::sequence<
	M< FLD_W, med::max<3> >
>
{
	static constexpr char const* name() { return "Seq-Of-1"; }
};

struct SEQOF_2 : med::sequence<
	M< FLD_W >,
	O< T<0x06>, L, FLD_CHO >
>
{
	static constexpr char const* name() { return "Seq-Of-2"; }
};

template <int INSTANCE>
struct SEQOF_3 : med::sequence<
	M< FLD_U8 >,
	M< T<0x21>, FLD_U16 >  //<TV>
>
{
	static constexpr char const* name() { return "Seq-Of-3"; }
	template <std::size_t N>
	void print(char (&sz)[N]) const
	{
		std::snprintf(sz, N, "%u:%04X (%d)"
			, this->template get<FLD_U8>().get(), this->template get<FLD_U16>().get()
			, INSTANCE);
	}
};

//tagged nibble
struct FLD_TN : med::value<uint8_t>
{
	static constexpr value_type TAG = 0xE0;
	static constexpr bool match(value_type v)   { return TAG == (v & 0xF0); }

	enum : value_type
	{
		tag_mask = 0xF0,
		mask     = 0x0F,
	};

	value_type get() const                { return base_t::get() & mask; }
	void set(value_type v)                { set_encoded( TAG | (v & mask) ); }

	static constexpr char const* name()   { return "Tagged-Bits"; }
	template <std::size_t N>
	void print(char (&sz)[N]) const       { std::snprintf(sz, N, "%02X", get()); }
};


struct MSG_MSEQ : med::sequence<
	M< FLD_UC, med::arity<2>>,            //<V>*2
	M< T<0x21>, FLD_U16, med::arity<2>>,  //<TV>*2
	M< L, FLD_U24, med::arity<2>>,        //<LV>*2
	M< T<0x42>, L, FLD_IP, med::max<2>>, //<TLV>(fixed)*[1,2]
	M< T<0x51>, FLD_DW, med::max<2>>,     //<TV>*[1,2]
	M< CNT, SEQOF_3<0>, med::max<2>>,
	O< FLD_TN, FLD_TN >,
	O< T<0x60>, L, FLD_CHO>,
	O< T<0x61>, L, SEQOF_1>,
	O< T<0x62>, L, SEQOF_2, med::max<2>>,
	O< L, VFLD1, med::max<3> >           //[LV(var)]*[0,3] till EoF
>
{
	static constexpr char const* name() { return "Msg-Multi-Seq"; }
};


struct MSG_SET : med::set<
	M< T16<0x0b>,    FLD_UC >, //<TV>
	M< T16<0x21>, L, FLD_U16 >, //<TLV>
	O< T16<0x49>, L, FLD_U24 >, //[TLV]
	O< T16<0x89>,    FLD_IP >, //[TV]
	O< T16<0x22>, L, VFLD1 >   //[TLV(var)]
>
{
	static constexpr char const* name() { return "Msg-Set"; }
};

struct MSG_MSET : med::set<
	M< T16<0x0b>,    FLD_UC, med::arity<2> >, //<TV>*2
	M< T16<0x0c>,    FLD_U8, med::max<2> >, //<TV>*[1,2]
	M< T16<0x21>, L, FLD_U16, med::max<3> >, //<TLV>*[1,3]
	O< T16<0x49>, L, FLD_U24, med::arity<2> >, //[TLV]*2
	O< T16<0x89>,    FLD_IP, med::arity<2> >, //[TV]*2
	O< T16<0x22>, L, VFLD1, med::max<3> > //[TLV(var)]*[1,3]
>
{
	static constexpr char const* name() { return "Msg-Multi-Set"; }
};

struct FLD_QTY : med::value<uint8_t>
{
	struct counter
	{
		template <class T>
		std::size_t operator()(T const& ies) const
		{
			return ies.template as<FLD_QTY>().get();
		}
	};

	struct setter
	{
		template <class T>
		void operator()(FLD_QTY& num_ips, T const& ies) const
		{
			if (std::size_t const qty = med::field_count(ies.template as<FLD_IP>()))
			{
				CODEC_TRACE("*********** qty = %zu", qty);
				num_ips.set(qty);
			}
		}
	};
};

struct FLD_FLAGS : med::value<uint8_t>
{
	enum : value_type
	{
		//bits for field presence
		U16 = 1 << 0,
		U24 = 1 << 1,
		QTY = 1 << 2,

		//counters for multi-instance fields
		QTY_MASK = 0x03, // 2 bits to encode 0..3
		UC_QTY = 4,
		U8_QTY = 6,
	};

	template <value_type BITS>
	struct has_bits
	{
		template <class T> bool operator()(T const& ies) const
		{
			return ies.template as<FLD_FLAGS>().get() & BITS;
		}
	};

	template <value_type QTY, int DELTA = 0>
	struct counter
	{
		template <class T>
		std::size_t operator()(T const& ies) const
		{
			return std::size_t(((ies.template as<FLD_FLAGS>().get() >> QTY) & QTY_MASK) + DELTA);
		}
	};

	struct setter
	{
		template <class T>
		void operator()(FLD_FLAGS& flags, T const& ies) const
		{
			//hdr.template as<version_flags>
			value_type bits =
				(ies.template as<FLD_U16>().is_set() ? U16 : 0 ) |
				(ies.template as<FLD_U24>().is_set() ? U24 : 0 );

			auto const uc_qty = med::field_count(ies.template as<FLD_UC>());
			auto const u8_qty = med::field_count(ies.template as<FLD_U8>());

			CODEC_TRACE("*********** bits=%02X uc=%zu u8=%zu", bits, uc_qty, u8_qty);

			if (ies.template as<FLD_IP>().is_set())
			{
				bits |= QTY;
			}

			bits |= (uc_qty-1) << UC_QTY;
			bits |= u8_qty << U8_QTY;
			CODEC_TRACE("*********** setting bits = %02X", bits);

			flags.set(bits);
			//ies.template as<FLD_FLAGS>().set(bits);
		}
	};
};

//optional fields with functors
struct MSG_FUNC : med::sequence<
	M< FLD_FLAGS, FLD_FLAGS::setter >,
	M< FLD_UC, med::min<2>, med::max<4>, FLD_FLAGS::counter<FLD_FLAGS::UC_QTY, 1> >,
	O< FLD_QTY, FLD_QTY::setter, FLD_FLAGS::has_bits<FLD_FLAGS::QTY> >,
	O< FLD_U8, med::max<2>, FLD_FLAGS::counter<FLD_FLAGS::U8_QTY> >,
	O< FLD_U16, FLD_FLAGS::has_bits<FLD_FLAGS::U16> >,
	O< FLD_U24, FLD_FLAGS::has_bits<FLD_FLAGS::U24> >,
	O< FLD_IP, med::max<8>, FLD_QTY::counter >
>
{
	static constexpr char const* name() { return "Msg-With-Functors"; }
};


struct PROTO : med::choice<
	M<C<0x01>, MSG_SEQ>,
	M<C<0x11>, MSG_MSEQ>,
	M<C<0x04>, MSG_SET>,
	M<C<0x14>, MSG_MSET>,
	M<C<0xFF>, MSG_FUNC>
>
{
};
