// ECE/CS 5544 S22 Assignment 2: available.cpp
// Group:

////////////////////////////////////////////////////////////////////////////////

#include "dataflow.h"

using namespace llvm;

namespace {

     // AE Analysis class
    class AEAnalysis : public DataFlow {

        public:

            AEAnalysis() : DataFlow(Direction::INVALID_DIRECTION, MeetOp::INVALID_MEETOP) { }
            AEAnalysis(Direction direction, MeetOp meet_op) : DataFlow(direction, meet_op) { }

        protected:

            TransferOutput transferFn(BitVector input, std::vector<void*> domain, std::unordered_map<void*, int> domainToIndex, BasicBlock* block)
            {
                TransferOutput transferOutput;

                // Calculating the set of expressions generated and killed in BB
                int domainSize = domainToIndex.size();
                BitVector GenSet(domainSize);
                BitVector KillSet(domainSize);

                for(Instruction& II : *block){
                //Instruction* I  = &II;
                //for (BasicBlock::iterator i = block->begin(), e = block->end(); i!=e; ++i) {
                    Instruction * I = &II;
                    // We only care about available expressions for BinaryOperators
                    if (BinaryOperator * BI = dyn_cast<BinaryOperator>(I)) {
                        // Create a new Expression to capture the RHS of the BinaryOperator
                        Expression *expr = new Expression(BI);
                        Expression *match = NULL;
                        bool found = false;

                        for(void* element : domain)
                        {
                            if((*expr) == *((Expression *) element))
                            {
                                found = true;
                                match = (Expression *) element;
                                break;
                            }
                        }

                        // Generated expression
                        if(found)
                        {
                            int valInd = domainToIndex[(void*)match];

                            // The instruction definitely evaluates the expression in RHS here
                            // The expression  will be killed if one of its operands is
                            // redefined subsequently in the BB.
                            GenSet.set(valInd);
                        }
                    }

                    // Killed expressions

                    // The assignment kills all expressions in which the LHS is an operand.
                    // They will be generated if subsequently recomputed in BB.
                    StringRef insn  =  I->getName();
                    if(!insn.empty())
                    {
                        //DBG(outs() << "Insn : " << insn  << "\n");
                        for(auto domain_itr = domain.begin() ; domain_itr != domain.end() ; domain_itr++)
                        {
                            Expression* expr = (Expression*) (*domain_itr);

                            StringRef op1 = expr->v1->getName();
                            StringRef op2 = expr->v2->getName();

                            if(op1.equals(insn) || op2.equals(insn))
                            {
                                //DBG(outs() << "Expr : " << expr->toString()  << " ");
                                // Kill if either operand 1 or 2 match the variable assigned
                                std::unordered_map<void*, int>::iterator iter = domainToIndex.find((void*) expr);

                                if (iter != domainToIndex.end())
                                {
                                    //DBG(outs() << "Index : " << (*iter).second  << "\n");
                                    KillSet.set((*iter).second);
                                    GenSet.reset((*iter).second);
                                }
                            }
                        }
                    }

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


    class AvailableExpressions : public FunctionPass {

        public:
            static char ID;

            AvailableExpressions() : FunctionPass(ID){
            }

        private:

            virtual bool runOnFunction(Function &F) {
                // Print Information
                // auto function_name = F.getName();
                DBG(outs() << "FUNCTION :: " << F.getName()  << "\n");


                // Setup the pass
                std::vector<void*> domain;
                // Compute the domain

                for(BasicBlock& B : F){
                    for(Instruction& I : B){
                        // We only care about available expressions for BinaryOperators
                        if (BinaryOperator * BI = dyn_cast<BinaryOperator>(&I)) {

                            // Create a new Expression to capture the RHS of the BinaryOperator
                            Expression *expr = new Expression(BI);
                            bool found = false;

                            for(void* element : domain)
                            {
                                if((*expr) == *((Expression *) element))
                                {
                                    found = true;
                                    break;
                                }
                            }

                            if(found == false)
                                domain.push_back(expr);
                            else
                                delete expr;
                        }
                    }
                }

                // For AEA, the boundary condition is phi and init condition is U.
                BitVector boundaryCond(domain.size(), false);
                BitVector initCond(domain.size(), true);

                // The pass
                AEAnalysis pass(Direction::FORWARD,  MeetOp::INTERSECTION);

                // Apply pass
                DataFlowResult output = pass.run(F, domain, boundaryCond, initCond);

                // PRINTING RESULTS
                // Map domain values to index in bitvector
                std::unordered_map<void*, int> domainToIndex;
                for (int i = 0; i < domain.size(); i++)
                    domainToIndex[(void*)domain[i]] = i;

                // We use the results to compute the available expressions
                std::stringstream ss;

                for(BasicBlock& BL : F){
                    BasicBlock* block = &BL;
                    
                    outs() << "\033[1;31m[BB Name]:\033[0m"<< block->getName() << "\n";
                    outs() << "\033[1;32m[IN]:  \033[0m bitvector = "<< formatBitVector(output.result[block].in) << ", set = "<< formatSet(domain, output.result[block].in, 1) << "\n";
                    outs() << "\033[1;32m[OUT]: \033[0m bitvector = "<< formatBitVector(output.result[block].out) << ", set = "<< formatSet(domain, output.result[block].out, 1) << "\n";
                    outs() << "\033[1;32m[Gen]: \033[0m bitvector = "<< formatBitVector(pass.genSet[block]) << ", set = "<< formatSet(domain, pass.genSet[block], 1) << "\n";
                    outs() << "\033[1;32m[Kill]:\033[0m bitvector = "<< formatBitVector(pass.killSet[block]) << ", set = "<< formatSet(domain, pass.killSet[block], 1) << "\n\n";

                }


                // No modification
                return false;
            }

    };

    char AvailableExpressions::ID = 0;
    RegisterPass<AvailableExpressions> X("available",
                           "ECE/CS 5544 Available Expressions");
}
