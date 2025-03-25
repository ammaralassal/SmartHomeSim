#ifndef SENECA_SECURITYCAMERAS_H
#define SENECA_SECURITYCAMERAS_H
#include "SmartDevices.h"
#include <string>

namespace seneca
{
#include "SmartDevices.h"
    class SecurityCameras :
        public SmartDevices
    {
        /// <summary>
        /// True if the camera is motion actiated; false if camera is not
        /// </summary>
        bool m_isMotionActivated;

        /// <summary>
        /// True if the camera's memory is full; false if it still has room to record
        /// </summary>
        bool m_memoryIsFull;

    public:
        /// <summary>
        /// Default constructor for security camera
        /// </summary>
        /// <param name="location">The location of the security camera on the property</param>
        /// <param name="memoryIsFull">True if the camera's memory is full and no further recording is possible</param>
        /// <param name="m_isMotionActivated">True if the camera type is 'motion activated'</param>
        /// <param name="on">True if the camera is currently on</param>
        SecurityCameras(const std::string& location = "", const bool& memoryIsFull = false, const bool& isMotionActivated = false, const bool& on = false);
        
        /// <summary>
        /// Wipes the camera's memory so that the memory is no longer full, regardless of what it was at before
        /// </summary>
        /// <returns>True to indicate the memory was wiped</returns>
        bool wipeMemory();

        /// <summary>
        /// Checks if the camera is out of memory
        /// </summary>
        /// <returns>True if the memory is full; false otherwise</returns>
        bool getMemoryIsFull();

        /// <summary>
        /// Determines if the camera is motion activated or not
        /// </summary>
        /// <returns>True if the camera is motion activated; false otherwise</returns>
        bool getIsMotionActivated();

        /// <summary>
        /// Get the status of the camera
        /// </summary>
        /// <returns>A string detailing the status of the camera</returns>
        std::string getStatus();

    };

}
#endif

