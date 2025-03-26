
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <winsock2.h>
#include <string>
#include <vector>
#include <thread>
#include <mutex>

#include "SmartDevices.h"
#include "Lights.h"
#include "SecurityCameras.h"
#include "Thermostat.h"
#pragma comment(lib, "Ws2_32.lib")

// A mutex to lock the lights array so that there are no race conditions
std::mutex mu_lights;
// A mutex to lock the cameras array so that there are no race conditions
std::mutex mu_cameras;
// A mutex to lock the thermostats array so that there are no race conditions
std::mutex mu_thermostats;
// A mutex to lock the logged in users collection so that there are no race conditions
std::mutex mu_loggedInUsers;
// Vector to store the smart lights in the home (shared resource across threads)
std::vector<seneca::Lights> lights;
// Vector to store the smart lights in the home (shared resource across threads)
std::vector<seneca::SecurityCameras> cameras;
// Vector to store the smart lights in the home (shared resource across threads)
std::vector<seneca::Thermostat> thermostats;
// Vector of the logged in users for the server (shared resource across threads)
std::vector<std::string> loggedInUsers;

// Constant to track number of clients the server can handle
int const MAX_SOCKETS = 5;
SOCKET Aux_Socket;
SOCKET ClientSockets[MAX_SOCKETS + 1] = { SOCKET_ERROR };
bool Active_Sockets[MAX_SOCKETS + 1] = { false };

// Basic implementation for testing, not done
void load()
{
    // Just load one light for now for testing purposes
    lights.push_back(seneca::Lights("Bedroom"));
    // Just load one camera for now for testing purposes
    cameras.push_back(seneca::SecurityCameras("Kitchen"));
    // Just load one thermostat for now for testing purposes
    thermostats.push_back(seneca::Thermostat("Living-Room", 18, 21, true));
}

/// <summary>
/// Check if a user is logged in
/// </summary>
/// <param name="username">The user to check</param>
/// <returns>True if they are logged in; false otherwise</returns>
bool isLoggedIn(const std::string& username)
{
    // Take control of the lock
    mu_loggedInUsers.lock();

    // Check if the user is already logged in
    auto index = std::find(loggedInUsers.begin(), loggedInUsers.end(), username);
    bool loggedIn = index == loggedInUsers.end();

    // Release control of the lock
    mu_loggedInUsers.unlock();

    return loggedIn;
}

/// <summary>
/// Check if the user is already logged in. If not, make sure their credentials are valid and log them in
/// </summary>
/// <param name="clientSocket">The socket</param>
/// <param name="credentials">The username and password of the user</param>
/// <param name="loggedInUsers">The already logged in users</param>
/// <param name="usernames">The valid usernames</param>
/// <param name="passwords">The valid passwords</param>
/// <returns>True if log in succeds; False otherwise</returns>
bool signIn(SOCKET& clientSocket, const std::string& credentials, const std::vector<std::string>& usernames, const std::vector<std::string>& passwords)
{
    // Message to send back to client
    std::string response;
    bool succeeded = false;

    // Get the user's credentials from the client's request
    std::string username = credentials.substr(0, credentials.find("/"));
    std::string password = credentials.substr(credentials.find("/") + 1);

    // Take control of the lock
    mu_loggedInUsers.lock();

    // Check if the user is already logged in
    if (std::find(loggedInUsers.begin(), loggedInUsers.end(), username) == loggedInUsers.end())
    {
        // The user attempting to login is not already logged in, see if the provided credentials are valid (see if the username is in the vector, and if it is, see if the corresponding password matches)

        // Check the position is within range and the password at the corret index
        auto pos = std::find(usernames.begin(), usernames.end(), username);
        if (pos != usernames.end() && passwords[std::distance(usernames.begin(), pos)] == password)
        {
            // The login attempt is valid and should be approved. Log them in and formulate a positive response message to the user
            loggedInUsers.push_back(username);
            response = "Succeeded. " + username + " is now logged in.";
            succeeded = true;
        }
        else
        {
            // Invalid credentials, formulate an approperiately negative response message to the user
            response = "Failed. Invalid username and/or password.";
        }

    }
    else
    {
        // This account is already logged in, reject the login attempt
        response = "Failed. Username provided was already logged in.";
    }

    // Release control of the lock
    mu_loggedInUsers.unlock();

    // Send response to client
    send(clientSocket, response.c_str(), response.size(), 0);

    return succeeded;
}

