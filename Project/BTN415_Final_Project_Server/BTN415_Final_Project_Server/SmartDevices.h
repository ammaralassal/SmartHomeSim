#ifndef SENECA_SMARTDEVICES_H
#define SENECA_SMARTDEVICES_H

#include <string>

namespace seneca {

	/// <summary>
	/// An abstract class to serve as the basis for the various smart devices in the home
	/// </summary>
	class SmartDevices
	{
		/// <summary>
        /// Stores the location of the device on the property
        /// </summary>
		std::string m_location;

		/// <summary>
		/// A boolean variable to determine whether a device is on or off
		/// When true, the device is on; when false, the device is off
		/// </summary>
		bool m_on;


	public:
		/// <summary>
		/// Default constructor
		/// </summary>
		/// <param name="on">A value to initialize whether the device starts on or off; defaults to off</param>
		/// <param name="location">A value to initialize the location of the device</param>
		SmartDevices(const std::string& location, const bool& on = false);

		/// <summary>
		/// Turn the device on if it was off
		/// If the device was already on, does nothing
		/// 
		/// After this function runs, the device will always be on
		/// </summary>
		/// <returns>True if the device was turned on; false if nothing happened</returns>
		bool turnOn();

		/// <summary>
		/// Turn the device off if it was on
		/// If the device was already off, does nothing
		/// 
		/// After this function runs, the device will always be off
		/// </summary>
		/// <returns>True if the device was turned off; false if nothing happened</returns>
		bool turnOff();

		/// <summary>
		/// Checks if the device is on or off
		/// </summary>
		/// <returns>True if the device is on; false if the device is off</returns>
		bool getOn();

		/// <summary>
		/// Get the location of the device
		/// </summary>
		/// <returns>A string detailing the location of the device on the property</returns>
		std::string getLocation();

		/// <summary>
		/// A pure virtual function to be implemented by child classes
		/// 
		/// This function should return the status of the object as a human readable string
		/// </summary>
		/// <returns>The status of the object as a human readable string</returns>
		virtual std::string getStatus() = 0;
	};
}

#endif
