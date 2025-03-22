using namespace QPI;

struct HM252
{
};

struct HM25 : public ContractBase
{
public:
    struct Echo_input{};
    struct Echo_output{};

    struct Burn_input{};
    struct Burn_output{};

    struct GetStats_input {};
    struct GetStats_output
    {
        uint64 numberOfEchoCalls;
        uint64 numberOfBurnCalls;
    };

    // Estructuras para setPrice
    struct SetPrice_input
    {
        uint64 amountInEuros;
        uint64 sellerIndex;
    };
    struct SetPrice_output
    {
        sint32 returnCode;
    };

    // Estructuras para processPayment
    struct ProcessPayment_input
    {
        uint64 buyerIndex;
        uint64 sellerIndex;
    };
    struct ProcessPayment_output
    {
        sint32 returnCode;
    };

    // Estructuras para convertToQubic
    struct ConvertToQubic_input
    {
        uint64 amountInEuros;
    };
    struct ConvertToQubic_output
    {
        uint64 qubicAmount;
        sint32 returnCode;
    };

private:
    uint64 numberOfEchoCalls;
    uint64 numberOfBurnCalls;
    Array<uint64, 1024> sellerBalances;
    uint64 exchangeRate;  // Tasa de cambio fija por ahora (1000 = 1 EUR = 1000 QUBIC)

    /**
    Send back the invocation amount
    */
    PUBLIC_PROCEDURE(Echo)
        state.numberOfEchoCalls++;
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }
    _

    /**
    * Burn all invocation amount
    */
    PUBLIC_PROCEDURE(Burn)
        state.numberOfBurnCalls++;
        if (qpi.invocationReward() > 0)
        {
            qpi.burn(qpi.invocationReward());
        }
    _

    /**
    * Set price in euros for a seller
    */
    PUBLIC_PROCEDURE(SetPrice)
        if (input.amountInEuros <= 0 || input.sellerIndex >= 1024)
        {
            output.returnCode = -1;
            return;
        }
        state.sellerBalances.set(input.sellerIndex, input.amountInEuros);
        output.returnCode = 0;
    _

    /**
    * Convert euros to QUBIC
    */
    PUBLIC_FUNCTION(ConvertToQubic)
        if (input.amountInEuros <= 0 || state.exchangeRate <= 0)
        {
            output.returnCode = -1;
            return;
        }
        output.qubicAmount = input.amountInEuros * state.exchangeRate;
        output.returnCode = 0;
    _

    /**
    * Process payment from buyer to seller
    */
    PUBLIC_PROCEDURE(ProcessPayment)
        if (input.sellerIndex >= 1024 || input.buyerIndex >= 1024)
        {
            output.returnCode = -1;
            return;
        }

        uint64 euroAmount = state.sellerBalances.get(input.sellerIndex);
        if (euroAmount <= 0)
        {
            output.returnCode = -2;
            return;
        }

        uint64 qubicAmount = euroAmount * state.exchangeRate;
        if (qpi.invocationReward() < qubicAmount)
        {
            output.returnCode = -3;
            return;
        }

        // Transfer QUBIC to seller
        qpi.transfer(input.sellerIndex, qubicAmount);
        
        // Reset seller balance
        state.sellerBalances.set(input.sellerIndex, 0);
        output.returnCode = 0;
    _

    PUBLIC_FUNCTION(GetStats)
        output.numberOfBurnCalls = state.numberOfBurnCalls;
        output.numberOfEchoCalls = state.numberOfEchoCalls;
    _

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES
        REGISTER_USER_PROCEDURE(Echo, 1);
        REGISTER_USER_PROCEDURE(Burn, 2);
        REGISTER_USER_PROCEDURE(SetPrice, 3);
        REGISTER_USER_PROCEDURE(ProcessPayment, 4);
        REGISTER_USER_FUNCTION(GetStats, 1);
        REGISTER_USER_FUNCTION(ConvertToQubic, 2);
    _

    INITIALIZE
        state.numberOfEchoCalls = 0;
        state.numberOfBurnCalls = 0;
        state.exchangeRate = 1000; // 1 EUR = 1000 QUBIC
    _
};
