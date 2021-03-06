//===- Symbol.hpp -----------------------------------------------*- C++ -*-===//
//
//  Copyright (C) 2018 GrammaTech, Inc.
//
//  This code is licensed under the MIT license. See the LICENSE file in the
//  project root for license terms.
//
//  This project is sponsored by the Office of Naval Research, One Liberty
//  Center, 875 N. Randolph Street, Arlington, VA 22203 under contract #
//  N68335-17-C-0700.  The content of the information does not necessarily
//  reflect the position or policy of the Government and no official
//  endorsement should be inferred.
//
//===----------------------------------------------------------------------===//
#ifndef GTIRB_SYMBOL_H
#define GTIRB_SYMBOL_H

#include <gtirb/Addr.hpp>
#include <gtirb/Block.hpp>
#include <gtirb/DataObject.hpp>
#include <gtirb/Node.hpp>
#include <proto/Symbol.pb.h>
#include <functional>
#include <optional>
#include <type_traits>

/// \file Symbol.hpp
/// \brief Class gtirb::Symbol.

namespace gtirb {
/// \class Symbol
///
/// \brief Represents a Symbol, which maps a name to an object in the IR.
class GTIRB_EXPORT_API Symbol : public Node {
  template <typename... Ts> struct TypeList {};

  // SFINAE overload handling a Callable with a void return type.
  template <typename Callable, typename Ty, typename CommonRetTy>
  auto visit_impl_help(Callable&& Visitor) const
      -> std::enable_if_t<std::is_void_v<CommonRetTy>, bool> {
    static_assert(std::is_invocable_v<Callable, Ty*>,
                  "Visitor must contain an overloaded function call operator "
                  "for each of the types in supported_types");
    if (Node* const* Ptr = std::get_if<Node*>(&Payload)) {
      if (Ty* Obj = dyn_cast_or_null<Ty>(*Ptr)) {
        std::invoke(Visitor, Obj);
        return true;
      }
    }
    return false;
  }

  // SFINAE overload handling a Callable with a non-void return type.
  template <typename Callable, typename Ty, typename CommonRetTy>
  bool
  visit_impl_help(Callable&& Visitor,
                  std::enable_if_t<!std::is_void_v<CommonRetTy>,
                                   std::optional<CommonRetTy>>& Ret) const {
    static_assert(std::is_invocable_v<Callable, Ty*>,
                  "Visitor must contain an overloaded function call operator "
                  "for each of the types in supported_types");
    if (Node* const* Ptr = std::get_if<Node*>(&Payload)) {
      if (Ty* Obj = dyn_cast_or_null<Ty>(*Ptr)) {
        Ret = std::invoke(Visitor, Obj);
        return true;
      }
    }
    return false;
  }

  // Helper type traits used to determine whether all of the Callable object's
  // return types agree. Note that the return types do not need to be the same,
  // but do need to be implicitly convertible to the same common type. e.g.,
  // this class is a valid Callable.
  //   struct Visitor {
  //     int operator()(Block*);
  //     long operator()(DataObject*);
  //  };
  template <typename AlwaysVoid, typename Callable,
            template <typename...> typename TypeList, typename... Types>
  struct common_return_type_impl : std::false_type {};

  template <typename Callable, template <typename...> typename TypeList,
            typename... Types>
  struct common_return_type_impl<
      std::void_t<
          std::common_type_t<std::invoke_result_t<Callable, Types*>...>>,
      Callable, TypeList, Types...> : std::true_type {
    using type = std::common_type_t<std::invoke_result_t<Callable, Types*>...>;
  };

  template <typename Callable, template <typename...> typename TypeList,
            typename... Types>
  using common_return_type_t =
      typename common_return_type_impl<void, Callable, TypeList,
                                       Types...>::type;

  template <typename Callable, template <typename...> typename TypeList,
            typename... Types>
  static constexpr bool common_return_type_v =
      common_return_type_impl<void, Callable, TypeList, Types...>::value;

