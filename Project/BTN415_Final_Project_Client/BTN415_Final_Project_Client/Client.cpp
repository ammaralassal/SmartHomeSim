#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <winsock2.h>
#include <string>
#include <vector>
#pragma comment(lib, "Ws2_32.lib")

/// <summary>
/// Get a valid menu choice from the user
/// </summary>
/// <param name="min">The minimum number value the user could have entered</param>
/// <param name="max">The maximum number value the user could have entered</param>
/// <returns>The choice as a string</returns>
std::string getMenuChoice(const int& min, const int& max)
{
    std::string choice = "";
    // Collect a valid choice from the user (assume it's not valid)
    bool validChoice = false;
    while (!validChoice)
    {
        // Collect a valid choice from the user
        std::cout << "Enter your choice (number): ";
        std::cin >> choice;

        // Check if choice was valid
        if (std::stoi(choice) < min || std::stoi(choice) > max)
        {
            // Choice was invalid, inform user and try again
            std::cout << "Whoops, " << choice << " wasn't a valid option. Please try again with a number between " << min << " and " << max << ", inclusive.\n\n";
        }
        else
        {
            // The user's choice was valid
            validChoice = true;
        }
    }

    return choice;
}


/// <summary>
/// Send a request to the server for information pertaining to a specific device
/// </summary>
/// <param name="deviceType">The type of smart device being queired</param>
/// <param name="requestType">The type of information being requested</param>
/// <param name="ClientSocket">The socket</param>
/// <param name="username">The user making the request</param>
/// <param name="ip">The ip address of the device</param>
/// <returns>True if request for information could be completed; false otherwise</returns>
bool getDetails(const std::string& deviceType, const std::string& requestType, SOCKET& ClientSocket, const std::string& username, const std::string& ip)
{
    // Flag to indicate if request succedded (assume failure)
    bool succeeded = false;

    // Make a 'GET' request to see if the device is on
    std::string requestString = "GET/" + deviceType + "/" + requestType + "/" + username + "/" + ip;

    // Send request to server
    send(ClientSocket, requestString.c_str(), requestString.length(), 0);

    // Response buffer
    char buffer[1024] = { 0 };
    // Receive and display response
    recv(ClientSocket, buffer, 1024, 0);
    std::string serverResponse(buffer);

    // Break up the response message for interpretation
    std::string result = serverResponse.substr(0, serverResponse.find("."));
    std::string details = serverResponse.substr(serverResponse.find(".") + 2);

    if (result == "Succeeded")
    {
        // Request succeeded
        succeeded = true;

        // Inform user
        std::cout << std::endl << details << std::endl << std::endl;
    }
    else
    {
        // Request failed, inform user of why
        std::cout << "\nThe request for information failed." << details << std::endl << std::endl;
    }

    return succeeded;
}

/// <summary>
/// Sends put requests to the sever to update the state of a smart device
/// </summary>
/// <param name="deviceType">The type of device to be updated</param>
/// <param name="requestType">The type of request being made</param>
/// <param name="ClientSocket">The socket</param>
/// <param name="username">The user making the request</param>
/// <param name="ip">The ip address of the device</param>
/// <returns>True if device was successfully updated; false otherwise</returns>
bool putDetails(const std::string& deviceType, const std::string& requestType, SOCKET& ClientSocket, const std::string& username, const std::string& ip)
{
    // Flag to indicate if request succedded (assume failure)
    bool succeeded = false;

    // Make a 'GET' request to see if the device is on
    std::string requestString = "PUT/" + deviceType + "/" + requestType + "/" + username + "/"  + ip;

    if (requestType == "ST")
    {
        // User wants to set a new temperature, prompt for new temperature
        std::cout << "\nWhat temperature would you like to set the thermostat for? Please note, your temperature must be a whole number between 2 and 35 degrees C.\n";
        // Collect the user's choice
        requestString += "/" + getMenuChoice(2, 35);
    }

    // Send request to server
    send(ClientSocket, requestString.c_str(), requestString.length(), 0);

    // Response buffer
    char buffer[1024] = { 0 };
    // Receive and display response
    recv(ClientSocket, buffer, 1024, 0);
    std::string serverResponse(buffer);

    // Break up the response message for interpretation
    std::string result = serverResponse.substr(0, serverResponse.find("."));
    std::string details = serverResponse.substr(serverResponse.find(".") + 2);

    if (result == "Succeeded")
    {
        // Request succeeded
        succeeded = true;

        // Inform user
        std::cout << std::endl << details << std::endl << std::endl;
    }
    else
    {
        // Request failed, inform user of why
        std::cout << "\nThe action has failed. " << details << std::endl << std::endl;
    }

    return succeeded;
}

