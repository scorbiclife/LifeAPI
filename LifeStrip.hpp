#pragma once

#include "LifeAPI.hpp"

struct StripIndex {
  unsigned index;
};

// TODO: template the width
struct LifeStateStrip {
  uint64_t state[4];

  LifeStateStrip() : state{0} {}
  LifeStateStrip(const LifeStateStripProxy &proxy);

  uint64_t &operator[](const unsigned i) { return state[i]; }
  uint64_t operator[](const unsigned i) const { return state[i]; }

  LifeStateStrip operator~() const {
    LifeStateStrip result;
    for (unsigned i = 0; i < 4; i++) {
      result[i] = ~state[i];
    }
    return result;
  }

  LifeStateStrip operator|(const LifeStateStrip &other) const {
    LifeStateStrip result;
    for (unsigned i = 0; i < 4; i++) {
      result[i] = state[i] | other[i];
    }
    return result;
  }

  LifeStateStrip operator&(const LifeStateStrip &other) const {
    LifeStateStrip result;
    for (unsigned i = 0; i < 4; i++) {
      result[i] = state[i] & other[i];
    }
    return result;
  }

  LifeStateStrip operator^(const LifeStateStrip &other) const {
    LifeStateStrip result;
    for (unsigned i = 0; i < 4; i++) {
      result[i] = state[i] ^ other[i];
    }
    return result;
  }

  bool IsEmpty() const {
    uint64_t all = 0;
    for (unsigned i = 0; i < 4; i++) {
      all |= state[i];
    }

    return all == 0;
  }

  inline friend std::ostream &operator<<(std::ostream &os,
                                         LifeStateStrip const &self);
};

struct LifeStateStripProxy {
  LifeState *state;
  StripIndex index;

  LifeStateStripProxy &operator=(const LifeStateStrip &strip) {
    for (unsigned i = 0; i < 4; i++) {
      state->state[index.index + i] = strip[i];
    }
    return *this;
  }

  uint64_t& operator[](const unsigned i) { return state->state[index.index + i]; }
  uint64_t operator[](const unsigned i) const { return state->state[index.index + i]; }

  LifeStateStrip operator~() const { return ~LifeStateStrip(*this); }
  LifeStateStrip operator|(LifeStateStripProxy proxy) const { return LifeStateStrip(*this) | LifeStateStrip(proxy); }
  LifeStateStrip operator&(LifeStateStripProxy proxy) const { return LifeStateStrip(*this) & LifeStateStrip(proxy); }
  LifeStateStrip operator^(LifeStateStripProxy proxy) const { return LifeStateStrip(*this) ^ LifeStateStrip(proxy); }
  LifeStateStripProxy &operator|=(LifeStateStrip strip){
  for (unsigned i = 0; i < 4; i++) {
    state->state[index.index + i] |= strip[i];
  }
  return *this;
}
  LifeStateStripProxy &operator&=(LifeStateStrip strip){
  for (unsigned i = 0; i < 4; i++) {
    state->state[index.index + i] &= strip[i];
  }
  return *this;
  }
};

inline LifeStateStrip::LifeStateStrip(const LifeStateStripProxy &proxy) {
  for (unsigned i = 0; i < 4; i++) {
    state[i] = proxy.state->state[proxy.index.index + i];
  }
}

struct LifeStateStripConstProxy {
  const LifeState *conststate;
  StripIndex index;

  uint64_t operator[](const unsigned i) const { return conststate->state[index.index + i]; }
};

LifeStateStripProxy LifeState::operator[](const StripIndex column) {
  return {this, column};
}
const LifeStateStripConstProxy LifeState::operator[](const StripIndex column) const {
  return {this, column};
}

struct StripIterator {
  struct IteratorState {
    using iterator_category = std::forward_iterator_tag;
    using value_type = StripIndex;

    uint64_t remaining;

    IteratorState(uint64_t mask) : remaining(mask) {}

    StripIndex operator*() const { return {static_cast<unsigned int>(std::min(std::countr_zero(remaining), N-4))}; }

    IteratorState &operator++() {
      unsigned i = std::min(std::countr_zero(remaining), N - 4); // Don't wrap
      uint64_t nyb = (1ULL << 4) - 1;
      remaining &= ~(nyb << i);
      return *this;
    }

    friend bool operator== (const IteratorState& a, const IteratorState& b) { return a.remaining == b.remaining; };
    friend bool operator!= (const IteratorState& a, const IteratorState& b) { return a.remaining != b.remaining; };
  };

  uint64_t mask;

  StripIterator(uint64_t mask) : mask(mask) {}

  IteratorState begin() { return IteratorState(mask); }
  IteratorState end() { return IteratorState(0); }
};

std::ostream &operator<<(std::ostream &os, LifeStateStrip const &self) {
  LifeState blank;
  blank[StripIndex{1}] = self;
  return os << blank.RLE();
}
