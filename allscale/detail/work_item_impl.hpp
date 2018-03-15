
#ifndef ALLSCALE_DETAIL_WORK_ITEM_IMPL_HPP
#define ALLSCALE_DETAIL_WORK_ITEM_IMPL_HPP

#include <allscale/treeture.hpp>
#include <allscale/monitor.hpp>
#include <allscale/data_item_manager.hpp>
#include <allscale/data_item_manager/acquire.hpp>
#include <allscale/data_item_manager/acquire_rank.hpp>
#include <allscale/data_item_manager/locate.hpp>
#include <allscale/data_item_requirement.hpp>
#include <allscale/detail/work_item_impl_base.hpp>

#include <hpx/include/serialization.hpp>
#include <hpx/include/parallel_execution.hpp>
#include <hpx/util/assert.hpp>
#include <hpx/util/annotated_function.hpp>
#include <hpx/util/tuple.hpp>
#include <hpx/lcos/when_all.hpp>

#include <memory>
#include <utility>

namespace allscale { namespace detail {
	struct set_id {
		set_id(this_work_item::id& id)
          :	old_id_(this_work_item::get_id_ptr()) {
			this_work_item::set_id(id);
		}

		~set_id() {
            if (old_id_)
                this_work_item::set_id(*old_id_);
		}

		this_work_item::id* old_id_;
	};

    template <typename Archive, typename T>
    auto has_serialize(hpx::traits::detail::wrap_int)
      -> std::false_type;

    template <typename Archive, typename T>
    auto has_serialize(int)
      -> decltype(serialize(std::declval<Archive&>(), std::declval<T&>(), 0), std::true_type{});

    template <typename T>
    struct is_serializable
      : std::integral_constant<bool,
            decltype(has_serialize<hpx::serialization::input_archive, T>(0))::value &&
            decltype(has_serialize<hpx::serialization::output_archive, T>(0))::value &&
            std::is_default_constructible<T>::value &&
            !std::is_pointer<typename std::decay<T>::type>::value &&
            !std::is_reference<T>::value
        >
    {};

    template <typename T>
    struct is_closure_serializable;

    template <>
    struct is_closure_serializable<hpx::util::tuple<>> : std::true_type {};

    template <typename...Ts>
    struct is_closure_serializable<hpx::util::tuple<Ts...>>
      : hpx::util::detail::all_of<is_serializable<Ts>...>
    {};

    template <typename Leases, std::size_t... Is>
    inline void release(hpx::util::detail::pack_c<std::size_t, Is...>, Leases const& leases)
    {
        int sequencer[] = {
            (allscale::data_item_manager::release(hpx::util::get<Is>(leases)), 0)...
        };
        (void)sequencer;
    }

    inline void release(hpx::util::detail::pack_c<std::size_t>, hpx::util::tuple<> const&)
    {
    }

    template<typename T, typename SharedState>
    inline void set_treeture(treeture<T>& t, SharedState& s) {
        t.set_value(std::move(*s->get_result()));
    }

    template<typename SharedState>
    inline void set_treeture(treeture<void>& t, SharedState& s) {
    // 	f.get(); // exception propagation
        s->get_result_void();
        t.set_value(hpx::util::unused_type{});
    }

