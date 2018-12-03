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
/*  along with SCIP; see the file COPYING. If not visit scip.zib.de.         */
/*                                                                           */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/**@file   benderscut_opt.c
 * @brief  Generates a standard Benders' decomposition optimality cut
 * @author Stephen J. Maher
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#include "nlpi/exprinterpret.h"
#include "nlpi/pub_expr.h"
#include "scip/benderscut_opt.h"
#include "scip/cons_linear.h"
#include "scip/pub_benderscut.h"
#include "scip/pub_benders.h"
#include "scip/pub_lp.h"
#include "scip/pub_nlp.h"
#include "scip/pub_message.h"
#include "scip/pub_misc.h"
#include "scip/pub_misc_linear.h"
#include "scip/pub_var.h"
#include "scip/scip_benders.h"
#include "scip/scip_cons.h"
#include "scip/scip_cut.h"
#include "scip/scip_general.h"
#include "scip/scip_lp.h"
#include "scip/scip_mem.h"
#include "scip/scip_message.h"
#include "scip/scip_nlp.h"
#include "scip/scip_numerics.h"
#include "scip/scip_param.h"
#include "scip/scip_prob.h"
#include "scip/scip_probing.h"
#include "scip/scip_var.h"
#include <string.h>

#define BENDERSCUT_NAME             "optimality"
#define BENDERSCUT_DESC             "Standard Benders' decomposition optimality cut"
#define BENDERSCUT_PRIORITY      5000
#define BENDERSCUT_LPCUT            TRUE

#define SCIP_DEFAULT_ADDCUTS             FALSE  /** Should cuts be generated, instead of constraints */

/*
 * Data structures
 */

/** Benders' decomposition cuts data */
struct SCIP_BenderscutData
{
   SCIP_Bool             addcuts;            /**< should cuts be generated instead of constraints */
};


/*
 * Local methods
 */

/** in the case of numerical troubles, the LP is resolved with solution polishing activated */
static
SCIP_RETCODE polishSolution(
   SCIP*                 subproblem,         /**< the SCIP data structure */
   SCIP_Bool*            success             /**< TRUE is the resolving of the LP was successful */
   )
{
   int oldpolishing;
   SCIP_Bool lperror;
   SCIP_Bool cutoff;

   assert(subproblem != NULL);
   assert(SCIPinProbing(subproblem));

   (*success) = FALSE;

   /* setting the solution polishing parameter */
   SCIP_CALL( SCIPgetIntParam(subproblem, "lp/solutionpolishing", &oldpolishing) );
   SCIP_CALL( SCIPsetIntParam(subproblem, "lp/solutionpolishing", 2) );

   /* resolving the probing LP */
   SCIP_CALL( SCIPsolveProbingLP(subproblem, -1, &lperror, &cutoff) );

   if( SCIPgetLPSolstat(subproblem) == SCIP_LPSOLSTAT_OPTIMAL )
      (*success) = TRUE;

   /* resetting the solution polishing parameter */
   SCIP_CALL( SCIPsetIntParam(subproblem, "lp/solutionpolishing", oldpolishing) );

   return SCIP_OKAY;
}

/** adds a variable and value to the constraint/row arrays */
static
SCIP_RETCODE addVariableToArray(
   SCIP*                 masterprob,         /**< the SCIP instance of the master problem */
   SCIP_VAR**            vars,               /**< the variables in the generated cut with non-zero coefficient */
   SCIP_Real*            vals,               /**< the coefficients of the variables in the generated cut */
   SCIP_VAR*             addvar,             /**< the variable that will be added to the array */
   SCIP_Real             addval,             /**< the value that will be added to the array */
   int*                  nvars,              /**< the number of variables in the variable array */
   int*                  varssize            /**< the length of the variable size */
   )
{
   assert(masterprob != NULL);
   assert(vars != NULL);
   assert(vals != NULL);
   assert(addvar != NULL);
   assert(nvars != NULL);
   assert(varssize != NULL);

   if( *nvars >= *varssize )
   {
      *varssize = SCIPcalcMemGrowSize(masterprob, *varssize + 1);
      SCIP_CALL( SCIPreallocBufferArray(masterprob, &vars, *varssize) );
      SCIP_CALL( SCIPreallocBufferArray(masterprob, &vals, *varssize) );
   }
   assert(*nvars < *varssize);

   vars[*nvars] = addvar;
   vals[*nvars] = addval;
   (*nvars)++;

   return SCIP_OKAY;
}

