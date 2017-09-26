
#include <allscale/components/scheduler.hpp>
#include <allscale/components/monitor.hpp>
#include <allscale/monitor.hpp>
#include <allscale/resilience.hpp>

#include <hpx/util/scoped_unlock.hpp>
#include <hpx/traits/executor_traits.hpp>

#include <sstream>
#include <iterator>
#include <algorithm>
#include <stdlib.h>


namespace allscale { namespace components {

    scheduler::scheduler(std::uint64_t rank)
      : num_localities_(hpx::get_num_localities().get())
      , num_threads_(hpx::get_num_worker_threads())
      , rank_(rank)
      , stopped_(false)
      , os_thread_count(hpx::get_os_thread_count())
      , active_threads(os_thread_count)
      , current_avg_iter_time(0.0)
      , sampling_interval(10)
      , enable_factor(1.0)
      , disable_factor(1.0)
      , min_threads(4)
	  , current_energy_usage(0)
      , last_actual_energy_usage(0)
      , actual_energy_usage(0)
      #if defined(ALLSCALE_HAVE_CPUFREQ)
      , target_freq_found(false)
      #endif
      , time_requested(false)
      , resource_requested(false)
      , energy_requested(false)
      , time_leeway(1.0)
      , resource_leeway(1.0)
      , energy_leeway(1.0)
//       , count_(0)
      , timer_(
            hpx::util::bind(
                &scheduler::collect_counters,
                this
            ),
            700,
            "scheduler::collect_counters",
            true
        )

      , throttle_timer_(
           hpx::util::bind(
               &scheduler::periodic_throttle,
               this
           ),
           100000, //0.1 sec
           "scheduler::periodic_throttle",
           true
        )
      , frequency_timer_(
           hpx::util::bind(
               &scheduler::periodic_frequency_scale,
               this
           ),
           10000000, //0.1 sec
           "scheduler::periodic_frequency_scale",
           true
        )
    {
        allscale_monitor = &allscale::monitor::get();
        thread_times.resize(hpx::get_os_thread_count());
    }