/// <summary>
/// Try to login the user to the server
/// </summary>
/// <param name="clientSocket">The socket</param>
/// <param name="username">The username of the currently logged-in accout</param>
/// <returns>True if login succeeds; False otherwise</returns>
bool login(SOCKET& clientSocket, std::string& username) {
    // Flag to indicate if login succedded (assume failure)
    bool succeeded = false;

    if (username == "")
    {
        // User is not currently logged in, a log-in attempt can be made, store the request into a string
        std::string requestString = "POST/USER/";
        std::string inputUsername, inputPassword;

        // Collect the username
        std::cout << "Username: ";
        std::cin >> inputUsername;
        requestString += inputUsername + "/";

        // Collect the password
        std::cout << "Password: ";
        std::cin >> inputPassword;
        requestString += inputPassword;

        // Send choice to server
        send(clientSocket, requestString.c_str(), requestString.length(), 0);

        // Response buffer
        char buffer[1024] = { 0 };
        // Receive and display response
        recv(clientSocket, buffer, 1024, 0);
        std::string serverResponse(buffer);

        // Break up the response message for interpretation
        std::string result = serverResponse.substr(0, serverResponse.find("."));
        std::string explaination = serverResponse.substr(serverResponse.find(".") + 1);

        if (result == "Succeeded")
        {
            // Login succeeded, inform user
            std::cout << "\nCongradulations, the login succeeded!" << explaination << std::endl << std::endl;
            succeeded = true;
            username = inputUsername;
        }
        else
        {
            // Login failed, inform user of why
            std::cout << "\nThe log in failed and you have not been logged in." << explaination << std::endl << std::endl;
        }

    }
    else
    {
        // User is already logged in, inform them
        std::cout << "Sorry " << username << ", but you're already logged in. Please log out before attempting to login again." << std::endl;
    }

    return succeeded;
}

/// <summary>
/// Allows the user to preform actions on a specific light
/// </summary>
/// <param name="ClientSocket">The socket</param>
/// <param name="username">The user trying to preform the actions</param>
/// <param name="">The light that was seclected', stored as a string's ip address</param>
void light(SOCKET& ClientSocket, const std::string& username, const std::string& ip)
{
    // String for storing the user's choice
    std::string choice = "";

    // Prompt user for actions until they are done with the light (enter 0)
    while (choice != "0")
    {
        // Display options
        std::cout << "What would you like to do with this light?\n1. Check if on\n2. Turn on\n3. Turn off\n4. Check if bulb has burned out\n5. Replace bulb\n6. Check location\n7. Get full status report\n";
        // Prompt for input
        std::cout << "\nPlease enter the number corresponding to your request, or 0 to return to the Lights Menu.\n";
        // Collect the user's choice
        choice = getMenuChoice(0, 7);

        switch (std::stoi(choice)) {
        case 1:
            // Check if the light is on
            getDetails ("L", "ON", ClientSocket, username, ip);
            break;
        case 2:
            // Try to turn the light on
            putDetails("L", "ON", ClientSocket, username, ip);
            break;
        case 3:
            // Try to turn the light off
            putDetails("L", "OF", ClientSocket, username, ip);
            break;
        case 4:
            // Check if the bulb has burned out
            getDetails("L", "BO", ClientSocket, username, ip);
            break;
        case 5:
            // Try to replace the bulb
            putDetails("L", "RB", ClientSocket, username, ip);
            break;
        case 6:
            // Check if the location
            getDetails("L", "LO", ClientSocket, username, ip);
            break;
        case 7:
            // Get a full status report on the bulb
            getDetails("L", "ST", ClientSocket, username, ip);
            break;
        }
    }
    // Release the lock on the light
    putDetails("L", "UL", ClientSocket, username, ip);

    // Return to main menu
    std::cout << "\nReturning to Lights menu...\n\n";
}

