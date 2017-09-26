/**
 * ------------------------ Auto-generated Code ------------------------
 *           This code was generated by the Insieme Compiler
 * ---------------------------------------------------------------------
 */
#include <alloca.h>
#include <allscale/runtime.hpp>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
#define INS_INIT(...) __VA_ARGS__
#else
#define INS_INIT(...) (__VA_ARGS__)
#endif
#ifdef __cplusplus
#include <new>
#define INS_INPLACE_INIT(Loc,Type) new(Loc) Type
#else
#define INS_INPLACE_INIT(Loc,Type) *(Loc) = (Type)
#endif
#ifdef __cplusplus
				/** Workaround for libstdc++/libc bug.
				 *  There's an inconsistency between libstdc++ and libc regarding whether
				 *  ::gets is declared or not, which is only evident when using certain
				 *  compilers and language settings
				 *  (tested positively with clang 3.9 --std=c++14 and libc 2.17).
				 */
				#include <initializer_list>  // force libstdc++ to include its config
				#undef _GLIBCXX_HAVE_GETS    // correct broken config
#endif

/* ------- Program Code --------- */

struct __wi_main_variant_0;
typedef struct __wi_main_variant_0 __wi_main_variant_0;

struct __wi_main_name {
    static const char* name() { return "__wi_main"; }
};

struct __wi_main_variant_1;
typedef struct __wi_main_variant_1 __wi_main_variant_1;

struct __wi_main_can_split;
typedef struct __wi_main_can_split __wi_main_can_split;

using __wi_main_work = allscale::work_item_description<int32_t, __wi_main_name, allscale::no_serialization, __wi_main_variant_0, __wi_main_variant_1, __wi_main_can_split >;

/* ------- Function Definitions --------- */
int32_t main() {
    return allscale::runtime::main_wrapper<__wi_main_work >();
}

struct IMP_pi_pair;
typedef struct IMP_pi_pair IMP_pi_pair;

struct IMP_pi_pair {
    int32_t num;
    int32_t seed;;
    ;
    ;
};

struct __wi_allscale_wi_1_variant_1;
typedef struct __wi_allscale_wi_1_variant_1 __wi_allscale_wi_1_variant_1;

struct __wi_allscale_wi_1_can_split;
typedef struct __wi_allscale_wi_1_can_split __wi_allscale_wi_1_can_split;

struct __wi_allscale_wi_1_name {
    static const char* name() { return "__wi_allscale_wi_1"; }
};

struct __wi_allscale_wi_1_variant_0;
typedef struct __wi_allscale_wi_1_variant_0 __wi_allscale_wi_1_variant_0;

using __wi_allscale_wi_1_work = allscale::work_item_description<int32_t, __wi_allscale_wi_1_name, allscale::no_serialization, __wi_allscale_wi_1_variant_0, __wi_allscale_wi_1_variant_1, __wi_allscale_wi_1_can_split >;