/** computes a standard Benders' optimality cut from the dual solutions of the LP */
static
SCIP_RETCODE computeStandardOptimalityCut(
   SCIP*                 masterprob,         /**< the SCIP instance of the master problem */
   SCIP*                 subproblem,         /**< the SCIP instance of the subproblem */
   SCIP_BENDERS*         benders,            /**< the benders' decomposition structure */
   SCIP_VAR**            vars,               /**< the variables in the generated cut with non-zero coefficient */
   SCIP_Real*            vals,               /**< the coefficients of the variables in the generated cut */
   SCIP_Real*            lhs,                /**< the left hand side of the cut */
   SCIP_Real*            rhs,                /**< the right hand side of the cut */
   int*                  nvars,              /**< the number of variables in the cut */
   int*                  varssize,           /**< the number of variables in the array */
   SCIP_Real*            checkobj,           /**< stores the objective function computed from the dual solution */
   SCIP_Bool*            success             /**< was the cut generation successful? */
   )
{
   SCIP_VAR** subvars;
   SCIP_VAR** fixedvars;
   int nsubvars;
   int nfixedvars;
   SCIP_Real dualsol;
   SCIP_Real addval;
   int nrows;
   int i;

   (*checkobj) = 0;

   assert(masterprob != NULL);
   assert(subproblem != NULL);
   assert(benders != NULL);
   assert(vars != NULL);
   assert(vals != NULL);

   (*success) = FALSE;

   /* looping over all LP rows and setting the coefficients of the cut */
   nrows = SCIPgetNLPRows(subproblem);
   for( i = 0; i < nrows; i++ )
   {
      SCIP_ROW* lprow;
      addval = 0;

      lprow = SCIPgetLPRows(subproblem)[i];
      assert(lprow != NULL);

      dualsol = SCIProwGetDualsol(lprow);
      assert( !SCIPisInfinity(subproblem, dualsol) && !SCIPisInfinity(subproblem, -dualsol) );

      if( SCIPisZero(subproblem, dualsol) )
         continue;

      if( dualsol > 0.0 )
         addval = dualsol*SCIProwGetLhs(lprow);
      else
         addval = dualsol*SCIProwGetRhs(lprow);

      (*lhs) += addval;

      /* if the bound becomes infinite, then the cut generation terminates. */
      if( SCIPisInfinity(masterprob, (*lhs)) || SCIPisInfinity(masterprob, -(*lhs))
         || SCIPisInfinity(masterprob, addval) || SCIPisInfinity(masterprob, -addval))
      {
         (*success) = FALSE;
         SCIPdebugMsg(masterprob, "Infinite bound when generating optimality cut. lhs = %g addval = %g.\n", (*lhs), addval);
         return SCIP_OKAY;
      }
   }

   nsubvars = SCIPgetNVars(subproblem);
   subvars = SCIPgetVars(subproblem);
   nfixedvars = SCIPgetNFixedVars(subproblem);
   fixedvars = SCIPgetFixedVars(subproblem);

   /* looping over all variables to update the coefficients in the computed cut. */
   for( i = 0; i < nsubvars + nfixedvars; i++ )
   {
      SCIP_VAR* var;
      SCIP_VAR* mastervar;
      SCIP_Real redcost;

      if( i < nsubvars )
         var = subvars[i];
      else
         var = fixedvars[i - nsubvars];

      /* retrieving the master problem variable for the given subproblem variable. */
      SCIP_CALL( SCIPgetBendersMasterVar(masterprob, benders, var, &mastervar) );

      redcost = SCIPgetVarRedcost(subproblem, var);

      (*checkobj) += SCIPvarGetUnchangedObj(var)*SCIPvarGetSol(var, TRUE);

      /* checking whether the subproblem variable has a corresponding master variable. */
      if( mastervar != NULL )
      {
         SCIP_Real coef;

         coef = -1.0*(SCIPvarGetObj(var) + redcost);

         if( !SCIPisZero(masterprob, coef) )
         {
            /* adding the variable to the storage */
            SCIP_CALL( addVariableToArray(masterprob, vars, vals, mastervar, coef, nvars, varssize) );
         }
      }
      else
      {
         if( !SCIPisZero(subproblem, redcost) )
         {
            addval = 0;

            if( SCIPisPositive(subproblem, redcost) )
               addval = redcost*SCIPvarGetLbLocal(var);
            else if( SCIPisNegative(subproblem, redcost) )
               addval = redcost*SCIPvarGetUbLocal(var);

            (*lhs) += addval;

            /* if the bound becomes infinite, then the cut generation terminates. */
            if( SCIPisInfinity(masterprob, (*lhs)) || SCIPisInfinity(masterprob, -(*lhs))
               || SCIPisInfinity(masterprob, addval) || SCIPisInfinity(masterprob, -addval))
            {
               (*success) = FALSE;
               SCIPdebugMsg(masterprob, "Infinite bound when generating optimality cut.\n");
               return SCIP_OKAY;
            }
         }
      }
   }

   assert(SCIPisInfinity(masterprob, (*rhs)));
   /* the rhs should be infinite. If it changes, then there is an error */
   if( !SCIPisInfinity(masterprob, (*rhs)) )
   {
      (*success) = FALSE;
      SCIPdebugMsg(masterprob, "RHS is not infinite. rhs = %g.\n", (*rhs));
      return SCIP_OKAY;
   }

   (*success) = TRUE;

   return SCIP_OKAY;
}

