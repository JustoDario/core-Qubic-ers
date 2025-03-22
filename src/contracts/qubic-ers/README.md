# Qubic-ers: Sistema de Pago Digital con Smart Contracts

## Descripción
Qubic-ers es un sistema de pago digital basado en la blockchain de Qubic que permite a comerciantes y clientes realizar transacciones de manera segura y eficiente. El sistema funciona de manera similar a un datáfono tradicional, pero utilizando la tecnología blockchain.

## Características Principales
- Creación de transacciones por parte del comerciante
- Generación de códigos QR para pago
- Firma digital por parte del cliente
- Sistema de expiración de transacciones
- Registro inmutable de todas las transacciones

## Contratos Inteligentes

### QPOS (Qubic Point of Sale)
El contrato principal que maneja las transacciones de pago.

#### Estructuras Principales
```cpp
struct PendingTransaction {
    uint64 amount;           // Monto a pagar
    uint64 timestamp;        // Tiempo de creación
    uint64 expiryTime;       // Tiempo de expiración
    uint64 sellerId;         // ID del vendedor
    uint64 buyerId;          // ID del comprador
    uint8 status;           // Estado de la transacción
};
```

#### Estados de Transacción
- 0: Pendiente
- 1: Aceptada
- 2: Rechazada
- 3: Expirada

#### Funciones Principales
1. `CREATE_TRANSACTION`
   - Crea una nueva transacción de pago
   - Parámetros: monto, tiempo de expiración, ID del vendedor

2. `SIGN_TRANSACTION`
   - Permite al cliente firmar la transacción
   - Parámetros: ID de transacción, ID del comprador, aceptar/rechazar

3. `GET_PENDING_TRANSACTION`
   - Consulta el estado de una transacción
   - Parámetros: ID de transacción

## Flujo de Uso

1. **Comerciante (Vendedor)**
   - Crea una nueva transacción con el monto a pagar
   - Recibe un código QR con la información de la transacción

2. **Cliente (Comprador)**
   - Escanea el código QR
   - Revisa los detalles de la transacción
   - Decide aceptar o rechazar el pago
   - Firma digitalmente la transacción

3. **Sistema**
   - Verifica la validez de la transacción
   - Actualiza el estado según la decisión del cliente
   - Registra la transacción en la blockchain

## Requisitos Técnicos
- Qubic Network
- Smart Contract compatible con QPI (Qubic Programming Interface)
- Aplicación móvil para escaneo de QR (desarrollo pendiente)

## Seguridad
- Transacciones con tiempo de expiración
- Verificación de identidad mediante IDs
- Registro inmutable en la blockchain
- Sin transferencia de fondos hasta la confirmación

## Desarrollo Futuro
- [ ] Implementación de la interfaz de usuario
- [ ] Sistema de notificaciones
- [ ] Historial de transacciones
- [ ] Estadísticas de ventas
- [ ] Integración con sistemas POS existentes

## Contribución
Para contribuir al proyecto:
1. Fork el repositorio
2. Crea una rama para tu feature
3. Realiza tus cambios
4. Envía un Pull Request

## Licencia
Este proyecto está bajo la licencia MIT. Ver el archivo `LICENSE` para más detalles. 