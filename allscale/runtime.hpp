
#ifndef ALLSCALE_RUNTIME_HPP
#define ALLSCALE_RUNTIME_HPP

/**
 * The header defining the interface to be utilzed by AllScale runtime applications.
 * In particular, this is the interface utilized by the compiler when generating applications
 * targeting the AllScale runtime.
 */

#include <hpx/config.hpp>

#include <allscale/config.hpp>
#include <allscale/no_split.hpp>
#include <allscale/spawn.hpp>
#include <allscale/work_item_description.hpp>

#include <allscale/monitor.hpp>
#include <allscale/resilience.hpp>
#include <allscale/do_serialization.hpp>
#include <allscale/no_serialization.hpp>
#include <allscale/data_item_reference.hpp>
#include <allscale/data_item_requirement.hpp>
#include <allscale/data_item_manager.hpp>
#include <allscale/data_requirements_check.hpp>

#include <hpx/hpx_init.hpp>
#include <hpx/runtime/config_entry.hpp>
#include <hpx/util/find_prefix.hpp>
#include <hpx/util/invoke.hpp>
#include <hpx/util/invoke_fused.hpp>
#include <hpx/util/unused.hpp>
#include <hpx/util/unwrap.hpp>
#include <hpx/lcos/local/dataflow.hpp>
#include <hpx/lcos/barrier.hpp>

#include <boost/program_options.hpp>
#include <boost/program_options/parsers.hpp>

namespace allscale {
namespace runtime {

// Adapting names for compiler
using AccessMode = allscale::access_mode;

template <typename T>
using DataItemReference = allscale::data_item_reference<T>;
template <typename T>
using DataItemRequirement = allscale::data_item_requirement<T>;

template<typename DataItemType>
data_item_requirement<DataItemType> createDataItemRequirement
(   const data_item_reference<DataItemType>& ref,
    const typename DataItemType::region_type& region,
    const access_mode& mode
)
{
    return allscale::createDataItemRequirement(ref, region, mode);
}

namespace DataItemManager {
    using namespace ::allscale::data_item_manager;
}

using unused_type = hpx::util::unused_type;

template<typename MainWorkItem>
inline int spawn_main(int(*)(hpx::util::tuple<int, char**> const&), int argc, char** argv)
{
    return MainWorkItem::process_variant::execute(hpx::util::make_tuple(argc, argv));
}

template<typename MainWorkItem>
inline int spawn_main(int(*)(hpx::util::tuple<> const&), int argc, char** argv)
{
    return MainWorkItem::process_variant::execute(hpx::util::make_tuple(argc, argv));
}

template<typename MainWorkItem>
inline int spawn_main(treeture<int>(*)(hpx::util::tuple<int, char**> const&), int argc, char** argv)
{
    return MainWorkItem::process_variant::execute(hpx::util::make_tuple(argc, argv)).get_result();
}

template<typename MainWorkItem>
inline int spawn_main(treeture<int>(*)(hpx::util::tuple<> const&), int, char**)
{
    return MainWorkItem::process_variant::execute(hpx::util::make_tuple()).get_result();
}

/**
 * A wrapper for the main function of an applicaiton handling the startup and
 * shutdown procedure as well as lunching the first work item.
 */
template<typename MainWorkItem>
int allscale_main(boost::program_options::variables_map &)
{
    // include monitoring support
    auto mon = allscale::monitor::run(hpx::get_locality_id());
    // include resilience support
    auto resi = allscale::resilience::run(hpx::get_locality_id());
    // start allscale scheduler ...
    auto sched = allscale::scheduler::run(hpx::get_locality_id());

    hpx::lcos::barrier::synchronize();

#if defined(ALLSCALE_DEBUG_DIM)
            std::stringstream filename;
            filename << "data_item." << hpx::get_locality_id() << ".log";
            std::ofstream os(filename.str(), std::ios_base::trunc);
            os.close();
#endif

    // trigger first work item on first node
    int res = EXIT_SUCCESS;
    if (hpx::get_locality_id() == 0) {
        std::string cmdline(hpx::get_config_entry("hpx.reconstructed_cmd_line", ""));

        using namespace boost::program_options;
#if defined(HPX_WINDOWS)
        std::vector<std::string> args = split_winmain(cmdline);
#else
        std::vector<std::string> args = split_unix(cmdline);
#endif

        // Copy all arguments which are not hpx related to a temporary array
        std::vector<char*> argv(args.size()+1);
        std::size_t argcount = 0;
        for (auto & arg : args)
        {
            if (0 != arg.find("--hpx:")) {
                argv[argcount++] = const_cast<char*>(arg.data());
            }
            else if (6 == arg.find("positional", 6)) {
                std::string::size_type p = arg.find_first_of('=');
                if (p != std::string::npos) {
                    arg = arg.substr(p+1);
                    argv[argcount++] = const_cast<char*>(arg.data());
                }
            }
        }

        // add a single nullptr in the end as some application rely on that
        argv[argcount] = nullptr;

        res = spawn_main<MainWorkItem>(
            &MainWorkItem::process_variant::execute, static_cast<int>(argcount), argv.data());
        allscale::scheduler::stop();
        allscale::resilience::stop();
        allscale::monitor::stop();

        hpx::finalize();
    }

    // also deliver data item access checks
    auto error_free = allscale::runtime::summarize_access_checks();
    if (!error_free && res == EXIT_SUCCESS) {
    	res = EXIT_FAILURE;	// mark as failed
    }

    // Force the optimizer to initialize the runtime...
    if (sched && mon && resi)
        // return result (only id == 0 will actually)
        return res;
    else return EXIT_FAILURE;
}

/**
 * A wrapper for the main function of an applicaiton handling the startup and
 * shutdown procedure as well as lunching the first work item.
 */
template<typename MainWorkItem>
int main_wrapper(int argc = 0, char **argv = nullptr) {
    typedef int(*hpx_main_type)(boost::program_options::variables_map &);
    hpx_main_type f = &allscale_main<MainWorkItem>;
    boost::program_options::options_description desc("Usage: [options]");
    hpx::util::set_hpx_prefix(HPX_PREFIX);
    allscale::scheduler::setup_resources(f, desc, argc, argv);
    return hpx::init();
}

dependencies after() {
	return dependencies{};
}

template<typename ... TaskRefs>
typename std::enable_if<(sizeof...(TaskRefs) > 0), dependencies>::type
after(TaskRefs&& ... task_refs) {
	return dependencies(hpx::when_all(task_refs.get_future()...));
}

// ---- a prec operation wrapper ----

template<typename R, typename Closure, typename F>
struct prec_operation
{
    typedef typename std::decay<Closure>::type closure_type;
    typedef typename std::decay<F>::type f_type;