/** computes a standard Benders' optimality cut from the dual solutions of the NLP */
static
SCIP_RETCODE computeStandardOptimalityCutNL(
   SCIP*                 masterprob,         /**< the SCIP instance of the master problem */
   SCIP*                 subproblem,         /**< the SCIP instance of the subproblem */
   SCIP_BENDERS*         benders,            /**< the benders' decomposition structure */
   SCIP_VAR**            vars,               /**< the variables in the generated cut with non-zero coefficient */
   SCIP_Real*            vals,               /**< the coefficients of the variables in the generated cut */
   SCIP_Real*            lhs,                /**< the left hand side of the cut */
   SCIP_Real*            rhs,                /**< the right hand side of the cut */
   int*                  nvars,              /**< the number of variables in the cut */
   int*                  varssize,           /**< the number of variables in the array */
   SCIP_Real*            checkobj,           /**< stores the objective function computed from the dual solution */
   SCIP_Bool*            success             /**< was the cut generation successful? */
   )
{
   SCIP_EXPRINT* exprinterpreter;
   SCIP_VAR** subvars;
   SCIP_VAR** fixedvars;
   int nsubvars;
   int nfixedvars;
   SCIP_Real dirderiv;
   SCIP_Real dualsol;
   int nrows;
   int i;

   (*checkobj) = 0;

   assert(masterprob != NULL);
   assert(subproblem != NULL);
   assert(benders != NULL);
   assert(SCIPisNLPConstructed(subproblem));
   assert(SCIPgetNLPSolstat(subproblem) <= SCIP_NLPSOLSTAT_LOCOPT);
   assert(SCIPhasNLPSolution(subproblem));

   (*success) = FALSE;

   nsubvars = SCIPgetNNLPVars(subproblem);
   subvars = SCIPgetNLPVars(subproblem);
   nfixedvars = SCIPgetNFixedVars(subproblem);
   fixedvars = SCIPgetFixedVars(subproblem);

   /* our optimality cut implementation assumes that SCIP did not modify the objective function and sense,
    * that is, that the objective function value of the NLP corresponds to the value of the auxiliary variable
    * if that wouldn't be the case, then the scaling and offset may have to be considered when adding the
    * auxiliary variable to the cut (cons/row)?
    */
   assert(SCIPgetTransObjoffset(subproblem) == 0.0);
   assert(SCIPgetTransObjscale(subproblem) == 1.0);
   assert(SCIPgetObjsense(subproblem) == SCIP_OBJSENSE_MINIMIZE);

   (*lhs) = SCIPgetNLPObjval(subproblem);
   assert(!SCIPisInfinity(subproblem, REALABS(*lhs)));

   (*rhs) = SCIPinfinity(masterprob);

   dirderiv = 0.0;

   SCIP_CALL( SCIPexprintCreate(SCIPblkmem(subproblem), &exprinterpreter) );

   /* looping over all NLP rows and setting the corresponding coefficients of the cut */
   nrows = SCIPgetNNLPNlRows(subproblem);
   for( i = 0; i < nrows; i++ )
   {
      SCIP_NLROW* nlrow;

      nlrow = SCIPgetNLPNlRows(subproblem)[i];
      assert(nlrow != NULL);

      dualsol = SCIPnlrowGetDualsol(nlrow);
      assert( !SCIPisInfinity(subproblem, dualsol) && !SCIPisInfinity(subproblem, -dualsol) );

      if( SCIPisZero(subproblem, dualsol) )
         continue;

      SCIP_CALL( SCIPaddNlRowGradientBenderscutOpt(masterprob, subproblem, benders, nlrow, exprinterpreter,
            -dualsol, &dirderiv, vars, vals, nvars, varssize) );
   }

   SCIP_CALL( SCIPexprintFree(&exprinterpreter) );

   /* looping over all variable bounds and updating the corresponding coefficients of the cut; compute checkobj */
   for( i = 0; i < nsubvars; i++ )
   {
      SCIP_VAR* var;
      SCIP_VAR* mastervar;
      SCIP_Real coef;

      var = subvars[i];

      (*checkobj) += SCIPvarGetUnchangedObj(var) * SCIPvarGetNLPSol(var);

      /* retrieving the master problem variable for the given subproblem variable. */
      SCIP_CALL( SCIPgetBendersMasterVar(masterprob, benders, var, &mastervar) );

      dualsol = SCIPgetNLPVarsUbDualsol(subproblem)[i] - SCIPgetNLPVarsLbDualsol(subproblem)[i];

      /* checking whether the subproblem variable has a corresponding master variable. */
      if( mastervar == NULL || dualsol == 0.0 )
         continue;

      coef = -dualsol;

      /* adding the variable to the storage */
      SCIP_CALL( addVariableToArray(masterprob, vars, vals, mastervar, coef, nvars, varssize) );

      dirderiv += coef * SCIPvarGetNLPSol(var);
   }

   for( i = 0; i < nfixedvars; i++ )
      *checkobj += SCIPvarGetUnchangedObj(fixedvars[i]) * SCIPvarGetNLPSol(fixedvars[i]);

   *lhs += dirderiv;

   /* if the side became infinite or dirderiv was infinite, then the cut generation terminates. */
   if( SCIPisInfinity(masterprob, *lhs) || SCIPisInfinity(masterprob, -*lhs)
      || SCIPisInfinity(masterprob, dirderiv) || SCIPisInfinity(masterprob, -dirderiv))
   {
      (*success) = FALSE;
      SCIPdebugMsg(masterprob, "Infinite bound when generating optimality cut. lhs = %g dirderiv = %g.\n", lhs, dirderiv);
      return SCIP_OKAY;
   }

   (*success) = TRUE;

   return SCIP_OKAY;
}


