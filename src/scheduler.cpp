
#include <allscale/scheduler.hpp>
#include <allscale/components/scheduler.hpp>

#include <hpx/include/components.hpp>

HPX_REGISTER_COMPONENT_MODULE()

typedef hpx::components::component<allscale::components::scheduler> scheduler_component;
HPX_REGISTER_COMPONENT(scheduler_component)

namespace allscale
{
    std::size_t scheduler::rank_ = std::size_t(-1);

    void scheduler::schedule(work_item work)
    {
        get().enqueue(work);
    }

    void scheduler::run(std::size_t rank)
    {
        rank_ = rank;
        hpx::apply(
            &components::scheduler::run,
            get_ptr()
        );
    }

    void scheduler::stop()
    {
        get().stop();
        get_ptr().reset();
    }

    components::scheduler &scheduler::get()
    {
        return *get_ptr();
    }

    std::shared_ptr<components::scheduler> &scheduler::get_ptr()
    {
        static scheduler s(rank_);
        return s.component_;
    }

    scheduler::scheduler(std::size_t rank)
    {
        hpx::id_type gid =
            hpx::new_<components::scheduler>(hpx::find_here(), rank).get();

        hpx::register_with_basename("allscale/scheduler", gid, rank).get();

        component_ = hpx::get_ptr<components::scheduler>(gid).get();
        component_->init();
    }
}
