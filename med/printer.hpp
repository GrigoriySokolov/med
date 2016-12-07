/*!
@file
TODO: define.

@copyright Denis Priyomov 2016
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
*/

#pragma once


#include "encode.hpp"
#include "error.hpp"
#include "name.hpp"

/*
struct SinkSample
{
	template <typename T>
	void on_value(std::size_t depth, char const* name, T const& value)
	{
		printf("%*c%s=%u (%Xh)\n", depth, ' ', name, value, value);
	}

	void on_custom(std::size_t depth, char const* name, std::string const& s)
	{
		printf("%*c%s=%s\n", depth, ' ', name, s.c_str());
	}
};
*/

namespace med {

//MAX_LINE - max length of a single output of IE.print
template <class SINK, std::size_t MAX_LINE>
class printer
{
	template <int N>
	using int_t = std::integral_constant<int, N>;

	template <typename T>
	struct has_print
	{
		//check for: void IE::print(char (&sz)[N])
		template <class C, class P>
		static auto test(P* p) -> decltype(std::declval<C>().print(*p), int_t<2>{});

		//check for: some_type IE::print()
		template <class C, class P>
		static std::enable_if_t<!std::is_same<void, decltype(std::declval<C>().print())>::value, int_t<1>>
		test(P* p);

		template <class C, class P>
		static int_t<0> test(...);

		using type = decltype(test<T, char[MAX_LINE]>(nullptr));
	};

public:

	struct null_encoder
	{
		template <class IE>
		constexpr std::enable_if_t<!has_name_v<IE>, bool>
		operator()(printer&, IE const&)
		{
			return true;
		}

		template <class IE>
		std::enable_if_t<has_name_v<IE>, bool>
		operator()(printer& me, IE const& ie)
		{
			me.print_named(ie);
			return true;
		}
	};

	class container_encoder
	{
	public:
		//treat un-named container as logical group -> depth not changing
		template <class IE>
		std::enable_if_t<!has_name_v<IE>, bool>
		operator()(printer& me, IE const& ie)
		{
			return ie.encode(me);
		}

		template <class IE>
		std::enable_if_t<has_name_v<IE>, bool>
		operator()(printer& me, IE const& ie)
		{
			return print_named(me, ie, typename has_print<IE>::type{});
		}

	private:
		//customized prints
		template <class IE>
		bool print_named(printer& me, IE const& ie, int_t<2> pt)
		{
			me.print_named(ie, pt);
			return true;
		}
		template <class IE>
		bool print_named(printer& me, IE const& ie, int_t<1> pt)
		{
			me.print_named(ie, pt);
			return true;
		}

		//no customized print, change depth level
		template <class IE>
		bool print_named(printer& me, IE const& ie, int_t<0> pt)
		{
			me.m_sink.on_container(me.m_depth, name<IE>());
			auto const depth = me.m_depth++;
			CODEC_TRACE("depth -> %zu < max=%zu", me.m_depth, me.m_max_depth);
			bool const ret = (0 == me.m_max_depth || me.m_max_depth > me.m_depth) ? ie.encode(me) : true;
			me.m_depth = depth;
			CODEC_TRACE("depth <- %zu", depth);
			return ret;
		}
	};


	printer(SINK&& sink, std::size_t max_depth) noexcept
		: m_sink{ std::forward<SINK>(sink) }
		, m_max_depth{ max_depth }
	{
	}

	//unnamed primitive
	template <class IE, class IE_TYPE>
	constexpr std::enable_if_t<!has_name_v<IE>, bool>
	operator() (IE const& ie, IE_TYPE const&) { return true; }

	//named primitive
	template <class IE, class IE_TYPE>
	constexpr std::enable_if_t<has_name_v<IE>, bool>
	operator() (IE const& ie, IE_TYPE const&)
	{
		print_named(ie, typename has_print<IE>::type{});
		return true;
	}

	//state
	constexpr void operator() (med::SNAPSHOT const&) { }
	//errors
	constexpr void operator() (error e, char const*, std::size_t, std::size_t = 0, std::size_t = 0) { }
	//length encoder
	template <int DELTA> constexpr bool operator() (placeholder::_length<DELTA> const&) { return true; }

private:
	friend class container_encoder;
	friend struct null_encoder;

	//customized print 2
	template <class IE>
	void print_named(IE const& ie, int_t<2>)
	{
		char sz[MAX_LINE];
		ie.print(sz);
		m_sink.on_custom(m_depth, name<IE>(), sz);
	}

	//customized print 1
	template <class IE>
	void print_named(IE const& ie, int_t<1>)
	{
		m_sink.on_custom(m_depth, name<IE>(), ie.print());
	}

	//regular print
	template <class IE>
	void print_named(IE const& ie, int_t<0>)
	{
		m_sink.on_value(m_depth, IE::name(), ie.get());
	}

	SINK        m_sink;
	std::size_t m_depth {0};
	std::size_t const m_max_depth;
};

//print named IEs only up to given depth (not including, i.e. < max_depth)
//or full depth when max_depth=0
template <class SINK, class IE, std::size_t MAX_LINE = 128>
void print(SINK&& sink, IE const& ie, std::size_t max_depth = 0)
{
	encode(printer<SINK, MAX_LINE>{std::forward<SINK>(sink), max_depth}, ie);
}



template <class SINK, std::size_t MAX_LINE>
class dumper
{
public:

	struct null_encoder
	{
		template <class IE>
		bool operator()(dumper& me, IE const& ie)
		{
			me.dump(ie);
			return true;
		}
	};

	struct container_encoder
	{
		template <class IE>
		bool operator()(dumper& me, IE const& ie)
		{
			me.m_sink.on_container(me.m_depth, name<IE>());
			auto const depth = me.m_depth++;
			bool const ret = ie.encode(me);
			me.m_depth = depth;
			return ret;
		}
	};

	explicit dumper(SINK&& sink) noexcept
		: m_sink{ std::forward<SINK>(sink) }
	{
	}

	//primitives
	template <class IE, class IE_TYPE>
	bool operator() (IE const& ie, IE_TYPE const&)
	{
		dump(ie);
		return true;
	}

	//state
	constexpr void operator() (med::SNAPSHOT const&) { }
	//errors
	constexpr void operator() (error e, char const*, std::size_t, std::size_t = 0, std::size_t = 0) { }
	//length encoder
	template <int DELTA> constexpr bool operator() (placeholder::_length<DELTA> const&) { return true; }

	template <class IE>
	void dump(IE const& ie)
	{
		m_sink.on_value(m_depth, name<IE>(), ie.get());
	}

private:
	friend struct container_encoder;

	SINK        m_sink;
	std::size_t m_depth {0};
};

//print all (named and not) IEs in full depth
template <class SINK, class IE, std::size_t MAX_LINE = 128>
void print_all(SINK&& sink, IE const& ie)
{
	encode(dumper<SINK, MAX_LINE>{std::forward<SINK>(sink)}, ie);
}

} //namespace med