    template<typename WorkItemDescription, typename Closure>
    struct work_item_impl
      : detail::work_item_impl_base
    {
        using result_type = typename WorkItemDescription::result_type;
        using closure_type = Closure;


        static constexpr bool is_serializable = WorkItemDescription::ser_variant::activated; /*&&
            is_closure_serializable<Closure>::value;*/

        // This ctor is called during serialization...
        work_item_impl(this_work_item::id const& id, treeture<result_type>&& tres, closure_type&& closure)
          : work_item_impl_base(id)
          , tres_(std::move(tres))
          , closure_(std::move(closure))
        {}

        template<typename ...Ts>
        work_item_impl(treeture<result_type> tres, Ts&&... vs)
         : tres_(std::move(tres)), closure_(std::forward<Ts>(vs)...)
        {
            HPX_ASSERT(tres_.valid());
        }

        work_item_impl(this_work_item::id const& id, hpx::shared_future<void> dep, treeture<result_type>&& tres, closure_type&& closure)
          : work_item_impl_base(id)
          , tres_(std::move(tres))
          , closure_(std::move(closure))
          , dep_(std::move(dep))
        {}

        template<typename ...Ts>
        work_item_impl(hpx::shared_future<void> dep, treeture<result_type> tres, Ts&&... vs)
         : tres_(std::move(tres)), closure_(std::forward<Ts>(vs)...), dep_(std::move(dep))
        {
            HPX_ASSERT(tres_.valid());
        }

        std::shared_ptr<work_item_impl> shared_this()
        {
            return std::static_pointer_cast<work_item_impl>(
                this->shared_from_this());
        }

        const char* name() const final {
            return WorkItemDescription::name();
        }

        treeture<void> get_treeture() final
        {
            return tres_;
        }

        bool valid() final
        {
            return tres_.valid() && bool(this->shared_from_this());
        }

        void on_ready(hpx::util::unique_function_nonser<void()> f) final
        {
            typename hpx::traits::detail::shared_state_ptr_for<treeture<result_type>>::type const& state
                = hpx::traits::future_access<treeture<result_type>>::get_shared_state(tres_);

            state->set_on_completed(std::move(f));
        }

        bool can_split() const final {
            return WorkItemDescription::can_split_variant::call(closure_) &&
                WorkItemDescription::split_variant::valid;
        }

        template <typename Variant>
        auto get_requirements(decltype(&Variant::template get_requirements<Closure>)) const
         -> decltype(Variant::get_requirements(std::declval<Closure const&>()))
        {
            return Variant::get_requirements(closure_);
        }

        template <typename Variant>
        auto get_requirements(decltype(&Variant::get_requirements)) const
         -> decltype(Variant::get_requirements(std::declval<Closure const&>()))
        {
            return Variant::get_requirements(closure_);
        }

        template <typename Variant>
        hpx::util::tuple<> get_requirements(...) const
        {
            return hpx::util::tuple<>();
        }

        template <typename Future, typename Leases>
        typename std::enable_if<
            hpx::traits::is_future<Future>::value ||
            traits::is_treeture<Future>::value
        >::type
        finalize(
            std::shared_ptr<work_item_impl> this_, Future && work_res, Leases leases)
        {
            monitor::signal(monitor::work_item_execution_finished,
                work_item(this_));

            typedef typename std::decay<Future>::type work_res_type;
            typedef typename hpx::traits::detail::shared_state_ptr_for<work_res_type>::type
                shared_state_type;
            shared_state_type state
                = hpx::traits::future_access<work_res_type>::get_shared_state(work_res);

            state->set_on_completed(
                hpx::util::deferred_call([](auto this_, auto state, Leases leases)
                {
                    typename hpx::util::detail::make_index_pack<
                        hpx::util::tuple_size<Leases>::type::value>::type pack;
                    release(pack, leases);

                    set_id si(this_->id_);
                    detail::set_treeture(this_->tres_ , state);
                    monitor::signal(monitor::work_item_result_propagated,
                        work_item(std::move(this_)));
                },
                std::move(this_), state, std::move(leases)));
        }

        template <typename Future, typename Leases>
        typename std::enable_if<
            !hpx::traits::is_future<Future>::value &&
            !traits::is_treeture<Future>::value
        >::type
        finalize(
            std::shared_ptr<work_item_impl> this_, Future && work_res, Leases leases)
        {
            monitor::signal(monitor::work_item_execution_finished,
                work_item(this_));

            typename hpx::util::detail::make_index_pack<
                hpx::util::tuple_size<Leases>::type::value>::type pack;
            release(pack, leases);

            set_id si(this_->id_);
            tres_.set_value(std::forward<Future>(work_res));
            monitor::signal(monitor::work_item_result_propagated,
                work_item(std::move(this_)));
        }

        template <typename Closure_, typename Leases>
        typename std::enable_if<
            !std::is_same<
                decltype(WorkItemDescription::process_variant::execute(std::declval<Closure_ const&>())),
                void
            >::value
        >::type
		do_process(Leases leases)
        {
            std::shared_ptr < work_item_impl > this_(shared_this());
            monitor::signal(monitor::work_item_execution_started,
                work_item(this_));
            set_id si(this->id_);

            auto work_res =
                WorkItemDescription::process_variant::execute(closure_);

            finalize(std::move(this_), std::move(work_res), std::move(leases));
		}

        template <typename Closure_, typename Leases>
        typename std::enable_if<
            std::is_same<
                decltype(WorkItemDescription::process_variant::execute(std::declval<Closure_ const&>())),
                void
            >::value
        >::type
		do_process(Leases leases)
        {
            std::shared_ptr < work_item_impl > this_(shared_this());
            monitor::signal(monitor::work_item_execution_started,
                work_item(this_));
            set_id si(this->id_);

            WorkItemDescription::process_variant::execute(closure_);

            finalize(std::move(this_), hpx::util::unused_type(), std::move(leases));
		}

        template <typename T>
        void set_children(treeture<T> const& tre)
        {
            if (tre.valid())
                tres_.set_children(tre.get_left_child(), tre.get_right_child());
        }

        template <typename T>
        void set_children(T const&)
        {
            // null-op as fallback...
        }

        template<typename Leases>
		void do_split(Leases leases)
        {
            std::shared_ptr < work_item_impl > this_(shared_this());
            monitor::signal(monitor::work_item_execution_started, work_item(this_));

            set_id si(this->id_);

            auto work_res =
                WorkItemDescription::split_variant::execute(closure_);
            set_children(work_res);

            finalize(std::move(this_), std::move(work_res), std::move(leases));
        }

        template <typename ProcessVariant, typename Leases>
        void get_deps(executor_type& exec, Leases leases,
            decltype(&ProcessVariant::template deps<Closure>))
        {
            typedef hpx::shared_future<void> deps_type;

            deps_type deps;
            if (dep_.valid() && !dep_.is_ready())
            {
                deps = hpx::when_all(ProcessVariant::deps(closure_), dep_).share();
            }
            else
            {
                deps = ProcessVariant::deps(closure_);
            }

            typename hpx::traits::detail::shared_state_ptr_for<deps_type>::type state
                = hpx::traits::future_access<deps_type>::get_shared_state(deps);

            auto this_ = shared_this();
            state->set_on_completed(
                [&exec, state = std::move(state), this_ = std::move(this_), leases = std::move(leases)]() mutable
                {
                    void (work_item_impl::*f)(
                            Leases
                    ) = &work_item_impl::do_process<Closure>;

                    hpx::parallel::execution::post(
                        exec, hpx::util::annotated_function(
                            hpx::util::deferred_call(f, std::move(this_), std::move(leases)),
                            "allscale::work_item::do_process"));
                });
        }

        template <typename ProcessVariant, typename Leases>
        void get_deps(executor_type& exec, Leases leases, ...)
        {
            auto this_ = shared_this();
            if (dep_.valid() && !dep_.is_ready())
            {
                typedef hpx::shared_future<void> deps_type;

                typename hpx::traits::detail::shared_state_ptr_for<deps_type>::type state
                    = hpx::traits::future_access<deps_type>::get_shared_state(dep_);
                state->set_on_completed(
                    [&exec, state = std::move(state), this_ = std::move(this_), leases = std::move(leases)]() mutable
                    {
                        void (work_item_impl::*f)(
                                Leases
                        ) = &work_item_impl::do_process<Closure>;

                        hpx::parallel::execution::post(
                            exec, hpx::util::annotated_function(
                                hpx::util::deferred_call(f, std::move(this_), std::move(leases)),
                                "allscale::work_item::do_process"));
                    }
                );
            }
            else
            {
                hpx::util::annotate_function("allscale::work_item::do_process_sync");
                do_process<Closure>(std::move(leases));
            }
        }

        hpx::future<std::size_t> process(executor_type& exec, hpx::util::tuple<>&&)
        {
            hpx::util::annotate_function("allscale::work_item::process");
            get_deps<typename WorkItemDescription::process_variant>(
                exec, hpx::util::tuple<>(), nullptr);
            return hpx::make_ready_future(std::size_t(-2));
        }

        template <typename Reqs>
        hpx::future<std::size_t> process(executor_type& exec, Reqs&& reqs)
        {
            typedef
                typename hpx::util::decay<decltype(hpx::util::unwrap(std::forward<Reqs>(reqs)))>::type
                reqs_type;
            auto this_ = shared_this();
            return hpx::dataflow(exec,
                hpx::util::annotated_function(hpx::util::unwrapping([this_ = std::move(this_), &exec](reqs_type reqs) mutable
                {
                    this_->template get_deps<typename WorkItemDescription::process_variant>(
                        exec, std::move(reqs), nullptr);
                    return std::size_t(-2);
                }), "allscale::work_item::process::reqs_cont_sync")
              , std::forward<Reqs>(reqs));
        }

        hpx::future<std::size_t> process(executor_type& exec) final
        {
            HPX_ASSERT(valid());

            auto this_ = shared_this();
            auto reqs = detail::merge_data_item_reqs(
                get_requirements<typename WorkItemDescription::process_variant>(nullptr)
            );

#if defined(ALLSCALE_DEBUG_DIM)
            std::string s = this->name();
            s += '(';
            s += id_.name();
            s += ").process";
            data_item_manager::log_req(s, reqs);
#endif
            return hpx::dataflow(hpx::launch::sync,
                hpx::util::annotated_function([reqs, this_ = std::move(this_), &exec](auto locate_future)
                {
                    auto infos = hpx::util::unwrap(locate_future);
                    std::size_t rank = data_item_manager::acquire_rank(reqs, infos);
                    // Write regions are on different localities, need split
                    if (rank == std::size_t(-1))
                    {
                        if (this_->can_split())
                        {
                            return hpx::make_ready_future(std::size_t(-1));
                        }
                        else
                        {
                            std::cerr << "Requesting split for " << this_->name()
                                << ", but can not split further!\n";
                            data_item_manager::print_location_info(reqs, infos);
                            std::abort();
                        }
                    }
                    // Route to different locality
                    if (rank != std::size_t(-2) && rank != hpx::get_locality_id())
                    {
                        return hpx::make_ready_future(rank);
                    }
                    HPX_ASSERT(rank == std::size_t(-2) || rank == hpx::get_locality_id());
                    return this_->process(exec, data_item_manager::acquire(reqs, infos));
                }, "allscale::work_item::process::locate_continuation"),
#if defined(ALLSCALE_DEBUG_DIM)
                data_item_manager::locate(std::move(s), reqs)
#else
                data_item_manager::locate(reqs)
#endif
            );
        }

        void split_impl(hpx::util::tuple<>&& reqs)
        {
            do_split(std::move(reqs));
        }

        template <typename...Ts>
        void split_impl(hpx::util::tuple<Ts...>&& reqs)
        {
            typedef
                typename hpx::util::decay<decltype(hpx::util::unwrap(std::move(reqs)))>::type
                reqs_type;
            auto this_ = shared_this();
            hpx::dataflow(//hpx::launch::sync,
                hpx::util::annotated_function(hpx::util::unwrapping([this_ = std::move(this_)](reqs_type reqs) mutable
                {
                    HPX_ASSERT(this_->valid());
                    this_->do_split(std::move(reqs));
                }), "allscale::work_item::reqs_cont_sync")
              , std::move(reqs));
        }

        template <typename WorkItemDescription_>
        typename std::enable_if<
            WorkItemDescription_::split_variant::valid,
            hpx::future<std::size_t>
        >::type split_impl()
        {
            auto this_ = shared_this();
            auto reqs = detail::merge_data_item_reqs(
                get_requirements<typename WorkItemDescription::split_variant>(nullptr)
            );
#if defined(ALLSCALE_DEBUG_DIM)
            std::string s = this->name();
            s += '(';
            s += id_.name();
            s += ").split";
            data_item_manager::log_req(s,
                detail::merge_data_item_reqs(
                    hpx::util::tuple_cat(
                        get_requirements<typename WorkItemDescription::split_variant>(nullptr),
                        get_requirements<typename WorkItemDescription::process_variant>(nullptr)
                    )
                )
            );
#endif
            return hpx::dataflow(hpx::launch::sync,
                hpx::util::annotated_function([reqs, this_ = std::move(this_)](auto locate_future)
                {
                    auto infos = hpx::util::unwrap(locate_future);
                    std::size_t rank = data_item_manager::acquire_rank(reqs, infos);
                    // Write regions are on different localities, need split
                    if (rank == std::size_t(-1))
                    {
                        return std::size_t(-1);
                    }
                    // Route to different locality
                    if (rank != std::size_t(-2) && rank != hpx::get_locality_id())
                    {
                        return rank;
                    }
                    HPX_ASSERT(rank == std::size_t(-2) || rank == hpx::get_locality_id());
                    this_->split_impl(data_item_manager::acquire(reqs, infos));
                    return std::size_t(-2);
                }, "allscale::work_item::split_impl::locate_cont"),
#if defined(ALLSCALE_DEBUG_DIM)
                data_item_manager::locate(std::move(s), reqs)
#else
                data_item_manager::locate(reqs)
#endif
            );
        }

        template <typename WorkItemDescription_>
        typename std::enable_if<
            !WorkItemDescription_::split_variant::valid,
            hpx::future<std::size_t>
        >::type split_impl()
        {
            throw std::logic_error(
                "Calling split on a work item without valid split variant");
            return hpx::make_ready_future(std::size_t(-1));
        }

        hpx::future<std::size_t> split() final
        {
            return split_impl<WorkItemDescription>();
        }

        bool enqueue_remote() const final
        {
            return is_serializable;
        }

        HPX_SERIALIZATION_POLYMORPHIC_TEMPLATE_SEMIINTRUSIVE(work_item_impl);

        treeture<result_type> tres_;
        closure_type closure_;
        hpx::shared_future<void> dep_;
    };

