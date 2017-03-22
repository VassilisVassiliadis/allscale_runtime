#include <iostream>
#include <array>
#include <string>
#include <type_traits>
#include <typeinfo>
//#include <unordered_map>
#include <allscale/data_item.hpp>
#include <allscale/data_item_description.hpp>
#include <allscale/region.hpp>
#include <allscale/fragment.hpp>

#include <allscale/data_item_manager_server.hpp>
#include <allscale/requirement.hpp>

#include <hpx/config.hpp>
#include <hpx/hpx_main.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/util/high_resolution_timer.hpp>

namespace allscale {
template<typename T>
struct grid_region_1d: public allscale::region<T> {
public:
	grid_region_1d() :
			begin_(0), end_(0) {
	}

	grid_region_1d(std::size_t b, std::size_t e) :
			begin_(b), end_(e) {
	}
	bool operator==(grid_region_1d const & rh) const {
		return (begin_ == rh.begin_ && end_ == rh.end_);
	}
	bool has_intersection_with(grid_region_1d const & rh) {
		return !((rh.end_ < begin_) || (rh.begin_ > end_));
	}

	template<typename Archive>
	void serialize(Archive & ar, unsigned) {
		ar & begin_;
		ar & end_;

	}

	std::string to_string() const {
		std::string text = "[";
		text += std::to_string(begin_);
		text += ":";
		text += std::to_string(end_);
		text += "]";
		return text;
	}

	std::size_t begin_;
	std::size_t end_;
};
}

using my_region = allscale::region<int>;
using my_fragment = allscale::fragment<my_region,int>;
using descr = allscale::data_item_description<my_region,my_fragment,int>;
using test_item = allscale::data_item<descr>;

using grid_region_type = allscale::grid_region_1d<int>;
using grid_fragment = allscale::fragment<grid_region_type,std::vector<int>>;
using data_item_descr = allscale::data_item_description<grid_region_type,grid_fragment,std::shared_ptr<std::vector<int>>>;
using test_item_2 = allscale::data_item<data_item_descr>;

using test_requirement2 = allscale::requirement<data_item_descr>;

typedef allscale::data_item_manager_server my_data_item_manager_server;
typedef std::pair<hpx::naming::id_type, my_data_item_manager_server> loc_server_pair;

ALLSCALE_REGISTER_DATA_ITEM_TYPE(descr);
ALLSCALE_REGISTER_DATA_ITEM_TYPE(data_item_descr);

ALLSCALE_REGISTER_DATA_ITEM_MANAGER_SERVER_COMPONENT()
;

std::vector<loc_server_pair> dms;

/*
 template<typename T>
 bool check(T first)
 {
 if(allscale::traits::is_data_item<T>::value){
 //std::cout<<"allscale::traits::is_treeture<T>::value==TRUE"<<std::endl;
 return true;
 }
 else
 {
 //std::cout<<"allscale::traits::is_treeture<T>::value==FALSE"<<std::endl;
 return false;
 }
 }
 */

bool test_creation_of_data_item_manager_server_components() {
	std::vector<hpx::naming::id_type> localities = hpx::find_all_localities();
	if (hpx::get_locality_id() == 0) {

		std::vector<hpx::naming::id_type> server_ids;
		for (hpx::naming::id_type const& node : localities) {
			std::cout << "Cycling thru localities, Locality: " << node
					<< std::endl;
			auto tmp = my_data_item_manager_server(node);
			dms.push_back(std::make_pair(node, tmp));
			server_ids.push_back(tmp.get_id());
		}
		for (auto & node_server_pair : dms) {
			auto& server = node_server_pair.second;
			server.servers_ = server_ids;
		}
	}
	return true;
}

bool test_creation_of_data_items() {
	//create a bunch of data items with a simple region on all the localities
	for (loc_server_pair& server_entry : dms) {
		for (int i = 0; i < 10; ++i) {
			my_region test_region(2);
			my_fragment test_frag(test_region, i);
			descr test_descr(test_region, test_frag, 0);
			auto td = server_entry.second.create_data_item < descr
					> (test_descr);
			//std::cout<<"test item fragment value: " << *((*td).fragment_.ptr_)  << " global id: " << hpx::naming::get_locality_from_gid((*td).get_id().get_gid()) << std::endl;
		}
	}
	return true;
}

