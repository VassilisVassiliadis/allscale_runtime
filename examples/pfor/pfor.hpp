
#include <allscale/no_split.hpp>
#include <allscale/no_serialization.hpp>
#include <allscale/do_serialization.hpp>
#include <allscale/treeture.hpp>
#include <allscale/spawn.hpp>
#include <allscale/scheduler.hpp>
#include <allscale/work_item_description.hpp>

//#include <allscale/runtime.hpp>

#include <unistd.h>


// a handle for loop iterations
class pfor_loop_handle {

    allscale::treeture<void> treeture;

public:

    pfor_loop_handle() : treeture(allscale::make_ready_treeture()) {}

    pfor_loop_handle(allscale::treeture<void>&& treeture)
        : treeture(std::move(treeture)) {}

    pfor_loop_handle(const pfor_loop_handle&) = delete;
    pfor_loop_handle(pfor_loop_handle&&) = default;

    ~pfor_loop_handle() {
        if (treeture.valid()) treeture.wait();
    }

    pfor_loop_handle& operator=(const pfor_loop_handle&) = delete;
    pfor_loop_handle& operator=(pfor_loop_handle&& other) = default;

    const allscale::treeture<void>& get_treeture() const {
        return treeture;
    }
};

template <typename Body>
struct pfor_can_split
{
    template <typename Body_, typename Closure>
    static bool call_impl(Closure&& closure, decltype(&Body_::can_split))
    {
        auto begin = hpx::util::get<0>(closure);
        auto end   = hpx::util::get<1>(closure);

        return Body::can_split(begin, end);
    }

    template <typename Body_, typename Closure>
    static bool call_impl(Closure&& closure, ...)
    {
        auto begin = hpx::util::get<0>(closure);
        auto end   = hpx::util::get<1>(closure);

        // this decision is arbitrary and should be fixed by the Body
        // implementations to give a good estimate for granularity
        return end - begin > 10000;
    }

    template <typename Closure>
    static bool call(Closure&& closure)
    {
        return call_impl<Body>(std::forward<Closure>(closure), nullptr);
    }
};


////////////////////////////////////////////////////////////////////////////////
// PFor Work Item:
//  - Result: std::int64_t
//  - Closure: [std::int64_t,std::int64_t,hpx::util::tuple<ExtraParams...>]
//  - SplitVariant: split range in half, compute halfs
//  - ProcessVariant: cumpute the given range


template<typename Body, typename ... ExtraParams>
struct pfor_split_variant;

template<typename Body, typename ... ExtraParams>
struct pfor_process_variant;

struct pfor_name
{
    static const char *name()
    {
        return "pfor";
    }
};

template<typename Body, typename ... ExtraParams>
using pfor_work =
    allscale::work_item_description<
        void,
        pfor_name,
        allscale::no_serialization,
        pfor_split_variant<Body,ExtraParams...>,
        pfor_process_variant<Body,ExtraParams...>,
        pfor_can_split<Body>
    >;

template<typename Body, typename ... ExtraParams>
struct pfor_split_variant
{
    static constexpr bool valid = true;
    using result_type = void;


    // It spawns two new tasks, processing each half the iterations
    template <typename Closure>
    static hpx::future<void> execute(Closure const& closure)
    {
        auto begin = hpx::util::get<0>(closure);
        auto end   = hpx::util::get<1>(closure);
        auto extra = hpx::util::get<2>(closure);

        // check whether there are iterations left
        if (begin >= end) return hpx::make_ready_future();

        // compute the middle
        auto mid = begin + (end - begin) / 2;

        // spawn two new sub-tasks
        return hpx::when_all(
            allscale::spawn<pfor_work<Body,ExtraParams...>>(begin,mid,extra).get_future(),
            allscale::spawn<pfor_work<Body,ExtraParams...>>(mid,end,extra).get_future()
        );
    }
};

template<typename Body, typename ... ExtraParams>
struct pfor_process_variant
{
    static constexpr bool valid = true;
    using result_type = void;

    static const char* name()
    {
        return "pfor_process_variant";
    }

    // Execute for serial
    template <typename Closure>
    static hpx::future<void> execute(Closure const& closure)
    {
        auto begin = hpx::util::get<0>(closure);
        auto end   = hpx::util::get<1>(closure);
        auto extra = hpx::util::get<2>(closure);

        // get a body instance
        Body body;

        // do some computation
        for(auto i = begin; i<end; ++i) {
            body(i,extra);
        }

        // done
        return hpx::make_ready_future();
    }
};


