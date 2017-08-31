
#ifndef ALLSCALE_COMPONENTS_SCHEDULER_HPP
#define ALLSCALE_COMPONENTS_SCHEDULER_HPP

#include <allscale/work_item.hpp>
#include <allscale/components/treeture_buffer.hpp>
#include <allscale/components/scheduler_network.hpp>

#include <hpx/include/components.hpp>
// #include <hpx/include/local_lcos.hpp>
#include <hpx/util/interval_timer.hpp>

#include <hpx/compute/host.hpp>
#include <hpx/compute/host/target.hpp>
#include <hpx/runtime/threads/executors/thread_pool_attached_executors.hpp>
#include <hpx/runtime/threads/policies/throttling_scheduler.hpp>
#include <hpx/runtime/threads/threadmanager.hpp>

#include <atomic>
#include <memory>
#include <deque>
#include <vector>
#include <unordered_map>

using executor_type = hpx::threads::executors::local_priority_queue_attached_executor;

namespace allscale { namespace components {

    struct scheduler
      : hpx::components::component_base<scheduler>
    {
        typedef hpx::lcos::local::spinlock mutex_type;

        scheduler()
        {
            HPX_ASSERT(false);
        }

        scheduler(std::uint64_t rank);
        void init();

        void enqueue(work_item work, this_work_item::id const&);
        HPX_DEFINE_COMPONENT_ACTION(scheduler, enqueue);

        void stop();
        HPX_DEFINE_COMPONENT_ACTION(scheduler, stop);

    private:
        hpx::resource::detail::partitioner *rp_;
        const hpx::threads::topology *topo_;
        std::uint64_t num_localities_;
        std::uint64_t num_threads_;
        std::uint64_t rank_;
        std::atomic<bool> stopped_;

        scheduler_network network_;

        mutex_type spawn_throttle_mtx_;
        std::unordered_map<const char*, treeture_buffer> spawn_throttle_;

        bool do_split(work_item const& work);

        bool collect_counters();

        bool periodic_throttle();

        hpx::util::interval_timer timer_;
        hpx::util::interval_timer throttle_timer_;

        mutex_type counters_mtx_;
        hpx::id_type idle_rate_counter_;
        double idle_rate_;

        hpx::id_type queue_length_counter_;
        std::size_t queue_length_;

        hpx::id_type threads_total_counter_id;
        double total_threads_time;

        hpx::id_type allscale_app_counter_id;

        std::vector<hpx::compute::host::target> numa_domains;
        std::vector<executor_type> executors;
        std::atomic<std::size_t> current_;

        std::size_t os_thread_count;
        std::size_t active_threads;
        std::size_t depth_cap;

        hpx::threads::policies::throttling_scheduler<>* thread_scheduler;

        mutable mutex_type throttle_mtx_;
        mutable mutex_type resize_mtx_;

        std::uint16_t sampling_interval;
        double last_avg_iter_time;
        double current_avg_iter_time;
        monitor *allscale_monitor;

        std::string input_objective;
        const std::vector<std::string> objectives = {
		"time",
		"resource",
		"energy",
		"time_resource",
		"time_energy",
		"resource_energy",
		"time_resource_energy"
	};

        enum objective_IDs {
              TIME = 0,
     	      RESOURCE,
              ENERGY,
              TIME_RESOURCE,
              TIME_ENERGY,
              RESOURCE_ENERGY,
              TIME_RESOURCE_ENERGY
        };

    };
}}

#endif