    template <typename Archive, typename WorkItemDescription, typename Closure>
    typename std::enable_if<
        work_item_impl<WorkItemDescription, Closure>::is_serializable
    >::type
    serialize(Archive& ar, work_item_impl<WorkItemDescription, Closure>& wi, unsigned)
    {
        ar & wi.id_;
        ar & wi.tres_;
        ar & wi.closure_;
        ar & wi.dep_;
    }

    template <typename Archive, typename WorkItemDescription, typename Closure>
    typename std::enable_if<
        !work_item_impl<WorkItemDescription, Closure>::is_serializable
    >::type
    serialize(Archive&, work_item_impl<WorkItemDescription, Closure>&, unsigned)
    {

        std::cerr << "Attempt to serialize non serializable work_item: " <<
            WorkItemDescription::name() << "\n";
        std::abort();
    }

    template <typename WorkItemDescription, typename Closure>
    typename std::enable_if<
        work_item_impl<WorkItemDescription, Closure>::is_serializable,
        work_item_impl<WorkItemDescription, Closure>*
    >::type
    load_work_item_impl(
        hpx::serialization::input_archive& ar,
        work_item_impl<WorkItemDescription, Closure>* /*unused*/)
    {
        typedef work_item_impl<WorkItemDescription, Closure> work_item_type;

        this_work_item::id id;
        treeture<typename work_item_type::result_type> tres;
        typename work_item_type::closure_type closure;
        hpx::shared_future<void> dep;

        ar & id;
        ar & tres;
        ar & closure;
        ar & dep;

        return new work_item_type(id, std::move(dep), std::move(tres), std::move(closure));
    }

