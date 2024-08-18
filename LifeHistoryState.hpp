#pragma once

#include "LifeAPI.hpp"
#include "Parsing.hpp"

// This uses lifelib "layers" which do not match Golly's state names,
// so the parsing has to adjust for this.
struct LifeHistoryState {
  LifeState state;
  LifeState history;
  LifeState marked;
  LifeState original;

  LifeHistoryState() = default;
  LifeHistoryState(const LifeState &state, const LifeState &history,
                   const LifeState &marked, const LifeState &original)
      : state{state}, history{history}, marked{marked}, original{original} {};
  LifeHistoryState(const LifeState &state, const LifeState &history, const LifeState &marked)
      : state{state}, history{history}, marked{marked}, original{LifeState()} {};
  LifeHistoryState(const LifeState &state, const LifeState &history)
      : state{state}, history{history}, marked{LifeState()},
        original{LifeState()} {};

  std::string RLE() const;
  std::string RLEWHeader() const {
    return "x = 0, y = 0, rule = LifeHistory\n" + RLE();
  }
  friend std::ostream& operator<<(std::ostream& os, LifeHistoryState const& self) {
    return os << self.RLEWHeader();
  }

  static char StateToChar(unsigned mask) {
    switch (mask) {
    case 0b0000: return '.';
    case 0b0001: return 'A';
    case 0b0010: return 'B';
    case 0b0101: return 'C';
    case 0b0100: return 'D';
    case 0b1001: return 'E';
    default:     return 'F';
    }
  }

  static LifeHistoryState Parse(const std::string &s);
  static LifeHistoryState ParseBellman(const std::string &s);

  void Move(int x, int y) {
    state.Move(x, y);
    history.Move(x, y);
    marked.Move(x, y);
    original.Move(x, y);
  }

  void Move(std::pair<int, int> vec) { Move(vec.first, vec.second); }

  // void AlignWith(const LifeHistoryState &other) {
  //   auto othercorner =
  //       (other.state & ~other.marked & ~other.original).FirstOn();
  //   auto self = (state & ~marked & ~original).FirstOn();
  //   Move(-self.first + othercorner.first, -self.second + othercorner.second);
  // }

  void AlignWith(const LifeState &other) {
    auto othercorner = other.XYBounds();
    auto self = state.XYBounds();
    Move(-self[0] + othercorner[0], -self[1] + othercorner[1]);
  }
};

std::string LifeHistoryState::RLE() const {
  return GenericRLE([&](int x, int y) -> char {
    unsigned val = state.Get(x, y) + (history.Get(x, y) << 1) + (marked.Get(x, y) << 2) + (original.Get(x, y) << 3);

    return StateToChar(val);
  });
}

LifeHistoryState LifeHistoryState::Parse(const std::string &rle) {
  return GenericParse<LifeHistoryState>(rle, [&](LifeHistoryState &result, char ch, int x, int y) -> void {
    switch(ch) {
    case 'A':
      result.state.Set(x, y);
      break;
    case 'B':
      result.history.Set(x, y);
      break;
    case 'C':
      result.state.Set(x, y);
      result.marked.Set(x, y);
      break;
    case 'D':
      result.marked.Set(x, y);
      break;
    case 'E':
      result.state.Set(x, y);
      result.original.Set(x, y);
      break;
    }
  });
}
LifeHistoryState LifeHistoryState::ParseBellman(const std::string &rle) {
  return GenericParse<LifeHistoryState>(rle, [&](LifeHistoryState &result, char ch, int x, int y) -> void {
    switch(ch) {
    case 'C':
      result.state.Set(x, y);
      break;
    case 'E':
      result.history.Set(x, y);
      break;
    }
  });
}