/// <summary>
/// Log out the user (assume they are logged in)
/// </summary>
/// <param name="clientSocket">The socket</param>
/// <param name="username">The username of the person attempting to log out</param>
/// <returns>True to indicate sign-out worked</returns>
bool signOut(SOCKET& clientSocket, const std::string& username)
{
    // Message to send back to client
    std::string response;
    bool succeeded = false;

    // Take control of the lock
    mu_loggedInUsers.lock();

    // Check if the user is already logged in
    auto index = std::find(loggedInUsers.begin(), loggedInUsers.end(), username);
    if (index == loggedInUsers.end())
    {
        // The user attempting to logout is not logged in so they cannot be logged out
        response = "Failed. Server does not recognise the user as being logged in.";

    }
    else
    {
        // This account is logged in, log them out
        loggedInUsers.erase(loggedInUsers.begin() + distance(loggedInUsers.begin(), index));
        response = "Succeeded. The user is now logged out.";
        succeeded = true;
    }

    // Release control of the lock
    mu_loggedInUsers.unlock();

    // Send response to client
    send(clientSocket, response.c_str(), response.size(), 0);

    return succeeded;
}

/// <summary>
/// Returns information regarding a smart device
/// </summary>
/// <param name="clientSocket">The socket</param>
/// <param name="requestDetails">The details of the request</param>
/// <returns>True if request could be fulfilled; false otherwise</returns>
bool getDetails(SOCKET& clientSocket, std::string& requestDetails)
{
    // Message to send back to client
    std::string response;
    bool succeeded = false;

    // Extract the device type from the request details
    std::string deviceType = requestDetails.substr(0, requestDetails.find("/"));
    requestDetails = requestDetails.substr(requestDetails.find("/") + 1);
    // Extract the request type from the request details
    std::string requestType = requestDetails.substr(0, requestDetails.find("/"));
    requestDetails = requestDetails.substr(requestDetails.find("/") + 1);
    // Extract the username from the request details
    std::string username = requestDetails.substr(0, requestDetails.find("/"));
    requestDetails = requestDetails.substr(requestDetails.find("/") + 1);

    // Check if the user is logged in
    if (isLoggedIn(username))
    {
        // The user attempting to view the inventory is not logged in so they request must be rejected
        response = "Failed. Server does not recognise the user as being logged in.";

    }
    else
    {
        // Extract the device number (index in array will be 1 less than user selection)
        int deviceNumber =  deviceNumber = std::stoi(requestDetails.substr(0, requestDetails.find(" "))) - 1;
        requestDetails = requestDetails.substr(requestDetails.find(" ") + 1);


        // Take control of the approperiate lock
        if (deviceType == "L")
        {
            // Device is a Light
            mu_lights.lock();

            if (requestType == "AL")
            {
                // Request is for all lights
                if (lights.size() > 0)
                {
                    // There are lights in the house, format the light data and send it back
                    response = "Succeeded. ";
                    succeeded = true;

                    for (int i = 0; i < lights.size(); ++i)
                    {
                        // Add the identifying information of the light to the string
                        response += lights[i].getLocation() + " ";
                    }
                }
                else
                {
                    // No light information could be delivered, no lights exist
                    response = "Failed. There are no smart lights in the house.";
                }
            }
            else if (lights.size() > 0 && deviceNumber < lights.size())
            {
                // Request is for 1 valid light
                response = "Succeeded. ";
                succeeded = true;
                
                // Fulfil request
                if (requestType == "ON")
                {
                    // Check if the light is on or off
                    response += "The light is ";

                        if (lights[deviceNumber].getOn())
                        {
                            // The light is on
                            response += "on.";
                        }
                        else
                        {
                            // The light is off
                            response += "off.";
                        }
                }
                else if (requestType == "LO")
                {
                    // Check the light's location
                    response += "The light is located in the " + lights[deviceNumber].getLocation() + ".";
                }
                else if (requestType == "BO")
                {
                    // Check if the light bulb has burned out
                    response += "The light bulb has ";

                    if (lights[deviceNumber].getBurnedOut())
                    {
                        // The light has burned out
                        response += "burned out.";
                    }
                    else
                    {
                        // The light bulb has not burned out
                        response += "not burned out.";
                    }
                }
                else if (requestType == "ST")
                {
                    // Get a full status report
                    response += lights[deviceNumber].getStatus();
                }
            }
            else
            {
                // No light information could be delivered, the light does not exist
                response = "Failed. That is not recognised as a valid light.";
            }

            // Release control of the lock
            mu_lights.unlock();
        }
        else if (deviceType == "C")
        {
            // Device is a Camera
            mu_cameras.lock();

            if (requestType == "AL")
            {
                // Request is for all cameras
                if (cameras.size() > 0)
                {
                    // There are lights in the house, format the camera data and send it back
                    response = "Succeeded. ";
                    succeeded = true;

                    for (int i = 0; i < cameras.size(); ++i)
                    {
                        // Add the identifying information of the camera to the string
                        response += cameras[i].getLocation() + " ";
                    }
                }
                else
                {
                    // No light information could be delivered, no cameras exist
                    response = "Failed. There are no smart security cameras in the house.";
                }
            }
            else if (cameras.size() > 0 && deviceNumber < cameras.size())
            {
                // Request is for 1 valid camera
                response = "Succeeded. ";
                succeeded = true;

                // Fulfil request
                if (requestType == "ON")
                {
                    // Check if the camera is on or off
                    response += "The camera is ";

                    if (cameras[deviceNumber].getOn())
                    {
                        // The camera is on
                        response += "on.";
                    }
                    else
                    {
                        // The camera is off
                        response += "off.";
                    }
                }
                else if (requestType == "LO")
                {
                    // Check the camera's location
                    response += "The camera is located in the " + cameras[deviceNumber].getLocation() + ".";
                }
                else if (requestType == "MA")
                {
                    // Check if the camera is motion activated
                    response += "The camera is ";

                    if (cameras[deviceNumber].getIsMotionActivated())
                    {
                        // The camera is motion activated
                        response += "motion activated.";
                    }
                    else
                    {
                        // The camera is not motion activated
                        response += "not motion activated.";
                    }
                }
                else if (requestType == "MF")
                {
                    // Check if the camera's memory is full
                    response += "The camera's memory is ";

                    if (cameras[deviceNumber].getMemoryIsFull())
                    {
                        // The camera's memory is full
                        response += "full.";
                    }
                    else
                    {
                        // The camera's memory is not full
                        response += "not full.";
                    }
                }
                else if (requestType == "ST")
                {
                    // Get a full status report
                    response += cameras[deviceNumber].getStatus();
                }
            }
            else
            {
                // No camera information could be delivered, the camera does not exist
                response = "Failed. That is not recognised as a valid camera.";
            }

            // Release control of the lock
            mu_cameras.unlock();
        }
        else if (deviceType == "T")
        {
            // Device is a Thermostat
            mu_thermostats.lock();

            if (requestType == "AL")
            {
                // Request is for all thermostats
                if (thermostats.size() > 0)
                {
                    // There are thermostats in the house, format the thermostat data and send it back
                    response = "Succeeded. ";
                    succeeded = true;

                    for (int i = 0; i < thermostats.size(); ++i)
                    {
                        // Add the identifying information of the thermostats to the string
                        response += thermostats[i].getLocation() + " ";
                    }
                }
                else
                {
                    // No thermostat information could be delivered, no thermostats exist
                    response = "Failed. There are no smart thermostats in the house.";
                }
            }
            else if (thermostats.size() > 0 && deviceNumber < thermostats.size())
            {
                // Request is for 1 valid thermostat
                response = "Succeeded. ";
                succeeded = true;

                // Fulfil request
                if (requestType == "ON")
                {
                    // Check if the thermostat is on or off
                    response += "The thermostat is ";

                    if (thermostats[deviceNumber].getOn())
                    {
                        // The thermostat is on
                        response += "on.";
                    }
                    else
                    {
                        // The thermostat is off
                        response += "off.";
                    }
                }
                else if (requestType == "LO")
                {
                    // Check the thermostat's location
                    response += "The thermostat is located in the " + thermostats[deviceNumber].getLocation() + ".";
                }
                else if (requestType == "CT")
                {
                    // Check the current house temperature
                    response += "The current temperature is " + std::to_string(thermostats[deviceNumber].getCurrentTemperature()) + "C.";
                }
                else if (requestType == "DT")
                {
                    // Check the current house temperature
                    response += "The desired/set temperature is " + std::to_string(thermostats[deviceNumber].getDesiredTemperature()) + "C.";
                }
                else if (requestType == "ST")
                {
                    // Get a full status report
                    response += thermostats[deviceNumber].getStatus();
                }
            }
            else
            {
                // No thermostat information could be delivered, the thermostat does not exist
                response = "Failed. That is not recognised as a valid thermostat.";
            }

            // Release control of the lock
            mu_thermostats.unlock();
        }
    }

    // Send response to user
    send(clientSocket, response.c_str(), response.size(), 0);

    return succeeded;
}

