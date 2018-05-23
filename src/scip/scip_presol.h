/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                           */
/*                  This file is part of the program and library             */
/*         SCIP --- Solving Constraint Integer Programs                      */
/*                                                                           */
/*    Copyright (C) 2002-2018 Konrad-Zuse-Zentrum                            */
/*                            fuer Informationstechnik Berlin                */
/*                                                                           */
/*  SCIP is distributed under the terms of the ZIB Academic License.         */
/*                                                                           */
/*  You should have received a copy of the ZIB Academic License              */
/*  along with SCIP; see the file COPYING. If not email to scip@zib.de.      */
/*                                                                           */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/**@file   scip_presol.h
 * @ingroup PUBLICCOREAPI
 * @brief  public methods for presolving plugins
 * @author Tobias Achterberg
 * @author Timo Berthold
 * @author Thorsten Koch
 * @author Alexander Martin
 * @author Marc Pfetsch
 * @author Kati Wolter
 * @author Gregor Hendel
 * @author Robert Lion Gottwald
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#ifndef __SCIP_SCIP_PRESOL_H__
#define __SCIP_SCIP_PRESOL_H__


#include <stdio.h>

#include "scip/def.h"
#include "blockmemshell/memory.h"
#include "scip/type_retcode.h"
#include "scip/type_result.h"
#include "scip/type_clock.h"
#include "scip/type_misc.h"
#include "scip/type_timing.h"
#include "scip/type_paramset.h"
#include "scip/type_event.h"
#include "scip/type_lp.h"
#include "scip/type_nlp.h"
#include "scip/type_var.h"
#include "scip/type_prob.h"
#include "scip/type_tree.h"
#include "scip/type_scip.h"

#include "scip/type_bandit.h"
#include "scip/type_branch.h"
#include "scip/type_conflict.h"
#include "scip/type_cons.h"
#include "scip/type_dialog.h"
#include "scip/type_disp.h"
#include "scip/type_heur.h"
#include "scip/type_compr.h"
#include "scip/type_history.h"
#include "scip/type_nodesel.h"
#include "scip/type_presol.h"
#include "scip/type_pricer.h"
#include "scip/type_reader.h"
#include "scip/type_relax.h"
#include "scip/type_sepa.h"
#include "scip/type_table.h"
#include "scip/type_prop.h"
#include "nlpi/type_nlpi.h"
#include "scip/type_concsolver.h"
#include "scip/type_syncstore.h"
#include "scip/type_benders.h"
#include "scip/type_benderscut.h"

/* In debug mode, we include the SCIP's structure in scip.c, such that no one can access
 * this structure except the interface methods in scip.c.
 * In optimized mode, the structure is included in scip.h, because some of the methods
 * are implemented as defines for performance reasons (e.g. the numerical comparisons).
 * Additionally, the internal "set.h" is included, such that the defines in set.h are
 * available in optimized mode.
 */
