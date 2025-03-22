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

    struct SetPrice_input {
        uint64 price;
    };
    struct SetPrice_output {
        sint32 returnCode;
    };

    struct SetSeller_input {
        id sellerId;
    };
    struct SetSeller_output {
        sint32 returnCode;
    };

    struct SetSender_input {
        id senderId;
    };
    struct SetSender_output {
        sint32 returnCode;
    };

    struct ProcessPayment_input {};
    struct ProcessPayment_output {
        sint32 returnCode;
    };

    struct GetStats_input {};
    struct GetStats_output {
        uint64 numberOfEchoCalls;
        uint64 numberOfBurnCalls;
        uint64 currentPrice;
        id currentSeller;
        id currentSender;
    };

private:
    uint64 numberOfEchoCalls;
    uint64 numberOfBurnCalls;
    uint64 currentPrice;
    id currentSeller;
    id currentSender;

    /**
    Send back the invocation amount
    */
    PUBLIC_PROCEDURE(Echo)
        numberOfEchoCalls++;
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }
    _

    /**
    * Burn all invocation amount
    */
    PUBLIC_PROCEDURE(Burn)
        numberOfBurnCalls++;
        if (qpi.invocationReward() > 0)
        {
            qpi.burn(qpi.invocationReward());
        }
    _

    PUBLIC_PROCEDURE(SetPrice)
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }
        currentPrice = input.price;
        output.returnCode = 1;
    _

    PUBLIC_PROCEDURE(SetSeller)
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }
        currentSeller = input.sellerId;
        output.returnCode = 1;
    _

    PUBLIC_PROCEDURE(SetSender)
        if (qpi.invocationReward() > 0)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward());
        }
        currentSender = input.senderId;
        output.returnCode = 1;
    _

    PUBLIC_PROCEDURE(ProcessPayment)
        if (qpi.invocationReward() < currentPrice)
        {
            output.returnCode = 0;
            if (qpi.invocationReward() > 0)
            {
                qpi.transfer(qpi.invocator(), qpi.invocationReward());
            }
            return;
        }

        if (qpi.invocationReward() > currentPrice)
        {
            qpi.transfer(qpi.invocator(), qpi.invocationReward() - currentPrice);
        }

        qpi.transfer(currentSeller, currentPrice);
        output.returnCode = 1;
    _

    PUBLIC_FUNCTION(GetStats)
        output.numberOfBurnCalls = numberOfBurnCalls;
        output.numberOfEchoCalls = numberOfEchoCalls;
        output.currentPrice = currentPrice;
        output.currentSeller = currentSeller;
        output.currentSender = currentSender;
    _

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES
        REGISTER_USER_PROCEDURE(Echo, 1);
        REGISTER_USER_PROCEDURE(Burn, 2);
        REGISTER_USER_PROCEDURE(SetPrice, 3);
        REGISTER_USER_PROCEDURE(SetSeller, 4);
        REGISTER_USER_PROCEDURE(SetSender, 5);
        REGISTER_USER_PROCEDURE(ProcessPayment, 6);
        REGISTER_USER_FUNCTION(GetStats, 1);
    _

    INITIALIZE
        numberOfEchoCalls = 0;
        numberOfBurnCalls = 0;
        currentPrice = 0;
        currentSeller = NULL_ID;
        currentSender = NULL_ID;
    _
};