/// <summary>
/// Attempts to modify a smart device 
/// </summary>
/// <param name="clientSocket">The socket</param>
/// <param name="requestDetails">The details of the modification request</param>
/// <returns>True if request could be fulfilled; false otherwise</returns>
bool putDetails(SOCKET& clientSocket, std::string& requestDetails)
{
    // Message to send back to client
    std::string response;
    bool succeeded = false;

    // Extract the device type from the request details
    std::string deviceType = requestDetails.substr(0, requestDetails.find("/"));
    requestDetails = requestDetails.substr(requestDetails.find("/") + 1);
    // Extract the request type from the request details
    std::string requestType = requestDetails.substr(0, requestDetails.find("/"));
    requestDetails = requestDetails.substr(requestDetails.find("/") + 1);
    // Extract the username from the request details
    std::string username = requestDetails.substr(0, requestDetails.find("/"));
    requestDetails = requestDetails.substr(requestDetails.find("/") + 1);

    // Check if the user is logged in
    if (isLoggedIn(username))
    {
        // The user attempting to view the inventory is not logged in so they request must be rejected
        response = "Failed. Server does not recognise the user as being logged in.";

    }
    else
    {
        // Extract the device number (index in array will be 1 less than user selection)
        int deviceNumber = std::stoi(requestDetails.substr(0, requestDetails.find("/"))) - 1;
        requestDetails = requestDetails.substr(requestDetails.find("/") + 1);


        // Take control of the approperiate lock
        if (deviceType == "L")
        {
            // Device is a Light
            mu_lights.lock();

            if (lights.size() > 0 && deviceNumber < lights.size())
            {
                // Valid light, try to fulfil request
                if (requestType == "ON")
                {
                    // Try to turn the light on
                    succeeded = lights[deviceNumber].turnOn();
                    if (succeeded)
                    {
                        // The light was sucessfully turned on
                        response = "Succeeded. The light has been turned on.";
                    }
                    else
                    {
                        // The light was already on
                        response = "Failed. The light was already turned on, nothing has happened.";
                    }
                }
                else if (requestType == "OF")
                {
                    // Try to turn the light off
                    succeeded = lights[deviceNumber].turnOff();
                    if (succeeded)
                    {
                        // The light was sucessfully turned off
                        response = "Succeeded. The light has been turned off.";
                    }
                    else
                    {
                        // The light was already off
                        response = "Failed. The light was already turned off, nothing has happened.";
                    }
                }
                else if (requestType == "RB")
                {
                    // Replace the light bulb
                    response += "Succeeded. The light bulb has been replaced.";
                    lights[deviceNumber].replaceBulb();
                }
            }
            else
            {
                // No light information could be delivered, the light does not exist
                response = "Failed. That is not recognised as a valid light.";
            }

            // Release control of the lock
            mu_lights.unlock();
        }
        else if (deviceType == "C")
        {
            // Device is a Camera
            mu_cameras.lock();

            if (cameras.size() > 0 && deviceNumber < cameras.size())
            {
                // Valid camera, try to fulfil request
                if (requestType == "ON")
                {
                    // Try to turn the camera on
                    succeeded = cameras[deviceNumber].turnOn();
                    if (succeeded)
                    {
                        // The camera was sucessfully turned on
                        response = "Succeeded. The camera has been turned on.";
                    }
                    else
                    {
                        // The camera was already on
                        response = "Failed. The camera was already turned on, nothing has happened.";
                    }
                }
                else if (requestType == "OF")
                {
                    // Try to turn the camera off
                    succeeded = cameras[deviceNumber].turnOff();
                    if (succeeded)
                    {
                        // The camera was sucessfully turned off
                        response = "Succeeded. The camera has been turned off.";
                    }
                    else
                    {
                        // The camera was already off
                        response = "Failed. The camera was already turned off, nothing has happened.";
                    }
                }
                else if (requestType == "EM")
                {
                    // Empty the memory
                    response += "Succeeded. The camera's memory has been emptied.";
                    succeeded = cameras[deviceNumber].wipeMemory();
                }
            }
            else
            {
                // No camera information could be delivered, the camera does not exist
                response = "Failed. That is not recognised as a valid camera.";
            }

            // Release control of the lock
            mu_cameras.unlock();
        }
        else if (deviceType == "T")
        {
            // Device is a Thermostat
            mu_thermostats.lock();

            if (thermostats.size() > 0 && deviceNumber < thermostats.size())
            {
                // Valid thermostat, try to fulfil request
                if (requestType == "ON")
                {
                    // Try to turn the thermostat on
                    succeeded = thermostats[deviceNumber].turnOn();
                    if (succeeded)
                    {
                        // The thermostat was sucessfully turned on
                        response = "Succeeded. The thermostat has been turned on.";
                    }
                    else
                    {
                        // The thermostat was already on
                        response = "Failed. The thermostat was already turned on, nothing has happened.";
                    }
                }
                else if (requestType == "OF")
                {
                    // Try to turn the thermostat off
                    succeeded = thermostats[deviceNumber].turnOff();
                    if (succeeded)
                    {
                        // The thermostat was sucessfully turned off
                        response = "Succeeded. The thermostat has been turned off.";
                    }
                    else
                    {
                        // The thermostat was already off
                        response = "Failed. The thermostat was already turned off, nothing has happened.";
                    }
                }
                else if (requestType == "ST")
                {
                    // Extract the new temperature
                    int newTemp = std::stoi(requestDetails.substr(0, requestDetails.find("/")));
                    requestDetails = requestDetails.substr(requestDetails.find("/") + 1);

                    // Try to set a new temperature
                    succeeded = thermostats[deviceNumber].setDesiredTemperature(newTemp);
                    if (succeeded)
                    {
                        // The thermostat was sucessfully set
                        response = "Succeeded. The thermostat has been set.";
                    }
                    else
                    {
                        // The thermostat was already off
                        response = "Failed. The thermostat was not able to be set.";
                    }
                }
            }
            else
            {
                // No thermostat information could be delivered, the thermostat does not exist
                response = "Failed. That is not recognised as a valid thermostat.";
            }

            // Release control of the lock
            mu_thermostats.unlock();
        }
    }

    // Send response to user
    send(clientSocket, response.c_str(), response.size(), 0);

    return succeeded;
}

