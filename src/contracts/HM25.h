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

    // Nuevas estructuras para setPrice
    struct SetPrice_input
    {
        uint64 amountInEuros;
        uint64 sellerIndex;  // Índice numérico para el vendedor
    };
    struct SetPrice_output
    {
        sint32 returnCode;
    };

private:
    uint64 numberOfEchoCalls;
    uint64 numberOfBurnCalls;
    Array<uint64, 1024> sellerBalances;  // Array para almacenar los balances de los vendedores

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

    PUBLIC_FUNCTION(GetStats)
        output.numberOfBurnCalls = state.numberOfBurnCalls;
        output.numberOfEchoCalls = state.numberOfEchoCalls;
    _

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES
        REGISTER_USER_PROCEDURE(Echo, 1);
        REGISTER_USER_PROCEDURE(Burn, 2);
        REGISTER_USER_PROCEDURE(SetPrice, 3);
        REGISTER_USER_FUNCTION(GetStats, 1);
    _

    INITIALIZE
        state.numberOfEchoCalls = 0;
        state.numberOfBurnCalls = 0;
    _
};
