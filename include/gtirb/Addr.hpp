#ifndef GTIRB_ADDR_H
#define GTIRB_ADDR_H

#include <gtirb/Export.hpp>
#include <cstddef>
#include <cstdint>

namespace gtirb {
/// A special class to store an Effective Address. It is a thin wrapper around
/// a uint64_t for 64-bit address storage. An Addr cannot store a relative
/// address as it cannot contain a negative number.
class GTIRB_EXPORT_API Addr {
  uint64_t Address{0};

public:
  constexpr Addr() = default;
  constexpr explicit Addr(uint64_t X) : Address(X) {}

  explicit operator uint64_t() const { return Address; }

  // Addr + integral should result in an Addr.
  friend Addr operator+(const Addr& A, uint64_t Offset) {
    return Addr(A.Address + Offset);
  }
  // Addr - integral should result in an Addr.
  friend Addr operator-(const Addr& A, uint64_t Offset) {
    return Addr(A.Address - Offset);
  }

  // Addr - Addr should result in a int64_t.
  friend int64_t operator-(const Addr& A, const Addr& B) {
    return static_cast<int64_t>(A.Address - B.Address);
  }

  // Equality and relational operators.
  friend bool operator==(const Addr& LHS, const Addr& RHS) {
    return LHS.Address == RHS.Address;
  }
  friend bool operator!=(const Addr& LHS, const Addr& RHS) {
    return !operator==(LHS, RHS);
  }
  friend bool operator<(const Addr& LHS, const Addr& RHS) {
    return LHS.Address < RHS.Address;
  }
  friend bool operator>(const Addr& LHS, const Addr& RHS) {
    return operator<(RHS, LHS);
  }
  friend bool operator<=(const Addr& LHS, const Addr& RHS) {
    return !operator<(RHS, LHS);
  }
  friend bool operator>=(const Addr& LHS, const Addr& RHS) {
    return !operator<(LHS, RHS);
  }
};

/// Exclusive limit of object's address range.
///
/// Object can be any type which specifies a range of addresses via
/// getAddress() and getSize() methods (e.g. DataObject).
template <typename T> Addr addressLimit(const T& Object) {
  return Object.getAddress() + Object.getSize();
}

/// Does object contain a given Addr?
///
/// Object can be any type which specifies a range of addresses via
/// getAddress() and getSize() methods (e.g. DataObject).
template <typename T> bool containsAddr(const T& Object, Addr Ea) {
  return Object.getAddress() <= Ea && addressLimit(Object) > Ea;
}
} // namespace gtirb

#endif // GTIRB_ADDR_H