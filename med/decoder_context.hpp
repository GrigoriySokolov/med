/**
@file
context for decoding

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once

#include "allocator.hpp"
#include "buffer.hpp"
#include "snapshot.hpp"
#include "error_context.hpp"

namespace med {

template <
		class BUFFER = buffer<uint8_t const*>,
		class ERR_CTX = error_context<octet_error_ctx_traits>,
		class ALLOCATOR = allocator<true, BUFFER, ERR_CTX>
		>
class decoder_context
{
public:
	using error_ctx_type = ERR_CTX;
	using allocator_type = ALLOCATOR;
	using buffer_type = BUFFER;
	using state_t = typename buffer_type::state_type;

private:
	//TODO: support during decode (possibly different with_snapshot types for encoding/decoding)
	struct snapshot_s
	{
		SNAPSHOT    snapshot;
		state_t     state;
		snapshot_s* next;
	};

public:
	decoder_context(void const* data, std::size_t size, void* alloc_data, std::size_t alloc_size)
		: m_allocator{ &m_errCtx }
	{
		reset(data, size);

		if (alloc_size)
		{
			m_allocator.reset(alloc_data, alloc_size);
		}
		else
		{
			m_allocator.reset();
		}
	}

	decoder_context()
		: decoder_context(nullptr, 0, nullptr, 0)
	{
	}

	decoder_context(void const* data, std::size_t size)
		: decoder_context(data, size, nullptr, 0)
	{
	}

	template <typename T, std::size_t SIZE>
	explicit decoder_context(T const (&data)[SIZE])
		: decoder_context(data, sizeof(data), nullptr, 0)
	{
	}

	template <typename T1, std::size_t SIZE, typename T2, std::size_t ALLOC_SIZE>
	decoder_context(T1 const (&data)[SIZE], T2 (&alloc_data)[ALLOC_SIZE])
		: decoder_context(data, sizeof(data), alloc_data, sizeof(alloc_data))
	{
	}


	buffer_type& buffer()                   { return m_buffer; }
	error_ctx_type& error_ctx()             { return m_errCtx; }
	explicit operator bool() const          { return static_cast<bool>(error_ctx()); }
	allocator_type& get_allocator()         { return m_allocator; }

	void reset(void const* data, std::size_t size)
	{
		get_allocator().reset();
		buffer().reset(static_cast<typename buffer_type::pointer>(data), size);
		error_ctx().reset();
	}
	template <typename T, std::size_t SIZE>
	void reset(T const (&data)[SIZE])       { reset(data, sizeof(data)); }

	void reset()
	{
		get_allocator().reset();
		buffer().reset();
		error_ctx().reset();
	}

private:
	decoder_context(decoder_context const&) = delete;
	decoder_context& operator=(decoder_context const&) = delete;

	error_ctx_type m_errCtx;
	buffer_type    m_buffer;
	allocator_type m_allocator;
};

} //namespace med
