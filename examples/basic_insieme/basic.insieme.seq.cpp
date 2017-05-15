/**
 * ------------------------ Auto-generated Code ------------------------ 
 *           This code was generated by the Insieme Compiler 
 * --------------------------------------------------------------------- 
 */
#include <alloca.h>
#include <allscale/runtime.hpp>
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

struct __wi_main_variant_1;
typedef struct __wi_main_variant_1 __wi_main_variant_1;

struct __wi_main_name {
    static const char* name() { return "__wi_main"; }
};

using __wi_main_work = allscale::work_item_description<int32_t, __wi_main_name, allscale::no_serialization, __wi_main_variant_0, __wi_main_variant_1 >;

/* ------- Function Definitions --------- */
int32_t main() {
    return allscale::runtime::main_wrapper<__wi_main_work >();
}

ALLSCALE_REGISTER_TREETURE_TYPE(int32_t)
struct __wi_allscale_wi_0_variant_1;
typedef struct __wi_allscale_wi_0_variant_1 __wi_allscale_wi_0_variant_1;

struct __wi_allscale_wi_0_variant_0;
typedef struct __wi_allscale_wi_0_variant_0 __wi_allscale_wi_0_variant_0;

struct __wi_allscale_wi_0_name {
    static const char* name() { return "__wi_allscale_wi_0"; }
};

using __wi_allscale_wi_0_work = allscale::work_item_description<int32_t, __wi_allscale_wi_0_name, allscale::no_serialization, __wi_allscale_wi_0_variant_0, __wi_allscale_wi_0_variant_1 >;

/* ------- Function Definitions --------- */
allscale::treeture<int32_t > allscale_fun_8(allscale::runtime::dependencies var_0, int32_t var_1) {
    return allscale::spawn<__wi_allscale_wi_0_work >(var_1);
}
struct allscale_type_6;
typedef struct allscale_type_6 allscale_type_6;

struct allscale_type_6 {
    allscale::treeture<int32_t >(* call)(allscale_type_6*,allscale::runtime::dependencies,int32_t);;
};

static inline allscale_type_6* allscale_type_6_ctr(allscale_type_6* target, allscale::treeture<int32_t >(* call)(allscale_type_6*,allscale::runtime::dependencies,int32_t)) {
    *target = INS_INIT(allscale_type_6){call};
    return target;
}
typedef allscale::treeture<int32_t > allscale_type_7(allscale::runtime::dependencies,int32_t);

/* -- Begin - Bind Constructs ------------------------------------------------------------ */
struct allscale_closure_5_closure;
typedef struct allscale_closure_5_closure allscale_closure_5_closure;

struct allscale_closure_5_closure {
    allscale::treeture<int32_t >(* call)(allscale_closure_5_closure*,allscale::runtime::dependencies,int32_t);
    allscale_type_7* nested;;
};

allscale::treeture<int32_t > allscale_closure_5_mapper(allscale_closure_5_closure* closure, allscale::runtime::dependencies c1, int32_t c2);

static inline allscale_type_6* allscale_closure_5_ctr(allscale_closure_5_closure* closure, allscale_type_7* nested) {
    INS_INPLACE_INIT(closure,allscale_closure_5_closure){&allscale_closure_5_mapper, nested};
    return (allscale_type_6*)closure;
}
/* --  End  - Bind Constructs ------------------------------------------------------------ */
/* ------- Function Definitions --------- */
int32_t IMP_main() {
    auto var_2 = allscale::runtime::make_prec_operation<int32_t, int32_t >(allscale::runtime::make_insieme_lambda_wrapper(allscale_closure_5_ctr((allscale_closure_5_closure*)alloca(sizeof(allscale_closure_5_closure)), &allscale_fun_8)));
    var_2(10).get_result();
    return 0;
}
/* ------- Function Definitions --------- */
allscale::treeture<int32_t > allscale_fun_2(hpx::util::tuple< > const& var_0) {
    return allscale::treeture<int32_t >(IMP_main());
}
struct __wi_main_variant_0 {
    static allscale::treeture<int32_t > execute(hpx::util::tuple< > const& var_0);
    static constexpr bool valid = true;
};

/* ------- Function Definitions --------- */
int32_t allscale_fun_19(hpx::util::tuple<int32_t > const& var_0) {
    int32_t var_1 = hpx::util::get<0 >(var_0);
    {
        return 1;
    };
}
/* ------- Function Definitions --------- */
int32_t allscale_fun_15(hpx::util::tuple<int32_t > const& var_0) {
    int32_t var_1 = hpx::util::get<0 >(var_0);
    int32_t var_2 = var_1;
    {
        return var_2;
    };
}
/* ------- Function Definitions --------- */
bool allscale_fun_13(hpx::util::tuple<int32_t > const& var_0) {
    int32_t var_1 = hpx::util::get<0 >(var_0);
    int32_t var_2 = var_1;
    {
        return var_2 < 2;
    };
}
/* ------- Function Definitions --------- */
int32_t rec(hpx::util::tuple<int32_t > const& var_0) {
    if (allscale_fun_13(var_0)) {
        return allscale_fun_15(var_0);
    } else {
        return allscale_fun_19(var_0);
    };
}
/* ------- Function Definitions --------- */
allscale::treeture<int32_t > allscale_fun_18(hpx::util::tuple<int32_t > const& var_0) {
    return allscale::treeture<int32_t >(rec(var_0));
}
struct __wi_allscale_wi_0_variant_1 {
    static allscale::treeture<int32_t > execute(hpx::util::tuple<int32_t > const& var_0);
    static constexpr bool valid = true;
};

allscale::treeture<int32_t > __wi_allscale_wi_0_variant_1::execute(hpx::util::tuple<int32_t > const& var_0) {
    return allscale_fun_18(var_0);
}
/* ------- Function Definitions --------- */
allscale::treeture<int32_t > allscale_fun_17(hpx::util::tuple<int32_t > const& var_0) {
    int32_t var_1 = hpx::util::get<0 >(var_0);
    {
        return allscale::treeture<int32_t >(1);
    };
}
/* ------- Function Definitions --------- */
allscale::treeture<int32_t > allscale_fun_11(hpx::util::tuple<int32_t > const& var_0) {
    if (allscale_fun_13(var_0)) {
        return allscale::treeture<int32_t >(allscale_fun_15(var_0));
    } else {
        return allscale_fun_17(var_0);
    };
}
struct __wi_allscale_wi_0_variant_0 {
    static allscale::treeture<int32_t > execute(hpx::util::tuple<int32_t > const& var_0);
    static constexpr bool valid = true;
};

allscale::treeture<int32_t > __wi_allscale_wi_0_variant_0::execute(hpx::util::tuple<int32_t > const& var_0) {
    return allscale_fun_11(var_0);
}
allscale::treeture<int32_t > allscale_closure_5_mapper(allscale_closure_5_closure* closure, allscale::runtime::dependencies c1, int32_t c2) {
    return closure->nested(c1, c2);
}
allscale::treeture<int32_t > __wi_main_variant_0::execute(hpx::util::tuple< > const& var_0) {
    return allscale_fun_2(var_0);
}
struct __wi_main_variant_1 {
    static allscale::treeture<int32_t > execute(hpx::util::tuple< > const& var_0);
    static constexpr bool valid = true;
};

allscale::treeture<int32_t > __wi_main_variant_1::execute(hpx::util::tuple< > const& var_0) {
    return allscale_fun_2(var_0);
}

