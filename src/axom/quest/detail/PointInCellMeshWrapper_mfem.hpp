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


#ifndef QUEST_POINT_IN_CELL_MFEM_IMPL_HPP_
#define QUEST_POINT_IN_CELL_MFEM_IMPL_HPP_

/*!
 * \file
 *
 * This file contains implementation classes for an mfem-based
 * specialization of quest's PointInCell query on meshes or
 * arbitrary order.
 *
 * It defines a mesh tag \a quest_point_in_cell_mfem_tag
 * and specializations of \a axom::quest::PointInCellTraits and
 * \a axom::quest::detail::PointInCellMeshWrapper for this tag.
 *
 * \sa axom::quest::PointInCell
 * \sa axom::quest::PointInCellTraits
 * \sa axom::quest::detail::PointInCellMeshWrapper
 */


#include "axom/config.hpp"
#include "axom/core/Macros.hpp"

#include "axom/slic/interface/slic.hpp"

#include "axom/primal/geometry/Point.hpp"
#include "axom/primal/geometry/BoundingBox.hpp"

#ifdef AXOM_USE_MFEM
#  include "mfem.hpp"
#else
#  error "PointInCell_mfem_impl depends on mfem."
#endif


namespace axom
{
namespace quest
{


/*! Tag for mfem-based specialization of the PointInCell query */
struct quest_point_in_cell_mfem_tag {};

// Predeclare PointInCellTraitsClass
template<typename mesh_tag>
struct PointInCellTraits;

/*!
 * Specialization of PointInCellTraits for \a quest_point_in_cell_mfem_tag
 *
 * \sa PointInCellTraits
 */
template<>
struct PointInCellTraits<quest_point_in_cell_mfem_tag>
{
public:
  typedef mfem::Mesh MeshType;
  typedef int IndexType;

  /*!  Special value to indicate an unsuccessful query */
  enum Values : IndexType { NO_CELL = -1 };
};

namespace detail
{

// Pre-declare classes to specialize for the mfem mesh_tag

template <typename mesh_tag>
class PointInCellMeshWrapper;


/*!
 * Wraps MFEM mesh access functionality for the PointInCell class.
 *
 * \note Specialization of PointInCellMeshWrapper
 * for \a quest_point_in_cell_mfem_tag
 *
 * \sa PointInCellMeshWrapper for required interface
 */
template <>
class PointInCellMeshWrapper< quest_point_in_cell_mfem_tag >
{
public:
  typedef int IndexType;
  typedef PointInCellMeshWrapper< quest_point_in_cell_mfem_tag > MeshWrapper;

  PointInCellMeshWrapper(mfem::Mesh* mesh) : m_mesh(mesh)
  {
    // Some sanity checks
    SLIC_ASSERT( m_mesh != nullptr);

    m_isHighOrder =
      (m_mesh->GetNodalFESpace() != nullptr) && (m_mesh->GetNE() > 0);
  }

  /*! Predicate to check if the given basis is positive (i.e. Bernstein) */
  static bool isPositiveBasis(const mfem::FiniteElementCollection* fec)
  {
    // HACK: Check against several common expected FE types

    if(fec == nullptr)
    {
      return false;
    }

    if(const mfem::H1_FECollection* h1Fec =
         dynamic_cast<const mfem::H1_FECollection*>(fec))
    {
      return h1Fec->GetBasisType() == mfem::BasisType::Positive;
    }

    if(const mfem::L2_FECollection* l2Fec =
         dynamic_cast<const mfem::L2_FECollection*>(fec))
    {
      return l2Fec->GetBasisType() == mfem::BasisType::Positive;
    }

    if( dynamic_cast<const mfem::NURBSFECollection*>(fec)       ||
        dynamic_cast<const mfem::LinearFECollection*>(fec)      ||
        dynamic_cast<const mfem::QuadraticPosFECollection*>(fec) )
    {
      return true;
    }

    return false;
  }

