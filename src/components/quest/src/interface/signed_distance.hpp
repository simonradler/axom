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

#ifndef QUEST_SIGNED_DISTANCE_INTERFACE_HPP_
#define QUEST_SIGNED_DISTANCE_INTERFACE_HPP_

// Axom includes
#include "axom/config.hpp"  // for compile-time definitions

// Quest includes
#include "quest/mpicomm_wrapper.hpp" // for MPI_Comm / MPI_COMM_SELF

// C/C++ includes
#include <string>   // for std::string

/*!
 * \file
 *
 * \brief The Signed Distance Query evaluates the signed distance function,
 *  \f$ \phi \f$, at an arbitrary point, \f$ \vec{p} \f$, with respect to an
 *  oriented surface, \f$ \mathcal S\f$
 *
 *  Given a discrete representation of the surface, i.e., a surface mesh, that
 *  is oriented according to an outward-facing unit normal, the signed distance
 *  function, \f$ \phi(\vec{p}) \f$, evaluated at a point, \f$ \vec{p} \f$, is
 *  defined such that:
 *  \f{equation}{
 *    \phi( \vec{p} ) =
 *      \begin{cases}
 *        +d,   & \vec{p} \mbox{ is in front of the surface, } \mathcal{S}  \\
 *        \pm0, & \vec{p} \mbox{ is on the boundary, } \partial\mathcal{S}  \\
 *        -d,   & \vec{p} \mbox{ is behind the surface, } \mathcal{S}
 *       \end{cases}
 *  \f}
 *
 *  \warning Currently, the Signed Distance Query supports:
 *    * reading in surface meshes given in the
 *      <a href="https://en.wikipedia.org/wiki/STL_(file_format)">STL format</a>
 *    * oriented 3D triangular surface meshes
 *
 * <b> Usage Example: </b>
 * \code
 *
 *  int main( int argc, char** argv )
 *  {
 *    MPI_Init( &argc, &argv );
 *    MPI_Comm mpi_comm = MPI_COMM_WORLD;
 *    ...
 *    // STEP 0: set parameters
 *    quest::signed_distance_set_dimension( 3 );
 *    quest::signed_distance_set_max_levels( 25 );
 *    quest::signed_distance_set_max_occupancy( 5 );
 *
 *    // STEP 1: initialize
 *    quest::signed_distance_init( "path/to/stlmesh.stl", mpi_comm );
 *
 *    // STEP 2: evaluate the signed distance at N query points
 *    for ( int i=0; i < N; ++i )
 *    {
 *       ...
 *       phi[ i ] = quest::signed_distance_evaluate( x[i], y[i], z[i] );
 *
 *    } // END for all N query points
 *
 *    // STEP 3: Finalize
 *    quest::signed_distance_finalize( );
 *
 *    MPI_Finalize( );
 *    return 0;
 *  }
 *
 * \endcode
 */

