/**
 * ------------------------ Auto-generated Code ------------------------
 *           This code was generated by the Insieme Compiler
 * ---------------------------------------------------------------------
 */
#include <alloca.h>
#include <allscale/api/user/data/static_grid.h>
#include <allscale/data_item_server.hpp>
#include <allscale/runtime.hpp>
#include <allscale/no_split.hpp>
#include <allscale/utils/vector.h>
#include <iostream>
#include <stdbool.h>
#include <stdint.h>

/* ------- Program Code --------- */

ALLSCALE_REGISTER_TREETURE_TYPE(int32_t)
using data_item_type_1 = allscale::api::user::data::StaticGrid<double, 200, 200 >;
REGISTER_DATAITEMSERVER_DECLARATION(data_item_type_1)
REGISTER_DATAITEMSERVER(data_item_type_1)

struct main_name {
    static const char* name() { return "__wi_main"; }
};

struct main_process;

using main_work = allscale::work_item_description<
    int32_t,
    main_name,
    allscale::no_serialization,
    allscale::no_split<int32_t>,
    main_process>;

/* ------- Function Definitions --------- */
int32_t main(int argc, char** argv) {
    return allscale::runtime::main_wrapper<main_work >(argc, argv);
}

/* ------- Function Definitions --------- */
int32_t IMP_main() {
}
/* ------- Function Definitions --------- */
struct main_process {
    static allscale::treeture<int32_t > execute(hpx::util::tuple< > const&)
    {
        using allscale::runtime::DataItemReference;
        using allscale::runtime::DataItemManager;
        using allscale::api::user::data::StaticGrid;
        using allscale::api::user::data::StaticGridRegion;
        using allscale::utils::Vector;
        const int32_t var_0 = 200;
        const int32_t var_1 = 10;
        const double var_2 = 1.0E-3;
        DataItemReference<StaticGrid<double, 200, 200 > > var_3 =
            DataItemManager::create<StaticGrid<double, 200, 200 > >();
        DataItemReference<StaticGrid<double, 200, 200 > > var_4 =
            DataItemManager::create<StaticGrid<double, 200, 200 > >();
        DataItemReference<StaticGrid<double, 200, 200 > >& var_5 = var_3;

        hpx::util::high_resolution_timer timer;

        auto lease1 = allscale::data_item_manager::acquire<StaticGrid<double, 200, 200 >>(
            allscale::createDataItemRequirement(
                var_3,
                StaticGridRegion<2>(200, 200),
                allscale::access_mode::ReadWrite)
        );

        auto lease2 = allscale::data_item_manager::acquire<StaticGrid<double, 200, 200 >>(
            allscale::createDataItemRequirement(
                var_4,
                StaticGridRegion<2>(200, 200),
                allscale::access_mode::ReadWrite)
        );

        int32_t var_6 = 0;
        while (var_6 < var_0)
        {
            int32_t var_7 = 0;
            while (var_7 < var_0) {
                {
                    DataItemManager::get(var_5)[Vector<long, 2>(var_6, var_7)] = 0.;
                    if (var_6 == var_0 / 2 && var_7 == var_0 /2)
                    {
                        DataItemManager::get(var_5)[Vector<long, 2>(var_6, var_7)] = 100.;
                    }
                };
                var_7++;
            };
            var_6++;
        }
        for (int32_t var_8 = 0; var_8 < var_1; ++var_8)
        {
            DataItemReference<StaticGrid<double, 200, 200>>& var_9 = var_3;
            DataItemReference<StaticGrid<double, 200, 200>>& var_10 = var_4;
            int32_t var_11 = 1;
            while (var_11 < var_0 - 1)
            {
                int32_t var_12 = 1;
                while (var_12 < var_0 - 1)
                {
                    DataItemManager::get(var_10)[Vector<long, 2 >(var_11, var_12)] =
                        DataItemManager::get(var_9)[Vector<long, 2 >(var_11, var_12)] +
                        var_2 * (
                            DataItemManager::get(var_9)[Vector<long, 2>(var_11 - 1, var_12)] +
                            DataItemManager::get(var_9)[Vector<long, 2>(var_11 + 1, var_12)] +
                            DataItemManager::get(var_9)[Vector<long, 2>(var_11, var_12 - 1)] +
                            DataItemManager::get(var_9)[Vector<long, 2>(var_11, var_12 + 1)] +
                            -4. * DataItemManager::get(var_9)[Vector<long, 2>(var_11, var_12)]);
                    var_12++;
                }
                var_11++;
            }
            if (var_8 % (var_1 / 10) == 0)
            {
                std::cout << "t=" << var_8 << " - center: " <<
                    DataItemManager::get(var_10)[Vector<long, 2 >(var_0 / 2, var_0 / 2)] <<
                    std::endl;
            }
            std::swap(var_3, var_4);
        }
        allscale::data_item_manager::release(lease1);
        allscale::data_item_manager::release(lease2);
        double elapsed = timer.elapsed();

        std::cout << "t=" << var_1 << " - center: " <<
            DataItemManager::get(var_5)[Vector<long, 2 >(var_0 / 2, var_0 / 2)] <<
            std::endl;
        std::cout << "Elapsed time: " << elapsed << '\n';
        return allscale::make_ready_treeture(
            DataItemManager::get(var_5)[Vector<long, 2 >(var_0 / 2, var_0 / 2)] < 100. ? 0 : 1);
    }
    static constexpr bool valid = true;
};