  /*!
   * \brief Utility function to get a positive (i.e. Bernstein)
   * collection of bases corresponding to the given FiniteElementCollection.
   *
   * \return A pointer to a newly allocated FiniteElementCollection
   * corresponding to \a fec
   * \note   It is the user's responsibility to deallocate this pointer.
   * \pre    \a fec is not already positive
   */
  static mfem::FiniteElementCollection* getCorrespondingPositiveFEC(
    const mfem::FiniteElementCollection* fec,
    int order,
    int dim,
    int mapType)
  {
    SLIC_CHECK_MSG( !isPositiveBasis( fec),
                    "This function is only meant to be called "
                    "on non-positive finite element collection" );

    // Attempt to find the corresponding positive H1 fec
    if(dynamic_cast<const mfem::H1_FECollection*>(fec))
    {
      return new mfem::H1_FECollection(order, dim, mfem::BasisType::Positive);
    }

    // Attempt to find the corresponding positive L2 fec
    if(dynamic_cast<const mfem::L2_FECollection*>(fec))
    {
      // should we throw a not supported error here?
      return new mfem::L2_FECollection(order, dim, mfem::BasisType::Positive,
                                       mapType);
    }

    // Attempt to find the corresponding quadratic or cubic fec
    // Note: Linear FECollections are positive
    if(dynamic_cast<const mfem::QuadraticFECollection*>(fec) ||
       dynamic_cast<const mfem::CubicFECollection*>(fec) )
    {
      SLIC_ASSERT( order == 2 || order == 3);
      return new mfem::H1_FECollection(order, dim, mfem::BasisType::Positive);
    }

    // Give up -- return NULL
    return nullptr;
  }


  /*! Returns the number of elements in the mesh */
  int numElements() const { return m_mesh->GetNE(); }

  /*! Returns the dimension of the mesh */
  int meshDimension() const { return m_mesh->Dimension(); }

  /*! Get a pointer to the mesh */
  mfem::Mesh* getMesh() const { return m_mesh; }

  /*!
   * Computes the bounding boxes of all mesh elements
   *
   * \param [in] bboxScaleFactor A scaling factor to expand the bounding boxes
   * \param [out] eltBBoxes A vector of bounding boxes, one per mesh element
   * \param [out] meshBBox A bounding box for the mesh
   *
   * \pre \a eltBBoxes has space to store \a numElements() bounding boxes
   * \pre \a bboxScaleFactor >= 1.
   */
  template<int NDIMS>
  void computeBoundingBoxes(
    double bboxScaleFactor,
    std::vector< axom::primal::BoundingBox<double,NDIMS> >& eltBBoxes,
    axom::primal::BoundingBox<double,NDIMS>& meshBBox) const
  {
    SLIC_ASSERT( static_cast<int>(eltBBoxes.size()) >= numElements() );

    if(m_isHighOrder)
    {
      computeHighOrderBoundingBoxes(bboxScaleFactor, eltBBoxes, meshBBox);
    }
    else
    {
      computeLowOrderBoundingBoxes(bboxScaleFactor, eltBBoxes, meshBBox);
    }
  }

  /*!
   * Evaluate the position of a point within a mesh cell at the given
   * isoparametric coordinates.
   *
   * \param [in] eltIdx The index of the element  within the mesh
   * \param [in[ isopar The isoparametric coordinates at which to evaluate
   * \param [out] pos The computed coordinates of the evaluated point
   *
   * \pre \a isopar must be non-NULL and have \a meshDimension() coordinates
   * \pre \a pt must be non-NULL and have space for \a meshDimension() coords
   *
   * \sa PointInCell::reconstructPoint()
   */
  void reconstructPoint(IndexType eltIdx,
                        const double* isopar,
                        double* pt) const
  {
    const int dim = meshDimension();

    mfem::IsoparametricTransformation tr;
    m_mesh->GetElementTransformation(eltIdx, &tr);

    mfem::IntegrationPoint ip;
    ip.Set(isopar, dim);

    mfem::Vector v(pt, dim);
    tr.Transform(ip, v);
  }

