#pragma once

#include "platform/concurrency.h"
#include "platform/debugging.h"
#include "platform/memory.h"

#include "contract_core/contract_def.h"
#include "contract_core/stack_buffer.h"

#include "public_settings.h"

// Used to store: locals and for first invocation level also input and output
typedef StackBuffer<unsigned int, 32 * 1024 * 1024> ContractLocalsStack;
ContractLocalsStack contractLocalsStack[NUMBER_OF_CONTRACT_EXECUTION_PROCESSORS];
static volatile char contractLocalsStackLock[NUMBER_OF_CONTRACT_EXECUTION_PROCESSORS];

bool initContractExec()
{
    for (ContractLocalsStack::SizeType i = 0; i < NUMBER_OF_CONTRACT_EXECUTION_PROCESSORS; ++i)
        contractLocalsStack[i].init();
    setMem((void*)contractLocalsStackLock, sizeof(contractLocalsStackLock), 0);
    return true;
}

// Acquire lock of an currently unused stack (may block if all in use)
void acquireContractLocalsStack(int& stackIdx)
{
    ASSERT(stackIdx < 0);

    int i = 0;
    while (TRY_ACQUIRE(contractLocalsStackLock[i]) == false)
    {
        _mm_pause();
        ++i;
        if (i == NUMBER_OF_CONTRACT_EXECUTION_PROCESSORS)
            i = 0;
    }

    stackIdx = i;
}

void releaseContractLocalsStack(int& stackIdx)
{
    ASSERT(stackIdx > 0);
    ASSERT(stackIdx < NUMBER_OF_CONTRACT_EXECUTION_PROCESSORS);
    ASSERT(contractLocalsStackLock[stackIdx]);
    RELEASE(contractLocalsStackLock[stackIdx]);
    stackIdx = -1;
}

void* QPI::QpiContextFunctionCall::__qpiAllocLocals(unsigned int sizeOfLocals) const
{
    ASSERT(_stackIndex >= 0 && _stackIndex < NUMBER_OF_CONTRACT_EXECUTION_PROCESSORS);
    if (_stackIndex < 0 || _stackIndex >= NUMBER_OF_CONTRACT_EXECUTION_PROCESSORS)
        return 0; // TODO: log problem / restart processor here instead of returning
    void* p = contractLocalsStack[_stackIndex].allocate(sizeOfLocals);
    if (p)
        setMem(p, sizeOfLocals, 0);
    return p;
}

void QPI::QpiContextFunctionCall::__qpiFreeLocals() const
{
    ASSERT(_stackIndex >= 0 && _stackIndex < NUMBER_OF_CONTRACT_EXECUTION_PROCESSORS);
    if (_stackIndex < 0 || _stackIndex >= NUMBER_OF_CONTRACT_EXECUTION_PROCESSORS)
        return;
    contractLocalsStack[_stackIndex].free();
}

const QpiContextFunctionCall& QPI::QpiContextFunctionCall::__qpiConstructContextOtherContractFunctionCall(unsigned int otherContractIndex) const
{
    ASSERT(_stackIndex >= 0 && _stackIndex < NUMBER_OF_CONTRACT_EXECUTION_PROCESSORS);
    char * buffer = contractLocalsStack[_stackIndex].allocate(sizeof(QpiContextFunctionCall));
    QpiContextFunctionCall& newContext = *reinterpret_cast<QpiContextFunctionCall*>(buffer);
    newContext.init(otherContractIndex, _originator, _currentContractId, _invocationReward);
    return newContext;
}

const QpiContextProcedureCall& QPI::QpiContextProcedureCall::__qpiConstructContextOtherContractProcedureCall(unsigned int otherContractIndex) const
{
    ASSERT(_stackIndex >= 0 && _stackIndex < NUMBER_OF_CONTRACT_EXECUTION_PROCESSORS);
    char* buffer = contractLocalsStack[_stackIndex].allocate(sizeof(QpiContextProcedureCall));
    QpiContextProcedureCall& newContext = *reinterpret_cast<QpiContextProcedureCall*>(buffer);
    newContext.init(otherContractIndex, _originator, _currentContractId, _invocationReward);
    return newContext;
}


void QPI::QpiContextFunctionCall::__qpiFreeContextOtherContract() const
{
    ASSERT(_stackIndex >= 0 && _stackIndex < NUMBER_OF_CONTRACT_EXECUTION_PROCESSORS);
    contractLocalsStack[_stackIndex].free();
}

void* QPI::QpiContextFunctionCall::__qpiAquireStateForReading(unsigned int contractIndex) const
{
    ASSERT(contractIndex < contractCount);
    ACQUIRE(contractStateLock[contractIndex]);
    return contractStates[contractIndex];
}

void QPI::QpiContextFunctionCall::__qpiReleaseStateForReading(unsigned int contractIndex) const
{
    ASSERT(contractIndex < contractCount);
    ASSERT(contractStateLock[contractIndex]);
    RELEASE(contractStateLock[contractIndex]);
}

void* QPI::QpiContextProcedureCall::__qpiAquireStateForWriting(unsigned int contractIndex) const
{
    return QpiContextFunctionCall::__qpiAquireStateForReading(contractIndex);
}

void QPI::QpiContextProcedureCall::__qpiReleaseStateForWriting(unsigned int contractIndex) const
{
    QpiContextFunctionCall::__qpiReleaseStateForReading(contractIndex);
}

struct QpiContextSystemProcedureCall : public QPI::QpiContextProcedureCall
{
    QpiContextSystemProcedureCall(unsigned int contractIndex) : QPI::QpiContextProcedureCall(contractIndex, NULL_ID, 0)
    {
    }

