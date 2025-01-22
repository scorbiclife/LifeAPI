#pragma once

#include "LifeAPI.hpp"
#include "NeighbourCount.hpp"
#include "Parsing.hpp"

enum class StableOptions : unsigned char {
  LIVE2 = 1 << 0,
  LIVE3 = 1 << 1,
  DEAD0 = 1 << 2,
  DEAD1 = 1 << 3,
  DEAD2 = 1 << 4,
  DEAD4 = 1 << 5,
  DEAD5 = 1 << 6,
  DEAD6 = 1 << 7,

  IMPOSSIBLE = 0,
  LIVE = LIVE2 | LIVE3,
  DEAD = DEAD0 | DEAD1 | DEAD2 | DEAD4 | DEAD5 | DEAD6,
};

StableOptions StableOptionsHighest(StableOptions t) {
  if (t == StableOptions::IMPOSSIBLE) return StableOptions::IMPOSSIBLE;

  unsigned char bits = static_cast<unsigned char>(t);
  return static_cast<StableOptions>(1 << (8*sizeof(bits)-1-__builtin_clz(bits)));
}

constexpr inline StableOptions operator~ (StableOptions a) { return static_cast<StableOptions>( ~static_cast<std::underlying_type<StableOptions>::type>(a) ); }
constexpr inline StableOptions operator| (StableOptions a, StableOptions b) { return static_cast<StableOptions>( static_cast<std::underlying_type<StableOptions>::type>(a) | static_cast<std::underlying_type<StableOptions>::type>(b) ); }
constexpr inline StableOptions operator& (StableOptions a, StableOptions b) { return static_cast<StableOptions>( static_cast<std::underlying_type<StableOptions>::type>(a) & static_cast<std::underlying_type<StableOptions>::type>(b) ); }
constexpr inline StableOptions operator^ (StableOptions a, StableOptions b) { return static_cast<StableOptions>( static_cast<std::underlying_type<StableOptions>::type>(a) ^ static_cast<std::underlying_type<StableOptions>::type>(b) ); }
constexpr inline StableOptions& operator|= (StableOptions& a, StableOptions b) { a = a | b; return a; }
constexpr inline StableOptions& operator&= (StableOptions& a, StableOptions b) { a = a & b; return a; }
constexpr inline StableOptions& operator^= (StableOptions& a, StableOptions b) { a = a ^ b; return a; }



class LifeStable {
public:
  LifeState state;
  LifeState unknown;

  // IMPORTANT: These are stored flipped to the meaning of StableOptions, so
  // 0 is possible, 1 is ruled out
  LifeState live2;
  LifeState live3;
  LifeState dead0;
  LifeState dead1;
  LifeState dead2;
  LifeState dead4;
  LifeState dead5;
  LifeState dead6;

  bool operator==(const LifeStable&) const = default;

  void Move(int x, int y) {
    state.Move(x, y);
    unknown.Move(x, y);
    live2.Move(x, y);
    live3.Move(x, y);
    dead0.Move(x, y);
    dead1.Move(x, y);
    dead2.Move(x, y);
    dead4.Move(x, y);
    dead5.Move(x, y);
    dead6.Move(x, y);
  }

  void Move(std::pair<int, int> vec) {
    Move(vec.first, vec.second);
  }

  LifeStable Moved(std::pair<int, int> vec) const {
    return {state.Moved(vec), unknown.Moved(vec),

            live2.Moved(vec), live3.Moved(vec),

            dead0.Moved(vec), dead1.Moved(vec),   dead2.Moved(vec),
            dead4.Moved(vec), dead5.Moved(vec),   dead6.Moved(vec)};
  }


  LifeStable Join(const LifeStable &other) const;
  LifeStable Graft(const LifeStable &other) const;
  LifeStable ClearUnmodified() const;
  LifeState Differences(const LifeStable &other) const;

  StableOptions GetOptions(std::pair<int, int> cell) const;
  void RestrictOptions(std::pair<int, int> cell, StableOptions options);
  void RestrictOptions(LifeState cells, StableOptions options);

  bool SingletonOptions(std::pair<int, int> cell) {
    unsigned char bits = static_cast<unsigned char>(GetOptions(cell));
    return bits && !(bits & (bits-1));
  }
  
  void SetOn(const LifeState &state);
  void SetOff(const LifeState &state);
  void SetOn(unsigned column, uint64_t which);
  void SetOff(unsigned column, uint64_t which);
  void SetOn(std::pair<int, int> cell);
  void SetOff(std::pair<int, int> cell);

  LifeStable operator|(const LifeStable &other) const {
    return {state | other.state, unknown & other.unknown,

            live2 | other.live2, live3 | other.live3,

            dead0 | other.dead0, dead1 | other.dead1,     dead2 | other.dead2,
            dead4 | other.dead4, dead5 | other.dead5,     dead6 | other.dead6};
  }

  LifeStable Transformed(SymmetryTransform t) const {
    return {state.Transformed(t), unknown.Transformed(t),

            live2.Transformed(t), live3.Transformed(t),

            dead0.Transformed(t), dead1.Transformed(t),   dead2.Transformed(t),
            dead4.Transformed(t), dead5.Transformed(t),   dead6.Transformed(t)};
  }

  struct PropagateResult {
    bool consistent;
    bool changed;
  };

  // Propagate just using state/unknown and not the counts
  PropagateResult PropagateSimpleStep();
  PropagateResult PropagateSimple();

  PropagateResult SynchroniseStateKnown();
  PropagateResult UpdateOptions(); // Assumes counts and state/unknown are in sync
  PropagateResult StabiliseOptions(); // Apply the above two repeatedly

  PropagateResult SignalNeighbours(); // Assumes counts and state/unknown are in sync
  PropagateResult PropagateStep();
  PropagateResult Propagate();

  std::pair<uint64_t, uint64_t> SynchroniseStateKnownColumn(unsigned column);

  PropagateResult PropagateSimpleStepStrip(unsigned column);
  PropagateResult PropagateSimpleStrip(unsigned column);

  PropagateResult SynchroniseStateKnownStrip(unsigned column);
  PropagateResult UpdateOptionsStrip(unsigned column); // Assumes counts and state/unknown are in sync
  PropagateResult StabiliseOptionsStrip(unsigned column);
  PropagateResult SignalNeighboursStrip(unsigned column); // Assumes counts and state/unknown are in sync
  PropagateResult PropagateStepStrip(unsigned column);
  PropagateResult PropagateStrip(unsigned column);

  PropagateResult SynchroniseStateKnown(std::pair<int, int> cell);

  LifeState PerturbedUnknowns() const {
    LifeState perturbed = live2 | live3 | dead0 | dead1 | dead2 | dead4 | dead5 | dead6;
    return perturbed & unknown;
  }
  LifeState Vulnerable() const;

  PropagateResult TestUnknown(std::pair<int, int> cell);
  PropagateResult TestUnknowns(const LifeState &cells);

