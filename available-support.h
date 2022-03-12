
////////////////////////////////////////////////////////////////////////////////

#ifndef __AVAILABLE_SUPPORT_H__
#define __AVAILABLE_SUPPORT_H__

#include <string>
#include <vector>

#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/BitVector.h"

#include <sstream>

// DEBUG mode
#undef DEBUG
//#define DEBUG 1

#ifdef DEBUG
#define DBG(a) a
#else
#define DBG(a)
#endif

#define WIDTH 60

namespace llvm {
    std::string getShortValueName(Value * v);

    class Expression {
    public:
      Value * v1;
      Value * v2;
      Instruction::BinaryOps op;
      Expression (Instruction * I);
      bool operator== (const Expression &e2) const;
      bool operator< (const Expression &e2) const;
      std::string toString() const;
    };

    void printSet(std::vector<Expression> * x);

      // Pretty printing utility functions

    void printBitVector(BitVector& b);

    //void printResult(DataFlowResult& output);

    std::string printValue(Value* v);

    std::string formatSet(std::vector<void*>& domain, BitVector& on, int mode);
}

#endif
