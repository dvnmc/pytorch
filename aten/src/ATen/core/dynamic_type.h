#pragma once

#include <memory>

#include <ATen/core/class_type.h>
#include <ATen/core/jit_type_base.h>
#include <c10/util/Optional.h>

using DynamicTypeBits = std::uint32_t;
#define DYNAMIC_TYPE_BIT(x) (1u << x)

constexpr DynamicTypeBits kDynamicCovariantTypeBit = DYNAMIC_TYPE_BIT(31);
constexpr DynamicTypeBits kDynamicAnyTypeBit = DYNAMIC_TYPE_BIT(30);
constexpr DynamicTypeBits kDynamicSingletonTypeBit = DYNAMIC_TYPE_BIT(29);

constexpr DynamicTypeBits kDynamicNoneTypeBit = DYNAMIC_TYPE_BIT(1);
constexpr DynamicTypeBits kDynamicIntTypeBit = DYNAMIC_TYPE_BIT(3);
constexpr DynamicTypeBits kDynamicFloatTypeBit = DYNAMIC_TYPE_BIT(4);
constexpr DynamicTypeBits kDynamicComplexTypeBit = DYNAMIC_TYPE_BIT(5);
constexpr DynamicTypeBits kDynamicListTypeBit = DYNAMIC_TYPE_BIT(7);
constexpr DynamicTypeBits kDynamicTupleTypeBit = DYNAMIC_TYPE_BIT(8);


#define FORALL_DYNAMIC_TYPES(_)                                              \
  _(Tensor, DYNAMIC_TYPE_BIT(0))                                             \
  _(None, kDynamicNoneTypeBit)                                               \
  _(Bool, DYNAMIC_TYPE_BIT(2))                                               \
  _(Int, kDynamicIntTypeBit)                                                 \
  _(Float, kDynamicFloatTypeBit)                                             \
  _(Complex, kDynamicComplexTypeBit)                                         \
  _(Number,                                                                  \
    (kDynamicIntTypeBit | kDynamicFloatTypeBit | kDynamicComplexTypeBit))    \
  _(String, DYNAMIC_TYPE_BIT(6))                                             \
  _(List, kDynamicListTypeBit)                                               \
  _(Tuple, (kDynamicTupleTypeBit | kDynamicCovariantTypeBit))                \
  _(Dict, DYNAMIC_TYPE_BIT(9))                                               \
  _(Class, DYNAMIC_TYPE_BIT(10))                                             \
  _(Optional,                                                                \
    (DYNAMIC_TYPE_BIT(11) | kDynamicNoneTypeBit | kDynamicCovariantTypeBit)) \
  _(AnyList, (kDynamicListTypeBit | kDynamicAnyTypeBit))                     \
  _(AnyTuple,                                                                \
    (kDynamicTupleTypeBit | kDynamicCovariantTypeBit | kDynamicAnyTypeBit))  \
  _(Any, 0xffffffff)

namespace c10 {

struct FunctionSchema;
struct LabeledDynamicType;
class DynamicType;
using DynamicTypePtr = std::shared_ptr<DynamicType>;

// Low dependency jit type with minimal subtyping and structucing support,
// designed for embedded cases.
class DynamicType : public Type {
  using ClassTypePtr = std::shared_ptr<const c10::ClassType>;

 public:
  ~DynamicType() override;

  struct Arguments {
    Arguments() = default;
    Arguments(c10::ArrayRef<TypePtr>);
    Arguments(const c10::FunctionSchema&, c10::ArrayRef<TypePtr>);
    std::vector<LabeledDynamicType> elems;
  };

  enum class Tag : DynamicTypeBits {
#define DYNAMIC_TYPE_ITEM(NAME, VAL) NAME = VAL,
    FORALL_DYNAMIC_TYPES(DYNAMIC_TYPE_ITEM)
#undef DYNAMIC_TYPE_ITEM
        Singleton = kDynamicSingletonTypeBit,
  };

  bool operator==(const Type& rhs) const override;
  bool isSubtypeOfExt(const Type& rhs, std::ostream* why_not) const override;
  std::string str() const override {
    return "Dynamic";
  }
  static const TypeKind Kind = TypeKind::DynamicType;
  static DynamicTypePtr create(Type& ty);

  explicit DynamicType(Tag, Arguments);

 private:
  friend struct Type;
  static std::shared_ptr<const DynamicType> create(const Type& ty);
  DynamicType(const Type& other);
  bool equals(const DynamicType& other) const;

  template <typename F>
  bool compare(const DynamicType& other, F&& f) const {
    if (arguments_.elems.size() != other.arguments_.elems.size()) {
      return false;
    }
    for (size_t i = 0; i < arguments_.elems.size(); i++) {
      if (!f(arguments_.elems[i], other.arguments_.elems[i])) {
        return false;
      }
    }
    return true;
  }

  Tag tag_;
  union {
    Arguments arguments_;
    ClassTypePtr class_;
    TypeKind typeKind_;
  };
};

struct LabeledDynamicType {
  c10::optional<std::string> label;
  DynamicTypePtr ty;
  explicit LabeledDynamicType(DynamicTypePtr t) : ty(std::move(t)) {}

  bool equals(const LabeledDynamicType& other) const;
  bool isSubtypeOf(const LabeledDynamicType& other) const;
};

} // namespace c10
