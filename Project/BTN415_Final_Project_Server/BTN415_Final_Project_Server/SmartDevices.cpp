#include "SmartDevices.h"

namespace seneca
{
	SmartDevices::SmartDevices(const std::string& location, const bool& on, const std::string& ip)
		: m_location(location), m_on(on), m_ipAddress(ip) {}

	bool SmartDevices::turnOn()
	{
		if (m_on) return false; // Already on, no action required

		// Turn on
		m_on = true;
		return true;
	}

	bool SmartDevices::turnOff()
	{
		if (!m_on) return false; // Already off, no action required

		// Turn off
		m_on = false;
		return true;
	}

	bool SmartDevices::getOn() { return m_on; }


	std::string SmartDevices::getLocation() const { return m_location; } // Return the location of the device

	std::string SmartDevices::getIPAddress() const { return m_ipAddress; } //Return the IP Address of the device
}