    void scheduler::init()
    {
        rp_ = &hpx::resource::get_partitioner();
        topo_ = &hpx::threads::get_topology();

        std::string input_objective_str = hpx::get_config_entry("allscale.objective", "");
        if ( !input_objective_str.empty() )
        {
            std::istringstream iss_leeways(input_objective_str);
            std::string objective_str;
            while (iss_leeways >> objective_str)
            {
                auto idx = objective_str.find(':');
                std::string obj = objective_str;
                // Don't scale objectives if none is given
                double leeway = 1.0;

                if (idx != std::string::npos)
                {
                    obj = objective_str.substr(0, idx);
                    leeway = std::stod( objective_str.substr(idx + 1) );
                }

                if (obj == "time")
                {
                    time_requested = true;
                    time_leeway = leeway;
                }
                else if (obj == "resource")
                {
                    resource_requested = true;
                    resource_leeway = leeway;
                }
                else if (obj == "energy")
                {
                    energy_requested = true;
                    energy_leeway = leeway;
                }
                else
                {
                    std::ostringstream all_keys;
                    copy(scheduler::objectives.begin(), scheduler::objectives.end(), std::ostream_iterator<std::string>(all_keys, ","));
                    std::string keys_str = all_keys.str();
                    keys_str.pop_back();
                    HPX_THROW_EXCEPTION(hpx::bad_request, "scheduler::init",
                            boost::str(boost::format("Wrong objective: %s, Valid values: [%s]") % obj % keys_str));
                }

                if ( time_leeway > 1 || resource_leeway > 1 || energy_leeway > 1 )
                {
                    HPX_THROW_EXCEPTION(hpx::bad_request, "scheduler::init", "leeways should be within [0, 1]");
                }

//                objectives_with_leeways.push_back(std::make_pair(obj, leeway));
            }

            if ( resource_requested && energy_requested )
                HPX_THROW_EXCEPTION(hpx::bad_request, "scheduler::init",
                            boost::str(boost::format("Sorry not supported yet. Check back soon!")));
        }


        // setup performance counter to use to decide on split/process
        static const char * queue_counter_name = "/threadqueue{locality#%d/total}/length";

        const std::uint32_t prefix = hpx::get_locality_id();

        queue_length_counter_ = hpx::performance_counters::get_counter(
            boost::str(boost::format(queue_counter_name) % prefix));

        hpx::performance_counters::stubs::performance_counter::start(
            hpx::launch::sync, queue_length_counter_);


        collect_counters();

        auto const& numa_domains = rp_->numa_domains();
        executors_.reserve(numa_domains.size());
        thread_pools_.reserve(numa_domains.size());

        for (std::size_t domain = 0; domain < numa_domains.size(); ++domain)
        {
            std::string pool_name;
            if (domain == 0)
                pool_name = "default";
            else
                pool_name = "allscale/numa/" + std::to_string(domain);

            thread_pools_.push_back(
                &hpx::resource::get_thread_pool(pool_name));
            std::cout << "Attached to " << pool_name << " (" << thread_pools_.back() << '\n';
            initial_masks_.push_back(thread_pools_.back()->get_used_processing_units());
            executors_.emplace_back(pool_name);
        }

//         timer_.start();

        if ( time_requested || resource_requested || energy_requested ) {
            if ( energy_requested )
            {
#if defined(ALLSCALE_HAVE_CPUFREQ)
                using hardware_reconf = allscale::components::util::hardware_reconf;
                cpu_freqs = hardware_reconf::get_frequencies(0);
                freq_step = 8; //cpu_freqs.size() / 2;
                freq_times.resize(cpu_freqs.size());

                auto min_max_freqs = std::minmax_element(cpu_freqs.begin(), cpu_freqs.end());
                min_freq = *min_max_freqs.first;
                max_freq = *min_max_freqs.second;

                // We have to set CPU governors to userpace in order to change frequencies later
                std::string governor = "userspace";
                policy.governor = const_cast<char*>(governor.c_str());
                policy.min = min_freq;
                policy.max = max_freq;

                topo = hardware_reconf::read_hw_topology();
                for (int cpu_id = 0; cpu_id < topo.num_logical_cores; cpu_id += topo.num_hw_threads)
                    int res = hardware_reconf::set_freq_policy(cpu_id, policy);

                // Set frequency of all threads to max when we start
                int res = hardware_reconf::set_frequency(0, os_thread_count, cpu_freqs[0]);

                std::this_thread::sleep_for(std::chrono::microseconds(2));

                // Make sure frequency change happened before continuing
                for (int cpu_id = 0; cpu_id < topo.num_logical_cores; cpu_id += topo.num_hw_threads)
                {
                    unsigned long hardware_freq = hardware_reconf::get_hardware_freq(cpu_id);
                    HPX_ASSERT(hardware_freq == cpu_freqs[0]);
                }


                frequency_timer_.start();
#else
                HPX_THROW_EXCEPTION(hpx::bad_request, "scheduler::init",
                            "Requesting energy objective without having compiled with cpufreq");
#endif
            }
        }

        std::cerr
            << "Scheduler with rank " << rank_ << " created!\n";
    }

    void scheduler::enqueue(work_item work, this_work_item::id const& id)
    {
        if (id)
            this_work_item::set_id(id);

        std::uint64_t schedule_rank = work.id().rank();
        if(!allscale::resilience::rank_running(schedule_rank))
        {
            schedule_rank = rank_;
        }

        HPX_ASSERT(work.valid());

        HPX_ASSERT(schedule_rank != std::uint64_t(-1));

#ifdef DEBUG_
        std::cout << "Will schedule task " << work.id().name() << " on rank " << schedule_rank << std::endl;
#endif

        // schedule locally
        if (schedule_rank == rank_)
        {
            if (work.is_first())
            {
                std::size_t current_id = work.id().last();
                const char* wi_name = work.name();
                {
                    std::unique_lock<mutex_type> lk(spawn_throttle_mtx_);
                    auto it = spawn_throttle_.find(wi_name);
                    if (it == spawn_throttle_.end())
                    {
                        auto em_res = spawn_throttle_.emplace(wi_name,
                            treeture_buffer(6));//num_threads_));
                        it = em_res.first;
                    }
                    it->second.add(std::move(lk), work.get_treeture());
                }
                allscale::monitor::signal(allscale::monitor::work_item_first, work);

                if (current_id % 100 == 0)
                {
                    periodic_throttle();
                }
            }
            else
            {
            }
            allscale::monitor::signal(allscale::monitor::work_item_enqueued, work);

            // FIXME: modulus shouldn't be needed here
            std::size_t numa_domain = work.id().numa_domain() % executors_.size();
            HPX_ASSERT(numa_domain < executors_.size());
            if (do_split(work))
            {
                work.split(executors_[numa_domain]);
            }
            else
            {
                work.process(executors_[numa_domain]);
            }

            return;
        }

        network_.schedule(schedule_rank, std::move(work), this_work_item::get_id());
    }

