#pragma once

#include "LifeAPI.hpp"
#include "LifeTarget.hpp"
#include "LifeStable.hpp"
#include "LifeHistory.hpp"
#include "Parsing.hpp"

#include <iostream>

// LifeWeld is a cheap replacement for LifeStable, intended for
// working with just the active part of a catalyst with as little of
// the stator as possible. LifeWeld stores a "frozen" count, which is
// added to the true neighbour counts when stepping.

// Only non-active cells should have a non-zero frozen value.

struct LifeWeld {
  LifeState state;

  LifeState frozen2, frozen1, frozen0;

  LifeWeld() : state{}, frozen2{}, frozen1{}, frozen0{} {};
  LifeWeld(const LifeState &state)
      : state{state}, frozen2{}, frozen1{}, frozen0{} {};
  LifeWeld(const LifeState &state, const LifeState &frozen2,
           const LifeState &frozen1, const LifeState &frozen0)
      : state{state}, frozen2{frozen2}, frozen1{frozen1}, frozen0{frozen0} {};

  bool operator==(const LifeWeld &b) const = default;

  static LifeWeld FromRequired(const LifeState &state,
                               const LifeState &required);

  static LifeWeld FromSoup(const LifeState &state, const LifeState &soup, unsigned maxtime = 200, unsigned stabletime = 4);


  LifeTarget ToTarget() const; // What to match to know if it's recovered

  LifeState AllFrozen() const { return frozen2 | frozen1 | frozen0; }
  
  LifeWeld operator|(const LifeWeld &other) const {
    return {state | other.state, frozen2 | other.frozen2,
            frozen1 | other.frozen1, frozen0 | other.frozen0};
  }

  LifeWeld &operator|=(const LifeWeld &other) {
    state |= other.state;
    frozen2 |= other.frozen2;
    frozen1 |= other.frozen1;
    frozen0 |= other.frozen0;
    return *this;
  }


  inline bool Contains(const LifeTarget &target) const {
    return state.Contains(target);
  }

  // mvrnote: good enough?
  uint64_t GetHash() const {
    return 3 * frozen2.GetHash() + (3 * frozen1.GetHash() + (3 * frozen0.GetHash() + state.GetHash()));
  }

  LifeWeld Moved(std::pair<int, int> vec) const {
    return {state.Moved(vec), frozen2.Moved(vec), frozen1.Moved(vec),
            frozen0.Moved(vec)};
  }

  LifeWeld Transformed(SymmetryTransform t) const {
    return {state.Transformed(t), frozen2.Transformed(t),
            frozen1.Transformed(t), frozen0.Transformed(t)};
  }

  std::string BellmanRLE(const LifeState &active) const;

  std::string BellmanRLEWHeader(const LifeState &active) const {
    return "x = 0, y = 0, rule = LifeBellman\n" + BellmanRLE(active);
  }

  std::string BellmanRLE() const { return BellmanRLE(LifeState()); }
  std::string BellmanRLEWHeader() const { return BellmanRLEWHeader(LifeState()); }
  
  friend std::ostream& operator<<(std::ostream& os, LifeWeld const& self) {
    return os << self.BellmanRLEWHeader();
  }

  void Step();
  void Step(unsigned numIters) {
    for (unsigned i = 0; i < numIters; i++) {
      Step();
    }
  }
  LifeWeld Stepped() const {
    LifeWeld copy = *this;
    copy.Step();
    return copy;
  }
  LifeWeld Stepped(unsigned numIters) const {
    LifeWeld copy = *this;
    copy.Step(numIters);
    return copy;
  }

  inline void InteractionCounts(LifeState &__restrict__ out1,
                                LifeState &__restrict__ out2,
                                LifeState &__restrict__ outMore) const;
  LifeState InteractionOffsets(const LifeWeld &other) const;

  LifeStable ToStable() const;
  LifeStable ToStable(const LifeState &active, unsigned duration, const LifeState &mask = ~LifeState()) const;

