using namespace QPI;

// Estructura para almacenar la información de una transacción pendiente
struct PendingTransaction {
    uint64 amount;           // Monto a pagar
    uint64 timestamp;        // Tiempo de creación
    uint64 expiryTime;       // Tiempo de expiración
    uint64 sellerId;         // ID del vendedor
    uint64 buyerId;          // ID del comprador (0 si no está firmado)
    uint8 status;           // 0: pendiente, 1: aceptado, 2: rechazado, 3: expirado
};

// Estructura para el estado del contrato
struct QPOSState {
    collection<PendingTransaction> pendingTransactions;
    uint64 transactionCounter;
};

// Estructura para los parámetros de entrada de crear transacción
struct CreateTransactionInput {
    uint64 amount;
    uint64 expiryTime;
    uint64 sellerId;
};

// Estructura para los parámetros de entrada de firmar transacción
struct SignTransactionInput {
    uint64 transactionId;
    uint64 buyerId;
    uint8 accept;  // 1 para aceptar, 0 para rechazar
};

struct QPOS : public ContractBase
{
    QPOSState state;

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES
    CREATE_TRANSACTION
    SIGN_TRANSACTION
    GET_PENDING_TRANSACTION
    _

    INITIALIZE
    state.transactionCounter = 0;
    _

    // Procedimiento para crear una nueva transacción
    CREATE_TRANSACTION
    CreateTransactionInput input;
    PendingTransaction newTx;
    
    newTx.amount = input.amount;
    newTx.timestamp = getTick();
    newTx.expiryTime = input.expiryTime;
    newTx.sellerId = input.sellerId;
    newTx.buyerId = 0;
    newTx.status = 0;
    
    state.pendingTransactions.push_back(newTx);
    state.transactionCounter = state.transactionCounter + 1;
    _

    // Procedimiento para firmar una transacción
    SIGN_TRANSACTION
    SignTransactionInput input;
    PendingTransaction tx;
    
    if (input.transactionId < state.pendingTransactions.size()) {
        tx = state.pendingTransactions[input.transactionId];
        
        if (tx.status == 0 && getTick() <= tx.expiryTime) {
            tx.buyerId = input.buyerId;
            tx.status = input.accept ? 1 : 2;
            state.pendingTransactions[input.transactionId] = tx;
        } else if (getTick() > tx.expiryTime) {
            tx.status = 3;
            state.pendingTransactions[input.transactionId] = tx;
        }
    }
    _

    // Función para obtener una transacción pendiente
    GET_PENDING_TRANSACTION
    uint64 transactionId;
    PendingTransaction result;
    
    if (transactionId < state.pendingTransactions.size()) {
        result = state.pendingTransactions[transactionId];
    }
    _
}; 