/// <summary>
/// Allows the user to preform actions on a specific thermostat
/// </summary>
/// <param name="ClientSocket">The socket</param>
/// <param name="username">The user trying to preform the actions</param>
/// <param name="ip">The thermostat that was seclected's ip address</param>
void thermostat(SOCKET& ClientSocket, const std::string& username, const std::string& ip)
{
    // String for storing the user's choice
    std::string choice = "";

    // Prompt user for actions until they are done with the thermostat (enter 0)
    while (choice != "0")
    {
        // Display options
        std::cout << "What would you like to do with this thermostat?\n1. Check if on\n2. Turn on\n3. Turn off\n4. Check the current temperature\n5. Check the set temperature\n6. Set a new temperature\n7. Check location\n8. Get full status report\n";
        // Prompt for input
        std::cout << "\nPlease enter the number corresponding to your request, or 0 to return to the Thermostat Menu.\n";
        // Collect the user's choice
        choice = getMenuChoice(0, 8);

        switch (std::stoi(choice)) {
        case 1:
            // Check if the thermostat is on
            getDetails("T", "ON", ClientSocket, username, ip);
            break;
        case 2:
            // Try to turn the thermostat on
            putDetails("T", "ON", ClientSocket, username, ip);
            break;
        case 3:
            // Try to turn the thermostat off
            putDetails("T", "OF", ClientSocket, username, ip);
            break;
        case 4:
            // Check the current temperature
            getDetails("T", "CT", ClientSocket, username, ip);
            break;
        case 5:
            // Check the desired temperature
            getDetails("T", "DT", ClientSocket, username, ip);
            break;
        case 6:
            // Change the desired temperature
            putDetails("T", "ST", ClientSocket, username, ip);
            break;
        case 7:
            // Check if the location
            getDetails("T", "LO", ClientSocket, username, ip);
            break;
        case 8:
            // Get a full status report on the thermostat
            getDetails("T", "ST", ClientSocket, username, ip);
            break;
        }
    }
    // Release the lock on the thermostat
    putDetails("T", "UL", ClientSocket, username, ip);

    // Return to main menu
    std::cout << "\nReturning to Thermostat menu...\n\n";
}

/// <summary>
/// Allows the user to preform actions on a specific camera
/// </summary>
/// <param name="ClientSocket">The socket</param>
/// <param name="username">The user preforming the actions</param>
/// <param name="ip">The camera that was selcted', stored as a string's ip address</param>
void camera(SOCKET& ClientSocket, const std::string& username, const std::string& ip)
{
    // String for storing the user's choice
    std::string choice = "";

    // Prompt user for actions until they are done with the camera (enter 0)
    while (choice != "0")
    {
        // Display options
        std::cout << "What would you like to do with this security camera?\n1. Check if on\n2. Turn on\n3. Turn off\n4. Check if memory is full\n5. Empty memory\n6. Check location\n7. Check if motion activated\n8. Get full status report\n";
        // Prompt for input
        std::cout << "\nPlease enter the number corresponding to your request, or 0 to return to the Cameras Menu.\n";
        // Collect the user's choice
        choice = getMenuChoice(0, 8);

        switch (std::stoi(choice)) {
        case 1:
            // Check if the camera is on
            getDetails("C", "ON", ClientSocket, username, ip);
            break;
        case 2:
            // Try to turn the camera on
            putDetails("C", "ON", ClientSocket, username, ip);
            break;
        case 3:
            // Try to turn the camera off
            putDetails("C", "OF", ClientSocket, username, ip);
            break;
        case 4:
            // Check the memory is full
            getDetails("C", "MF", ClientSocket, username, ip);
            break;
        case 5:
            // Try to empty the memory
            putDetails("C", "WM", ClientSocket, username, ip);
            break;
        case 6:
            // Check the location
            getDetails("C", "LO", ClientSocket, username, ip);
            break;
        case 7:
            // Check if the camera is motion activated
            getDetails("C", "MA", ClientSocket, username, ip);
            break; 
        case 8:
            // Get a full status report on the camera
            getDetails("C", "ST", ClientSocket, username, ip);
            break;
        }
    }
    // Release the lock on the camera
    putDetails("C", "UL", ClientSocket, username, ip);

    // Return to main menu
    std::cout << "\nReturning to Cameras menu...\n\n";
}