  // Very expensive!
  LifeState UnweldableMask(const LifeWeld &other, const LifeState &startingGood, const LifeState &startingBad) const;
  LifeState UnweldableMask(const LifeWeld &other) const;

  // For debugging
  LifeHistory ToHistory() const;
};

std::string LifeWeld::BellmanRLE(const LifeState &active) const {
  LifeState frozen = frozen2 | frozen1 | frozen0;
  LifeState marked = (state & frozen).ZOI() & ~(state & ~frozen).ZOI();

  return GenericRLE([&](int x, int y) -> char {
    if (active.Get(x, y) && !state.Get(x, y)) return 'A';
    if (state.Get(x, y)) return 'C';
    if (marked.Get(x, y)) return 'E';
    return '.';
  });
}

LifeWeld LifeWeld::FromRequired(const LifeState &state,
                                const LifeState &required) {
  LifeState active = state.ZOI() & ~required;
  LifeState stator = state & ~active.ZOI(); // Cells to be deleted

  LifeWeld result;

  result.state = state & ~stator;
  
  // Determine which cells need to have neighbours added
  LifeState frozen = active.ZOI() & required;

  // Dumb way of determining unintended births:
  frozen |= result.state.Stepped() & ~result.state;

  // Count the missing neighbours
  LifeState bit3(InitializedTag::UNINITIALIZED); // Not possible in a stable pattern
  stator.CountNeighbourhood(bit3, result.frozen2, result.frozen1,
                            result.frozen0);
  result.frozen2 &= frozen;
  result.frozen1 &= frozen;
  result.frozen0 &= frozen;



  return result;
}

// LifeWeld LifeWeld::FromSoup(const LifeState &state, const LifeState &soup, unsigned maxtime, unsigned stabletime) {
//   LifeWeld result;

//   LifeState truth = state | soup;

//   return result;
// }

void LifeWeld::Step() {
  // No doubt this could be fused to be more efficient
  // mvrnote: bit3 irrelevant, should avoid computing it

  LifeState bit3(InitializedTag::UNINITIALIZED), bit2(InitializedTag::UNINITIALIZED), bit1(InitializedTag::UNINITIALIZED),
      bit0(InitializedTag::UNINITIALIZED);
  state.CountNeighbourhood(bit3, bit2, bit1, bit0);

  // Add the frozen counts to the actual counts:
  LifeState sum2(InitializedTag::UNINITIALIZED), sum1(InitializedTag::UNINITIALIZED), sum0(InitializedTag::UNINITIALIZED);
  LifeState carry2(InitializedTag::UNINITIALIZED), carry1(InitializedTag::UNINITIALIZED), carry0(InitializedTag::UNINITIALIZED);
  LifeState::HalfAdd(sum0, carry0, bit0, frozen0);
  LifeState::FullAdd(sum1, carry1, bit1, frozen1, carry0);
  LifeState::FullAdd(sum2, carry2, bit2, frozen2, carry1);

  // Now apply the ordinary life rule:
  state = (sum0 ^ sum2) & (sum1 ^ sum2) & (state | sum0);
}

LifeTarget LifeWeld::ToTarget() const {
  LifeState nonFrozen = state & ~frozen2 & ~frozen1 & ~frozen0;
  return LifeTarget(state, nonFrozen.ZOI() & ~state);
}

inline void LifeWeld::InteractionCounts(LifeState &__restrict__ out1,
                                        LifeState &__restrict__ out2,
                                        LifeState &__restrict__ outMore) const {

  state.InteractionCounts(out1, out2, outMore);

  // Delete the false counts near the boundary
  LifeState nonFrozenZOI = (state & ~frozen2 & ~frozen1 & ~frozen0).ZOI();
  out1 &= nonFrozenZOI;
  out2 &= nonFrozenZOI;
  outMore &= nonFrozenZOI;
}

