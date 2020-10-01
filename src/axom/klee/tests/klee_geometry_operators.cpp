// Copyright (c) 2017-2020, Lawrence Livermore National Security, LLC and
// other Axom Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: (BSD-3-Clause)

#include "axom/klee/GeometryOperators.hpp"

#include "axom/primal/geometry/Vector.hpp"

#include <array>
#include <cmath>
#include <stdexcept>
#include <utility>

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "axom/klee/tests/KleeMatchers.hpp"
#include "axom/klee/tests/KleeTestUtils.hpp"

namespace axom { namespace klee { namespace {

using test::affine;
using test::makePoint;
using test::makeVector;
using test::AlmostEqMatrix;
using test::AlmostEqPoint;
using test::AlmostEqVector;
using test::MockOperator;

using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::Matcher;
using ::testing::Ref;
using ::testing::Return;

using primal::Vector3D;
using primal::Point3D;

template<typename ColumnVector>
ColumnVector operator*(const numerics::Matrix<double> & matrix,
        const ColumnVector &rhs) {
    if (matrix.getNumRows() != matrix.getNumRows() ||
        matrix.getNumRows() != rhs.dimension()) {
        throw std::logic_error("Can't multiply entities of this size");
    }
    ColumnVector result;
    matrix_vector_multiply(matrix, rhs.data(), result.data());
    return result;
}

primal::Vector<double, 4> affineVec(const std::array<double, 3> &values) {
    primal::Vector<double, 4> vector{values.data(), 3};
    vector[3] = 0;
    return vector;
}

primal::Vector<double, 4> affineVec(const Vector3D &vec3d) {
    primal::Vector<double, 4> vector{vec3d.data(), 3};
    vector[3] = 0;
    return vector;
}

primal::Point<double, 4> affinePoint(const std::array<double, 3> &values) {
    primal::Point<double, 4> point{values.data(), 3};
    point[3] = 1;
    return point;
}

primal::Point<double, 4> affinePoint(const Point3D &point3d) {
    primal::Point<double, 4> point{point3d.data(), 3};
    point[3] = 1;
    return point;
}

Dimensions ALL_DIMS[] = {Dimensions::Two, Dimensions::Three};

class MockVisitor : public GeometryOperatorVisitor {
public:
    MOCK_METHOD(void, visit, (const Translation &translation), (override));
    MOCK_METHOD(void, visit, (const Rotation &rotation), (override));
    MOCK_METHOD(void, visit, (const Scale &scale), (override));
    MOCK_METHOD(void, visit, (const ArbitraryMatrixOperator &op), (override));
    MOCK_METHOD(void, visit, (const CompositeOperator &op), (override));
    MOCK_METHOD(void, visit, (const SliceOperator &op), (override));
};

TEST(Tanslation, basics) {
    for (Dimensions dims : ALL_DIMS) {
        Vector3D offset = makeVector({10, 20, 30});
        Translation translation{offset, dims};
        EXPECT_EQ(dims, translation.startDims());
        EXPECT_EQ(dims, translation.endDims());
        EXPECT_THAT(translation.getOffset(), AlmostEqVector(offset));
    }
}

TEST(Tanslation, toMatrix) {
    for (Dimensions dims : ALL_DIMS) {
        Vector3D offset = makeVector({10, 20, 30});
        Translation translation{offset, dims};
        EXPECT_THAT(translation.toMatrix(), AlmostEqMatrix(affine({
            1, 0, 0, 10,
            0, 1, 0, 20,
            0, 0, 1, 30
        })));
    }
}

TEST(Tanslation, accept) {
    Translation translation{makeVector({10, 20, 30}), Dimensions::Three};
    MockVisitor visitor;
    EXPECT_CALL(visitor, visit(Matcher<const Translation &>(Ref(translation))));
    translation.accept(visitor);
}

TEST(Rotation, basics) {
    for (Dimensions dims : ALL_DIMS) {
        Vector3D axis = makeVector({10, 20, 30});
        Point3D center = makePoint({40, 50, 60});
        double angle = 45;
        Rotation rotation{angle, center, axis, dims};
        EXPECT_EQ(dims, rotation.startDims());
        EXPECT_EQ(dims, rotation.endDims());
        EXPECT_DOUBLE_EQ(angle, rotation.getAngle());
        EXPECT_THAT(rotation.getCenter(), AlmostEqPoint(center));
        EXPECT_THAT(rotation.getAxis(), AlmostEqVector(axis));
    }
}

TEST(Rotation, rotate2d_with_center){
    Vector3D axis = makeVector({0, 0, 1});
    Point3D center = makePoint({10, 20, 0});
    double angle = 30;
    Rotation rotation{angle, center, axis, Dimensions::Two};

    double sin30 = 0.5;
    double cos30 = std::sqrt(3) / 2;

    double xOffset = 10 - 10 * cos30 + 20 * sin30;
    double yOffset = 20 - 10 * sin30 - 20 * cos30;

    EXPECT_THAT(rotation.toMatrix(), AlmostEqMatrix(affine({
            cos30, -sin30, 0, xOffset,
            sin30, cos30, 0, yOffset,
            0, 0, 1, 0})));

    // Sanity check to make sure things work with nice rotation since
    // the test and implementation use similar approaches (though the equations
    // were verified by hand).
    Rotation rotate90{90, makePoint({10, 20, 0}), makeVector({0, 0, 1}),
                      Dimensions::Two};

    EXPECT_THAT(rotate90.toMatrix() * affinePoint({15, 20, 0}),
            AlmostEqPoint(affinePoint({10, 25, 0})));
}

TEST(Rotation, rotate3d_axis_aligned){
    double sin30 = 0.5;
    double cos30 = std::sqrt(3) / 2;

    Point3D origin = makePoint({0, 0, 0});

    Rotation rotatedAboutXAxis{30, origin, makeVector({1, 0, 0}),
                               Dimensions::Three};

    EXPECT_THAT(rotatedAboutXAxis.toMatrix(), AlmostEqMatrix(affine({
            1, 0, 0, 0,
            0, cos30, -sin30, 0,
            0, sin30, cos30, 0})));

    Rotation rotatedAboutYAxis{30, origin, makeVector({0, 1, 0}),
                               Dimensions::Three};
    EXPECT_THAT(rotatedAboutYAxis.toMatrix(), AlmostEqMatrix(affine({
            cos30, 0, sin30, 0,
            0, 1, 0, 0,
            -sin30, 0, cos30, 0})));

    Rotation rotatedAboutZAxis{30, origin, makeVector({0, 0, 1}),
                               Dimensions::Three};
    EXPECT_THAT(rotatedAboutZAxis.toMatrix(), AlmostEqMatrix(affine({
            cos30, -sin30, 0, 0,
            sin30, cos30, 0, 0,
            0, 0, 1, 0})));
}

TEST(Rotation, rotate3d_with_center){
    Rotation rotation{90, makePoint({10, 20, 30}), makeVector({1, 1, 0}),
                      Dimensions::Three};

    double halfRoot2 = std::sqrt(2) / 2;
    numerics::Matrix<double> expected = affine({
            0.5, 0.5, halfRoot2, 0,
            0.5, 0.5, -halfRoot2, 0,
            -halfRoot2, halfRoot2, 0, 0,
    });
    expected(0, 3) = 10 - 10 * expected(0, 0) - 20 * expected(0, 1) - 30 * expected(0, 2);
    expected(1, 3) = 20 - 10 * expected(1, 0) - 20 * expected(1, 1) - 30 * expected(1, 2);
    expected(2, 3) = 30 - 10 * expected(2, 0) - 20 * expected(2, 1) - 30 * expected(2, 2);

    EXPECT_THAT(rotation.toMatrix(), AlmostEqMatrix(expected));

    EXPECT_THAT(rotation.toMatrix() * affinePoint({11, 20, 30}),
            AlmostEqPoint(affinePoint({10.5, 20.5, 30 -1 / std::sqrt(2)})));

    // Use the pythagorean quadruple (1, 4, 8, 9) for easier manual checking
    rotation = Rotation{90, makePoint({10, 20, 30}), makeVector({1, -4, 8}),
                        Dimensions::Three};

    expected = affine({
            1, -76, -28, 0,
            68, 16, -41, 0,
            44, -23, 64, 0,
    });
    matrix_scalar_multiply(expected, 1 / 81.0);
    expected(3, 3) = 1;
    expected(0, 3) = 10 - 10 * expected(0, 0) - 20 * expected(0, 1) - 30 * expected(0, 2);
    expected(1, 3) = 20 - 10 * expected(1, 0) - 20 * expected(1, 1) - 30 * expected(1, 2);
    expected(2, 3) = 30 - 10 * expected(2, 0) - 20 * expected(2, 1) - 30 * expected(2, 2);
    EXPECT_THAT(rotation.toMatrix(), AlmostEqMatrix(expected));
}

TEST(Rotation, accept) {
    Rotation rotation{90, makePoint({0, 0, 0}), makeVector({1, 2, 3}),
                      Dimensions::Three};
    MockVisitor visitor;
    EXPECT_CALL(visitor, visit(Matcher<const Rotation &>(Ref(rotation))));
    rotation.accept(visitor);
}

TEST(Scale, basics) {
    for (Dimensions dims : ALL_DIMS) {
        Scale scale{2, 3, 4, dims};
        EXPECT_EQ(dims, scale.startDims());
        EXPECT_EQ(dims, scale.endDims());
        EXPECT_DOUBLE_EQ(2, scale.getXFactor());
        EXPECT_DOUBLE_EQ(3, scale.getYFactor());
        EXPECT_DOUBLE_EQ(4, scale.getZFactor());
    }
}

TEST(Scale, toMatrix) {
    for (Dimensions dims : ALL_DIMS) {
        Scale scale{2, 3, 4, dims};
        EXPECT_THAT(scale.toMatrix(), AlmostEqMatrix(affine({
            2, 0, 0, 0,
            0, 3, 0, 0,
            0, 0, 4, 0
        })));
    }
}

TEST(Scale, accept) {
    Scale scale{1, 2, 3, Dimensions::Three};
    MockVisitor visitor;
    EXPECT_CALL(visitor, visit(Matcher<const Scale &>(Ref(scale))));
    scale.accept(visitor);
}

TEST(ArbitraryMatrixOperator, basics) {
    for (Dimensions dims : ALL_DIMS) {
        auto transformation = affine({
            1, -2, 3, -4,
            -5, 6, -7, 8,
            9, -10, 11, -12
        });
        ArbitraryMatrixOperator op{transformation, dims};
        EXPECT_EQ(dims, op.startDims());
        EXPECT_EQ(dims, op.endDims());
        EXPECT_THAT(op.toMatrix(), AlmostEqMatrix(transformation));
    }
}

TEST(ArbitraryMatrixOperator, accept) {
    ArbitraryMatrixOperator op{numerics::Matrix<double>::identity(4),
                               Dimensions::Three};
    MockVisitor visitor;
    EXPECT_CALL(visitor, visit(Matcher<const ArbitraryMatrixOperator &>(Ref(op))));
    op.accept(visitor);
}

TEST(CompositeOperator, empty) {
    CompositeOperator op;
    EXPECT_THROW(op.startDims(), std::logic_error);
    EXPECT_THROW(op.endDims(), std::logic_error);
    EXPECT_THAT(op.getOperators(), IsEmpty());
}

TEST(CompositeOperator, addOperator) {
    CompositeOperator compositeOp;

    auto startMock = std::make_shared<MockOperator>();
    ON_CALL(*startMock, startDims()).WillByDefault(Return(Dimensions::Two));
    ON_CALL(*startMock, endDims()).WillByDefault(Return(Dimensions::Three));
    compositeOp.addOperator(startMock);
    EXPECT_CALL(*startMock, startDims());
    EXPECT_EQ(Dimensions::Two, compositeOp.startDims());
    EXPECT_CALL(*startMock, endDims());
    EXPECT_EQ(Dimensions::Three, compositeOp.endDims());
    EXPECT_THAT(compositeOp.getOperators(), ElementsAre(startMock));

    auto midMock = std::make_shared<MockOperator>();
    ON_CALL(*midMock, startDims()).WillByDefault(Return(Dimensions::Three));
    ON_CALL(*midMock, endDims()).WillByDefault(Return(Dimensions::Three));
    compositeOp.addOperator(midMock);
    EXPECT_CALL(*startMock, startDims());
    EXPECT_EQ(Dimensions::Two, compositeOp.startDims());
    EXPECT_CALL(*midMock, endDims());
    EXPECT_EQ(Dimensions::Three, compositeOp.endDims());
    EXPECT_THAT(compositeOp.getOperators(), ElementsAre(startMock, midMock));

    auto endMock = std::make_shared<MockOperator>();
    ON_CALL(*endMock, startDims()).WillByDefault(Return(Dimensions::Three));
    ON_CALL(*endMock, endDims()).WillByDefault(Return(Dimensions::Two));
    compositeOp.addOperator(endMock);
    EXPECT_CALL(*startMock, startDims());
    EXPECT_EQ(Dimensions::Two, compositeOp.startDims());
    EXPECT_CALL(*endMock, endDims());
    EXPECT_EQ(Dimensions::Two, compositeOp.endDims());
    EXPECT_THAT(compositeOp.getOperators(),
            ElementsAre(startMock, midMock, endMock));
}

TEST(CompositeOperator, accept) {
    CompositeOperator composite;
    MockVisitor visitor;
    EXPECT_CALL(visitor,
            visit(Matcher<const CompositeOperator &>(Ref(composite))));
    composite.accept(visitor);
}

TEST(Slice, basics) {
    Point3D origin = makePoint({1, 2, 3});
    Vector3D normal = makeVector({4, 5, 6});
    Vector3D up = makeVector({-5, 4, 0});
    SliceOperator slice{origin, normal, up};
    EXPECT_THAT(slice.getOrigin(), AlmostEqPoint(origin));
    EXPECT_THAT(slice.getNormal(), AlmostEqVector(normal));
    EXPECT_THAT(slice.getUp(), AlmostEqVector(up));
    EXPECT_EQ(Dimensions::Three, slice.startDims());
    EXPECT_EQ(Dimensions::Two, slice.endDims());
}

TEST(Slice, toMatrix_translateOnly) {
    Point3D origin = makePoint({10, 20, 30});
    Vector3D normal = makeVector({0, 0, 1});
    Vector3D up = makeVector({0, 1, 0});
    SliceOperator slice{origin, normal, up};
    EXPECT_THAT(slice.toMatrix(), AlmostEqMatrix(affine({
        1, 0, 0, -10,
        0, 1, 0, -20,
        0, 0, 1, -30
    })));
}

TEST(Slice, toMatrix_vectorScaleIgnored) {
    Point3D origin = makePoint({10, 20, 30});
    Vector3D normal = makeVector({0, 0, 100});
    Vector3D up = makeVector({0, 200, 0});
    SliceOperator slice{origin, normal, up};
    EXPECT_THAT(slice.toMatrix(), AlmostEqMatrix(affine({
        1, 0, 0, -10,
        0, 1, 0, -20,
        0, 0, 1, -30
    })));
}

TEST(Slice, toMatrix_general) {
    Point3D origin = makePoint({10, 20, 30});
    Vector3D normal = makeVector({1, 4, 8});
    Vector3D up = makeVector({-4, 1, 0});
    SliceOperator slice{origin, normal, up};

    auto sliceMatrix = slice.toMatrix();

    // Expected matrix calculated with Mathematica
    double root17 = std::sqrt(17);
    EXPECT_THAT(sliceMatrix, AlmostEqMatrix(affine({
         8 / (9 * root17), 32 / (9 * root17), -root17 / 9, -70 / (3 * root17),
         -4 / root17, 1 / root17, 0, 20 / root17,
         1 / 9.0, 4 / 9.0, 8 / 9.0, -110 / 3.0
    })));

    // Extra checks to make sure we input things right both here and in
    // Mathematica. Verify expected operations on vectors and points.
    EXPECT_THAT(sliceMatrix * affinePoint(origin),
            AlmostEqPoint(affinePoint(primal::Point3D{0.0})));
    EXPECT_THAT(sliceMatrix * affineVec(normal.unitVector()),
            AlmostEqVector(affineVec({0, 0, 1})));
    EXPECT_THAT(sliceMatrix * affineVec(up.unitVector()),
            AlmostEqVector(affineVec({0, 1, 0})));
}

TEST(Slice, accept) {
    SliceOperator slice{makePoint({0, 0, 0}), makeVector({1, 0, 0}),
                 makeVector({0, 1, 0})};
    MockVisitor visitor;
    EXPECT_CALL(visitor,
            visit(Matcher<const SliceOperator &>(Ref(slice))));
    slice.accept(visitor);
}

}}}
