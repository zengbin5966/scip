/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                           */
/*                  This file is part of the program and library             */
/*         SCIP --- Solving Constraint Integer Programs                      */
/*                                                                           */
/*    Copyright (C) 2002-2013 Konrad-Zuse-Zentrum                            */
/*                            fuer Informationstechnik Berlin                */
/*                                                                           */
/*  SCIP is distributed under the terms of the ZIB Academic License.         */
/*                                                                           */
/*  You should have received a copy of the ZIB Academic License              */
/*  along with SCIP; see the file COPYING. If not email to scip@zib.de.      */
/*                                                                           */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/**@file   reopt.h
 * @brief  internal methods for collecting primal CIP solutions and primal informations
 * @author Jakob Witzig
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#ifndef __SCIP_REOPT_H__
#define __SCIP_REOPT_H__


#include "scip/def.h"
#include "blockmemshell/memory.h"
#include "scip/type_retcode.h"
#include "scip/type_reopt.h"

#include "scip/struct_reopt.h"

#ifdef __cplusplus
extern "C" {
#endif

/** creates reopt data */
extern
SCIP_RETCODE SCIPreoptCreate(
   SCIP_REOPT**          reopt,                   /**< pointer to primal data */
   SCIP_SET*             set,                     /**< global SCIP settings */
   BMS_BLKMEM*           blkmem                   /**< block memory */
   );

/** frees reopt data */
extern
SCIP_RETCODE SCIPreoptFree(
   SCIP*                 scip,                    /**< scip data */
   SCIP_REOPT**          reopt,                   /**< pointer to primal data */
   BMS_BLKMEM*           blkmem                   /**< block memory */
   );

/* returns the number constraints added by reoptimization plug-in */
extern
int SCIPreoptGetNAddedConss(
   SCIP_REOPT*           reopt,
   SCIP_NODE*            node
   );

/** add a solution to sols */
extern
SCIP_RETCODE SCIPreoptAddSol(
   SCIP*                 scip,
   SCIP_REOPT*           reopt,
   SCIP_SET*             set,
   SCIP_STAT*            stat,
   SCIP_SOL*             sol,
   SCIP_Bool             bestsol,
   SCIP_Bool*            added,
   int                   run
   );

/* add optimal solution */
extern
SCIP_RETCODE SCIPreoptAddOptSol(
   SCIP*                 scip,                    /**< SCIP data structure */
   SCIP_REOPT*           reopt,                   /**< reopt data */
   SCIP_SOL*             sol                      /**< solution to add */
   );

/* add a run */
extern
SCIP_RETCODE SCIPreoptAddRun(
   SCIP*                 scip,
   SCIP_SET*             set,
   SCIP_REOPT*           reopt,
   BMS_BLKMEM*           blkmem,
   int                   run,
   int                   size
   );

/* get the number of checked during the reoptimization process */
extern
int SCIPreoptGetNCheckedsols(
   SCIP_REOPT*           reopt
   );

/* update the number of checked during the reoptimization process */
extern
void SCIPreoptSetNCheckedsols(
   SCIP_REOPT*           reopt,
   int                   ncheckedsols
   );

/* get the number of checked during the reoptimization process */
extern
int SCIPreoptGetNImprovingsols(
   SCIP_REOPT*           reopt
   );

/* update the number of checked during the reoptimization process */
extern
void SCIPreoptSetNImprovingsols(
   SCIP_REOPT*           reopt,
   int                   nimprovingsols
   );

/* returns number of solution */
extern
int SCIPreoptGetNSolsRun(
   SCIP_REOPT*           reopt,
   int                   run
   );

/* returns number of all solutions */
extern
int SCIPreoptGetNSols(
   SCIP_REOPT*           reopt
   );

/* return the stored solutions of a given run */
extern
SCIP_RETCODE SCIPreoptGetSolsRun(
   SCIP_REOPT*           reopt,
   int                   run,
   SCIP_SOL**            sols,
   int                   allocmem,
   int*                  nsols
   );

/* returns the number of saved solutions overall runs */
extern
int SCIPreoptNSavedSols(
   SCIP_REOPT*           reopt
   );

/* returns the number of reused sols over all runs */
extern
int SCIPreoptNUsedSols(
   SCIP_REOPT*           reopt
   );

/* save objective function */
extern
SCIP_RETCODE SCIPreoptSaveNewObj(
      SCIP*                 scip,                    /**< SCIP data structure */
      SCIP_REOPT*           reopt,                   /**< reopt data */
      SCIP_SET*             set,                     /**< global SCIP settings */
      BMS_BLKMEM*           blkmem                   /**< block memory */
   );