LifeState LifeWeld::InteractionOffsets(const LifeWeld &b) const {
  const LifeWeld &a = *this;

  // Mostly copy-pasted from LifeState, but we have to ignore
  // interactions involving frozen cells

  const LifeState &a_state = a.state;
  const LifeState a_ignored =
      ~(a.state & ~a.frozen2 & ~a.frozen1 & ~a.frozen0).ZOI();

  LifeState a_bit3(InitializedTag::UNINITIALIZED), a_bit2(InitializedTag::UNINITIALIZED), a_bit1(InitializedTag::UNINITIALIZED),
      a_bit0(InitializedTag::UNINITIALIZED);
  a_state.CountNeighbourhood(a_bit3, a_bit2, a_bit1, a_bit0);
  LifeState a_out1 = ~a_bit3 & ~a_bit2 & ~a_bit1 & a_bit0;
  LifeState a_out2 = ~a_bit3 & ~a_bit2 & a_bit1 & ~a_bit0;
  LifeState a_out3 = ~a_bit3 & ~a_bit2 & a_bit1 & a_bit0;

  const LifeState b_state = b.state.Mirrored();
  const LifeState b_ignored =
      ~(b.state & ~b.frozen2 & ~b.frozen1 & ~b.frozen0).ZOI().Mirrored();

  LifeState b_bit3(InitializedTag::UNINITIALIZED), b_bit2(InitializedTag::UNINITIALIZED), b_bit1(InitializedTag::UNINITIALIZED),
      b_bit0(InitializedTag::UNINITIALIZED);
  b_state.CountNeighbourhood(b_bit3, b_bit2, b_bit1, b_bit0);
  LifeState b_out1 = ~b_bit3 & ~b_bit2 & ~b_bit1 & b_bit0;
  LifeState b_out2 = ~b_bit3 & ~b_bit2 & b_bit1 & ~b_bit0;
  LifeState b_out3 = ~b_bit3 & ~b_bit2 & b_bit1 & b_bit0;

  return
      // Overlaps
      a_state.Convolve(b_state) |
      // Positions that cause births
      (a_out1 & ~a_state & ~a_ignored).Convolve(b_out2 & ~b_state & ~a_ignored) |
      (b_out1 & ~b_state & ~b_ignored).Convolve(a_out2 & ~a_state & ~b_ignored) |
      // positions that cause overcrowding
      (a_out3 & a_state & ~a_ignored).Convolve((b_bit3 | b_bit2 | b_bit1) & ~b_state & ~b_ignored) |
      ((a_bit2 | a_bit3) & a_state & ~a_ignored).Convolve((b_bit3 | b_bit2 | b_bit1 | b_bit0) & ~b_state & ~b_ignored) |
      (b_out3 & b_state & ~b_ignored).Convolve((a_bit3 | a_bit2 | a_bit1) & ~a_state & ~a_ignored) |
      ((b_bit2 | b_bit3) & b_state & ~b_ignored).Convolve((a_bit3 | a_bit2 | a_bit1 | a_bit0) & ~a_state & ~a_ignored);
}

LifeState LifeWeld::UnweldableMask(const LifeWeld &other, const LifeState &startingGood, const LifeState &startingBad) const {
  LifeState knownGood = startingGood;
  LifeState knownBad = InteractionOffsets(other) | startingBad;

  // mvrnote: if we used full LifeStable rather than LifeWeld, we could possibly
  // get catalysts slightly closer.

  LifeState toTest = ~knownGood & ~knownBad;

  for (auto cell = toTest.FirstOn(); cell != std::make_pair(-1, -1);
       toTest.Erase(cell), cell = toTest.FirstOn()) {
    LifeWeld placed = *this | other.Moved(cell);

    // auto propagateResult = placed.ToStable().Propagate();
    // if (!propagateResult.consistent)
    //   knownBad.Set(cell);
    
    auto [placedResult, completedPlaced] = placed.ToStable().CompleteStable(0.05, false);
    switch (placedResult) {
    case LifeStable::CompletionResult::COMPLETED:
      break;
    case LifeStable::CompletionResult::INCONSISTENT:
      knownBad.Set(cell);
      break;
    case LifeStable::CompletionResult::TIMEOUT:
      break;
    }
  }

  return knownBad;
}