    template <typename WorkItemDescription, typename Closure>
    typename std::enable_if<
        !work_item_impl<WorkItemDescription, Closure>::is_serializable,
        work_item_impl<WorkItemDescription, Closure>*
    >::type
    load_work_item_impl(
        hpx::serialization::input_archive&,
        work_item_impl<WorkItemDescription, Closure>* /*unused*/)
    {
        std::cerr << "Attempt to serialize non serializable work_item: " <<
            WorkItemDescription::name() << "\n";
        std::abort();
        return nullptr;
    }
}}

HPX_SERIALIZATION_WITH_CUSTOM_CONSTRUCTOR_TEMPLATE(
    (template <typename WorkItemDescription, typename Closure>),
    (allscale::detail::work_item_impl<WorkItemDescription, Closure>),
    allscale::detail::load_work_item_impl
);

HPX_TRAITS_NONINTRUSIVE_POLYMORPHIC_TEMPLATE(
    (template <typename WorkItemDescription, typename Closure>),
    (allscale::detail::work_item_impl<WorkItemDescription, Closure>));
HPX_SERIALIZATION_REGISTER_CLASS_NAME_TEMPLATE(
    (template <typename WorkItemDescription, typename Closure>),
    (allscale::detail::work_item_impl<WorkItemDescription, Closure>),
    WorkItemDescription::name());

template <typename WorkItemDescription, typename Closure>
hpx::serialization::detail::register_class<allscale::detail::work_item_impl<WorkItemDescription, Closure>>
allscale::detail::work_item_impl<WorkItemDescription, Closure>::hpx_register_class_instance;

#endif