  /*!
   * Attempts to find isoparametric coordinates \a isopar of
   * a point \a pt in space.
   *
   * \return True if \a pt is contained in the element, in which case
   * the coordinates of \a isopar will be in the unit cube (of dimension NDIMS)
   *
   * \sa PointInCell::locatePointInCell()
   */
  bool locatePointInCell(IndexType eltIdx,
                         const double* pt,
                         double* isopar) const
  {
    const int dim = meshDimension();

    mfem::IsoparametricTransformation tr;
    m_mesh->GetElementTransformation(eltIdx, &tr);
    mfem::Vector ptSpace(const_cast<double*>(pt), dim);

    mfem::IntegrationPoint ipRef;

    // Set up the inverse element transformation
    typedef mfem::InverseElementTransformation InvTransform;
    InvTransform invTrans(&tr);

    invTrans.SetSolverType( InvTransform::Newton );
    invTrans.SetInitialGuessType(InvTransform::ClosestPhysNode);

    // Status codes: {0 -> successful; 1 -> outside elt; 2-> did not converge}
    int err = invTrans.Transform(ptSpace, ipRef);


    ipRef.Get(isopar, dim);

    return (err == 0);
  }

private:
  /*!
   * Helper function to initialize the bounding boxes for a high-order mfem mesh
   *
   * \pre This function can only be called when m_isHighOrder is true
   * \pre bboxScaleFactor must be greater than or equal to 1.
   * \sa computeBoundingBoxes()
   */
  template<int NDIMS>
  void computeHighOrderBoundingBoxes(
    double bboxScaleFactor,
    std::vector< axom::primal::BoundingBox<double, NDIMS> >& eltBBoxes,
    axom::primal::BoundingBox<double,NDIMS>& meshBBox)  const
  {
    typedef axom::primal::Point<double, NDIMS> SpacePoint;
    typedef axom::primal::BoundingBox<double, NDIMS> SpatialBoundingBox;

    // Sanity checks
    SLIC_ASSERT( m_isHighOrder );
    SLIC_ASSERT(bboxScaleFactor >= 1.);

    /// Generate (or access existing) positive (Bernstein) nodal grid function
    const mfem::FiniteElementSpace* nodalFESpace = m_mesh->GetNodalFESpace();

    mfem::GridFunction* positiveNodes = nullptr;
    bool mustDeleteNodes = false;

    const mfem::FiniteElementCollection* nodalFEColl = nodalFESpace->FEColl();

    // Check if grid function is positive, if not create positive grid function
    if( MeshWrapper::isPositiveBasis( nodalFEColl ) )
    {
      positiveNodes = m_mesh->GetNodes();
    }
    else
    {
      // Assume that all elements of the mesh have the same order and geom type
      int order = nodalFESpace->GetOrder(0);
      int dim = meshDimension();
      auto GeomType = m_mesh->GetElementBaseGeometry(0);
      int mapType =
        (nodalFEColl != nullptr)
        ? nodalFEColl->FiniteElementForGeometry(GeomType)->GetMapType()
        : static_cast<int>(mfem::FiniteElement::VALUE);

      mfem::FiniteElementCollection* posFEColl =
        MeshWrapper::getCorrespondingPositiveFEC(nodalFEColl, order, dim,
                                                 mapType);

      SLIC_ASSERT_MSG(
        posFEColl != nullptr,
        "Problem generating a positive finite element collection "
        << "corresponding to the mesh's '"<< nodalFEColl->Name()
        << "' finite element collection.");

      if(posFEColl != nullptr)
      {
        // Create a positive (Bernstein) grid function for the nodes
        mfem::FiniteElementSpace* posFESpace =
          new mfem::FiniteElementSpace(m_mesh, posFEColl, NDIMS);
        positiveNodes = new mfem::GridFunction(posFESpace);

        // m_bernsteinNodes takes ownership of posFEColl's memory
        positiveNodes->MakeOwner(posFEColl);
        mustDeleteNodes = true;

        // Project the nodal grid function onto this
        positiveNodes->ProjectGridFunction(*(m_mesh->GetNodes()));
      }
    }

    // Output some information
    SLIC_INFO(
      "Mesh nodes fec -- "
      << nodalFEColl->Name()
      << " with ordering " << nodalFESpace->GetOrdering()
      << "\n\t -- Positive nodes are fec -- "
      << positiveNodes->FESpace()->FEColl()->Name()
      << " with ordering " << positiveNodes->FESpace()->GetOrdering() );


    /// For each element, compute bounding box, and overall mesh bbox
    mfem::Array<int> dofIndices;
    mfem::FiniteElementSpace* fes = positiveNodes->FESpace();
    const int numMeshElements = numElements();
    for(int elem=0 ; elem < numMeshElements ; ++elem)
    {
      SpatialBoundingBox& bbox = eltBBoxes[elem];

      // Add each dof of the element to the bbox
      // Note: positivity of Bernstein bases ensures that convex
      //       hull of element nodes contain entire element
      fes->GetElementDofs(elem, dofIndices);
      for(int i = 0 ; i< dofIndices.Size() ; ++i)
      {
        int nIdx = dofIndices[i];

        SpacePoint pt;
        for(int j=0 ; j< NDIMS ; ++j)
          pt[j] = (*positiveNodes)(fes->DofToVDof(nIdx,j));

        bbox.addPoint( pt );
      }

      // Slightly scale the bbox to account for numerical noise
      bbox.scale(bboxScaleFactor);

      meshBBox.addBox( bbox );
    }

    /// Clean up -- deallocate grid function if necessary
    if(mustDeleteNodes)
    {
      delete positiveNodes;
      positiveNodes = nullptr;
    }
  }

