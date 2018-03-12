#ifndef ALLSCALE_READERS_WRITERS_MUTEX_HPP
#define ALLSCALE_READERS_WRITERS_MUTEX_HPP

#include <hpx/config.hpp>
#include <hpx/util/yield_while.hpp>

#include <atomic>

namespace allscale {
namespace util {

    class readers_writers_mutex
    {
    private:
        static constexpr std::int64_t writer_mark_ = -1;
        std::atomic<std::int64_t> readers_count_;

    public:
        readers_writers_mutex() : readers_count_(0) {}

        // acquire lock for a unique writer
        void lock()
        {
            hpx::util::yield_while(
                [this]()
                {
                    return !try_lock();
                },
                "readers_writers_mutex::lock"
            );
        }

        // try to obtain unique writer lock
        bool try_lock()
        {
            std::int64_t readers = 0;
            bool res = readers_count_.compare_exchange_weak(readers, writer_mark_,
                std::memory_order_acq_rel);
#if !defined(NDEBUG)
            if (res)
            {
                HPX_ASSERT(readers == 0);
            }
            else
            {
                HPX_ASSERT(readers >= 0 || readers == writer_mark_);
            }
#endif
            return res;
        }

        // unlock writer
        void unlock()
        {
#if !defined(NDEBUG)
            int64_t prev = readers_count_.exchange(0, std::memory_order_acq_rel);
            HPX_ASSERT(prev == writer_mark_);
#else
            readers_count_.store(0, std::memory_order_release);
#endif
        }

        // obtain a reader lock, many readers may have the lock simultaneously
        void lock_shared()
        {
            hpx::util::yield_while(
                [this]()
                {
                    return !try_lock_shared();
                },
                "readers_writers_mutex::lock_shared"
            );
        }

        // try to obtain a reader lock
        bool try_lock_shared()
        {
            std::int64_t readers = readers_count_.load(std::memory_order_release);
            if (readers != writer_mark_)
            {
                bool res = readers_count_.compare_exchange_weak(readers, readers + 1,
                    std::memory_order_acq_rel);
#if !defined(NDEBUG)
                if (res)
                {
                    HPX_ASSERT(readers >= 0);
                    HPX_ASSERT(readers != writer_mark_);
                }
#endif
                return res;
            }
            return false;
        }

        // unlock one reader
        void unlock_shared()
        {
            hpx::util::yield_while(
                [this]()
                {
                    std::int64_t readers = readers_count_.load(std::memory_order_release);
                    bool res = readers_count_.compare_exchange_weak(readers, readers - 1,
                        std::memory_order_acq_rel);
#if !defined(NDEBUG)
                    if (res)
                    {
                        HPX_ASSERT(readers > 0);
                    }
#endif
                    return !res;
                },
                "readers_writers_mutex::unlock_shared()"
            );
        }

        // return true if a reader or writer has the lock
        bool owns_lock()
        {
            std::int64_t readers = readers_count_.load(std::memory_order_release);
            return readers > 0 || readers == writer_mark_;
        }
    };
}
}

#endif
