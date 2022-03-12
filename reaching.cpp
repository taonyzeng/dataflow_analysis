// ECE/CS 5544 S22 Assignment 2: reaching.cpp
// Group:

////////////////////////////////////////////////////////////////////////////////

#include "dataflow.h"

using namespace llvm;

namespace {


    // Liveness Analysis class
    class ReachingAnalysis : public DataFlow {
        public:

            ReachingAnalysis() : DataFlow(Direction::INVALID_DIRECTION, MeetOp::INVALID_MEETOP) {   }
            ReachingAnalysis(Direction direction, MeetOp meet_op) : DataFlow(direction, meet_op) { }

        protected:
            TransferOutput transferFn(BitVector input,  std::vector<void*> domain, std::map<void*, int> domainToIndex, BasicBlock* block)
            {

                TransferOutput transferOutput;

                int domainSize = domainToIndex.size();
                BitVector GenSet(domainSize);
                BitVector KillSet(domainSize);

                for (auto inst = block->begin(); inst != block->end(); ++inst) {
                    std::string val = "";
                    if (isa<Instruction> (&*inst)) {
                        val = getShortValueName(&*inst);
                    }

                    if (auto load_ins = dyn_cast<StoreInst>(&*inst)) {
                        val = getShortValueName(load_ins->getPointerOperand());
                    }

                    if (val.length() == 0) {
                        continue;
                    }

                    // if input has be re-assigned, kill it
                    for(int i = 0; i < domain.size(); i++){
                        if(getShortValueName((Value *) domain[i]) == val){
                            KillSet.set(i);
                        }
                    }

                    // some values are killed in the same Basic Block, so sad
                    for (int i = 0; i < GenSet.size(); i++) {
                        if (GenSet[i] && getShortValueName((Value *) domain[i]) == val) {
                            GenSet.reset(i);
                        }
                    }

                    std::map<void*, int>::iterator iter = domainToIndex.find((void*)(&*inst));
                    if (iter != domainToIndex.end())
                        GenSet.set((*iter).second);
                }



                //update the genSet and killSet for each block.
                if(genSet.find(block) == genSet.end()){
                    genSet[block] = GenSet;
                }

                if(killSet.find(block) == killSet.end()){
                    killSet[block] = KillSet;
                }

                //printBitVector(GenSet);
                //printBitVector(KillSet);
                // Transfer function = GenSet U (input - KillSet)

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
                    if (getShortValueName(val).length() > 0) {
                        if (std::find(domain.begin(), domain.end(), val) == domain.end()) {
                            domain.push_back(val);
                        }
                    }
                }
            }

            // For reaching definition, both are empty sets
            BitVector boundaryCond(domain.size(), false);
            BitVector initCond(domain.size(), false);

            // The pass
            ReachingAnalysis pass(Direction::FORWARD, MeetOp::UNION);

            // Apply pass
            DataFlowResult output = pass.run(F, domain, boundaryCond, initCond);

            for (Function::iterator BI = F.begin(), BE = F.end(); BI != BE; ++BI) {
                BasicBlock* block = &*BI;

                // PRINTING RESULTS
                outs() << "BB Name: "<< block->getName() << "\n";
                outs() << "IN: " << formatSet(domain, output.result[block].in, 0) << "\n";
                outs() << "OUT: "<< formatSet(domain, output.result[block].out, 0) << "\n";
                outs() << "Gen: "<< formatSet(domain, pass.genSet[block], 0) << "\n";
                outs() << "Kill: "<< formatSet(domain, pass.killSet[block], 0) << "\n\n";
            }

            return false;
        }

    };

    char Reaching::ID = 0;
    RegisterPass<Reaching> X("reaching", "ECE/CS 5544 Reaching");
}
