// disable intrinsics if not supported
#define Lib_IntVector_Intrinsics_vec128 void *
#if !defined(HACL_CAN_COMPILE_VEC256)
#define Lib_IntVector_Intrinsics_vec256 void *
#endif