    bool scheduler::do_split(work_item const& w)
    {
        // Check if the work item is splittable first
        if (w.can_split())
        {
            // Check if we reached the required depth
            // FIXME: make the cut off runtime configurable...
            if (w.id().splittable())
            {
                // FIXME: add more elaborate splitting criterions
                return true;
            }
            return false;
        }
        // return false if it isn't
        return false;

    }

    bool scheduler::collect_counters()
    {
//         hpx::performance_counters::counter_value idle_value;
        hpx::performance_counters::counter_value length_value;
//         idle_value = hpx::performance_counters::stubs::performance_counter::get_value(
//                 hpx::launch::sync, idle_rate_counter_);
        length_value = hpx::performance_counters::stubs::performance_counter::get_value(
                hpx::launch::sync, queue_length_counter_);

//         double idle_rate = idle_value.get_value<double>() * 0.01;
        std::size_t queue_length = length_value.get_value<std::size_t>();

        {
            std::unique_lock<mutex_type> l(counters_mtx_);

//             idle_rate_ = idle_rate;
            queue_length_ = queue_length;
        }

        return true;
    }


    bool scheduler::periodic_throttle()
    {
        if ( num_threads_ > 1 && ( time_requested || resource_requested ) )
        {
            std::unique_lock<mutex_type> l(resize_mtx_);
            if ( current_avg_iter_time == 0.0 || allscale_monitor->get_number_of_iterations() < sampling_interval)
            {
                {
                    hpx::util::unlock_guard<std::unique_lock<mutex_type>> ul(l);
                    current_avg_iter_time = allscale_monitor->get_avg_time_last_iterations(sampling_interval);
//                    current_avg_iter_time = allscale_monitor->get_last_iteration_time();
//                    current_avg_iter_time = allscale_monitor->get_avg_work_item_times(sampling_interval);
                }
                return true;
            } else if ( current_avg_iter_time > 0 )
            {
                last_avg_iter_time = current_avg_iter_time;

                hpx::threads::mask_type active_mask;
                std::size_t active_threads_ = 0;
                std::size_t domain_active_threads = 0;
                std::size_t pool_idx = 0;
                {
                    hpx::util::unlock_guard<std::unique_lock<mutex_type>> ul(l);
//                    current_avg_iter_time = allscale_monitor->get_last_iteration_time();
                    current_avg_iter_time = allscale_monitor->get_avg_time_last_iterations(sampling_interval);

                    // Select thread pool with the highest number of activated threads
                    for (std::size_t i = 0; i != thread_pools_.size(); ++i)
                    {
                        hpx::threads::mask_type curr_mask = thread_pools_[i]->get_used_processing_units();
                        std::size_t curr_active_pus =
                            hpx::threads::count(curr_mask);
                        active_threads_ += curr_active_pus;
                        if (curr_active_pus > domain_active_threads)
                        {
                            domain_active_threads = curr_active_pus;
                            active_mask = curr_mask;
                            pool_idx = i;
                        }
                    }
                }

                active_threads = active_threads_;

                auto blocked_os_threads = active_mask & hpx::threads::not_(initial_masks_[pool_idx]);

                unsigned thread_use_count = thread_times[active_threads - 1].second;
                double thread_exe_time = current_avg_iter_time + thread_times[active_threads - 1].first;
                thread_times[active_threads - 1] = std::make_pair(thread_exe_time, thread_use_count + 1);

                std::size_t suspend_cap = 1; //active_threads < SMALL_SYSTEM  ? SMALL_SUSPEND_CAP : LARGE_SUSPEND_CAP;
                std::size_t resume_cap = 1;  //active_threads < SMALL_SYSTEM  ? LARGE_RESUME_CAP : SMALL_RESUME_CAP;

                double time_threshold = current_avg_iter_time;
                bool disable_flag = last_avg_iter_time > time_threshold;
                bool enable_flag = last_avg_iter_time * enable_factor < time_threshold;


                if ( resource_requested )
                {
                    // If we have a sublinear speedup then prefer resources over time and throttle
                    time_threshold = current_avg_iter_time * (active_threads - suspend_cap ) / active_threads;
                    disable_flag = last_avg_iter_time > time_threshold;
                    enable_flag = last_avg_iter_time < time_threshold;
                    min_threads = 1;
                }


                if (disable_flag && domain_active_threads > min_threads)
                {
                    std::vector<std::size_t> suspend_threads;
                    std::size_t thread_count = thread_pools_[pool_idx]->get_os_thread_count();
                    suspend_threads.reserve(thread_count);
                    for (std::size_t i = 0; i < thread_count; ++i)
                    {
                        std::size_t pu_num = rp_->get_pu_num(i + thread_pools_[pool_idx]->get_thread_offset());
                        if (hpx::threads::test(active_mask, pu_num))
                        {
                            suspend_threads.push_back(i);
                            if (suspend_threads.size() == suspend_cap)
                                break;
                        }
                    }
                    {
                        hpx::util::unlock_guard<std::unique_lock<mutex_type> > ul(l);
                        for(auto& pu: suspend_threads)
                        {
                            thread_pools_[pool_idx]->remove_processing_unit(pu);
                        }
                    }
                    std::cout << "Sent disable signal. Active threads: " << active_threads - suspend_cap << std::endl;
                }
                else if (enable_flag && hpx::threads::any(blocked_os_threads))
                {
                    if ( active_threads < topo_->get_number_of_pus() / topo_->get_number_of_cores() + min_threads &&  enable_factor < 1.01 )
                        enable_factor *= 1.0005;


                    std::vector<std::size_t> resume_threads;
                    std::size_t thread_count = thread_pools_[pool_idx]->get_os_thread_count();
                    resume_threads.reserve(thread_count);
                    for (std::size_t i = 0; i < thread_count; ++i)
                    {
                        std::size_t pu_num = rp_->get_pu_num(i + thread_pools_[pool_idx]->get_thread_offset());
                        if (hpx::threads::test(blocked_os_threads, pu_num))
                        {
                            resume_threads.push_back(i);
                            if (resume_threads.size() == resume_cap)
                                break;
                        }
                    }
                    {
                        hpx::util::unlock_guard<std::unique_lock<mutex_type> > ul(l);
                        for(auto& pu: resume_threads)
                        {
                            thread_pools_[pool_idx]->add_processing_unit(pu, pu + thread_pools_[pool_idx]->get_thread_offset());
                        }
                    }
                    std::cout << "Sent enable signal. Active threads: " << active_threads + resume_cap << std::endl;
                }
            }
        }

        return true;
    }