// Not implemented yet
bool postDetails(SOCKET& clientSocket, std::string& requestDetails)
{
    // Message to send back to client
    std::string response;
    bool succeeded = false;

    // Extract the device type from the request details
    std::string deviceType = requestDetails.substr(0, requestDetails.find("/"));
    requestDetails = requestDetails.substr(requestDetails.find("/") + 1);
    // Extract the request type from the request details
    std::string requestType = requestDetails.substr(0, requestDetails.find("/"));
    requestDetails = requestDetails.substr(requestDetails.find("/") + 1);
    // Extract the username from the request details
    std::string username = requestDetails.substr(0, requestDetails.find("/"));
    requestDetails = requestDetails.substr(requestDetails.find("/") + 1);

    return succeeded;

}

int find_available_socket(void) {
    int socket_number = MAX_SOCKETS;
    for (int i = 0; i < MAX_SOCKETS; i++) {
        // Find the first available socket
        if (!Active_Sockets[i]) {
            socket_number = i;
            break;
        }
    }
    return socket_number;
}

void Run(int Index, const std::vector<std::string> usernames, const std::vector<std::string> passwords) {
    std::cout << "Thread Started at Index " << Index << std::endl;
    Active_Sockets[Index] = true;

    // Flag for ending the session
    bool keepGoing = true;

    // Accept messages from the client and send responses
    while (keepGoing) {
        char RxBuffer[128] = { };
        memset(RxBuffer, 0, sizeof(RxBuffer));
        recv(ClientSockets[Index], RxBuffer, sizeof(RxBuffer), 0);
        if (sizeof(RxBuffer) != 0)
        {
            std::string clientRequest(RxBuffer);
            std::string response;

            // Get the action from the client's request
            std::string action = clientRequest.substr(0, clientRequest.find("/"));
            std::string requestDetails = clientRequest.substr(clientRequest.find("/") + 1);

            /* Instead of the mess that is below, just extract the action word as a variable, and extract the request type as a variable (change get/l to get/al/L)*/

            // Process client choice
            if (action == "POST") {
                // Check if request has to do with user
                std::string type = requestDetails.substr(0, requestDetails.find("/"));
                requestDetails = requestDetails.substr(requestDetails.find("/") + 1);

                // Check if the type is user (trying to log in)
                if (type == "USER")
                {
                    signIn(ClientSockets[Index], requestDetails, usernames, passwords);
                }
            }
            else if (action == "GET") {
                // Request device's information
                getDetails(ClientSockets[Index], requestDetails);
            }
            else if (action == "PUT") {
                // Update device's information
                putDetails(ClientSockets[Index], requestDetails);
            }
            else if (action == "DELETE") {
                // Check if request has to do with user
                std::string type = requestDetails.substr(0, requestDetails.find("/"));
                requestDetails = requestDetails.substr(requestDetails.find("/") + 1);

                // Check if the type is user (trying to logout)
                if (type == "USER")
                {
                    signOut(ClientSockets[Index], requestDetails);
                }
            }
            else if (action == "End") {
                // Stop the connection on the server side 
                keepGoing = false;
            }
        }
    }

    // Close connection
    std::cout << "Closing Connection" << std::endl;
    closesocket(ClientSockets[Index]);
    Active_Sockets[Index] = false;
}

