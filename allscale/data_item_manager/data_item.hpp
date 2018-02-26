
#ifndef ALLSCALE_DATA_ITEM_MANAGER_DATA_ITEM_HPP
#define ALLSCALE_DATA_ITEM_MANAGER_DATA_ITEM_HPP

#include <allscale/data_item_manager/location_info.hpp>

#include <hpx/lcos/local/spinlock.hpp>
#include <hpx/runtime/naming/id_type.hpp>

#include <memory>

namespace allscale { namespace data_item_manager {
    // The data_item represents a fragment of the globally distributed data item
    // It knows about ownership of its own local region, and the ones of it's
    // parents and children for faster lookup. The lookup is organized in a
    // binary tree fashion.
    // For faster lookup times, frequently accessed fragments are cached.
    template <typename DataItemType>
    struct data_item
    {
        using mutex_type = hpx::lcos::local::spinlock;
        using region_type = typename DataItemType::region_type;
        using fragment_type = typename DataItemType::fragment_type;

        // The mutex which protects this data item from concurrent accesses
        mutex_type mtx;

        std::unique_ptr<fragment_type> fragment;

        // A simple location info data structure will serve as a cache to
        // accelerate lookups.
        location_info<region_type> location_cache;

        // The parent region marks the region from the parent.
        // It is the union of left, right and owned.
        region_type parent_region;
        // The left region is what is owned by the left child or any of its
        // descendants
        region_type left_region;
        // The right region is what is owned by the right child or any of its
        // descendants
        region_type right_region;
        // The owned region is what is allocated and owned by this locality
        region_type owned_region;
    };
}}

#endif