#ifdef NDEBUG
#include "scip/struct_scip.h"
#include "scip/struct_stat.h"
#include "scip/set.h"
#include "scip/tree.h"
#include "scip/misc.h"
#include "scip/var.h"
#include "scip/cons.h"
#include "scip/solve.h"
#include "scip/debug.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**@addtogroup PublicPresolverMethods
 *
 * @{
 */

/** creates a presolver and includes it in SCIP
 *
 *  @note method has all presolver callbacks as arguments and is thus changed every time a new
 *        callback is added
 *        in future releases; consider using SCIPincludePresolBasic() and setter functions
 *        if you seek for a method which is less likely to change in future releases
 */
EXTERN
SCIP_RETCODE SCIPincludePresol(
   SCIP*                 scip,               /**< SCIP data structure */
   const char*           name,               /**< name of presolver */
   const char*           desc,               /**< description of presolver */
   int                   priority,           /**< priority of the presolver (>= 0: before, < 0: after constraint handlers) */
   int                   maxrounds,          /**< maximal number of presolving rounds the presolver participates in (-1: no limit) */
   SCIP_PRESOLTIMING     timing,             /**< timing mask of the presolver */
   SCIP_DECL_PRESOLCOPY  ((*presolcopy)),    /**< copy method of presolver or NULL if you don't want to copy your plugin into sub-SCIPs */
   SCIP_DECL_PRESOLFREE  ((*presolfree)),    /**< destructor of presolver to free user data (called when SCIP is exiting) */
   SCIP_DECL_PRESOLINIT  ((*presolinit)),    /**< initialization method of presolver (called after problem was transformed) */
   SCIP_DECL_PRESOLEXIT  ((*presolexit)),    /**< deinitialization method of presolver (called before transformed problem is freed) */
   SCIP_DECL_PRESOLINITPRE((*presolinitpre)),/**< presolving initialization method of presolver (called when presolving is about to begin) */
   SCIP_DECL_PRESOLEXITPRE((*presolexitpre)),/**< presolving deinitialization method of presolver (called after presolving has been finished) */
   SCIP_DECL_PRESOLEXEC  ((*presolexec)),    /**< execution method of presolver */
   SCIP_PRESOLDATA*      presoldata          /**< presolver data */
   );

/** Creates a presolver and includes it in SCIP with its fundamental callback. All non-fundamental (or optional)
 *  callbacks as, e.g., init and exit callbacks, will be set to NULL. Optional callbacks can be set via specific setter
 *  functions. These are SCIPsetPresolCopy(), SCIPsetPresolFree(), SCIPsetPresolInit(), SCIPsetPresolExit(),
 *  SCIPsetPresolInitpre(), and SCIPsetPresolExitPre().
 *
 *  @note if you want to set all callbacks with a single method call, consider using SCIPincludePresol() instead
 */
EXTERN
SCIP_RETCODE SCIPincludePresolBasic(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_PRESOL**         presolptr,          /**< reference to presolver, or NULL */
   const char*           name,               /**< name of presolver */
   const char*           desc,               /**< description of presolver */
   int                   priority,           /**< priority of the presolver (>= 0: before, < 0: after constraint handlers) */
   int                   maxrounds,          /**< maximal number of presolving rounds the presolver participates in (-1: no limit) */
   SCIP_PRESOLTIMING     timing,             /**< timing mask of the presolver */
   SCIP_DECL_PRESOLEXEC  ((*presolexec)),    /**< execution method of presolver */
   SCIP_PRESOLDATA*      presoldata          /**< presolver data */
   );

/** sets copy method of presolver */
EXTERN
SCIP_RETCODE SCIPsetPresolCopy(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_PRESOL*          presol,             /**< presolver */
   SCIP_DECL_PRESOLCOPY  ((*presolcopy))     /**< copy method of presolver or NULL if you don't want to copy your plugin into sub-SCIPs */
   );

/** sets destructor method of presolver */
EXTERN
SCIP_RETCODE SCIPsetPresolFree(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_PRESOL*          presol,             /**< presolver */
   SCIP_DECL_PRESOLFREE ((*presolfree))      /**< destructor of presolver */
   );

/** sets initialization method of presolver */
EXTERN
SCIP_RETCODE SCIPsetPresolInit(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_PRESOL*          presol,             /**< presolver */
   SCIP_DECL_PRESOLINIT  ((*presolinit))     /**< initialize presolver */
   );

/** sets deinitialization method of presolver */
EXTERN
SCIP_RETCODE SCIPsetPresolExit(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_PRESOL*          presol,             /**< presolver */
   SCIP_DECL_PRESOLEXIT  ((*presolexit))     /**< deinitialize presolver */
   );

/** sets solving process initialization method of presolver */
EXTERN
SCIP_RETCODE SCIPsetPresolInitpre(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_PRESOL*          presol,             /**< presolver */
   SCIP_DECL_PRESOLINITPRE ((*presolinitpre))/**< solving process initialization method of presolver */
   );

/** sets solving process deinitialization method of presolver */
SCIP_RETCODE SCIPsetPresolExitpre(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_PRESOL*          presol,             /**< presolver */
   SCIP_DECL_PRESOLEXITPRE ((*presolexitpre))/**< solving process deinitialization method of presolver */
   );

/** returns the presolver of the given name, or NULL if not existing */
EXTERN
SCIP_PRESOL* SCIPfindPresol(
   SCIP*                 scip,               /**< SCIP data structure */
   const char*           name                /**< name of presolver */
   );

/** returns the array of currently available presolvers */
EXTERN
SCIP_PRESOL** SCIPgetPresols(
   SCIP*                 scip                /**< SCIP data structure */
   );

/** returns the number of currently available presolvers */
EXTERN
int SCIPgetNPresols(
   SCIP*                 scip                /**< SCIP data structure */
   );

/** sets the priority of a presolver */
EXTERN
SCIP_RETCODE SCIPsetPresolPriority(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_PRESOL*          presol,             /**< presolver */
   int                   priority            /**< new priority of the presolver */
   );

/* @} */

#ifdef __cplusplus
}
#endif

#endif
