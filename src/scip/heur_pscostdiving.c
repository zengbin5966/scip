/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                           */
/*                  This file is part of the program and library             */
/*         SCIP --- Solving Constraint Integer Programs                      */
/*                                                                           */
/*    Copyright (C) 2002-2014 Konrad-Zuse-Zentrum                            */
/*                            fuer Informationstechnik Berlin                */
/*                                                                           */
/*  SCIP is distributed under the terms of the ZIB Academic License.         */
/*                                                                           */
/*  You should have received a copy of the ZIB Academic License              */
/*  along with SCIP; see the file COPYING. If not email to scip@zib.de.      */
/*                                                                           */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/**@file   heur_pscostdiving.c
 * @brief  LP diving heuristic that chooses fixings w.r.t. the pseudo cost values
 * @author Tobias Achterberg
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#include <assert.h>
#include <string.h>

#include "scip/heur_pscostdiving.h"


#define HEUR_NAME             "pscostdiving"
#define HEUR_DESC             "LP diving heuristic that chooses fixings w.r.t. the pseudo cost values"
#define HEUR_DISPCHAR         'p'
#define HEUR_PRIORITY         -1002000
#define HEUR_FREQ             10
#define HEUR_FREQOFS          2
#define HEUR_MAXDEPTH         -1
#define HEUR_TIMING           SCIP_HEURTIMING_AFTERLPPLUNGE
#define HEUR_USESSUBSCIP      FALSE  /**< does the heuristic use a secondary SCIP instance? */


/*
 * Default parameter settings
 */

#define DEFAULT_MINRELDEPTH         0.0 /**< minimal relative depth to start diving */
#define DEFAULT_MAXRELDEPTH         1.0 /**< maximal relative depth to start diving */
#define DEFAULT_MAXLPITERQUOT      0.05 /**< maximal fraction of diving LP iterations compared to node LP iterations */
#define DEFAULT_MAXLPITEROFS       1000 /**< additional number of allowed LP iterations */
#define DEFAULT_MAXDIVEUBQUOT       0.8 /**< maximal quotient (curlowerbound - lowerbound)/(cutoffbound - lowerbound)
                                              *   where diving is performed (0.0: no limit) */
#define DEFAULT_MAXDIVEAVGQUOT      0.0 /**< maximal quotient (curlowerbound - lowerbound)/(avglowerbound - lowerbound)
                                              *   where diving is performed (0.0: no limit) */
#define DEFAULT_MAXDIVEUBQUOTNOSOL  0.1 /**< maximal UBQUOT when no solution was found yet (0.0: no limit) */
#define DEFAULT_MAXDIVEAVGQUOTNOSOL 0.0 /**< maximal AVGQUOT when no solution was found yet (0.0: no limit) */
#define DEFAULT_BACKTRACK          TRUE /**< use one level of backtracking if infeasibility is encountered? */

#define MINLPITER                 10000 /**< minimal number of LP iterations allowed in each LP solving call */


/* locally defined heuristic data */
struct SCIP_HeurData
{
   SCIP_SOL*             sol;                /**< working solution */
   SCIP_DIVESET*         diveset;            /**< diving settings */
};

/*
 * local methods
 */

/*
 * Callback methods
 */

/** copy method for primal heuristic plugins (called when SCIP copies plugins) */
static
SCIP_DECL_HEURCOPY(heurCopyPscostdiving)
{  /*lint --e{715}*/
   assert(scip != NULL);
   assert(heur != NULL);
   assert(strcmp(SCIPheurGetName(heur), HEUR_NAME) == 0);

   /* call inclusion method of primal heuristic */
   SCIP_CALL( SCIPincludeHeurPscostdiving(scip) );

   return SCIP_OKAY;
}

/** destructor of primal heuristic to free user data (called when SCIP is exiting) */
static
SCIP_DECL_HEURFREE(heurFreePscostdiving) /*lint --e{715}*/
{  /*lint --e{715}*/
   SCIP_HEURDATA* heurdata;

   assert(heur != NULL);
   assert(strcmp(SCIPheurGetName(heur), HEUR_NAME) == 0);
   assert(scip != NULL);

   /* free heuristic data */
   heurdata = SCIPheurGetData(heur);
   assert(heurdata != NULL);
   SCIP_CALL( SCIPdivesetFree(&heurdata->diveset) );
   SCIPfreeMemory(scip, &heurdata);
   SCIPheurSetData(heur, NULL);

   return SCIP_OKAY;
}


