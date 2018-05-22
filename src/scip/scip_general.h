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

/**@file   scip_general.h
 * @ingroup PUBLICCOREAPI
 * @brief  general public methods
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

#ifndef __SCIP_SCIP_GENERAL_H__
#define __SCIP_SCIP_GENERAL_H__


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

/**@addtogroup MiscellaneousMethods
 *
 * @{
 */

/** returns complete SCIP version number in the format "major . minor tech"
 *
 *  @return complete SCIP version
 */
EXTERN
SCIP_Real SCIPversion(
   void
   );

/** returns SCIP major version
 *
 *  @return major SCIP version
 */
EXTERN
int SCIPmajorVersion(
   void
   );

/** returns SCIP minor version
 *
 *  @return minor SCIP version
 */
EXTERN
int SCIPminorVersion(
   void
   );

/** returns SCIP technical version
 *
 *  @return technical SCIP version
 */
EXTERN
int SCIPtechVersion(
   void
   );

/** returns SCIP sub version number
 *
 *  @return subversion SCIP version
 */
EXTERN
int SCIPsubversion(
   void
   );

/** prints a version information line to a file stream via the message handler system
 *
 *  @note If the message handler is set to a NULL pointer nothing will be printed
 */
EXTERN
void SCIPprintVersion(
   SCIP*                 scip,               /**< SCIP data structure */
   FILE*                 file                /**< output file (or NULL for standard output) */
   );

/** prints detailed information on the compile-time flags
 *
 *  @note If the message handler is set to a NULL pointer nothing will be printed
 */
EXTERN
void SCIPprintBuildOptions(
   SCIP*                 scip,               /**< SCIP data structure */
   FILE*                 file                /**< output file (or NULL for standard output) */
   );

/** prints error message for the given SCIP_RETCODE via the error prints method */
EXTERN
void SCIPprintError(
   SCIP_RETCODE          retcode             /**< SCIP return code causing the error */
   );

/**@} */

/**@addtogroup GeneralSCIPMethods
 *
 * @{
 */

/** creates and initializes SCIP data structures
 *
 *  @note The SCIP default message handler is installed. Use the method SCIPsetMessagehdlr() to install your own
 *        message handler or SCIPsetMessagehdlrLogfile() and SCIPsetMessagehdlrQuiet() to write into a log
 *        file and turn off/on the display output, respectively.
 *
 *  @return \ref SCIP_OKAY is returned if everything worked. Otherwise a suitable error code is passed. See \ref
 *          SCIP_Retcode "SCIP_RETCODE" for a complete list of error codes.
 *
 *  @post After calling this method @p scip reached the solving stage \ref SCIP_STAGE_INIT
 *
 *  See \ref SCIP_Stage "SCIP_STAGE" for a complete list of all possible solving stages.
 */
EXTERN
SCIP_RETCODE SCIPcreate(
   SCIP**                scip                /**< pointer to SCIP data structure */
   );

/** frees SCIP data structures
 *
 *  @return \ref SCIP_OKAY is returned if everything worked. Otherwise a suitable error code is passed. See \ref
 *          SCIP_Retcode "SCIP_RETCODE" for a complete list of error codes.
 *
 *  @pre This method can be called if @p scip is in one of the following stages:
 *       - \ref SCIP_STAGE_INIT
 *       - \ref SCIP_STAGE_PROBLEM
 *       - \ref SCIP_STAGE_TRANSFORMED
 *       - \ref SCIP_STAGE_INITPRESOLVE
 *       - \ref SCIP_STAGE_PRESOLVING
 *       - \ref SCIP_STAGE_PRESOLVED
 *       - \ref SCIP_STAGE_EXITPRESOLVE
 *       - \ref SCIP_STAGE_SOLVING
 *       - \ref SCIP_STAGE_SOLVED
 *       - \ref SCIP_STAGE_FREE
 *
 *  @post After calling this method \SCIP reached the solving stage \ref SCIP_STAGE_FREE
 *
 *  See \ref SCIP_Stage "SCIP_STAGE" for a complete list of all possible solving stages.
 */
EXTERN
SCIP_RETCODE SCIPfree(
   SCIP**                scip                /**< pointer to SCIP data structure */
   );

/** returns current stage of SCIP
 *
 *  @return the current SCIP stage
 *
 *  See \ref SCIP_Stage "SCIP_STAGE" for a complete list of all possible solving stages.
 */
EXTERN
SCIP_STAGE SCIPgetStage(
   SCIP*                 scip                /**< SCIP data structure */
   );

/** outputs SCIP stage and solution status if applicable via the message handler
 *
 *  @note If the message handler is set to a NULL pointer nothing will be printed
 *
 *  @note If limits have been changed between the solution and the call to this function, the status is recomputed and
 *        thus may to correspond to the original status.
 *
 *  @return \ref SCIP_OKAY is returned if everything worked. Otherwise a suitable error code is passed. See \ref
 *          SCIP_Retcode "SCIP_RETCODE" for a complete list of error codes.
 *
 *  See \ref SCIP_Stage "SCIP_STAGE" for a complete list of all possible solving stages.
 */
