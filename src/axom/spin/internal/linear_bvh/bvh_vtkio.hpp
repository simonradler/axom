// Copyright (c) 2017-2022, Lawrence Livermore National Security, LLC and
// other Axom Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)

#ifndef AXOM_SPIN_LINEAR_BVH_VTKIO_HPP_
#define AXOM_SPIN_LINEAR_BVH_VTKIO_HPP_

#include "axom/primal/geometry/BoundingBox.hpp"

namespace axom
{
namespace spin
{
namespace internal
{
namespace linear_bvh
{
//------------------------------------------------------------------------------
template <typename FloatType>
void write_box2d(const FloatType& xmin,
                 const FloatType& ymin,
                 const FloatType& xmax,
                 const FloatType& ymax,
                 int32& numPoints,
                 int32& numBins,
                 std::ostringstream& nodes,
                 std::ostringstream& cells)
{
  nodes << xmin << " " << ymin << " 0.0\n";
  nodes << xmax << " " << ymin << " 0.0\n";
  nodes << xmax << " " << ymax << " 0.0\n";
  nodes << xmin << " " << ymax << " 0.0\n";

  constexpr int32 NUM_NODES = 4;
  int32 offset = numPoints;
  cells << NUM_NODES << " ";
  for(int32 i = 0; i < NUM_NODES; ++i)
  {
    cells << (offset + i) << " ";
  }
  cells << "\n";

  numBins += 1;
  numPoints += NUM_NODES;
}

//------------------------------------------------------------------------------
template <typename FloatType>
void write_box3d(const FloatType& xmin,
                 const FloatType& ymin,
                 const FloatType& zmin,
                 const FloatType& xmax,
                 const FloatType& ymax,
                 const FloatType& zmax,
                 int32& numPoints,
                 int32& numBins,
                 std::ostringstream& nodes,
                 std::ostringstream& cells)
{
  nodes << xmin << " " << ymin << " " << zmin << std::endl;
  nodes << xmax << " " << ymin << " " << zmin << std::endl;
  nodes << xmax << " " << ymax << " " << zmin << std::endl;
  nodes << xmin << " " << ymax << " " << zmin << std::endl;

  nodes << xmin << " " << ymin << " " << zmax << std::endl;
  nodes << xmax << " " << ymin << " " << zmax << std::endl;
  nodes << xmax << " " << ymax << " " << zmax << std::endl;
  nodes << xmin << " " << ymax << " " << zmax << std::endl;

  constexpr int32 NUM_NODES = 8;
  int32 offset = numPoints;
  cells << NUM_NODES << " ";
  for(int32 i = 0; i < NUM_NODES; ++i)
  {
    cells << (offset + i) << " ";
  }
  cells << "\n";

  numBins += 1;
  numPoints += NUM_NODES;
}

//------------------------------------------------------------------------------
template <typename FloatType, int NDIMS>
void write_box(const primal::BoundingBox<FloatType, NDIMS>& box,
               int32& numPoints,
               int32& numBins,
               std::ostringstream& nodes,
               std::ostringstream& cells)
{
  const FloatType& xmin = box.getMin()[0];
  const FloatType& ymin = box.getMin()[1];

  const FloatType& xmax = box.getMax()[0];
  const FloatType& ymax = box.getMax()[1];

  if(NDIMS == 2)
  {
    write_box2d(xmin, ymin, xmax, ymax, numPoints, numBins, nodes, cells);
  }
  else
  {
    const FloatType& zmin = box.getMin()[2];
    const FloatType& zmax = box.getMax()[2];

    write_box3d(xmin, ymin, zmin, xmax, ymax, zmax, numPoints, numBins, nodes, cells);
  }
}

//------------------------------------------------------------------------------
template <typename FloatType>
void write_root(const primal::BoundingBox<FloatType, 2>& root,
                int32& numPoints,
                int32& numBins,
                std::ostringstream& nodes,
                std::ostringstream& cells,
                std::ostringstream& levels)
{
  write_box2d(root.getMin()[0],
              root.getMin()[1],
              root.getMax()[0],
              root.getMax()[1],
              numPoints,
              numBins,
              nodes,
              cells);
  levels << "0\n";
}

//------------------------------------------------------------------------------
template <typename FloatType>
void write_root(const primal::BoundingBox<FloatType, 3>& root,
                int32& numPoints,
                int32& numBins,
                std::ostringstream& nodes,
                std::ostringstream& cells,
                std::ostringstream& levels)
{
  write_box3d(root.getMin()[0],
              root.getMin()[1],
              root.getMin()[2],
              root.getMax()[0],
              root.getMax()[1],
              root.getMax()[2],
              numPoints,
              numBins,
              nodes,
              cells);
  levels << "0\n";
}

//------------------------------------------------------------------------------
template <typename FloatType, int NDIMS>
void write_recursive(const primal::BoundingBox<FloatType, NDIMS>* inner_nodes,
                     const int32* inner_node_children,
                     int32 current_node,
                     int32 level,
                     int32& numPoints,
                     int32& numBins,
                     std::ostringstream& nodes,
                     std::ostringstream& cells,
                     std::ostringstream& levels)
{
  // STEP 0: get the flat BVH bounding boxes
  primal::BoundingBox<FloatType, NDIMS> l_box = inner_nodes[current_node + 0];
  primal::BoundingBox<FloatType, NDIMS> r_box = inner_nodes[current_node + 1];

  // STEP 1: extract children information
  int32 l_child = inner_node_children[current_node + 0];
  int32 r_child = inner_node_children[current_node + 1];

  write_box<FloatType, NDIMS>(l_box, numPoints, numBins, nodes, cells);
  levels << level << std::endl;
  write_box<FloatType, NDIMS>(r_box, numPoints, numBins, nodes, cells);
  levels << level << std::endl;

  // STEP 2: check left
  if(l_child > -1)
  {
    write_recursive<FloatType, NDIMS>(inner_nodes,
                                      inner_node_children,
                                      l_child,
                                      level + 1,
                                      numPoints,
                                      numBins,
                                      nodes,
                                      cells,
                                      levels);
  }

  // STEP 3: check right
  if(r_child > -1)
  {
    write_recursive<FloatType, NDIMS>(inner_nodes,
                                      inner_node_children,
                                      r_child,
                                      level + 1,
                                      numPoints,
                                      numBins,
                                      nodes,
                                      cells,
                                      levels);
  }
}

} /* namespace linear_bvh */
} /* namespace internal */
} /* namespace spin */
} /* namespace axom */

#endif /* AXOM_SPIN_LINEAR_BVH_VTKIO_HPP_ */
