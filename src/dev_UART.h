#ifndef __DEV_UART_H__ 
#define __DEV_UART_H__ 


#define IOADDR_UART0_BASE  0x18020000
#define IOADDR_UART1_BASE  0x18500000

#define IOADDR_UART_SIZE  0x020
#define IOADDR_UART_MASK  0x01f

#define UART_REG_RXBUF     (0x0<<2) /* read       */
#define UART_REG_TXBUF     (0x0<<2) /* write      */
#define UART_REG_INTEN     (0x1<<2) /* read/write */
#define UART_REG_INTID     (0x2<<2) /* read       */
#define UART_REG_LINECTL   (0x3<<2) /* read/write */
#define UART_REG_MODEMCTL  (0x4<<2) /* read/write */
#define UART_REG_LINESTAT  (0x5<<2) /* read/write */
#define UART_REG_MODEMSTAT (0x6<<2) /* read/write */
#define UART_REG_SCRATCH   (0x7<<2) /* write      */

#define UART_TX_EMPTY     0x40
#define UART_TX_BUF_EMPTY 0x20
#define UART_RX_BUF_FULL  0x01

#define UART_REG_LINECTL_BIT_DLAB   7 /* divisor latch access bit */

#define UART_REG_INTEN_BIT_RX_DATA_AVAIL 0
#define UART_REG_INTEN_BIT_TX_DATA_EMPTY 1

#define UART_REG_INTID_NO_INT        (1<<0)
#define UART_REG_INTID_TX_DATA_EMPTY (1<<1)
#define UART_REG_INTID_RX_DATA_AVAIL (1<<2)

void requestUARTSendBreak(struct stMachineState *pM, struct stIO_UART *pUART);
uint8_t readUARTReg(struct stMachineState *pM, struct stIO_UART *pUART, uint32_t addr);
void writeUARTReg  (struct stMachineState *pM, struct stIO_UART *pUART, uint32_t addr, uint8_t data);

#endif
