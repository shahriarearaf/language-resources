// festus/value-weight-singleton.h
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Copyright 2016 Google, Inc.
// Author: mjansche@google.com (Martin Jansche)
//
// \file
// Façade for OpenFst weights around a singleton semiring object.

#ifndef FESTUS_VALUE_WEIGHT_SINGLETON_H__
#define FESTUS_VALUE_WEIGHT_SINGLETON_H__

#include <cstddef>
#include <functional>
#include <istream>
#include <ostream>
#include <type_traits>
#include <utility>

#include <fst/compat.h>
#include <fst/weight.h>

namespace festus {

template <class S>
struct DefaultInstance {
  // Owned pointer, will never be deleted.
  static const S *const kInstance;
  static constexpr const S &Instance() { return *kInstance; }
};

template <class S>
const S *const DefaultInstance<S>::kInstance = new S();

template <class S>
struct DefaultStaticInstance {
  static constexpr S kStaticInstance = {};
  static constexpr const S &Instance() { return kStaticInstance; }
};

template <class S>
constexpr S DefaultStaticInstance<S>::kStaticInstance;

// OpenFst weight façade for semirings whose elements are passed by value.
//
// This version works with a semiring class S with const member functions and
// uses a singleton instance of S, which must never be deleted. The singleton
// semiring object should be thread safe.
template <class S, class Singleton = DefaultInstance<S>>
class ValueWeightSingleton {
 public:
  typedef S SemiringType;
  typedef typename S::ValueType ValueType;
  typedef ValueWeightSingleton ReverseWeight;

  ValueWeightSingleton() = default;
  ~ValueWeightSingleton() = default;
  ValueWeightSingleton(ValueWeightSingleton &&) = default;
  ValueWeightSingleton(const ValueWeightSingleton &) = default;
  ValueWeightSingleton &operator=(ValueWeightSingleton &&) = default;
  ValueWeightSingleton &operator=(const ValueWeightSingleton &) = default;

  // Deprecated implicit constructor. Use From() instead.
  constexpr ValueWeightSingleton(ValueType value) : value_(value) {}

  template <typename... Args>
  static constexpr ValueWeightSingleton From(Args &&... args) {
    return ValueWeightSingleton(Semiring().From(std::forward<Args>(args)...));
  }

  constexpr ValueType Value() const { return value_; }

  static constexpr const S &Semiring() { return Singleton::Instance(); }

  static constexpr ValueWeightSingleton NoWeight() {
    return ValueWeightSingleton(Semiring().NoWeight());
  }

  static constexpr ValueWeightSingleton Zero() {
    return ValueWeightSingleton(Semiring().Zero());
  }

  static constexpr ValueWeightSingleton One() {
    return ValueWeightSingleton(Semiring().One());
  }

  friend inline ValueWeightSingleton Plus(
      ValueWeightSingleton lhs,
      ValueWeightSingleton rhs) {
    lhs.value_ = Semiring().OpPlus(lhs.value_, rhs.value_);
    return lhs;
  }

  friend inline ValueWeightSingleton Minus(
      ValueWeightSingleton lhs,
      ValueWeightSingleton rhs) {
    lhs.value_ = Semiring().OpMinus(lhs.value_, rhs.value_);
    return lhs;
  }

  friend inline ValueWeightSingleton Times(
      ValueWeightSingleton lhs,
      ValueWeightSingleton rhs) {
    lhs.value_ = Semiring().OpTimes(lhs.value_, rhs.value_);
    return lhs;
  }

  friend inline ValueWeightSingleton Divide(
      ValueWeightSingleton lhs,
      ValueWeightSingleton rhs,
      fst::DivideType typ = fst::DIVIDE_ANY) {
    if (Semiring().Commutative()) {
      return DivideCommutative(lhs, rhs);
    } else {
      return DivideNoncommutative(lhs, rhs, typ);
    }
  }

  friend inline ValueWeightSingleton Star(
      ValueWeightSingleton w) {
    w.value_ = Semiring().OpStar(w.value_);
    return w;
  }

  constexpr ValueWeightSingleton Reverse() const {
    return ValueWeightSingleton(Semiring().Reverse(value_));
  }

  constexpr ValueWeightSingleton Quantize(float delta = fst::kDelta) const {
    return ValueWeightSingleton(Semiring().Quantize(value_, delta));
  }

  constexpr bool Member() const { return Semiring().Member(value_); }

  friend constexpr bool operator==(
      ValueWeightSingleton lhs,
      ValueWeightSingleton rhs) {
    return Semiring().EqualTo(lhs.value_, rhs.value_);
  }

  friend constexpr bool operator!=(
      ValueWeightSingleton lhs,
      ValueWeightSingleton rhs) {
    return !Semiring().EqualTo(lhs.value_, rhs.value_);
  }

  friend constexpr bool ApproxEqual(
      ValueWeightSingleton lhs,
      ValueWeightSingleton rhs,
      float delta = fst::kDelta) {
    return Semiring().ApproxEqualTo(lhs.value_, rhs.value_, delta);
  }

  constexpr std::size_t Hash() const { return std::hash<ValueType>()(value_); }

  friend std::ostream &operator<<(
      std::ostream &strm,
      ValueWeightSingleton w) {
    return Semiring().Print(strm, w.value_);
  }

  std::istream &Read(std::istream &strm) {
    static_assert(std::is_pod<ValueType>::value, "value type should be POD");
    return strm.read(reinterpret_cast<char *>(&value_), sizeof(value_));
  }

  std::ostream &Write(std::ostream &strm) const {
    return strm.write(reinterpret_cast<const char *>(&value_), sizeof(value_));
  }

  static const string &Type() {
    static const string type = Semiring().Name();
    return type;
  }

  static constexpr uint64 Properties() {
    return fst::kSemiring
        | (Semiring().Commutative() ? fst::kCommutative : 0)
        | (Semiring().Idempotent()  ? fst::kIdempotent  : 0);
  }

 private:
  static ValueWeightSingleton DivideCommutative(
      ValueWeightSingleton lhs,
      ValueWeightSingleton rhs) {
    lhs.value_ = Semiring().OpDivide(lhs.value_, rhs.value_);
    return lhs;
  }

  static ValueWeightSingleton DivideNoncommutative(
      ValueWeightSingleton lhs,
      ValueWeightSingleton rhs,
      fst::DivideType typ) {
    switch (typ) {
      case fst::DIVIDE_LEFT: {
        auto recip = Semiring().Reciprocal(rhs.value_);
        lhs.value_ = Semiring().OpTimes(recip, lhs.value_);
        break;
      }
      case fst::DIVIDE_RIGHT: {
        auto recip = Semiring().Reciprocal(rhs.value_);
        lhs.value_ = Semiring().OpTimes(lhs.value_, recip);
        break;
      }
      case fst::DIVIDE_ANY:
        FSTERROR() << "Only explicit left or right division is defined "
                   << "for the noncommutative " << Type() << " semiring";
        lhs.value_ = Semiring().NoWeight();
        break;
    }
    return lhs;
  }

  ValueType value_;
};

}  // namespace festus

#endif  // FESTUS_VALUE_WEIGHT_SINGLETON_H__
