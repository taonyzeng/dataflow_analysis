// ECE/CS 5544 S22 Assignment 2: liveness.cpp
// Group:

////////////////////////////////////////////////////////////////////////////////

#include "dataflow.h"

using namespace llvm;

namespace {

    // Liveness Analysis class
    class LivenessAnalysis : public DataFlow {

        public:

            LivenessAnalysis() : DataFlow(Direction::INVALID_DIRECTION, MeetOp::INVALID_MEETOP) {   }
            LivenessAnalysis(Direction direction, MeetOp meet_op) : DataFlow(direction, meet_op) { }

        protected:
            TransferOutput transferFn(BitVector input,  std::vector<void*> domain, std::unordered_map<void*, int> domainToIndex, BasicBlock* block){

                TransferOutput transferOutput;

                // Calculating the set of locally exposed uses and variables defined
                int domainSize = domainToIndex.size();
                BitVector defSet(domainSize);
                BitVector useSet(domainSize);
                for (BasicBlock::iterator insn = block->begin(); insn != block->end(); ++insn) {
                    // Locally exposed uses

                    // Phi nodes: add operands to the list we store in transferOutput
                    if (PHINode* phi_insn = dyn_cast<PHINode>(&*insn)) {
                        for (int ind = 0; ind < phi_insn->getNumIncomingValues(); ind++) {

                            Value* val = phi_insn->getIncomingValue(ind);
                            if (isa<Instruction>(val) || isa<Argument>(val)) {

                                BasicBlock* valBlock = phi_insn->getIncomingBlock(ind);
                                // neighborVals has no mapping for this block, then create one
                                if (transferOutput.neighborVals.find(valBlock) == transferOutput.neighborVals.end())
                                    transferOutput.neighborVals[valBlock] = BitVector(domainSize);

                                int valInd = domainToIndex[(void*)val];
                                // Set the bit corresponding to "val"
                                transferOutput.neighborVals[valBlock].set(valInd);

                            }
                        }
                    }

                    //Non-phi nodes: Simply add operands to the use set
                    else {
                        for (User::op_iterator opnd = insn->op_begin(), opE = insn->op_end(); opnd != opE; ++opnd) {
                            Value* val = *opnd;
                            if (isa<Instruction>(val) || isa<Argument>(val)) {
                                int valInd = domainToIndex[(void*)val];

                                // Add to useSet only if not already defined in the block somewhere earlier
                                if (!defSet[valInd])
                                    useSet.set(valInd);
                            }
                        }
                    }

                    // Definitions
                    std::unordered_map<void*, int>::iterator iter = domainToIndex.find((void*)(&*insn));
                    if (iter != domainToIndex.end())
                        defSet.set((*iter).second);
                }

                //update the genSet and killSet for each block.
                if(genSet.find(block) == genSet.end()){
                    genSet[block] = useSet;
                }

                if(killSet.find(block) == killSet.end()){
                    killSet[block] = defSet;
                }

                // Transfer function = useSet U (input - defSet)

                transferOutput.element = defSet;
                // Complement of defSet
                transferOutput.element.flip();
                // input - defSet = input INTERSECTION Complement of defSet
                transferOutput.element &= input;
                transferOutput.element |= useSet;

                return transferOutput;
            }

    };


    class Liveness : public FunctionPass {
        public:
            static char ID;

            Liveness() : FunctionPass(ID) {}

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
                for(inst_iterator II = inst_begin(F), IE = inst_end(F); II!=IE; ++II) {
                    Instruction& insn(*II);

                    // Look for insn-defined values and function args
                    for (User::op_iterator OI = insn.op_begin(), OE = insn.op_end(); OI != OE; ++OI)
                    {
                        Value *val = *OI;
                        if (isa<Instruction>(val) || isa<Argument>(val)) {
                            // Val is used by insn
                            if(std::find(domain.begin(),domain.end(),val) == domain.end())
                                domain.push_back((void*)val);
                        }
                    }
                }

                // For LVA, both are empty sets
                BitVector boundaryCond(domain.size(), false);
                BitVector initCond(domain.size(), false);

                // The pass
                LivenessAnalysis pass(Direction::BACKWARD, MeetOp::UNION);

                // Apply pass
                DataFlowResult output = pass.run(F, domain, boundaryCond, initCond);

                for(BasicBlock& BL : F){
                    BasicBlock* block = &BL;

                    outs() << "\033[1;31m[BB Name]:\033[0m"<< block->getName() << "\n";
                    outs() << "\033[1;32m[IN]:  \033[0m bitvector = "<< formatBitVector(output.result[block].in) << ", set = "<< formatSet(domain, output.result[block].in, 0) << "\n";
                    outs() << "\033[1;32m[OUT]: \033[0m bitvector = "<< formatBitVector(output.result[block].out) << ", set = "<< formatSet(domain, output.result[block].out, 0) << "\n";
                    outs() << "\033[1;32m[Use]: \033[0m bitvector = "<< formatBitVector(pass.genSet[block]) << ", set = "<< formatSet(domain, pass.genSet[block], 0) << "\n";
                    outs() << "\033[1;32m[Def]: \033[0m bitvector = "<< formatBitVector(pass.killSet[block]) << ", set = "<< formatSet(domain, pass.killSet[block], 0) << "\n\n";
                }

                // No modification
                return false;
            }
    };

    char Liveness::ID = 0;
    RegisterPass<Liveness> X("liveness", "ECE/CS 5544 Liveness");

}
