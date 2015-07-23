/*
 * This file is part of HiKoB Openlab.
 *
 * HiKoB Openlab is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, version 3.
 *
 * HiKoB Openlab is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with HiKoB Openlab. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2011,2012 HiKoB.
 */

/**
 * \file rf2xx.h
 * RF2XX header file
 *
 * \date Jul 8, 2011
 * \author Clément Burin des Roziers <clement.burin-des-roziers.at.hikob.com>
 */

#ifndef RF2XX_H_
#define RF2XX_H_

/**
 * \addtogroup periph
 * @{
 */

/**
 * \defgroup rf2xx RF231 and RF212 radio transceiver
 *
 * This provides all the methods and types required to control the RF2xx
 * transceiver chip.
 *
 * The RF2xx radio is referenced by an abstract type \ref rf2xx_t provided
 * by the platform.
 *
 * There are three types of data access: register, FIFO and RAM.
 * Register accesses enable the configuration and the control of the radio chip
 * and its internal state. The FIFO accesses allow transferring a radio frame
 * from the internal memory to the chip, and transferring a received radio frame
 * from the chip to the internal memory.
 *
 * There are two interrupts that may be generated by the driver: the IRQ interrupt
 * is triggered from the IRQ pin of the RF2xx chip, when enabled; and the DIG2
 * interrupt which is triggered by a timer in input mode, used to timestamp received
 * frames if configured correctly.
 *
 * Commands to control the  SLP_TR pin (to trigger internal
 * state changes) are also provided.
 *
 * Refer to the AT86RF231 datasheet for further details.
 *
 * \see \ref example_rf231_registers.c and \ref example_rf231_spi.c examples for
 * register access and asynchronous FIFO accesses examples.
 *
 * @{
 */

#include <stdint.h>

#include "rf2xx/rf2xx_timing.h"
#include "rf2xx/rf2xx_regs.h"
#include "handler.h"
#include "timer.h"

/**
 * Variants of the RF2xx, defining its frequency band.
 */
typedef enum
{
    RF2XX_TYPE_2_4GHz = 0,
    RF2XX_TYPE_868MHz = 1,
} rf2xx_type_t;

typedef enum
{
    TX_OK,      /**< transmission completed successfully */
    TX_CCA_FAIL,    /**< channel was busy (TX_AUTO only) */
    TX_NO_ACK,      /**< no ACK received (TX_AUTO only) */
    TX_FAIL,      /**< unexpected error */
} radio_tx_done_t;



/**
 * Abstract type defining a radio chip.
 *
 * This should be the first argument to any of the methods related to the
 * RF2XX chip. It allows to have several radio chip on the same hardware platform.
 */
typedef void *rf2xx_t;

/**
 * Reset the RF2xx radio chip.
 *
 * This will reset all register values to their default values and put the radio
 * state in TRX_OFF mode.
 *
 * \param radio the radio chip to operate on
 */
void rf2xx_reset(rf2xx_t radio);

/**
 * Get the type (i.e. the frequency band it is operating in) of the radio.
 *
 * It may be either 2.4GHz or 868MHz
 *
 * \param radio the radio chip to operate on
 * \return the type of the radio, as a \ref rf2xx_type_t
 */
rf2xx_type_t rf2xx_get_type(rf2xx_t radio);

/** \name Register access
 * @{
 */
/**
 * Read a single 8bit register from the radio chip.
 *
 * \param radio the radio chip to operate on
 * \param addr the register address to access
 * \return the read register value
 */
uint8_t rf2xx_reg_read(rf2xx_t radio, uint8_t addr);
/**
 * Write a single 8bit register to the radio chip.
 *
 * \param radio the radio chip to operate on
 * \param addr the register address to access
 * \param value the register value to write
 */
void rf2xx_reg_write(rf2xx_t radio, uint8_t addr, uint8_t value);
/** @} */

/** \name FIFO access
 * @{
 */
/**
 * Read a sequence of bytes from the FIFO to the internal memory.
 *
 * This operation synchronously copies a given amount of bytes from the FIFO,
 * most often to read a received radio frame.
 *
 * \param radio the radio chip to operate on
 * \param buffer a pointer to the internal memory to copy the FIFO content to;
 * \param length the number of bytes to copy
 */
