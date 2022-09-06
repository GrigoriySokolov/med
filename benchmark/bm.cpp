#include <benchmark/benchmark.h>

//#pragma GCC diagnostic push
//#pragma GCC diagnostic pop
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"

#include "med.hpp"
#include "encode.hpp"
#include "decode.hpp"
#include "encoder_context.hpp"
#include "decoder_context.hpp"
#include "octet_encoder.hpp"
#include "octet_decoder.hpp"

namespace {

template <typename ...T> using M = med::mandatory<T...>;
template <typename ...T> using O = med::optional<T...>;
using L = med::length_t<med::value<uint8_t>>;
template <uint8_t TAG> using T = med::value<med::fixed<TAG, uint8_t>>;

struct FLD_UC : med::value<uint8_t>{};
struct FLD_U16 : med::value<uint16_t>{};
struct FLD_U24 : med::value<med::bytes<3>>{};
struct FLD_IP : med::value<uint32_t>{};
struct FLD_DW : med::value<uint32_t>{};
struct VFLD1 : med::ascii_string<med::min<5>, med::max<10>>{};

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
};

struct PROTO : med::choice<
	M< T<0x01>, MSG_SEQ >
>
{
};


void BM_encode_ok(benchmark::State& state)
{
	PROTO proto;
	uint8_t buffer[1024];
	med::encoder_context<> ctx{ buffer };

	MSG_SEQ& msg = proto.select();
	msg.ref<FLD_UC>().set(37);
	msg.ref<FLD_U16>().set(0x35D9);
	msg.ref<FLD_U24>().set(0xDABEEF);
	msg.ref<FLD_IP>().set(0xFee1ABBA);


	msg.ref<FLD_DW>().set(0x01020304);
	msg.ref<VFLD1>().set("test.this!");

	std::uint8_t dummy = 0;
	while (state.KeepRunning())
	{
		ctx.reset();
		dummy += buffer[0];
		msg.ref<FLD_UC>().set(dummy);
		encode(med::octet_encoder{ctx}, proto);
		benchmark::DoNotOptimize(dummy);
	}
}
BENCHMARK(BM_encode_ok);

void BM_encode_fail(benchmark::State& state)
{
	PROTO proto;
	uint8_t buffer[1024];
	med::encoder_context<> ctx{ buffer };

	MSG_SEQ& msg = proto.select();
	msg.ref<FLD_UC>().set(0);
	msg.ref<FLD_U24>().set(0);

	while (state.KeepRunning())
	{
		ctx.reset();
		try
		{
			encode(med::octet_encoder{ctx}, proto);
			std::abort();
		}
		catch (med::missing_ie const& ex)
		{
		}
	}
}
BENCHMARK(BM_encode_fail);

void BM_decode_ok(benchmark::State& state)
{
	PROTO proto;
	med::decoder_context<> ctx;

	uint8_t const encoded[] = { 1
		, 37
		, 0x21, 0x35, 0xD9
		, 3, 0xDA, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
		, 0x51, 0x01, 0x02, 0x03, 0x04
		, 0x12, 4, 't', 'e', 's', 't', '.', 't', 'h', 'i', 's', '!'
	};

	std::size_t dummy = 0;
	while (state.KeepRunning())
	{
		ctx.reset(encoded, sizeof(encoded));
		decode(med::octet_decoder{ctx}, proto);
		dummy += proto.get<MSG_SEQ>()->get<FLD_UC>().get();
		benchmark::DoNotOptimize(dummy);
	}

	//std::printf("dummy=%zu\n", dummy);
}
BENCHMARK(BM_decode_ok);

void BM_decode_fail(benchmark::State& state)
{
	PROTO proto;
	med::decoder_context<> ctx;

	//invalid length of variable field (longer)
	uint8_t const bad_var_len_hi[] = { 1
		, 37
		, 0x21, 0x35, 0xD9
		, 3, 0xDA, 0xBE, 0xEF
		, 0x42, 4, 0xFE, 0xE1, 0xAB, 0xBA
		, 0x12, 5, 't', 'e', 's', 't', 'e', 's', 't', 'e', 's', 't', 'e'
	};

	while (state.KeepRunning())
	{
		ctx.reset(bad_var_len_hi, sizeof(bad_var_len_hi));
		try
		{
			decode(med::octet_decoder{ctx}, proto);
			std::printf("SHOULD NOT REACH HERE!\n");
			std::abort();
		}
		catch (med::exception const& ex)
		{
		}
	}
}
BENCHMARK(BM_decode_fail);

} //end: namespace

BENCHMARK_MAIN();
