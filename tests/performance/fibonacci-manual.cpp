/**
 * ------------------------ Auto-generated Code ------------------------
 *           This code was generated by the Insieme Compiler
 * ---------------------------------------------------------------------
 */
#include <allscale/runtime.hpp>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <stdbool.h>
#include <stdint.h>

/* ------- Program Code --------- */

struct __wi_main_variant_0;

struct __wi_main_can_split;

struct __wi_main_variant_1;

struct __wi_main_name {
    static const char* name() { return "__wi_main"; }
};

using __wi_main_work = allscale::work_item_description<int32_t, __wi_main_name, allscale::no_serialization, __wi_main_variant_0, __wi_main_variant_1, __wi_main_can_split >;

/* ------- Function Definitions --------- */
int32_t main(int32_t var_0, char** var_1) {
    return allscale::runtime::main_wrapper<__wi_main_work >(var_0, var_1);
}

ALLSCALE_REGISTER_TREETURE_TYPE(int32_t)

struct __wi_allscale_wi_1_can_split;

struct __wi_allscale_wi_1_name {
    static const char* name() { return "__wi_allscale_wi_1"; }
};

struct __wi_allscale_wi_1_variant_0;

struct __wi_allscale_wi_1_variant_1;

using __wi_allscale_wi_1_work = allscale::work_item_description<int32_t, __wi_allscale_wi_1_name, allscale::no_serialization, __wi_allscale_wi_1_variant_0, __wi_allscale_wi_1_variant_1, __wi_allscale_wi_1_can_split >;

/* ------- Function Definitions --------- */
allscale::treeture<int32_t > allscale_fun_28(allscale::runtime::dependencies const& var_0, hpx::util::tuple<int32_t > const& var_1) {
    return allscale::spawn_first_with_dependencies<__wi_allscale_wi_1_work >(var_0, hpx::util::get<0 >(var_1));
}
/* ------- Function Definitions --------- */
int64_t IMP_fib(int64_t i) {
    auto var_1 = allscale::runtime::make_prec_operation<int32_t, int32_t >(hpx::util::tuple< >{}, &allscale_fun_28);
    return (int64_t)var_1((int32_t)i).get_result();
}
/* ------- Function Definitions --------- */
void IMP_execute(int32_t var_0) {
    std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<int64_t, std::ratio<1, 1000000000 > > > var_1 = (std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<int64_t, std::ratio<1, 1000000000 > > >&&)std::chrono::system_clock::now();
    IMP_fib((int64_t)var_0);
    std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<int64_t, std::ratio<1, 1000000000 > > > var_2 = (std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<int64_t, std::ratio<1, 1000000000 > > >&&)std::chrono::system_clock::now();
    std::chrono::duration<int64_t, std::ratio<1, 1000000000 > > var_3 = (std::chrono::duration<int64_t, std::ratio<1, 1000000000 > > const&)(var_2 - var_1);
    std::cout << var_3.count() << &std::endl;
}
/* ------- Function Definitions --------- */
int32_t IMP_main(int32_t var_0, char** var_1) {
    int32_t var_2 = -1;
    {
        int32_t var_3 = 0;
        while (var_3 < var_0) {
            if ((int32_t)var_1[var_3][0] == (int32_t)((char)'-')) {
                {
                    for (int32_t var_4 = var_3 + 1; var_4 < var_0; ++var_4) {
                        var_1[var_4 - 1] = var_1[var_4];
                    };
                };
                var_0--;
            };
            var_3++;
        };
    };
    if (var_0 != 2) {
        std::cerr << "Usage: " << var_1[0] << " [NUMBER]" << &std::endl;
        var_2 = 38;
        std::cerr << "Setting default number " << var_2 << &std::endl;
    } else {
        var_2 = atoi(var_1[1]);
        if (var_2 < 0) {
            std::cerr << "Number has to be >= 0" << &std::endl;
            return 1;
        };
    };
    IMP_execute(var_2);
    return 0;
}
/* ------- Function Definitions --------- */
allscale::treeture<int32_t > allscale_fun_3(hpx::util::tuple<int32_t, char** > const& var_0) {
    return allscale::treeture<int32_t >(IMP_main(hpx::util::get<0 >(var_0), hpx::util::get<1 >(var_0)));
}
struct __wi_main_variant_0 {
    static allscale::treeture<int32_t > execute(hpx::util::tuple<int32_t, char** > const& var_0);
    static constexpr bool valid = true;
};

