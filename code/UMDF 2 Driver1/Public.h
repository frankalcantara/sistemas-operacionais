/*++

Module Name:

    public.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications.

Environment:

    driver and application

--*/

//
// Define an Interface Guid so that apps can find the device and talk to it.
//

DEFINE_GUID (GUID_DEVINTERFACE_UMDF2Driver1,
    0xa5d44433,0xed67,0x476a,0x87,0x3a,0x81,0x5b,0x5d,0x66,0x8b,0x57);
// {a5d44433-ed67-476a-873a-815b5d668b57}
