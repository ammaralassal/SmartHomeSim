
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <winsock2.h>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <bitset>
#include <sstream>
#include <memory>
#include <unordered_map>

#include "SmartDevices.h"
#include "Lights.h"
#include "SecurityCameras.h"
#include "Thermostat.h"
#pragma comment(lib, "Ws2_32.lib")


/// <summary>
/// Represents a static route in the routing table
/// </summary>
struct RouteEntry {
    std::string destinationNetwork; //CIDR Block
    std::string nextHop; // Next hop IP or "local"
};

/// <summary>
/// Static routing table entries
/// Lights, Camera, Thermostat
/// </summary>
std::vector<RouteEntry> routingTable = {
    {"192.168.1.0/28", "local"},
    {"192.168.2.0/29", "local"},
    {"192.168.3.0/30", "local"}
};

/// <summary>
/// Simulated ARP table for resolving IP addresses to MAC addresses. 
/// </summary>
std::unordered_map<std::string, std::string> arpTable = {
    {"192.168.1.1", "AA:BB:CC:DD:EE:01"},
    {"192.168.2.1", "AA:BB:CC:DD:EE:02"},
    {"192.168.3.1", "AA:BB:CC:DD:EE:03"}
};

// A mutex to lock the lights collection so that there are no race conditions
std::mutex mu_lightsCollection;
// A mutex to lock the cameras collection so that there are no race conditions
std::mutex mu_camerasCollection;
// A mutex to lock the thermostats collection so that there are no race conditions
std::mutex mu_thermostatsCollection;
// An unordered mapping of ip addresses to smart pointers to mutexes (so that they work with a vector) to lock the lights devices so that there are no race conditions
std::unordered_map<std::string, std::unique_ptr<std::mutex>> mu_lights;
// An unordered mapping of ip addresses to smart pointers to mutexes (so that they work with a vector) to lock the cameras devices so that there are no race conditions
std::unordered_map<std::string, std::unique_ptr<std::mutex>> mu_cameras;
// An unordered mapping of ip addresses to pointers to mutexes (so that they work with a vector) to lock the thermostats devices array so that there are no race conditions
std::unordered_map<std::string, std::unique_ptr<std::mutex>> mu_thermostats;
// A mutex to lock the logged in users collection so that there are no race conditions
std::mutex mu_loggedInUsers;
// An unordered mapping of ip addresses and smart lights in the home (shared resource across threads)
std::unordered_map<std::string, seneca::Lights> lights;
// An unordered mapping of ip addresses and security cameras in the home (shared resource across threads)
std::unordered_map<std::string, seneca::SecurityCameras> cameras;
// An unordered mapping of ip addresses and thermostats in the home (shared resource across threads)
std::unordered_map<std::string, seneca::Thermostat> thermostats;
// Vector of the logged in users for the server (shared resource across threads)
std::vector<std::string> loggedInUsers;



bool ipInSubnet(const std::string& ip, const std::string& network, int cidr) {
    auto ipToBits = [](const std::string& ipStr) { //Convert the IP Address into a 32-bit integer
        std::istringstream iss(ipStr); //Wrap the string in a stream, split by "."
        std::string token; //For each octet
        uint32_t ip = 0;
        //For each of the four octets, read one, shift the existing bits left by 8 and bitwise OR the current octet.
        for (int i = 0; i < 4; ++i) {
            std::getline(iss, token, '.');
            ip = (ip << 8) | std::stoi(token);
        }
        return ip; //Return the 32 bit representation.
        };
    //Convert both the device IP and the network address into a 32 bit form
    uint32_t ipBits = ipToBits(ip);
    uint32_t netBits = ipToBits(network);
    ///This creates a subnet via VLSM
    //i.e /24 -> 255.255.255.0 -> 0xFFFFFF00
    uint32_t mask = cidr == 0 ? 0 : ~((1 << (32 - cidr)) - 1);

    //Compare the subnet of the device and the subnet of the network. If they are equal, then the IP is inside that network.
    return (ipBits & mask) == (netBits & mask);
}

