#include <stdio.h>
#include <stdbool.h>
#include <Windows.h>

#define MAX_BUFFER_SIZE 1024

#define DEVICE_SEND CTL_CODE(FILE_DEVICE_UNKNOWN,0x901,METHOD_BUFFERED,FILE_WRITE_DATA)
#define DEVICE_RECEIVE CTL_CODE(FILE_DEVICE_UNKNOWN,0x902,METHOD_BUFFERED,FILE_READ_DATA)

// Menu options
#define CREATE_DISPATCH          0x01
#define SEND_DATA_DISPATCH       0x02
#define RECEIVE_DATA_DISPATCH    0x03
#define CLOSE_DISPATCH           0x04
#define EXIT_PROGRAM             0x05 

// Function declarations
int promptMenuOption();
void handleCreateDispatch();
void handleSendDataDispatch();
void handleReceiveDataDispatch();
void handleCloseDispatch();

HANDLE hDevice;
TCHAR* DeviceSymbolLink = L"\\\\.\\myDeviceLink8585";

int main() {
    bool exitFlag = false;

    while (!exitFlag) {
        int option = promptMenuOption();

        switch (option) {
        case CREATE_DISPATCH:
            handleCreateDispatch();
            break;

        case SEND_DATA_DISPATCH:
            handleSendDataDispatch();
            break;
    
        case RECEIVE_DATA_DISPATCH:
            handleReceiveDataDispatch();
            break;

        case CLOSE_DISPATCH:
            handleCloseDispatch();
            break;

        case EXIT_PROGRAM:
            exitFlag = true;
            printf("Exiting the program. Goodbye!\n");
            break;

        default:
            printf("Unexpected error. Exiting.\n");
            exitFlag = true;
            break;
        }
    }

    return 0;
}

// Display menu and prompt for a valid menu option
int promptMenuOption() {
    int option = 0;

    while (1) {
        printf("\nMenu Options:\n");
        printf("1. Create Dispatch\n");
        printf("2. Send Data Dispatch\n");
        printf("3. Receive Data Dispatch\n");
        printf("4. Close Dispatch\n");
        printf("5. Exit Program\n");
        printf("Enter your choice: ");

        if (scanf_s("%d", &option) == 1 && CREATE_DISPATCH <= option && option <= EXIT_PROGRAM) {
            break; // Valid input
        }

        printf("Invalid option. Please enter a number between %d and %d.\n", CREATE_DISPATCH, EXIT_PROGRAM);

        // Clear input buffer
        while (getchar() != '\n');
    }

    return option;
}

// Handle the creation of a dispatch by opening a device file.
// This function uses the CreateFile API to establish a connection with the device
// represented by the symbolic link 'DeviceSymbolLink'.
void handleCreateDispatch() {
    printf("Creating dispatch...\r\n");

    hDevice = CreateFile(
        DeviceSymbolLink,          // Symbolic link name of the device
        GENERIC_ALL,               // Access mode (read, write, execute, etc.)
        0,                         // Share mode (no sharing)
        NULL,                      // Security attributes (default)
        OPEN_EXISTING,             // Opens an existing device file
        FILE_ATTRIBUTE_SYSTEM,     // System-level attributes
        NULL                       // No template file
    );

    if (hDevice == INVALID_HANDLE_VALUE) {
        printf("Failed to create dispatch. Error: %lu\r\n", GetLastError());
    }
    else {
        printf("Dispatch created successfully!\r\n");
    }
}

// Function to handle the Send Data Dispatch option
void handleSendDataDispatch() {
    printf("Send Data dispatch...\r\n");

    WCHAR* message = L"This data is sent from the user-mode App";

    // Check if the device handle is invalid
    if (hDevice == NULL || hDevice == INVALID_HANDLE_VALUE) {
        printf("Failed to close dispatch. Invalid handle or no handle created. Error: %lu\r\n", GetLastError());
        return;
    }

    // Calculate the message length in bytes
    ULONG messageLength = (wcsnlen(message, MAX_BUFFER_SIZE - 1) + 1) * sizeof(WCHAR);

    // Initialize return output size variable
    ULONG returnOutputSize = 0;

    // Call DeviceIoControl for sending data
    BOOL result = DeviceIoControl(
        hDevice,            // Device Handle
        DEVICE_SEND,        // Io Control Code
        message,            // Input buffer (message)
        messageLength,      // Size of the input buffer
        NULL,               // No output buffer required for sending
        0,                  // No output buffer expected
        &returnOutputSize,  // Receive the size of the output
        NULL                // No overlap
    );

    if (!result) {
        printf("DeviceIoControl failed with error: %lu\r\n", GetLastError());
    }
    else {

        printf("Send Data dispatch completed.\r\n");
    }
}



// Function to handle the Receive Data Dispatch option
void handleReceiveDataDispatch() {
    printf("Receive Data dispatch...\r\n");

    // Check if the device handle is invalid
    if (hDevice == NULL || hDevice == INVALID_HANDLE_VALUE) {
        // Print an error message if the handle is invalid
        printf("Failed to dispatch. Invalid handle or no handle created. Error: %lu\r\n", GetLastError());
        return;
    }

    // Buffer to receive the data
    WCHAR message[MAX_BUFFER_SIZE / sizeof(WCHAR)] = { 0 };
    ULONG returnOutputSize = 0;

    // Call DeviceIoControl to receive data
    BOOL result = DeviceIoControl(
        hDevice,                // Device Handle
        DEVICE_RECEIVE,         // Io Control Code
        NULL,                   // No input buffer required
        0,                      // No input data size
        message,                // Output buffer to receive data
        MAX_BUFFER_SIZE,        // Size of output buffer
        &returnOutputSize,      // Return size of the received data
        NULL                    // No overlap
    );

    // Check if the call to DeviceIoControl succeeded
    if (!result) {
        printf("DeviceIoControl failed with error: %lu\r\n", GetLastError());
    }
    else {
        printf("Receive Data dispatch completed. Data received: %ws\r\n", message);
    }
}


// Handle the closure of a dispatch by closing the device handle.
// This function checks the validity of the device handle and closes it if valid.
void handleCloseDispatch() {
    printf("Attempting to close dispatch...\r\n");

    if (hDevice == NULL || hDevice == INVALID_HANDLE_VALUE) {
        // If the handle is invalid, print an error message with the error code.
        printf("Failed to close dispatch. Invalid handle or no handle created. Error: %lu\r\n", GetLastError());
        return;
    }

    // Close the handle to release the dispatch.
    int result = CloseHandle(hDevice);
    if (result == 0){
        printf("Failed to close dispatch. Unable to Close Handle. Error: %lu\r\n", GetLastError());
        return;
    }
    hDevice = NULL; // Reset the handle to avoid accidental reuse.

    printf("Dispatch closed successfully.\r\n");
}

