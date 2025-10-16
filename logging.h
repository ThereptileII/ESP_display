/*
 * logging.h
 *
 * Provides functions for logging sensor data to the SD card and
 * maintaining in‑memory ring buffers for one‑hour, six‑hour and
 * twenty‑four‑hour time horizons.  The buffers have fixed length
 * defined in config.h.  Data are sampled at intervals defined in
 * config.h and aggregated for lower resolutions.
 */

#pragma once

#include <stdint.h>

namespace logging {

  // Initialise the SD card and create log files if missing
  void init();

  // Called in the main loop to sample and log data periodically
  void tick();

  // Enumeration for selecting a time horizon
  enum class Range { R1H=0, R6H=1, R24H=2 };

  // Provide access to the battery voltage series buffers for drawing
  void getBatterySeries(Range range, const float *&buf, bool &filled, uint16_t &index);

  // Provide access to the wind true wind speed series buffers for drawing
  void getWindSeries(Range range, const float *&buf, bool &filled, uint16_t &index);
}
