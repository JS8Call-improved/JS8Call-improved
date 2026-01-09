/**
 * @file AudioDevice.cpp
 * @brief Implementation of AudioDevice class
 */
#include "AudioDevice.h"

/**
 * @brief Initializes the audio device with the specified mode and channel configuration.
 * @param mode The open mode for the device (e.g., read, write).
 * @param channel The channel configuration (Mono, Left, Right, Both).
 * @return True if initialization is successful, false otherwise.
 */
bool AudioDevice::initialize (OpenMode mode, Channel channel)
{
  m_channel = channel;

  // open and ensure we are unbuffered if possible
  return QIODevice::open (mode | QIODevice::Unbuffered);
}