LifeStable LifeWeld::ToStable() const {
  LifeStable stable;
  stable.unknown = ~LifeState();

  NeighbourCount stateCounts(state);
  NeighbourCount sumCounts;

  // std::cout << LifeHistory(state, LifeState(), AllFrozen()) << std::endl;
  // std::cout << LifeHistory(state, LifeState(), frozen1) << std::endl;
  // std::cout << LifeHistory(state, LifeState(), frozen0) << std::endl;

  // LifeState bit3(InitializedTag::UNINITIALIZED), bit2(InitializedTag::UNINITIALIZED), bit1(InitializedTag::UNINITIALIZED),
  //     bit0(InitializedTag::UNINITIALIZED);
  // state.CountNeighbourhood(bit3, bit2, bit1, bit0);

  // Add the frozen counts to the actual counts:
  LifeState carry2(InitializedTag::UNINITIALIZED), carry1(InitializedTag::UNINITIALIZED), carry0(InitializedTag::UNINITIALIZED);
  LifeState::HalfAdd(sumCounts.bit0, carry0, stateCounts.bit0, frozen0);
  LifeState::FullAdd(sumCounts.bit1, carry1, stateCounts.bit1, frozen1, carry0);
  LifeState::FullAdd(sumCounts.bit2, carry2, stateCounts.bit2, frozen2, carry1);
  sumCounts.bit3 = stateCounts.bit3 ^ carry2;

  LifeState frozen = frozen2 | frozen1 | frozen0;
  LifeState nonFrozenZOI = (state & ~frozen).ZOI();

  stable.SetOn(state);
  stable.SetOff(~state & nonFrozenZOI);

  stable.RestrictOptions(frozen &  state & sumCounts.WithExactly(3), StableOptions::LIVE2); // Remember the sum includes the center square
  stable.RestrictOptions(frozen &  state & sumCounts.WithExactly(4), StableOptions::LIVE3);
  stable.RestrictOptions(frozen & ~state & sumCounts.WithExactly(1), StableOptions::DEAD1);
  stable.RestrictOptions(frozen & ~state & sumCounts.WithExactly(2), StableOptions::DEAD2);
  stable.RestrictOptions(frozen & ~state & sumCounts.WithExactly(4), StableOptions::DEAD4);
  stable.RestrictOptions(frozen & ~state & sumCounts.WithExactly(5), StableOptions::DEAD5);
  stable.RestrictOptions(frozen & ~state & sumCounts.WithExactly(6), StableOptions::DEAD6);

  // `FromRequired` is not accurate enough for this to work: in an eater only two cells are frozen
  // stable.RestrictOptions(nonFrozenZOI &  state & sumCounts.WithExactly(3), StableOptions::LIVE2); // Remember the sum includes the center square
  // stable.RestrictOptions(nonFrozenZOI &  state & sumCounts.WithExactly(4), StableOptions::LIVE3);
  // stable.RestrictOptions(nonFrozenZOI & ~state & sumCounts.WithExactly(1), StableOptions::DEAD1);
  // stable.RestrictOptions(nonFrozenZOI & ~state & sumCounts.WithExactly(2), StableOptions::DEAD2);
  // stable.RestrictOptions(nonFrozenZOI & ~state & sumCounts.WithExactly(4), StableOptions::DEAD4);
  // stable.RestrictOptions(nonFrozenZOI & ~state & sumCounts.WithExactly(5), StableOptions::DEAD5);
  // stable.RestrictOptions(nonFrozenZOI & ~state & sumCounts.WithExactly(6), StableOptions::DEAD6);

  return stable;
}

