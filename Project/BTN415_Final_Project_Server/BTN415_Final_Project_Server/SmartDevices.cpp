#include "SmartDevices.h"

namespace seneca
{
	SmartDevices::SmartDevices(const std::string& location, const bool& on)
	{
		// Initialize the on attribute
		m_location = location;
		m_on = on;
	}

	bool SmartDevices::turnOn()
	{
		if (m_on)
		{
			// Already on, no action required
			return false;
		}

		// Turn on
		m_on = true;
		return true;
	}

	bool SmartDevices::turnOff()
	{
		if (!m_on)
		{
			// Already off, no action required
			return false;
		}

		// Turn off
		m_on = false;
		return false;
	}

	bool SmartDevices::getOn()
	{
		return m_on;
	}

	std::string SmartDevices::getLocation()
	{
		// Return the location of the device
		return m_location;
	}
}