ALLSCALE_REGISTER_TREETURE_TYPE(int32_t)
/* ------- Function Definitions --------- */
allscale::treeture<int32_t > allscale_fun_23(allscale::runtime::dependencies const& var_0, hpx::util::tuple<IMP_pi_pair > const& var_1) {
    return allscale::spawn_with_dependencies<__wi_allscale_wi_1_work >(var_0, hpx::util::get<0 >(var_1));
}
/* ------- Function Definitions --------- */
double IMP_compute() {
    int32_t var_0 = 0;
    std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<int64_t, std::ratio<1, 1000000000 > > > var_1 = (std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<int64_t, std::ratio<1, 1000000000 > > >&&)std::chrono::system_clock::now();
    srand((uint32_t)time((int64_t*)0));
    auto var_2 = allscale::runtime::make_prec_operation<IMP_pi_pair, int32_t >(INS_INIT(hpx::util::tuple< >){}, &allscale_fun_23);
    IMP_pi_pair var_3;
    var_3.num = 50000000;
    var_3.seed = rand();
    var_0 = var_2((IMP_pi_pair const&)var_3).get_result();
    var_0 = 0;
    std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<int64_t, std::ratio<1, 1000000000 > > > var_4 = (std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<int64_t, std::ratio<1, 1000000000 > > >&&)std::chrono::system_clock::now();
    std::chrono::duration<int64_t, std::ratio<1, 1000000000 > > var_5 = (std::chrono::duration<int64_t, std::ratio<1, 1000000000 > > const&)(var_4 - var_1);
    std::cout << var_5.count() << &std::endl;
    return (double)var_0 / (double)50000000 * (double)4;
}
/* ------- Function Definitions --------- */
int32_t IMP_main() {
    IMP_compute();
    return 0;
}
/* ------- Function Definitions --------- */
allscale::treeture<int32_t > allscale_fun_1(hpx::util::tuple< > const& var_0) {
    return allscale::treeture<int32_t >(IMP_main());
}
struct __wi_main_variant_0 {
    static allscale::treeture<int32_t > execute(hpx::util::tuple< > const& var_0);
    static constexpr bool valid = true;
};

/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
/* ------- Function Definitions --------- */
/* ------- Function Prototypes ---------- */
int32_t rec(hpx::util::tuple<IMP_pi_pair > const& p1);

/* ------- Function Definitions --------- */
allscale::treeture<int32_t > allscale_fun_35(hpx::util::tuple<IMP_pi_pair > const& var_0) {
    return allscale::treeture<int32_t >(rec(var_0));
}
struct __wi_allscale_wi_1_variant_1 {
    static allscale::treeture<int32_t > execute(hpx::util::tuple<IMP_pi_pair > const& var_0);
    static constexpr bool valid = true;
};

/* ------- Function Definitions --------- */
bool allscale_fun_37(hpx::util::tuple<IMP_pi_pair > const& var_0) {
    return (*(const IMP_pi_pair*)(&hpx::util::get<0 >(var_0))).num < 1;
}
/* ------- Function Definitions --------- */
int32_t allscale_fun_39(hpx::util::tuple<IMP_pi_pair > const& var_0) {
    return 0;
}
/* ------- Function Definitions --------- */
int32_t allscale_fun_40(hpx::util::tuple<IMP_pi_pair > const& var_0) {
    int32_t var_1 = 0;
    uint32_t var_2 = (uint32_t)(*(const IMP_pi_pair*)(&hpx::util::get<0 >(var_0))).seed;
    int32_t var_3 = (*(const IMP_pi_pair*)(&hpx::util::get<0 >(var_0))).num / 2;
    double var_4 = (double)rand_r(&var_2) / (double)2147483647;
    double var_5 = (double)rand_r(&var_2) / (double)2147483647;
    if (std::pow(var_4, 2) + std::pow(var_5, 2) <= (double)1) {
        var_1 = 1;
    };
    IMP_pi_pair var_6;
    IMP_pi_pair var_7;
    var_6.num = var_3;
    var_6.seed = (*(const IMP_pi_pair*)(&hpx::util::get<0 >(var_0))).seed << 1;
    var_7.num = (*(const IMP_pi_pair*)(&hpx::util::get<0 >(var_0))).num - var_3 - 1;
    var_7.seed = ((*(const IMP_pi_pair*)(&hpx::util::get<0 >(var_0))).seed << 1) + 1;
    int32_t var_8 = rec(INS_INIT(hpx::util::tuple<IMP_pi_pair >){(IMP_pi_pair const&)var_6});
    int32_t var_9 = rec(INS_INIT(hpx::util::tuple<IMP_pi_pair >){(IMP_pi_pair const&)var_7});
    return var_1 + var_8 + var_9;
}
/* ------- Function Definitions --------- */
int32_t rec(hpx::util::tuple<IMP_pi_pair > const& var_0) {
    if (allscale_fun_37(var_0)) {
        return allscale_fun_39(var_0);
    } else {
        return allscale_fun_40(var_0);
    };
}
allscale::treeture<int32_t > __wi_allscale_wi_1_variant_1::execute(hpx::util::tuple<IMP_pi_pair > const& var_0) {
    return allscale_fun_35(var_0);
}
/* ------- Function Definitions --------- */
bool allscale_fun_41(hpx::util::tuple<IMP_pi_pair > const& var_0) {
    return !allscale_fun_37(var_0);
}
struct __wi_allscale_wi_1_can_split {
    static bool call(hpx::util::tuple<IMP_pi_pair > const& var_0);
};

