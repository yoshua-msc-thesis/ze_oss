// Copyright (C) ETH Zurich, Wyss Zurich, Zurich Eye - All Rights Reserved
// Unauthorized copying of this file, via any medium is strictly prohibited
// Proprietary and confidential

#include <cmath>

#include <ze/common/test_entrypoint.hpp>
#include <ze/common/test_manifold.hpp>
#include <ze/common/transformation.hpp>



TEST(TransformationTests, testSetRandom)
{
  ze::Transformation T;
  T.setRandom();
  ze::Matrix3 R = T.getRotation().getRotationMatrix();

  // Check if orthonormal
  EXPECT_TRUE(EIGEN_MATRIX_NEAR(R*R.transpose(), ze::I_3x3, 1e-6));
}

TEST(TransformationTests, testExpLog)
{
  for(int i = 0; i < 10; ++i)
  {
    ze::Transformation T1;
    T1.setRandom();
    ze::Transformation::Vector6 v = T1.log();
    ze::Transformation T2 = ze::Transformation::exp(v);
    ze::Matrix4 TT1 = T1.getTransformationMatrix();
    ze::Matrix4 TT2 = T2.getTransformationMatrix();
    for(int r = 0; r < 4; ++r)
    {
      for(int c = 0; c < 4; ++c)
      {
        EXPECT_NEAR(TT1(r,c), TT2(r,c), 1e-6) << "Failed at (" << r << "," << c << ")";
      }
    }
  }
}

TEST(TransformationTests, quaternionMatrices)
{
  Eigen::Quaterniond q_AB, q_BC, q_AC_plus, q_AC_oplus, q_AC_quatmult;
  q_AB.coeffs() = Eigen::Vector4d::Random().normalized();
  q_BC.coeffs() = Eigen::Vector4d::Random().normalized();
  q_AC_quatmult = q_AB * q_BC;
  q_AC_plus.coeffs() = ze::quaternionPlusMatrix(q_AB) * q_BC.coeffs();
  q_AC_oplus.coeffs() = ze::quaternionOplusMatrix(q_BC) * q_AB.coeffs();

  Eigen::Vector3d v_C = Eigen::Vector3d::Random();
  EXPECT_TRUE(EIGEN_MATRIX_EQUAL_DOUBLE(q_AC_quatmult * v_C, q_AC_plus * v_C));
  EXPECT_TRUE(EIGEN_MATRIX_EQUAL_DOUBLE(q_AC_quatmult * v_C, q_AC_oplus * v_C));
  EXPECT_TRUE(EIGEN_MATRIX_EQUAL_DOUBLE(
                (q_AC_quatmult.inverse() * q_AC_plus).coeffs(),
                Eigen::Quaterniond::Identity().coeffs()));
  EXPECT_TRUE(EIGEN_MATRIX_EQUAL_DOUBLE(
                (q_AC_quatmult.inverse() * q_AC_oplus).coeffs(),
                Eigen::Quaterniond::Identity().coeffs()));
}

ZE_UNITTEST_ENTRYPOINT