#ifndef _DAO_SCRIPTING
#define _DAO_SCRIPTING

#include <wex/lkscript.h>

//extern ssc_bool_t ssc_progress_handler(ssc_module_t, ssc_handler_t, int action, float f0, float f1, const char *s0, const char *, void *);

extern void _test(lk::invoke_t &cxt);
extern void _var(lk::invoke_t &cxt);
extern void _power_cycle(lk::invoke_t &cxt);
extern void _simulate_optical(lk::invoke_t &cxt);
extern void _simulate_solarfield(lk::invoke_t &cxt);
extern void _simulate_performance(lk::invoke_t &cxt);
extern void _initialize(lk::invoke_t &cxt);

#endif