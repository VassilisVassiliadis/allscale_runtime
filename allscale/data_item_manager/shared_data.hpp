
#ifndef ALLSCALE_DATA_ITEM_MANAGER_SHARED_DATA_HPP
#define ALLSCALE_DATA_ITEM_MANAGER_SHARED_DATA_HPP

namespace allscale { namespace data_item_manager {
    template <typename DataItemReference>
    typename DataItemReference::shared_data_type shared_data(DataItemReference const& ref);

    template <typename DataItemReference>
    struct shared_data_action
      : hpx::actions::make_direct_action<
            decltype(&shared_data<DataItemReference>),
            &shared_data<DataItemReference>,
            shared_data_action<DataItemReference>
        >::type
    {};

    template <typename DataItemReference>
    typename DataItemReference::shared_data_type shared_data(DataItemReference const& ref)
    {
        using data_item_type = typename DataItemReference::data_item_type;
        using mutex_type = typename data_item_store<data_item_type>::data_item_type::mutex_type;
        using shared_data_type = typename DataItemReference::shared_data_type;

        auto& item = data_item_store<data_item_type>::lookup(ref.id());

        std::unique_lock<mutex_type> l(item.mtx);

        if (item.shared_data == nullptr)
        {
            shared_data_type res;
            {
                hpx::util::unlock_guard<std::unique_lock<mutex_type>> ul(l);
                std::size_t this_id = hpx::get_locality_id();
                HPX_ASSERT(this_id != 0);
                // FIXME: make resilient
                hpx::id_type target(
                    hpx::naming::get_id_from_locality_id(
                        (this_id-1)/2
                    )
                );
                res = shared_data_action<DataItemReference>()(target, ref);
            }
            if (item.shared_data == nullptr)
            {
                item.shared_data.reset(new shared_data_type(std::move(res)));
            }
        }

        return *item.shared_data;
    }
}}

#endif
