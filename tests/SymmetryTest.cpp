#include <gtest/gtest.h>

#include "../LifeAPI.hpp"
#include "../Parsing.hpp"
#include "../Symmetry.hpp"

void TestFundamentalDomain(StaticSymmetry s, std::pair<int, int> offset) {
  LifeState domain = FundamentalDomain(s);
  domain.Move(HalveOffset(s, offset));
  LifeState missing = ~Symmetricize(domain, s, offset);
  EXPECT_TRUE(missing.IsEmpty())
      << "Failed for " << SymmetryToString(s) << " offset (" << offset.first
      << ", " << offset.second << "): " << missing.RLE();
}

TEST(SymmetryTest, FundamentalDomainSymmetricizeOrigin) {
  using enum StaticSymmetry;
  for (auto s : {C1, C2, C4, D2AcrossX, D2AcrossY, D2diagodd, D2negdiagodd, D4, D4diag, }) {
    TestFundamentalDomain(s, {0, 0});
  }
}

TEST(SymmetryTest, FundamentalDomainSymmetricizeOffset) {
  using enum StaticSymmetry;
  for (auto s : {C1, C2, C4, D4, D4diag, }) {
    for (int i = 1; i < 10; i++) {
      for (int j = 1; j < 10; j++) {
        if(s == D4diag && (i + j) % 2 == 1)
          continue;

        TestFundamentalDomain(s, {i, j});
        TestFundamentalDomain(s, {64 - i, j});
        TestFundamentalDomain(s, {i, 64 - j});
        TestFundamentalDomain(s, {64 - i, 64 - j});
      }
    }
  }
  for (int i = 1; i < 10; i++)
    TestFundamentalDomain(D2AcrossX, {0, i});
  for (int i = 1; i < 10; i++)
    TestFundamentalDomain(D2AcrossY, {i, 0});
  for (int i = 1; i < 10; i++)
    TestFundamentalDomain(D2diagodd, {i, 64 - i});
  for (int i = 1; i < 10; i++)
    TestFundamentalDomain(D2negdiagodd, {i, i});
}

TEST(SymmetryTest, PerpComponentDiag) {
  for (int x = 0; x < 10; x++) {
    for (int y = 0; y < 10; y++) {
      if ((x + y) % 2 == 1)
        continue;

      std::pair<int, int> pt = {x, y};
      auto c1 = PerpComponent(SymmetryTransform::ReflectAcrossYeqX, pt);
      auto c2 = PerpComponent(SymmetryTransform::ReflectAcrossYeqNegXP1, pt);
      std::pair<int, int> recombined = {(c1.first + c2.first) % 64,
                                        (c1.second + c2.second) % 64};
      EXPECT_EQ(pt, recombined);
    }
  }
}

void TestIntersectingOffset(StaticSymmetry s, std::pair<int, int> offset) {
  for (int x = 0; x < 10; x++) {
    for (int y = 0; y < 10; y++) {
      std::pair<int, int> pt = {x, y};

      LifeState state = LifeState::Cell(pt);
      LifeState symmetricized = Symmetricize(state, s, offset);

      LifeState offsets = IntersectingOffsets(state, symmetricized, s);

      EXPECT_TRUE(offsets.Get(offset));
    }
  }
}

TEST(SymmetryTest, IntersectingOffsets) {
  using enum StaticSymmetry;
  for (auto s : {C2, C4, }) {
    for (int i = 0; i < 10; i++) {
      for (int j = 0; j < 10; j++) {
        TestIntersectingOffset(s, {i, j});
      }
    }
  }
  for (int i = 0; i < 10; i++)
    TestIntersectingOffset(D2AcrossX, {0, i});
  for (int i = 0; i < 10; i++)
    TestIntersectingOffset(D2AcrossY, {i, 0});
  for (int i = 1; i < 10; i++)
    TestIntersectingOffset(D2diagodd, {i, 64 - i});
  for (int i = 0; i < 10; i++)
    TestIntersectingOffset(D2negdiagodd, {i, i});
}
