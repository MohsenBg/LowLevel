#include <stdio.h>
#include <stdbool.h>
#include "../header/cDrvCtrl.h"

// Menu options
#define LOAD_DRIVER     0x01
#define IO_CTL_SEND     0x02
#define UNLOAD_DRIVER   0x03
#define EXIT_PROGRAM    0x04

#define FILE_DEVICE_HIDE 0x8000          // Custom device type
#define IOCTL_BASE 0x800                 // Base for IOCTL codes
#define CTL_CODE_HIDE(i)                \
    CTL_CODE(FILE_DEVICE_HIDE, IOCTL_BASE + i, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_TEST_START CTL_CODE_HIDE(1) 

#define DRV_PATH		"C:\\WDM_STUPID.sys"
#define SERVICE_NAME	"WDM_STUPID"
#define DISPLAY_NAME	SERVICE_NAME 

// Function declarations
int promptMenuOption();
void handleLoadDriver();
void handleIOControlSend();
void handleUnloadDriver();

cDrvCtrl drv;
int main() {
    bool exitFlag = false;

    while (!exitFlag) {
        int option = promptMenuOption();

        switch (option) {
        case LOAD_DRIVER:
            handleLoadDriver();
            break;

        case IO_CTL_SEND:
            handleIOControlSend();
            break;

        case UNLOAD_DRIVER:
            handleUnloadDriver();
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
        printf("1. Load Driver\n");
        printf("2. Send IOCTL\n");
        printf("3. Unload Driver\n");
        printf("4. Exit Program\n");
        printf("Enter your choice: ");

        if (scanf_s("%d", &option) == 1 && LOAD_DRIVER <= option && option <= EXIT_PROGRAM) {
            break; // Valid input
        }

        printf("Invalid option. Please enter a number between %d and %d.\n", LOAD_DRIVER, EXIT_PROGRAM);

        // Clear input buffer
        while (getchar() != '\n');
    }

    return option;
}

// Function to handle the Load Driver option
void handleLoadDriver() {
    printf_s("Creating Driver Service \r\n");
    drv.Install((PCHAR)DRV_PATH, (PCHAR)SERVICE_NAME, (PCHAR)DISPLAY_NAME);

    printf("Loading Driver= %x \r\n", GetLastError());

    //Start Service
    if (!drv.Start((PCHAR)SERVICE_NAME))
    {
        printf("Start service Unsuccessfully= %x\n", GetLastError());
        drv.Remove((PCHAR)SERVICE_NAME);
        return;
    }
    printf("Started Driver Successfully \r\n");
}

// Function to handle the Send IOCTL option
void handleIOControlSend() {
    printf("Sending IOCTL");
    ULONG RetBytes = 0;
    ULONG x = 2;
    ULONG ret = 0;
    drv.IoControl((PCHAR)"\\Device\\STUPID", IOCTL_TEST_START, &x, sizeof(ULONG), &ret, sizeof(ULONG), &RetBytes);
    printf("ret= %x \r\n", ret);
}

// Function to handle the Unload Driver option
void handleUnloadDriver() {
    printf("UnLoading Driver Service \r\n");
    if (!drv.Stop((PCHAR)SERVICE_NAME)) {
        printf("UnLoading Driver Failed= %x \r\n", GetLastError());
        return;
    }
    printf("UnLoaded Driver Service \r\n");

    printf("Removing Service \r\n");
    if (!drv.Remove((PCHAR)SERVICE_NAME)) {
        printf("Removing Driver Failed= %x \r\n", GetLastError());
    }
    printf("Removed Driver Service \r\n");
}