/// <summary>
/// Gets all the light information from the server and propmts user to pick a light to preform actions on
/// </summary>
/// <param name="ClientSocket">The client</param>
/// <param name="username">The user trying to preform actions</param>
/// <returns>True if able to get information on the lights; false otherwise</returns>
bool lights(SOCKET& ClientSocket, std::string& username)
{
    // Flag to indicate if light request succedded (assume failure)
    bool succeeded = false;

    // Make a 'GET' request for the light information (ip address is 0 as a placeholder)
    std::string requestString = "GET/L/AL/" + username + "/0";

    // Send request to server
    send(ClientSocket, requestString.c_str(), requestString.length(), 0);

    // Response buffer
    char buffer[1024] = { 0 };
    // Receive and display response
    recv(ClientSocket, buffer, 1024, 0);
    std::string serverResponse(buffer);

    // Break up the response message for interpretation
    std::string result = serverResponse.substr(0, serverResponse.find("."));
    std::string details = serverResponse.substr(serverResponse.find(".") + 2);

    if (result == "Succeeded")
    {
        // Request succeeded
        succeeded = true;

        // Organise data into a human readable menu
        std::string lightsMenu = "Lights:\n";
        int count = 0;

        // Store the ip addresses of the devices
        std::vector<std::string> ipAddresses;

        // Get all the lights from the details
        while (!(details == " " || details == ""))
        {
            // Add the light number to the menu
            lightsMenu += std::to_string(++count) + ". Light in ";

            // Get the location 
            int indexOfSpace = details.find(" ");
            lightsMenu += details.substr(0, indexOfSpace);

            // Remove the light location from the string
            details = details.substr(indexOfSpace + 1);

            // Add a newline character for formatting
            lightsMenu += "\n";

            // Record the ip of the light and remove it from the string
            indexOfSpace = details.find(" ");
            ipAddresses.push_back(details.substr(0, indexOfSpace));
            details = details.substr(indexOfSpace + 1);
        }

        // Menu ending
        lightsMenu += "\nPlease enter the light number you would like to examine further, or 0 to go back.\n";

        // String for storing the user's choice
        std::string choice = "";

        // Prompt user for actions until they are done with lights (enter 0)
        while (choice != "0")
        {
            // Display menu 
            std::cout << lightsMenu;
            // Collect the user's choice
            choice = getMenuChoice(0, count);

            // Add an extra newline to improve readability
            std::cout << std::endl;

            if (choice != "0")
            {
                // Try to put a lock on the light
                if (putDetails("L", "LK", ClientSocket, username, ipAddresses[std::stoi(choice) - 1]))
                {
                    // A light has successfully been selected, handle requests pertaining to that light
                    light(ClientSocket, username, ipAddresses[std::stoi(choice) - 1]);
                }
                else
                {
                    // Could not lock device, clear the choice so the user can try again
                    choice = "";
                }
            }
            else
            {
                // Return to main menu
                std::cout << "Returning to main menu...\n\n";
            }
        }
        
    }
    else
    {
        // Request failed, inform user of why
        std::cout << "\nThe request for the light information failed. " << details << std::endl << std::endl;
    }

    return succeeded;
}

