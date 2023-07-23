/**
 * get-parent-device
 * =====================
 *
 *
 * LICENSE (MIT):
 * ==============
 *
 * Copyright (C) 2012 Marcin Kielar
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 *
 * DESCRIPTION:
 * ============
 *
 * This program finds Device Instance ID of a parent of a Device given by another Device Instance ID.
 * The main reason to create this program was to be able to obtain "bus relation" of Win32_PnPEntity objects
 * connected to Win32_UsbController, because plain WMI cannot do that. See below, and read docs for the `main` function
 * for details.
 *
 * Example:
 * --------
 *
 * Say one is observing - using WMI - (dis)connection of USB Drives.
 * One can listen to Win32_LogicalDisk events using WMI, and from that, one can go as far as Win32_PnpEntity of type "USBSTOR".
 * Such device has Device Instance ID = "USBSTOR\DISK&VEN_GENERIC&PROD_STORAGE_DEVICE&REV_0207\000000000207&0" (or similar).
 *
 * But, there is another Win32_PnPEntity under the Win32_UsbController object, and it's Device Instance ID is:
 *
 *     USB\VID_05E3&PID_0727\000000000207
 *
 * which is what one needs if one is to get numerical values of Vendor ID, Product ID and Serial number.
 *
 * To make things harder, sometimes the tree is deeper than two elements - if a USB Drive is available
 * via USB Composite Device (like a SD Card inside a Smartphone).
 *
 * Unfortunetly, WMI does not have any binging or relation between the two, and one needs to fallback to C code, to get the relation.
 * This program does exactly what WMI cannot.
 *
 *
 * BUILD INSTRUCTION:
 * ==================
 *
 * This program compiled without problems with "Visual Studio 2012 Express Edition for Desktop" and "Windows Driver Kit 7600.16385.1" installed.
 *
 * Dependencies:
 * -------------
 *
 *   setupapi.lib  - Installed with Driver Developer Kit
 *   cfgmgr32.lib  - Installed with Driver Developer Kit
 *
 */

#pragma comment (lib, "setupapi.lib")
#pragma comment (lib, "cfgmgr32.lib")

 // initguid.h must be included first or else we get 
 // "LNK2001: unresolved external symbol _GUID_DEVINTERFACE_USB_DEVICE" errors
#include <initguid.h>

#include <windows.h>  

#include <setupapi.h> 
#include <cfgmgr32.h>
#include <usbiodef.h>

#include <regex>

#include <stdio.h>    

#define ERR_BAD_ARGUMENTS 1;
#define ERR_NO_DEVICES_FOUND 2;
#define ERR_NO_DEVICE_INFO 3;