/** Adds the auxiliary variable to the generated cut. If this is the first optimality cut for the subproblem, then the
 *  auxiliary variable is first created and added to the master problem.
 */
static
SCIP_RETCODE addAuxiliaryVariableToCut(
   SCIP*                 masterprob,         /**< the SCIP instance of the master problem */
   SCIP_BENDERS*         benders,            /**< the benders' decomposition structure */
   SCIP_VAR**            vars,               /**< the variables in the generated cut with non-zero coefficient */
   SCIP_Real*            vals,               /**< the coefficients of the variables in the generated cut */
   int*                  nvars,              /**< the number of variables in the cut */
   int                   probnumber          /**< the number of the pricing problem */
   )
{
   SCIP_VAR* auxiliaryvar;

   assert(masterprob != NULL);
   assert(benders != NULL);
   assert(vars != NULL);
   assert(vals != NULL);

   auxiliaryvar = SCIPbendersGetAuxiliaryVar(benders, probnumber);

   vars[(*nvars)] = auxiliaryvar;
   vals[(*nvars)] = 1.0;
   (*nvars)++;

   return SCIP_OKAY;
}


/** generates and applies Benders' cuts */
static
SCIP_RETCODE generateAndApplyBendersCuts(
   SCIP*                 masterprob,         /**< the SCIP instance of the master problem */
   SCIP*                 subproblem,         /**< the SCIP instance of the pricing problem */
   SCIP_BENDERS*         benders,            /**< the benders' decomposition */
   SCIP_BENDERSCUT*      benderscut,         /**< the benders' decomposition cut method */
   SCIP_SOL*             sol,                /**< primal CIP solution */
   int                   probnumber,         /**< the number of the pricing problem */
   SCIP_BENDERSENFOTYPE  type,               /**< the enforcement type calling this function */
   SCIP_RESULT*          result              /**< the result from solving the subproblems */
   )
{
   SCIP_BENDERSCUTDATA* benderscutdata;
   SCIP_CONSHDLR* consbenders;
   SCIP_CONS* cons;
   SCIP_ROW* row;
   SCIP_VAR** vars;
   SCIP_Real* vals;
   SCIP_Real lhs;
   SCIP_Real rhs;
   int nvars;
   int varssize;
   int nmastervars;
   char cutname[SCIP_MAXSTRLEN];
   SCIP_Bool optimal;
   SCIP_Bool addcut;
   SCIP_Bool success;

   SCIP_Real checkobj;
   SCIP_Real verifyobj;

   assert(masterprob != NULL);
   assert(subproblem != NULL);
   assert(benders != NULL);
   assert(benderscut != NULL);
   assert(result != NULL);

   row = NULL;
   cons = NULL;

   /* retrieving the Benders' cut data */
   benderscutdata = SCIPbenderscutGetData(benderscut);

   /* if the cuts are generated prior to the solving stage, then rows can not be generated. So constraints must be
    * added to the master problem.
    */
   if( SCIPgetStage(masterprob) < SCIP_STAGE_INITSOLVE )
      addcut = FALSE;
   else
      addcut = benderscutdata->addcuts;

   /* retrieving the Benders' decomposition constraint handler */
   consbenders = SCIPfindConshdlr(masterprob, "benders");

   /* checking the optimality of the original problem with a comparison between the auxiliary variable and the
    * objective value of the subproblem */
   SCIP_CALL( SCIPcheckBendersSubproblemOptimality(masterprob, benders, sol, probnumber, &optimal) );

   if( optimal )
   {
      (*result) = SCIP_FEASIBLE;
      SCIPdebugMsg(masterprob, "No cut added for subproblem %d\n", probnumber);
      return SCIP_OKAY;
   }

   /* allocating memory for the variable and values arrays */
   nmastervars = SCIPgetNVars(masterprob) + SCIPgetNFixedVars(masterprob);
   SCIP_CALL( SCIPallocClearBufferArray(masterprob, &vars, nmastervars) );
   SCIP_CALL( SCIPallocClearBufferArray(masterprob, &vals, nmastervars) );
   lhs = 0.0;
   rhs = SCIPinfinity(masterprob);
   nvars = 0;
   varssize = nmastervars;

   /* setting the name of the generated cut */
   (void) SCIPsnprintf(cutname, SCIP_MAXSTRLEN, "optimalitycut_%d_%d", probnumber,
      SCIPbenderscutGetNFound(benderscut) );

   if( SCIPisNLPConstructed(subproblem) )
   {
      /* computing the coefficients of the optimality cut */
      SCIP_CALL( computeStandardOptimalityCutNL(masterprob, subproblem, benders, vars, vals, &lhs, &rhs, &nvars,
            &varssize, &checkobj, &success) );
   }
   else
   {
      /* computing the coefficients of the optimality cut */
      SCIP_CALL( computeStandardOptimalityCut(masterprob, subproblem, benders, vars, vals, &lhs, &rhs, &nvars,
            &varssize, &checkobj, &success) );
   }

   /* if success is FALSE, then there was an error in generating the optimality cut. No cut will be added to the master
    * problem. Otherwise, the constraint is added to the master problem.
    */
   if( !success )
   {
      (*result) = SCIP_DIDNOTFIND;
      SCIPdebugMsg(masterprob, "Error in generating Benders' optimality cut for problem %d.\n", probnumber);
   }
   else
   {
      /* creating an empty row or constraint for the Benders' cut */
      if( addcut )
      {
         SCIP_CALL( SCIPcreateEmptyRowCons(masterprob, &row, consbenders, cutname, lhs, rhs, FALSE, FALSE, TRUE) );
         SCIP_CALL( SCIPaddVarsToRow(masterprob, row, nvars, vars, vals) );
      }
      else
      {
         SCIP_CALL( SCIPcreateConsBasicLinear(masterprob, &cons, cutname, nvars, vars, vals, lhs, rhs) );
         SCIP_CALL( SCIPsetConsDynamic(masterprob, cons, TRUE) );
         SCIP_CALL( SCIPsetConsRemovable(masterprob, cons, TRUE) );
      }

      /* computing the objective function from the cut activity to verify the accuracy of the constraint */
      verifyobj = 0.0;
      if( addcut )
      {
         verifyobj += SCIProwGetLhs(row) - SCIPgetRowSolActivity(masterprob, row, sol);
      }
      else
      {
         verifyobj += SCIPgetLhsLinear(masterprob, cons) - SCIPgetActivityLinear(masterprob, cons, sol);
      }

      /* it is possible that numerics will cause the generated cut to be invalid. This cut should not be added to the
       * master problem, since its addition could cut off feasible solutions. The success flag is set of false, indicating
       * that the Benders' cut could not find a valid cut.
       */
      if( !SCIPisFeasEQ(masterprob, checkobj, verifyobj) )
      {
         success = FALSE;
         SCIPdebugMsg(masterprob, "The objective function and cut activity are not equal (%g != %g).\n", checkobj,
            verifyobj);
#ifdef SCIP_DEBUG
         SCIPABORT();
#endif
      }

      if( success )
      {
         /* adding the auxiliary variable to the optimality cut */
         SCIP_CALL( addAuxiliaryVariableToCut(masterprob, benders, vars, vals, &nvars, probnumber) );

         /* adding the constraint to the master problem */
         if( addcut )
         {
            SCIP_Bool infeasible;

            /* adding the auxiliary variable coefficient to the row */
            SCIP_CALL( SCIPaddVarToRow(masterprob, row, vars[nvars - 1], vals[nvars - 1]) );

            if( type == SCIP_BENDERSENFOTYPE_LP || type == SCIP_BENDERSENFOTYPE_RELAX )
            {
               SCIP_CALL( SCIPaddRow(masterprob, row, FALSE, &infeasible) );
               assert(!infeasible);
            }
            else
            {
               assert(type == SCIP_BENDERSENFOTYPE_CHECK || type == SCIP_BENDERSENFOTYPE_PSEUDO);
               SCIP_CALL( SCIPaddPoolCut(masterprob, row) );
            }

            (*result) = SCIP_SEPARATED;
         }
         else
         {
            /* adding the auxiliary variable coefficient to the constraint */
            SCIP_CALL( SCIPaddCoefLinear(masterprob, cons, vars[nvars - 1], vals[nvars - 1]) );

            SCIP_CALL( SCIPaddCons(masterprob, cons) );

            SCIPdebugPrintCons(masterprob, cons, NULL);

            (*result) = SCIP_CONSADDED;
         }

         /* storing the data that is used to create the cut */
         SCIP_CALL( SCIPstoreBenderscutCut(masterprob, benderscut, vars, vals, lhs, rhs, nvars) );
      }

      /* releasing the row or constraint */
      if( addcut )
      {
         /* release the row */
         SCIP_CALL( SCIPreleaseRow(masterprob, &row) );
      }
      else
      {
         /* release the constraint */
         SCIP_CALL( SCIPreleaseCons(masterprob, &cons) );
      }
   }

   SCIPfreeBufferArray(masterprob, &vals);
   SCIPfreeBufferArray(masterprob, &vars);

   return SCIP_OKAY;
}

