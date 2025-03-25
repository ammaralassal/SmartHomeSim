#include "SecurityCameras.h"
#include <string>

namespace seneca {
    SecurityCameras::SecurityCameras(const std::string& location, const bool& memoryIsFull, const bool& isMotionActivated, const bool& on) : SmartDevices(location, on)
    {
        // Initialize the security camera
        m_memoryIsFull = memoryIsFull;
        m_isMotionActivated = isMotionActivated;

    }

    bool SecurityCameras::wipeMemory()
    {
        // Reset the memory so that it is no longer full
        m_memoryIsFull = false;

        return true;
    }

    bool SecurityCameras::getMemoryIsFull()
    {
        // Return if the memory is full
        return m_memoryIsFull;
    }

    bool SecurityCameras::getIsMotionActivated()
    {
        // Return if the camera is motion activated
        return m_isMotionActivated;
    }

    std::string SecurityCameras::getStatus()
    {
        std::string status = "The";
        if (m_isMotionActivated)
        {
            // The camera is motion activated
            status += " motion activated";
        }
        else
        {
            // The camera is just a standard model
            status += " standard model";
        }
        
        status += " camera in the " + getLocation() + " is";

        if (getOn())
        {
            // The camera is on
            if (m_memoryIsFull)
            {
                // The light is burned out
                status += "  on, but the memory is full so it is not recording.";
            }
            else
            {
                // The light is functioning
                status += " on and recording.";
            }

        }
        else
        {
            // The thermostat is off
            status += " currently off.";
            if (m_memoryIsFull)
            {
                // The light is burned out
                status += " The memory is full, so it would be unable to record even if it were on.";
            }
        }

        return status;
    }
}