#define GUID_DISK_DRIVE_STRING L"{21EC2020-3AEA-1069-A2DD-08002B30309D}"
extern "C" {
/**
 * Finds a parent Device Instance ID for given hCurrentDeviceInstanceId and returns it's value (as string) and handle.
 *
 * @param [out] szParentDeviceInstanceId
 *			pointer to memory location where parent Device Instance ID value will be stored
 * @param [out] hParentDeviceInstanceId
 *          pointer to memory location where parent Device Instance ID handle will be stored
 * @param [in] hCurrentDeviceInstanceId
 *          reference to Device Instance ID of the device, whose parent is to be found
 *
 * @return TRUE if the parent was found, otherwise FALSE
 */
BOOL GetParentDeviceInstanceId(_Out_ PWCHAR pszParentDeviceInstanceId, _Out_ PDEVINST phParentDeviceInstanceId, _In_ DEVINST hCurrentDeviceInstanceId) {

	// Handle to parent Device Instance ID
	BOOL result = CM_Get_Parent(phParentDeviceInstanceId, hCurrentDeviceInstanceId, 0);
	if (result == CR_SUCCESS) {

		// Device ID as String
		memset(pszParentDeviceInstanceId, 0, MAX_DEVICE_ID_LEN);
		result = CM_Get_Device_ID(*phParentDeviceInstanceId, pszParentDeviceInstanceId, MAX_DEVICE_ID_LEN, 0);
		if (result == CR_SUCCESS) {
			return TRUE;
		}
	}

	return FALSE;
}

/**
 * Tests, whether given Device Instance ID matches given pattern.
 *
 * @param pszDeviceInstanceId
 *          pointer to Device Instance ID value
 * @param pszPattern
 *          pointer to pattern string
 *
 * @return TRUE if the instance id matches pattern, otherwise FALSE
 */
BOOL DeviceIdMatchesPattern(_In_ PWCHAR pszDeviceInstanceId, _In_ PWCHAR pszPattern) {

	std::wstring sParentDeviceInstanceId(pszDeviceInstanceId);
	std::wregex rRegex(pszPattern);

	return std::regex_match(sParentDeviceInstanceId, rRegex);

}




__declspec(dllexport) WCHAR* process_func(PWCHAR pszSearchedDeviceInstanceId, PWCHAR pszParentDeviceInstanceIdPattern) {
	// GUID to match devices by class
	GUID guid;
	CLSIDFromString(GUID_DISK_DRIVE_STRING, &guid);

	// Get matching devices info
	HDEVINFO devInfo = SetupDiGetClassDevs(&guid, NULL, NULL, DIGCF_PRESENT | DIGCF_ALLCLASSES);

	// Device Instance ID as string
	WCHAR szDeviceInstanceId[MAX_DEVICE_ID_LEN];
	WCHAR* myerror = new WCHAR[10];
	if (devInfo != INVALID_HANDLE_VALUE) {

		DWORD devIndex = 0;
		SP_DEVINFO_DATA devInfoData;
		devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

		// Loop over devices found in SetupDiGetClassDevs
		while (SetupDiEnumDeviceInfo(devInfo, devIndex, &devInfoData)) {

			// Read Device Instance ID of current device
			memset(szDeviceInstanceId, 0, MAX_DEVICE_ID_LEN);
			SetupDiGetDeviceInstanceId(devInfo, &devInfoData, szDeviceInstanceId, MAX_PATH, 0);

			// Case insensitive comparison (because Device Instance IDs can vary?)
			if (lstrcmpi(pszSearchedDeviceInstanceId, szDeviceInstanceId) == 0) {

				// Handle of current defice instance id
				DEVINST hCurrentDeviceInstanceId = devInfoData.DevInst;

				// Handle of parent Device Instance ID
				DEVINST hParentDeviceInstanceId;

				// Parent Device Instance ID as string
				WCHAR* pszParentDeviceInstanceId= new WCHAR[MAX_DEVICE_ID_LEN];

				// Search "up" parent tree until a parent with matching Device Instance ID is found.
				while (true) {

					// Initialize / clean variables
					memset(szDeviceInstanceId, 0, MAX_DEVICE_ID_LEN);
					hParentDeviceInstanceId = NULL;

					if (GetParentDeviceInstanceId(pszParentDeviceInstanceId, &hParentDeviceInstanceId, hCurrentDeviceInstanceId)) {

						if (DeviceIdMatchesPattern(pszParentDeviceInstanceId, pszParentDeviceInstanceIdPattern)) {

							// Parent Device Instance ID matches given regexp - print it out and exit
							//wprintf(L"- %s\n", pszParentDeviceInstanceId);
							return pszParentDeviceInstanceId;

						}

						// Parent Device Instance ID does not match the pattern - check parent's parent
						hCurrentDeviceInstanceId = hParentDeviceInstanceId;

					}
					else {

						// There is no parent. Stop the loop.
						break;

					}
				}
			}

			devIndex++;
		}

		if (devIndex == 0)
		{
			swprintf_s(myerror, 10, L"%d", 10);
			return myerror;
		}

	}
	else {
		swprintf_s(myerror, 10, L"%d", 10);
		return myerror;
	}
	swprintf_s(myerror, 10, L"%d", 0);
	return myerror;
}

}