  // Helper function to unpack all of the types in the type list and attempt to
  // visit Callable once per type. Verifies that the Callable objects all share
  // a compatible return type.
  //
  // SFINAE overload handling a Callable where there is a common, non-void
  // return type. Returns a std::optional<common_type>.
  template <typename Callable, template <typename...> typename TypeList,
            typename... Types>
  auto visit_impl(Callable&& Visitor, TypeList<Types...>) const
      -> std::enable_if_t<
          !std::is_void_v<common_return_type_t<Callable, TypeList, Types...>>,
          std::optional<common_return_type_t<Callable, TypeList, Types...>>> {
    // If this assertion fails, the return values from the Callable object are
    // not compatible enough. This can happen if they return incompatible types
    // or if there is not an overload for each referent type.
    static_assert(common_return_type_v<Callable, TypeList, Types...>,
                  "incompatible return types for the Callable object");
    // Instantiate a call to the Visitor once for each of the listed types, but
    // only issue the call at runtime if the Referent can be dynamically cast to
    // the given type. In this way, the Visitor needs to be able to handle any
    // of the supported types, but will only be called once for the concrete
    // type of the Referent.
    //
    // If you get an error about there being no matching overloaded function for
    // the call to visit_impl_help, that is most likely because there are one
    // or more overloads missing for each referent type.
    std::optional<common_return_type_t<Callable, TypeList, Types...>> Res;
    (... ||
     visit_impl_help<Callable, Types, typename decltype(Res)::value_type>(
         std::forward<Callable>(Visitor), Res));
    return Res;
  }

  // SFINAE overload handling a Callable where every return type is void.
  // Returns void.
  template <typename Callable, template <typename...> typename TypeList,
            typename... Types>
  auto visit_impl(Callable&& Visitor, TypeList<Types...>) const
      -> std::enable_if_t<
          std::is_void_v<common_return_type_t<Callable, TypeList, Types...>>,
          void> {
    // Call each of the overloads on Callable, but there is no value to be
    // returned from any of the calls.
    (... ||
     visit_impl_help<Callable, Types, void>(std::forward<Callable>(Visitor)));
  }

  // Helper function that determines whether the passed NodeTy is the same as
  // any of the types in the Types... list.
  template <typename NodeTy, template <typename...> typename TypeList,
            typename... Types>
  static constexpr bool is_supported_type_impl(TypeList<Types...>) {
    return (... || std::is_same_v<NodeTy, Types>);
  }

public:
  // Helper function that determines whether the passed NodeTy is one of the
  // supported referent types.
  template <typename NodeTy> static constexpr bool is_supported_type() {
    return is_supported_type_impl<NodeTy>(
        std::decay_t<supported_referent_types>{});
  }

  /// \brief The list of supported referent types.
  using supported_referent_types = TypeList<Block, DataObject>;

  /// \brief Visits the symbol's referent, if one is present, by concrete
  /// referent type.
  ///
  /// \tparam Callable  A callable function type. This type must be able to be
  /// called with a pointer to all of the types listed in \ref
  /// supported_referent_types. All overloaded functions must have a common,
  /// compatible return type.
  ///
  /// \param Visitor  A callable object that will be called with a nonnull
  /// symbol referent.
  ///
  /// \return The common type of each of the return types in the \p Callable
  /// overload set. Notionally returns:
  /// std::common_type_t<Overload1(Ty1), Overload2(Ty2), ...>
  /// which can be void.
  ///
  /// For example:
  ///
  /// \code
  /// struct Visitor {
  ///   int operator()(Block*) { return 0; }
  ///   long operator()(DataObject*) { return 1; }
  /// };
  ///
  /// Context Ctx;
  /// Symbol* SymB = Symbol::Create(Ctx, Block::Create(Ctx), "");
  /// Symbol* SymD = Symbol::Create(Ctx, DataObject::Create(Ctx), "");
  /// Symbol* SymX = Symbol::Create(Ctx, Addr(42), "");
  /// Symbol* SymN = Symbol::Create(Ctx);
  ///
  /// SymB->visit(Visitor{}); // Will call Visitor::operator()(Block*);
  /// SymD->visit(Visitor{}); // Will call Visitor::operator()(DataObject*);
  /// SymX->visit(Visitor{}); // Will not call any overload
  /// SymN->visit(Visitor{}); // Will not call any overload
  /// \endcode
  template <typename Callable> auto visit(Callable&& Visitor) const {
    return visit_impl(std::forward<Callable>(Visitor),
                      std::decay_t<supported_referent_types>{});
  }