    bool scheduler::periodic_frequency_scale()
    {
#if defined(ALLSCALE_HAVE_CPUFREQ)
        std::unique_lock<mutex_type> l(resize_mtx_);
        if ( !target_freq_found )
        {
            if ( current_energy_usage == 0 )
            {
                current_energy_usage = hardware_reconf::read_system_energy();
                return true;
            } else if ( current_energy_usage > 0 )
            {
                last_energy_usage = current_energy_usage;
                current_energy_usage = hardware_reconf::read_system_energy();
                last_actual_energy_usage = actual_energy_usage;
                actual_energy_usage = current_energy_usage - last_energy_usage;

                last_avg_iter_time = current_avg_iter_time;
                {
                    hpx::util::unlock_guard<std::unique_lock<mutex_type>> ul(l);
//                    current_avg_iter_time = allscale_monitor->get_avg_time_last_iterations(sampling_interval);
                    current_avg_iter_time = allscale_monitor->get_last_iteration_time();
                }

                unsigned int freq_idx = -1;
                unsigned long current_freq_hw = hardware_reconf::get_hardware_freq(0);
                //unsigned long current_freq_kernel = hardware_reconf::get_kernel_freq(0);
                //std::cout << "current_freq_hw: " << current_freq_hw << ", current_freq_kernel: " << current_freq_kernel << std::endl;

                // Get freq index in cpu_freq
                std::vector<unsigned long>::iterator it = std::find(cpu_freqs.begin(), cpu_freqs.end(), current_freq_hw);
                if ( it != cpu_freqs.end() )
                    freq_idx = it - cpu_freqs.begin();
                else
                {
                    // If you run it without sudo, get_hardware_freq will fail and end up here as well!
                    HPX_THROW_EXCEPTION(hpx::bad_request, "scheduler::periodic_frequency_scale",
                        boost::str(boost::format("Cannot find frequency: %s in the list of frequencies. Something must be wrong!") % current_freq_hw));
                }

                freq_times[freq_idx] = std::make_pair(actual_energy_usage, current_avg_iter_time);

                unsigned long target_freq = current_freq_hw;
                // If we have not finished until the minimum frequnecy then continue
                if ( target_freq != min_freq ) {
                    hpx::util::unlock_guard<std::unique_lock<mutex_type> > ul(l);
                    //Start decreasing frequencies by 2
                    hardware_reconf::set_next_frequency(freq_step, true);
                    target_freq = hardware_reconf::get_hardware_freq(0);
                    std::cout << "Decrease frequency. " << "actual_energy_usage: "
                         << actual_energy_usage << ", current_freq_hw: "
                         << current_freq_hw << ", target_freq: "
                         << target_freq << ", current_avg_iter_time: "
                         << current_avg_iter_time << std::endl;
                }
                else
                {   // We should have measurement with all frequencies with step 2
                    // End of freq_times contains minimum frequency
                    unsigned long long min_energy = freq_times.back().first;
                    unsigned int min_energy_idx = freq_times.size() - 1;

                    double min_exec_time = freq_times[0].second;
                    unsigned int min_exec_time_idx = 0;

                    for (int i = 0; i < freq_times.size(); i++)
                    {
                        // If we have frequencies with zero energy usage
                        // it means we haven't measured them, so skip them
                        if ( freq_times[i].first == 0 )
                            continue;

                        if ( min_energy > freq_times[i].first )
                        {
                            min_energy = freq_times[i].first;
                            min_energy_idx = i;
                        }

                        if ( min_exec_time > freq_times[i].second )
                        {
                            min_exec_time = freq_times[i].second;
                            min_exec_time_idx = i;
                        }
                    }

                    // We will save minimum energy and execution time
                    // and use them for comparision using leeways
                    unsigned long long optimal_energy = min_energy;
                    double optimal_exec_time = min_exec_time;

                    if ( time_requested && energy_requested )
                    {
                        for (int i = 0; i < freq_times.size(); i++)
                        {
                            // the frequencies that have not been used will have default value of zero
                            if ( freq_times[i].first == 0 )
                                continue;

                            if ( ( time_leeway < 1 && energy_leeway == 1.0 ) && freq_times[i].second  - min_exec_time <  min_exec_time * time_leeway )
                            {
                                if ( optimal_energy > freq_times[i].first )
                                {
                                    min_energy_idx = i;
                                    min_exec_time_idx = i;
                                    optimal_energy = freq_times[i].first;
                                }
                            }
                            else if ( ( energy_leeway < 1 && time_leeway == 1.0 ) && freq_times[i].first  - min_energy <  min_energy * energy_leeway  )
                            {

                                if ( optimal_exec_time > freq_times[i].second )
                                {
                                    min_energy_idx = i;
                                    min_exec_time_idx = i;
                                    optimal_exec_time = freq_times[i].second;
                                }
                            }
                            else if ( time_leeway == 1.0 && energy_leeway == 1.0 )
                            {
                                if ( optimal_energy > freq_times[i].first && optimal_exec_time > freq_times[i].second )
                                {
                                    optimal_energy = freq_times[i].first;
                                    min_energy_idx = i;
                                    min_exec_time_idx = i;
                                    optimal_exec_time = freq_times[i].second;
                                }
                            }
                        }
                    }

                    hardware_reconf::set_frequencies_bulk(os_thread_count, cpu_freqs[min_energy_idx]);
                    target_freq_found = true;
                    std::cout << "Min energy: " << freq_times[min_energy_idx].first << " with freq: "
                              << cpu_freqs[min_energy_idx] << ", Min time with freq: "
                              << cpu_freqs[min_exec_time_idx] << std::endl;
                }
            }
        }
#endif

        return true;
    }



    void scheduler::stop()
    {
//         timer_.stop();

//        if ( time_requested || resource_requested )
//            throttle_timer_.stop();

        if ( energy_requested )
            frequency_timer_.stop();

        if(stopped_)
            return;

        //Resume all sleeping threads
        if ( ( time_requested || resource_requested ) && !energy_requested)
        {
            double resource_usage = 0;
            for (int i = 0; i < thread_times.size(); i++)
            {
                resource_usage += thread_times[i].first * (i + 1);
            }

            std::cout << "Resource usage: " << resource_usage << std::endl;
        }

        if ( energy_requested )
        {
#if defined(ALLSCALE_HAVE_CPUFREQ)
            std::string governor = "ondemand";
            policy.governor = const_cast<char*>(governor.c_str());

            for (int cpu_id = 0; cpu_id < topo.num_logical_cores; cpu_id += topo.num_hw_threads)
                int res = hardware_reconf::set_freq_policy(cpu_id, policy);

            std::cout << "Set CPU governors back to " << governor << std::endl;
#endif
        }

        stopped_ = true;
//         work_queue_cv_.notify_all();
//         std::cout << "rank(" << rank_ << "): scheduled " << count_ << "\n";

        network_.stop();
    }

}}
