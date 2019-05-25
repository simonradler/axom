// Copyright (c) 2017-2019, Lawrence Livermore National Security, LLC and
// other Axom Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)

#ifndef AXOM_PRIMAL_RADIXTREE_HPP_
#define AXOM_PRIMAL_RADIXTREE_HPP_

#include "axom/core/memory_management.hpp"
#include "axom/primal/spatial_acceleration/linear_bvh/aabb.hpp"

namespace axom
{
namespace primal
{
namespace bvh
{

/*!
 * \brief RadixTree provides a binary radix tree representation that stores a
 *  list of axis-aligned bounding boxes sorted according to their Morton code.
 *
 * \note This data-structure provides an intermediate representation that serves
 *  as the building-block to construct a BVH in parallel.
 */
template < typename FloatType, int NDIMS >
struct RadixTree
{
  int32  m_inner_size;
  int32  *m_left_children;
  int32  *m_right_children;
  int32  *m_parents;
  int32  *m_leafs;
  uint32 *m_mcodes;

  bvh::AABB< FloatType, NDIMS > *m_inner_aabbs;
  bvh::AABB< FloatType, NDIMS > *m_leaf_aabbs;

  void allocate( int32 size )
  {
    m_inner_size = size;
    m_left_children = axom::allocate<int32>(size);
    m_right_children = axom::allocate<int32>(size);
    m_parents = axom::allocate<int32>(size);
    m_inner_aabbs = axom::allocate< bvh::AABB< FloatType,NDIMS > >( size );
  }

  void deallocate()
  {
    axom::deallocate( m_left_children );
    axom::deallocate( m_right_children );
    axom::deallocate( m_parents );
    axom::deallocate( m_inner_aabbs );
  }

};

} /* namespace bvh */
} /* namespace primal */
} /* namespace axom */




#endif /* AXOM_RADIXTREE_HPP_ */