bool test_locate_method() {
	//create a bunch of data items with a simple 1d grid region on all the localities
	int c = 0;
	for (loc_server_pair& server_entry : dms) {
		auto a = std::make_shared<std::vector<int>>(std::vector<int>(10, 337));
		grid_region_type gr(c, c + 9);
		grid_fragment frag(gr, a);
		data_item_descr descr(gr, frag, nullptr);
		auto td = server_entry.second.create_data_item < data_item_descr
				> (descr);
		auto k = (std::vector<int>) *(td->fragment_.ptr_);
		c += 10;
	}

	// now locate a region consisting of both the upper regions.
	grid_region_type search_region(0, 20);
	auto b = std::make_shared<std::vector<int>>(std::vector<int>(20, 0));
	test_requirement2 tr(search_region);
	//attach automatic continuation to the futures, this can be acquiring too
	for (auto& fut : dms[0].second.locate < data_item_descr > (tr)) {
		fut.then(
				[](auto f) -> int
				{

					return 22;
				});
	}
	return true;
}

bool test_acquire_method() {
	//create a bunch of data items with a simple 1d grid region on all the localities
	int c = 0;
	int i = 33;
	for (loc_server_pair& server_entry : dms) {
		auto a = std::make_shared<std::vector<int>>(std::vector<int>(10, i));
		grid_region_type gr(c, c + 9);
		grid_fragment frag(gr, a);
		data_item_descr descr(gr, frag, nullptr);
		auto td = server_entry.second.create_data_item < data_item_descr
				> (descr);
		auto k = (std::vector<int>) *(td->fragment_.ptr_);
		c += 10;
		i *= 2;
	}

	// now locate a region consisting of both the upper regions.
	grid_region_type search_region(0, 20);
	auto b = std::make_shared<std::vector<int>>(std::vector<int>(20, 0));
	test_requirement2 tr(search_region);
	//attach automatic continuation to the futures, this can be acquiring too
	for (auto& fut : dms[0].second.locate < data_item_descr > (tr)) {
		fut.then(
				[](auto f) -> int
				{
					for(auto& el : f.get()) {
//						std::cout<< el.first.to_string() << " " << el.second << " " << std::endl;

						using action_type = typename allscale::components::data_item_manager_server::acquire_async_action<data_item_descr>;
						auto res = hpx::async<action_type>(el.second,el.first);
//						std::cout << (*(res.get()))[3]  << std::endl;
					}
					return 22;
				});

	}
	return true;
}

/*
 bool test_acquire_method() {
 //create a bunch of data items with a simple 1d grid region on all the localities
 int c = 0;
 for (loc_server_pair& server_entry : dms) {
 auto a = std::make_shared<std::vector<int>>(std::vector<int>(10, 337));
 grid_region_type gr(c, c + 9);
 grid_fragment frag(gr, a);
 data_item_descr descr(gr, frag, nullptr);
 auto td = server_entry.second.create_data_item < data_item_descr
 > (descr);
 auto k = (std::vector<int>) *(td->fragment_.ptr_);
 c += 10;
 }

 // now locate a region consisting of both the upper regions.
 grid_region_type search_region(0, 7);
 auto b = std::make_shared<std::vector<int>>(std::vector<int>(20, 0));
 test_requirement2 tr(search_region);
 using fut_type = decltype(dms[0].second.locate<data_item_descr>(tr));
 std::vector<fut_type> vec_of_futs;
 // locate by running locate on the distributed
 for (loc_server_pair& el : dms) {
 vec_of_futs.push_back(
 std::forward<fut_type>(
 el.second.locate < data_item_descr > (tr)));
 }

 for (auto& el : vec_of_futs) {
 auto res = el.get();
 for (auto& el2 : res) {
 std::cout << el2.first.begin_ << " " << el2.first.end_ << " " << el2.second << std::endl;
 }
 }
 return true;
 }


 */

int hpx_main(int argc, char* argv[]) {
	if (hpx::get_locality_id() == 0) {
		if (test_creation_of_data_item_manager_server_components() == true) {
			std::cout
					<< "Test(1): Creation of data_item_manager_servers : PASSED"
					<< std::endl;
		}

//		if (test_creation_of_data_items() == true) {
//			std::cout << "Test(2): Creation of data_items : PASSED"
//					<< std::endl;
//		}
		//test_creation_of_data_item_manager_server_components();
		//test_locate_method();
		test_acquire_method();
//
//		if (test_locate_method() == true) {
//			std::cout
//					<< "Test(3): Locate all data_items related to a region of a requirement: PASSED"
//					<< std::endl;
//		}
	}

	return hpx::finalize();
}

int main(int argc, char* argv[])
{
	return hpx::init(argc, argv);
}