/// <summary>
/// Checks if a route exists to a given IP by matching its subnet prefix
/// </summary>
bool routeExists(const std::string& ip) {
    for (const auto& entry : routingTable) {
        std::string net = entry.destinationNetwork.substr(0, entry.destinationNetwork.find('/'));
        int cidr = std::stoi(entry.destinationNetwork.substr(entry.destinationNetwork.find('/') + 1));
        if (ipInSubnet(ip, net, cidr)) {
            return true;
        }
    }
    return false;
}

/// <summary>
/// Simulates ARP Resolution by looking up an IP address in the ARP table
/// </summary>
/// <param name="ip"></param>
/// <returns> The mac address of the device with the passed IP</returns>
std::string resolveMAC(const std::string& ip) {
    auto it = arpTable.find(ip);
    if (it != arpTable.end()) return it->second;
    return "00:00:00:00:00:00";
}

// Constant to track number of clients the server can handle
int const MAX_SOCKETS = 5;
SOCKET Aux_Socket;
SOCKET ClientSockets[MAX_SOCKETS + 1] = { SOCKET_ERROR };
bool Active_Sockets[MAX_SOCKETS + 1] = { false };

// Basic implementation for testing, not done
void load()
{
    // Just load one light for now for testing purposes
    lights["192.168.1.1"] = seneca::Lights("Bedroom", false, false, "192.168.1.1");
    mu_lights["192.168.1.1"] = std::make_unique<std::mutex>();
    // Just load one camera for now for testing purposes
    cameras["192.168.3.1"] = seneca::SecurityCameras("Kitchen", false, false, false, "192.168.3.1");
    mu_cameras["192.168.3.1"] = std::make_unique<std::mutex>();
    // Just load one thermostat for now for testing purposes
    thermostats["192.168.2.1"] = seneca::Thermostat("Living-Room", 18, 21, true, "192.168.2.1");
    mu_thermostats["192.168.2.1"] = std::make_unique<std::mutex>();
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
    std::string response;
    bool succeeded = false;

    // Parse request details
    std::string deviceType = requestDetails.substr(0, requestDetails.find("/"));
    requestDetails = requestDetails.substr(requestDetails.find("/") + 1);

    std::string requestType = requestDetails.substr(0, requestDetails.find("/"));
    requestDetails = requestDetails.substr(requestDetails.find("/") + 1);

    std::string username = requestDetails.substr(0, requestDetails.find("/"));
    requestDetails = requestDetails.substr(requestDetails.find("/") + 1);

    // Check if user is logged in
    if (isLoggedIn(username)) {
        response = "Failed. Server does not recognise the user as being logged in.";
        send(clientSocket, response.c_str(), response.size(), 0);
        return false;
    }

    // Extract the ip address
    std::string ip = requestDetails.substr(0, requestDetails.find("/"));
    requestDetails = requestDetails.substr(requestDetails.find("/") + 1);

    // Lambda to handle routing & ARP checks
    auto checkNetworkAccess = [&](const std::string& ip) -> bool {
        if (!routeExists(ip)) {
            response = "Failed. No route to device " + ip + ".";
            send(clientSocket, response.c_str(), response.size(), 0);
            return false;
        }
        std::string mac = resolveMAC(ip);
        std::cout << "Resolved IP " << ip << " to MAC " << mac << std::endl;
        if (mac == "00:00:00:00:00:00") {
            response = "Failed. MAC address could not be resolved for " + ip + ".";
            send(clientSocket, response.c_str(), response.size(), 0);
            return false;
        }
        return true;
        };

    // Lights
    if (deviceType == "L") {
        mu_lightsCollection.lock();
        if (requestType != "AL" && (lights.empty() || lights.find(ip) == lights.end())) {
            response = "Failed. That is not recognised as a valid light.";
        }
        else if (requestType != "AL" && !checkNetworkAccess(ip)) {
            mu_lightsCollection.unlock();
            return false;
        }
        else if (requestType == "AL") {
            if (!lights.empty()) {
                response = "Succeeded. ";
                succeeded = true;
                for (const auto& light : lights) response += light.second.getLocation() + " " + light.first + " ";
            }
            else {
                response = "Failed. There are no smart lights in the house.";
            }
        }
        else {
            response = "Succeeded. ";
            succeeded = true;
            if (requestType == "ON")
                response += "The light is " + std::string(lights[ip].getOn() ? "on." : "off.");
            else if (requestType == "LO")
                response += "The light is located in the " + lights[ip].getLocation() + ".";
            else if (requestType == "BO")
                response += "The light bulb has " + std::string(lights[ip].getBurnedOut() ? "burned out." : "not burned out.");
            else if (requestType == "ST")
                response += lights[ip].getStatus();
        }
        mu_lightsCollection.unlock();
    }

    // Cameras
    else if (deviceType == "C") {
        mu_camerasCollection.lock();
        if (requestType != "AL" && (cameras.empty() || cameras.find(ip) == cameras.end())) {
            response = "Failed. That is not recognised as a valid camera.";
        }
        else if (requestType != "AL" && !checkNetworkAccess(ip)) {
            mu_camerasCollection.unlock();
            return false;
        }
        else if (requestType == "AL") {
            if (!cameras.empty()) {
                response = "Succeeded. ";
                succeeded = true;
                for (const auto& cam : cameras) response += cam.second.getLocation() + " " + cam.first + " ";;
            }
            else {
                response = "Failed. There are no smart security cameras in the house.";
            }
        }
        else {
            response = "Succeeded. ";
            succeeded = true;
            if (requestType == "ON")
                response += "The camera is " + std::string(cameras[ip].getOn() ? "on." : "off.");
            else if (requestType == "LO")
                response += "The camera is located in the " + cameras[ip].getLocation() + ".";
            else if (requestType == "MA")
                response += "The camera is " + std::string(cameras[ip].getIsMotionActivated() ? "motion activated." : "not motion activated.");
            else if (requestType == "MF")
                response += "The camera's memory is " + std::string(cameras[ip].getMemoryIsFull() ? "full." : "not full.");
            else if (requestType == "ST")
                response += cameras[ip].getStatus();
        }
        mu_camerasCollection.unlock();
    }

    // Thermostats
    else if (deviceType == "T") {
        mu_thermostatsCollection.lock();
        if (requestType != "AL" && (thermostats.empty() || thermostats.find(ip) == thermostats.end())) {
            response = "Failed. That is not recognised as a valid thermostat.";
        }
        else if (requestType != "AL" && !checkNetworkAccess(thermostats[ip].getIPAddress())) {
            mu_thermostatsCollection.unlock();
            return false;
        }
        else if (requestType == "AL") {
            if (!thermostats.empty()) {
                response = "Succeeded. ";
                succeeded = true;
                for (const auto& t : thermostats) response += t.second.getLocation() + " " + t.first + " ";;
            }
            else {
                response = "Failed. There are no smart thermostats in the house.";
            }
        }
        else {
            response = "Succeeded. ";
            succeeded = true;
            if (requestType == "ON")
                response += "The thermostat is " + std::string(thermostats[ip].getOn() ? "on." : "off.");
            else if (requestType == "LO")
                response += "The thermostat is located in the " + thermostats[ip].getLocation() + ".";
            else if (requestType == "CT")
                response += "The current temperature is " + std::to_string(thermostats[ip].getCurrentTemperature()) + "C.";
            else if (requestType == "DT")
                response += "The desired/set temperature is " + std::to_string(thermostats[ip].getDesiredTemperature()) + "C.";
            else if (requestType == "ST")
                response += thermostats[ip].getStatus();
        }
        mu_thermostatsCollection.unlock();
    }

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

    std::string response;
    bool succeeded = false;

    // Extract request components
    std::string deviceType = requestDetails.substr(0, requestDetails.find("/"));
    std::cout << "[DEBUG] Raw Request: '" << requestDetails << "'" << std::endl;
    requestDetails = requestDetails.substr(requestDetails.find("/") + 1);

    // Parse the requestType so it doesn't break subsequent runs...
    std::string requestType = requestDetails.substr(0, requestDetails.find("/"));
    std::cout << "[DEBUG] Request Type: '" << requestType << "'" << std::endl;
    requestType.erase(remove_if(requestType.begin(), requestType.end(), ::isspace), requestType.end());
    requestDetails = requestDetails.substr(requestDetails.find("/") + 1);

    std::string username = requestDetails.substr(0, requestDetails.find("/"));
    requestDetails = requestDetails.substr(requestDetails.find("/") + 1);

    //Check login
    if (isLoggedIn(username)) {
        response = "Failed. Server does not recognise the user as being logged in.";
        send(clientSocket, response.c_str(), response.size(), 0);
        return false;
    }

    //Extract device number
    std::string ip = requestDetails.substr(0, requestDetails.find("/"));
    requestDetails = requestDetails.substr(requestDetails.find("/") + 1);

    //Lambda for network checks
    auto checkNetworkAccess = [&](const std::string& ip) -> bool {
        if (!routeExists(ip)) {
            response = "Failed. No route to device " + ip + ".";
            send(clientSocket, response.c_str(), response.size(), 0);
            return false;
        }
        std::string mac = resolveMAC(ip);
        std::cout << "Resolved IP " << ip << " to MAC " << mac << std::endl;
        if (mac == "00:00:00:00:00:00") {
            response = "Failed. MAC address could not be resolved for " + ip + ".";
            send(clientSocket, response.c_str(), response.size(), 0);
            return false;
        }
        return true;
        };


    // Lights
    if (deviceType == "L") {
        mu_lightsCollection.lock();
        if (lights.empty() || lights.find(ip) == lights.end()) {
            response = "Failed. That is not recognised as a valid light.";
        }
        else if (!checkNetworkAccess(ip)) {
            mu_lightsCollection.unlock();
            return false;
        }
        else {
            succeeded = true;
            std::cout << "[DEBUG] Request Type: '" << requestType << "'" << std::endl;
            if (requestType == "ON") {
                lights[ip].turnOn();
                response = "Succeeded. Light turned on.";
            }
            else if (requestType == "OF") {
                lights[ip].turnOff();
                response = "Succeeded. Light turned off.";
            }
            else if (requestType == "RB") {
                lights[ip].replaceBulb();
                response = "Succeeded. Bulb replaced.";
            }
            else if (requestType == "LK") {
                // Attempt to lock the device
                if (mu_lights[ip]->try_lock()){
                    // This device was locked and the user can now examine it or make modifications to it
                    response += "Succeeded. Loading Light.";
                    succeeded = true;
                }
                else {
                    response += "Failed. This light is already being examined by another user, try again later.";
                }
            }
            else if (requestType == "UL") {
                // Unlock the device
                mu_lights[ip]->unlock();
                response += "Succeeded. Light has been released.";
                succeeded = true;
            }
            else {
                succeeded = false;
                response = "Failed. Unknown request type for light.";
            }
        }
        mu_lightsCollection.unlock();
    }

    // Cameras
    else if (deviceType == "C") {
        mu_camerasCollection.lock();
        if (cameras.empty() || cameras.find(ip) == cameras.end()) {
            response = "Failed. That is not recognised as a valid camera.";
        }
        else if (!checkNetworkAccess(ip)) {
            mu_camerasCollection.unlock();
            return false;
        }
        else {
            succeeded = true;
            if (requestType == "ON") {
                cameras[ip].turnOn();
                response = "Succeeded. Camera turned on.";
            }
            else if (requestType == "OF") {
                cameras[ip].turnOff();
                response = "Succeeded. Camera turned off.";
            }
            else if (requestType == "WM") {
                cameras[ip].wipeMemory();
                response = "Succeeded. Camera memory wiped.";
            }
            else if (requestType == "LK") {
                // Attempt to lock the device
                if (mu_cameras[ip]->try_lock()) {
                    // This device was locked and the user can now examine it or make modifications to it
                    response += "Succeeded. Loading Camera.";
                    succeeded = true;
                }
                else {
                    response += "Failed. This security camera is already being examined by another user, try again later.";
                }
            }
            else if (requestType == "UL") {
                // Unlock the device
                mu_cameras[ip]->unlock();
                response += "Succeeded. Camera has been released.";
                succeeded = true;
            }
            else {
                succeeded = false;
                response = "Failed. Unknown request type for camera.";
            }
        }
        mu_camerasCollection.unlock();
    }

    // Thermostats
    else if (deviceType == "T") {
        mu_thermostatsCollection.lock();
        if (thermostats.empty() || thermostats.find(ip) == thermostats.end()) {
            response = "Failed. That is not recognised as a valid thermostat.";
        }
        else if (!checkNetworkAccess(ip)) {
            mu_thermostatsCollection.unlock();
            return false;
        }
        else {
            succeeded = true;
            if (requestType == "ON") {
                thermostats[ip].turnOn();
                response = "Succeeded. Thermostat turned on.";
            }
            else if (requestType == "OF") {
                thermostats[ip].turnOff();
                response = "Succeeded. Thermostat turned off.";
            }
            else if (requestType == "ST") {
                int newTemp = std::stoi(requestDetails);
                if (thermostats[ip].setDesiredTemperature(newTemp)) {
                    response = "Succeeded. Desired temperature set to " + std::to_string(newTemp) + "C.";
                }
                else {
                    succeeded = false;
                    response = "Failed. Temperature must be between 2C and 35C.";
                }
            }
            else if (requestType == "LK") {
                // Attempt to lock the device
                if (mu_thermostats[ip]->try_lock()) {
                    // This device was locked and the user can now examine it or make modifications to it
                    response += "Succeeded. Loading Thermostat.";
                    succeeded = true;
                }
                else {
                    response += "Failed. This thermostat is already being examined by another user, try again later.";
                }
            }
            else if (requestType == "UL") {
                // Unlock the device
                mu_thermostats[ip]->unlock();
                response += "Succeeded. Thermostat has been released.";
                succeeded = true;
            }
            else {
                succeeded = false;
                response = "Failed. Unknown request type for thermostat.";
            }
        }
        mu_thermostatsCollection.unlock();
    }

    // Send response
    send(clientSocket, response.c_str(), response.size(), 0);
    return succeeded;
}