  PropagateResult PropagateAndTest() {
    bool everChanged = false;
    bool anyChanges = true;
    while (anyChanges) {
      anyChanges = false;

      auto propagateResult = Propagate();
      if (!propagateResult.consistent) {
        return {false, false};
      }
      anyChanges = anyChanges || propagateResult.changed;

      propagateResult = TestUnknowns(Vulnerable().ZOI());
      if (!propagateResult.consistent) {
        return {false, false};
      }
      anyChanges = anyChanges || propagateResult.changed;

      everChanged = anyChanges || everChanged;
    }
    return {true, everChanged};
  }
  
  enum struct CompletionResult {
    COMPLETED,
    INCONSISTENT,
    TIMEOUT,
  };

  CompletionResult CompleteStableStep(std::chrono::system_clock::time_point &timeLimit, bool minimise, bool useSeed, const LifeState &seed, unsigned &maxPop, LifeState &best);
  std::pair<LifeStable::CompletionResult, LifeState> CompleteStable(float timeout, bool minimise, bool useSeed = false, const LifeState &seed = LifeState());
  // std::pair<CompletionResult, LifeState> CompleteStable(float timeout, bool minimise);

  std::string RLE() const;
  std::string RLEWHeader() const {
    return "x = 0, y = 0, rule = LifeBellman\n" + RLE();
  }
  friend std::ostream& operator<<(std::ostream& os, LifeStable const& self) {
    return os << self.RLEWHeader();
  }

  bool CompatibleWith(const LifeState &desired) const;
  bool CompatibleWith(const LifeStable &desired) const;

  void SanityCheck() {
#ifdef DEBUG
    // LifeStable copy = *this;
    // auto [consistent, changes] = copy.SynchroniseStateKnown();
    // assert(consistent);
    // assert(!changes);
#endif
  }
};

LifeStable LifeStable::Join(const LifeStable &other) const {
  LifeStable result;

  result.unknown = unknown | other.unknown | (state ^ other.state);
  result.state = state & ~result.unknown;

  result.live2 = live2 & other.live2;
  result.live3 = live3 & other.live3;
  result.dead0 = dead0 & other.dead0;
  result.dead1 = dead1 & other.dead1;
  result.dead2 = dead2 & other.dead2;
  result.dead4 = dead4 & other.dead4;
  result.dead5 = dead5 & other.dead5;
  result.dead6 = dead6 & other.dead6;

  return result;
}

LifeStable LifeStable::Graft(const LifeStable &other) const {
  LifeStable result;

  result.unknown = unknown & ~(~other.unknown & other.dead0);
  result.state = state | other.state;

  result.live2 = live2 | (other.live2 & other.dead0);
  result.live3 = live3 | (other.live3 & other.dead0);
  result.dead0 = dead0 | (other.dead0 & other.dead0);
  result.dead1 = dead1 | (other.dead1 & other.dead0);
  result.dead2 = dead2 | (other.dead2 & other.dead0);
  result.dead4 = dead4 | (other.dead4 & other.dead0);
  result.dead5 = dead5 | (other.dead5 & other.dead0);
  result.dead6 = dead6 | (other.dead6 & other.dead0);

  return result;
}

LifeStable LifeStable::ClearUnmodified() const {
  LifeStable result = *this;

  LifeState toClear = unknown & ~(dead0.ZOI());

  result.unknown = unknown & ~toClear;
  result.state = state;

  result.UpdateOptions();

  return result;
}

LifeState LifeStable::Differences(const LifeStable &other) const {
  LifeState result;

  result |= state ^ other.state;
  result |= unknown ^ other.unknown;

  result |= live2 ^ other.live2;
  result |= live3 ^ other.live3;
  result |= dead0 ^ other.dead0;
  result |= dead1 ^ other.dead1;
  result |= dead2 ^ other.dead2;
  result |= dead4 ^ other.dead4;
  result |= dead5 ^ other.dead5;
  result |= dead6 ^ other.dead6;

  return result;
}

StableOptions LifeStable::GetOptions(std::pair<int, int> cell) const {
  auto result = StableOptions::IMPOSSIBLE;
  if(!live2.Get(cell)) result |= StableOptions::LIVE2;
  if(!live3.Get(cell)) result |= StableOptions::LIVE3;
  if(!dead0.Get(cell)) result |= StableOptions::DEAD0;
  if(!dead1.Get(cell)) result |= StableOptions::DEAD1;
  if(!dead2.Get(cell)) result |= StableOptions::DEAD2;
  if(!dead4.Get(cell)) result |= StableOptions::DEAD4;
  if(!dead5.Get(cell)) result |= StableOptions::DEAD5;
  if(!dead6.Get(cell)) result |= StableOptions::DEAD6;
  return result;
}
void LifeStable::RestrictOptions(std::pair<int, int> cell,
                                      StableOptions options) {
  if ((options & StableOptions::LIVE2) != StableOptions::LIVE2) live2.Set(cell);
  if ((options & StableOptions::LIVE3) != StableOptions::LIVE3) live3.Set(cell);
  if ((options & StableOptions::DEAD0) != StableOptions::DEAD0) dead0.Set(cell);
  if ((options & StableOptions::DEAD1) != StableOptions::DEAD1) dead1.Set(cell);
  if ((options & StableOptions::DEAD2) != StableOptions::DEAD2) dead2.Set(cell);
  if ((options & StableOptions::DEAD4) != StableOptions::DEAD4) dead4.Set(cell);
  if ((options & StableOptions::DEAD5) != StableOptions::DEAD5) dead5.Set(cell);
  if ((options & StableOptions::DEAD6) != StableOptions::DEAD6) dead6.Set(cell);
}

void LifeStable::RestrictOptions(LifeState cells,
                                      StableOptions options) {
  if ((options & StableOptions::LIVE2) != StableOptions::LIVE2) live2 |= cells;
  if ((options & StableOptions::LIVE3) != StableOptions::LIVE3) live3 |= cells;
  if ((options & StableOptions::DEAD0) != StableOptions::DEAD0) dead0 |= cells;
  if ((options & StableOptions::DEAD1) != StableOptions::DEAD1) dead1 |= cells;
  if ((options & StableOptions::DEAD2) != StableOptions::DEAD2) dead2 |= cells;
  if ((options & StableOptions::DEAD4) != StableOptions::DEAD4) dead4 |= cells;
  if ((options & StableOptions::DEAD5) != StableOptions::DEAD5) dead5 |= cells;
  if ((options & StableOptions::DEAD6) != StableOptions::DEAD6) dead6 |= cells;
}

void LifeStable::SetOn(const LifeState &which) {
  state |= which;
  unknown &= ~which;
  dead0 |= which;
  dead1 |= which;
  dead2 |= which;
  dead4 |= which;
  dead5 |= which;
  dead6 |= which;
}
void LifeStable::SetOff(const LifeState &which) {
  state &= ~which;
  unknown &= ~which;
  live2 |= which;
  live3 |= which;
}

void LifeStable::SetOn(unsigned i, uint64_t which) {
  state[i] |= which;
  unknown[i] &= ~which;
  dead0[i] |= which;
  dead1[i] |= which;
  dead2[i] |= which;
  dead4[i] |= which;
  dead5[i] |= which;
  dead6[i] |= which;
}
void LifeStable::SetOff(unsigned i, uint64_t which) {
  state[i] &= ~which;
  unknown[i] &= ~which;
  live2[i] |= which;
  live3[i] |= which;
}

