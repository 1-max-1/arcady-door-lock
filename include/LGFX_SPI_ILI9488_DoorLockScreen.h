#ifndef _LGFX_SPI_ILI9488_DoorLockScreen_
#define _LGFX_SPI_ILI9488_DoorLockScreen_

#include <LovyanGFX.hpp>

/**
 * Configuration class for the screen.
*/
class LGFX_SPI_ILI9488_DoorLockScreen : public lgfx::LGFX_Device {
	private:

	lgfx::Panel_ILI9488 _panel_instance;
	lgfx::Bus_SPI _bus_instance;
	lgfx::Touch_XPT2046 _touch_instance;

	public:

  	LGFX_SPI_ILI9488_DoorLockScreen() {
		// Configure bus control settings.
		{
			auto cfg = _bus_instance.config();

			// Spi bus configuration
			cfg.spi_host = VSPI_HOST; // Select the SPI to use ESP32-S2,C3: SPI2_HOST or SPI3_HOST /ESP32: VSPI_HOST or HSPI_HOST
			// With the ESP-IDF version upgrade, the description of VSPI_HOST and HSPI_HOST will be deprecated, so if an error occurs, please use SPI2_HOST and SPI3_HOST instead.
			cfg.spi_mode = 0; // Set SPI communication mode (0 ~ 3)
			cfg.freq_write = 40000000; // SPI clock when transmitting (maximum 80MHz, rounded to 80MHz divided by an integer)
			cfg.freq_read  = 16000000; // SPI clock when receiving
			cfg.spi_3wire  = true; // Set true if receiving is done using MOSI pin
			cfg.use_lock   = true; // Set true to use transaction lock
			cfg.dma_channel = SPI_DMA_CH_AUTO; // Set the DMA channel to use (0=DMA not used /1=1ch /2=ch /SPI_DMA_CH_AUTO=automatic setting)
			// Due to the ESP-IDF version upgrade, SPI_DMA_CH_AUTO (automatic setting) is recommended for the DMA channel. Specifying 1ch or 2ch is not recommended.
			cfg.pin_sclk = 18; // Set SPI SCLK pin number
			cfg.pin_mosi = 23; // Set the SPI MOSI pin number
			cfg.pin_miso = -1; // Set SPI MISO pin number (-1 = disable)
			cfg.pin_dc   = 21; // Set SPI D/C pin number (-1 = disable)

			_bus_instance.config(cfg); // Reflect the settings on the bus.
			_panel_instance.setBus(&_bus_instance); // Set the bus on the panel.
		}

		// Set display panel control.
		{
			auto cfg = _panel_instance.config();
			cfg.pin_cs           =    22; // Pin number to which CS is connected (-1 = disable)
			cfg.pin_rst          =    -1; // Pin number to which RST is connected (-1 = disable)
			cfg.pin_busy         =    -1; // Pin number to which BUSY is connected (-1 = disable)
			_panel_instance.config(cfg);
		}

		// Configure touch screen control settings. (Delete if unnecessary)
		{
			auto cfg = _touch_instance.config();

			cfg.x_min      = 0; // Minimum X value obtained from touch screen (raw value)
			cfg.x_max      = 239; // Maximum X value (raw value) obtained from the touch screen
			cfg.y_min      = 0; // Minimum Y value (raw value) obtained from the touch screen
			cfg.y_max      = 319; // Maximum Y value (raw value) obtained from the touch screen
			cfg.pin_int    = 32; // Pin number to which INT is connected
			cfg.bus_shared = true; // Set true if using the same bus as the screen
			cfg.offset_rotation = 0; // Adjustment when the display and touch direction do not match Set as a value from 0 to 7

			// For touch Spi connection
			cfg.spi_host = VSPI_HOST; // Select the SPI to use (HSPI_HOST or VSPI_HOST)
			cfg.freq = 1000000; // Set SPI clock
			cfg.pin_sclk = 18; // Pin number to which SCLK is connected
			cfg.pin_mosi = 23; // Pin number to which MOSI is connected
			cfg.pin_miso =  19; // Pin number to which MISO is connected
			cfg.pin_cs   =  5; // Pin number to which CS is connected

			_touch_instance.config(cfg);
			_panel_instance.setTouch(&_touch_instance); // Set the touchscreen on the panel.
		}

		setPanel(&_panel_instance); // Set the panel to use.
  	}
};

#endif