/*
 * Callback methods of Benders' decomposition cuts
 */

/** destructor of Benders' decomposition cuts to free user data (called when SCIP is exiting) */
static
SCIP_DECL_BENDERSCUTFREE(benderscutFreeOpt)
{  /*lint --e{715}*/
   SCIP_BENDERSCUTDATA* benderscutdata;

   assert( benderscut != NULL );
   assert( strcmp(SCIPbenderscutGetName(benderscut), BENDERSCUT_NAME) == 0 );

   /* free Benders' cut data */
   benderscutdata = SCIPbenderscutGetData(benderscut);
   assert( benderscutdata != NULL );

   SCIPfreeBlockMemory(scip, &benderscutdata);

   SCIPbenderscutSetData(benderscut, NULL);

   return SCIP_OKAY;
}


/** execution method of Benders' decomposition cuts */
static
SCIP_DECL_BENDERSCUTEXEC(benderscutExecOpt)
{  /*lint --e{715}*/
   SCIP* subproblem;

   assert(scip != NULL);
   assert(benders != NULL);
   assert(benderscut != NULL);
   assert(result != NULL);
   assert(probnumber >= 0 && probnumber < SCIPbendersGetNSubproblems(benders));

   subproblem = SCIPbendersSubproblem(benders, probnumber);

   /* only generate optimality cuts if the subproblem is optimal */
   if( SCIPgetStatus(subproblem) == SCIP_STATUS_OPTIMAL ||
    (SCIPgetStage(subproblem) == SCIP_STAGE_SOLVING && !SCIPisNLPConstructed(subproblem) && SCIPgetLPSolstat(subproblem) == SCIP_LPSOLSTAT_OPTIMAL) ||
    (SCIPgetStage(subproblem) == SCIP_STAGE_SOLVING && SCIPisNLPConstructed(subproblem) && SCIPgetNLPSolstat(subproblem) <= SCIP_NLPSOLSTAT_LOCOPT) )
   {
      /* generating a cut for a given subproblem */
      SCIP_CALL( generateAndApplyBendersCuts(scip, subproblem, benders, benderscut,
            sol, probnumber, type, result) );

      /* if it was not possible to generate a cut, this could be due to numerical issues. So the solution to the LP is
       * resolved and the generation of the cut is reattempted. For NLPs, we do not have such a polishing yet.
       */
      if( (*result) == SCIP_DIDNOTFIND && !SCIPisNLPConstructed(subproblem) )
      {
         SCIP_Bool success;

         SCIPinfoMessage(scip, NULL, "Numerical trouble generating optimality cut for subproblem %d. Attempting to "
            "polish the LP solution to find an alternative dual extreme point.\n", probnumber);

         SCIP_CALL( polishSolution(subproblem, &success) );

         /* only attempt to generate a cut if the solution polishing was successful */
         if( success )
         {
            SCIP_CALL( generateAndApplyBendersCuts(scip, subproblem, benders, benderscut,
                  sol, probnumber, type, result) );
         }
      }
   }

   return SCIP_OKAY;
}


