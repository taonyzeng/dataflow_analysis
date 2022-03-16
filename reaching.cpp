// ECE/CS 5544 S22 Assignment 2: reaching.cpp
// Group: TAO ZENG

////////////////////////////////////////////////////////////////////////////////

#include "dataflow.h"

using namespace llvm;

namespace {


    // reaching definition Analysis class
    class ReachingAnalysis : public DataFlow {
        public:

            ReachingAnalysis() : DataFlow(Direction::INVALID_DIRECTION, MeetOp::INVALID_MEETOP) {   }
            ReachingAnalysis(Direction direction, MeetOp meet_op) : DataFlow(direction, meet_op) { }

        protected:
            TransferOutput transferFn(BitVector input,  std::vector<void*> domain, std::unordered_map<void*, int> domainToIndex, BasicBlock* block){

                TransferOutput transferOutput;

                //First, calculate the set of downwards exposed definition generations and the set of killed definitions in this block
                int domainSize = domainToIndex.size();
                BitVector GenSet(domainSize);
                BitVector KillSet(domainSize);

                for (auto inst = block->begin(); inst != block->end(); ++inst) {

                    auto currDefIter = domainToIndex.find(&*inst);

                    if (currDefIter != domainToIndex.end()) {

                        //Kill prior definitions for the same variable (including those in this block's gen set)
                        for (void* item : domain) {
                            std::string prevDefStr = valueToDefinitionVarStr((Value*)item);
                            std::string currDefStr = valueToDefinitionVarStr((Value*)currDefIter->first);
                            if (prevDefStr == currDefStr) {
                                KillSet.set(domainToIndex[item]);
                                GenSet.reset(domainToIndex[item]);
                            }
                        }
                        //Add this new definition to gen set (note that we might later remove it if another def in this block kills it)
                        GenSet.set(currDefIter->second);
                    }
                }

                //update the genSet and killSet for each block.
                if(genSet.find(block) == genSet.end()){
                    genSet[block] = GenSet;
                }

                if(killSet.find(block) == killSet.end()){
                    killSet[block] = KillSet;
                }

                //Then, apply transfer function: Y = GenSet \union (X - KillSet)
                transferOutput.element = KillSet;
                    // Complement of KillSet
                transferOutput.element.flip();
                    // input - KillSet = input INTERSECTION Complement of KillSet
                transferOutput.element &= input;
                transferOutput.element |= GenSet;

                return transferOutput;
            }
    };


    class Reaching : public FunctionPass {
    public:
        static char ID;

        Reaching() : FunctionPass(ID) {
        }

        virtual void getAnalysisUsage(AnalysisUsage& AU) const {
            AU.setPreservesAll();
        }

    private:
        virtual bool runOnFunction(Function &F) {
            // Print Information
            // std::string function_name = F.getName();
            DBG(outs() << "FUNCTION :: " << F.getName()  << "\n");

            // Setup the pass
            std::vector<void*> domain;

            // Compute domain for function
            // use functions arguments to initialize
            for (auto arg = F.arg_begin(); arg != F.arg_end(); ++arg) {
                domain.push_back(&*arg);
            }
              
            // add remaining instruction
            for (auto I = inst_begin(F); I != inst_end(F); ++I) {
                if (Value* val = dyn_cast<Value> (&*I)) {
                    if (!valueToDefinitionStr(val).empty()){
                    // if (getShortValueName(val).length() > 0) {
                        if (std::find(domain.begin(), domain.end(), val) == domain.end()) {
                            domain.push_back(val);
                        }
                    }
                }
            }

            // For reaching definition, both are empty sets
            BitVector boundaryCond(domain.size(), false);
            //Set the initial boundary dataflow value to be the set of input argument definitions for this function
            for (int i = 0; i < domain.size(); i++){
                if (isa<Argument>((Value*)domain[i]))
                    boundaryCond.set(i);
            }

            //Set interior initial dataflow values to be empty sets
            BitVector initCond(domain.size(), false);

            //Initial the ReachingAnalysis.
            ReachingAnalysis pass(Direction::FORWARD, MeetOp::UNION);

            // Apply pass
            DataFlowResult output = pass.run(F, domain, boundaryCond, initCond);

            //output the running result for each basic block.
            for (Function::iterator BI = F.begin(), BE = F.end(); BI != BE; ++BI) {
                BasicBlock* block = &*BI;

                // PRINTING RESULTS
                outs() << "\033[1;31m[BB Name]:\033[0m"<< block->getName() << "\n";
                outs() << "\033[1;32m[IN]:  \033[0m bitvector = "<< formatBitVector(output.result[block].in) << ", set = "<< setToStr(domain, output.result[block].in) << "\n";
                outs() << "\033[1;32m[OUT]: \033[0m bitvector = "<< formatBitVector(output.result[block].out) << ", set = "<< setToStr(domain, output.result[block].out) << "\n";
                outs() << "\033[1;32m[Gen]: \033[0m bitvector = "<< formatBitVector(pass.genSet[block]) << ", set = "<< setToStr(domain, pass.genSet[block]) << "\n";
                outs() << "\033[1;32m[Kill]:\033[0m bitvector = "<< formatBitVector(pass.killSet[block]) << ", set = "<< setToStr(domain, pass.killSet[block]) << "\n\n";
            }

            return false;
        }

    };

    char Reaching::ID = 0;
    RegisterPass<Reaching> X("reaching", "ECE/CS 5544 Reaching");
}