/* check if the current and the previous objective are similar enough
 * returns TRUE if we want to restart, otherwise FALSE */
extern
SCIP_RETCODE SCIPreoptCheckRestart(
   SCIP_REOPT*           reopt,
   SCIP_SET*             set,
   BMS_BLKMEM*           blkmem
   );

/* returns an array of indices with similar objective functions
 * to obj_idx */
extern
int* SCIPreoptGetSimilarityIdx(
   SCIP*                 scip,
   SCIP_REOPT*           reopt,
   int                   obj_id,
   int*                  sim_ids,
   int*                  nids
   );

/*
 * returns the similarity to the previous objective function
 */
extern
SCIP_Real SCIPreoptGetSimToPrevious(
      SCIP_REOPT*        reopt
   );

/*
 * returns the similarity to the first objective function
 */
extern
SCIP_Real SCIPreoptGetSimToFirst(
      SCIP_REOPT*        reopt
   );

/*
 * return the similarity between two of objective functions of two given runs
 */
extern
SCIP_Real SCIPreoptGetSim(
   SCIP_REOPT*           reopt,                   /**< reopt data */
   int                   run1,
   int                   run2
   );

/*
 * returns the best solution of the last run
 */
extern
SCIP_SOL* SCIPreoptGetLastBestSol(
   SCIP_REOPT*           reopt
   );

/*
 * returns the coefficent of variable with index @param idx in run @param run
 */
extern
SCIP_Real SCIPreoptGetObjCoef(
   SCIP_REOPT*           reopt,
   int                   run,
   int                   idx
   );

/* checks the changes of the objective coefficients */
extern
void SCIPreoptGetVarCoefChg(
   SCIP_REOPT*           reopt,
   int                   varidx,
   SCIP_Bool*            negated,
   SCIP_Bool*            entering,
   SCIP_Bool*            leaving
   );

/*
 * print the optimal solutions of all previous runs
 */
extern
SCIP_RETCODE SCIPreoptPrintOptSols(
   SCIP*                 scip,
   SCIP_REOPT*           reopt
   );

/*
 * return all optimal solutions of the previous runs
 * depending on the current stage the method copies the solutions into
 * the origprimal or primal space. That means, all solutions need to be
 * freed before starting a new iteration!!!
 */
extern
SCIP_RETCODE SCIPreoptGetOptSols(
   SCIP*                 scip,
   SCIP_REOPT*           reopt,
   SCIP_SOL**            sols,
   int*                  nsols
   );

/* reset marks of stored solutions to not updated */
extern
void SCIPreoptResetSolMarks(
   SCIP_REOPT*           reopt
   );

/* returns the number of stored nodes */
extern
int SCIPreoptGetNNodes(
   SCIP_REOPT*           reopt
   );

/*
 *  Save information if infeasible nodes
 */
extern
SCIP_RETCODE SCIPreoptAddInfNode(
   SCIP*                 scip,
   SCIP_REOPT*           reopt,
   SCIP_NODE*            node
   );

/**
 * check the reason for cut off a node and if necessary store the node
 */
extern
SCIP_RETCODE SCIPreoptCheckCutoff(
   SCIP*                 scip,
   SCIP_REOPT*           reopt,
   SCIP_NODE*            node,
   SCIP_EVENT*           event
   );

/** store bound changes based on dual information */
extern
SCIP_RETCODE SCIPreoptAddDualBndchg(
   SCIP*                 scip,
   SCIP_REOPT*           reopt,
   SCIP_NODE*            node,
   SCIP_VAR*             var,
   SCIP_Real             newval,
   SCIP_Real             oldval
   );

/* returns the number of bound changes based on dual information */
extern
int SCIPreoptGetNDualBndchs(
   SCIP_REOPT*           reopt,
   SCIP_NODE*            node
   );

/* returns the number of child nodes */
extern
int SCIPreoptNChilds(
   SCIP_REOPT*           reopt,
   SCIP_NODE*            node
   );

/* checks if the reoptimization process should be restarted, i.e.,
 * the next problem should be solved from scratch */
extern
SCIP_RETCODE SCIPreoptRestart(
   SCIP_REOPT*           reopt,
   BMS_BLKMEM*           blkmem
   );

/* returns the child nodes of @param node that need to be
 * reoptimized next or NULL if @param node is a leaf */
extern
SCIP_RETCODE SCIPreoptGetNodeIDsToReoptimize(
   SCIP_REOPT*           reopt,
   SCIP*                 scip,
   SCIP_NODE*            node,
   int*                  childs,
   int                   mem,
   int*                  nchilds
   );