LifeStable LifeWeld::ToStable(const LifeState &active, unsigned duration, const LifeState &mask) const {
  LifeStable stable = ToStable();

  LifeState everActive;

  // Replay the reaction to determine region we can use
  
  LifeWeld current = *this;
  current.state |= active;
  
  for (unsigned i = 0; i < duration; i++) {
    everActive |= this->state ^ current.state;
    current.Step();
  }

  stable.SetOff(mask & ~state & everActive);
  
  // Replay the reaction to make sure correct births happen

  // mvrnote: it's a bit annoying we can't just use the history
  // information from the search, but there's not enough information
  // in there

  NeighbourCount stateCounts(state);

  current = *this;
  current.state |= active;

  for (unsigned i = 0; i < duration; i++) {
    LifeWeld next = current;
    next.Step();

    LifeState stayDead = ~state & ~current.state & ~next.state;
    LifeState getsBorn = ~state & ~current.state &  next.state;

    NeighbourCount currentCounts(current.state);

    // mvrnote: TODO; this sucks, should do it by NeighbourCount subtraction (clamping at 0)

    // LifeStable copy = stable;
    // auto result = copy.CompleteStable(1, false);
    // if(result.first == LifeStable::CompletionResult::INCONSISTENT) {
    // LifeState bad = ~state & stayDead & currentCounts.WithExactly(1) & stateCounts.WithExactly(0);
    //   std::cout << "Bad" << std::endl;
    //   std::cout << current << std::endl;
    //   std::cout << LifeHistory(state, LifeState(), bad) << std::endl;
    //   return stable;
    // }

    stable.RestrictOptions(mask & getsBorn & currentCounts.WithExactly(3) & stateCounts.WithExactly(0), StableOptions::DEAD0);
    stable.RestrictOptions(mask & getsBorn & currentCounts.WithExactly(3) & stateCounts.WithExactly(1), StableOptions::DEAD1);
    stable.RestrictOptions(mask & getsBorn & currentCounts.WithExactly(3) & stateCounts.WithExactly(2), StableOptions::DEAD2);

    stable.RestrictOptions(mask & stayDead & currentCounts.WithExactly(1) & stateCounts.WithExactly(0), ~StableOptions::DEAD2);
    stable.RestrictOptions(mask & stayDead & currentCounts.WithExactly(2) & stateCounts.WithExactly(0), ~StableOptions::DEAD1);
    stable.RestrictOptions(mask & stayDead & currentCounts.WithExactly(2) & stateCounts.WithExactly(1), ~StableOptions::DEAD2);

    stable.RestrictOptions(mask & stayDead & currentCounts.WithExactly(1) & stateCounts.WithExactly(2), ~StableOptions::DEAD4);
    stable.RestrictOptions(mask & stayDead & currentCounts.WithExactly(0) & stateCounts.WithExactly(2), ~StableOptions::DEAD5);

    stable.RestrictOptions(mask & stayDead & currentCounts.WithExactly(3) & stateCounts.WithExactly(4), ~StableOptions::DEAD4);
    stable.RestrictOptions(mask & stayDead & currentCounts.WithExactly(2) & stateCounts.WithExactly(4), ~StableOptions::DEAD5);
    stable.RestrictOptions(mask & stayDead & currentCounts.WithExactly(1) & stateCounts.WithExactly(4), ~StableOptions::DEAD6);

    stable.RestrictOptions(mask & stayDead & currentCounts.WithExactly(3) & stateCounts.WithExactly(5), ~StableOptions::DEAD5);
    stable.RestrictOptions(mask & stayDead & currentCounts.WithExactly(2) & stateCounts.WithExactly(5), ~StableOptions::DEAD6);

    stable.RestrictOptions(mask & stayDead & currentCounts.WithExactly(3) & stateCounts.WithExactly(6), ~StableOptions::DEAD6);

    current = next;
  }
   
  return stable;
}

LifeHistory LifeWeld::ToHistory() const {
  return LifeHistory(state, LifeState(), AllFrozen());
}
