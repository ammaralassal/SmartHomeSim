#include "Thermostat.h"
#include <string>

namespace seneca
{
	Thermostat::Thermostat(const std::string& location, const int& houseTemp, const int& desiredTemp, const bool& on) : SmartDevices(location, on)
	{
		// Initialize temperatures
		m_currentTemperature = houseTemp;
		m_desiredTemperature = desiredTemp;
	}

	int Thermostat::getCurrentTemperature()
	{
		return m_currentTemperature;
	}

	int Thermostat::getDesiredTemperature()
	{
		return m_desiredTemperature;
	}

	bool Thermostat::setDesiredTemperature(const int& newTemp)
	{
		// Validate the temperature
		if (newTemp >= 2 && newTemp <= 35)
		{
			m_desiredTemperature = newTemp;
			return true;
		}
		return false;
	}

	std::string Thermostat::getStatus()
	{
		std::string status = "The current temperature recorded by the thermostat in the " + getLocation() + " is " + std::to_string(m_currentTemperature) + "C and the desired temperature is " + std::to_string(m_desiredTemperature) + "C,";
		if (getOn())
		{
			// The thermostat is on
			if (m_currentTemperature - m_desiredTemperature > 0)
			{
				// Temperature is too high, air condition
				status += " so the air conditioning is on.";
			}
			else if (m_currentTemperature - m_desiredTemperature < 0)
			{
				// Temperature is too low, heating
				status += " so the heating is on.";
			}
			else
			{
				// Temperature is perfect
				status += " so the current temperature will be maintained.";
			}

		}
		else
		{
			// The thermostat is off
			status += " but the thermostat is currently off.";
		}

		return status;
	}
}