void LifeStable::SetOn(std::pair<int, int> cell) {
  state.Set(cell);
  unknown.Erase(cell);
  RestrictOptions(cell, StableOptions::LIVE);
}

void LifeStable::SetOff(std::pair<int, int> cell) {
  state.Erase(cell);
  unknown.Erase(cell);
  RestrictOptions(cell, StableOptions::DEAD);
}

LifeState LifeStable::Vulnerable() const {
  NeighbourCount stateCount(state);
  NeighbourCount unknownCount(unknown);

  LifeState new_vulnerable_on(InitializedTag::UNINITIALIZED);
  LifeState new_vulnerable_off(InitializedTag::UNINITIALIZED);
  LifeState new_vulnerable_center_on(InitializedTag::UNINITIALIZED);
  LifeState new_vulnerable_center_off(InitializedTag::UNINITIALIZED);

  for (int i = 0; i < N; i++) {
    uint64_t l2 = live2[i];
    uint64_t l3 = live3[i];
    uint64_t d0 = dead0[i];
    uint64_t d1 = dead1[i];
    uint64_t d2 = dead2[i];
    uint64_t d4 = dead4[i];
    uint64_t d5 = dead5[i];
    uint64_t d6 = dead6[i];

    uint64_t s2 = stateCount.bit2[i];
    uint64_t s1 = stateCount.bit1[i];
    uint64_t s0 = stateCount.bit0[i];

    uint64_t unk3 = unknownCount.bit3[i];
    uint64_t unk2 = unknownCount.bit2[i];
    uint64_t unk1 = unknownCount.bit1[i];
    uint64_t unk0 = unknownCount.bit0[i];

    uint64_t vulnerable_on = 0;
    uint64_t vulnerable_off = 0;
    uint64_t vulnerable_center_on = 0;
    uint64_t vulnerable_center_off = 0;

    // Begin Autogenerated
#include "bitslicing/stable_vulnerable.hpp"
    // End Autogenerated

    new_vulnerable_on[i] = vulnerable_on;
    new_vulnerable_off[i] = vulnerable_off;
    new_vulnerable_center_on[i] = vulnerable_center_on;
    new_vulnerable_center_off[i] = vulnerable_center_off;
  }

  LifeState on = new_vulnerable_on.ZOIHollow() | new_vulnerable_center_on;
  LifeState off = new_vulnerable_off.ZOIHollow() | new_vulnerable_center_off;
  return on & off;
}

LifeStable::PropagateResult LifeStable::PropagateSimpleStep() {
  LifeState startUnknown = unknown;

  NeighbourCount stateCount(state);
  NeighbourCount unknownCount(unknown);

  LifeState new_off(InitializedTag::UNINITIALIZED), new_on(InitializedTag::UNINITIALIZED), new_signal_off(InitializedTag::UNINITIALIZED), new_signal_on(InitializedTag::UNINITIALIZED);

  uint64_t has_set_off = 0;
  uint64_t has_set_on = 0;
  uint64_t has_signal_off = 0;
  uint64_t has_signal_on = 0;
  uint64_t has_abort = 0;

  #pragma clang loop vectorize_width(4)
  for (int i = 0; i < N; i++) {
    uint64_t on2 = stateCount.bit2[i];
    uint64_t on1 = stateCount.bit1[i];
    uint64_t on0 = stateCount.bit0[i];

    uint64_t unk3 = unknownCount.bit3[i];
    uint64_t unk2 = unknownCount.bit2[i];
    uint64_t unk1 = unknownCount.bit1[i];
    uint64_t unk0 = unknownCount.bit0[i];

    unk1 |= unk2 | unk3;
    unk0 |= unk2 | unk3;

    uint64_t stateon = state[i];
    uint64_t stateunk = unknown[i];

    // These are the 5 output bits that are calculated for each cell
    uint64_t set_off = 0; // Set an UNKNOWN cell to OFF
    uint64_t set_on = 0;
    uint64_t signal_off = 0; // Set all UNKNOWN neighbours of the cell to OFF
    uint64_t signal_on = 0;
    uint64_t abort = 0; // The neighbourhood is inconsistent

    // Begin Autogenerated
#include "bitslicing/stable_simple.hpp"
    // End Autogenerated

   signal_off &= unk0 | unk1;
   signal_on  &= unk0 | unk1;

   new_off[i] = set_off & stateunk;
   new_on[i] = set_on & stateunk;
   new_signal_off[i] = signal_off;
   new_signal_on[i] = signal_on;

   has_set_off |= set_off;
   has_set_on |= set_on;
   has_signal_off |= signal_off;
   has_signal_on |= signal_on;
   has_abort |= abort;
  }

  if(has_abort != 0)
    return {false, false};

  if (has_set_on != 0) {
    state |= new_on;
    unknown &= ~new_on;
  }

  if (has_set_off != 0) {
    unknown &= ~new_off;
  }

  LifeState off_zoi(InitializedTag::UNINITIALIZED);
  LifeState on_zoi(InitializedTag::UNINITIALIZED);
  if (has_signal_off != 0) {
    off_zoi = new_signal_off.ZOI();
    unknown &= ~off_zoi;
  }

  if (has_signal_on != 0) {
    on_zoi = new_signal_on.ZOI();
    state |= on_zoi & unknown;
    unknown &= ~on_zoi;
  }

  if (has_signal_on != 0 && has_signal_off != 0) {
    if (!(on_zoi & off_zoi & unknown).IsEmpty()) {
      return {false, false};
    }
  }

  return {true, unknown != startUnknown};
}

LifeStable::PropagateResult LifeStable::PropagateSimple() {
  bool done = false;
  bool changed = false;
  while (!done) {
    auto result = PropagateSimpleStep();
    if (!result.consistent)
      return {false, false};
    if (result.changed)
      changed = true;
    done = !result.changed;
  }

  if (changed) {
    auto result = StabiliseOptions();
    if (!result.consistent)
      return {false, false};
  }

  return {true, changed};
}

LifeStable::PropagateResult LifeStable::SynchroniseStateKnown() {
  LifeState changes;

  LifeState knownOn = ~unknown & state;
  LifeState maybeDead = ~(dead0 & dead1 & dead2 & dead4 & dead5 & dead6);
  changes |= maybeDead & knownOn;
  dead0 |= knownOn;
  dead1 |= knownOn;
  dead2 |= knownOn;
  dead4 |= knownOn;
  dead5 |= knownOn;
  dead6 |= knownOn;

  LifeState knownOff = ~unknown & ~state;
  LifeState maybeLive = ~(live2 & live3);
  changes |= maybeLive & knownOff;
  live2 |= knownOff;
  live3 |= knownOff;

  LifeState abort = ~maybeLive & ~maybeDead;
  if (!abort.IsEmpty())
    return {false, false};

  changes |= ~state & (maybeLive & ~maybeDead);
  state |= maybeLive & ~maybeDead;

  changes |= ~unknown & (maybeLive & maybeDead);
  unknown &= maybeLive & maybeDead;

  return {true, !changes.IsEmpty()};
}

