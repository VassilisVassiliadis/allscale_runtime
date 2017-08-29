#ifndef ALLSCALE_DATA_ITEM_SERVER
#define ALLSCALE_DATA_ITEM_SERVER

#include <allscale/data_item_server_network.hpp>
#include <allscale/locality.h>
#include <allscale/data_item_requirement.hpp>
#include <allscale/data_item_reference.hpp>
#include <allscale/lease.hpp>
#include <allscale/location_info.hpp>
#include <map>

#include <allscale/util.hpp>
namespace allscale{
    
    template<typename DataItemType>
    struct data_item_server{
        
        using data_item_shared_data_type = typename DataItemType::shared_data_type;
        using data_item_fragment_type = typename DataItemType::fragment_type;
        using data_item_region_type = typename DataItemType::region_type;
        using network_type = data_item_server_network<DataItemType>;
	    
        // mock types 
        using locality_type = simulator::locality_type;
	    using id_type = std::size_t;
        
        
        
        struct fragment_info {

            // the managed fragment
            data_item_fragment_type fragment;

            // the regions currently locked through read leases
            data_item_region_type readLocked;

            // the list of all granted read leases
            std::vector<data_item_region_type> readLeases;

            // the regions currently locked through write access
            data_item_region_type writeLocked;

            fragment_info(const data_item_shared_data_type& shared)
                : fragment(shared), readLocked(), writeLocked() {}

            fragment_info(const fragment_info&) = delete;
            fragment_info(fragment_info&&) = default;


            void addReadLease(const data_item_region_type& region) {

                // add to lease set
                readLeases.push_back(region);

                // merge into read locked region
                readLocked = data_item_region_type::merge(readLocked, region);
            }

            void removeReadLease(const data_item_region_type& region) {

                // remove lease from lease set
                auto pos = std::find(readLeases.begin(),readLeases.end(),region);
                assert_true(pos != readLeases.end()) << "Attempting to delete non-existing read-lease: " << region << " in " << readLeases;
                readLeases.erase(pos);

                // update readLocked status
                data_item_region_type locked = data_item_region_type();
                for(const auto& cur : readLeases) {
                    locked = data_item_region_type::merge(locked,cur);
                }

                // exchange read locked region
                std::swap(readLocked,locked);
            }

        };


        locality_type myLocality;

        network_type& network;

        std::map<id_type,fragment_info> store;

        id_type idCounter;

        bool alive;
    public:


    data_item_server(locality_type loc, network_type& network)
        : myLocality(loc), network(network), idCounter(0), alive(true) {}

    void kill() {
        alive = false;
        store.clear();
    }
 
    template<typename ... Args>
    data_item_reference<DataItemType> create(const Args& ... args) {
        assert_true(alive);

        // create shared data
        data_item_shared_data_type shared(args...);

        // prepare shared data to be distributed
        auto sharedStateArchive = allscale::utils::serialize(shared);
        id_type dataItemID = (myLocality << 20) + (idCounter++);

        // inform other servers
        network.broadcast([&sharedStateArchive,dataItemID](data_item_server& server) {

            // retrieve shared data
            auto sharedData = allscale::utils::deserialize<data_item_shared_data_type>(sharedStateArchive);

            // create new local fragment
            server.store.emplace(dataItemID, std::move(fragment_info(sharedData)));
        });

        // return reference
        return { dataItemID };
    }
    
    location_info<DataItemType> locate(const data_item_reference<DataItemType>& ref, const data_item_region_type& region) 
    {
        assert_true(alive);
        location_info<DataItemType> res;
        network.broadcast([&](const data_item_server& server) {
            if (!server.alive) return;
            auto& info = server.getInfo(ref);
            auto part = data_item_region_type::intersect(region,info.fragment.getCoveredRegion());
            if (part.empty()) return;
            res.addPart(part,server.myLocality);
        });
        return res;
    }



    lease<DataItemType> acquire(const data_item_requirement<DataItemType>& request) {

        assert_true(alive);

        // collect data on data distribution
        auto locationInfo = locate(request.ref,request.region);
        // get local fragment info
        auto& info = getInfo(request.ref);

        // allocate storage for requested data on local fragment
        info.fragment.resize(merge(info.fragment.getCoveredRegion(), request.region));

        // transfer data using a transfer plan
        auto success = execute(buildPlan(locationInfo,myLocality,request));

        // make sure the transfer was ok
        assert_true(success);
/*
        // lock requested data as required
        switch(request.mode) {
            case AccessMode::ReadOnly: {

                // check that access can be granted
                network.broadcast([&](DataItemServer& server) {
                    assert_true(intersect(server.getInfo(request.ref).writeLocked,request.region).empty())
                            << "Error: requesting read access to write-locked data: " << data_item_region_type::intersect(server.getInfo(request.ref).writeLocked,request.region);
                });

                // lock data for read
                info.addReadLease(request.region);

                break;
            }
            case AccessMode::ReadWrite: {

                // check that access can be granted
                network.broadcast([&](DataItemServer& server) {

                    if (!server.alive) return;

                    auto& locInfo = server.getInfo(request.ref);

                    // check that access can be granted
                    assert_true(intersect(locInfo.readLocked,request.region).empty())
                            << "Error: requesting write access to read-locked data: " << intersect(locInfo.readLocked,request.region);

                    // check that access can be granted
                    assert_true(intersect(locInfo.writeLocked,request.region).empty())
                            << "Error: requesting write access to write-locked data: " << intersect(locInfo.writeLocked,request.region);

                    // invalidate the remote-servers copy
                    if (&server != this) {
                        locInfo.fragment.resize(difference(locInfo.fragment.getCoveredRegion(),request.region));
                    }

                });

                // lock data for write
                info.writeLocked = data_item_region_type::merge(info.writeLocked, request.region);

                break;
            }
        }
        */
        return request;
    }

    private:
    fragment_info& getInfo(const data_item_reference<DataItemType>& ref) {
        auto pos = store.find(ref.id);
        assert_true(pos != store.end()) << "Requested invalid data item id: " << ref.id;
        return pos->second;
    }

    const fragment_info& getInfo(const data_item_reference<DataItemType>& ref) const {
        auto pos = store.find(ref.id);
        assert_true(pos != store.end()) << "Requested invalid data item id: " << ref.id;
        return pos->second;
    }








    };
}

#endif
