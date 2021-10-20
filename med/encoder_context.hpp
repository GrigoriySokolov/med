/**
@file
context for encoding

@copyright Denis Priyomov 2016-2017
Distributed under the MIT License
(See accompanying file LICENSE or visit https://github.com/cppden/med)
*/

#pragma once


#include "allocator.hpp"
#include "snapshot.hpp"
#include "buffer.hpp"
#include "debug.hpp"

namespace med {

template <
		class BUFFER = buffer<uint8_t*>,
		class ALLOCATOR = allocator<false, BUFFER>
		>
class encoder_context
{
public:
	using allocator_type = ALLOCATOR;
	using buffer_type = BUFFER;
	using state_t = typename buffer_type::state_type;

private:
	struct snapshot_s
	{
		SNAPSHOT    snapshot;
		state_t     state;
		snapshot_s* next;
	};

public:
	encoder_context(void* data, std::size_t size)
		: m_buffer{ }
		, m_allocator{ m_buffer }
	{
		reset(data, size);
	}

	template <typename T, std::size_t SIZE>
	explicit encoder_context(T (&buf)[SIZE])
		: encoder_context(buf, sizeof(buf))
	{
	}

	encoder_context(encoder_context const&) = delete;
	encoder_context& operator=(encoder_context const&) = delete;

	buffer_type& buffer()                   { return m_buffer; }
	allocator_type& get_allocator()         { return m_allocator; }

	template <typename T, std::size_t SIZE>
	void reset(T (&data)[SIZE]) noexcept   { reset(data, SIZE * sizeof(T)); }

	void reset(void* data, std::size_t size) noexcept
	{
		get_allocator().reset(static_cast<uint8_t*>(data), size);
		m_buffer.reset(static_cast<typename buffer_type::pointer>(data), size);
		m_snapshot = nullptr;
	}

	void reset()
	{
		get_allocator().reset();
		m_buffer.reset();
		m_snapshot = nullptr;
	}

	/**
	 * Stores the buffer snapshot
	 * @param snap
	 */
	void snapshot(SNAPSHOT snap)
	{
		CODEC_TRACE("snapshot %p{%zu}", static_cast<void const*>(snap.id), snap.size);
		snapshot_s* p = get_allocator().template allocate<snapshot_s>();
		p->snapshot = snap;
		p->state = m_buffer.get_state();

		p->next = m_snapshot ? m_snapshot->next : nullptr;
		m_snapshot = p;
	}

	class snap_s : public state_t
	{
	public:
		bool validate_length(std::size_t s) const
		{
			CODEC_TRACE("%s(%zu ? %zu)=%d", __FUNCTION__, m_length, s, m_length == s);
			return m_length == s;
		}

	private:
		friend class encoder_context;
		snap_s(state_t const& st, std::size_t len) : state_t{st}, m_length{len} {}
		snap_s() : state_t{}, m_length{} {}

		std::size_t m_length;
	};

	/**
	 * Finds snapshot of the buffer state for the IE
	 * @details Used in encoder to set the buffer state before updating
	 * @param IE to retrieve its snapshot
	 * @return snapshot or empty snapshot if not found
	 */
	template <class IE>
	snap_s snapshot(IE const&) const
	{
		static_assert(std::is_base_of<with_snapshot, IE>(), "IE WITH med::with_snapshot EXPECTED");

		for (snapshot_s const* p = m_snapshot; p != nullptr; p = p->next)
		{
			if (p->snapshot.id == snapshot_id<IE>) { return snap_s{p->state, p->snapshot.size}; }
		}
		return snap_s{};
	}

private:
	using const_iterator = snapshot_s const*;

	buffer_type    m_buffer;
	allocator_type m_allocator;

	snapshot_s*    m_snapshot{ nullptr };
};

} //namespace med