LifeStable::PropagateResult LifeStable::UpdateOptions() {
  NeighbourCount stateCount(state);
  NeighbourCount offCount(~unknown & ~state);

  uint64_t has_abort = 0;
  uint64_t changes = 0;

  #pragma clang loop vectorize_width(4)
  for (int i = 0; i < N; i++) {
    uint64_t on2 = stateCount.bit2[i];
    uint64_t on1 = stateCount.bit1[i];
    uint64_t on0 = stateCount.bit0[i];

    uint64_t off3 = offCount.bit3[i];
    uint64_t off2 = offCount.bit2[i];
    uint64_t off1 = offCount.bit1[i];
    uint64_t off0 = offCount.bit0[i];

    uint64_t known_on = state[i];
    uint64_t known_off = ~unknown[i] & ~state[i];

    uint64_t abort = 0;

    uint64_t l2 = 0;
    uint64_t l3 = 0;
    uint64_t d0 = 0;
    uint64_t d1 = 0;
    uint64_t d2 = 0;
    uint64_t d4 = 0;
    uint64_t d5 = 0;
    uint64_t d6 = 0;

// Begin Autogenerated, see bitslicing/stable_count.py
#include "bitslicing/stable_count.hpp"
// End Autogenerated

    changes |= l2 & ~live2[i];
    changes |= l3 & ~live3[i];
    changes |= d0 & ~dead0[i];
    changes |= d1 & ~dead1[i];
    changes |= d2 & ~dead2[i];
    changes |= d4 & ~dead4[i];
    changes |= d5 & ~dead5[i];
    changes |= d6 & ~dead6[i];

    live2[i] |= l2;
    live3[i] |= l3;
    dead0[i] |= d0;
    dead1[i] |= d1;
    dead2[i] |= d2;
    dead4[i] |= d4;
    dead5[i] |= d5;
    dead6[i] |= d6;
    has_abort |= abort;
  }

  return {has_abort == 0, changes != 0};
}

LifeStable::PropagateResult LifeStable::SignalNeighbours() {
  NeighbourCount stateCount(state);
  NeighbourCount maxCount(state | unknown);

  LifeState new_signal_off(InitializedTag::UNINITIALIZED), new_signal_on(InitializedTag::UNINITIALIZED);
  LifeState new_center_off(InitializedTag::UNINITIALIZED), new_center_on(InitializedTag::UNINITIALIZED);

  #pragma clang loop vectorize_width(4)
  for (int i = 0; i < N; i++) {
    uint64_t l2 = live2[i];
    uint64_t l3 = live3[i];
    uint64_t d0 = dead0[i];
    uint64_t d1 = dead1[i];
    uint64_t d2 = dead2[i];
    uint64_t d4 = dead4[i];
    uint64_t d5 = dead5[i];
    uint64_t d6 = dead6[i];

    uint64_t s2 = stateCount.bit2[i];
    uint64_t s1 = stateCount.bit1[i];
    uint64_t s0 = stateCount.bit0[i];

    uint64_t m3 = maxCount.bit3[i];
    uint64_t m2 = maxCount.bit2[i];
    uint64_t m1 = maxCount.bit1[i];
    uint64_t m0 = maxCount.bit0[i];

    uint64_t stateon = state[i];
    uint64_t stateunk = unknown[i];

    uint64_t signaloff = 0;
    uint64_t signalon = 0;

    uint64_t centeroff = 0;
    uint64_t centeron = 0;

// Begin Autogenerated, see bitslicing/stable_signal.py
#include "bitslicing/stable_signal.hpp"
// End Autogenerated

    new_signal_off[i] = signaloff;
    new_signal_on[i] = signalon;
    new_center_off[i] = centeroff;
    new_center_on[i] = centeron;
  }

  LifeState off_zoi = new_signal_off.ZOIHollow() | new_center_off;
  LifeState on_zoi = new_signal_on.ZOIHollow() | new_center_on;

  if (!(off_zoi & on_zoi & unknown).IsEmpty())
    return {false, false};

  LifeState changes = (off_zoi & unknown) | (on_zoi & unknown);

  SetOff(off_zoi & unknown);
  SetOn(on_zoi & unknown);

  return {true, !changes.IsEmpty()};
}

LifeStable::PropagateResult LifeStable::StabiliseOptions() {
  bool changedEver = false;
  bool done = false;
  while (!done) {
    PropagateResult knownresult = SynchroniseStateKnown();
    if (!knownresult.consistent)
      return {false, false};

    PropagateResult optionsresult = UpdateOptions();
    if (!optionsresult.consistent)
      return {false, false};

    done = !optionsresult.changed && !knownresult.changed;
    changedEver = changedEver || !done;
  }
  return {true, changedEver};
}

LifeStable::PropagateResult LifeStable::PropagateStep() {
  PropagateResult knownresult = SynchroniseStateKnown();
  if (!knownresult.consistent)
    return {false, false};

  PropagateResult optionsresult = UpdateOptions();
  if (!optionsresult.consistent)
    return {false, false};

  PropagateResult signalresult = SignalNeighbours();
  if (!signalresult.consistent)
    return {false, false};

  // PropagateResult simpleresult = PropagateSimpleStep();
  // if (!simpleresult.consistent)
  //   return {false, false};
  PropagateResult simpleresult = {true, false};

  bool changed = simpleresult.changed || optionsresult.changed ||
                 knownresult.changed || signalresult.changed;
  return {true, changed};
}

LifeStable::PropagateResult LifeStable::Propagate() {
  bool changedEver = false;
  bool done = false;
  while (!done) {
    PropagateResult result = PropagateStep();
    if (!result.consistent)
      return {false, false};
    done = !result.changed;
    changedEver = changedEver || !done;
  }
  return {true, changedEver};
}

void inline CountNeighbourhoodStrip(
    const std::array<uint64_t, 6> &state,
    std::array<uint64_t, 4> &bit3,
    std::array<uint64_t, 4> &bit2,
    std::array<uint64_t, 4> &bit1,
    std::array<uint64_t, 4> &bit0) {

  std::array<uint64_t, 6> col0;
  std::array<uint64_t, 6> col1;

  for (unsigned i = 0; i < 6; i++) {
    uint64_t a = state[i];
    uint64_t l = std::rotl(a, 1);
    uint64_t r = std::rotr(a, 1);

    col0[i] = l ^ r ^ a;
    col1[i] = ((l ^ r) & a) | (l & r);
  }

  #pragma clang loop vectorize(enable)
  for (unsigned i = 1; i < 5; i++) {
    int idxU = i-1;
    int idxB = i+1;

    uint64_t u_on1 = col1[idxU];
    uint64_t u_on0 = col0[idxU];
    uint64_t c_on1 = col1[i];
    uint64_t c_on0 = col0[i];
    uint64_t l_on1 = col1[idxB];
    uint64_t l_on0 = col0[idxB];

    uint64_t uc0, uc1, uc2, uc_carry0;
    LifeState::HalfAdd(uc0, uc_carry0, u_on0, c_on0);
    LifeState::FullAdd(uc1, uc2, u_on1, c_on1, uc_carry0);

    uint64_t on_carry1, on_carry0;
    LifeState::HalfAdd(bit0[i-1], on_carry0, uc0, l_on0);
    LifeState::FullAdd(bit1[i-1], on_carry1, uc1, l_on1, on_carry0);
    LifeState::HalfAdd(bit2[i-1], bit3[i-1], uc2, on_carry1);
  }
}

