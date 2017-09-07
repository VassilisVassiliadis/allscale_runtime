#include <allscale/components/resilience.hpp>
#include <allscale/resilience.hpp>
#include <allscale/monitor.hpp>
#include <allscale/scheduler.hpp>
#include <allscale/work_item.hpp>

#include <hpx/include/thread_executors.hpp>
#include <hpx/util/detail/yield_k.hpp>
#include <hpx/util/asio_util.hpp>
#include <chrono>
#include <thread>

//#include <boost/asio.hpp>

using std::chrono::milliseconds;

namespace allscale { namespace components {

    resilience::resilience(std::uint64_t rank)
      : rank_(rank)
    {
    }

    void resilience::set_guard(hpx::id_type guard, uint64_t guard_rank) {
        guard_ = guard;
        guard_rank_ = guard_rank;
#ifdef DEBUG_
        std::cout << "My rank: " << rank_ << " and my new guard = " << guard_rank << "\n";
#endif
        std::string guard_ip_addr = hpx::async<get_ip_address_action>(guard_).get();
        delete guard_receiver_endpoint;
        guard_receiver_endpoint = new udp::endpoint(boost::asio::ip::address::from_string(guard_ip_addr), UDP_RECV_PORT+guard_rank_);
    }

    void resilience::kill_me() {
        raise(SIGKILL);
    }

    void resilience::failure_detection_loop_async() {
        if (resilience_disabled)
            return;

        // Previously:
        // hpx::apply(&resilience::failure_detection_loop, this));

#ifdef DEBUG_ 
        std::cout << "Before failure detection loop thread ...\n";
#endif
        scheduler->add(hpx::util::bind(&resilience::send_heartbeat_loop, this));
        scheduler->add(hpx::util::bind(&resilience::receive_heartbeat_loop, this));
    }

    bool resilience::rank_running(uint64_t rank) {
        return rank_running_[rank];
    }

    void resilience::init_recovery() {
        if (my_state == SUSPECT) {
#ifdef DEBUG_ 
            std::cout << "protectee state = SUSPECT\n";
#endif
            hpx::apply(&resilience::protectee_crashed, this);
            std::unique_lock<std::mutex> lk(cv_m);
#ifdef DEBUG_ 
            std::cout << "waiting on finished recovery...\n";
#endif
            cv.wait(lk, [this]{return recovery_done;});
#ifdef DEBUG_ 
            std::cout << "done waiting on finished recovery...\n";
#endif
            recovery_done = false;
            my_state = TRUST;
        }
        else {
#ifdef DEBUG_ 
            std::cout << "protectee state = TRUST\n";
#endif
        }
    }
   
    // ignore errors while sending -- we don't care about our guard
    void resilience::send_handler(boost::shared_ptr<std::string> message, const boost::system::error_code& error, std::size_t bytes_transferred) {
#ifdef DEBUG_
        if (error)
            std::cout << "Something went wrong sending ...\n";
#endif
    }

    //
    void resilience::send_heartbeat_loop () {
        std::size_t actual_epoch = 0;
        auto & service = hpx::get_thread_pool("io_pool")->get_io_service();
        udp::socket send_sock(service, udp::endpoint(udp::v4(), 0));
        while (resilience_component_running && (get_running_ranks() > 1)) {
            auto t_now =  std::chrono::high_resolution_clock::now();
            actual_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(t_now-start_time).count()/1000;
#ifdef DEBUG_
            std::cout << "Actual epoch: " << actual_epoch << "\n";
#endif
            boost::shared_ptr<std::string> data(new std::string(std::to_string(actual_epoch)));
            std::this_thread::sleep_for(milliseconds(miu));
#ifdef DEBUG_
            std::cout << "Send: " << rank_ << " -> " << guard_rank_ << "\n";
#endif
            // ToDo: protect access to guard_receiver_endpoint (which I may modify) !!!
            send_sock.async_send_to(boost::asio::buffer(*data), *guard_receiver_endpoint, boost::bind(&resilience::send_handler, this, data,
                                boost::asio::placeholders::error,
                                            boost::asio::placeholders::bytes_transferred));
        }
    }

