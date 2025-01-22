#pragma once

#include "LifeAPI.hpp"

struct LifeTarget {
    LifeState wanted;
    LifeState unwanted;

    LifeTarget() = default;
    explicit LifeTarget(const LifeState& state)
        : wanted{state}
        , unwanted{state.GetBoundary()} 
    {}

    LifeTarget(const LifeState& wanted, const LifeState& unwanted)
        : wanted{wanted}
        , unwanted{unwanted} 
    {}

    void Transform(SymmetryTransform transf) {
        wanted.Transform(transf);
        unwanted.Transform(transf);
    }

    [[nodiscard]]
    LifeTarget Transformed(SymmetryTransform transf) const {
        LifeTarget result = *this;
        result.Transform(transf);
        return result;
    }

    [[nodiscard]]
    LifeTarget Moved(std::pair<int, int> vec) const {
        return LifeTarget(wanted.Moved(vec), unwanted.Moved(vec));
    }
};

inline bool LifeState::Contains(const LifeTarget &target, int dx,
                                int dy) const {
  return Contains(target.wanted, dx, dy) &&
         AreDisjoint(target.unwanted, dx, dy);
}

inline bool LifeState::Contains(const LifeTarget &target) const {
  uint64_t differences = 0;
  for (unsigned i = 0; i < N; i++) {
    uint64_t difference = (state[i] ^ target.wanted[i]) & (target.wanted[i] | target.unwanted[i]);
    differences |= difference;
  }
  return differences == 0;
}

inline LifeState LifeState::Match(const LifeTarget &target) const {
  return MatchLiveAndDead(target.wanted, target.unwanted);
}