LifeStable::PropagateResult LifeStable::PropagateSimpleStepStrip(unsigned column) {
  std::array<uint64_t, 6> nearbyState = state.GetStrip<6>(column);
  std::array<uint64_t, 6> nearbyUnknown = unknown.GetStrip<6>(column);

  std::array<uint64_t, 4> state3, state2, state1, state0;
  std::array<uint64_t, 4> unknown3, unknown2, unknown1, unknown0;
  CountNeighbourhoodStrip(nearbyState, state3, state2, state1, state0);
  CountNeighbourhoodStrip(nearbyUnknown, unknown3, unknown2, unknown1, unknown0);

  std::array<uint64_t, 6> new_off {0};
  std::array<uint64_t, 6> new_on {0};

  std::array<uint64_t, 6> new_signal_off {0};
  std::array<uint64_t, 6> new_signal_on {0};

  uint64_t has_abort = 0;

  #pragma clang loop vectorize_width(4)
  for (int i = 1; i < 5; i++) {
    uint64_t on3 = state3[i-1];
    uint64_t on2 = state2[i-1];
    uint64_t on1 = state1[i-1];
    uint64_t on0 = state0[i-1];
    on2 |= on3;
    on1 |= on3;
    on0 |= on3;

    uint64_t unk3 = unknown3[i-1];
    uint64_t unk2 = unknown2[i-1];
    uint64_t unk1 = unknown1[i-1];
    uint64_t unk0 = unknown0[i-1];

    unk1 |= unk2 | unk3;
    unk0 |= unk2 | unk3;

    uint64_t stateon = nearbyState[i];
    uint64_t stateunk = nearbyUnknown[i];

    // These are the 5 output bits that are calculated for each cell
    uint64_t set_off = 0; // Set an UNKNOWN cell to OFF
    uint64_t set_on = 0;
    uint64_t signal_off = 0; // Set all UNKNOWN neighbours of the cell to OFF
    uint64_t signal_on = 0;
    uint64_t abort = 0; // The neighbourhood is inconsistent

    // Begin Autogenerated
#include "bitslicing/stable_simple.hpp"
    // End Autogenerated

    signal_off &= unk0 | unk1;
    signal_on  &= unk0 | unk1;

    new_off[i] = set_off & stateunk;
    new_on[i] = set_on & stateunk;
    new_signal_off[i] = signal_off;
    new_signal_on[i] = signal_on;

    has_abort |= abort;
  }

  if(has_abort != 0)
    return {false, false};

  std::array<uint64_t, 6> signalled_off {0};
  std::array<uint64_t, 6> signalled_on {0};

  #pragma clang loop vectorize_width(4)
  for (int i = 1; i < 5; i++) {
    uint64_t smear_off = std::rotl(new_signal_off[i], 1) | new_signal_off[i] | std::rotr(new_signal_off[i], 1);
   signalled_off[i-1] |= smear_off;
   signalled_off[i]   |= smear_off;
   signalled_off[i+1] |= smear_off;

   uint64_t smear_on  = std::rotl(new_signal_on[i], 1)  | new_signal_on[i]  | std::rotr(new_signal_on[i], 1);
   signalled_on[i-1] |= smear_on;
   signalled_on[i]   |= smear_on;
   signalled_on[i+1] |= smear_on;
  }

  uint64_t signalled_overlaps = 0;
  #pragma clang loop vectorize_width(4)
  for (int i = 0; i < 6; i++) {
    signalled_overlaps |= nearbyUnknown[i] & signalled_off[i] & signalled_on[i];
  }
  if(signalled_overlaps != 0)
    return {false, false};

  for (int i = 1; i < 5; i++) {
   signalled_off[i] &= ~new_signal_off[i];
   signalled_on[i] &= ~new_signal_on[i];
  }

  #pragma clang loop vectorize_width(4)
  for (int i = 1; i < 5; i++) {
    int orig = (column + i - 2 + N) % N;
    state[orig]  |= new_on[i];
    unknown[orig] &= ~new_off[i];
    unknown[orig] &= ~new_on[i];
  }

  #pragma clang loop vectorize_width(4)
  for (int i = 0; i < 6; i++) {
    int orig = (column + i - 2 + N) % N;
    state[orig]  |= signalled_on[i] & nearbyUnknown[i];
    unknown[orig] &= ~signalled_on[i];
    unknown[orig] &= ~signalled_off[i];
  }

  uint64_t unknownChanges = 0;
  #pragma clang loop vectorize_width(4)
  for (int i = 0; i < 6; i++) {
    int orig = (column + i - 2 + N) % N;
    unknownChanges |= unknown[orig] ^ nearbyUnknown[i];
  }

  return { true, unknownChanges != 0 };
}


std::pair<uint64_t, uint64_t> LifeStable::SynchroniseStateKnownColumn(unsigned i) {
  uint64_t changes = 0;

  uint64_t knownOn = ~unknown[i] & state[i];
  uint64_t maybeDead = ~(dead0[i] & dead1[i] & dead2[i] & dead4[i] & dead5[i] & dead6[i]);
  changes |= maybeDead & knownOn;
  dead0[i] |= knownOn;
  dead1[i] |= knownOn;
  dead2[i] |= knownOn;
  dead4[i] |= knownOn;
  dead5[i] |= knownOn;
  dead6[i] |= knownOn;

  uint64_t knownOff = ~unknown[i] & ~state[i];
  uint64_t maybeLive = ~(live2[i] & live3[i]);
  changes |= maybeLive & knownOff;
  live2[i] |= knownOff;
  live3[i] |= knownOff;

  changes |= ~state[i] & (maybeLive & ~maybeDead);
  state[i] |= maybeLive & ~maybeDead;

  changes |= ~unknown[i] & (maybeLive & maybeDead);
  unknown[i] &= maybeLive & maybeDead;

  uint64_t abort = ~maybeLive & ~maybeDead;
  return {abort, changes};
}

LifeStable::PropagateResult LifeStable::SynchroniseStateKnownStrip(unsigned column) {
  const unsigned width = 6;
  const unsigned offset = (width - 1) / 2; // 0, 0, 1, 1, 2, 2
  if (offset <= column && column + width - 1 - offset < N) {
    uint64_t abort = 0;
    uint64_t changes = 0;

    #pragma clang loop vectorize_width(4)
    for (unsigned i = 0; i < width; i++) {
      const unsigned c = column + i - offset;
      auto [column_abort, column_changes] = SynchroniseStateKnownColumn(c);
      abort |= column_abort;
      changes |= column_changes;
    }
    return {abort == 0, changes != 0};
  } else {
    uint64_t abort = 0;
    uint64_t changes = 0;

    for (unsigned i = 0; i < width; i++) {
      const unsigned c = (column + i + N - offset) % N;
      auto [column_abort, column_changes] = SynchroniseStateKnownColumn(c);
      abort |= column_abort;
      changes |= column_changes;
    }
    return {abort == 0, changes != 0};
  }
}