/// <summary>
/// Gets all the thermostat information from the server and propmts user to pick a thermostat to preform actions on
/// </summary>
/// <param name="ClientSocket">The client</param>
/// <param name="username">The user trying to preform actions</param>
/// <returns>True if able to get information on the thermostats; false otherwise</returns>
bool thermostats(SOCKET& ClientSocket, std::string& username)
{
    // Flag to indicate if thermostat request succedded (assume failure)
    bool succeeded = false;

    // Make a 'GET' request for the thermostat information device number is 0
    std::string requestString = "GET/T/AL/" + username + "/0";

    // Send request to server
    send(ClientSocket, requestString.c_str(), requestString.length(), 0);

    // Response buffer
    char buffer[1024] = { 0 };
    // Receive and display response
    recv(ClientSocket, buffer, 1024, 0);
    std::string serverResponse(buffer);

    // Break up the response message for interpretation
    std::string result = serverResponse.substr(0, serverResponse.find("."));
    std::string details = serverResponse.substr(serverResponse.find(".") + 2);

    if (result == "Succeeded")
    {
        // Request succeeded
        succeeded = true;

        // Organise data into a human readable menu
        std::string menu = "Thermostats:\n";
        int count = 0;

        // Store the ip addresses of the devices
        std::vector<std::string> ipAddresses;

        // Get all the thermostats from the details
        while (!(details == " " || details == ""))
        {
            // Add the thermostat number to the menu
            menu += std::to_string(++count) + ". Thermostat in the ";

            // Get the location 
            int indexOfSpace = details.find(" ");
            menu += details.substr(0, indexOfSpace);

            // Remove the thermostat location from the string
            details = details.substr(indexOfSpace + 1);

            // Add a newline character for formatting
            menu += "\n";

            // Record the ip of the thermostat and remove it from the string
            indexOfSpace = details.find(" ");
            ipAddresses.push_back(details.substr(0, indexOfSpace));
            details = details.substr(indexOfSpace + 1);
        }

        // Menu ending
        menu += "\nPlease enter the thermostat number you would like to examine further, or 0 to go back.\n";

        // String for storing the user's choice
        std::string choice = "";

        // Prompt user for actions until they are done with thermostats (enter 0)
        while (choice != "0")
        {
            // Display menu 
            std::cout << menu;
            // Collect the user's choice
            choice = getMenuChoice(0, count);

            // Add an extra newline to improve readability
            std::cout << std::endl;

            if (choice != "0")
            {
                // Try to put a lock on the thermostat
                if (putDetails("T", "LK", ClientSocket, username, ipAddresses[std::stoi(choice) - 1]))
                {
                    // A thermostat has successfully been selected, handle requests pertaining to that thermostat
                    thermostat(ClientSocket, username, ipAddresses[std::stoi(choice) -  1]);
                }
                else
                {
                    // Could not lock device, clear the choice so the user can try again
                    choice = "";
                }
            }
            else
            {
                // Return to main menu
                std::cout << "Returning to main menu...\n\n";
            }
        }

    }
    else
    {
        // Request failed, inform user of why
        std::cout << "\nThe request for the thermostat information failed. " << details << std::endl << std::endl;
    }

    return succeeded;
}

