#include "Arduino.h"
#include "spc700.h"

/**
 * Changes the direction of the data bus for writing / reading.
 *
 * @param {uint8_t} direction The direction of the databuse: INPUT / OUTPUT
 */
void change_data_direction(uint8_t direction) {
  DDRD = (DDRD & 0x03) | (direction == OUTPUT ? 0xFC : 0x00);
  DDRB = (DDRB & 0xFC) | (direction == OUTPUT ? 0x03 : 0x00);
}

/**
 * Resets the SPC
 */
void spc_reset() {
  digitalWrite(PIN_RESET, LOW);
  NOP;
  digitalWrite(PIN_RESET, HIGH);

  uint8_t port0 = 0;
  uint8_t port1 = 0;

  // Wait for ports zero and one to read 0xAA and 0xBB respectively. This
  // is the sign that IPL ROM has finished and is waiting for commands.
  uint8_t bail = 255;
  while (port0 != 0xAA && port1 != 0xBB && bail > 0) {
    port0 = spc_read(PORT_0);
    port1 = spc_read(PORT_1);
    bail--;
  }

  if (bail == 0) {
    Serial.println("Failed to reset");
    Serial.println(port0, HEX);
    Serial.println(port1, HEX);
  }
}

/**
 * Reads a byte from one of the SPC ports
 *
 * @param {uint8_t} addr The port address to read from
 * @return {uint8_t} The value read from the data bus
 */
uint8_t spc_read(uint8_t addr) {
  // Set the address
  PORTB = (PORTB & B11110011) | ((addr & 0x03) << 2);
  digitalWrite(PIN_READ, LOW);
  NOP;

  // Read the data straight off the registers
  change_data_direction(INPUT);
  uint8_t result = ((PINB & 0x03) << 6) | ((PIND & 0xFC) >> 2);

  change_data_direction(OUTPUT);
  digitalWrite(PIN_READ, HIGH);

  return result;
}

/**
 * Writes a value out to the database at a particular port
 *
 * @param {uint8_t} addr The port address to write to
 * @param {uint8_t} value The value to write
 */
void spc_write(uint8_t addr, uint8_t value) {
  change_data_direction(OUTPUT);

  // Set the address and the upper two bits of data
  PORTB = (PORTB & 0xF0) | ((addr & 0x03) << 2) | ((value & 0xC0) >> 6);

  // Lower six bits
  PORTD = (PORTD & 0x03) | ((value & 0x3F) << 2);

  // Cycle the write line
  digitalWrite(PIN_WRITE, LOW);
  NOP;
  digitalWrite(PIN_WRITE, HIGH);
}

/**
 * Writes a chunk of data to the SPC. Requires that transfer mode
 * already be active.
 *
 * @param {uint8_t *} data Pointer to the data to write
 * @param {uint16_t} len The length of the data to write
 */
void spc_write_chunk(uint8_t *data, uint16_t len) {
  uint8_t check = 0;
  for (uint16_t i = 0; i < len; i++) {
    spc_write(PORT_1, data[i]);
    spc_write(PORT_0, check);
    spc_zero_wait(check++);
  }
}

/**
 * Waits for PORT0 to present a particular value
 */
void spc_zero_wait(uint8_t value) {
  while (spc_read(PORT_0) != value);
}