    // Run detection forever ...
    void resilience::receive_heartbeat_loop () {
        //udp::endpoint sender_endpoint(boost::asio::ip::address::from_string(protectee_ip_addr), UDP_SEND_PORT);
        std::size_t actual_epoch = 0;
        char rcv_buf[16];
        std::size_t n;
        boost::system::error_code ec;
        while (resilience_component_running && (get_running_ranks() > 1)) {
            auto t_now =  std::chrono::high_resolution_clock::now();
            actual_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(t_now-start_time).count()/1000;

            if (my_state == TRUST) {
                n = c->receive(boost::asio::buffer(rcv_buf,16), boost::posix_time::milliseconds(delta), ec);
                // Generalization: assume that the error is the lost connection
                if (ec)
                {
#ifdef DEBUG_
                    std::cout << "Receive error: " << ec.message() << "\n"; 
#endif
                    my_state = SUSPECT;
                    init_recovery();
                }
                else {
                    std::size_t rcv_epoch = std::stoi(rcv_buf);
#ifdef DEBUG_
                    std::cout << "Received epoch: " << rcv_epoch << "\n";
#endif
                    if (rcv_epoch < actual_epoch) {
                        my_state = SUSPECT;
                        init_recovery();
                    }
                    else
                        my_state = TRUST;
                }
            }
        }
    }

    std::pair<hpx::id_type,uint64_t> resilience::get_protectee() {
        return std::make_pair(protectee_,protectee_rank_);
    }

    std::map<this_work_item::id,work_item> resilience::get_local_backups() {
        return local_backups_;
    }

    void resilience::init() {


        num_localities = hpx::get_num_localities().get();
        rank_running_.resize(num_localities, true);

        if (get_running_ranks() < 2) {
            resilience_component_running = false;
            resilience_disabled = true;
            std::cout << "Resilience disabled for single locality!\n";
            return;
        }
        else {
            resilience_disabled = false;
            resilience_component_running = true;
        }

        recovery_done = false;
        start_time = std::chrono::high_resolution_clock::now();
        scheduler.reset(new hpx::threads::executors::io_pool_executor);

        allscale::monitor::connect(allscale::monitor::work_item_execution_started, resilience::global_w_exec_start_wrapper);
        allscale::monitor::connect(allscale::monitor::work_item_result_propagated, resilience::global_w_exec_finish_wrapper);
        hpx::get_num_localities().get();

        std::uint64_t right_id = (rank_ + 1) % num_localities;
        std::uint64_t left_id = (rank_ == 0)?(num_localities-1):(rank_-1);
        std::uint64_t left_left_id = (left_id == 0)?(num_localities-1):(left_id-1);
        guard_ = hpx::find_from_basename("allscale/resilience", right_id).get();
        guard_rank_ = right_id;
        protectee_ = hpx::find_from_basename("allscale/resilience", left_id).get();
        protectee_rank_ = left_id;
        protectees_protectee_ = hpx::find_from_basename("allscale/resilience", left_left_id).get();
        protectees_protectee_rank_ = left_left_id;
#ifdef DEBUG_
        std::cout << "Resilience component with rank " << rank_ << "started. Protecting " << protectee_rank_ << "\n";
#endif


        auto & service = hpx::get_thread_pool("io_pool")->get_io_service();

        //sock = new boost::asio::ip::udp::socket(service, udp::endpoint(udp::v4(), UDP_RECV_PORT+rank_));

        //std::string protectee_ip_addr = hpx::async<get_ip_address_action>(protectee_).get();
        std::string my_ip_addr = hpx::async<get_ip_address_action>(hpx::find_here()).get();
        my_receiver_endpoint = new udp::endpoint(boost::asio::ip::address::from_string(my_ip_addr), UDP_RECV_PORT+rank_);
        c = new client(*my_receiver_endpoint);
        std::string guard_ip_addr = hpx::async<get_ip_address_action>(guard_).get();
        guard_receiver_endpoint = new udp::endpoint(boost::asio::ip::address::from_string(guard_ip_addr), UDP_RECV_PORT+guard_rank_);

    }

    std::string resilience::get_ip_address() {
        return hpx::util::resolve_public_ip_address();
    }

    void resilience::global_w_exec_start_wrapper(work_item const& w)
    {
        allscale::resilience::get().w_exec_start_wrapper(w);
    }

    void resilience::global_w_exec_finish_wrapper(work_item const& w)
    {
        allscale::resilience::get().w_exec_finish_wrapper(w);
    }