/// <summary>
/// Gets all the camera information from the server and prompts the user to pick a camera to preferom actions on
/// </summary>
/// <param name="ClientSocket">The socket</param>
/// <param name="username">The user making the requests</param>
/// <returns>True if able to get information on the cameras; false otherwise</returns>
bool cameras(SOCKET& ClientSocket, std::string& username)
{
    // Flag to indicate if camera request succedded (assume failure)
    bool succeeded = false;

    // Make a 'GET' request for the camera information (device number is 0)
    std::string requestString = "GET/C/AL/" + username + "/0";

    // Send request to server
    send(ClientSocket, requestString.c_str(), requestString.length(), 0);

    // Response buffer
    char buffer[1024] = { 0 };
    // Receive and display response
    recv(ClientSocket, buffer, 1024, 0);
    std::string serverResponse(buffer);

    // Break up the response message for interpretation
    std::string result = serverResponse.substr(0, serverResponse.find("."));
    std::string details = serverResponse.substr(serverResponse.find(".") + 2);

    if (result == "Succeeded")
    {
        // Request succeeded
        succeeded = true;

        // Organise data into a human readable menu
        std::string menu = "Cameras:\n";
        int count = 0;

        // Store the ip addresses of the devices
        std::vector<std::string> ipAddresses;

        // Get all the cameras from the details
        while (!(details == " " || details == ""))
        {
            // Add the camera number to the menu
            menu += std::to_string(++count) + ". Security camera in the ";

            // Get the location 
            int indexOfSpace = details.find(" ");
            menu += details.substr(0, indexOfSpace);

            // Remove the camera location from the string
            details = details.substr(indexOfSpace + 1);

            // Add a newline character for formatting
            menu += "\n";

            // Record the ip of the camera and remove it from the string
            indexOfSpace = details.find(" ");
            ipAddresses.push_back(details.substr(0, indexOfSpace));
            details = details.substr(indexOfSpace + 1);
        }

        // Menu ending
        menu += "\nPlease enter the camera number you would like to examine further, or 0 to go back.\n";

        // String for storing the user's choice
        std::string choice = "";

        // Prompt user for actions until they are done with cameras (enter 0)
        while (choice != "0")
        {
            // Display menu 
            std::cout << menu;
            // Collect the user's choice
            choice = getMenuChoice(0, count);

            // Add an extra newline to improve readability
            std::cout << std::endl;

            if (choice != "0")
            {
                // Try to put a lock on the camera
                if (putDetails("C", "LK", ClientSocket, username, ipAddresses[std::stoi(choice) - 1]))
                {
                    // A camera has successfully been selected, handle requests pertaining to that camera
                    camera(ClientSocket, username, ipAddresses[std::stoi(choice) - 1]);
                }
                else
                {
                    // Could not lock device, clear the choice so the user can try again
                    choice = "";
                }
            }
            else
            {
                // Return to main menu
                std::cout << "Returning to main menu...\n\n";
            }
        }

    }
    else
    {
        // Request failed, inform user of why
        std::cout << "\nThe request for the camera information failed. " << details << std::endl << std::endl;
    }

    return succeeded;
}

/// <summary>
/// Attempt to log the user out
/// </summary>
/// <param name="ClientSocket">The socket</param>
/// <param name="username">The user who wants to log out</param>
/// <returns>True to indicate a sucessful logout; false otherwise</returns>
bool logout(SOCKET& ClientSocket, std::string& username)
{
    // Flag to indicate if logout succedeed (assume failure)
    bool succeeded = false;

    if (username == "")
    {
        // No user is logged in, cannot log anyone out
        std::cout << "Sorry, but you have to be logged in to log out. Please log in and try again." << std::endl;
    }
    else
    {
        // User is currently logged in, a log-out attempt can be made, store the request into a string
        std::string requestString = "DELETE/USER/" + username;

        // Send choice to server
        send(ClientSocket, requestString.c_str(), requestString.length(), 0);

        // Response buffer
        char buffer[1024] = { 0 };
        // Receive and display response
        recv(ClientSocket, buffer, 1024, 0);
        std::string serverResponse(buffer);

        // Break up the response message for interpretation
        std::string result = serverResponse.substr(0, serverResponse.find("."));
        std::string explanation = serverResponse.substr(serverResponse.find(".") + 1);

        if (result == "Succeeded")
        {
            // Logout succeeded, inform user
            std::cout << "\nYou have been logged out" << explanation << std::endl << std::endl;
            succeeded = true;
            username = "";

            // End the session
            requestString = "End";
            send(ClientSocket, requestString.c_str(), requestString.length(), 0);
            memset(buffer, 0, sizeof(buffer));
            recv(ClientSocket, buffer, sizeof(buffer), 0);
        }
        else
        {
            // Logout failed, inform user of why
            std::cout << "\nThe logout failed. Try again later." << explanation << std::endl << std::endl;
        }
    }

    return succeeded;
}

