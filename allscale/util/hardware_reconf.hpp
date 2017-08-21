#ifndef ALLSCALE_HARDWARE_RECONF_HPP
#define ALLSCALE_HARDWARE_RECONF_HPP

#include <vector>
#include <string>
#include <thread>

#include <hwloc.h>
#include <cpufreq.h>

#include <hpx/lcos/local/spinlock.hpp>

namespace allscale { namespace components { namespace util {


    ////////////////////////////////////////////////////////////////////////////////////
    ///  ... A class that provides a wrapper for cpufreq APIs and utility functions 
    ///      to read sysfs values ...
    ////////////////////////////////////////////////////////////////////////////////////
    struct hardware_reconf 
    {
        typedef hpx::lcos::local::spinlock mutex_type;

        struct hw_topology
        {
            int num_physical_cores;
            int num_logical_cores;
            int num_hw_threads;
        };


        /// \brief This function reads available CPU frequencies in KHz and returns them
        /// in a vector
        ///
        /// \param cpu          [in] CPU number
        ///
        /// \returns            Vector of available frequencies in KHz
        static std::vector<unsigned long> get_frequencies(unsigned int cpu);

        /// \brief This function reads available CPU governors and returns them
        /// in a vector of strings
        ///
        /// \param cpu          [in] CPU number
        /// 
        /// \returns            Vector of available CPU governors
        static std::vector<std::string> get_governors(unsigned int cpu);


        /// \brief This function sets/changes the frequency of the given CPU
        ///
        /// \param cpu                    [in] CPU number
        /// \param target_frequency       [in] CPU frequency
        ///
        /// \returns            Returns zero if successful, otherwise a minus return code
        ///
        /// \note               This function requires a root/sudo access and
        ///                     only works if userspace governor can be used and no external
        ///                     interference (other calls to this function or to set/modify_policy) 
        ///                     occurs. Also does not work on ->range() cpufreq drivers. See
        ///                     /usr/include/cpufreq.h file for more info about cpufreq APIs.
        static int set_frequency(unsigned int cpu, unsigned long target_frequency);

        /// \brief This function sets/changes the governor and frequency of the given CPU.
        ///
        /// \param cpu          [in] CPU number
        /// \param policy       [in] CPU policy
        ///
        /// \returns            Returns zero if successful, otherwise a minus return code
        ///
        /// \note               This function requires a root/sudo access
        static int set_freq_policy(unsigned int cpu, cpufreq_policy policy);

        /// \brief This function returns current CPU frequency seen by the kernel
        ///        It does not require sudo access.
        /// 
        /// \param cpu          [in] CPU number
        ///
        /// \returns            Zero on failure, else frequency in kHz.
        static unsigned long get_kernel_freq(unsigned int cpu);

        /// \brief This function returns current CPU frequency seen by the hardware.
        ///        It requires sudo access.
        /// 
        /// \param cpu          [in] CPU number
        ///
        /// \returns            Zero on failure, else frequency in kHz.
        static unsigned long get_hardware_freq(unsigned int cpu);


        /// \brief This function returns CPU transition latency
        ///
        /// \param cpu          [in] CPU number
        ///
        /// \returns            0 on failure, else transition latency in 10^(-9) s = nanoseconds
        static unsigned long get_cpu_transition_latency(unsigned int cpu);

        
        /// \brief This function changes frequencies of the given number of cores.
        ///
        /// \param num_cpus     [in] number of cpus that needs to be changed
        /// \param target_frequency   [in] target frequency that is going to be assigned
        ///
        /// \note               This function works in best effort way, i.e. it will try to
        ///                     change frequencies up to the given number of cpu cores. If
        ///                     for some reason it fails after some number, it won't reset
        ///                     frequencies of the previously affected cpus.
        static void set_frequencies_bulk(unsigned int num_cpus, unsigned long target_frequency);

        /// \brief This function reads system energy from sysfs on POWER8/+ machines and             
        ///        returns cumulative energy.
        ///
        /// \param sysfs_file   [in] The absolute path of sysfs file that provides energy readings.
        ///
        /// \returns            This function reads system energy from sysfs on POWER8/+ machines.
        ///                     If the file does not exist, i.e. on architectures other than ppc,
        ///                     or if there is parsing error it will return zero.
        ///
        /// \note               This function may be removed and moved into the monitoring component
        ///                     in future. 
        static unsigned long long read_system_energy(const std::string &sysfs_file = "/sys/devices/system/cpu/occ_sensors/system/system-energy");


        /// \brief This function reads number of physical, logical cores, and hardware threads on a node.
        ///
        /// \returns            Returns number of physical, logical cores, and hardware threads on a node.
        static hw_topology read_hw_topology();

        private:
             static mutex_type freq_mtx_;

    };
}}}
#endif