LifeStable::PropagateResult LifeStable::SynchroniseStateKnown(std::pair<int, int> cell) {
  bool knownOn = !unknown.Get(cell) && state.Get(cell);
  if(knownOn) {
    dead0.Set(cell);
    dead1.Set(cell);
    dead2.Set(cell);
    dead4.Set(cell);
    dead5.Set(cell);
    dead6.Set(cell);
  }
  bool knownOff = !unknown.Get(cell) && !state.Get(cell);
  if (knownOff) {
    live2.Set(cell);
    live3.Set(cell);
  }

  bool maybeLive = !(live2.Get(cell) && live3.Get(cell));
  bool maybeDead = !(dead0.Get(cell) && dead1.Get(cell) && dead2.Get(cell) && dead4.Get(cell) && dead5.Get(cell) && dead6.Get(cell));
  state.Set(cell, maybeLive && !maybeDead);
  unknown.Set(cell, maybeLive && maybeDead);

  return {true, false};
}

LifeStable::PropagateResult LifeStable::PropagateSimpleStrip(unsigned column) {
  bool done = false;
  bool changed = false;
  while (!done) {
    auto result = PropagateSimpleStepStrip(column);
    if (!result.consistent)
      return {false, false};
    changed = changed || result.changed;
    done = !result.changed;
  }


  if (changed) {
    auto result = StabiliseOptionsStrip(column);
    if (!result.consistent)
      return {false, false};
  }

  return {true, changed};
}

LifeStable::PropagateResult LifeStable::SignalNeighboursStrip(unsigned column) {
  std::array<uint64_t, 6> nearbyState = state.GetStrip<6>(column);
  std::array<uint64_t, 6> nearbyUnknown = unknown.GetStrip<6>(column);
  std::array<uint64_t, 6> nearbyMax;
  for(int i = 0; i < 6; i++) nearbyMax[i] = nearbyState[i] | nearbyUnknown[i];

  std::array<uint64_t, 4> nearbylive2 = live2.GetStrip<4>(column);
  std::array<uint64_t, 4> nearbylive3 = live3.GetStrip<4>(column);
  std::array<uint64_t, 4> nearbydead0 = dead0.GetStrip<4>(column);
  std::array<uint64_t, 4> nearbydead1 = dead1.GetStrip<4>(column);
  std::array<uint64_t, 4> nearbydead2 = dead2.GetStrip<4>(column);
  std::array<uint64_t, 4> nearbydead4 = dead4.GetStrip<4>(column);
  std::array<uint64_t, 4> nearbydead5 = dead5.GetStrip<4>(column);
  std::array<uint64_t, 4> nearbydead6 = dead6.GetStrip<4>(column);

  std::array<uint64_t, 4> state3, state2, state1, state0;
  std::array<uint64_t, 4> max3, max2, max1, max0;
  CountNeighbourhoodStrip(nearbyState, state3, state2, state1, state0);
  CountNeighbourhoodStrip(nearbyMax, max3, max2, max1, max0);

  std::array<uint64_t, 6> new_signal_off, new_signal_on;
  std::array<uint64_t, 6> new_center_off, new_center_on;

  #pragma clang loop vectorize_width(4)
  for (int i = 1; i < 5; i++) {
    uint64_t l2 = nearbylive2[i-1];
    uint64_t l3 = nearbylive3[i-1];
    uint64_t d0 = nearbydead0[i-1];
    uint64_t d1 = nearbydead1[i-1];
    uint64_t d2 = nearbydead2[i-1];
    uint64_t d4 = nearbydead4[i-1];
    uint64_t d5 = nearbydead5[i-1];
    uint64_t d6 = nearbydead6[i-1];

    uint64_t s2 = state2[i-1];
    uint64_t s1 = state1[i-1];
    uint64_t s0 = state0[i-1];

    uint64_t m3 = max3[i-1];
    uint64_t m2 = max2[i-1];
    uint64_t m1 = max1[i-1];
    uint64_t m0 = max0[i-1];

    uint64_t stateon = nearbyState[i];
    uint64_t stateunk = nearbyUnknown[i];

    uint64_t signaloff = 0;
    uint64_t signalon = 0;
    uint64_t centeroff = 0;
    uint64_t centeron = 0;

// Begin Autogenerated, see bitslicing/stable_signal.py
#include "bitslicing/stable_signal.hpp"
// End Autogenerated

    new_signal_off[i] = signaloff;
    new_signal_on[i] = signalon;
    new_center_off[i] = centeroff;
    new_center_on[i] = centeron;
  }

  std::array<uint64_t, 6> signalled_off {0};
  std::array<uint64_t, 6> signalled_on {0};

  #pragma clang loop vectorize_width(4)
  for (int i = 1; i < 5; i++) {
   signalled_off[i]  |= new_center_off[i];
   signalled_on[i]   |= new_center_on[i];

   uint64_t smear_off = std::rotl(new_signal_off[i], 1) | new_signal_off[i] | std::rotr(new_signal_off[i], 1);
   uint64_t smear_off_mid = std::rotl(new_signal_off[i], 1) |  std::rotr(new_signal_off[i], 1);
   signalled_off[i-1] |= smear_off;
   signalled_off[i]   |= smear_off_mid;
   signalled_off[i+1] |= smear_off;

   uint64_t smear_on  = std::rotl(new_signal_on[i], 1)  | new_signal_on[i]  | std::rotr(new_signal_on[i], 1);
   uint64_t smear_on_mid  = std::rotl(new_signal_on[i], 1) | std::rotr(new_signal_on[i], 1);
   signalled_on[i-1] |= smear_on;
   signalled_on[i]   |= smear_on_mid;
   signalled_on[i+1] |= smear_on;
  }

  uint64_t signalled_overlaps = 0;
  #pragma clang loop vectorize_width(4)
  for (int i = 0; i < 6; i++) {
    signalled_overlaps |= nearbyUnknown[i] & signalled_off[i] & signalled_on[i];
  }
  if(signalled_overlaps != 0)
    return {false, false};

  const unsigned width = 6;
  const unsigned offset = (width - 1) / 2;
  if (offset <= column && column + width - 1 - offset < N) {
    #pragma clang loop vectorize_width(4)
    for (unsigned i = 0; i < width; i++) {
      int orig = column + i - offset;
      SetOff(orig, signalled_off[i] & nearbyUnknown[i]);
      SetOn( orig, signalled_on[i]  & nearbyUnknown[i]);
    }
  } else {
    for (unsigned i = 0; i < width; i++) {
      int orig = (column + i - offset + N) % N;
      SetOff(orig, signalled_off[i] & nearbyUnknown[i]);
      SetOn( orig, signalled_on[i]  & nearbyUnknown[i]);
    }
  }

  uint64_t unknownChanges = 0;
  #pragma clang loop vectorize_width(4)
  for (int i = 0; i < 6; i++) {
    unknownChanges |= (signalled_on[i] & nearbyUnknown[i]) | (signalled_off[i] & nearbyUnknown[i]);
  }

  return {true, unknownChanges != 0};
}