  /// \enum StorageKind
  ///
  /// \brief Indicates how a symbol is stored and where it is accessible.
  enum class StorageKind : uint8_t {
    Undefined = proto::Storage_Undefined, ///< Symbol is not defined.
    Normal = proto::Storage_Normal,       ///< Accessible outside the Module.
    Static = proto::Storage_Static,       ///< Accessible within the Module.
    Extern = proto::Storage_Extern,       ///< Defined outside the Module.
    Local = proto::Storage_Local ///< Stored locally in the context of a
                                 ///< function's activation frame.
  };

  /// \brief Create a Symbol object in its default state.
  ///
  /// \param C  The Context in which this object will be held.
  ///
  /// \return The newly created object.
  static Symbol* Create(Context& C) { return C.Create<Symbol>(C); }

  /// \brief Create a Symbol object.
  ///
  /// \param C    The Context in which this object will be held.
  /// \param Name The name of the symbol.
  ///
  /// \return The newly created object.
  static Symbol* Create(Context& C, const std::string& Name) {
    return C.Create<Symbol>(C, Name);
  }

  /// \brief Create a Symbol object.
  ///
  /// \param C  The Context in which this object will be held.
  /// \param X  The address of the symbol.
  /// \param Name The name of the symbol.
  /// \param Kind The storage kind the symbol has; defaults to
  /// StorageKind::Extern
  ///
  /// \return The newly created object.
  static Symbol* Create(Context& C, Addr X, const std::string& Name,
                        StorageKind Kind = StorageKind::Extern) {
    return C.Create<Symbol>(C, X, Name, Kind);
  }

  /// \brief Create a Symbol object.
  ///
  /// \param C  The Context in which this object will be held.
  /// \param Referent The DataObject this symbol refers to.
  /// \param Name The name of the symbol.
  /// \param Kind The storage kind the symbol has; defaults to
  /// StorageKind::Extern
  ///
  /// \return The newly created object.
  template <typename NodeTy>
  static Symbol* Create(Context& C, NodeTy* Referent, const std::string& Name,
                        StorageKind Kind = StorageKind::Extern) {
    static_assert(is_supported_type<NodeTy>(), "unsupported referent type");
    return C.Create<Symbol>(C, Referent, Name, Kind);
  }

  /// \brief Get the effective address.
  ///
  /// \return The effective address.
  std::optional<Addr> getAddress() const {
    return std::visit(
        [](const auto& arg) {
          using T = std::decay_t<decltype(arg)>;
          if constexpr (std::is_same_v<T, std::monostate>)
            return std::optional<Addr>{};
          else if constexpr (std::is_same_v<T, Addr>)
            return std::make_optional(arg);
          else if constexpr (std::is_same_v<T, Node*>) {
            if (Block* b = dyn_cast_or_null<Block>(arg))
              return std::make_optional(b->getAddress());
            else if (DataObject* d = dyn_cast_or_null<DataObject>(arg))
              return std::make_optional(d->getAddress());
            else
              assert(arg == nullptr && "unsupported referent type");
            return std::optional<Addr>{};
          } else {
            static_assert(
                // Assert condition must depend on T, but will always be false.
                std::bool_constant<!std::is_same_v<T, T>>::value,
                "unsupported symbol payload type");
          }
        },
        Payload);
    if (const Addr* value = std::get_if<Addr>(&Payload))
      return *value;
    return std::nullopt;
  }

