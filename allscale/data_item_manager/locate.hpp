
#ifndef ALLSCALE_DATA_ITEM_MANAGER_LOCATE_HPP
#define ALLSCALE_DATA_ITEM_MANAGER_LOCATE_HPP

#include <allscale/data_item_manager/data_item_store.hpp>
#include <allscale/data_item_manager/location_info.hpp>

#include <hpx/include/actions.hpp>
#include <hpx/runtime/naming/id_type.hpp>
#include <hpx/util/tuple.hpp>

#include <vector>

namespace allscale { namespace data_item_manager {
    namespace detail
    {
        enum class locate_state
        {
            init,
            up,
            down
        };

        template <typename Requirement, typename Region>
        void register_child(hpx::naming::gid_type ref, Region region, std::size_t child_rank);

        template <typename Requirement, typename Region>
        struct register_child_action
          : hpx::actions::make_action<
                decltype(&register_child<Requirement, Region>),
                &register_child<Requirement, Region>,
                register_child_action<Requirement, Region>>::type
        {};

        template <typename Requirement, typename Region>
        void register_child(hpx::naming::gid_type ref, Region region, std::size_t child_rank)
        {
            using region_type = typename Requirement::region_type;
            using location_info_type = location_info<region_type>;
            using data_item_type = typename Requirement::data_item_type;
            using fragment_type = typename data_item_store<data_item_type>::data_item_type::fragment_type;
            using mutex_type = typename data_item_store<data_item_type>::data_item_type::mutex_type;

            std::size_t this_id = hpx::get_locality_id();
            auto& item = data_item_store<data_item_type>::lookup(ref);
            std::unique_lock<mutex_type> l(item.mtx);

            // Check if left child
            if (this_id * 2 + 1 == child_rank)
            {
                item.left_region = region_type::merge(item.left_region, region);
            }

            // Check if right child
            if (this_id * 2 + 2 == child_rank)
            {
                item.right_region = region_type::merge(item.right_region, region);
            }

            l.unlock();

            if (this_id == 0) return;

            // FIXME: futurize, make resilient
            hpx::id_type target(
                hpx::naming::get_id_from_locality_id(
                    (this_id-1)/2
                )
            );
            register_child_action<Requirement, Region>()(target, ref, region, this_id);
        }

        template <locate_state state, typename Requirement>
        hpx::future<location_info<typename Requirement::region_type>>
        locate(Requirement req);

        template <locate_state state, typename Requirement>
        struct locate_action
          : hpx::actions::make_action<
                decltype(&locate<state, Requirement>),
                &locate<state, Requirement>,
                locate_action<state, Requirement>>::type
        {};