int main(int argc, char* argv[]) {
    // Hardcoded valid usernames 
    std::vector<std::string> usernames = { "Lucy", "Lockwood", "George", "Holly", "Kipps" };
    // Hardcoded passwords 
    std::vector<std::string> passwords = { "Password", "LucyStoleMyPassword", "LockwoodAndLucyHaveTerriblePasswords", "1tfhb5jfle083n", "WhyAmIFriendsWithChildren" };
    
    // Populate the data 
    load();

    int Socket_Number;

    // Attempt to start DLLs
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cout << "Could not start DLLs" << std::endl;
        return 0;
    }

    // Attempt to create a socket for listening out for clients
    SOCKET ListenSocket;
    ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ListenSocket == INVALID_SOCKET) {
        std::cout << "Could not create socket" << std::endl;
        WSACleanup();
        return 0;
    }

    struct sockaddr_in SvrAddr;
    SvrAddr.sin_family = AF_INET;
    SvrAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    SvrAddr.sin_port = htons(27000);
    if (bind(ListenSocket, (struct sockaddr*)&SvrAddr,
        sizeof(SvrAddr)) == SOCKET_ERROR) {
        std::cout << "Could not bind socket to port" << std::endl;
        closesocket(ListenSocket);
        WSACleanup();
        return 0;
    }

    if (listen(ListenSocket, 1) == SOCKET_ERROR) {
        std::cout << "Could not start to listen" << std::endl;
        closesocket(ListenSocket);
        WSACleanup();
        return 0;
    }

    while (true) {
        std::cout << "Ready to accept a connection" << std::endl;
        Aux_Socket = accept(ListenSocket, NULL, NULL);
        if (Aux_Socket == SOCKET_ERROR) {
            return 0;
        }
        else {
            // Check for available sockets
            Socket_Number = find_available_socket();
            if (Socket_Number < MAX_SOCKETS) {
                // There is an available socket
                ClientSockets[Socket_Number] = Aux_Socket;
                send(ClientSockets[Socket_Number], "Welcome",
                    sizeof("Welcome"), 0);
                // Make the connection
                std::thread(Run, Socket_Number, usernames, passwords).detach();
            }
            else {
                // Reject the cluent connection, no available sockets
                send(ClientSockets[MAX_SOCKETS], "Full",
                    sizeof("Full"), 0);
                std::cout << "Connection Fail" << std::endl;
            }
        }
    }

    // Clean up
    closesocket(ListenSocket);
    WSACleanup();
    return 0;
}