EXTERN
SCIP_RETCODE SCIPprintStage(
   SCIP*                 scip,               /**< SCIP data structure */
   FILE*                 file                /**< output file (or NULL for standard output) */
   );

/** gets solution status
 *
 *  @return SCIP solution status
 *
 *  See \ref SCIP_Status "SCIP_STATUS" for a complete list of all possible solving status.
 */
EXTERN
SCIP_STATUS SCIPgetStatus(
   SCIP*                 scip                /**< SCIP data structure */
   );

/** outputs solution status
 *
 *  @return \ref SCIP_OKAY is returned if everything worked. Otherwise a suitable error code is passed. See \ref
 *          SCIP_Retcode "SCIP_RETCODE" for a complete list of error codes.
 *
 *  See \ref SCIP_Status "SCIP_STATUS" for a complete list of all possible solving status.
 */
EXTERN
SCIP_RETCODE SCIPprintStatus(
   SCIP*                 scip,               /**< SCIP data structure */
   FILE*                 file                /**< output file (or NULL for standard output) */
   );

/** returns whether the current stage belongs to the transformed problem space
 *
 *  @return Returns TRUE if the \SCIP instance is transformed, otherwise FALSE
 */
EXTERN
SCIP_Bool SCIPisTransformed(
   SCIP*                 scip                /**< SCIP data structure */
   );

/** returns whether the solution process is arithmetically exact, i.e., not subject to roundoff errors
 *
 *  @note This feature is not supported yet!
 *
 *  @return Returns TRUE if \SCIP is exact solving mode, otherwise FALSE
 */
EXTERN
SCIP_Bool SCIPisExactSolve(
   SCIP*                 scip                /**< SCIP data structure */
   );

/** returns whether the presolving process would be finished given no more presolving reductions are found in this
 *  presolving round
 *
 *  Checks whether the number of presolving rounds is not exceeded and the presolving reductions found in the current
 *  presolving round suffice to trigger another presolving round.
 *
 *  @note if subsequent presolvers find more reductions, presolving might continue even if the method returns FALSE
 *  @note does not check whether infeasibility or unboundedness was already detected in presolving (which would result
 *        in presolving being stopped although the method returns TRUE)
 *
 *  @return Returns TRUE if presolving is finished if no further reductions are detected
 */
EXTERN
SCIP_Bool SCIPisPresolveFinished(
   SCIP*                 scip                /**< SCIP data structure */
   );

/** returns whether SCIP has performed presolving during the last solve
 *
 *  @return Returns TRUE if presolving was performed during the last solve
 */
EXTERN
SCIP_Bool SCIPhasPerformedPresolve(
   SCIP*                 scip                /**< SCIP data structure */
   );

/** returns whether the user pressed CTRL-C to interrupt the solving process
 *
 *  @return Returns TRUE if Ctrl-C was pressed, otherwise FALSE.
 */
EXTERN
SCIP_Bool SCIPpressedCtrlC(
   SCIP*                 scip                /**< SCIP data structure */
   );

/** returns whether the solving process should be / was stopped before proving optimality;
 *  if the solving process should be / was stopped, the status returned by SCIPgetStatus() yields
 *  the reason for the premature abort
 *
 *  @return Returns TRUE if solving process is stopped/interrupted, otherwise FALSE.
 */
EXTERN
SCIP_Bool SCIPisStopped(
   SCIP*                 scip                /**< SCIP data structure */
   );

/**@} */

/**@addtogroup PublicExternalCodeMethods
 *
 * @{
 */



/** includes information about an external code linked into the SCIP library */
EXTERN
SCIP_RETCODE SCIPincludeExternalCodeInformation(
   SCIP*                 scip,               /**< SCIP data structure */
   const char*           name,               /**< name of external code */
   const char*           description         /**< description of external code, or NULL */
   );

/** returns an array of names of currently included external codes */
EXTERN
char** SCIPgetExternalCodeNames(
   SCIP*                 scip                /**< SCIP data structure */
   );

/** returns an array of the descriptions of currently included external codes
 *
 *  @note some descriptions may be NULL
 */
EXTERN
char** SCIPgetExternalCodeDescriptions(
   SCIP*                 scip                /**< SCIP data structure */
   );

/** returns the number of currently included information on external codes */
EXTERN
int SCIPgetNExternalCodes(
   SCIP*                 scip                /**< SCIP data structure */
   );

/** prints information on external codes to a file stream via the message handler system
 *
 *  @note If the message handler is set to a NULL pointer nothing will be printed
 */
EXTERN
void SCIPprintExternalCodes(
   SCIP*                 scip,               /**< SCIP data structure */
   FILE*                 file                /**< output file (or NULL for standard output) */
   );

/* @} */

#ifdef __cplusplus
}
#endif

#endif