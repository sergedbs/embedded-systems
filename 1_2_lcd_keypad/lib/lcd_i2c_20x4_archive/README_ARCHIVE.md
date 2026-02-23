# LCD2004 (20x4) Driver Archive

This folder contains the original LCD2004 (20 columns × 4 rows) driver implementation.

## Archived Date

February 23, 2026

## Reason for Archive

Migrated to LCD1602 (16 columns × 2 rows) to simplify UI and match updated Wokwi diagram.

## Technical Details

### Display Configuration

- **Model**: LCD2004 with i2c PCF8574 backpack
- **Dimensions**: 20 columns × 4 rows
- **Row Offsets**: 0x00, 0x40, 0x14, 0x54

### Features Implemented

- HD44780 4-bit mode initialization
- PCF8574 i2c communication
- Backlight control
- Cursor positioning
- Character/string output with line wrapping
- Row clearing
- Special character handling (\n, \r, \b)

### Files

- `lcd_i2c.h` - Header file with API declarations
- `lcd_i2c.c` - Implementation file

## Usage Notes

This driver can be restored if needed by replacing the current lcd_i2c library.
All API functions remain compatible between 20x4 and 16x2 versions; only dimensions differ.

## Configuration

Used with:

- i2c address: 0x27 (or 0x3F as fallback)
- i2c frequency: 100kHz
- SDA: GPIO21, SCL: GPIO22
