
#ifndef ALLSCALE_THIS_WORK_ITEM_HPP
#define ALLSCALE_THIS_WORK_ITEM_HPP

namespace allscale {
    namespace detail {
        struct work_item_impl_base;
    }
    struct work_item;
}

namespace allscale {
    namespace this_work_item
    {
        struct set
        {
            set(detail::work_item_impl_base& wi);
            ~set();

        private:
            detail::work_item_impl_base *old;
        };

        detail::work_item_impl_base* get();
    }
}


#endif