bool __wi_allscale_wi_1_can_split::call(hpx::util::tuple<IMP_pi_pair > const& var_0) {
    return allscale_fun_41(var_0);
}
/* ------- Function Definitions --------- */
allscale::treeture<int32_t > allscale_fun_27(hpx::util::tuple<IMP_pi_pair > const& var_0) {
    int32_t var_1 = 0;
    uint32_t var_2 = (uint32_t)(*(const IMP_pi_pair*)(&hpx::util::get<0 >(var_0))).seed;
    int32_t var_3 = (*(const IMP_pi_pair*)(&hpx::util::get<0 >(var_0))).num / 2;
    double var_4 = (double)rand_r(&var_2) / (double)2147483647;
    double var_5 = (double)rand_r(&var_2) / (double)2147483647;
    if (std::pow(var_4, 2) + std::pow(var_5, 2) <= (double)1) {
        var_1 = 1;
    };
    IMP_pi_pair var_6;
    IMP_pi_pair var_7;
    var_6.num = var_3;
    var_6.seed = (*(const IMP_pi_pair*)(&hpx::util::get<0 >(var_0))).seed << 1;
    var_7.num = (*(const IMP_pi_pair*)(&hpx::util::get<0 >(var_0))).num - var_3 - 1;
    var_7.seed = ((*(const IMP_pi_pair*)(&hpx::util::get<0 >(var_0))).seed << 1) + 1;
    allscale::treeture<int32_t > var_8 = allscale::spawn_with_dependencies<__wi_allscale_wi_1_work >(allscale::runtime::after(), (IMP_pi_pair const&)var_6);
    allscale::treeture<int32_t > var_9 = allscale::spawn_with_dependencies<__wi_allscale_wi_1_work >(allscale::runtime::after(), (IMP_pi_pair const&)var_7);
    return allscale::treeture<int32_t >(var_1 + var_8.get_result() + var_9.get_result());
}
/* ------- Function Definitions --------- */
allscale::treeture<int32_t > allscale_fun_25(hpx::util::tuple<IMP_pi_pair > const& var_0) {
    return allscale_fun_27(var_0);
}
struct __wi_allscale_wi_1_variant_0 {
    static allscale::treeture<int32_t > execute(hpx::util::tuple<IMP_pi_pair > const& var_0);
    static constexpr bool valid = true;
};

allscale::treeture<int32_t > __wi_allscale_wi_1_variant_0::execute(hpx::util::tuple<IMP_pi_pair > const& var_0) {
    return allscale_fun_25(var_0);
}
allscale::treeture<int32_t > __wi_main_variant_0::execute(hpx::util::tuple< > const& var_0) {
    return allscale_fun_1(var_0);
}
struct __wi_main_variant_1 {
    static allscale::treeture<int32_t > execute(hpx::util::tuple< > const& var_0);
    static constexpr bool valid = true;
};

allscale::treeture<int32_t > __wi_main_variant_1::execute(hpx::util::tuple< > const& var_0) {
    return allscale_fun_1(var_0);
}
/* ------- Function Definitions --------- */
bool allscale_fun_53(hpx::util::tuple< > const& var_0) {
    return (bool)false;
}
struct __wi_main_can_split {
    static bool call(hpx::util::tuple< > const& var_0);
};

bool __wi_main_can_split::call(hpx::util::tuple< > const& var_0) {
    return allscale_fun_53(var_0);
}