LifeStable::PropagateResult LifeStable::UpdateOptionsStrip(unsigned column) {
  std::array<uint64_t, 6> nearbyState = state.GetStrip<6>(column);
  std::array<uint64_t, 6> nearbyUnknown = unknown.GetStrip<6>(column);
  std::array<uint64_t, 6> nearbyOff;
  for (int i = 0; i < 6; i++) {
    nearbyOff[i] = ~nearbyUnknown[i] & ~nearbyState[i];
  }

  std::array<uint64_t, 4> nearbylive2 = live2.GetStrip<4>(column);
  std::array<uint64_t, 4> nearbylive3 = live3.GetStrip<4>(column);
  std::array<uint64_t, 4> nearbydead0 = dead0.GetStrip<4>(column);
  std::array<uint64_t, 4> nearbydead1 = dead1.GetStrip<4>(column);
  std::array<uint64_t, 4> nearbydead2 = dead2.GetStrip<4>(column);
  std::array<uint64_t, 4> nearbydead4 = dead4.GetStrip<4>(column);
  std::array<uint64_t, 4> nearbydead5 = dead5.GetStrip<4>(column);
  std::array<uint64_t, 4> nearbydead6 = dead6.GetStrip<4>(column);

  std::array<uint64_t, 4> state3, state2, state1, state0;
  std::array<uint64_t, 4> offState3, offState2, offState1, offState0;
  CountNeighbourhoodStrip(nearbyState, state3, state2, state1, state0);
  CountNeighbourhoodStrip(nearbyOff, offState3, offState2, offState1, offState0);

  uint64_t has_abort = 0;
  uint64_t changes = 0;

  #pragma clang loop vectorize_width(4)
  for (int i = 1; i < 5; i++) {
    uint64_t on2 = state2[i-1];
    uint64_t on1 = state1[i-1];
    uint64_t on0 = state0[i-1];

    uint64_t off3 = offState3[i-1];
    uint64_t off2 = offState2[i-1];
    uint64_t off1 = offState1[i-1];
    uint64_t off0 = offState0[i-1];

    uint64_t known_on = nearbyState[i];
    uint64_t known_off = nearbyOff[i];

    uint64_t abort = 0;

    uint64_t l2 = 0;
    uint64_t l3 = 0;
    uint64_t d0 = 0;
    uint64_t d1 = 0;
    uint64_t d2 = 0;
    uint64_t d4 = 0;
    uint64_t d5 = 0;
    uint64_t d6 = 0;

// Begin Autogenerated, see bitslicing/stable_count.py
#include "bitslicing/stable_count.hpp"
// End Autogenerated

    changes |= l2 & ~nearbylive2[i-1];
    changes |= l3 & ~nearbylive3[i-1];
    changes |= d0 & ~nearbydead0[i-1];
    changes |= d1 & ~nearbydead1[i-1];
    changes |= d2 & ~nearbydead2[i-1];
    changes |= d4 & ~nearbydead4[i-1];
    changes |= d5 & ~nearbydead5[i-1];
    changes |= d6 & ~nearbydead6[i-1];

    nearbylive2[i-1] |= l2;
    nearbylive3[i-1] |= l3;
    nearbydead0[i-1] |= d0;
    nearbydead1[i-1] |= d1;
    nearbydead2[i-1] |= d2;
    nearbydead4[i-1] |= d4;
    nearbydead5[i-1] |= d5;
    nearbydead6[i-1] |= d6;
    has_abort |= abort;
  }

  live2.SetStrip<4>(column, nearbylive2);
  live3.SetStrip<4>(column, nearbylive3);
  dead0.SetStrip<4>(column, nearbydead0);
  dead1.SetStrip<4>(column, nearbydead1);
  dead2.SetStrip<4>(column, nearbydead2);
  dead4.SetStrip<4>(column, nearbydead4);
  dead5.SetStrip<4>(column, nearbydead5);
  dead6.SetStrip<4>(column, nearbydead6);

  return {has_abort == 0, changes != 0};
}

LifeStable::PropagateResult LifeStable::StabiliseOptionsStrip(unsigned column) {
  bool changedEver = false;
  bool done = false;
  while (!done) {
    PropagateResult knownresult = SynchroniseStateKnownStrip(column);
    if (!knownresult.consistent)
      return {false, false};

    PropagateResult optionsresult = UpdateOptionsStrip(column);
    if (!optionsresult.consistent)
      return {false, false};

    done = !optionsresult.changed && !knownresult.changed;
    changedEver = changedEver || !done;
  }
  return {true, changedEver};
}

LifeStable::PropagateResult LifeStable::PropagateStepStrip(unsigned column) {
  PropagateResult knownresult = SynchroniseStateKnownStrip(column);
  if (!knownresult.consistent)
    return {false, false};

  PropagateResult optionsresult = UpdateOptionsStrip(column);
  if (!optionsresult.consistent)
    return {false, false};

  PropagateResult signalresult = SignalNeighboursStrip(column);
  if (!signalresult.consistent)
    return {false, false};

  // PropagateResult simpleresult = PropagateSimpleStepStrip(column);
  // if (!simpleresult.consistent)
  //   return {false, false};
  PropagateResult simpleresult = {true, false};

  bool changed = simpleresult.changed || optionsresult.changed ||
                 knownresult.changed || signalresult.changed;
  return {true, changed};
}

LifeStable::PropagateResult LifeStable::PropagateStrip(unsigned column) {
  bool changedEver = false;
  bool done = false;
  while (!done) {
    PropagateResult result = PropagateStepStrip(column);
    if (!result.consistent)
      return {false, false};
    done = !result.changed;
    changedEver = changedEver || !done;
  }
  return {true, changedEver};
}

LifeStable::PropagateResult LifeStable::TestUnknown(std::pair<int, int> cell) {
  // Try on
  LifeStable onSearch = *this;
  onSearch.SetOn(cell);
  auto onResult = onSearch.PropagateStrip(cell.first);

  if (!onResult.consistent) {
    SetOff(cell);
    auto offResult = PropagateStrip(cell.first);
    if (!offResult.consistent)
      return {false, false};
    else
      return {true, true};
  }

  // Try off
  LifeStable offSearch = *this;
  offSearch.SetOff(cell);
  auto offResult = offSearch.PropagateStrip(cell.first);

  if (onResult.consistent && !offResult.consistent) {
    *this = onSearch;
    return {true, true};
  }

  if (onResult.consistent && offResult.consistent && onResult.changed && offResult.changed) {
    LifeStable joined = onSearch.Join(offSearch);
    bool change = joined != *this;
    *this = joined;
    return {true, change};
  }

  return {true, false}; // Impossible
}

