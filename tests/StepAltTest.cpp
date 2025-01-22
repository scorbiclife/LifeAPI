#include <gtest/gtest.h>

#include "../LifeAPI.hpp"

TEST(StepAltTest, Random) {
  for (unsigned i = 0; i < 10000; i++) {
    LifeState random = LifeState::RandomState();
    LifeState randomCopy = random;
    random.Step();
    randomCopy.StepAlt();
    EXPECT_EQ(random, randomCopy);
  }
}