void rf2xx_fifo_read(rf2xx_t radio, uint8_t *buffer, uint16_t length);
/**
 * Read a sequence of bytes from the FIFO to the internal memory, asynchronously.
 *
 * This does as \ref rf2xx_fifo_read but in an asynchronous manner. A provided
 * handler will be called when the transfer is complete.
 *
 * \note The handler will be called from an interrupt service routine.
 *
 * \param radio the radio chip to operate on
 * \param buffer a pointer to the internal memory to copy the FIFO content to;
 * \param length the number of bytes to copy
 * \param handler a function pointer to be called when the transfer is complete
 * \param arg the argument to pass to the handler when it'll be called
 */
void rf2xx_fifo_read_async(rf2xx_t radio, uint8_t *buffer, uint16_t length,
                           handler_t handler, handler_arg_t arg);

/**
 * Read the first byte of the FIFO and return it, but does NOT end the
 * SPI transfer.
 *
 * This reads the first byte in the FIFO but doesn't terminate the SPI transfer.
 * This allows to read the length of the received frame and to copy only the
 * required number of bytes from the FIFO.
 *
 * \note Either \ref rf2xx_fifo_read_remaining or \ref rf2xx_fifo_read_remaining_async
 * MUST be called after to continue and properly terminate the SPI transfer,
 * otherwise unpredictable behavior can occur.
 *
 * \param radio the radio chip to operate on
 * \return the first byte of the FIFO
 */
uint8_t rf2xx_fifo_read_first(rf2xx_t radio);

/**
 * Read the following bytes from the FIFO after a call to \ref rf2xx_fifo_read_first.
 *
 * After having called \ref rf2xx_fifo_read_first, calling this function reads a given
 * amount of bytes from the FIFO and terminates the SPI transfer.
 *
 * This function is synchronous.
 *
 * \note This may be called ONLY after a single call to \ref rf2xx_fifo_read_first.
 *
 * \param radio the radio chip to operate on
 * \param buffer a pointer to the internal memory to copy the FIFO to
 * \param length the number of bytes to copy
 */
void rf2xx_fifo_read_remaining(rf2xx_t radio, uint8_t *buffer, uint16_t length);

/**
 * Read the following bytes from the FIFO asynchronously, after a call to \ref rf2xx_fifo_read_first.
 *
 * After having called \ref rf2xx_fifo_read_first, calling this function reads a given
 * amount of bytes from the FIFO asynchronously and terminates the SPI transfer.
 *
 * This function is asynchronous and will operate in the background. A provided handler
 * will be call when the transfer is complete.
 *
 * \note This may be called ONLY after a single call to \ref rf2xx_fifo_read_first.
 * \note The handler function will be called from an interrupt service routing
 *
 * \param radio the radio chip to operate on
 * \param buffer a pointer to the internal memory to copy the FIFO to
 * \param length the number of bytes to copy
 * \param handler a function pointer to be called when the transfer is complete
 * \param arg an argument to provide to the handler
 */
void rf2xx_fifo_read_remaining_async(rf2xx_t radio, uint8_t *buffer,
                                     uint16_t length, handler_t handler, handler_arg_t arg);

/**
 * Write a sequence of bytes from the internal memory to the FIFO.
 *
 * This operation synchronously copies a given amount of bytes from the internal memory
 * to the FIFO, most often to write a radio frame ready to be sent.
 *
 * \param radio the radio chip to operate on
 * \param buffer a pointer to the internal memory to copy to the FIFO content
 * \param length the number of bytes to copy
 */
void rf2xx_fifo_write(rf2xx_t radio, const uint8_t *buffer, uint16_t length);

/**
 * Write a sequence of bytes from the internal memory to the FIFO, asynchronously.
 *
 * This operation asynchronously copies a given amount of bytes from the internal memory
 * to the FIFO, most often to write a radio frame ready to be sent.
 *
 * A handler is provided to be called when the transfer is complete.
 *
 * \note The transfer will operate in background, and the handler will be called
 * from an interrupt service routine.
 *
 * \param radio the radio chip to operate on
 * \param buffer a pointer to the internal memory to copy to the FIFO content
 * \param length the number of bytes to copy
 * \param handler a function pointer to be called when the transfer completes
 * \param arg an argument to provide to the handler
 */
void rf2xx_fifo_write_async(rf2xx_t radio, const uint8_t *buffer,
                            uint16_t length, handler_t handler, handler_arg_t arg);

