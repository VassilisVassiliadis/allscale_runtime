
#ifndef ALLSCALE_TASK_ID_HPP
#define ALLSCALE_TASK_ID_HPP

#include <allscale/api/core/impl/reference/task_id.h>
#include <allscale/utils/serializer.h>

#include <hpx/runtime/serialization/serialize.hpp>
#include <hpx/traits/is_bitwise_serializable.hpp>

#include <memory>

namespace allscale {
    struct profile;
    struct task_id
    {
        using task_path = allscale::api::core::impl::reference::TaskPath;
        std::uint64_t locality_id;
        std::uint64_t id;
        task_path path;
        mutable std::uint8_t numa_depth = 0;
        mutable std::uint8_t numa_node_ = std::uint8_t(-1);

        // This is used for monitoring...
        std::shared_ptr<allscale::profile> profile;

        std::size_t numa_node(std::size_t numa_d, std::size_t cut_off) const
        {
            if (depth() < cut_off) return 0;

            if (numa_node_ == std::uint8_t(-1))
            {
                numa_depth = numa_d;
                numa_node_ = 0;
            }
            else
            {
                if (numa_depth > 0)
                {
                    --numa_depth;
                    numa_node_ = numa_node_ * 2 + (path.getPath() & 0x1);
                }
            }

            return numa_node_;
        }

        template <typename Archive>
        void serialize(Archive& ar, unsigned)
        {
            ar & locality_id;
            ar & id;
            ar & path;
        }

        task_id parent() const;

        bool is_root() const
        {
            return path.isRoot();
        }

        bool is_left() const
        {
            return (path.getPath() & 1) == 0;
        }

        bool is_right() const
        {
            return (path.getPath() & 1) == 1;
        }

        task_id left_child() const
        {
            return {locality_id, id, path.getLeftChildPath(), numa_depth, numa_node_, nullptr};
        }

        task_id right_child() const
        {
            return {locality_id, id, path.getRightChildPath(), numa_depth, numa_node_, nullptr};
        }

        std::size_t depth() const
        {
            return path.getLength();
        }

        bool is_parent_of(task_id const& id) const
        {
            return this->id == id.id && locality_id == id.locality_id && path.isPrefixOf(id.path);
        }

        friend bool operator==(task_id const& lhs, task_id const& rhs)
        {
            return
                std::tie(lhs.locality_id, lhs.id, lhs.path) == std::tie(rhs.locality_id, rhs.id, rhs.path);
        }

        friend bool operator!=(task_id const& lhs, task_id const& rhs)
        {
            return
                std::tie(lhs.locality_id, lhs.id, lhs.path) != std::tie(rhs.locality_id, rhs.id, rhs.path);
        }

        friend bool operator<(task_id const& lhs, task_id const& rhs)
        {
            return
                std::tie(lhs.locality_id, lhs.id, lhs.path) < std::tie(rhs.locality_id, rhs.id, rhs.path);
        }

        friend bool operator>(task_id const& lhs, task_id const& rhs)
        {
            return
                std::tie(lhs.locality_id, lhs.id, lhs.path) > std::tie(rhs.locality_id, rhs.id, rhs.path);
        }

        friend bool operator>=(task_id const& lhs, task_id const& rhs)
        {
            return
                std::tie(lhs.locality_id, lhs.id, lhs.path) >= std::tie(rhs.locality_id, rhs.id, rhs.path);
        }

        friend bool operator<=(task_id const& lhs, task_id const& rhs)
        {
            return
                std::tie(lhs.locality_id, lhs.id, lhs.path) <= std::tie(rhs.locality_id, rhs.id, rhs.path);
        }

        friend std::ostream& operator<<(std::ostream& os, task_id const& id)
        {
            return os << "T-" << id.locality_id << "." << id.id << id.path << '(' << id.path.getPath() << ',' << +id.path.getLength() << ')';
        }

        friend std::string to_string(task_id const& id);

        static HPX_EXPORT task_id create_root();
        static HPX_EXPORT task_id create_child();

    };
}

HPX_IS_BITWISE_SERIALIZABLE(allscale::task_id)

#endif
