#pragma once

// Course framework code; together with `World` the only direct use of clean-core in the game.
#include <clean-core/alloc_array.hh>
#include <clean-core/atomic_linked_pool.hh>
#include <clean-core/storage.hh>
#include <clean-core/utility.hh>

namespace gamedev
{
/**
 * Fixed-capacity pool with stable handles and densely packed values.
 *
 *  - Acquire, release and access are O(1).
 *  - Values are contiguous (`get_span()`); access via a handle goes through one extra indirection (handle -> slot index).
 *  - Releasing a value moves the last value into the freed slot, so references and pointers to values are invalidated by every `release`.
 *    Keep handles instead.
 */
template <class T>
struct compact_pool
{
public:
    using handle_t = cc::atomic_linked_pool<size_t>::handle_t; ///< Stable handle of a value.

    compact_pool() = default;

    /**
     * Creates a pool and allocates its storage (see `initialize`).
     *
     * @param size Capacity; 0 leaves the pool uninitialized.
     * @param alloc Allocator for the storage.
     */
    explicit compact_pool(size_t size, cc::allocator* alloc = cc::system_allocator) { initialize(size, alloc); }

    compact_pool(compact_pool&&) = delete;
    compact_pool(compact_pool const&) = delete;
    compact_pool& operator=(compact_pool&&) = delete;
    compact_pool& operator=(compact_pool const&) = delete;

    ~compact_pool() { _destroy(); }

    /**
     * Allocates storage for a fixed number of values. Asserts if already initialized.
     *
     * @param size Capacity; 0 does nothing.
     * @param alloc Allocator for the storage.
     */
    void initialize(size_t size, cc::allocator* alloc = cc::system_allocator)
    {
        if (size == 0)
            return;

        CC_ASSERT(_compact_values.size() == 0 && "double init");
        _sparse_indices.initialize(size, alloc);
        _compact_values = _compact_values.uninitialized(size, alloc);
        _compact_pool_handles = _compact_pool_handles.uninitialized(size, alloc);
    }

    /// Destroys all values and frees the storage; the pool can be initialized again.
    void destroy() { _destroy(); }

    /**
     * Default-constructs a new value at the end of the packed range. Asserts if full.
     *
     * @return Handle of the new value.
     */
    [[nodiscard]] handle_t acquire()
    {
        CC_ASSERT(!is_full() && "overcommited");
        // acquire
        auto const pool_handle = _sparse_indices.acquire();
        auto const compact_idx = _compact_head;
        ++_compact_head;

        // write cross-reference indices
        _sparse_indices.get(pool_handle) = compact_idx;
        _compact_pool_handles[compact_idx] = pool_handle;

        // ctor
        if constexpr (!std::is_trivially_constructible_v<T>)
            new (cc::placement_new, &_compact_values[compact_idx].value) T();
        return pool_handle;
    }

    /**
     * Destroys a value and moves the last value into its slot.
     *
     * @param handle Handle of a live value; it is invalid afterwards.
     * @return True if a value was moved (from index `size()` after the call to the released index), false if the released value was the last one.
     */
    bool release(handle_t handle)
    {
        CC_ASSERT(!is_empty() && "already empty");

        // release pool
        size_t const compact_idx = _sparse_indices.get(handle);
        _sparse_indices.release(handle);

        // dtor
        if constexpr (!std::is_trivially_destructible_v<T>)
            _compact_values[compact_idx].value.~T();

        --_compact_head;

        if (compact_idx == _compact_head)
        {
            // this was the last element, done
            return false;
        }

        // move head to this position (move-construct: the value in this slot was destroyed above)
        new (cc::placement_new, &_compact_values[compact_idx].value) T(cc::move(_compact_values[_compact_head].value));
        _compact_pool_handles[compact_idx] = _compact_pool_handles[_compact_head];

        // update pool entry
        uint32_t const modified_pool_handle = _compact_pool_handles[compact_idx];
        _sparse_indices.get(modified_pool_handle) = compact_idx;

        return true;
    }

    /**
     * Destroys all values and invalidates all handles; the storage is kept.
     *
     * @return Number of released handles.
     */
    unsigned release_all()
    {
        if constexpr (!std::is_trivially_destructible_v<T>)
            for (auto& val : get_span())
            {
                val.~T();
            }

        _compact_head = 0;
        return _sparse_indices.release_all();
    }

    /// Returns the value of a live `handle`. Do not keep the reference: it is invalidated by the next `release`.
    CC_FORCE_INLINE T& get(handle_t handle) { return _compact_values[_sparse_indices.get(handle)].value; }

    /// Const version of `get`.
    CC_FORCE_INLINE T const& get(handle_t handle) const { return _compact_values[_sparse_indices.get(handle)].value; }

    /// Pointer to the first of `size()` packed values.
    CC_FORCE_INLINE T* data() { return reinterpret_cast<T*>(_compact_values.data()); }

    /// Const version of `data`.
    CC_FORCE_INLINE T const* data() const { return reinterpret_cast<T const*>(_compact_values.data()); }

    /// Number of live values.
    size_t size() const { return _compact_head; }

    /// Size of the live values in bytes.
    size_t size_bytes() const { return size() * sizeof(T); }

    /// Capacity given to `initialize`.
    size_t max_size() const { return _compact_values.size(); }

    /// True if no more values can be acquired.
    bool is_full() const { return size() == max_size(); }

    /// True if there are no live values.
    bool is_empty() const { return _compact_head == 0; }

    /// True if `handle` is valid and refers to a live value.
    bool is_alive(handle_t handle) const { return handle != unsigned(-1) && _sparse_indices.is_alive(handle); }

    /// All live values, for iteration: `for (auto const& val : pool.get_span()) { ... }`
    CC_FORCE_INLINE cc::span<T> get_span() { return {data(), size()}; }

    /// Const version of `get_span`.
    CC_FORCE_INLINE cc::span<T const> get_span() const { return {data(), size()}; }

    /// Current packed index of a live `handle`'s value.
    CC_FORCE_INLINE size_t get_handle_index(handle_t handle) const { return _sparse_indices.get(handle); }

    /// Same as `size()`.
    CC_FORCE_INLINE size_t get_index_head() const { return _compact_head; }

    /// Handle of the value at packed `index`.
    CC_FORCE_INLINE handle_t get_nth_handle(unsigned index) const { return _compact_pool_handles[index]; }

private:
    /// Destroys all live values and frees the storage (no-op if never initialized).
    void _destroy()
    {
        if (max_size() == 0)
            return;

        if constexpr (!std::is_trivially_destructible_v<T>)
            for (auto& val : get_span())
            {
                val.~T();
            }

        _sparse_indices.destroy();
        _compact_values = {};
        _compact_pool_handles = {};
        _compact_head = 0;
    }


private:
    cc::atomic_linked_pool<size_t, true> _sparse_indices; ///< Handle -> packed index.
    cc::alloc_array<cc::storage_for<T>> _compact_values;  ///< Packed values; [0, _compact_head) are live.
    cc::alloc_array<handle_t> _compact_pool_handles;      ///< Packed index -> handle.
    size_t _compact_head = 0;                             ///< Number of live values.
};

}
