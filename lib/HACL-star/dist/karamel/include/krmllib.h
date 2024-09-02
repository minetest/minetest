#ifndef __KRMLLIB_H
#define __KRMLLIB_H

/******************************************************************************/
/* The all-in-one krmllib.h header                                            */
/******************************************************************************/

/* This is a meta-header that is included by default in KaRaMeL generated
 * programs. If you wish to have a more lightweight set of headers, or are
 * targeting an environment where controlling these macros yourself is
 * important, consider using:
 *
 *   krml -minimal
 *
 * to disable the inclusion of this file (note: this also disables the default
 * argument "-bundle FStar.*"). You can then include the headers of your choice
 * one by one, using -add-early-include. */

#include "krml/internal/target.h"
#include "krml/internal/callconv.h"
#include "krml/internal/builtin.h"
#include "krml/internal/debug.h"
#include "krml/internal/types.h"

#include "krml/lowstar_endianness.h"
#include "krml/fstar_int.h"

#endif     /* __KRMLLIB_H */