int main() {
    // Username of the logged in user (starts as empty string, not logged in)
    std::string username = "";

    // Attempt to start DLLs
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cout << "Could not start DLLs" << std::endl;
        return 0;
    }

    // Attempt to create a clinet socket
    SOCKET ClientSocket;
    ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ClientSocket == INVALID_SOCKET) {
        std::cout << "Could not create socket" << std::endl;
        WSACleanup();
        return 0;
    }

    struct sockaddr_in SvrAddr;
    SvrAddr.sin_family = AF_INET;
    SvrAddr.sin_port = htons(27000);
    SvrAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    //SvrAddr.sin_addr.s_addr = inet_addr("159.203.26.94");
    // Try to connect to server
    if ((connect(ClientSocket, (struct sockaddr*)&SvrAddr,
        sizeof(SvrAddr))) == SOCKET_ERROR) {
        std::cout << "Failed to connect to server" << std::endl;
        closesocket(ClientSocket);
        WSACleanup();
        return 0;
    }

    // Recieve opening message from server
    char RxBuffer[128] = {};
    recv(ClientSocket, RxBuffer, sizeof(RxBuffer), 0);
    if (!strcmp(RxBuffer, "Full")) {
        // The server was full, cannot establish connection
        std::cout << "Server full" << std::endl;
        return 0;
    }
    else {
        // Connection is established, begin sending messages
        std::string TxBuffer;
        // Inform user of login rules
        std::cout << "Before accessing the Smart Home server, you must log-in. You only have 3 attempts." << std::endl;

        // Track failed login attempts
        int failedLogins = 0;

        // Try to login the user
        while (failedLogins < 3 && !login(ClientSocket, username))
        {
            // Login failed, count it
            ++failedLogins;
        }

        // Check if logins failed too many times
        if (failedLogins < 3)
        {
            // Flag for ending the session
            bool keepGoing = true;

            // Prompt user for actions until they end their session
            while (keepGoing)
            {
                // User logged in, display menu to them
                std::cout << "Choose an option by typing the corresponding number:\n1. Review Lights\n2. Review Thermostat\n3. Review Security Cameras\n4. Log Out and End Session\n";
                // Variable to store the user's choice
                std::string choice;

                // Collect a valid choice from the user
                do
                {
                    // Collect a valid choice from the user
                    std::cout << "Enter your choice (number): ";
                    std::cin >> choice;

                    // Check if choice was valid
                    if (!(choice == "1" || choice == "2" || choice == "3" || choice == "4"))
                    {
                        // Choice was invalid, inform user and try again
                        std::cout << "Whoops," << choice << " wasn't a valid option. Please try again with a number between 1 and 4, inclusive.\n\n";
                    }

                } while (choice != "1" && choice != "2" && choice != "3" && choice != "4");

                // Add an extra newline to improve readability
                std::cout << std::endl;

                if (choice == "1")
                {
                    // Selected lights
                    lights(ClientSocket, username);

                }
                else if (choice == "2")
                {
                    // Selected thermostat
                    thermostats(ClientSocket, username);

                }
                else if (choice == "3")
                {
                    // Selected cameras
                    cameras(ClientSocket, username);

                }
                else if (choice == "4")
                {
                    // The user wants to logout and end the session
                    keepGoing = false;
                    std::cout << "Thank you " + username + ", goodbye!" << std::endl;
                    // Attempt to log out
                    logout(ClientSocket, username);
                }
            }
        }
        else
        {
            // The logins failed too many times, end the connection
            std::cout << "Too many failed log-in attempts, ending session." << std::endl;
            TxBuffer = "End";
            send(ClientSocket, TxBuffer.c_str(), TxBuffer.length(), 0);
            memset(RxBuffer, 0, sizeof(RxBuffer));
            recv(ClientSocket, RxBuffer, sizeof(RxBuffer), 0);
        }
    }
    // Clean up 
    closesocket(ClientSocket);
    WSACleanup();

    return 0;
}