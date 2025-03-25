#include "Lights.h"
#include <string>

namespace seneca
{
    Lights::Lights(const std::string& location, const bool& isBurnedOut, const bool& on) : SmartDevices(location, on)
    {
        // Initialize light
        m_burnedOut = isBurnedOut;
    }


    std::string Lights::getStatus()
    {
		std::string status = "The light in the " + getLocation() + " is";
		if (getOn())
		{
			// The light is on
			if (m_burnedOut)
			{
				// The light is burned out
				status += " set to be on, but the bulb is burned out.";
			}
			else
			{
				// The light is functioning
				status += " on.";
			}

		}
		else
		{
			// The thermostat is off
			status += " currently off.";
			if (m_burnedOut)
			{
				// The light is burned out
				status += " The bulb has burned out, so turning it on will have no effect.";
			}
		}

		return status;
    }

    bool Lights::getBurnedOut()
    {
        // Return if the bulb has burned out
        return m_burnedOut;
    }

    void Lights::replaceBulb()
    {
        // Replace the bulb with a fresh one
        m_burnedOut = false;
    }
}
