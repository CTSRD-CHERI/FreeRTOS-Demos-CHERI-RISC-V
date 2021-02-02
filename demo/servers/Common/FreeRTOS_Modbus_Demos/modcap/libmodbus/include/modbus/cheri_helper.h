#ifndef _CHERI_HELPER_H_
#define _CHERI_HELPER_H_

#ifndef __has_feature
#define __has_feature(x) 0
#endif

#if __has_feature(capabilities)
#include <cheriintrin.h>

#define CHERI_PRINT_CAP(cap)                         \
        printf("%-20s: %-16s t:%d s:%d p:%08x "      \
               "b:%016jx l:%016zx o:%jx type:%lx\n", \
               __func__,                             \
               #cap,                                 \
               cheri_tag_get(cap),                   \
               cheri_is_sealed(cap),                 \
               cheri_perms_get(cap),                 \
               cheri_base_get(cap),                  \
               cheri_length_get(cap),                \
               cheri_offset_get(cap),                \
               cheri_type_get(cap))

#define CHERI_PRINT_CAP_LITE(cap)                    \
        printf("t:%d s:%d p:%08x "                   \
               "b:%016jx l:%016zx o:%jx type:%lx\n", \
               cheri_tag_get(cap),                   \
               cheri_is_sealed(cap),                 \
               cheri_perms_get(cap),                 \
               cheri_base_get(cap),                  \
               cheri_length_get(cap),                \
               cheri_offset_get(cap),                \
               cheri_type_get(cap))

#define CHERI_EXPAND_PERMS(cap)                                              \
        printf(                                                              \
            "CHERI_PERM_GLOBAL:\t\t%d\n"                                     \
            "CHERI_PERM_EXECUTE:\t\t%d\n"                                    \
            "CHERI_PERM_LOAD:\t\t%d\n"                                       \
            "CHERI_PERM_STORE:\t\t%d\n"                                      \
            "CHERI_PERM_LOAD_CAP:\t\t%d\n"                                   \
            "CHERI_PERM_STORE_CAP:\t\t%d\n"                                  \
            "CHERI_PERM_STORE_LOCAL_CAP:\t%d\n"                              \
            "CHERI_PERM_SEAL:\t\t%d\n"                                       \
            "CHERI_PERM_CCALL:\t\t%d\n"                                      \
            "CHERI_PERM_UNSEAL:\t\t%d\n"                                     \
            "CHERI_PERM_SYSTEM_REGS:\t\t%d\n",                               \
            (cheri_perms_get(cap) & CHERI_PERM_GLOBAL) > 0 ? 1 : 0,          \
            (cheri_perms_get(cap) & CHERI_PERM_EXECUTE) > 0 ? 1 : 0,         \
            (cheri_perms_get(cap) & CHERI_PERM_LOAD) > 0 ? 1 : 0,            \
            (cheri_perms_get(cap) & CHERI_PERM_STORE) > 0 ? 1 : 0,           \
            (cheri_perms_get(cap) & CHERI_PERM_LOAD_CAP) > 0 ? 1 : 0,        \
            (cheri_perms_get(cap) & CHERI_PERM_STORE_CAP) > 0 ? 1 : 0,       \
            (cheri_perms_get(cap) & CHERI_PERM_STORE_LOCAL_CAP) > 0 ? 1 : 0, \
            (cheri_perms_get(cap) & CHERI_PERM_SEAL) > 0 ? 1 : 0,            \
            (cheri_perms_get(cap) & CHERI_PERM_CCALL) > 0 ? 1 : 0,           \
            (cheri_perms_get(cap) & CHERI_PERM_UNSEAL) > 0 ? 1 : 0,          \
            (cheri_perms_get(cap) & CHERI_PERM_SYSTEM_REGS) > 0 ? 1 : 0)
#else
#define CHERI_PRINT_CAP(cap) printf("b:%jx\n", (uintmax_t)cap)
#define CHERI_PRINT_CAP_LITE(cap) CHERI_PRINT_CAP(cap)
#define CHERI_EXPAND_PERMS(cap) CHERI_PRINT_CAP(cap)
#endif /* __has_feature(capabilities) */

#endif /* _CHERI_HELPER_H_ */
