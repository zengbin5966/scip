/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                           */
/*                  This file is part of the program and library             */
/*         SCIP --- Solving Constraint Integer Programs                      */
/*                                                                           */
/*    Copyright (C) 2002-2019 Konrad-Zuse-Zentrum                            */
/*                            fuer Informationstechnik Berlin                */
/*                                                                           */
/*  SCIP is distributed under the terms of the ZIB Academic License.         */
/*                                                                           */
/*  You should have received a copy of the ZIB Academic License              */
/*  along with SCIP; see the file COPYING. If not visit scip.zib.de.         */
/*                                                                           */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/**@file   dptermsinterns.h
 * @brief  Dynamic programming internals for Steiner tree (sub-) problems with small number of terminals
 * @author Daniel Rehfeldt
 *
 * Internal methods and data structures for DP.
 *
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/


#ifndef APPLICATIONS_STP_SRC_DPTERMSINTERNS_H_
#define APPLICATIONS_STP_SRC_DPTERMSINTERNS_H_

#include "scip/scip.h"
#include "scip/rbtree.h"
#include "graph.h"
#include "stpvector.h"
#include "stpbitset.h"
#include "stpprioqueue.h"

/** dynamic programming search tree */
typedef struct dynamic_programming_search_tree DPSTREE;



/*
 * Data structures
 */


/** trace for reconstructing a sub-solution */
typedef struct solution_trace
{
   int                   prevs[2];           /**< marker to get ancestor solutions (0,1,2 ancestors possible) */
   SCIP_Real             cost;               /**< solution cost */
   int                   root;               /**< solution root */
} SOLTRACE;


/** sub-solution with extension */
typedef struct dynamic_programming_subsolution
{
   SCIP_RBTREE_HOOKS;                        /**< for red-black tree */
   STP_Bitset            bitkey;             /**< key marking the terminals in sub-solution */
   STP_Vectype(SOLTRACE) extensions;         /**< extensions of solution */
} DPSUBSOL;



/** saves some data updated in every iteration */
typedef struct dynamic_programming_iterator
{
   DPSUBSOL*             dpsubsol;
   STP_Vectype(int)      stack;              /**< general purpose stack */
   STP_Vectype(SOLTRACE) sol_traces;         /**< traces of current sub-solution */
   STP_Bitset            sol_bitset;         /**< marks terminals of sub-solution */
   SCIP_Real*            nodes_dist;         /**< weight of sub-ST rooted at node */
   SCIP_Real*            nodes_ub;           /**< upper bounds for rule-out */
   int*                  nodes_pred1;        /**< predecessor NOTE: with shift! */
   int*                  nodes_pred2;        /**< predecessor */
   SCIP_Bool*            nodes_isValidRoot;  /**< is node a valid root? */
   int                   nnodes;             /**< number of nodes */
   int                   sol_nterms;         /**< popcount */
} DPITER;



/** compressed graph with less information */
typedef struct dynamic_programming_graph
{
   int*                  terminals;          /**< array of terminals; in {0,1,...,nnodes - 1} */
   int*                  nodes_termId;       /**< per node: terminal (0,1,..), or -1 if non-terminal */
   int                   nnodes;             /**< number of nodes */
   int                   nedges;             /**< number of edges */
   int                   nterms;             /**< number of terminals */
} DPGRAPH;


/** additional data */
typedef struct dynamic_programming_misc
{
   STP_Bitset            allTrueBits;        /**< helper; of size nnodes */
   STP_Vectype(int)      bits_count;
   STP_Vectype(int)      bits;
   STP_Vectype(int)      offsets;
   STP_Vectype(int)      data;
   int                   min_prev[2];
   SCIP_Real             min;
   int                   min_x;
   int                   total_size;
} DPMISC;


/** solver */
typedef struct dynamic_programming_solver
{
   int*                  soledges;           /**< solution; NON-OWNED! */
   DPGRAPH*              dpgraph;            /**< graph */
   DPSUBSOL*             soltree_root;       /**< root of solution tree */
   DPSTREE*              dpstree;            /**< tree for finding solution combinations */
   DPMISC*               dpmisc;             /**< this and that */
   STP_PQ*               solpqueue;          /**< sub-solutions */
} DPSOLVER;


/*
 * Macro hacks
 */


/* NOTE: needed to find element in a red-black tree */
#define SUBSOL_LT(key,subsol)  stpbitset_GT(key, subsol->bitkey)
#define SUBSOL_GT(key,subsol)  stpbitset_LT(key, subsol->bitkey)

static inline
SCIP_DEF_RBTREE_FIND(findSubsol, STP_Bitset, DPSUBSOL, SUBSOL_LT, SUBSOL_GT) /*lint !e123*/



/*
 * Inline methods
 */


/** initializes */
static inline
SCIP_RETCODE dpterms_dpsubsolInit(
   SCIP*                 scip,               /**< SCIP data structure */
   DPSUBSOL**            subsol              /**< solution */
)
{
   DPSUBSOL* sub;
   SCIP_CALL( SCIPallocBlockMemory(scip, subsol) );
   sub = *subsol;
   sub->bitkey = NULL;
   sub->extensions = NULL;

   return SCIP_OKAY;
}


/** frees */
static inline
void dpterms_dpsubsolFree(
   SCIP*                 scip,               /**< SCIP data structure */
   DPSUBSOL**            subsol              /**< solution */
)
{
   DPSUBSOL* sub = *subsol;

   if( sub->bitkey )
      stpbitset_free(scip, &(sub->bitkey));

   if( sub->extensions )
   {
      StpVecFree(scip, sub->extensions);
   }

   SCIPfreeBlockMemory(scip, subsol);
}



/*
 *
 */


/* dpterms_util.c
 */
extern SCIP_RETCODE     dpterms_streeInit(SCIP*, int, int, DPSTREE**);
extern void             dpterms_streeFree(SCIP*, DPSTREE**);
extern SCIP_RETCODE     dpterms_streeInsert(SCIP*, STP_Bitset, STP_Bitset, int64_t, DPSTREE*);
extern STP_Vectype(int) dpterms_streeCollectIntersects(SCIP*, STP_Bitset, STP_Bitset, DPSTREE*);


/* dpterms_core.c
 */
extern SCIP_RETCODE     dpterms_coreSolve(SCIP*, GRAPH*, DPSOLVER*);



#endif /* APPLICATIONS_STP_SRC_DPTERMSINTERNS_H_ */
