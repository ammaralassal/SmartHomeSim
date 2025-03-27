#ifndef SENECA_THERMOSTAT_H
#define SENECA_THERMOSTAT_H
#include "SmartDevices.h"
#include <string>

namespace seneca
{
    class Thermostat :
        public SmartDevices
    {
        /// <summary>
        /// An integer value to store the house's current temperature
        /// </summary>
        int m_currentTemperature;

        /// <summary>
        /// An integer value to store the desired temperature
        /// </summary>
        int m_desiredTemperature;

    public:
        /// <summary>
        /// Default constructor that initalizes whether the thermostat is on/off and what the initial temperatures should be
        /// </summary>
        /// <param name="houseTemp">The house's current temperature</param>
        /// <param name="desiredTemp">The temperature the thermostat is set to</param>
        /// <param name="on">Whether it is on or off (defaults to off)</param>
        Thermostat(const std::string& location, const int& houseTemp, const int& desiredTemp, const bool& on = false, const std::string& ip = "");

        /// <summary>
        /// Return the house's current temperature
        /// </summary>
        /// <returns>The house's current temperature</returns>
        int getCurrentTemperature();

        /// <summary>
        /// Return the house's desired temperature
        /// </summary>
        /// <returns>The house's desired temperature</returns>
        int getDesiredTemperature();

        /// <summary>
        /// Set the house's desired temperature
        /// </summary>
        /// <param name="newTemp">The new desired temperature</param>
        /// <returns>True if temperature was set; false otherwise</returns>
        bool setDesiredTemperature(const int& newTemp);

        /// <summary>
        /// The current and the desired temperature, whether the thermostat is heating or cooling the house
        /// </summary>
        /// <returns>Information about the thermostat in a human readable string</returns>
        std::string getStatus();
    };
}

#endif

