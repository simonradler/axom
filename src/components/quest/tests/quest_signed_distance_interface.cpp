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

// Mint includes
#include "mint/config.hpp"
#include "mint/CellTypes.hpp"
#include "mint/Mesh.hpp"
#include "mint/MeshTypes.hpp"
#include "mint/UniformMesh.hpp"
#include "mint/UnstructuredMesh.hpp"
#include "mint/vtk_utils.hpp"

// Primal includes
#include "primal/BoundingBox.hpp"
#include "primal/Sphere.hpp"

// Quest includes
#include "quest/signed_distance.hpp"

// Google Test includes
#include "gtest/gtest.h"
#include "quest_test_utilities.hpp"  // for test-utility functions

// Slic includes
#include "slic/slic.hpp"
#include "slic/UnitTestLogger.hpp"
using axom::slic::UnitTestLogger;

// C/C++ includes
#include <fstream> // for std::ofstream

// Aliases
namespace quest  = axom::quest;
namespace mint   = axom::mint;
namespace primal = axom::primal;

using UnstructuredMesh = mint::UnstructuredMesh< mint::SINGLE_SHAPE >;

const char IGNORE_OUTPUT[] = ".*";

//------------------------------------------------------------------------------
//  HELPER METHODS
//------------------------------------------------------------------------------
namespace
{

/*!
 * \brief Generates a simple ASCII STL file consisting of a single triangle.
 *
 * \param [in] file the file to write to.
 * \pre file.empty() == false.
 *
 * \note Used primarily for debugging.
 */
void generate_stl_file( const std::string& file )
{
  EXPECT_FALSE( file.empty() );

  std::ofstream ofs( file.c_str( ) );
  EXPECT_TRUE( ofs.is_open() );

  ofs << "solid triangle" << std::endl;
  ofs << "\t facet normal 0.0 0.0 1.0" << std::endl;
  ofs << "\t\t outer loop" << std::endl;
  ofs << "\t\t\t vertex 0.0 0.0 0.0" << std::endl;
  ofs << "\t\t\t vertex 1.0 0.0 0.0" << std::endl;
  ofs << "\t\t\t vertex 0.0 1.0 0.0" << std::endl;
  ofs << "\t\t endloop" << std::endl;
  ofs << "\t endfacet" << std::endl;
  ofs << "endsolid triangle" << std::endl;

  ofs.close( );
}

/*!
 * \brief Returns the bounding box of the mesh.
 * \param [in] mesh pointer to the mesh instance.
 * \return bb bounding box of the mesh
 */
primal::BoundingBox< double,3 > getBounds( const axom::mint::Mesh* mesh )
{
  SLIC_ASSERT( mesh != AXOM_NULLPTR );

  primal::BoundingBox< double,3 > bb;
  primal::Point< double,3 > pt;

  const int nnodes = mesh->getNumberOfNodes();
  for ( int inode=0 ; inode < nnodes ; ++inode )
  {
    mesh->getNode( inode, pt.data() );
    bb.addPoint( pt );
  }

  return( bb );
}

/*!
 * \brief Generates a uniform mesh surrounding the given triangle mesh.
 * \param [in] mesh pointer to the input mesh.
 * \param [in] umesh pointer to the uniform mesh;
 */
void getUniformMesh( const UnstructuredMesh* mesh,
                     mint::UniformMesh*& umesh )
{
  SLIC_ASSERT( mesh != AXOM_NULLPTR );
  SLIC_ASSERT( umesh == AXOM_NULLPTR );

  constexpr int N = 16; // number of points along each dimension

  primal::BoundingBox< double,3 > bb = getBounds( mesh );
  bb.expand( 2.0 );

  const double* lo = bb.getMin().data();
  const double* hi = bb.getMax().data();

  // construct an N x N x N grid
  umesh = new mint::UniformMesh( lo, hi, N, N, N );
}


} /* end detail namespace */

//------------------------------------------------------------------------------
//  UNIT TESTS
//------------------------------------------------------------------------------
TEST( quest_signed_distance_interface_DeathTest, get_mesh_bounds_invalid_calls)
{
  EXPECT_FALSE( quest::signed_distance_initialized() );

  double lo[3];
  double hi[3];
  EXPECT_DEATH_IF_SUPPORTED( quest::signed_distance_get_mesh_bounds( lo, hi ),
                             IGNORE_OUTPUT );

  constexpr int NDIMS             = 3;
  constexpr double SPHERE_RADIUS  = 0.5;
  constexpr int SPHERE_THETA_RES  = 25;
  constexpr int SPHERE_PHI_RES    = 25;
  const double SPHERE_CENTER[ 3 ] = { 0.0, 0.0, 0.0 };

  UnstructuredMesh* surface_mesh = new UnstructuredMesh(NDIMS,mint::TRIANGLE);
  quest::utilities::getSphereSurfaceMesh( surface_mesh,
                                          SPHERE_CENTER,
                                          SPHERE_RADIUS,
                                          SPHERE_THETA_RES, SPHERE_PHI_RES );

  quest::signed_distance_init( surface_mesh );

  EXPECT_DEATH_IF_SUPPORTED(
      quest::signed_distance_get_mesh_bounds(AXOM_NULLPTR, hi),
      IGNORE_OUTPUT );

  EXPECT_DEATH_IF_SUPPORTED(
      quest::signed_distance_get_mesh_bounds(lo,AXOM_NULLPTR),
      IGNORE_OUTPUT );

  quest::signed_distance_finalize( );
  delete surface_mesh;
  surface_mesh = AXOM_NULLPTR;
}

