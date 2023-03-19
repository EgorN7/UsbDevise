#include <windows.h>
#include <devguid.h>    // for GUID_DEVCLASS_CDROM etc
#include <setupapi.h>
#include <cfgmgr32.h>   // for MAX_DEVICE_ID_LEN, CM_Get_Parent and CM_Get_Device_ID
#define INITGUID
#include <tchar.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <fstream>
#include <cstring>
#include <locale>
#include <codecvt>

using namespace std;

//preparatory operations

#ifdef INITGUID
#define DEFINE_DEVPROPKEY(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8, pid) EXTERN_C const DEVPROPKEY DECLSPEC_SELECTANY name = { { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }, pid }
#else
#define DEFINE_DEVPROPKEY(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8, pid) EXTERN_C const DEVPROPKEY name
#endif

DEFINE_DEVPROPKEY(DEVPKEY_Device_BusReportedDeviceDesc, 0x540b947e, 0x8b40, 0x45bc, 0xa8, 0xa2, 0x6a, 0x0b, 0x89, 0x4c, 0xbd, 0xa2, 4);     // DEVPROP_TYPE_STRING
DEFINE_DEVPROPKEY(DEVPKEY_Device_ContainerId, 0x8c7ed206, 0x3f8a, 0x4827, 0xb3, 0xab, 0xae, 0x9e, 0x1f, 0xae, 0xfc, 0x6c, 2);     // DEVPROP_TYPE_GUID
DEFINE_DEVPROPKEY(DEVPKEY_DeviceDisplay_Category, 0x78c34fc8, 0x104a, 0x4aca, 0x9e, 0xa4, 0x52, 0x4d, 0x52, 0x99, 0x6e, 0x57, 0x5a);  // DEVPROP_TYPE_STRING_LIST
DEFINE_DEVPROPKEY(DEVPKEY_Device_Manufacturer, 0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 13);    // DEVPROP_TYPE_STRING
DEFINE_DEVPROPKEY(DEVPKEY_Device_LocationInfo, 0xa45c254e, 0xdf1c, 0x4efd, 0x80, 0x20, 0x67, 0xd1, 0x46, 0xa8, 0x50, 0xe0, 15);    // DEVPROP_TYPE_STRING

#define ARRAY_SIZE(arr)     (sizeof(arr)/sizeof(arr[0]))

#pragma comment (lib, "setupapi.lib")

typedef BOOL(WINAPI* FN_SetupDiGetDevicePropertyW)(
    __in       HDEVINFO DeviceInfoSet,
    __in       PSP_DEVINFO_DATA DeviceInfoData,
    __in       const DEVPROPKEY* PropertyKey,
    __out      DEVPROPTYPE* PropertyType,
    __out_opt  PBYTE PropertyBuffer,
    __in       DWORD PropertyBufferSize,
    __out_opt  PDWORD RequiredSize,
    __in       DWORD Flags
    );


/// <summary>
/// class for working with usb devices
/// </summary>
class UsbDevices {

public:

    /// <summary>
    /// structure of a USB device
    /// </summary>
    struct UsbDevice
    {
        string name_device;
        string id_device;
        string dev_descrip;
        string manufacture;
        string number_port;
    };

    /// <summary>
    //class constructor with filling in the list of usb devices 
    //when creating a class object
    /// </summary>
    UsbDevices() {
        list_usb_dev = ListDevices();
    };

    /// <summary>
    /// method of filling in the list of connected USB devices
    /// </summary>
    void UpdateListUsbDevices() {
        list_usb_dev = ListDevices();
    }

    /// <summary>
    /// writing device data to a file
    /// </summary>
    /// <param name="number_usb"> - device number in the list on the console</param>
    void write_phone_data_txt(int number_usb) {
        UsbDevice need_usb = list_usb_dev.at(--number_usb);
        setlocale(LC_ALL, "Russian");
        cout << "\nВы выбрали устройство : " << need_usb.name_device << endl << endl;
        ofstream fout;//создаем  поток для записи
        fout.open((char*)"Add_phone.txt", ios_base::out);//открываем(создаем) файл
        fout << need_usb.id_device << " - " << need_usb.number_port << endl;//записываем в файл
        fout.close();//закрываем поток 
    }

    /// <summary>
    /// displaying a list of devices on the console
    /// </summary>
    void PrintListUSBDevise() {
        int i = 0;
        setlocale(LC_ALL, "Russian");
        cout << "Здраствуйте, Вы находистесь в приложении по добалению телефона для блокировки\n";
        cout << "----------------------------------------------------------------------\n";
        setlocale(LC_ALL, "Russian");
        cout << "                     Список активных usb-устройств:                   \n";
        cout << "----------------------------------------------------------------------\n";
        if (list_usb_dev.size() == 0) {
            cout << endl << "               Не найдены подлюченные USB устройства\n" << endl;
            cout << "----------------------------------------------------------------------\n";
        }
        else {
            for (const UsbDevice& usb_dev : list_usb_dev) {
                setlocale(LC_ALL, "Russian");
                cout << ++i << " - Имя устройства: \"" << usb_dev.name_device << "\"" << endl;
                if (usb_dev.dev_descrip != ""s) {
                    cout << "    Bus Reported устройства: \"" << usb_dev.dev_descrip << "\"" << endl;
                }
                cout << "    Device Manufacturer: \"" << usb_dev.manufacture << "\"" << endl;
                cout << "    Device Location Info: \"" << usb_dev.number_port << "\"" << endl;
                cout << "----------------------------------------------------------------------\n";
            }
        }
    }

private:

    //list of usb devices
    vector<UsbDevice> list_usb_dev;

    /// <summary>
    /// method of translating from wsrting to string
    /// </summary>
    std::string convert(
        const wchar_t* first,
        const size_t len,
        const std::locale& loc = std::locale(""),
        const char default_char = '?'
    )
    {
        if (len == 0)
            return std::string();
        const std::ctype<wchar_t>& facet =
            std::use_facet<std::ctype<wchar_t> >(loc);
        const wchar_t* last = first + len;
        std::string result(len, default_char);
        facet.narrow(first, last, default_char, &result[0]);
        return result;
    }

    /// <summary>
    /// method of translating from wsrting to string
    /// </summary>
    std::string  convert(const std::wstring& s)
    {
        return convert(s.c_str(), s.length());
    }

    /// <summary>
    /// method for getting a list of usb devices
    /// </summary>
    vector<UsbDevice> ListDevices() {
        unsigned i;
        vector<UsbDevice> now_usb_dev;
        DWORD dwSize, dwPropertyRegDataType;
        DEVPROPTYPE ulPropertyType;
        CONFIGRET status;
        HDEVINFO hDevInfo;
        SP_DEVINFO_DATA DeviceInfoData;
        const static LPCTSTR arPrefix[3] = { TEXT("VID_"), TEXT("PID_"), TEXT("MI_") };
        TCHAR szDeviceInstanceID[MAX_DEVICE_ID_LEN];
        TCHAR szDesc[1024];
        WCHAR szBuffer[4096];
        FN_SetupDiGetDevicePropertyW fn_SetupDiGetDevicePropertyW = (FN_SetupDiGetDevicePropertyW)
            GetProcAddress(GetModuleHandle(TEXT("Setupapi.dll")), "SetupDiGetDevicePropertyW");

        // List all connected USB devices
        hDevInfo = SetupDiGetClassDevs(NULL, TEXT("USB"), NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT);
        DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        for (i = 0; ; ++i) {
            UsbDevice cirrent_usb_dev;
            cirrent_usb_dev.number_port = ""s;
            if (!SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInfoData))
                break;
            status = CM_Get_Device_ID(DeviceInfoData.DevInst, szDeviceInstanceID, MAX_PATH, 0);
            wstring ws_DeviceInstanceID(szDeviceInstanceID);
            string str_DeviceInstanceID = string(ws_DeviceInstanceID.begin(), ws_DeviceInstanceID.end());
            cirrent_usb_dev.id_device = str_DeviceInstanceID;

            if (SetupDiGetDeviceRegistryPropertyW(hDevInfo, &DeviceInfoData, SPDRP_DEVICEDESC,
                &dwPropertyRegDataType, (BYTE*)szDesc,
                sizeof(szDesc),   // The size, in bytes
                &dwSize)) {
                wstring _ws_Desc(szDesc);
                cirrent_usb_dev.name_device = convert(_ws_Desc);
            }

            if (fn_SetupDiGetDevicePropertyW && fn_SetupDiGetDevicePropertyW(hDevInfo, &DeviceInfoData, &DEVPKEY_Device_BusReportedDeviceDesc,
                &ulPropertyType, (BYTE*)szBuffer, sizeof(szBuffer), &dwSize, 0)) {

                if (fn_SetupDiGetDevicePropertyW(hDevInfo, &DeviceInfoData, &DEVPKEY_Device_BusReportedDeviceDesc,
                    &ulPropertyType, (BYTE*)szBuffer, sizeof(szBuffer), &dwSize, 0)) {
                    wstring ws_Buffer(szBuffer);
                    string str_Buffer = convert(ws_Buffer);;
                    cirrent_usb_dev.dev_descrip = str_Buffer;
                }
                if (fn_SetupDiGetDevicePropertyW(hDevInfo, &DeviceInfoData, &DEVPKEY_Device_Manufacturer,
                    &ulPropertyType, (BYTE*)szBuffer, sizeof(szBuffer), &dwSize, 0)) {
                    wstring ws_Buffer(szBuffer);
                    string str_Buffer = convert(ws_Buffer);;
                    cirrent_usb_dev.manufacture = str_Buffer;
                }
                if (fn_SetupDiGetDevicePropertyW(hDevInfo, &DeviceInfoData, &DEVPKEY_Device_LocationInfo,
                    &ulPropertyType, (BYTE*)szBuffer, sizeof(szBuffer), &dwSize, 0)) {
                    wstring ws_Buffer(szBuffer);
                    string str_Buffer = convert(ws_Buffer);
                    if (!str_Buffer.find("Port")) { cirrent_usb_dev.number_port = str_Buffer; }
                }
            }
            if (cirrent_usb_dev.number_port != ""s) {
                now_usb_dev.push_back(cirrent_usb_dev);
            }
        }
        return now_usb_dev;
    }
};



int main() {
    string help_int = "1"s;
    UsbDevices usbdevices = UsbDevices();
    usbdevices.PrintListUSBDevise();
    while (help_int != "0"s) {
        cout << "Введите цифру с интересующим Вас телефоном, '0' для выхода \nили 'r' для обновления списка: ";
        cin >> help_int;
        if (help_int != "0"s && help_int != "r"s) {
            usbdevices.write_phone_data_txt(stoi(help_int));
        }
        if (help_int == "r"s) {
            system("cls");
            usbdevices.UpdateListUsbDevices();
            usbdevices.PrintListUSBDevise();
        }

    }
    return 0;
}