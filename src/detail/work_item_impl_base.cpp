
#include <allscale/detail/work_item_impl_base.hpp>

namespace allscale { namespace detail {
    void work_item_impl_base::set_this_id(machine_config const& mconfig)
    {
        id_.set(this, mconfig);
    }

    this_work_item::id const& work_item_impl_base::id() const
    {
        return id_;
    }
}}