  /*!
   * Helper function to initialize the bounding boxes for a low-order mfem mesh
   *
   * \param bboxScaleFactor Scale factor for expanding bounding boxes
   * \note This function can only be called before the class is initialized
   * \pre This function can only be called when m_isHighOrder is false
   * \pre bboxScaleFactor must be greater than or equal to 1.
   *
   * \sa computeBoundingBoxes()
   */
  template<int NDIMS>
  void computeLowOrderBoundingBoxes(
    double bboxScaleFactor,
    std::vector< axom::primal::BoundingBox<double, NDIMS> >& eltBBoxes,
    axom::primal::BoundingBox<double,NDIMS>& meshBBox)  const
  {
    typedef axom::primal::Point<double, NDIMS> SpacePoint;
    typedef axom::primal::BoundingBox<double, NDIMS> SpatialBoundingBox;

    SLIC_ASSERT( !m_isHighOrder );
    SLIC_ASSERT( bboxScaleFactor >= 1. );

    /// For each element, compute bounding box, and overall mesh bbox
    const int numMeshElements = numElements();
    for(int elem=0 ; elem < numMeshElements ; ++elem)
    {
      SpatialBoundingBox& bbox = eltBBoxes[elem];

      mfem::Element* elt = m_mesh->GetElement(elem);
      int* eltVerts = elt->GetVertices();
      for(int i = 0 ; i< elt->GetNVertices() ; ++i)
      {
        int vIdx = eltVerts[i];
        bbox.addPoint( SpacePoint( m_mesh->GetVertex( vIdx ) ) );
      }

      // scale the bounding box to account for numerical noise
      bbox.scale(bboxScaleFactor);

      meshBBox.addBox( bbox );
    }
  }

private:
  mfem::Mesh* m_mesh;
  bool m_isHighOrder;
};



} // end namespace detail

} // end namespace quest
} // end namespace axom



#endif // QUEST_POINT_IN_CELL_MFEM_IMPL_HPP_
