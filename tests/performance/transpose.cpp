
#include <allscale/runtime.hpp>
//#include <allscale/api/user/data/static_grid.h>
#include <allscale/api/user/data/grid.h>
#include <hpx/util/assert.hpp>

#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>


#include <allscale/data_item_reference.hpp>
#include <allscale/data_item_manager.hpp>
#include <allscale/data_item_requirement.hpp>
#include <algorithm>

#include <hpx/config.hpp>
#include <hpx/include/components.hpp>
#include <boost/program_options.hpp>
#include <cstdlib>

ALLSCALE_REGISTER_TREETURE_TYPE(int)

#define EXPECT_EQ(X,Y)  X==Y
#define EXPECT_NE(X,Y)  X!=Y

HPX_REGISTER_COMPONENT_MODULE();

//using data_item_type_grid = allscale::api::user::data::Grid<int,1>;

//using data_item_type = allscale::api::user::data::StaticGrid<int, N, N>;
using data_item_type = allscale::api::user::data::Grid<int, 2>;
using region_type = data_item_type::region_type;
using coordinate_type = data_item_type::coordinate_type;
using data_item_shared_data_type = typename data_item_type::shared_data_type;

namespace po = boost::program_options;



REGISTER_DATAITEMSERVER_DECLARATION(data_item_type);
REGISTER_DATAITEMSERVER(data_item_type);



using namespace std;


////////////////////////////////////////////////////////////////////////////////
// grid init

struct grid_init_name {
    static const char* name() { return "grid_init"; }
};

struct grid_init_split;
struct grid_init_process;
struct grid_init_can_split;

using grid_init = allscale::work_item_description<
    void,
    grid_init_name,
    allscale::do_serialization,
    grid_init_split,
    grid_init_process,
    grid_init_can_split
>;

struct grid_init_can_split
{
    template <typename Closure>
    static bool call(Closure const& c)
    {
        auto begin = hpx::util::get<1>(c);
        auto end = hpx::util::get<2>(c);

        return sumOfSquares(end - begin) > 1;
    }
};

struct grid_init_split {
    template <typename Closure>
    static allscale::treeture<void> execute(Closure const& c)
    {
        auto data = hpx::util::get<0>(c);
        auto begin = hpx::util::get<1>(c);
        auto end = hpx::util::get<2>(c);
        auto dim = hpx::util::get<3>(c);

        std::size_t depth = allscale::this_work_item::get_id().depth();
        auto range = allscale::api::user::algorithm::detail::range<coordinate_type>(begin, end);
        auto fragments = allscale::api::user::algorithm::detail::range_spliter<coordinate_type>::split(dim, range);

        return allscale::runtime::treeture_parallel(
            allscale::spawn<grid_init>(data, fragments.left.begin(), fragments.left.end(), dim),
            allscale::spawn<grid_init>(data, fragments.right.begin(), fragments.right.end(), dim)
        );
    }

    static constexpr bool valid = true;
};

struct grid_init_process {
    template <typename Closure>
    static hpx::util::unused_type execute(Closure const& c)
    {
        auto ref = hpx::util::get<0>(c);
        auto data = allscale::data_item_manager::get(ref);
        auto begin = hpx::util::get<1>(c);
        auto end = hpx::util::get<2>(c);

        region_type region(begin, end);

        for(int i = begin[0]; i < end[0]; ++i){
            for(int j = begin[1]; j < end[1]; ++j){

                coordinate_type pos( allscale::utils::Vector<long,2>( i, j ) );
                data[pos] = i;
            }
        }

        /*
        region.scan(
            [&](auto const& pos)
            {
//                 std::cout << "Setting " << pos << ' ' << pos.x + pos.y << '\n';
                data[pos] =  hpx::get_locality_id();
            }
        );*/

        return hpx::util::unused;
    }

    template <typename Closure>
    static hpx::util::tuple<
        allscale::data_item_requirement<data_item_type >
    >
    get_requirements(Closure const& c)
    {
        region_type r(hpx::util::get<1>(c), hpx::util::get<2>(c));
        return hpx::util::make_tuple(
            allscale::createDataItemRequirement(
                hpx::util::get<0>(c),
                r,
                allscale::access_mode::ReadWrite
            )
        );
    }
    static constexpr bool valid = true;
};



////////////////////////////////////////////////////////////////////////////////
// transpose

struct transpose_name {
    static const char* name() { return "transpose"; }
};

struct transpose_split;
struct transpose_process;
struct transpose_can_split;

using transpose = allscale::work_item_description<
    void,
    transpose_name,
    allscale::do_serialization,
    transpose_split,
    transpose_process,
    transpose_can_split
>;