    // equiv. to taskAcquired in prototype
    void resilience::w_exec_start_wrapper(work_item const& w) {
        if (resilience_disabled) return;

        if (w.id().depth() != get_cp_granularity()) return;

        //@ToDo: do I really need to block (via get) here?
        hpx::async<remote_backup_action>(guard_, w).get();
        local_backups_[w.id()] = w;
    }

    void resilience::w_exec_finish_wrapper(work_item const& w) {
        if (resilience_disabled) return;

#ifdef DEBUG_ 
        std::cout << "Finish " << w.id().name() << "\n";
#endif // DEBUG_

        if (w.id().depth() != get_cp_granularity()) return;

        //@ToDo: do I really need to block (via get) here?
        hpx::async<remote_unbackup_action>(guard_, w).get();
        local_backups_.erase(w.id());

    }

    std::size_t resilience::get_running_ranks() {
        return rank_running_.count();
    }

    void resilience::protectee_crashed() {

#ifdef DEBUG_ 
        std::cout << "Begin recovery ...\n";
        std::cout << "set bitrank of " << protectee_rank_ << " to false\n";
#endif // DEBUG_ 
        rank_running_[protectee_rank_] = false;

        for (auto c : remote_backups_) {
            work_item restored = c.second;
#ifdef DEBUG_ 
            std::cout << "Will reschedule task " << restored.id().name() << "\n";
#endif // DEBUG_
            allscale::scheduler::schedule(std::move(restored));
        }
#ifdef DEBUG_ 
        std::cout << "Done rescheduling ...\n";
#endif
        // restore guard / protectee connections
        hpx::util::high_resolution_timer t;
        protectee_ = protectees_protectee_;
        protectee_rank_ = protectees_protectee_rank_;
        hpx::async<set_guard_action>(protectee_, this->get_id(), rank_).get();
        std::pair<hpx::id_type,uint64_t> p = hpx::async<get_protectee_action>(protectee_).get();
        protectees_protectee_ = p.first;
        protectees_protectee_rank_ = p.second;
        remote_backups_.clear();
        remote_backups_ = hpx::async<get_local_backups_action>(protectee_).get();
        {
            std::lock_guard<std::mutex> lk(cv_m);
            recovery_done = true;
        }
        cv.notify_one();
#ifdef DEBUG_ 
        std::cout << "Finish recovery\n";
#endif // DEBUG_
    }

	int resilience::get_cp_granularity() {
		return 3;
	}

    void resilience::remote_backup(work_item w) {
#ifdef DEBUG_
        std::cout << "Will backup task " << w.id().name() << "\n";
#endif
        std::unique_lock<std::mutex> lock(backup_mutex_);
        remote_backups_[w.id()] = w;
    }

    void resilience::remote_unbackup(work_item w) {

#ifdef DEBUG_
        std::cout << "Will unbackup task " << w.id().name() << "\n";
#endif
        std::unique_lock<std::mutex> lock(backup_mutex_);
        auto b = remote_backups_.find(w.id());
        if (b == remote_backups_.end())
            std::cerr << "ERROR: Backup not found that should be there!\n";
        remote_backups_.erase(b);
    }

    void resilience::shutdown() {
        if (resilience_component_running) {
            resilience_component_running = false;
            scheduler.reset();
            // We need to invoke synchronously here
            hpx::apply<shutdown_action>(guard_);
        }
    }

} // end namespace components
} // end namespace allscale

HPX_REGISTER_ACTION(allscale::components::resilience::remote_backup_action, remote_backup_action);
HPX_REGISTER_ACTION(allscale::components::resilience::remote_unbackup_action, remote_unbackup_action);
HPX_REGISTER_ACTION(allscale::components::resilience::set_guard_action, set_guard_action);
HPX_REGISTER_ACTION(allscale::components::resilience::get_protectee_action, get_protectee_action);
HPX_REGISTER_ACTION(allscale::components::resilience::get_local_backups_action, get_local_backups_action);
HPX_REGISTER_ACTION(allscale::components::resilience::shutdown_action, allscale_resilience_shutdown_action);
HPX_REGISTER_ACTION(allscale::components::resilience::kill_me_action, kill_me_action);
HPX_REGISTER_ACTION(allscale::components::resilience::get_ip_address_action, get_ip_address_action);
