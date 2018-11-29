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

// Axom includes
#include "axom/core.hpp"
#include "axom/mint.hpp"

// C/C++ includes
#include <cmath>

// aliases
namespace mint      = axom::mint;
namespace utilities = axom::utilities;
namespace policy    = mint::policy;
namespace xargs     = mint::xargs;
using IndexType     = mint::IndexType;

constexpr double R  = 2.5;
constexpr double M  = (2* M_PI) / 50.0;
/*!
 * \file
 *
 * \brief Illustrates how to construct and use a mint::CurvilinearMesh object.
 */

int main ( int AXOM_NOT_USED(argc), char** AXOM_NOT_USED(argv) )
{

  constexpr int N = 100;
  constexpr double h = 0.5;

  // STEP 0: construct a N x N curvilinear mesh
  mint::CurvilinearMesh mesh( N, N );
  double* x          = mesh.getCoordinateArray( mint::X_COORDINATE );
  double* y          = mesh.getCoordinateArray( mint::Y_COORDINATE );

  // STEP 1: add fiducial fields
  double* dx = mesh.createField< double >( "dx", mint::NODE_CENTERED );
  double* dy = mesh.createField< double >( "dy", mint::NODE_CENTERED );

  // STEP 2: fill in the coordinates
  mint::for_all_nodes< policy::serial, xargs::ij >(
    &mesh, AXOM_LAMBDA(IndexType nodeIdx, IndexType i, IndexType j)
  {

    const double xx     = h*i;
    const double yy     = h*j;
    const double alpha  = yy + R;
    const double beta   = xx*M;

    x[ nodeIdx ] = alpha * cos( beta );
    y[ nodeIdx ] = alpha * sin( beta );

    dx[ nodeIdx ] = utilities::random_real( -10.0, 10.0 );
    dy[ nodeIdx ] = utilities::random_real( -5.0, 5.0 );

  } );

  // STEP 3: dump file
  mint::write_vtk( &mesh, "curvilinear_mesh.vtk" );

  return 0;
}