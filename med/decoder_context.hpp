/*!
@file
TODO: define.

@copyright Denis Priyomov 2016
Distributed under the MIT License
(See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT)
*/

#pragma once

#include "allocator.hpp"
#include "buffer.hpp"
#include "error_context.hpp"

namespace med {

template < class BUFFER = buffer<uint8_t const*>, class ALLOCATOR = allocator >
class decoder_context
{
public:
	using allocator_type = ALLOCATOR;
	using buffer_type = BUFFER;

	decoder_context(void const* data, std::size_t size, void* alloc_data, std::size_t alloc_size)
		: m_allocator{ m_errCtx }
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

	template <std::size_t SIZE>
	explicit decoder_context(uint8_t const (&data)[SIZE])
		: decoder_context(data, SIZE, nullptr, 0)
	{
	}

	template <std::size_t SIZE, typename T, std::size_t ALLOC_SIZE>
	decoder_context(uint8_t const (&data)[SIZE], T const (&alloc_data)[ALLOC_SIZE])
		: decoder_context(data, SIZE, alloc_data, ALLOC_SIZE*sizeof(T))
	{
	}


	buffer_type& buffer()                   { return m_buffer; }
	error_context& error_ctx()              { return m_errCtx; }
	explicit operator bool() const          { return static_cast<bool>(error_ctx()); }
	allocator_type& get_allocator()         { return m_allocator; }

	void reset(void const* data, std::size_t size)
	{
		m_allocator.reset();
		buffer().reset(static_cast<typename buffer_type::pointer>(data), size);
		error_ctx().reset();
	}

	void reset()
	{
		m_allocator.reset();
		buffer().reset();
		error_ctx().reset();
	}

private:
	decoder_context(decoder_context const&) = delete;
	decoder_context& operator=(decoder_context const&) = delete;

	error_context  m_errCtx;
	buffer_type    m_buffer;
	allocator_type m_allocator;
};

} //namespace med