template<typename Body, typename ... ExtraParams>
pfor_loop_handle pfor(int a, int b, const ExtraParams& ... params) {
    return allscale::spawn_first<pfor_work<Body,ExtraParams...>>(a,b,hpx::util::make_tuple(params...));
}

////////////////////////////////////////////////////////////////////////////////
// PFor Work Item with fine-grained synchronization (simple)
//  - Result: std::int64_t
//  - Closure: [std::int64_t,std::int64_t,hpx::util::tuple<ExtraParams...>]
//  - SplitVariant: split range in half, compute halfs
//  - ProcessVariant: cumpute the given range

template<typename Body, typename ... ExtraParams>
struct pfor_neighbor_sync_split_variant;

template<typename Body, typename ... ExtraParams>
struct pfor_neighbor_sync_process_variant;

struct pfor_neighbor_sync_name
{
    static const char *name()
    {
        return "pfor_neighbor_sync";
    }
};

template<typename Body, typename ... ExtraParams>
using pfor_neighbor_sync_work =
    allscale::work_item_description<
        void,
        pfor_name,
        allscale::do_serialization,
        pfor_neighbor_sync_split_variant<Body,ExtraParams...>,
        pfor_neighbor_sync_process_variant<Body,ExtraParams...>,
        pfor_can_split<Body>
    >;

template<typename Body, typename ... ExtraParams>
struct pfor_neighbor_sync_split_variant
{
    static constexpr bool valid = true;
    using result_type = void;


    // It spawns two new tasks, processing each half the iterations
    template <typename Closure>
    static hpx::future<void> execute(Closure const& closure)
    {
        // extract parameters
        auto begin = hpx::util::get<0>(closure);
        auto end   = hpx::util::get<1>(closure);
        auto extra = hpx::util::get<2>(closure);

        // extract the dependencies
        auto deps  = std::move(hpx::util::get<3>(closure));

        // check whether there are iterations left
        if (begin >= end) return hpx::make_ready_future();

        // compute the middle
        auto mid = begin + (end - begin) / 2;
        // refine dependencies
        auto& dl = hpx::util::get<0>(deps);
        auto& dc = hpx::util::get<1>(deps);
        auto& dr = hpx::util::get<2>(deps);

        // refine dependencies
        auto dlr = dl.get_right_child();
        auto dcl = dc.get_left_child();
        auto dcr = dc.get_right_child();
        auto drl = dr.get_left_child();

        // spawn two new sub-tasks
        auto left = allscale::spawn<pfor_neighbor_sync_work<Body,ExtraParams...>>(begin, mid, extra, hpx::util::make_tuple(dlr,dcl,dcr));
        auto right = allscale::spawn<pfor_neighbor_sync_work<Body,ExtraParams...>>(mid,   end, extra, hpx::util::make_tuple(dcl,dcr,drl));

        return
            hpx::when_all(
                left.get_future(), right.get_future());
    }
};

template<typename Body, typename ... ExtraParams>
struct pfor_neighbor_sync_process_variant
{
    static constexpr bool valid = true;
    using result_type = void;

    static const char* name()
    {
        return "pfor_process_variant";
    }

    // Execute for serial
    template <typename Closure>
    static hpx::future<void> execute(Closure const& closure)
    {
        // extract parameters
        auto begin = hpx::util::get<0>(closure);
        auto end   = hpx::util::get<1>(closure);
        auto extra = hpx::util::get<2>(closure);
        // extract the dependencies
        auto deps  = std::move(hpx::util::get<3>(closure));

        // refine dependencies
        auto& dl = hpx::util::get<0>(deps);
        auto& dc = hpx::util::get<1>(deps);
        auto& dr = hpx::util::get<2>(deps);


        return
            hpx::dataflow(
                hpx::util::unwrapped(
                    [begin, end, extra]()
                    {
                        // get a body instance
                        Body body;

                        // do some computation
                        for(auto i = begin; i<end; i++) {

                            body(i,extra);
                        }
                    }
                )
        		,
                dl.get_future(),
                dc.get_future(),
                dr.get_future()
            );
    }
};


template<typename Body, typename ... ExtraParams>
pfor_loop_handle pfor_neighbor_sync(const pfor_loop_handle& loop, int a, int b, const ExtraParams& ... params) {
    allscale::treeture<void> done = allscale::make_ready_treeture();
    return allscale::spawn_first<pfor_neighbor_sync_work<Body,ExtraParams...>>(
        a,b,                                                    // the range
        hpx::util::make_tuple(params...),                       // the body parameters
        hpx::util::make_tuple(done,loop.get_treeture(),done)     // initial dependencies
    );
}





