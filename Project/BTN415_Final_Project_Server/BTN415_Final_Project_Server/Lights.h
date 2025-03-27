#ifndef SENECA_LIGHTS_H
#define SENECA_LIGHTS_H
#include "SmartDevices.h"
#include <string>

namespace seneca
{
    class Lights :
        public SmartDevices
    {
        /// <summary>
        /// Records whether the bulb has burned out and needs to be replaced
        /// 
        /// If true, the blub has burned out and the light cannot work; otherwise, if it is working, is false
        /// </summary>
        bool m_burnedOut;

    public:
        /// <summary>
        /// Default constructor for intitaliazing a light object
        /// </summary>
        /// <param name="location">The location of the light in the house</param>
        /// <param name="isBurnedOut">True if the bulb is burned out; false otherwise</param>
        /// <param name="on">True if the light is on, false otherwise</param>
        Lights(const std::string& location, const bool& isBurnedOut = false, const bool& on = false, const std::string& ip = "");

        /// <summary>
        /// Get the status of the light
        /// </summary>
        /// <returns>A string detailing the status of the light</returns>
        std::string getStatus();

        /// <summary>
        /// Check if the buld is burned out
        /// </summary>
        /// <returns>True if the bulb is burned out; false if it still functions</returns>
        bool getBurnedOut();

        /// <summary>
        /// Replace the bulb so that it will no longer be burned out
        /// </summary>
        void replaceBulb();
    };

}

#endif

