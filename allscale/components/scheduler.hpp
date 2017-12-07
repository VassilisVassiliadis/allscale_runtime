
#ifndef ALLSCALE_COMPONENTS_SCHEDULER_HPP
#define ALLSCALE_COMPONENTS_SCHEDULER_HPP

#include <allscale/work_item.hpp>
#include <allscale/this_work_item.hpp>
#include <allscale/components/treeture_buffer.hpp>
#include <allscale/components/scheduler_network.hpp>
#if defined(ALLSCALE_HAVE_CPUFREQ)
#include <allscale/util/hardware_reconf.hpp>
#endif

#include <hpx/include/components.hpp>
// #include <hpx/include/local_lcos.hpp>
#include <hpx/util/interval_timer.hpp>

#include <hpx/compute/host.hpp>
#include <hpx/compute/host/target.hpp>
#include <hpx/runtime/threads/executors/pool_executor.hpp>
#include <hpx/runtime/threads/policies/throttling_scheduler.hpp>
#include <hpx/runtime/threads/threadmanager.hpp>

#include <atomic>
#include <memory>
#include <deque>
#include <vector>
#include <unordered_map>


namespace allscale { namespace components {

    struct scheduler
      : hpx::components::component_base<scheduler>
    {
        typedef hpx::lcos::local::spinlock mutex_type;
#if defined(ALLSCALE_HAVE_CPUFREQ)
        using hardware_reconf = allscale::components::util::hardware_reconf;
#endif

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
        std::size_t get_num_numa_nodes();
        std::size_t get_num_numa_cores(std::size_t domain);
        hpx::resource::detail::partitioner *rp_;
        const hpx::threads::topology *topo_;
        machine_config mconfig_;
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
        bool periodic_frequency_scale();

        hpx::util::interval_timer timer_;
        hpx::util::interval_timer throttle_timer_;
        hpx::util::interval_timer frequency_timer_;

        mutex_type counters_mtx_;
        hpx::id_type idle_rate_counter_;
        double idle_rate_;

        hpx::id_type queue_length_counter_;
        std::size_t queue_length_;

        hpx::id_type threads_total_counter_id;
        double total_threads_time;

        hpx::id_type allscale_app_counter_id;

        std::vector<hpx::threads::detail::thread_pool_base*> thread_pools_;
        std::vector<hpx::threads::mask_type> initial_masks_;
        std::vector<executor_type> executors_;
        std::atomic<std::size_t> current_;

        std::size_t os_thread_count;
        std::size_t active_threads;

        double enable_factor;
        double disable_factor;
        unsigned int min_threads;
        // Indices show number of threads, which hold pair of
        // execution times and number of times that particular thread used
        // due to suspend and resume
        std::vector<std::pair<double, unsigned int>> thread_times;

        unsigned long min_freq;
        unsigned long max_freq;
        unsigned long long current_energy_usage;
        unsigned long long last_energy_usage;
        unsigned long long last_actual_energy_usage;
        unsigned long long actual_energy_usage;
#if defined(ALLSCALE_HAVE_CPUFREQ)
        cpufreq_policy policy;
        hardware_reconf::hw_topology topo;
        std::vector<unsigned long> cpu_freqs;
        // Indices correspond to the freq id in cpu_freqs, and
        // each pair holds energy usage and execution time
        std::vector<std::pair<unsigned long long, double>> freq_times;
        unsigned int freq_step;
        bool target_freq_found;
#endif

        mutable mutex_type throttle_mtx_;
        mutable mutex_type resize_mtx_;

        std::uint16_t sampling_interval;
        double last_avg_iter_time;
        double current_avg_iter_time;
        monitor *allscale_monitor;

        std::vector<std::pair<std::string, double>> objectives_with_leeways;
        const std::vector<std::string> objectives = {
            "time",
            "resource",
            "energy",
    	};

        bool time_requested;
        bool resource_requested;
        bool energy_requested;

        double time_leeway;
        double resource_leeway;
        double energy_leeway;

    };
}}

#endif