//------------------------------------------------------------------------------
TEST( quest_signed_distance_interface_DeathTest, call_evaluate_before_init )
{
  EXPECT_FALSE( quest::signed_distance_initialized() );
  EXPECT_DEATH_IF_SUPPORTED( quest::signed_distance_evaluate( 0.0, 0.0 ),
                             IGNORE_OUTPUT );

  double x[ 2 ];
  double y[ 2 ];
  double z[ 2 ];
  double phi[ 2 ];

  EXPECT_DEATH_IF_SUPPORTED( quest::signed_distance_evaluate(x,y,z,2,phi),
                             IGNORE_OUTPUT );
}

//------------------------------------------------------------------------------
TEST( quest_signed_distance_interface_DeathTest, set_params_after_init )
{
  EXPECT_FALSE( quest::signed_distance_initialized() );

  constexpr int NDIMS = 3;
  constexpr double SPHERE_RADIUS  = 0.5;
  constexpr int SPHERE_THETA_RES  = 25;
  constexpr int SPHERE_PHI_RES    = 25;
  const double SPHERE_CENTER[ 3 ] = { 0.0, 0.0, 0.0 };

  // STEP 0: initialiaze quest
  UnstructuredMesh* surface_mesh = new UnstructuredMesh(NDIMS,mint::TRIANGLE);
  quest::utilities::getSphereSurfaceMesh( surface_mesh,
                                          SPHERE_CENTER,
                                          SPHERE_RADIUS,
                                          SPHERE_THETA_RES, SPHERE_PHI_RES );

  quest::signed_distance_init( surface_mesh );

  // STEP 1: setting parameters after init() should fail
  EXPECT_DEATH_IF_SUPPORTED( quest::signed_distance_set_dimension( 3 ),
                             IGNORE_OUTPUT );
  EXPECT_DEATH_IF_SUPPORTED(
      quest::signed_distance_set_geometry( quest::WATERTIGHT ),
      IGNORE_OUTPUT );

  EXPECT_DEATH_IF_SUPPORTED( quest::signed_distance_set_max_levels( 5 ),
                             IGNORE_OUTPUT );

  EXPECT_DEATH_IF_SUPPORTED( quest::signed_distance_set_max_occupancy( 5 ),
                             IGNORE_OUTPUT );

  EXPECT_DEATH_IF_SUPPORTED( quest::signed_distance_set_verbose( true ),
                             IGNORE_OUTPUT );

  // STEP 2: finalize
  quest::signed_distance_finalize();
  delete surface_mesh;
  surface_mesh = AXOM_NULLPTR;
}

//------------------------------------------------------------------------------
TEST( quest_signed_distance_interface, initialize )
{
  int rc = -1;

  // initializing with an empty file should return a non-zero value
  rc = quest::signed_distance_init( "" );
  EXPECT_TRUE( rc != 0 );
  EXPECT_FALSE( quest::signed_distance_initialized() );

  // initializing with a file that does not exist, should fail.
  rc = quest::signed_distance_init( "badfile.stl" );
  EXPECT_TRUE( rc != 0 );
  EXPECT_FALSE( quest::signed_distance_initialized() );

  // generate a temp STL file
  const std::string fileName = "test_triangle.stl";
  generate_stl_file( fileName );

  // test valid initialization
  quest::signed_distance_set_dimension( 3 );
  rc = quest::signed_distance_init( fileName );
  EXPECT_EQ( rc, 0 );
  EXPECT_TRUE( quest::signed_distance_initialized() );

  // finalize
  quest::signed_distance_finalize();
  EXPECT_FALSE( quest::signed_distance_initialized() );

  // remove temp STL file
  std::remove( fileName.c_str( ) );
}

//------------------------------------------------------------------------------
TEST( quest_signed_distance_interface, get_mesh_bounds )
{
  constexpr int NDIMS             = 3;
  constexpr double SPHERE_RADIUS  = 0.5;
  constexpr int SPHERE_THETA_RES  = 25;
  constexpr int SPHERE_PHI_RES    = 25;
  const double SPHERE_CENTER[ 3 ] = { 0.0, 0.0, 0.0 };
  constexpr double EXPECTED_LO    = -0.5;
  constexpr double EXPECTED_HI    =  0.5;

  double lo[ NDIMS ];
  double hi[ NDIMS ];

  UnstructuredMesh* surface_mesh = new UnstructuredMesh(NDIMS,mint::TRIANGLE);
  quest::utilities::getSphereSurfaceMesh( surface_mesh,
                                          SPHERE_CENTER,
                                          SPHERE_RADIUS,
                                          SPHERE_THETA_RES, SPHERE_PHI_RES );

  quest::signed_distance_init( surface_mesh );


  quest::signed_distance_get_mesh_bounds( lo, hi );

  for ( int i=0; i < NDIMS; ++i )
  {
    EXPECT_DOUBLE_EQ( lo[ i ], EXPECTED_LO );
    EXPECT_DOUBLE_EQ( hi[ i ], EXPECTED_HI );
  }

  quest::signed_distance_finalize( );

  delete surface_mesh;
  surface_mesh = AXOM_NULLPTR;
}

