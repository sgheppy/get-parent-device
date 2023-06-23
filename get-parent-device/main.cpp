
#include <stdio.h>    

#include <windows.h>  

#include <setupapi.h> 
#include <cfgmgr32.h>
#include <usbiodef.h>
#include <initguid.h>


#define ERR_BAD_ARGUMENTS 1;
#define ERR_NO_DEVICES_FOUND 2;
#define ERR_NO_DEVICE_INFO 3;

int process_func(PWCHAR pszSearchedDeviceInstanceId, PWCHAR pszParentDeviceInstanceIdPattern);

/**
 * Returns executable file name from full path.
 *
 * @param pszExecutablePath
 *          pointer to full path of executable
 *
 * @return pointer to executable name
 */
PWCHAR GetExecutableName(PWCHAR pszExecutablePath) {

	PWCHAR pszExecutableName = (PWCHAR)pszExecutablePath + lstrlen(pszExecutablePath);

	for (; pszExecutableName > pszExecutablePath; pszExecutableName--) {

		if ((*pszExecutableName == '\\') || (*pszExecutableName == '/'))
		{
			pszExecutableName++;
			break;
		}
	}

	return pszExecutableName;
}

/**
 * Prints program usage.
 */
void ShowHelp(PWCHAR pszProgramName) {

	wprintf(L"Usage:\n\n");

	wprintf(L"\t%s DII PATTERN\n\n", pszProgramName);

	wprintf(L"Arguments:\n\n");

	wprintf(L"\tDII     - Device Instance ID of the Device whose parent is to be found\n");
	wprintf(L"\tPATTERN - Regular expression to match Parent's Device Instance ID\n");
	wprintf(L"\n");

	wprintf(L"Examples:\n\n");

	wprintf(L"Example 1. Get immediate parent:\n\n");
	wprintf(L"\t%s \"%s\" \"%s\"\n\n", pszProgramName, L"USBSTOR\\DISK&VEN_GENERIC&PROD_STORAGE_DEVICE&REV_0207\\000000000207&0", L".*");
	wprintf(L"In this case the \".*\" will cause first found parent to be returned.\n");
	wprintf(L"\n");

	wprintf(L"Example 2. Get usb hub the device is connected to:\n\n");
	wprintf(L"\t%s \"%s\" \"%s\"\n\n", pszProgramName, L"USBSTOR\\DISK&VEN_GENERIC&PROD_STORAGE_DEVICE&REV_0207\\000000000207&0", L".*\\\\ROOT_HUB.*");
	wprintf(L"The program will search \"up\" the device tree until it finds a parent with a matching Device Instance ID.\n");
	wprintf(L"\n");

}


/**
 * get-parent-device entry point.
 *
 * Program takes two arguments:
 *
 * 1. pszSearchedDeviceInstanceId - This is Device Instance ID of a device whose parent is to be found.
 *                    This should be in a form returned by WMI, e.g. "USBSTOR\DISK&VEN_GENERIC&PROD_STORAGE_DEVICE&REV_0207\000000000207&0".
 *                    The value is used as second argument (i.e. the Enumerator) for the SetupDiGetClassDevs - see WDK for details.
 *
 * 2. pszParentDeviceInstanceIdPattern - Regular expression handled by <regex> library, to match the structure of the Parent Device Instance ID.
 *                    The program searches up the parent tree until it finds a parent, whose Device Instance ID matches given pattern.
 *                    If there is no Parent with matching Device Instance ID, nothing is returned.
 *
 * Error handling is quite minimalistic - no exceptions are caught and "bad" return values just cause the program to exit witho no output.
 *
 */
int main(void) {

	// Input arguments as WCHAR - not using main function arguments, as they are "hardly" convertable to PWCHAR
	int argc;
	PWCHAR* argv = CommandLineToArgvW(GetCommandLine(), &argc);

	// Check argument count
	if (argc != 3) {
		ShowHelp(GetExecutableName(argv[0]));
		return ERR_BAD_ARGUMENTS;
	}

	// Device ID - e.g.: "USBSTOR\DISK&VEN_GENERIC&PROD_STORAGE_DEVICE&REV_0207\000000000207&0"
	PWCHAR pszSearchedDeviceInstanceId = argv[1];

	// Pattern to match parent's Device Instance ID - w.g. "USB\\VID_(\w+)&PID_(\w+)\\(?!.*[&_].*)(\w+)"
	PWCHAR pszParentDeviceInstanceIdPattern = argv[2];


	return process_func(pszSearchedDeviceInstanceId, pszParentDeviceInstanceIdPattern);
}
