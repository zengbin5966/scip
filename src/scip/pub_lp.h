/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                           */
/*                  This file is part of the program and library             */
/*         SCIP --- Solving Constraint Integer Programs                      */
/*                                                                           */
/*    Copyright (C) 2002-2004 Tobias Achterberg                              */
/*                                                                           */
/*                  2002-2004 Konrad-Zuse-Zentrum                            */
/*                            fuer Informationstechnik Berlin                */
/*                                                                           */
/*  SCIP is distributed under the terms of the SCIP Academic Licence.        */
/*                                                                           */
/*  You should have received a copy of the SCIP Academic License             */
/*  along with SCIP; see the file COPYING. If not email to scip@zib.de.      */
/*                                                                           */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#pragma ident "@(#) $Id: pub_lp.h,v 1.14 2004/10/12 14:06:07 bzfpfend Exp $"

/**@file   lp.h
 * @brief  public methods for LP management
 * @author Tobias Achterberg
 */

/*---+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+----9----+----0----+----1----+----2*/

#ifndef __PUB_LP_H__
#define __PUB_LP_H__


#include <stdio.h>

#include "def.h"
#include "memory.h"
#include "type_set.h"
#include "type_stat.h"
#include "type_lp.h"
#include "type_var.h"
#include "type_sol.h"

#ifdef NDEBUG
#include "struct_lp.h"
#endif



/*
 * Column methods
 */

/** output column to file stream */
extern
void SCIPcolPrint(
   COL*             col,                /**< LP column */
   FILE*            file                /**< output file (or NULL for standard output) */
   );

#ifndef NDEBUG

/* In debug mode, the following methods are implemented as function calls to ensure
 * type validity.
 */

/** gets objective value of column */
extern
Real SCIPcolGetObj(
   COL*             col                 /**< LP column */
   );

/** gets lower bound of column */
extern
Real SCIPcolGetLb(
   COL*             col                 /**< LP column */
   );

/** gets upper bound of column */
extern
Real SCIPcolGetUb(
   COL*             col                 /**< LP column */
   );

/** gets best bound of column with respect to the objective function */
extern
Real SCIPcolGetBestBound(
   COL*             col                 /**< LP column */
   );

/** gets the primal LP solution of a column */
extern
Real SCIPcolGetPrimsol(
   COL*             col                 /**< LP column */
   );

/** gets variable this column represents */
extern
VAR* SCIPcolGetVar(
   COL*             col                 /**< LP column */
   );

/** returns whether the associated variable is of integral type (binary, integer, implicit integer) */
extern
Bool SCIPcolIsIntegral(
   COL*             col                 /**< LP column */
   );

/** returns TRUE iff column is removeable from the LP (due to aging or cleanup) */
extern
Bool SCIPcolIsRemoveable(
   COL*             col                 /**< LP column */
   );

/** gets position of column in current LP, or -1 if it is not in LP */
extern
int SCIPcolGetLPPos(
   COL*             col                 /**< LP column */
   );

/** gets depth in the tree where the column entered the LP, or -1 if it is not in LP */
extern
int SCIPcolGetLPDepth(
   COL*             col                 /**< LP column */
   );

/** returns TRUE iff column is member of current LP */
extern
Bool SCIPcolIsInLP(
   COL*             col                 /**< LP column */
   );

/** get number of nonzero entries in column vector */
extern
int SCIPcolGetNNonz(
   COL*             col                 /**< LP column */
   );

/** get number of nonzero entries in column vector, that correspond to rows currently in the LP;
 *  Warning! This method is only applicable on columns, that are completely linked to their rows (e.g. a column
 *  that is in the current LP and the LP was solved, or a column that was in a solved LP and didn't change afterwards
 */
extern
int SCIPcolGetNLPNonz(
   COL*             col                 /**< LP column */
   );

/** gets array with rows of nonzero entries */
extern
ROW** SCIPcolGetRows(
   COL*             col                 /**< LP column */
   );

/** gets array with coefficients of nonzero entries */
extern
Real* SCIPcolGetVals(
   COL*             col                 /**< LP column */
   );

/** gets node number of the last node in current branch and bound run, where strong branching was used on the
 *  given column, or -1 if strong branching was never applied to the column in current run
 */
extern
Longint SCIPcolGetStrongbranchNode(
   COL*             col                 /**< LP column */
   );

#else

/* In optimized mode, the methods are implemented as defines to reduce the number of function calls and
 * speed up the algorithms.
 */

#define SCIPcolGetObj(col)              (col)->obj
#define SCIPcolGetLb(col)               (col)->lb
#define SCIPcolGetUb(col)               (col)->ub
#define SCIPcolGetBestBound(col)        ((col)->obj >= 0.0 ? (col)->lb : (col)->ub)
#define SCIPcolGetPrimsol(col)          ((col)->lppos >= 0 ? (col)->primsol : 0.0)
#define SCIPcolGetVar(col)              (col)->var
#define SCIPcolIsIntegral(col)          (col)->integral
#define SCIPcolIsRemoveable(col)        (col)->removeable
#define SCIPcolGetLPPos(col)            (col)->lppos
#define SCIPcolGetLPDepth(col)          (col)->lpdepth
#define SCIPcolIsInLP(col)              ((col)->lppos >= 0)
#define SCIPcolGetNNonz(col)            (col)->len
#define SCIPcolGetNLPNonz(col)          (col)->nlprows
#define SCIPcolGetRows(col)             (col)->rows
#define SCIPcolGetVals(col)             (col)->vals
#define SCIPcolGetStrongbranchNode(col) (col)->strongbranchnode

#endif




/*
 * Row methods
 */

/** locks an unmodifiable row, which forbids further changes; has no effect on modifiable rows */
extern
void SCIProwLock(
   ROW*             row                 /**< LP row */
   );

