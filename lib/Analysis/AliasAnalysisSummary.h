//=====- CFLSummary.h - Abstract stratified sets implementation. --------=====//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
/// This file defines various utility types and functions useful to
/// summary-based alias analysis.
///
/// Summary-based analysis, also known as bottom-up analysis, is a style of
/// interprocedrual static analysis that tries to analyze the callees before the
/// callers get analyzed. The key idea of summary-based analysis is to first
/// process each function indepedently, outline its behavior in a condensed
/// summary, and then instantiate the summary at the callsite when the said
/// function is called elsewhere. This is often in contrast to another style
/// called top-down analysis, in which callers are always analyzed first before
/// the callees.
///
/// In a summary-based analysis, functions must be examined independently and
/// out-of-context. We have no information on the state of the memory, the
/// arguments, the global values, and anything else external to the function. To
/// carry out the analysis conservative assumptions have to be made about those
/// external states. In exchange for the potential loss of precision, the
/// summary we obtain this way is highly reusable, which makes the analysis
/// easier to scale to large programs even if carried out context-sensitively.
///
/// Currently, all CFL-based alias analyses adopt the summary-based approach
/// and therefore heavily rely on this header.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_ANALYSIS_ALIASANALYSISSUMMARY_H
#define LLVM_ANALYSIS_ALIASANALYSISSUMMARY_H

#include "llvm/ADT/Optional.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/CallSite.h"
#include <bitset>

namespace llvm {
namespace cflaa {

//===----------------------------------------------------------------------===//
// AliasAttr related stuffs
//===----------------------------------------------------------------------===//

/// The number of attributes that AliasAttr should contain. Attributes are
/// described below, and 32 was an arbitrary choice because it fits nicely in 32
/// bits (because we use a bitset for AliasAttr).
static const unsigned NumAliasAttrs = 32;

/// These are attributes that an alias analysis can use to mark certain special
/// properties of a given pointer. Refer to the related functions below to see
/// what kinds of attributes are currently defined.
typedef std::bitset<NumAliasAttrs> AliasAttrs;

/// Attr represent whether the said pointer comes from an unknown source
/// (such as opaque memory or an integer cast).
AliasAttrs getAttrNone();

/// AttrUnknown represent whether the said pointer comes from a source not known
/// to alias analyses (such as opaque memory or an integer cast).
AliasAttrs getAttrUnknown();
bool hasUnknownAttr(AliasAttrs);

/// AttrCaller represent whether the said pointer comes from a source not known
/// to the current function but known to the caller. Values pointed to by the
/// arguments of the current function have this attribute set
AliasAttrs getAttrCaller();
bool hasCallerAttr(AliasAttrs);
bool hasUnknownOrCallerAttr(AliasAttrs);

/// AttrEscaped represent whether the said pointer comes from a known source but
/// escapes to the unknown world (e.g. casted to an integer, or passed as an
/// argument to opaque function). Unlike non-escaped pointers, escaped ones may
/// alias pointers coming from unknown sources.
AliasAttrs getAttrEscaped();
bool hasEscapedAttr(AliasAttrs);

/// AttrGlobal represent whether the said pointer is a global value.
/// AttrArg represent whether the said pointer is an argument, and if so, what
/// index the argument has.
AliasAttrs getGlobalOrArgAttrFromValue(const Value &);
bool isGlobalOrArgAttr(AliasAttrs);

/// Given an AliasAttrs, return a new AliasAttrs that only contains attributes
/// meaningful to the caller. This function is primarily used for
/// interprocedural analysis
/// Currently, externally visible AliasAttrs include AttrUnknown, AttrGlobal,
/// and AttrEscaped
AliasAttrs getExternallyVisibleAttrs(AliasAttrs);

//===----------------------------------------------------------------------===//
// Function summary related stuffs
//===----------------------------------------------------------------------===//

/// The maximum number of arguments we can put into a summary.
LLVM_CONSTEXPR static unsigned MaxSupportedArgsInSummary = 50;

/// We use InterfaceValue to describe parameters/return value, as well as
/// potential memory locations that are pointed to by parameters/return value,
/// of a function.
/// Index is an integer which represents a single parameter or a return value.
/// When the index is 0, it refers to the return value. Non-zero index i refers
/// to the i-th parameter.
/// DerefLevel indicates the number of dereferences one must perform on the
/// parameter/return value to get this InterfaceValue.
struct InterfaceValue {
  unsigned Index;
  unsigned DerefLevel;
};

inline bool operator==(InterfaceValue lhs, InterfaceValue rhs) {
  return lhs.Index == rhs.Index && lhs.DerefLevel == rhs.DerefLevel;
}
inline bool operator!=(InterfaceValue lhs, InterfaceValue rhs) {
  return !(lhs == rhs);
}

/// We use ExternalRelation to describe an externally visible aliasing relations
/// between parameters/return value of a function.
struct ExternalRelation {
  InterfaceValue From, To;
};

/// We use ExternalAttribute to describe an externally visible AliasAttrs
/// for parameters/return value.
struct ExternalAttribute {
  InterfaceValue IValue;
  AliasAttrs Attr;
};

/// AliasSummary is just a collection of ExternalRelation and ExternalAttribute
struct AliasSummary {
  // RetParamRelations is a collection of ExternalRelations.
  SmallVector<ExternalRelation, 8> RetParamRelations;

  // RetParamAttributes is a collection of ExternalAttributes.
  SmallVector<ExternalAttribute, 8> RetParamAttributes;
};

/// This is the result of instantiating InterfaceValue at a particular callsite
struct InstantiatedValue {
  Value *Val;
  unsigned DerefLevel;
};
Optional<InstantiatedValue> instantiateInterfaceValue(InterfaceValue, CallSite);

/// This is the result of instantiating ExternalRelation at a particular
/// callsite
struct InstantiatedRelation {
  InstantiatedValue From, To;
};
Optional<InstantiatedRelation> instantiateExternalRelation(ExternalRelation,
                                                           CallSite);

/// This is the result of instantiating ExternalAttribute at a particular
/// callsite
struct InstantiatedAttr {
  InstantiatedValue IValue;
  AliasAttrs Attr;
};
Optional<InstantiatedAttr> instantiateExternalAttribute(ExternalAttribute,
                                                        CallSite);
}
}

#endif
