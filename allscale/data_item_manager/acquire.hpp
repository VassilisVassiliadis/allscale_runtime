
#ifndef ALLSCALE_DATA_ITEM_MANAGER_ACQUIRE_HPP
#define ALLSCALE_DATA_ITEM_MANAGER_ACQUIRE_HPP

#include <allscale/lease.hpp>
#include <allscale/data_item_manager/data_item_store.hpp>
#include <allscale/data_item_manager/data_item_view.hpp>
#include <allscale/data_item_manager/location_info.hpp>
#include <hpx/plugins/parcel/coalescing_message_handler_registration.hpp>

#include <hpx/runtime/naming/id_type.hpp>
#include <hpx/util/tuple.hpp>

#include <vector>

namespace allscale { namespace data_item_manager {
    namespace detail
    {
        template <typename DataItemType>
        data_item_view<DataItemType> transfer(hpx::naming::gid_type id, typename DataItemType::region_type region)
        {
            auto& item = data_item_store<DataItemType>::lookup(id);

            HPX_ASSERT(item.fragment);

            // The data_item_view transparently extracts and inserts the parts
            // into the responsible fragments.
            return data_item_view<DataItemType>(id, std::move(region));
        }

        template <typename DataItemType>
        struct transfer_action
          : hpx::actions::make_direct_action<
                decltype(&transfer<DataItemType>),
                &transfer<DataItemType>,
                transfer_action<DataItemType>
            >::type
        {};

        template <typename Requirement, typename LocationInfo>
        hpx::future<allscale::lease<typename Requirement::data_item_type>>
        acquire(Requirement const& req, LocationInfo const& info)
        {
            using data_item_type = typename Requirement::data_item_type;
            using lease_type = allscale::lease<data_item_type>;
            using transfer_action_type = transfer_action<data_item_type>;
            using region_type = typename data_item_type::region_type;
            using mutex_type = typename data_item_store<data_item_type>::data_item_type::mutex_type;

            HPX_ASSERT(!req.region.empty());

            // Resize data to the requested size...
            auto& item = data_item_store<data_item_type>::lookup(req.ref);
            {
                std::unique_lock<mutex_type> l(item.mtx);
                HPX_ASSERT(item.fragment);
                region_type new_region;
                if (req.mode == access_mode::ReadOnly)
                {
                    new_region =
                        region_type::merge(item.fragment->getCoveredRegion(),
                        req.region);
                }
                else
                {
                    HPX_ASSERT(req.mode == access_mode::ReadWrite);
                    new_region =
                        region_type::merge(item.fragment->getCoveredRegion(),
                        // clip region to registered region...
                        region_type::intersect(item.owned_region, req.region));
                }
                item.fragment->resize(new_region);
            }


            if (req.mode == access_mode::ReadWrite)
            {
                return hpx::make_ready_future(lease_type(req));
            }
            else
            {
                HPX_ASSERT(req.mode == access_mode::ReadOnly);

                // collect the parts we need to transfer...
                std::vector<hpx::future<data_item_view<data_item_type>>> transfers;
                
		transfers.reserve(info.regions.size());
                
		for(auto const& part: info.regions)
                {
                    if (part.first != hpx::get_locality_id())
                    {
                        hpx::id_type target(
                            hpx::naming::get_id_from_locality_id(part.first));
                        transfers.push_back(
                            hpx::async<transfer_action_type>(target,
                                req.ref.id(), std::move(part.second)
                            )
                        );
                    }
                }
                if (transfers.empty())
                {
                    return hpx::make_ready_future(lease_type(req));
                }

                return hpx::dataflow(
                    [req = std::move(req)](
                        std::vector<hpx::future<data_item_view<data_item_type>>> transfers) mutable
                    {
                        // check for errors...
                        for (auto & transfer: transfers) transfer.get();
                        return lease_type(std::move(req));
                    },
                    std::move(transfers));
            }
        }

        template <typename Requirement, typename RequirementAllocator,
            typename LocationInfo, typename LocationInfoAllocator>
        std::vector<hpx::future<allscale::lease<typename Requirement::data_item_type>>>
        acquire(std::vector<Requirement, RequirementAllocator> const& reqs,
            std::vector<LocationInfo, LocationInfoAllocator> const& infos)
        {
            HPX_ASSERT(reqs.size() == infos.size());
            std::vector<hpx::future<allscale::lease<typename Requirement::data_item_type>>> leases;
            leases.reserve(reqs.size());
            for (std::size_t i = 0; i != reqs.size(); ++i)
            {
                leases.push_back(detail::acquire(reqs[i], infos[i]));
            }
            return leases;
        }

        template <typename Requirements, typename LocationInfos, std::size_t...Is>
        auto
        acquire(Requirements const& reqs, LocationInfos const& infos,
            hpx::util::detail::pack_c<std::size_t, Is...>)
         -> hpx::util::tuple<decltype(detail::acquire(hpx::util::get<Is>(reqs), hpx::util::get<Is>(infos)))...>
        {
            return hpx::util::make_tuple(
                detail::acquire(hpx::util::get<Is>(reqs), hpx::util::get<Is>(infos))...
            );
        }
    }

    template <typename Requirements, typename LocationInfos>
    auto
    acquire(Requirements const& reqs, LocationInfos const& infos)
     -> decltype(
        detail::acquire(reqs, infos,
            typename hpx::util::detail::make_index_pack<
                hpx::util::tuple_size<Requirements>::type::value>::type{}))
    {
        static_assert(
            hpx::util::tuple_size<Requirements>::value ==
            hpx::util::tuple_size<LocationInfos>::value,
            "requirements and location info sizes do not match");

        return detail::acquire(reqs, infos,
            typename hpx::util::detail::make_index_pack<
                hpx::util::tuple_size<Requirements>::type::value>::type{});
    }
}}

// #if defined(HPX_HAVE_PARCEL_COALESCING) && !defined(HPX_PARCEL_COALESCING_MODULE_EXPORTS)
//
// namespace hpx { namespace traits {
//     template <typename DataItemType>
//     struct action_message_handler<allscale::data_item_manager::detail::transfer_action<DataItemType>>
//     {
//         static parcelset::policies::message_handler* call(
//             parcelset::parcelhandler* ph, parcelset::locality const& loc,
//             parcelset::parcel const& /*p*/)
//         {
//             error_code ec(lightweight);
//             return parcelset::get_message_handler(ph, "allscale_transfer_action",
//                 "coalescing_message_handler", std::size_t(-1), std::size_t(-1), loc, ec);
//         }
//     };
// }}
//
// #endif

#endif
