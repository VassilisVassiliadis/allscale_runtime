//#include "allscale/api/user/data/scalar.h"
#include "allscale/api/user/data/grid.h"
#include <allscale/data_item_reference.hpp>
#include <allscale/data_item_server.hpp>
#include <allscale/data_item_server_network.hpp>
#include <allscale/data_item_manager.hpp>
#include <allscale/data_item_requirement.hpp>
#include <algorithm>

#include <hpx/hpx_main.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/config.hpp>
#include <hpx/include/components.hpp>

#include <hpx/runtime/serialization/serialize.hpp>
#include <hpx/runtime/serialization/input_archive.hpp>
#include <hpx/runtime/serialization/output_archive.hpp>
#include <hpx/runtime/serialization/vector.hpp>

#define EXPECT_EQ(X,Y)  X==Y
#define EXPECT_NE(X,Y)  X!=Y


using namespace allscale::api::user::data;
using namespace allscale;
HPX_REGISTER_COMPONENT_MODULE();

/*
using data_item_type = Scalar<int>;
REGISTER_DATAITEMSERVER_DECLARATION(data_item_type);
REGISTER_DATAITEMSERVER(data_item_type);
*/




using data_item_type_grid = Grid<int,1>;
REGISTER_DATAITEMSERVER_DECLARATION(data_item_type_grid);
REGISTER_DATAITEMSERVER(data_item_type_grid);








template <typename DataItemType, typename ... Args>
auto simulate_data_item_manager_create_and_get(const Args& ... args){
    
    std::vector<allscale::data_item_server<DataItemType>> result;
    typedef typename allscale::data_item_manager manager_type;

    if (hpx::get_locality_id() == 0) {
        typedef typename  allscale::server::data_item_server<DataItemType>::template acquire_action<allscale::data_item_requirement<DataItemType>> action_type;
        auto dataRef = manager_type::create<DataItemType>(args...);
        
        auto res0 = manager_type::get_server<DataItemType>();
        auto res1 = manager_type::get_server<DataItemType>(1);
        auto res2 = manager_type::get_server<DataItemType>(2);
        
        auto req0 = allscale::createDataItemRequirement(dataRef, GridRegion<1>(100,150), access_mode::ReadWrite); 
        auto req1 = allscale::createDataItemRequirement(dataRef, GridRegion<1>(50,99), access_mode::ReadWrite); 
        auto req2 = allscale::createDataItemRequirement(dataRef, GridRegion<1>(50,150), access_mode::ReadWrite); 
        
        action_type()(res0,req0);
        
        action_type()(res1,req1);
        action_type()(res2,req2);
        // acquire bigger lease
    //    auto req2 = allscale::createDataItemRequirement(dataRef, GridRegion<1>(5000,5100), access_mode::ReadWrite); 
      //  auto lease2 = allscale::data_item_manager::acquire<DataItemType>(req2);


        //auto data = allscale::data_item_manager::get(dataRef);
        
        //std::cout<<data[150]<<std::endl;
        //data[5000] = 5;
        //std::cout<<data[5000]<<std::endl;
    }


    return result;
}


void test_grid_data_item_server_create_and_get() {
    GridPoint<1> size = 200;
    using data_item_shared_data_type = typename data_item_type_grid::shared_data_type;
    data_item_shared_data_type sharedData(size);
    simulate_data_item_manager_create_and_get<Grid<int,1>>(sharedData);
}





int hpx_main(int argc, char* argv[]) {
    test_grid_data_item_server_create_and_get();
    return hpx::finalize();
}
