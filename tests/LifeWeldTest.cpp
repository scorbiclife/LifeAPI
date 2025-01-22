#include <gtest/gtest.h>

#include "../LifeAPI.hpp"
#include "../LifeWeld.hpp"

TEST(LifeWeldTest, StableTest) {
  auto examples = {
    LifeState::ConstantParse("2b2o$bobo$bo$2o!"),
    LifeState::ConstantParse("2o$2o!"),
  };
  for (auto &e : examples) {
    LifeWeld weld = LifeWeld(e);
    LifeWeld copy = weld;
    copy.Step();
    EXPECT_EQ(weld, copy);
  }
}

TEST(LifeWeldTest, RequiredTest) {
  std::vector<std::pair<LifeState, LifeState>> examples = {
    {LifeState::ConstantParse("2b2o$bobo$bo$2o!"),
     LifeState::ConstantParse("2b2o$b3o$b4o$5o$4o$4o!").Moved(-1, -1)},
    {LifeState::ConstantParse("2o$o2bob2o$b3obobo$5bobo$b5ob3o$bo4bo3bo$4bobo2b2o$4b2o!"),
     LifeState::ConstantParse("4o$5o2bo$4o$5o4bo$b5ob5o$b12o$b12o$b12o$4b9o$4b4o!").Moved(-1, -1)},
    {LifeState::ConstantParse("4b2ob2o$3bobobobo$b3o3bobo$o4bobob3o$b3ob2obo3bo$3bo4bo2b2o$5b3o$4b2o!"),
     LifeState::ConstantParse("4b2o$3b2o2bo2b2o$b4o6bo$6obob5o$15o$15o$b14o$3b12o$4b6o$4b4o!").Moved(-1, -1)},
     };
  for (auto &[state, required] : examples) {
    LifeWeld weld = LifeWeld::FromRequired(state, required);
    LifeWeld copy = weld;
    copy.Step();
    EXPECT_EQ(weld, copy);
  }
}