/**
 * Write the first byte to the FIFO, but does NOT end the SPI transfer.
 *
 * This writes the first byte to the FIFO but doesn't terminate the SPI transfer.
 *
 * \note Either \ref rf2xx_fifo_write_remaining or \ref rf2xx_fifo_write_remaining_async
 * MUST be called after to continue and properly terminate the SPI transfer,
 * otherwise unpredictable behavior can occur.
 *
 * \param radio the radio chip to operate on
 * \param first the first byte to write to the FIFO
 */
void rf2xx_fifo_write_first(rf2xx_t radio, uint8_t first);

/**
 * Write the following bytes to the FIFO after a call to \ref rf2xx_fifo_write_first.
 *
 * After having called \ref rf2xx_fifo_write_first, calling this function writes a given
 * amount of bytes to the FIFO and terminates the SPI transfer.
 *
 * This function is synchronous.
 *
 * \note This may be called ONLY after a single call to \ref rf2xx_fifo_write_first.
 *
 * \param radio the radio chip to operate on
 * \param buffer a pointer to the internal memory to copy to the FIFO
 * \param length the number of bytes to copy
 */
void rf2xx_fifo_write_remaining(rf2xx_t radio, const uint8_t *buffer,
                                uint16_t length);

/**
 * Write the following bytes to the FIFO after a call to \ref rf2xx_fifo_write_first,
 *  asynchronously.
 *
 * After having called \ref rf2xx_fifo_write_first, calling this function writes
 * asynchronously a given
 * amount of bytes to the FIFO and terminates the SPI transfer.
 *
 * This function is asynchronous and will operate in the background. The provided
 * handler will be called from an interrupt service routing.
 *
 * \note This may be called ONLY after a single call to \ref rf2xx_fifo_write_first.
 *
 * \param radio the radio chip to operate on
 * \param buffer a pointer to the internal memory to copy to the FIFO
 * \param length the number of bytes to copy
 * \param handler a function pointer to be called when the transfer completes
 * \param arg an argument to provide to the handler
 */
void rf2xx_fifo_write_remaining_async(rf2xx_t radio, const uint8_t *buffer,
                                      uint16_t length, handler_t handler, handler_arg_t arg);

/**
 * Cancel any asynchronous access.
 *
 * This cancel a backgroud SPI transfer if any, it is used to properly halt a
 * transfer.
 *
 * \param radio the radio chip to operate on
 */
void rf2xx_fifo_access_cancel(rf2xx_t radio);
/** @} */

/** \name RAM access
 * @{
 */
/**
 * Read a sequence of bytes from the RAM to the internal memory.
 *
 * This operation synchronously copies a given amount of bytes from the RAM.
 *
 * \param radio the radio chip to operate on
 * \param buffer a pointer to the internal memory to copy the RAM content to;
 * \param length the number of bytes to copy
 */
void rf2xx_sram_read(rf2xx_t radio, uint8_t addr, uint8_t *buffer,
                     uint16_t length);
/**
 * Write a sequence of bytes from the internal memory to the RAM.
 *
 * This operation synchronously copies a given amount of bytes to the RAM.
 *
 * \param radio the radio chip to operate on
 * \param buffer a pointer to the internal memory to copy to the RAM content;
 * \param length the number of bytes to copy
 */
void rf2xx_sram_write(rf2xx_t radio, uint8_t addr, const uint8_t *buffer,
                      uint16_t length);
/** @} */

/** \name IRQ configuration
 * @{
 */
/**
 * Configure the handler function and argument for the IRQ interrupt.
 *
 * \note The handler will be called from an interrupt service routing.
 *
 * \param radio the radio chip to operate on
 * \param handler a function pointer to be called on interrupt
 * \param arg an argument to provide to the handler
 */
void rf2xx_irq_configure(rf2xx_t radio, handler_t handler, handler_arg_t arg);
/**
 * Enable the IRQ interrupt.
 *
 * Enable the interrupt for the IRQ pin. The handler configured with \ref
 * rf2xx_irq_configure will be called on IRQ changes.
 *
 * \param radio the radio chip to operate on
 */
void rf2xx_irq_enable(rf2xx_t radio);

/**
 * Disable the IRQ interrupt.
 *
 * Disable the interrupt for the IRQ pin.
 *
 * \param radio the radio chip to operate on
 */