//------------------------------------------------------------------------------
TEST( quest_signed_distance_interface, analytic_sphere )
{
  // STEP 0: constants
  constexpr int NDIMS = 3;
  constexpr double l1norm_expected = 6.7051997372579715;
  constexpr double l2norm_expected = 2.5894400431865519;
  constexpr double linf_expected   = 0.00532092;
  constexpr double TOL             = 1.e-3;

  constexpr double SPHERE_RADIUS  = 0.5;
  constexpr int SPHERE_THETA_RES  = 25;
  constexpr int SPHERE_PHI_RES    = 25;
  const double SPHERE_CENTER[ 3 ] = { 0.0, 0.0, 0.0 };

  constexpr int MAX_LEVELS    = 10;
  constexpr int MAX_OCCUPANCY = 5;

  // STEP 1: create analytic sphere object to compare results with
  primal::Sphere< double,3 > analytic_sphere( SPHERE_RADIUS );

  // STEP 2: generate sphere mesh to pass to the signed distance query
  UnstructuredMesh* surface_mesh = new UnstructuredMesh(NDIMS,mint::TRIANGLE);
  quest::utilities::getSphereSurfaceMesh( surface_mesh,
                                          SPHERE_CENTER,
                                          SPHERE_RADIUS,
                                          SPHERE_THETA_RES, SPHERE_PHI_RES );

  // STEP 1: generate the uniform mesh where the signed distance will be
  //         computed
  mint::UniformMesh* umesh = AXOM_NULLPTR;
  getUniformMesh( surface_mesh, umesh );

  // STEP 2: create node-centered fields on the uniform mesh to store
  //         the signed distance etc.
  double* phi_computed =
      umesh->createField< double >( "phi_computed", mint::NODE_CENTERED );
  double* phi_expected =
      umesh->createField< double >( "phi_expected", mint::NODE_CENTERED );
  double* phi_diff =
      umesh->createField< double >( "phi_diff", mint::NODE_CENTERED );
  double* phi_err =
      umesh->createField< double >( "phi_err", mint::NODE_CENTERED );

  // STEP 3: initialize the signed distance query
  quest::signed_distance_set_max_levels( MAX_LEVELS );
  quest::signed_distance_set_max_occupancy( MAX_OCCUPANCY );
  quest::signed_distance_init( surface_mesh );
  EXPECT_TRUE( quest::signed_distance_initialized() );

  // STEP 4: Compute signed distance
  double l1norm = 0.0;
  double l2norm = 0.0;
  double linf   = std::numeric_limits< double >:: min( );
  mint::IndexType nnodes = umesh->getNumberOfNodes();
  for ( mint::IndexType inode=0; inode < nnodes; ++inode )
  {
    double pt[ NDIMS ];
    umesh->getNode( inode, pt );

    phi_computed[ inode ] = quest::signed_distance_evaluate(pt[0],pt[1],pt[2]);
    phi_expected[ inode ] = analytic_sphere.computeSignedDistance( pt );
    EXPECT_NEAR( phi_computed[ inode ], phi_expected[ inode ], 1.e-2 );

    // compute error
    phi_diff[ inode ] = phi_computed[ inode ] - phi_expected[ inode ];
    phi_err[ inode ]  = std::fabs( phi_diff[ inode ] );

    // update norms
    l1norm += phi_err[ inode ];
    l2norm += phi_diff[ inode ];
    linf = ( phi_err[ inode ] > linf ) ? phi_err[ inode ] : linf;

  } // END for all nodes

  l2norm = std::sqrt( l2norm );

  EXPECT_NEAR( l1norm_expected, l1norm, TOL );
  EXPECT_NEAR( l2norm_expected, l2norm, TOL );
  EXPECT_NEAR( linf_expected, linf, TOL );

  // STEP 5: finalize the signed distance query
  quest::signed_distance_finalize( );
  EXPECT_FALSE( quest::signed_distance_initialized() );

  // STEP 6: delete mesh objects
  delete umesh;
  delete surface_mesh;

  umesh        = AXOM_NULLPTR;
  surface_mesh = AXOM_NULLPTR;
}

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{

#ifdef AXOM_USE_MPI
  MPI_Init( &argc, &argv );
#endif

  int result = 0;

  ::testing::InitGoogleTest(&argc, argv);

  UnitTestLogger logger;  // create & initialize test logger,

  // finalized when exiting main scope

  result = RUN_ALL_TESTS();

#ifdef AXOM_USE_MPI
  MPI_Finalize();
#endif

  return result;
}