/** initialization method of primal heuristic (called after problem was transformed) */
static
SCIP_DECL_HEURINIT(heurInitPscostdiving) /*lint --e{715}*/
{  /*lint --e{715}*/
   SCIP_HEURDATA* heurdata;

   assert(heur != NULL);
   assert(strcmp(SCIPheurGetName(heur), HEUR_NAME) == 0);

   /* get heuristic data */
   heurdata = SCIPheurGetData(heur);
   assert(heurdata != NULL);

   /* create working solution */
   SCIP_CALL( SCIPcreateSol(scip, &heurdata->sol, heur) );

   /* initialize data */
   SCIPresetDiveset(scip, heurdata->diveset);

   return SCIP_OKAY;
}


/** deinitialization method of primal heuristic (called before transformed problem is freed) */
static
SCIP_DECL_HEUREXIT(heurExitPscostdiving) /*lint --e{715}*/
{  /*lint --e{715}*/
   SCIP_HEURDATA* heurdata;

   assert(heur != NULL);
   assert(strcmp(SCIPheurGetName(heur), HEUR_NAME) == 0);

   /* get heuristic data */
   heurdata = SCIPheurGetData(heur);
   assert(heurdata != NULL);

   /* free working solution */
   SCIP_CALL( SCIPfreeSol(scip, &heurdata->sol) );

   return SCIP_OKAY;
}


/** execution method of primal heuristic */
static
SCIP_DECL_HEUREXEC(heurExecPscostdiving) /*lint --e{715}*/
{  /*lint --e{715}*/
   SCIP_HEURDATA* heurdata;
   heurdata = SCIPheurGetData(heur);
   assert(heurdata != NULL);

   SCIP_CALL( SCIPperformGenericDivingAlgorithm(scip, heurdata->diveset, heurdata->sol, heur, result, nodeinfeasible) );

   return SCIP_OKAY;
}

/** determine the candidate direction. if the variable may be trivially rounded in one direction, take the other direction;
 *  otherwise, consider first the direction from the root solution, second the candidate fractionality, and
 *  last the direction of smaller pseudo costs
 */
static
SCIP_BRANCHDIR getCandidateDirection(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_VAR*             cand,               /**< candidate for diving */
   SCIP_Real             candsfrac,          /**< candidate fractionality */
   SCIP_Real             candsol             /**< candidate LP solution value */
   )
{
   SCIP_Bool mayrounddown;
   SCIP_Bool mayroundup;

   mayrounddown = SCIPvarMayRoundDown(cand);
   mayroundup = SCIPvarMayRoundUp(cand);


   /* bound fractions to not prefer variables that are nearly integral */
   candsfrac = MAX(candsfrac, 0.1);
   candsfrac = MIN(candsfrac, 0.9);


   if( mayrounddown != mayroundup )
      return mayrounddown ? SCIP_BRANCHDIR_UPWARDS : SCIP_BRANCHDIR_DOWNWARDS;
   else
   {
      /* choose rounding direction */
      if( candsol < SCIPvarGetRootSol(cand) - 0.4 )
         return SCIP_BRANCHDIR_DOWNWARDS;
      else if( candsol > SCIPvarGetRootSol(cand) + 0.4 )
         return SCIP_BRANCHDIR_UPWARDS;
      else if( candsfrac < 0.3 )
         return SCIP_BRANCHDIR_DOWNWARDS;
      else if( candsfrac > 0.7 )
         return SCIP_BRANCHDIR_UPWARDS;
      else
      {
         SCIP_Real pscostdown;
         SCIP_Real pscostup;

         /* get pseudo costs in both directions */
         pscostdown = SCIPgetVarPseudocostVal(scip, cand, 0.0 - candsfrac);
         pscostup = SCIPgetVarPseudocostVal(scip, cand, 1.0 - candsfrac);
         assert(pscostdown >= 0.0 && pscostup >= 0.0);
         if( pscostdown < pscostup )
            return SCIP_BRANCHDIR_DOWNWARDS;
         else
            return SCIP_BRANCHDIR_UPWARDS;
      }
   }
}