        template <locate_state state, typename Requirement>
        hpx::future<location_info<typename Requirement::region_type>>
        locate(Requirement req)
        {
            using region_type = typename Requirement::region_type;
            using location_info_type = location_info<region_type>;
            using data_item_type = typename Requirement::data_item_type;
            using fragment_type = typename data_item_store<data_item_type>::data_item_type::fragment_type;
            using mutex_type = typename data_item_store<data_item_type>::data_item_type::mutex_type;

            HPX_ASSERT(!req.region.empty());

            std::size_t this_id = hpx::get_locality_id();

            location_info_type info;

            auto& item = data_item_store<data_item_type>::lookup(req.ref);
            std::unique_lock<mutex_type> l(item.mtx);
            // FIXME: wait for migration and lock here.

            // Now try to locate ...
            region_type remainder = req.region;

            // First check the intersection of our own region
            region_type part = region_type::intersect(remainder, item.owned_region);
            if (!part.empty())
            {
                info.regions.insert(std::make_pair(this_id, part));
                // Subtract what we got from what we requested
                remainder = region_type::difference(remainder, part);

                // If the remainder is empty, we got everything covered...
                if (remainder.empty())
                {
                    HPX_ASSERT(!info.regions.empty());
                    return hpx::make_ready_future(std::move(info));
                }
            }

            // Lookup in our cache
            for (auto const& cached: item.location_cache.regions)
            {
                part = region_type::intersect(remainder, cached.second);
                // We got a hit!
                if (!part.empty())
                {
                    // Otherwise, collect the location
                    info.regions.insert(std::make_pair(cached.first, part));

                    // Subtract what we got from what we requested
                    remainder = region_type::difference(remainder, part);

                    // If the remainder is empty, we got everything covered...
                    if (remainder.empty())
                    {
                        HPX_ASSERT(!info.regions.empty());
                        return hpx::make_ready_future(std::move(info));
                    }
                }
            }

            // We couldn't satisfy the request with locally stored information...
            // now branch out to our parent and children...
            std::array<hpx::future<location_info_type>, 3> remote_infos;

            // Check parent
            part = region_type::intersect(remainder, item.parent_region);
            using locate_down_action_type = locate_action<locate_state::down, Requirement>;
            using locate_up_action_type = locate_action<locate_state::up, Requirement>;
            if (!part.empty())
            {
                hpx::util::unlock_guard<std::unique_lock<mutex_type>> ul(l);
                HPX_ASSERT(this_id != 0);
                hpx::id_type target(
                    hpx::naming::get_id_from_locality_id(
                        (this_id-1)/2
                    )
                );
                // FIXME: make resilient
                remote_infos[0] = hpx::async<locate_down_action_type>(
                    target, Requirement(req.ref, part, req.mode));

                // Subtract what we got from what we requested
                remainder = region_type::difference(remainder, part);
            }
            else if (state != locate_state::up && this_id != 0 && !remainder.empty())
            {
                hpx::util::unlock_guard<std::unique_lock<mutex_type>> ul(l);
                hpx::id_type target(
                    hpx::naming::get_id_from_locality_id(
                        (this_id-1)/2
                    )
                );
                // FIXME: make resilient
                remote_infos[0] = hpx::async<locate_down_action_type>(
                    target, Requirement(req.ref, remainder, req.mode));
            }
            // Check left
            part = region_type::intersect(remainder, item.left_region);
            if (!part.empty())
            {
                hpx::util::unlock_guard<std::unique_lock<mutex_type>> ul(l);
                hpx::id_type target(
                    hpx::naming::get_id_from_locality_id(
                        this_id * 2 + 1
                    )
                );
                // FIXME: make resilient
                remote_infos[1] = hpx::async<locate_up_action_type>(
                    target, Requirement(req.ref, part, req.mode));

                // Subtract what we got from what we requested
                remainder = region_type::difference(remainder, part);
            }
            else if (state != locate_state::down && !remainder.empty())
            {
                hpx::util::unlock_guard<std::unique_lock<mutex_type>> ul(l);
                if (this_id * 2 + 1 < hpx::get_num_localities().get())
                {
                    hpx::id_type target(
                        hpx::naming::get_id_from_locality_id(
                            this_id * 2 + 1
                        )
                    );
                    // FIXME: make resilient
                    remote_infos[1] = hpx::async<locate_up_action_type>(
                        target, Requirement(req.ref, remainder, req.mode));
                }
            }
            // Check right...
            part = region_type::intersect(remainder, item.right_region);
            if (!part.empty())
            {
                hpx::util::unlock_guard<std::unique_lock<mutex_type>> ul(l);
                hpx::id_type target(
                    hpx::naming::get_id_from_locality_id(
                        this_id * 2 + 2
                    )
                );
                // FIXME: make resilient
                remote_infos[2] = hpx::async<locate_up_action_type>(
                    target, Requirement(req.ref, part, req.mode));

                // Subtract what we got from what we requested
                remainder = region_type::difference(remainder, part);
            }
            else if (state != locate_state::down && !remainder.empty())
            {
                hpx::util::unlock_guard<std::unique_lock<mutex_type>> ul(l);
                if (this_id * 2 + 2 < hpx::get_num_localities().get())
                {
                    hpx::id_type target(
                        hpx::naming::get_id_from_locality_id(
                            this_id * 2 + 2
                        )
                    );
                    // FIXME: make resilient
                    remote_infos[2] = hpx::async<locate_up_action_type>(
                        target, Requirement(req.ref, remainder, req.mode));
                }
            }

            l.unlock();

            return hpx::when_all(remote_infos).then(
                [info = std::move(info), req, remainder = std::move(remainder)](auto remote_infos_fut) mutable
                {
                    auto remote_infos = remote_infos_fut.get();
                    auto& item = data_item_store<data_item_type>::lookup(req.ref);
                    std::size_t this_id = hpx::get_locality_id();

                    std::unique_lock<mutex_type> l(item.mtx);
//                     if (state == locate_state::init)
//                     {
//                         std::cout << "Located " << req.ref.id() << " (" << req.region << ", " <<
//                             (req.mode == access_mode::ReadOnly? "ro" : "rw") << ") on " << this_id << '\n';
//                     }

                    // Merge infos
                    std::size_t idx = 0;
                    for (auto& info_parts_fut: remote_infos)
                    {
                        if (info_parts_fut.valid())
                        {
                            auto info_parts = info_parts_fut.get();
                            for (auto const& remote_info: info_parts.regions)
                            {
                                // update cache.
                                auto & cached_part = item.location_cache.regions[remote_info.first];
                                cached_part = region_type::merge(cached_part, remote_info.second);

                                // Insert location information...
                                auto & part = info.regions[remote_info.first];
                                part = region_type::merge(part, remote_info.second);


                                remainder = region_type::difference(remainder, part);
//                                 std::cout << " part " << part << " on " << remote_info.second << " remaining from request: " << remainder << "\n";

                                // update our own info
                                // TODO: is this needed?
//                                 if (remote_info.first < this_id)
//                                 {
//                                     item.parent_region = region_type::merge(item.parent_region, remote_info.second);
//                                 }
//                                 if (idx == 1)
//                                 {
//                                     item.left_region = region_type::merge(item.left_region, remote_info.second);
//                                 }
//                                 if (idx == 2)
//                                 {
//                                     item.right_region = region_type::merge(item.right_region, remote_info.second);
//                                 }
                            }
                        }
                        ++idx;
                    }

                    // We have a first touch if we couldn't locate all parts.
                    if (state == locate_state::init && !remainder.empty())
                    {
                        // propagate information up.
                        // FIXME: futurize
                        if (this_id != 0)
                        {
                            hpx::util::unlock_guard<std::unique_lock<mutex_type>> ul(l);

                            // FIXME: make resilient
                            hpx::id_type target(
                                hpx::naming::get_id_from_locality_id(
                                    (this_id-1)/2
                                )
                            );
                            register_child_action<Requirement, region_type>()(
                                target, req.ref.id(), remainder, this_id);
                        }

                        info.regions.insert(std::make_pair(this_id, remainder));

                        // Merge with our own region
                        item.owned_region = region_type::merge(item.owned_region, remainder);

                        // And finally, allocate the fragment if it wasn't there already.
                        if (item.fragment == nullptr)
                        {
                            // FIXME: distribute shared data correctly...
                            item.fragment.reset(new fragment_type(req.ref.shared_data()));
                        }
//                         std::cout << "First touch " << req.ref.id() << " (" << req.region << ", " <<
//                             (req.mode == access_mode::ReadOnly? "ro" : "rw") << ") on " << this_id << '\n';
                    }

//                     HPX_ASSERT(!info.regions.empty());

                    return info;
                }
            );
        }