/** unlocks a lock of an unmodifiable row; a row with no sealed lock may be modified; has no effect on modifiable rows */
extern
void SCIProwUnlock(
   ROW*             row                 /**< LP row */
   );

/** returns the scalar product of the coefficient vectors of the two given rows */
extern
Real SCIProwGetScalarProduct(
   ROW*             row1,               /**< first LP row */
   ROW*             row2                /**< second LP row */
   );

/** returns the degree of parallelism between the hyperplanes defined by the two row vectors v, w:
 *  p = |v*w|/(|v|*|w|);
 *  the hyperplanes are parellel, iff p = 1, they are orthogonal, iff p = 0
 */
extern
Real SCIProwGetParallelism(
   ROW*             row1,               /**< first LP row */
   ROW*             row2                /**< second LP row */
   );

/** returns the degree of orthogonality between the hyperplanes defined by the two row vectors v, w:
 *  o = 1 - |v*w|/(|v|*|w|);
 *  the hyperplanes are orthogonal, iff p = 1, they are parallel, iff p = 0
 */
extern
Real SCIProwGetOrthogonality(
   ROW*             row1,               /**< first LP row */
   ROW*             row2                /**< second LP row */
   );

/** output row to file stream */
extern
void SCIProwPrint(
   ROW*             row,                /**< LP row */
   FILE*            file                /**< output file (or NULL for standard output) */
   );

#ifndef NDEBUG

/* In debug mode, the following methods are implemented as function calls to ensure
 * type validity.
 */

/** get number of nonzero entries in row vector */
extern
int SCIProwGetNNonz(
   ROW*             row                 /**< LP row */
   );

/** get number of nonzero entries in row vector, that correspond to columns currently in the LP;
 *  Warning! This method is only applicable on rows, that are completely linked to their columns (e.g. a row
 *  that is in the current LP and the LP was solved, or a row that was in a solved LP and didn't change afterwards
 */
extern
int SCIProwGetNLPNonz(
   ROW*             row                 /**< LP row */
   );

/** gets array with columns of nonzero entries */
extern
COL** SCIProwGetCols(
   ROW*             row                 /**< LP row */
   );

/** gets array with coefficients of nonzero entries */
extern
Real* SCIProwGetVals(
   ROW*             row                 /**< LP row */
   );

/** gets constant shift of row */
extern
Real SCIProwGetConstant(
   ROW*             row                 /**< LP row */
   );

/** gets euclidean norm of row vector */
extern
Real SCIProwGetNorm(
   ROW*             row                 /**< LP row */
   );

/** returns the left hand side of the row */
extern
Real SCIProwGetLhs(
   ROW*             row                 /**< LP row */
   );

/** returns the right hand side of the row */
extern
Real SCIProwGetRhs(
   ROW*             row                 /**< LP row */
   );

/** gets the dual LP solution of a row */
extern
Real SCIProwGetDualsol(
   ROW*             row                 /**< LP row */
   );

/** returns the name of the row */
extern
const char* SCIProwGetName(
   ROW*             row                 /**< LP row */
   );

/** gets unique index of row */
extern
int SCIProwGetIndex(
   ROW*             row                 /**< LP row */
   );

/** returns TRUE iff the activity of the row (without the row's constant) is always integral in a feasible solution */
extern
Bool SCIProwIsIntegral(
   ROW*             row                 /**< LP row */
   );

/** returns TRUE iff row is only valid locally */
extern
Bool SCIProwIsLocal(
   ROW*             row                 /**< LP row */
   );

/** returns TRUE iff row is modifiable during node processing (subject to column generation) */
extern
Bool SCIProwIsModifiable(
   ROW*             row                 /**< LP row */
   );

/** returns TRUE iff row is removeable from the LP (due to aging or cleanup) */
extern
Bool SCIProwIsRemoveable(
   ROW*             row                 /**< LP row */
   );

/** gets position of row in current LP, or -1 if it is not in LP */
extern
int SCIProwGetLPPos(
   ROW*             row                 /**< LP row */
   );

/** gets depth in the tree where the row entered the LP, or -1 if it is not in LP */
extern
int SCIProwGetLPDepth(
   ROW*             row                 /**< LP row */
   );

/** returns TRUE iff row is member of current LP */
extern
Bool SCIProwIsInLP(
   ROW*             row                 /**< LP row */
   );

#else

/* In optimized mode, the methods are implemented as defines to reduce the number of function calls and
 * speed up the algorithms.
 */

#define SCIProwGetNNonz(row)            (row)->len
#define SCIProwGetNLPNonz(row)          (row)->nlpcols
#define SCIProwGetCols(row)             (row)->cols
#define SCIProwGetVals(row)             (row)->vals
#define SCIProwGetConstant(row)         (row)->constant
#define SCIProwGetNorm(row)             sqrt((row)->sqrnorm)
#define SCIProwGetLhs(row)              (row)->lhs
#define SCIProwGetRhs(row)              (row)->rhs
#define SCIProwGetDualsol(row)          ((row)->lppos >= 0 ? (row)->dualsol : 0.0)
#define SCIProwGetName(row)             (row)->name
#define SCIProwGetIndex(row)            (row)->index
#define SCIProwIsIntegral(row)          (row)->integral
#define SCIProwIsLocal(row)             (row)->local
#define SCIProwIsModifiable(row)        (row)->modifiable
#define SCIProwIsRemoveable(row)        (row)->removeable
#define SCIProwGetLPPos(row)            (row)->lppos
#define SCIProwGetLPDepth(row)          (row)->lpdepth
#define SCIProwIsInLP(row)              ((row)->lppos >= 0)

#endif


#endif