    void call(SystemProcedureID systemProcId)
    {
        ASSERT(_currentContractIndex < contractCount);

        // reserve resources for this processor (may block)
        ACQUIRE(contractStateLock[_currentContractIndex]);

        const unsigned long long startTick = __rdtsc();
        contractSystemProcedures[_currentContractIndex][systemProcId](*this, contractStates[_currentContractIndex]);
        contractTotalExecutionTicks[_currentContractIndex] += __rdtsc() - startTick;
        
        RELEASE(contractStateLock[_currentContractIndex]);
    }
};

struct QpiContextUserProcedureCall : public QPI::QpiContextProcedureCall
{
    QpiContextUserProcedureCall(unsigned int contractIndex, const m256i& originator, long long invocationReward) : QPI::QpiContextProcedureCall(contractIndex, originator, invocationReward)
    {
    }

    void call(unsigned short inputType, const void* inputPtr, unsigned short inputSize)
    {
        ASSERT(_currentContractIndex < contractCount);
        ASSERT(contractUserProcedures[_currentContractIndex][inputType]);

        // reserve stack for this processor (may block)
        acquireContractLocalsStack(_stackIndex);
        ASSERT(contractLocalsStack[_stackIndex].size() == 0);

        // allocate input, output, and locals buffer from stack and init them
        unsigned short fullInputSize = contractUserProcedureInputSizes[_currentContractIndex][inputType];
        unsigned short outputSize = contractUserProcedureOutputSizes[_currentContractIndex][inputType];
        unsigned int localsSize = contractUserProcedureLocalsSizes[_currentContractIndex][inputType];
        char* inputBuffer = contractLocalsStack[_stackIndex].allocate(fullInputSize + outputSize + localsSize);
        ASSERT(inputBuffer); // TODO: error handling
        char* outputBuffer = inputBuffer + fullInputSize;
        char* localsBuffer = outputBuffer + outputSize;
        if (fullInputSize > inputSize)
            setMem(inputBuffer + inputSize, fullInputSize - inputSize, 0);
        copyMem(inputBuffer, inputPtr, inputSize);
        setMem(outputBuffer, outputSize + localsSize, 0);

        // aquire lock of contract state for writing (may block)
        ACQUIRE(contractStateLock[_currentContractIndex]);

        // run procedure
        const unsigned long long startTick = __rdtsc();
        contractUserProcedures[_currentContractIndex][inputType](*this, contractStates[_currentContractIndex], inputBuffer, outputBuffer, localsBuffer);
        contractTotalExecutionTicks[_currentContractIndex] += __rdtsc() - startTick;

        // release lock of contract state
        RELEASE(contractStateLock[_currentContractIndex]);

        // free data on stack (output is unused)
        contractLocalsStack[_stackIndex].free();
        ASSERT(contractLocalsStack[_stackIndex].size() == 0);

        // release stack lock
        releaseContractLocalsStack(_stackIndex);
    }
};


struct QpiContextUserFunctionCall : public QPI::QpiContextFunctionCall
{
    char* outputBuffer;
    unsigned short outputSize;

    QpiContextUserFunctionCall(unsigned int contractIndex) : QPI::QpiContextFunctionCall(contractIndex, NULL_ID, 0)
    {
        outputBuffer = nullptr;
        outputSize = 0;
    }

    ~QpiContextUserFunctionCall()
    {
        freeBuffer();
    }

    // call function
    void call(unsigned short inputType, const void* inputPtr, unsigned short inputSize)
    {
        ASSERT(_currentContractIndex < contractCount);
        ASSERT(contractUserFunctions[_currentContractIndex][inputType]);

        // reserve stack for this processor (may block)
        acquireContractLocalsStack(_stackIndex);
        ASSERT(contractLocalsStack[_stackIndex].size() == 0);

        // allocate input, output, and locals buffer from stack and init them
        unsigned short fullInputSize = contractUserFunctionInputSizes[_currentContractIndex][inputType];
        outputSize = contractUserFunctionOutputSizes[_currentContractIndex][inputType];
        unsigned int localsSize = contractUserFunctionLocalsSizes[_currentContractIndex][inputType];
        char* inputBuffer = contractLocalsStack[_stackIndex].allocate(fullInputSize + outputSize + localsSize);
        ASSERT(inputBuffer); // TODO: error handling
        outputBuffer = inputBuffer + fullInputSize;
        char* localsBuffer = outputBuffer + outputSize;
        if (fullInputSize > inputSize)
            setMem(inputBuffer + inputSize, fullInputSize - inputSize, 0);
        copyMem(inputBuffer, inputPtr, inputSize);
        setMem(outputBuffer, outputSize + localsSize, 0);

        // aquire lock of contract state for writing (may block)
        ACQUIRE(contractStateLock[_currentContractIndex]);

        // run function
        const unsigned long long startTick = __rdtsc();
        contractUserFunctions[_currentContractIndex][inputType](*this, contractStates[_currentContractIndex], inputBuffer, outputBuffer, localsBuffer);
        contractTotalExecutionTicks[_currentContractIndex] += __rdtsc() - startTick;

        // release lock of contract state
        RELEASE(contractStateLock[_currentContractIndex]);
    }

    // free buffer after output has been copied
    void freeBuffer()
    {
        if (_stackIndex < 0)
            return;

        // free data on stack
        contractLocalsStack[_stackIndex].free();
        ASSERT(contractLocalsStack[_stackIndex].size() == 0);

        // release locks
        releaseContractLocalsStack(_stackIndex);
    }
};



