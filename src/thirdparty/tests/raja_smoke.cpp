/*
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Copyright (c) 2017-2018, Lawrence Livermore National Security, LLC.
 *
 * Produced at the Lawrence Livermore National Laboratory
 *
 * LLNL-CODE-741217
 *
 * All rights reserved.
 *
 * This file is part of Axom.
 *
 * For details about use and distribution, please read axom/LICENSE.
 *
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#include "axom/config.hpp"                  // for compile-time definitions
#include "axom/core/memory_management.hpp"  // alloc() / free() methods
#include "axom/core/Macros.hpp"             // for AXOM_LAMBDA

// RAJA includes
#include "RAJA/RAJA.hpp"     // for RAJA

// Google Test
#include "gtest/gtest.h"     // for google test functions

// C/C++ includes
#include <iostream> // for std::cout

//------------------------------------------------------------------------------
// HELPER METHODS
//------------------------------------------------------------------------------
namespace
{

template< typename execution_policy >
void raja_basic_usage_test( )
{
  constexpr int N = 100;
  int* a = axom::alloc< int >( N );
  int* b = axom::alloc< int >( N );
  int* c = axom::alloc< int >( N );

  // initialize
  RAJA::forall< execution_policy >( RAJA::RangeSegment( 0, N ), 
    AXOM_LAMBDA( int i ) {
      a[ i ] = b[ i ] = 1;
      c[ i ] = 0;
    }
  );

  // add vectors
  RAJA::forall< execution_policy >( RAJA::RangeSegment( 0, N ),
    AXOM_LAMBDA( int i ) {
      c[ i ] = a[ i ] + b[ i ];
    }
  );

  // check result in serial
  for ( int i = 0 ; i < N ; ++i )
  {
    EXPECT_EQ( c[ i ], 2 );
  }

  axom::free( a );
  axom::free( b );
  axom::free( c );
}

} /* end anonymous namespace */

//------------------------------------------------------------------------------
// UNIT TESTS
//------------------------------------------------------------------------------
AXOM_CUDA_TEST( raja_smoke, basic_use )
{
  std::cout << "Testing RAJA Sequential execution" << std::endl;
  raja_basic_usage_test< RAJA::seq_exec >( );

#if defined(AXOM_USE_OPENMP) && defined(RAJA_ENABLE_OPENMP)
  std::cout << "Testing RAJA OpenMP(CPU) execution" << std::endl;
  raja_basic_usage_test< RAJA::omp_parallel_for_exec >( );
#endif

#if defined(AXOM_USE_CUDA) && defined(RAJA_ENABLE_CUDA)
  std::cout << "Testing RAJA CUDA execution" << std::endl;
  constexpr int BLOCKSIZE = 256;
  raja_basic_usage_test< RAJA::cuda_exec< BLOCKSIZE > >( );
#endif
}

//------------------------------------------------------------------------------
int main( int argc, char** argv )
{
  ::testing::InitGoogleTest(&argc, argv);
  int result = RUN_ALL_TESTS( );
  return( result );
}