/* returns the time needed to store the nodes */
extern
SCIP_Real SCIPreoptGetSavingtime(
   SCIP_REOPT*           reopt
   );

/* store a global constraint that should be added at the beginning of the next iteration */
extern
SCIP_RETCODE SCIPreoptAddGlbCons(
   SCIP_REOPT*           reopt,
   LOGICORDATA*          consdata,
   BMS_BLKMEM*           blkmem
   );

/* add the stored constraints globally to the problem */
extern
SCIP_RETCODE SCIPreoptApplyGlbConss(
   SCIP*                 scip,
   SCIP_REOPT*           reopt
   );

extern
SCIP_RETCODE SCIPreoptAddGlbSolCons(
   SCIP_REOPT*           reopt,
   SCIP_SOL*             sol,
   SCIP_VAR**            vars,
   SCIP_SET*             set,
   SCIP_STAT*            stat,
   BMS_BLKMEM*           blkmem,
   int                   nvars
   );

extern
SCIP_RETCODE SCIPreoptGetSolveLP(
   SCIP_REOPT*           reopt,
   SCIP_NODE*            node,
   SCIP_Bool*            solvelp
   );

/* returns the reopttype of a node stored at ID id */
extern
SCIP_REOPTTYPE SCIPreoptnodeGetType(
   SCIP_REOPT*           reopt,
   int                   id
   );

/* reoptimize the node stored at ID id */
extern
SCIP_RETCODE SCIPreoptApply(
   SCIP*                 scip,
   SCIP_REOPT*           reopt,
   SCIP_NODE*            node_fix,
   SCIP_NODE*            node_cons,
   int                   id,
   BMS_BLKMEM*           blkmem
   );

/* delete a node stored in the reopttree */
extern
SCIP_RETCODE SCIPreopttreeDeleteNode(
   SCIP_REOPT*           reopt,
   int                   id,
   BMS_BLKMEM*           blkmem
   );

/* replace the node stored at ID id by its child nodes */
extern
SCIP_RETCODE SCIPreoptShrinkNode(
   SCIP*                 scip,
   SCIP_REOPT*           reopt,
   BMS_BLKMEM*           blkmem,
   int                   id
   );

/* return the branching path stored at ID id */
extern
void SCIPreoptnodeGetPath(
   SCIP_REOPT*           reopt,
   int                   id,
   SCIP_VAR**            vars,
   SCIP_Real*            vals,
   SCIP_BOUNDTYPE*       boundtypes,
   int                   mem,
   int*                  nvars,
   int*                  nafterdualvars
   );

/* returns the number of bound changes at the node
 * stored at ID id */
extern
int SCIPreoptnodeGetNConss(
   SCIP_REOPT*           reopt,
   int                   id
   );

/* returns the number of bound changes based on primal information including bound
 * changes directly after the first bound change based on dual information at the node
 * stored at ID id */
extern
int SCIPreoptnodeGetNVars(
   SCIP_REOPT*           reopt,
   int                   id
   );

/* reset the stored information abound bound changes based on dual information */
extern
void SCIPreoptResetDualcons(
   SCIP_REOPT*           reopt,
   SCIP_NODE*            node,
   BMS_BLKMEM*           blkmem
   );

/* split the root node and move all children to one of the two resulting nodes */
extern
SCIP_RETCODE SCIPreoptSplitRoot(
   SCIP_REOPT*           reopt,
   SCIP_SET*             set,
   BMS_BLKMEM*           blkmem
   );

extern
void SCIPreoptCreateSplitCons(
   SCIP_REOPT*           reopt,
   int                   id,
   LOGICORDATA*          consdata
   );

/* returns if a node need to be split because some bound changes
 * were based on dual information */
extern
SCIP_Bool SCIPreoptSplitNode(
   SCIP_REOPT*           reopt,
   SCIP_NODE*            node
   );

/* calculates a local similarity of a given node and returns if the subproblem
 * should be solved from scratch */
extern
SCIP_RETCODE SCIPreoptCheckLocalRestart(
   SCIP*                 scip,
   SCIP_REOPT*           reopt,
   SCIP_NODE*            node,
   SCIP_Bool*            localrestart
   );

/* add the node @param node to the reopttree */
extern
SCIP_RETCODE SCIPreoptAddNode(
   SCIP*                 scip,
   SCIP_REOPT*           reopt,
   SCIP_NODE*            node,
   SCIP_REOPTTYPE        reopttype,
   SCIP_Bool             saveafterduals,
   BMS_BLKMEM*           blkmem
   );

/* returns the number of restarts */
extern
int SCIPreoptGetNRestarts(
   SCIP_REOPT*           reopt
   );

#ifdef __cplusplus
}
#endif

#endif