void rf2xx_irq_disable(rf2xx_t radio);
/** @} */

/** \name DI2 configuration
 * @{
 */
/**
* Indicates if DIG2 exists.
*
* Returns 1 if the radio chip has its DIG2 signal connected to a timer, 0 if not.
*
* \param radio the radio chip to operate on
* \return 1 if the radio has DIG2, 0 if not
*/
int32_t rf2xx_has_dig2(rf2xx_t radio);
/**
 * Configure the DIG2 interrupt handler function.
 *
 *
 * \param radio the radio chip to operate on
 * \param handler the function pointer to be called on DIG2 interrupt
 * \param arg the argument to provide to the handler
 */
void rf2xx_dig2_configure(rf2xx_t radio, timer_handler_t handler,
                          handler_arg_t arg);
/**
 * Enable the DIG2 timer interrupt.
 *
 * Enable the timer interrupt for the DIG2 pin. The handler configured with \ref
 * rf2xx_dig2_configure will be called on DIG2 interrupt.
 *
 * \param radio the radio chip to operate on
 */
void rf2xx_dig2_enable(rf2xx_t radio);

/**
 * Disable the DIG2 timer interrupt.
 *
 * Disable the timer interrupt for the DIG2 pin.
 *
 * \param radio the radio chip to operate on
 */
void rf2xx_dig2_disable(rf2xx_t radio);
/** @} */


/** \name External PA configuration
 * @{
 */

/**
 * Indicates if the radio is configured wit an external PA.
 *
 * \param radio the radio chip to operate on
 * \return 1 if the radio has an external PA, 0 if it has not
 */
int32_t rf2xx_has_pa(rf2xx_t radio);
/**
 * Enable (power up) the external PA.
 *
 * \param radio the radio chip to operate on
 */
void rf2xx_pa_enable(rf2xx_t radio);
/**
 * Disable (power down) the external PA.
 *
 * \param radio the radio chip to operate on
 */
void rf2xx_pa_disable(rf2xx_t radio);
/** @} */

/** \name SLP_TR related commands */
/** @{ */
/**
 * Set (logical 1) the SLP_TR pin
 * \param radio the radio chip to operate on
 */
void rf2xx_slp_tr_set(rf2xx_t radio);
/**
 * Clear (logical 0) the SLP_TR pin
 * \param radio the radio chip to operate on
 */
void rf2xx_slp_tr_clear(rf2xx_t radio);

/**
 * Configure the SLP_TR pin in normal output mode.
 */
void rf2xx_slp_tr_config_output(rf2xx_t radio);
/**
 * Configure the SLP_TR pin in timer output mode.
 */
void rf2xx_slp_tr_config_timer(rf2xx_t radio);
/** @} */

/**
 * Set the state of the radio.
 *
 * \param radio the radio chip to operate on
 * \param state the new state to set
 */
static inline void rf2xx_set_state(rf2xx_t radio, uint8_t state)
{
    rf2xx_reg_write(radio, RF2XX_REG__TRX_STATE, state);
}

/**
 * Get the status of the radio.
 *
 * \param radio the radio chip to operate on
 * \return the current status of the radio
 */
static inline uint8_t rf2xx_get_status(rf2xx_t radio)
{
    return rf2xx_reg_read(radio, RF2XX_REG__TRX_STATUS)
           & RF2XX_TRX_STATUS_MASK__TRX_STATUS;
}

/**
 * Put the radio in SLEEP mode.
 *
 * This function does the necessary operations to put the device in SLEEP mode.
 *
 * \param radio the radio chip to operate on
 */
void rf2xx_sleep(rf2xx_t radio);

/**
 * Wakeup the radio from SLEEP mode.
 *
 * This function does the necessary operations to put the device in TRX_OFF mode
 * from SLEEP mode.
 *
 * \param radio the radio chip to operate on
 */
void rf2xx_wakeup(rf2xx_t radio);


/**
 *NGUYEN Ky Anh
 *
 **/
typedef struct
{
    uint8_t  *rxframe;      /**< Pointer for frame data storage. */
    uint8_t  rxframesz;     /**< Length of the buffer rxframesz */
} radio_status_t;



/**
 * @}
 * @}
 */

#endif /* RF2XX_H_ */