/* ------- Function Definitions --------- */
bool allscale_fun_36(hpx::util::tuple<int32_t > const& var_0) {
    return *(int32_t*)(&hpx::util::get<0 >(var_0)) < 2;
}
/* ------- Function Definitions --------- */
bool allscale_fun_40(hpx::util::tuple<int32_t > const& var_0) {
    return !allscale_fun_36(var_0);
}
struct __wi_allscale_wi_1_can_split {
    static bool call(hpx::util::tuple<int32_t > const& var_0);
};

bool __wi_allscale_wi_1_can_split::call(hpx::util::tuple<int32_t > const& var_0) {
    return allscale_fun_40(var_0);
}
/* ------- Function Definitions --------- */
allscale::treeture<int32_t > allscale_fun_32(hpx::util::tuple<int32_t > const& var_0) {
    allscale::treeture<int32_t > var_1 = allscale::spawn_with_dependencies<__wi_allscale_wi_1_work >(allscale::runtime::after(), *(int32_t*)(&hpx::util::get<0 >(var_0)) - 1);
    allscale::treeture<int32_t > var_2 = allscale::spawn_with_dependencies<__wi_allscale_wi_1_work >(allscale::runtime::after(), *(int32_t*)(&hpx::util::get<0 >(var_0)) - 2);
    return allscale::treeture<int32_t >(var_1.get_result() + var_2.get_result());
}
/* ------- Function Definitions --------- */
allscale::treeture<int32_t > allscale_fun_30(hpx::util::tuple<int32_t > const& var_0) {
    return allscale_fun_32(var_0);
}
struct __wi_allscale_wi_1_variant_0 {
    static allscale::treeture<int32_t > execute(hpx::util::tuple<int32_t > const& var_0);
    static constexpr bool valid = true;
};

allscale::treeture<int32_t > __wi_allscale_wi_1_variant_0::execute(hpx::util::tuple<int32_t > const& var_0) {
    return allscale_fun_30(var_0);
}
/* ------- Function Prototypes ---------- */
int32_t rec(hpx::util::tuple<int32_t > const& p1);

/* ------- Function Definitions --------- */
allscale::treeture<int32_t > allscale_fun_34(hpx::util::tuple<int32_t > const& var_0) {
    return allscale::treeture<int32_t >(rec(var_0));
}
struct __wi_allscale_wi_1_variant_1 {
    static allscale::treeture<int32_t > execute(hpx::util::tuple<int32_t > const& var_0);
    static constexpr bool valid = true;
};

/* ------- Function Definitions --------- */
int32_t allscale_fun_38(hpx::util::tuple<int32_t > const& var_0) {
    return *(int32_t*)(&hpx::util::get<0 >(var_0));
}
/* ------- Function Definitions --------- */
int32_t allscale_fun_39(hpx::util::tuple<int32_t > const& var_0) {
    int32_t var_1 = rec(hpx::util::tuple<int32_t >{hpx::util::get<0 >(var_0) - 1});
    int32_t var_2 = rec(hpx::util::tuple<int32_t >{hpx::util::get<0 >(var_0) - 2});
    return var_1 + var_2;
}
/* ------- Function Definitions --------- */
int32_t rec(hpx::util::tuple<int32_t > const& var_0) {
    if (allscale_fun_36(var_0)) {
        return allscale_fun_38(var_0);
    } else {
        return allscale_fun_39(var_0);
    };
}
allscale::treeture<int32_t > __wi_allscale_wi_1_variant_1::execute(hpx::util::tuple<int32_t > const& var_0) {
    return allscale_fun_34(var_0);
}
allscale::treeture<int32_t > __wi_main_variant_0::execute(hpx::util::tuple<int32_t, char** > const& var_0) {
    return allscale_fun_3(var_0);
}
/* ------- Function Definitions --------- */
bool allscale_fun_47(hpx::util::tuple<int32_t, char** > const& var_0) {
    return (bool)false;
}
struct __wi_main_can_split {
    static bool call(hpx::util::tuple<int32_t, char** > const& var_0);
};

bool __wi_main_can_split::call(hpx::util::tuple<int32_t, char** > const& var_0) {
    return allscale_fun_47(var_0);
}
struct __wi_main_variant_1 {
    static allscale::treeture<int32_t > execute(hpx::util::tuple<int32_t, char** > const& var_0);
    static constexpr bool valid = true;
};

allscale::treeture<int32_t > __wi_main_variant_1::execute(hpx::util::tuple<int32_t, char** > const& var_0) {
    return allscale_fun_3(var_0);
}

