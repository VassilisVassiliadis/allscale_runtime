
#ifndef ALLSCALE_DATA_ITEM_MANAGER_DATA_ITEM_HPP
#define ALLSCALE_DATA_ITEM_MANAGER_DATA_ITEM_HPP

#include <allscale/data_item_manager/location_info.hpp>
#include <allscale/util/readers_writers_mutex.hpp>
#include <hpx/lcos/local/spinlock.hpp>
#include <hpx/runtime/naming/id_type.hpp>

#include <memory>
#include <unordered_map>

namespace allscale { namespace data_item_manager {
    // The data_item represents a fragment of the globally distributed data item
    // It knows about ownership of its own local region, and the ones of it's
    // parents and children for faster lookup. The lookup is organized in a
    // binary tree fashion.
    // For faster lookup times, frequently accessed fragments are cached.
    template <typename DataItemType>
    struct data_item
    {
        using data_item_type = DataItemType;
        using mutex_type = allscale::util::readers_writers_mutex;
        using region_type = typename DataItemType::region_type;
        using fragment_type = typename DataItemType::fragment_type;
        using shared_data_type = typename DataItemType::shared_data_type;

        ~data_item()
        {
//             std::cerr << this << " Locate access count: " << locate_access << '\n';
//             std::cerr << this << " Cache lookup time: " << cache_lookup_time << '\n';
//             std::cerr << this << " Cache misses: " << cache_miss << '\n';
//             std::cerr << this << " Cache miss ratio: " << double(cache_miss)/double(locate_access) * 100. << "%\n";
//             std::cerr << this << " Fragment extract time: " << extract_time << '\n';
//             std::cerr << this << " Fragment insert time: " << insert_time << '\n';
//             std::cerr << this << " Intersect time: " << intersect_time << '\n';
//             std::cerr << this << " Merge time: " << merge_time << '\n';
//             std::cerr << this << " Difference time: " << difference_time << '\n';
//             std::cerr << '\n';
        }

        std::size_t locate_access = 0;
        std::size_t cache_miss = 0;
        double cache_lookup_time = 0.0;
        double intersect_time = 0.0;
        double merge_time = 0.0;
        double difference_time = 0.0;
        double extract_time = 0.0;
        double insert_time = 0.0;

        // The mutex which protects this data item from concurrent accesses
        mutex_type region_mtx;
        mutex_type fragment_mtx;

        std::unique_ptr<fragment_type> fragment;
        std::unique_ptr<shared_data_type> shared_data;

        // A simple location info data structure will serve as a cache to
        // accelerate lookups.
        location_info<region_type> location_cache;

        // The child regions mark the regions occupied by the children spawend
        // from this locality.
        std::unordered_map<std::size_t, region_type> child_regions;
        // The owned region is what is allocated and owned by this locality
        region_type owned_region;
    };
}}

#endif
