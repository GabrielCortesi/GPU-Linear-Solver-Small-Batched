﻿#ifdef __CDT_PARSER__
#undef __CUDA_RUNTIME_H__
#include <cuda_runtime.h>
#endif

#include <cuda_runtime.h>
#include <math.h>
#include <string.h>
#include "utils.h"
#include "magma_types.h"
#include "operation_batched.h"


#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

/***************************************************************************//**
    Purpose
    -------
    SGETRF computes an LU factorization of a general M-by-N matrix A
    using partial pivoting with row interchanges.

    The factorization has the form
        A = P * L * U
    where P is a permutation matrix, L is lower triangular with unit
    diagonal elements (lower trapezoidal if m > n), and U is upper
    triangular (upper trapezoidal if m < n).

    This is the right-looking Level 3 BLAS version of the algorithm.

    This is a batched version that factors batchCount M-by-N matrices in parallel.
    dA, ipiv, and info become arrays with one entry per matrix.

    Arguments
    ---------
    @param[in]
    m       INTEGER
            The number of rows of each matrix A.  M >= 0.

    @param[in]
    n       INTEGER
            The number of columns of each matrix A.  N >= 0.

    @param[in,out]
    dA_array    Array of pointers, dimension (batchCount).
            Each is a REAL array on the GPU, dimension (LDDA,N).
            On entry, each pointer is an M-by-N matrix to be factored.
            On exit, the factors L and U from the factorization
            A = P*L*U; the unit diagonal elements of L are not stored.

    @param[in]
    ldda    INTEGER
            The leading dimension of each array A.  LDDA >= max(1,M).

    @param[out]
    ipiv_array  Array of pointers, dimension (batchCount), for corresponding matrices.
            Each is an INTEGER array, dimension (min(M,N))
            The pivot indices; for 1 <= i <= min(M,N), row i of the
            matrix was interchanged with row IPIV(i).

    @param[out]
    info_array  Array of INTEGERs, dimension (batchCount), for corresponding matrices.
      -     = 0:  successful exit
      -     < 0:  if INFO = -i, the i-th argument had an illegal value
                  or another error occured, such as memory allocation failed.
      -     > 0:  if INFO = i, U(i,i) is exactly zero. The factorization
                  has been completed, but the factor U is exactly
                  singular, and division by zero will occur if it is used
                  to solve a system of equations.

    @param[in]
    batchCount  INTEGER
                The number of matrices to operate on.

    @param[in]
    queue   magma_queue_t
            Queue to execute in.

    @ingroup magma_getrf_batched
*******************************************************************************/
extern "C" int
linearDecompSLU_batched(
    int m, int n,
    float** dA_array,
    int ldda,
    int * *ipiv_array, int * info_array,
    int batchCount, cudaStream_t queue)
{
#define dAarray(i_, j_)  dA_array, i_, j_   
#define ipiv_array(i_)    ipiv_array, i_

    int min_mn = min(m, n);
    /* Check arguments */
    int arginfo = 0;
    if (m < 0)
        arginfo = -1;
    else if (n < 0)
        arginfo = -2;
    else if (ldda < max(1, m))
        arginfo = -4;

    if (arginfo != 0) {
        utils_reportError(__func__, -(arginfo));
        return arginfo;
    }

    /* Quick return if possible */
    if (m == 0 || n == 0)
        if (min_mn == 0) return arginfo;

    /* Special case for tiny square matrices */
    if (m == n && m <= 32) {
        int arch = utils_getdevice_arch();
        if (arch >= 700) {
            return magma_sgetrf_batched_smallsq_noshfl(m, dA_array, ldda, ipiv_array, info_array, batchCount, queue);
        }
        else {
            return magma_sgetrf_batched_smallsq_shfl(m, dA_array, ldda, ipiv_array, info_array, batchCount, queue);
        }
    }

#undef dAarray
#undef ipiv_array
}

#undef min
#undef max