// LifeStable::PropagateResult LifeStable::TestUnknown(std::pair<int, int> cell) {
//   // Try on
//   LifeStable onSearch = *this;
//   onSearch.SetOn(cell);
//   auto onResult = onSearch.Propagate();

//   if (!onResult.consistent) {
//     SetOff(cell);
//     auto offResult = Propagate();
//     if (!offResult.consistent)
//       return {false, false};
//     else
//       return {true, true};
//   }

//   // Try off
//   LifeStable offSearch = *this;
//   offSearch.SetOff(cell);
//   auto offResult = offSearch.Propagate();

//   if (onResult.consistent && !offResult.consistent) {
//     *this = onSearch;
//     return {true, true};
//   }

//   if (onResult.consistent && offResult.consistent && onResult.changed && offResult.changed) {
//     LifeStable joined = onSearch.Join(offSearch);
//     bool change = joined != *this;
//     *this = joined;
//     return {true, change};
//   }

//   return {true, false}; // Impossible
// }

LifeStable::PropagateResult LifeStable::TestUnknowns(const LifeState &cells) {
  // Try all the nearby changes to see if any are forced
  LifeState remainingCells = cells & unknown;
  bool anyChanges = false;
  while (!remainingCells.IsEmpty()) {
    auto cell = remainingCells.FirstOn();
    remainingCells.Erase(cell);

    auto result = TestUnknown(cell);
    if (!result.consistent)
      return {false, false};
    anyChanges = anyChanges || result.changed;

    remainingCells &= unknown;
  }

  return {true, anyChanges};
}

LifeStable::CompletionResult LifeStable::CompleteStableStep(
    std::chrono::system_clock::time_point &timeLimit, bool minimise,
    bool useSeed, const LifeState &seed, unsigned &maxPop, LifeState &best) {
  auto currentTime = std::chrono::system_clock::now();
  if (currentTime > timeLimit)
      return CompletionResult::TIMEOUT;

  bool consistent = Propagate().consistent;
  if (!consistent)
    return CompletionResult::INCONSISTENT;

  unsigned currentPop = state.GetPop();

  if (currentPop >= maxPop) {
    return CompletionResult::COMPLETED;
  }

  LifeState settable = PerturbedUnknowns() & dead0.ZOI();

  if (settable.IsEmpty()) {
    // We win
    best = state;
    maxPop = state.GetPop();
    return CompletionResult::COMPLETED;
  }

  if(useSeed) {
    // Prefer cells close to the seed
    LifeState seedZOI = seed;
    while (true) {
      if (!(settable & seedZOI).IsEmpty())
        break;
      seedZOI = seedZOI.ZOI();
    }
    settable &= seedZOI;
  }

  // Now make a guess for the best cell to branch on
  std::pair<int, int> newPlacement = {-1, -1};
  newPlacement = (Vulnerable() & settable).FirstOn();

  if (newPlacement.first == -1) {
    NeighbourCount unknownCount(unknown);

    newPlacement = (settable & (~unknownCount.bit3 & ~unknownCount.bit2 & unknownCount.bit1 & ~unknownCount.bit0)).FirstOn();
    if(newPlacement.first == -1)
      newPlacement = (settable & (~unknownCount.bit3 & ~unknownCount.bit2 & unknownCount.bit1 & unknownCount.bit0)).FirstOn();
    if(newPlacement.first == -1)
      newPlacement = settable.FirstOn();
    if(newPlacement.first == -1)
      return CompletionResult::INCONSISTENT;
  }

  // Try off
  {
    LifeStable nextState = *this;
    nextState.SetOff(newPlacement);
    auto result = nextState.CompleteStableStep(timeLimit, minimise, useSeed, seed, maxPop, best);
    if (result == CompletionResult::TIMEOUT)
      return CompletionResult::TIMEOUT;
    if (!minimise && result == CompletionResult::COMPLETED)
      return CompletionResult::COMPLETED;
  }

  // Then must be on
  {
    LifeStable &nextState = *this;
    nextState.SetOn(newPlacement);

    [[clang::musttail]]
    return nextState.CompleteStableStep(timeLimit, minimise, useSeed, seed, maxPop, best);
  }
}

std::pair<LifeStable::CompletionResult, LifeState> LifeStable::CompleteStable(float timeout, bool minimise, bool useSeed, const LifeState &seed) {
  if (state.IsEmpty()) {
    return {CompletionResult::COMPLETED, LifeState()};
  }
  if (unknown.IsEmpty()) {
    return {CompletionResult::COMPLETED, state};
  }

  LifeState best;
  unsigned maxPop = std::numeric_limits<int>::max();

  auto startTime = std::chrono::system_clock::now();
  auto timeLimit = startTime + std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::duration<float>(timeout)); // Why is C++ like this

  CompletionResult result;

  // First find a solution with small BB
  LifeState searchArea = state.ZOI();
  while (!(unknown & ~searchArea).IsEmpty()) {
    searchArea = searchArea.ZOI();

    LifeStable copy = *this;
    copy.unknown &= searchArea;
    result = copy.CompleteStableStep(timeLimit, minimise, useSeed, seed, maxPop, best);

    auto currentTime = std::chrono::system_clock::now();
    if (best.GetPop() > 0 || currentTime > timeLimit)
      break;
  }

  if (result == CompletionResult::TIMEOUT && best.IsEmpty())
    return {CompletionResult::TIMEOUT, LifeState()};

  if (result == CompletionResult::INCONSISTENT && best.IsEmpty())
    return {CompletionResult::INCONSISTENT, LifeState()};

  if (minimise) {
    // Then try again with a little more space
    LifeStable copy = *this;
    copy.unknown &= searchArea.BigZOI();
    copy.CompleteStableStep(timeLimit, minimise, true, state | best, maxPop, best);
  }

  return {CompletionResult::COMPLETED, best};
}


bool LifeStable::CompatibleWith(const LifeState &desired) const {
  LifeStable desiredStable = LifeStable();
  desiredStable.state = desired;
  desiredStable.StabiliseOptions();
  return CompatibleWith(desiredStable);
}

bool LifeStable::CompatibleWith(const LifeStable &desired) const {
  if(!(live2 & ~desired.live2).IsEmpty()) return false;
  if(!(live3 & ~desired.live3).IsEmpty()) return false;
  if(!(dead0 & ~desired.dead0).IsEmpty()) return false;
  if(!(dead1 & ~desired.dead1).IsEmpty()) return false;
  if(!(dead2 & ~desired.dead2).IsEmpty()) return false;
  if(!(dead4 & ~desired.dead4).IsEmpty()) return false;
  if(!(dead5 & ~desired.dead5).IsEmpty()) return false;
  if(!(dead6 & ~desired.dead6).IsEmpty()) return false;
  if(!(~unknown & ~desired.unknown & (state ^ desired.state)).IsEmpty()) return false;
  return true;
}

std::string LifeStable::RLE() const {
  LifeState marked = unknown | state;
  return GenericRLE([&](int x, int y) -> char {
    const std::array<char, 4> table = {'.', 'A', 'E', 'C'};
    return table[state.Get(x, y) + (marked.Get(x, y) << 1)];
  });
}