/*
 * Benders' decomposition cuts specific interface methods
 */

/** creates the opt Benders' decomposition cuts and includes it in SCIP */
SCIP_RETCODE SCIPincludeBenderscutOpt(
   SCIP*                 scip,               /**< SCIP data structure */
   SCIP_BENDERS*         benders             /**< Benders' decomposition */
   )
{
   SCIP_BENDERSCUTDATA* benderscutdata;
   SCIP_BENDERSCUT* benderscut;
   char paramname[SCIP_MAXSTRLEN];

   assert(benders != NULL);

   /* create opt Benders' decomposition cuts data */
   SCIP_CALL( SCIPallocBlockMemory(scip, &benderscutdata) );

   benderscut = NULL;

   /* include Benders' decomposition cuts */
   SCIP_CALL( SCIPincludeBenderscutBasic(scip, benders, &benderscut, BENDERSCUT_NAME, BENDERSCUT_DESC,
         BENDERSCUT_PRIORITY, BENDERSCUT_LPCUT, benderscutExecOpt, benderscutdata) );

   assert(benderscut != NULL);

   /* setting the non fundamental callbacks via setter functions */
   SCIP_CALL( SCIPsetBenderscutFree(scip, benderscut, benderscutFreeOpt) );

   /* add opt Benders' decomposition cuts parameters */
   (void) SCIPsnprintf(paramname, SCIP_MAXSTRLEN, "benders/%s/benderscut/%s/addcuts",
      SCIPbendersGetName(benders), BENDERSCUT_NAME);
   SCIP_CALL( SCIPaddBoolParam(scip, paramname,
         "should cuts be generated and added to the cutpool instead of global constraints directly added to the problem.",
         &benderscutdata->addcuts, FALSE, SCIP_DEFAULT_ADDCUTS, NULL, NULL) );

   return SCIP_OKAY;
}