struct transpose_can_split
{
    template <typename Closure>
    static bool call(Closure const& c)
    {
        auto begin = hpx::util::get<2>(c);
        auto end = hpx::util::get<3>(c);

        return sumOfSquares(end - begin) > 1;
    }
};

struct transpose_split {
    template <typename Closure>
    static allscale::treeture<void> execute(Closure const& c)
    {
        auto mat_a = hpx::util::get<0>(c);
        auto mat_b = hpx::util::get<1>(c);


        auto begin = hpx::util::get<2>(c);
        auto end = hpx::util::get<3>(c);
        long tile_size = hpx::util::get<4>(c);

        std::size_t depth = allscale::this_work_item::get_id().depth();
        auto range = allscale::api::user::algorithm::detail::range<coordinate_type>(begin, end);
        //std::cout<<"about to split range : " << range.begin() << " to " << range.end() << std::endl;

        auto fragments = allscale::api::user::algorithm::detail::range_spliter<coordinate_type>::split(0, range);

        //std::cout<<"splitted, got left: " << fragments.left.begin() << " " << fragments.left.end() << " right: " << fragments.right.begin() << " " <<fragments.right.end() << std::endl;
        return allscale::runtime::treeture_parallel(
            allscale::spawn<transpose>(mat_a, mat_b, fragments.left.begin(), fragments.left.end(), tile_size),
            allscale::spawn<transpose>(mat_a, mat_b,  fragments.right.begin(), fragments.right.end(), tile_size)
        );
    }

    static constexpr bool valid = true;
};

struct transpose_process {
    template <typename Closure>
    static void execute(Closure const& c)
    {
        auto ref_mat_a = hpx::util::get<0>(c);
        auto data_a = allscale::data_item_manager::get(ref_mat_a);

        auto ref_mat_b = hpx::util::get<1>(c);
        auto data_b = allscale::data_item_manager::get(ref_mat_b);

        auto begin = hpx::util::get<2>(c);
        auto end = hpx::util::get<3>(c);
        long tile_size = hpx::util::get<4>(c);

        region_type region(begin, end);

        coordinate_type s_src(hpx::util::get<2>(c));
        coordinate_type e_src(hpx::util::get<3>(c));
        coordinate_type s_dst(allscale::utils::Vector<long,2>( s_src[1], s_src[0] ) );
        coordinate_type e_dst(allscale::utils::Vector<long,2>( e_src[1], e_src[0] ) );

        /*std::cout << hpx::get_locality_id() << " transpose process: s_src" << s_src << " e_src: " << e_src << " s_dst: " << s_dst
            << " e_dst " << e_dst <<  '\n';
        */
        for(long i = s_src[0]; i < e_src[0]; i+=tile_size)
        {
            for(long j = s_src[1]; j < e_src[1]; j+=tile_size)
            {
                //iterate thru tiles
                for (long it=i; it< std::min(e_src[0],i+tile_size); it++)
                {
                    for (long jt=j; jt< std::min(e_src[1],j+tile_size);jt++)
                    {
                        coordinate_type pos_src( allscale::utils::Vector<long,2>( it, jt ) );
                        coordinate_type pos_dst( allscale::utils::Vector<long,2>( jt, it ) );
                        data_b[pos_dst] = data_a[pos_src];
                        data_a[pos_src] += 1.0f;

                    }
                }
            }
        }
    }

    template <typename Closure>
    static hpx::util::tuple<
        allscale::data_item_requirement<data_item_type >,
        allscale::data_item_requirement<data_item_type >
    >
    get_requirements(Closure const& c)
    {
        // READ ACCESS REQUIRED TO SRC MATRIX
        region_type r_src(hpx::util::get<2>(c), hpx::util::get<3>(c));

        // READ/WRITE ACCESS REQUIRED TO DST MATRIX
        coordinate_type s_src(hpx::util::get<2>(c));
        coordinate_type e_src(hpx::util::get<3>(c));
        coordinate_type s_dst(allscale::utils::Vector<long,2>( s_src[1], s_src[0] ) );
        coordinate_type e_dst(allscale::utils::Vector<long,2>( e_src[1], e_src[0] ) );
        region_type r_dst(s_dst,e_dst);

        //acquire regions now
        return hpx::util::make_tuple(
            allscale::createDataItemRequirement(
                hpx::util::get<0>(c),
                r_src,
                allscale::access_mode::ReadWrite
            ),
            allscale::createDataItemRequirement(
                hpx::util::get<1>(c),
                r_dst,
                allscale::access_mode::ReadWrite
            )
        );
    }
    static constexpr bool valid = true;
};


////////////////////////////////////////////////////////////////////////////////
// main
struct main_process;

struct main_name {
    static const char* name() { return "main"; }
};