    closure_type closure;
    f_type impl;

    template <typename A>
    treeture<R> operator()(A&& a) {
        return (*this)(dependencies{}, std::forward<A>(a));
    }

    template <typename A>
    treeture<R> operator()(dependencies const& d, A&& a) {
        return hpx::util::invoke(impl, d,
            hpx::util::tuple_cat(hpx::util::make_tuple(std::forward<A>(a)),
                std::move(closure)));
    }

    template <typename A>
    treeture<R> operator()(dependencies && d, A&& a) {
        return hpx::util::invoke(impl, std::move(d),
            hpx::util::tuple_cat(hpx::util::make_tuple(std::forward<A>(a)),
                std::move(closure)));
    }

};

template<typename A, typename R, typename Closure, typename F>
prec_operation<R,Closure, F> make_prec_operation(Closure && closure, F&& impl)
{
    return prec_operation<R, Closure, F>{
        std::forward<Closure>(closure),std::forward<F>(impl)};
}

// ---- a generic treeture combine operator ----
template<typename A, typename B, typename Op, typename R = std::result_of_t<Op(A,B)>>
allscale::treeture<R> treeture_combine(
    const dependencies& dep,
    allscale::treeture<A>&& a,
    allscale::treeture<B>&& b,
    Op&& op)
{
    allscale::treeture<R> res;
    if (dep.dep_.valid() && !dep.dep_.is_ready())
    {
        res = hpx::dataflow(hpx::launch::sync, hpx::util::unwrapping(std::forward<Op>(op)),
            a.get_future(), b.get_future(), dep.dep_);
    }
    else
    {
        res = hpx::dataflow(hpx::launch::sync, hpx::util::unwrapping(std::forward<Op>(op)),
            a.get_future(), b.get_future());
    }

    res.set_children(std::move(a), std::move(b));

    return res;
}

template<typename A, typename B, typename Op, typename R = std::result_of_t<Op(A,B)>>
allscale::treeture<R> treeture_combine(
    allscale::treeture<A>&& a,
    allscale::treeture<B>&& b,
    Op&& op)
{
    allscale::treeture<R> res(
        hpx::dataflow(hpx::launch::sync, hpx::util::unwrapping(std::forward<Op>(op)),
        a.get_future(), b.get_future()));

    res.set_children(std::move(a), std::move(b));

    return res;
}

allscale::treeture<void> treeture_combine(
    const dependencies& dep,
    allscale::treeture<void>&& a,
    allscale::treeture<void>&& b)
{
    allscale::treeture<void> res;

    if (dep.dep_.valid() && !dep.dep_.is_ready())
    {
        res = hpx::when_all(dep.dep_,
            a.get_future(), b.get_future());
    }
    else
    {
        res = hpx::when_all(
            a.get_future(), b.get_future());
    }

    res.set_children(std::move(a), std::move(b));

    return res;
}

allscale::treeture<void> treeture_combine(
    allscale::treeture<void>&& a,
    allscale::treeture<void>&& b)
{
    allscale::treeture<void> res(hpx::when_all(a.get_future(), b.get_future()));

    res.set_children(std::move(a), std::move(b));

    return res;
}


template<typename A, typename B>
allscale::treeture<void> treeture_parallel(const dependencies& dep,
    allscale::treeture<A>&& a, allscale::treeture<B>&& b) {
    return treeture_combine(dep, std::move(a), std::move(b));
}

template<typename A, typename B>
allscale::treeture<void> treeture_parallel(allscale::treeture<A>&& a, allscale::treeture<B>&& b) {
    return treeture_combine(std::move(a), std::move(b));
}


template<typename A, typename B>
allscale::treeture<void> treeture_sequential(const dependencies&, allscale::treeture<A>&& a, allscale::treeture<B>&& b) {
	assert(false && "Not implemented!");
	return treeture_parallel(std::move(a),std::move(b));
}

template<typename A, typename B>
allscale::treeture<void> treeture_sequential(allscale::treeture<A>&& a, allscale::treeture<B>&& b) {
	return treeture_sequential(dependencies{hpx::future<void>()}, std::move(a),std::move(b));
}


} // end namespace runtime
} // end namespace allscale

#endif
