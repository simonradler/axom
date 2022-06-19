// Copyright (c) 2017-2022, Lawrence Livermore National Security, LLC and
// other Axom Project Developers. See the top-level LICENSE file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)

#include <limits>
#include <algorithm>

#include "gtest/gtest.h"

#include "axom/primal/geometry/NumericArray.hpp"
#include "axom/primal/geometry/Point.hpp"
#include "axom/primal/geometry/Vector.hpp"
#include "axom/primal/geometry/OrientedBoundingBox.hpp"
#include "axom/primal/operators/closest_point.hpp"

namespace primal = axom::primal;

//------------------------------------------------------------------------------
TEST(primal_closest_point, obb_test_closest_point_interior)
{
  constexpr int DIM = 3;
  using CoordType = double;
  using QPoint = primal::Point<CoordType, DIM>;
  using QVector = primal::Vector<CoordType, DIM>;
  using QOBBox = primal::OrientedBoundingBox<CoordType, DIM>;

  constexpr double ONE_OVER_SQRT_TWO = 0.7071;
  QPoint pt1;      // origin
  QVector u[DIM];  // make axes
  u[0][0] = ONE_OVER_SQRT_TWO;
  u[0][1] = ONE_OVER_SQRT_TWO;
  u[1][0] = ONE_OVER_SQRT_TWO;
  u[1][1] = -ONE_OVER_SQRT_TWO;
  u[2][2] = 1.;

  QVector e = QVector(1.);

  QOBBox obbox1(pt1, u, e);

  // interior point
  EXPECT_TRUE(primal::closest_point(pt1, obbox1) == pt1);
}

//------------------------------------------------------------------------------
TEST(primal_closest_point, obb_test_closest_point_vertex)
{
  constexpr int DIM = 3;
  using CoordType = double;
  using QPoint = primal::Point<CoordType, DIM>;
  using QVector = primal::Vector<CoordType, DIM>;
  using QOBBox = primal::OrientedBoundingBox<CoordType, DIM>;

  constexpr double ONE_OVER_SQRT_TWO = 0.7071;
  QPoint pt1;      // origin
  QVector u[DIM];  // make axes
  u[0][0] = ONE_OVER_SQRT_TWO;
  u[0][1] = ONE_OVER_SQRT_TWO;
  u[1][0] = ONE_OVER_SQRT_TWO;
  u[1][1] = -ONE_OVER_SQRT_TWO;
  u[2][2] = 1.;

  QVector e = QVector(1.);
  QOBBox obbox1(pt1, u, e);
  std::vector<QPoint> verts = obbox1.vertices();

  QPoint pt2 = verts[0];
  QPoint pt3(10. * pt2.array());

  // closest point is a vertex
  EXPECT_TRUE(primal::closest_point(pt3, obbox1) == pt2);
}

//------------------------------------------------------------------------------
TEST(primal_closest_point, obb_test_closest_point_face)
{
  constexpr int DIM = 3;
  using CoordType = double;
  using QPoint = primal::Point<CoordType, DIM>;
  using QVector = primal::Vector<CoordType, DIM>;
  using QOBBox = primal::OrientedBoundingBox<CoordType, DIM>;

  constexpr double ONE_OVER_SQRT_TWO = 0.7071;
  constexpr double EPS = 0.01;
  QPoint pt1;      // origin
  QVector u[DIM];  // make standard axes
  u[0][0] = ONE_OVER_SQRT_TWO;
  u[0][1] = ONE_OVER_SQRT_TWO;
  u[1][0] = ONE_OVER_SQRT_TWO;
  u[1][1] = -ONE_OVER_SQRT_TWO;
  u[2][2] = 1.;

  QVector e = QVector(1.);
  QOBBox obbox1(pt1, u, e);

  QPoint pt2(u[0].array());
  QPoint pt3(10. * pt2.array());

  QVector found(primal::closest_point(pt3, obbox1));
  QVector expected(pt2);

  // closest point is on a face
  EXPECT_TRUE((found - expected).squared_norm() < EPS);
}

//------------------------------------------------------------------------------
TEST(primal_closest_point, obb_test_closest_point_edge)
{
  constexpr int DIM = 3;
  using CoordType = double;
  using QPoint = primal::Point<CoordType, DIM>;
  using QVector = primal::Vector<CoordType, DIM>;
  using QOBBox = primal::OrientedBoundingBox<CoordType, DIM>;

  constexpr double ONE_OVER_SQRT_TWO = 0.7071;
  constexpr double EPS = 0.01;
  QPoint pt1;      // origin
  QVector u[DIM];  // make standard axes
  u[0][0] = ONE_OVER_SQRT_TWO;
  u[0][1] = ONE_OVER_SQRT_TWO;
  u[1][0] = ONE_OVER_SQRT_TWO;
  u[1][1] = -ONE_OVER_SQRT_TWO;
  u[2][2] = 1.;

  QVector e = QVector(1.);
  QOBBox obbox1(pt1, u, e);

  QPoint pt2(u[0].array() + u[1].array());
  QPoint pt3(10. * pt2.array());

  QVector found(primal::closest_point(pt3, obbox1));
  QVector expected(pt2);

  // closest point is on an edge
  EXPECT_TRUE((found - expected).squared_norm() < EPS);
}
