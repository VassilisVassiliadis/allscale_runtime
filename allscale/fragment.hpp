#ifndef ALLSCALE_FRAGMENT_HPP
#define ALLSCALE_FRAGMENT_HPP

#include <allscale/components/fragment.hpp>
#include <hpx/include/serialization.hpp>
#include <memory>
#include <iostream>

namespace allscale {

    struct fragment_base{
        virtual ~fragment_base(){}
    };


template<typename Region, typename T>
struct fragment : hpx::components::client_base<fragment<Region,T>,
                    components::fragment<Region,T> >, fragment_base
{
public:
	using value_type = T;
	using region_type = Region;

	fragment() {
	}

	fragment(Region const& r, T t) :region_(r)
	{
		T *tmp = new T(t);
		ptr_ = std::shared_ptr < T > (tmp);
	}

	fragment(Region const& r, std::shared_ptr<T> ptr ) :
			region_(r),
			ptr_(ptr)
	{

	}

	fragment(Region const& r) : region_(r), ptr_(std::make_shared<value_type>(value_type(region_.get_elements())))
	{

	}

	void resize(fragment const& fragment, Region const& region) {
	}

	fragment mask(fragment const& fragment, Region const& region) {
	}

	void insert(fragment & destination, fragment const& source,
			Region const& region) {
	}

	template<typename Archive>
	void save(Archive& ar, fragment const& fragment) {
	}

	template<typename Archive>
	void serialize(Archive & ar, unsigned) {
		ar & region_;
		ar & ptr_;
	}

	template<typename Archive>
	void load(Archive& ar, fragment & fragment) {
	}

	Region region_;
	std::shared_ptr<T> ptr_;
};
}

#endif