/** returns the preferred branching direction of candidate */
static
SCIP_DECL_DIVESETCANDBRANCHDIR(divesetCandbranchdirPscostdiving)
{
   return getCandidateDirection(scip, cand, candsfrac, candsol);
}

/** returns a score for the given candidate -- the best candidate minimizes the diving score */
static
SCIP_DECL_DIVESETGETSCORE(divesetGetScorePscostdiving)
{
   SCIP_BRANCHDIR dir;
   SCIP_Real pscostdown;
   SCIP_Real pscostup;
   SCIP_Real pscostquot;

   /* get candidate direction */
   dir = getCandidateDirection(scip, cand, candsfrac, candsol);

   /* bound fractions to not prefer variables that are nearly integral */
   candsfrac = MAX(candsfrac, 0.1);
   candsfrac = MIN(candsfrac, 0.9);

   /* get pseudo cost quotient */
   pscostdown = SCIPgetVarPseudocostVal(scip, cand, 0.0 - candsfrac);
   pscostup = SCIPgetVarPseudocostVal(scip, cand, 1.0 - candsfrac);
   assert(pscostdown >= 0.0 && pscostup >= 0.0);

   if( dir == SCIP_BRANCHDIR_UPWARDS )
      pscostquot = sqrt(candsfrac) * (1.0 + pscostdown) / (1.0 + pscostup);
   else
      pscostquot = sqrt(1.0 - candsfrac) * (1.0 + pscostup) / (1.0 + pscostdown);

   /* prefer decisions on binary variables */
   if( SCIPvarIsBinary(cand) && !(SCIPvarMayRoundDown(cand) || SCIPvarMayRoundUp(cand)))
      pscostquot *= 1000.0;

   assert(pscostquot >= 0);
   /* return negative cost quotient because diving algorithm minimizes the score */
   return -pscostquot;
}

/*
 * heuristic specific interface methods
 */

/** creates the pscostdiving heuristic and includes it in SCIP */
SCIP_RETCODE SCIPincludeHeurPscostdiving(
   SCIP*                 scip                /**< SCIP data structure */
   )
{
   SCIP_HEURDATA* heurdata;
   SCIP_HEUR* heur;

   /* create Pscostdiving primal heuristic data */
   SCIP_CALL( SCIPallocMemory(scip, &heurdata) );

   /* include primal heuristic */
   SCIP_CALL( SCIPincludeHeurBasic(scip, &heur,
         HEUR_NAME, HEUR_DESC, HEUR_DISPCHAR, HEUR_PRIORITY, HEUR_FREQ, HEUR_FREQOFS,
         HEUR_MAXDEPTH, HEUR_TIMING, HEUR_USESSUBSCIP, heurExecPscostdiving, heurdata) );

   assert(heur != NULL);

   /* set non-NULL pointers to callback methods */
   SCIP_CALL( SCIPsetHeurCopy(scip, heur, heurCopyPscostdiving) );
   SCIP_CALL( SCIPsetHeurFree(scip, heur, heurFreePscostdiving) );
   SCIP_CALL( SCIPsetHeurInit(scip, heur, heurInitPscostdiving) );
   SCIP_CALL( SCIPsetHeurExit(scip, heur, heurExitPscostdiving) );

   heurdata->diveset = NULL;
   /* create a diveset (this will automatically install some additional parameters for the heuristic)*/
   SCIP_CALL( SCIPcreateDiveset(scip, &heurdata->diveset, heur, DEFAULT_MINRELDEPTH, DEFAULT_MAXRELDEPTH, DEFAULT_MAXLPITERQUOT,
         DEFAULT_MAXDIVEUBQUOT, DEFAULT_MAXDIVEAVGQUOT, 1.0, 1.0, DEFAULT_MAXLPITEROFS,
         DEFAULT_BACKTRACK, divesetGetScorePscostdiving, divesetCandbranchdirPscostdiving, NULL, NULL) );

   return SCIP_OKAY;
   return SCIP_OKAY;
}