namespace axom
{

// Forward Mint declarations
namespace mint
{
class Mesh;
}

namespace quest
{

/*!
 * \brief Enumerates the supported input types for the Signed Distance Query.
 */
enum GeometryType
{
  WATERTIGHT=0,
  NUM_GEOMETRY_TYPES
};

/// \name Signed Distance Query Initialization Methods
/// @{

/*!
 * \brief Initializes the Signed Distance Query with a surface given in an
 *  <a href="https://en.wikipedia.org/wiki/STL_(file_format)">STL</a> formatted
 *  file.
 *
 * \param [in] file the name of the file consisting of the surface mesh.
 * \param [in] comm the MPI communicator (applicable when MPI is available)
 *
 * \return status zero on success, or a non-zero value if an error occurs
 *
 * \note The Signed Distance Query currently only supports reading in meshes
 *  defined in <a href="https://en.wikipedia.org/wiki/STL_(file_format)">STL</a>
 *  file format.
 *
 * \pre file.empty() == false
 * \pre comm != MPI_COMM_NULL (when MPI is available)
 * \pre signed_distance_initialized() == false
 * \post signed_distance_initialized() == true
 */
int signed_distance_init( const std::string& file,
                          MPI_Comm comm=MPI_COMM_SELF );

/*!
 * \brief Initializes the Signed Distance Query with the given surface mesh
 *
 * \param [in] m pointer to the surface mesh object
 * \param [in] comm the MPI communicator (applicable whem MPI is available)
 *
 * \return status zero on success, or a non-zero value if an error occurs.
 *
 * \note The Signed Distance Query currently only supports 3-D triangular
 *  surface meshes.
 *
 * \pre m != AXOM_NULLPTR
 * \pre comm != MPI_COMM_NULL (when MPI is available)
 * \pre signed_distance_initialized() == false
 * \post signed_distance_initialized() == true
 *
 * \see mint::Mesh
 */
int signed_distance_init( const mint::Mesh* m, MPI_Comm comm=MPI_COMM_SELF );

/// @}

/*!
 * \brief Checks if the Signed Distance Query has been initialized
 * \return status true if initialized, else, false.
 */
bool signed_distance_initialized( );

/*!
 * \brief Computes the bounds of the specified input mesh supplied to the
 *  Signed Distance Query.
 *
 * \param [out] lo buffer to store the lower bound mesh coordinates.
 * \param [out] hi buffer to store the upper bound mesh coordinates.
 *
 * \pre lo != AXOM_NULLPTR
 * \pre hi != AXOM_NULLPTR
 * \pre hi & lo must point to a buffer that is at least ndims long.
 * \pre signed_distance_initialized() == true
 */
void signed_distance_get_mesh_bounds( double* lo, double* hi );

/// \name Signed Distance Query Options
/// @{

/*!
 * \brief Sets the dimension for the Signed Distance Query.
 * \param [in] dim the dimension, e.g., 2 or 3
 *
 * \warning The Signed Distance function is currently supported in 3D
 * \note Options must be set before initializing the Signed Distance Query.
 */
void signed_distance_set_dimension( int dim );

/*!
 * \brief Sets the surface mesh input type for the Signed Distance Query
 * \param [in] type the input surface mesh type.
 *
 * \note By default the input type is assumed to be a Watertight surface mesh.
 * \note Options must be set before initializing the Signed Distance Query.
 *
 * \see GeometryType
 */
void signed_distance_set_geometry( int type );

/*!
 * \brief Sets the maximum levels of subdivision for the BVH decomposition.
 * \param [in] maxLevels the maximum levels of subdivision.
 *
 * \note Options must be set before initializing the Signed Distance Query.
 */
void signed_distance_set_max_levels( int maxLevels );

/*!
 * \brief Sets threshold on the max number of items per BVH bin. This option
 *  controls the BVH decomposition.
 *
 * \param [in] threshold max number of items per bin.
 *
 * \note Options must be set before initializing the Signed Distance Query.
 *
 * \pre theshold >= 1
 */
void signed_distance_set_max_occupancy( int threshold );

/*!
 * \brief Enables/Disables verbose output for the Signed Distance Query.
 * \param [in] status flag indicating whether to enable/disable verbose output
 *
 * \note Options must be set before initializing the Signed Distance Query.
 *
 * \note Currently, this is only applicable when the Signed Distance Query
 *  initializes the SLIC logging environment.
 */
void signed_distance_set_verbose( bool status );

/// @}

/// \name Signed Distance Query Evaluation Methods
/// @{

/*!
 * \brief Evaluates the signed distance function at the given point.
 *
 * \param [in] x the x-coordinate of the point in query
 * \param [in] y the y-coordinate of the point in query
 * \param [in] z the z-coordinate of the point in query
 *
 * \return d the signed distance evaluated at the specified point.
 */
double signed_distance_evaluate( double x, double y, double z=0.0 );

/*!
 * \brief Evaluates the signed distance function at the given set of points.
 *
 * \param [in] x array consisting of the x-coordinates for each query point
 * \param [in] y array consisting of the y-coordinates for each query point
 * \param [in] z array consisting of the z-coordinates for each query point
 * \param [in] npoints the number of query point
 * \param [out] phi output array storing the signed distance of each point
 *
 * \pre x != AXOM_NULLPTR
 * \pre y != AXOM_NULLPTR
 * \pre z != AXOM_NULLPTR
 * \pre phi != AXOM_NULLPTR
 */
void signed_distance_evaluate( const double* x,
                               const double* y,
                               const double* z,
                               int npoints,
                               double* phi );

/// @}

/// \name Signed Distance Query Finalization Methods
/// @{

/*!
 * \brief Finalizes the SignedDistance query
 * \post signed_distance_initialized() == false.
 */
void signed_distance_finalize();

/// @}

} // end namespace quest
} // end namespace axom

#endif /* QUEST_SIGNED_DISTANCE_INTERFACE_HPP_ */