/** adds the gradient of a nonlinear row in the current NLP solution of a subproblem to a linear row or constraint in the master problem
 *
 * Only computes gradient w.r.t. master problem variables.
 * Computes also the directional derivative, that is, mult times gradient times solution.
 */
SCIP_RETCODE SCIPaddNlRowGradientBenderscutOpt(
   SCIP*                 masterprob,         /**< the SCIP instance of the master problem */
   SCIP*                 subproblem,         /**< the SCIP instance of the subproblem */
   SCIP_BENDERS*         benders,            /**< the benders' decomposition structure */
   SCIP_NLROW*           nlrow,              /**< nonlinear row */
   SCIP_EXPRINT*         exprint,            /**< expressions interpreter */
   SCIP_Real             mult,               /**< multiplier */
   SCIP_Real*            dirderiv,           /**< storage to add directional derivative */
   SCIP_VAR**            vars,               /**< the variables in the generated cut with non-zero coefficient */
   SCIP_Real*            vals,               /**< the coefficients of the variables in the generated cut */
   int*                  nvars,              /**< the number of variables in the cut */
   int*                  varssize            /**< the number of variables in the array */
   )
{
   SCIP_EXPRTREE* tree;
   SCIP_VAR* var;
   SCIP_VAR* mastervar;
   SCIP_Real coef;
   int i;

   assert(masterprob != NULL);
   assert(subproblem != NULL);
   assert(benders != NULL);
   assert(nlrow != NULL);
   assert(exprint != NULL);
   assert(mult != 0.0);
   assert(dirderiv != NULL);

   /* linear part */
   for( i = 0; i < SCIPnlrowGetNLinearVars(nlrow); i++ )
   {
      var = SCIPnlrowGetLinearVars(nlrow)[i];
      assert(var != NULL);

      /* retrieving the master problem variable for the given subproblem variable. */
      SCIP_CALL( SCIPgetBendersMasterVar(masterprob, benders, var, &mastervar) );
      if( mastervar == NULL )
         continue;

      coef = mult * SCIPnlrowGetLinearCoefs(nlrow)[i];

      /* adding the variable to the storage */
      SCIP_CALL( addVariableToArray(masterprob, vars, vals, mastervar, coef, nvars, varssize) );

      *dirderiv += coef * SCIPvarGetNLPSol(var);
   }

   /* quadratic part */
   for( i = 0; i < SCIPnlrowGetNQuadElems(nlrow); i++ )
   {
      SCIP_VAR* var1;
      SCIP_VAR* var2;
      SCIP_VAR* mastervar1;
      SCIP_VAR* mastervar2;
      SCIP_Real coef1;
      SCIP_Real coef2;

      assert(SCIPnlrowGetQuadElems(nlrow)[i].idx1 < SCIPnlrowGetNQuadVars(nlrow));
      assert(SCIPnlrowGetQuadElems(nlrow)[i].idx2 < SCIPnlrowGetNQuadVars(nlrow));

      var1  = SCIPnlrowGetQuadVars(nlrow)[SCIPnlrowGetQuadElems(nlrow)[i].idx1];
      var2  = SCIPnlrowGetQuadVars(nlrow)[SCIPnlrowGetQuadElems(nlrow)[i].idx2];

      /* retrieving the master problem variables for the given subproblem variables. */
      SCIP_CALL( SCIPgetBendersMasterVar(masterprob, benders, var1, &mastervar1) );
      SCIP_CALL( SCIPgetBendersMasterVar(masterprob, benders, var2, &mastervar2) );

      coef1 = mult * SCIPnlrowGetQuadElems(nlrow)[i].coef * SCIPvarGetNLPSol(var2);
      coef2 = mult * SCIPnlrowGetQuadElems(nlrow)[i].coef * SCIPvarGetNLPSol(var1);

      /* adding the variable to the storage */
      if( mastervar1 != NULL )
      {
         SCIP_CALL( addVariableToArray(masterprob, vars, vals, mastervar1, coef1, nvars, varssize) );
      }
      if( mastervar2 != NULL )
      {
         SCIP_CALL( addVariableToArray(masterprob, vars, vals, mastervar2, coef2, nvars, varssize) );
      }

      if( mastervar1 != NULL )
         *dirderiv += coef1 * SCIPvarGetNLPSol(var1);

      if( mastervar2 != NULL )
         *dirderiv += coef2 * SCIPvarGetNLPSol(var2);
   }

   /* tree part */
   tree = SCIPnlrowGetExprtree(nlrow);
   if( tree != NULL )
   {
      SCIP_Real* treegrad;
      SCIP_Real* x;
      SCIP_Real val;

      SCIP_CALL( SCIPallocBufferArray(subproblem, &x, SCIPexprtreeGetNVars(tree)) );
      SCIP_CALL( SCIPallocBufferArray(subproblem, &treegrad, SCIPexprtreeGetNVars(tree)) );

      /* compile expression tree, if not done before */
      if( SCIPexprtreeGetInterpreterData(tree) == NULL )
      {
         SCIP_CALL( SCIPexprintCompile(exprint, tree) );
      }

      /* sets the solution value */
      for( i = 0; i < SCIPexprtreeGetNVars(tree); ++i )
         x[i] = SCIPvarGetNLPSol(SCIPexprtreeGetVars(tree)[i]);

      SCIP_CALL( SCIPexprintGrad(exprint, tree, x, TRUE, &val, treegrad) );

      /* update corresponding gradient entry */
      for( i = 0; i < SCIPexprtreeGetNVars(tree); ++i )
      {
         var = SCIPexprtreeGetVars(tree)[i];
         assert(var != NULL);

         /* retrieving the master problem variable for the given subproblem variable. */
         SCIP_CALL( SCIPgetBendersMasterVar(masterprob, benders, var, &mastervar) );
         if( mastervar == NULL )
            continue;

         coef = mult * treegrad[i];

         /* adding the variable to the storage */
         SCIP_CALL( addVariableToArray(masterprob, vars, vals, mastervar, coef, nvars, varssize) );

         *dirderiv += coef * SCIPvarGetNLPSol(var);
      }

      SCIPfreeBufferArray(subproblem, &treegrad);
      SCIPfreeBufferArray(subproblem, &x);
   }

   return SCIP_OKAY;
}