using main_work = allscale::work_item_description<
    int,
    main_name,
    allscale::no_serialization,
    allscale::no_split<int>,
    main_process>;

struct main_process
{
    static allscale::treeture<int> execute(hpx::util::tuple<int, char**> const& c)
    {
        int argc = hpx::util::get<0>(c);
        char** argv = hpx::util::get<1>(c);
        long iterations = 1;
        if (argc > 1)
            iterations = std::atoi(argv[1]);
        long N = 10000;
        if (argc > 2)
            N = std::atoi(argv[2]);
        long tile_size = 32;
        if (argc > 3)
            tile_size = std::atoi(argv[3]);

        allscale::api::user::data::GridPoint<2> size(N,N);
        data_item_shared_data_type sharedData(size);

        allscale::data_item_reference<data_item_type> mat_a
            = allscale::data_item_manager::create<data_item_type>(sharedData);

        allscale::data_item_reference<data_item_type> mat_b
            = allscale::data_item_manager::create<data_item_type>(sharedData);


        coordinate_type begin(0, 0);
        coordinate_type end(N, N);

        std::cout
            << "Transpose AllScale benchmark:\n"
            << "\t    localities = " << hpx::get_num_localities().get() << "\n"
            << "\tcores/locality = " << hpx::get_os_thread_count() << "\n"
            << "\t    iterations = " << iterations << "\n"
            << "\t             N = " << N << "\n"
            << "\t     tile_size = " << tile_size << "\n"
            << "\n"
            ;


        allscale::spawn_first<grid_init>(mat_a, begin, end, 0).wait();

        allscale::spawn_first<grid_init>(mat_b, begin, end, 1).wait();

        region_type whole_region({0,0}, {N, N});

        // First iteration as warm up...
        allscale::spawn_first<transpose>(mat_a, mat_b, begin, end, tile_size).wait();

        // DO ACTUAL WORK: SPAWN FIRST WORK ITEM
        hpx::util::high_resolution_timer timer;
        //auto startus = std::chrono::high_resolution_clock::now();

        for (long i = 0; i != iterations; ++i)
        {
            allscale::spawn_first<transpose>(mat_a, mat_b, begin, end, tile_size).wait();
        }


       //auto endus = std::chrono::high_resolution_clock::now();
       //std::chrono::duration<double> diff = endus-startus;
       //std::cout << "time: " << diff.count() << " s\n";
       double elapsed = timer.elapsed();

       double mups = (((N*N)/(elapsed/iterations))/1000000);
       double combined_matrix_size = 2.0 * sizeof(double) * N * N;
       double mbytes_per_second = combined_matrix_size/(elapsed/iterations) * 1.0E-06;
       std::cout << "Rate (MBs) | avg time (s) | MUP/S: "<<'\n';
       std::cout << mbytes_per_second << "," << elapsed/iterations << "," << mups << '\n';
//       // ===================================================
//
//
//     // ============ LOOK AT RESULT==========================
//       auto lease_result_a = allscale::data_item_manager::acquire(
//           allscale::createDataItemRequirement(
//               mat_a,
//               whole_region,
//               allscale::access_mode::ReadOnly
//           )).get();
//       auto ref_result_a = allscale::data_item_manager::get(mat_a);
//       for(int j = 0; j < N; ++j){
//           for(int i = 0; i < N; ++i){
//               coordinate_type tmp(allscale::utils::Vector<long,2>(i,j));
//               std::cout<< ref_result_a[tmp] << " ";
//
//           }
//           std::cout<<std::endl;
//       }
//       std::cout<<std::endl;
//     // ===================================================
//
//
//       std::cout<<std::endl;
//       std::cout<<std::endl;
//
//       std::cout<< N << std::endl;
//
     // ============ LOOK AT RESULT==========================
//       auto lease_result = allscale::data_item_manager::acquire(
//           allscale::createDataItemRequirement(
//               mat_b,
//               whole_region,
//               allscale::access_mode::ReadOnly
//           )).get();
//       auto ref_result = allscale::data_item_manager::get(mat_b);
//
//       std::cout<< " N " << N << std::endl;
//       for(int j = 0; j < N; ++j){
//           for(int i = 0; i < N; ++i){
//               coordinate_type tmp(allscale::utils::Vector<long,2>(i,j));
//           }
//           std::cout<<std::endl;
//       }
//     // ===================================================

     //allscale::data_item_manager::release(lease_result);


        allscale::data_item_manager::destroy(mat_a);
        allscale::data_item_manager::destroy(mat_b);

        return allscale::make_ready_treeture(0);
    }
    static constexpr bool valid = true;
};

int main(int argc, char **argv) {
    return allscale::runtime::main_wrapper<main_work>(argc, argv);
}