// Not implemented yet
bool postDetails(SOCKET& clientSocket, std::string& requestDetails)
{
    std::string response;
    bool succeeded = false;

    // Parse: POST/<deviceType>/<username>/<...>
    std::string deviceType = requestDetails.substr(0, requestDetails.find("/"));
    requestDetails = requestDetails.substr(requestDetails.find("/") + 1);

    std::string username = requestDetails.substr(0, requestDetails.find("/"));
    requestDetails = requestDetails.substr(requestDetails.find("/") + 1);

    if (isLoggedIn(username)) {
        response = "Failed. Server does not recognise the user as being logged in.";
        send(clientSocket, response.c_str(), response.size(), 0);
        return false;
    }

    auto parseNext = [&](std::string& str) -> std::string {
        std::string val = str.substr(0, str.find("/"));
        str = str.substr(str.find("/") + 1);
        return val;
        };

    std::string location, ip;
    location = parseNext(requestDetails);
    ip = parseNext(requestDetails);

    // Check for duplicate IP
    if (arpTable.find(ip) != arpTable.end()) {
        response = "Failed. Device with that IP already exists.";
        send(clientSocket, response.c_str(), response.size(), 0);
        return false;
    }

    if (!routeExists(ip)) {
        response = "Failed. IP does not match any valid subnet.";
        send(clientSocket, response.c_str(), response.size(), 0);
        return false;
    }

    std::string mac = "AA:BB:CC:DD:EE:" + std::to_string(rand() % 90 + 10);  // Simulate MAC gen
    arpTable[ip] = mac;

    if (deviceType == "L") {
        mu_lightsCollection.lock();
        lights[ip] = seneca::Lights(location, false, false, ip);
        mu_lights[ip] = std::make_unique<std::mutex>();
        mu_lightsCollection.unlock();
        response = "Succeeded. Light added at " + location;
        succeeded = true;
    }
    else if (deviceType == "C") {
        mu_camerasCollection.lock();
        cameras[ip] = seneca::SecurityCameras(location, false, false, false, ip);
        mu_cameras[ip] = std::make_unique<std::mutex>();
        mu_camerasCollection.unlock();
        response = "Succeeded. Camera added at " + location;
        succeeded = true;
    }
    else if (deviceType == "T") {
        int curTemp = std::stoi(parseNext(requestDetails));
        int desTemp = std::stoi(parseNext(requestDetails));

        mu_thermostatsCollection.lock();
        thermostats[ip] = seneca::Thermostat(location, curTemp, desTemp, false, ip);
        mu_thermostats[ip] = std::make_unique<std::mutex>();
        mu_thermostatsCollection.unlock();
        response = "Succeeded. Thermostat added at " + location;
        succeeded = true;
    }
    else {
        response = "Failed. Invalid device type.";
    }

    send(clientSocket, response.c_str(), response.size(), 0);
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

void Run(int Index, const std::vector<std::string> usernames, const std::vector<std::string> passwords)
{
    std::cout << "Thread Started at Index " << Index << std::endl;
    Active_Sockets[Index] = true;

    char RxBuffer[128];
    bool keepGoing = true;

    while (keepGoing) {
        memset(RxBuffer, 0, sizeof(RxBuffer));

        // Receive message from client
        int bytesReceived = recv(ClientSockets[Index], RxBuffer, sizeof(RxBuffer), 0);

        // Check if we received anything
        if (bytesReceived > 0) {
            std::string clientRequest(RxBuffer);
            std::string response;

            // Extract the action keyword (e.g., GET, PUT, POST, DELETE, End)
            std::string action = clientRequest.substr(0, clientRequest.find("/"));
            std::string requestDetails = clientRequest.substr(clientRequest.find("/") + 1);

            if (action == "POST") {
                std::string type = requestDetails.substr(0, requestDetails.find("/"));
                requestDetails = requestDetails.substr(requestDetails.find("/") + 1);

                if (type == "USER") {
                    signIn(ClientSockets[Index], requestDetails, usernames, passwords);
                }
                else {
                    postDetails(ClientSockets[Index], requestDetails);
                }
            }
            else if (action == "GET") {
                getDetails(ClientSockets[Index], requestDetails);
            }
            else if (action == "PUT") {
                putDetails(ClientSockets[Index], requestDetails);
            }
            else if (action == "DELETE") {
                std::string type = requestDetails.substr(0, requestDetails.find("/"));
                requestDetails = requestDetails.substr(requestDetails.find("/") + 1);

                if (type == "USER") {
                    signOut(ClientSockets[Index], requestDetails);
                }
            }
            else if (action == "End") {
                keepGoing = false;
            }
            else {
                std::string error = "Failed. Invalid request format.";
                send(ClientSockets[Index], error.c_str(), error.size(), 0);
            }
        }
    }

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
            WSACleanup();
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