        template <locate_state state, typename Requirement, typename Allocator>
        std::vector<hpx::future<location_info<typename Requirement::region_type>>>
        locate(std::vector<Requirement, Allocator> const& reqs)
        {
            std::vector<hpx::future<location_info<typename Requirement::region_type>>> res;
            res.reserve(reqs.size());
            for (auto const& req: reqs)
            {
                res.push_back(locate<state>(req));
            }
            return res;
        }

        template <locate_state state, typename Requirements, std::size_t...Is>
        auto locate(Requirements const& reqs, hpx::util::detail::pack_c<std::size_t, Is...>)
         -> hpx::util::tuple<decltype(locate<state>(hpx::util::get<Is>(reqs)))...>
        {
            return hpx::util::make_tuple(locate<state>(hpx::util::get<Is>(reqs))...);
        }
    }

    template <typename Requirements>
    auto locate(Requirements const& reqs)
     -> decltype(detail::locate<detail::locate_state::init>(reqs,
            std::declval<typename hpx::util::detail::make_index_pack<
                hpx::util::tuple_size<Requirements>::type::value>::type>()))
    {
        return detail::locate<detail::locate_state::init>(reqs,
            typename hpx::util::detail::make_index_pack<
                hpx::util::tuple_size<Requirements>::type::value>::type{});
    }

}}

#endif