  /// \brief Get the name.
  ///
  /// \return The name.
  const std::string& getName() const { return Name; }

  /// \brief Get the referent to which this symbol refers.
  ///
  /// \tparam NodeTy A Node type of a supported referent.
  ///
  /// \return The data, dynamically typed as the given \p NodeTy, or
  /// null if there is no referent of that type.
  template <typename NodeTy> NodeTy* getReferent() {
    if (Node* const* Ptr = std::get_if<Node*>(&Payload))
      return dyn_cast_or_null<NodeTy>(*Ptr);
    return nullptr;
  }

  /// \brief Get the referent to which this symbol refers.
  ///
  /// \tparam NodeTy A Node type of a supported referent.
  ///
  /// \return The data, dynamically typed as the given \p NodeTy, or
  /// null if there is no referent of that type.
  template <typename NodeTy> const NodeTy* getReferent() const {
    if (Node* const* Ptr = std::get_if<Node*>(&Payload))
      return dyn_cast_or_null<NodeTy>(*Ptr);
    return nullptr;
  }

  /// \brief Check if this symbol has a referent.
  ///
  /// \return \p true if the symbol has a referent, \p false otherwise.
  bool hasReferent() const { return std::holds_alternative<Node*>(Payload); }

  /// \brief Set the storage kind.
  ///
  /// \param X The storage kind to use.
  ///
  /// \return void
  void setStorageKind(Symbol::StorageKind X) { Storage = X; }

  /// \brief Get the storage kind.
  ///
  /// \return The storage kind.
  Symbol::StorageKind getStorageKind() const { return Storage; }

  /// \brief The protobuf message type used for serializing Symbol.
  using MessageType = proto::Symbol;

  /// \brief Serialize into a protobuf message.
  ///
  /// \param[out] Message   Serialize into this message.
  ///
  /// \return void
  void toProtobuf(MessageType* Message) const;

  /// \brief Construct a Symbol from a protobuf message.
  ///
  /// \param C   The Context in which the deserialized Symbol will be held.
  /// \param Message  The protobuf message from which to deserialize.
  ///
  /// \return The deserialized Symbol object, or null on failure.
  static Symbol* fromProtobuf(Context& C, const MessageType& Message);

  /// \cond INTERNAL
  static bool classof(const Node* N) { return N->getKind() == Kind::Symbol; }
  /// \endcond

private:
  Symbol(Context& C) : Node(C, Kind::Symbol) {}
  Symbol(Context& C, const std::string& N, StorageKind SK = StorageKind::Extern)
      : Node(C, Kind::Symbol), Payload(), Name(N), Storage(SK) {}
  Symbol(Context& C, Addr X, const std::string& N,
         StorageKind SK = StorageKind::Extern)
      : Node(C, Kind::Symbol), Payload(X), Name(N), Storage(SK) {}
  template <typename NodeTy>
  Symbol(Context& C, NodeTy* R, const std::string& N,
         StorageKind SK = StorageKind::Extern)
      : Node(C, Kind::Symbol), Payload(R), Name(N), Storage(SK) {}

  std::variant<std::monostate, Addr, Node*> Payload;
  std::string Name;
  Symbol::StorageKind Storage{StorageKind::Extern};

  friend class Context; // Allow Context to construct Symbols.

  // Allow these methods to update Symbol contents.
  friend void renameSymbol(Module& M, Symbol& S, const std::string& N);
  friend void setSymbolAddress(Module& M, Symbol& S, Addr A);
  template <typename NodeTy>
  friend std::enable_if_t<Symbol::is_supported_type<NodeTy>()>
  setReferent(Module& M, Symbol& S, NodeTy* N);
};
} // namespace gtirb

#endif // GTIRB_SYMBOL_H
