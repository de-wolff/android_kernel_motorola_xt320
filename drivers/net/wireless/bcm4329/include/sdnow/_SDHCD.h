#ifdef BCMSDIO
///////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2002 BSQUARE Corporation.  All rights reserved.
//
// Module Name:
//
//    _SDHCD.h   
//
// Abstract:
//
//    Header file defining the host controller interface
//
// Notes:
//
///////////////////////////////////////////////////////////////////////////////


#ifndef _SDCARD_HCD_DEFINED
#define _SDCARD_HCD_DEFINED

#define SD_HC_TAG 'HCXT'

#define SD_HC_MAX_NAME_LENGTH 16

typedef struct _SDCARD_HC_CONTEXT *PSDCARD_HC_CONTEXT;

    // typedef for slot option handler codes
typedef enum _SD_SLOT_OPTION_CODE{
    SDHCDNop = -1,
    SDHCDSetSlotPower,              // set slot power, takes a DWORD for the power bit mask
    SDHCDSetSlotInterface,          // set slot interface, takes a SD_CARD_INTERFACE structure
    SDHCDEnableSDIOInterrupts,      // enable SDIO interrupts on the slot, no parameters
    SDHCDDisableSDIOInterrupts,     // disable SDIO interrupts on the slot, no parameters
    SDHCDAckSDIOInterrupt,          // acknowledge that the SDIO interrupt was handled, no parameters
    SDHCDGetWriteProtectStatus,     // get Write protect status. Updates SD_CARD_INTERFACE structure
    SDHCDQueryBlockCapability,      // query whether HC supports requested block length, 
                                    //  takes SD_HOST_BLOCK_CAPABILITY structure
}SD_SLOT_OPTION_CODE, *PSD_SLOT_OPTION_CODE;

    // typedef for asynchronous slot event codes
typedef enum _SD_SLOT_EVENT{
    NOP,                // no operation
    DeviceEjected,      // device was ejected
    DeviceInserted,     // device was inserted 
    DeviceInterrupting, // device is interrupting
    BusRequestComplete, // a bus request was completed
}SD_SLOT_EVENT, *PSD_SLOT_EVENT;

    // typdef for the current slot state
typedef enum _SD_SLOT_STATE{
    SlotInactive = 0,    // slot is inactive
    SlotIdle,            // slot is idle (after power up)
    Ready,               // the slot is ready, the client driver now has control
    SlotDeviceEjected,   // the device is being ejected
    SlotInitFailed       // slot initialization failed
}SD_SLOT_STATE, *PSD_SLOT_STATE;

    // slot context , one per slot
typedef struct _SDCARD_HC_SLOT_CONTEXT {
    PSDCARD_HC_CONTEXT          pHostController;   // the host controller this slot belongs to
    SD_DEVICE_HANDLE            hDevice;           // handle to the device occupying this slot
    SD_SLOT_STATE               SlotState;         // slot state
    UCHAR                       SlotIndex;         // the slot index
    SD_REQUEST_QUEUE            RequestQueue;      // request queue for this slot
    ULONG                       PowerUpDelay;      // power up delay for the slot
    PVOID                       pWorkItem;         // work item 
    DWORD                       Flags;             // flags
}SDCARD_HC_SLOT_CONTEXT, *PSDCARD_HC_SLOT_CONTEXT;

    // the following bits are for flags field in the slot context
#define SD_SLOT_FLAG_SDIO_INTERRUPTS_ENABLED   0x00000001

#define IS_SLOT_SDIO_INTERRUPT_ON(pSlot) ((pSlot)->Flags & SD_SLOT_FLAG_SDIO_INTERRUPTS_ENABLED)
#define FLAG_SD_SLOT_INTERRUPTS_ON(pSlot) ((pSlot)->Flags |= SD_SLOT_FLAG_SDIO_INTERRUPTS_ENABLED)
#define FLAG_SD_SLOT_INTERRUPTS_OFF(pSlot) ((pSlot)->Flags &= ~SD_SLOT_FLAG_SDIO_INTERRUPTS_ENABLED)

    // typedef for the Send command Handler
typedef SD_API_STATUS (*PSD_BUS_REQUEST_HANDLER) (PSDCARD_HC_CONTEXT, UCHAR, PSD_BUS_REQUEST);

    // typedef for the I/O cancel handler
typedef BOOLEAN (*PSD_CANCEL_REQUEST_HANDLER) (PSDCARD_HC_CONTEXT, UCHAR, PSD_BUS_REQUEST);

    // typedef for set slot options
typedef SD_API_STATUS (*PSD_GET_SET_SLOT_OPTION) (PSDCARD_HC_CONTEXT, UCHAR, SD_SLOT_OPTION_CODE, PVOID, ULONG);

    // typedef for initialize controller
typedef SD_API_STATUS (*PSD_INITIALIZE_CONTROLLER) (PSDCARD_HC_CONTEXT);

    // typedef for initialize controller
typedef SD_API_STATUS (*PSD_DEINITIALIZE_CONTROLLER) (PSDCARD_HC_CONTEXT);

    // the following bit masks define the slot capabilities of a host controller
    // slot supports 1 bit data transfers
#define SD_SLOT_SD_1BIT_CAPABLE                  0x00000004
    // the slot supports 4 bit data transfers
#define SD_SLOT_SD_4BIT_CAPABLE                  0x00000008
    // the slot is SD I/O capable, minimally supporting interrupt signalling. 
    // driver can OR-in SD_SLOT_SDIO_INT_DETECT_4BIT_MULTI_BLOCK if the slot is capable
    // of handling interrupt signalling during 4 bit Data transfers
#define SD_SLOT_SDIO_CAPABLE                     0x00000020
    // slot supports interrupt detection during 4-bit multi-block transfers
#define SD_SLOT_SDIO_INT_DETECT_4BIT_MULTI_BLOCK 0x00000100
 
    // host controller context
typedef struct _SDCARD_HC_CONTEXT {
    LIST_ENTRY               ListEntry;              // list entry link
    PVOID                    pSystemContext;         // system context
    UCHAR                    NumberOfSlots;          // number of slots
    DWORD                    Capabilities;           // bit mask defining capabilities
    DWORD                    VoltageWindowMask;      // bit mask for slot's voltage window capability, same format at OCR register
    DWORD                    DesiredVoltageMask;     // desired voltage bit mask
    DWORD                    MaxClockRate;           // maximum clock rate in HZ, Max 4.29 GHz
    DWORD                    PowerUpDelay;           // power up delay in MS
    WCHAR                    HostControllerName[SD_HC_MAX_NAME_LENGTH]; // friendly name of slot controller
    CRITICAL_SECTION         HCCritSection;          // host controller critical section
    PSD_BUS_REQUEST_HANDLER  pBusRequestHandler;     // bus request handler
    PSD_GET_SET_SLOT_OPTION  pSlotOptionHandler;     // slot option handler
    PSD_CANCEL_REQUEST_HANDLER pCancelIOHandler;     // cancel request handler
    PVOID                    pHCSpecificContext;     // host controller specific context
    PSD_INITIALIZE_CONTROLLER   pInitHandler;        // init handler       
    PSD_DEINITIALIZE_CONTROLLER pDeinitHandler;      // deinit handler
    SDCARD_HC_SLOT_CONTEXT   SlotList[1];            // beginning of slot list array
}SDCARD_HC_CONTEXT, *PSDCARD_HC_CONTEXT;

  
#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

    // SDHCDAllocateContext - Allocate an HCD Context
    //
    // Input: NumberOfSlots - Number of slots
    //        HCDeviceExtensionSize - Size of the HC specific device extension 
    // Output:
    //        ppHostContext - caller supplied storage for the host context
    // Return: SD_API_STATUS 
    // Notes:
    //
SD_API_STATUS SDHCDAllocateContext(UCHAR NumberOfSlots, ULONG HCDeviceExtensionSize, PSDCARD_HC_CONTEXT *ppHostContext);

    // SDHCDDeleteContext - Delete an HCD context
    //
    // Input: pHostContext - Host Context to delete
    // Output:
    // Return:
    //
VOID SDHCDDeleteContext(PSDCARD_HC_CONTEXT pHostContext);

    // macro to extract the extension specific context
#define GetExtensionFromHCDContext(d, pContext) ((d)((pContext)->pHCSpecificContext))

    // macros to set context structure entries
    // the following structures entries must be filled out by all host controller drivers
#define SDHCDSetSlotCapabilities(pHc, d)    ((pHc)->Capabilities = d)
#define SDHCDSetVoltageWindowMask(pHc, d)   ((pHc)->VoltageWindowMask = d) 
#define SDHCDSetDesiredSlotVoltage(pHc,d)   ((pHc)->DesiredVoltageMask = d)      
#define SDHCDSetMaxClockRate(pHc, d)        ((pHc)->MaxClockRate = d)      
#define SDHCDSetBusRequestHandler(pHc, d)  ((pHc)->pBusRequestHandler = d)
#define SDHCDSetCancelIOHandler(pHc, d)     ((pHc)->pCancelIOHandler = d)
#define SDHCDSetSlotOptionHandler(pHc, d)   ((pHc)->pSlotOptionHandler = d)
#define SDHCDSetControllerInitHandler(pHc, d)   ((pHc)->pInitHandler = d)
#define SDHCDSetControllerDeinitHandler(pHc, d) ((pHc)->pDeinitHandler = d)
#define SDHCDSetPowerUpDelay(pHc, d)        ((pHc)->PowerUpDelay = d)
#define SDHCDSetHCName(pHc, d)              (_tcscpy((pHc)->HostControllerName, d))

    // the following macros provide HC synchronization and information
#define SDHCDGetSlotCount(pHc)              ((pHc)->NumberOfSlots)
#define SDHCDAcquireHCLock(pHc)             EnterCriticalSection(&((pHc)->HCCritSection))
#define SDHCDReleaseHCLock(pHc)             LeaveCriticalSection(&((pHc)->HCCritSection))
#define SDHCDGetSlotContext(pHc, Index)     &((pHc)->SlotList[Index])  
#define SDHCDGetHCName(pHC)                 (pHC)->HostControllerName
#define SDHCDGetPowerUpDelay(pHc)           ((pHc)->PowerUpDelay)


    // macros used by the bus driver to invoke slot option handlers

#define SDEnableSDIOInterrupts(pSlot) \
        (pSlot)->pHostController->pSlotOptionHandler((pSlot)->pHostController,  \
                                                     (pSlot)->SlotIndex,        \
                                                     SDHCDEnableSDIOInterrupts, \
                                                     NULL,                      \
                                                     0)
#define SDDisableSDIOInterrupts(pSlot) \
        (pSlot)->pHostController->pSlotOptionHandler((pSlot)->pHostController,   \
                                                     (pSlot)->SlotIndex,         \
                                                     SDHCDDisableSDIOInterrupts, \
                                                     NULL,                       \
                                                     0)    
#define SDAckSDIOInterrupts(pSlot) \
    (pSlot)->pHostController->pSlotOptionHandler((pSlot)->pHostController,   \
                                                 (pSlot)->SlotIndex,         \
                                                 SDHCDAckSDIOInterrupt,      \
                                                 NULL,                       \
                                                 0)

    // SDHCDRegisterHostController - Register a host controller with the bus driver
    //
    // Input: pHCContext - Allocated Host controller context
    //
    // Output:
    // Return: SD_API_STATUS 
    // Notes:      
    //      the caller must allocate a host controller context and 
    //      initialize the various parameters
SD_API_STATUS SDHCDRegisterHostController(PSDCARD_HC_CONTEXT pHCContext);

    // SDHCDDeregisterHostController - Deregister a host controller 
    //
    // Input: pHCContext - Host controller context that was previously registered
    //        
    // Output:
    // Return: SD_API_STATUS 
    // Notes:       
    //      A host controller must call this api before deleting the HC context
    //      
    // returns SD_API_STATUS
SD_API_STATUS SDHCDDeregisterHostController(PSDCARD_HC_CONTEXT pHCContext);


    // SDHCDIndicateSlotStateChange - indicate a change in the SD Slot 
    //
    // Input: pHCContext - Host controller context that was previously registered
    //        SlotNumber - Slot Number
    //        Event     - new event code
    // Output:
    // Return:
    // Notes:        
    //      A host controller driver calls this api when the slot changes state (i.e.
    //      device insertion/deletion).
    //      
    // 
VOID SDHCDIndicateSlotStateChange(PSDCARD_HC_CONTEXT pHCContext, 
                                  UCHAR              SlotNumber,
                                  SD_SLOT_EVENT      Event);

    // SDHCDIndicateBusRequestComplete - indicate to the bus driver that
    //                                   the request is complete
    //
    // Input: pHCContext - host controller context
    //        pRequest   - the request to indicate
    //        Status     - the ending status of the request
    // Output:
    // Return: 
    // Notes:       
    //     
VOID SDHCDIndicateBusRequestComplete(PSDCARD_HC_CONTEXT pHCContext,
                                     PSD_BUS_REQUEST    pRequest,
                                     SD_API_STATUS      Status);


    // SDHCDUnlockRequest - Unlock a request that was previous locked
    //                             
    // Input:   pHCContext - host controller context   
    //          pRequest  - the request to lock
    // Output:
    // Return:    
    // Notes:   This function unlocks the request that was returned from the
    //          function SDHCDGetAndLockCurrentRequest()
    //          
    //          This request can now be cancelled from any thread context
VOID SDHCDUnlockRequest(PSDCARD_HC_CONTEXT  pHCContext,
                        PSD_BUS_REQUEST     pRequest);

    // SDHCDGetAndLockCurrentRequest - get the current request in the host controller
    //                                 slot and lock it to keep it from being cancelable
    // Input:   pHCContext - host controller context   
    //          SlotIndex  - the slot number 
    // Output:
    // Return: current bus request  
    // Notes:
    //          This function retrieves the current request and marks the
    //          request as NON-cancelable.  To return the request back to the
    //          cancelable state the caller must call SDHCDUnlockRequest()     
    //          This function returns the current request which can be NULL if 
    //          the request was previously marked cancelable and the host controller's
    //          cancelIo Handler completed the request 
PSD_BUS_REQUEST SDHCDGetAndLockCurrentRequest(PSDCARD_HC_CONTEXT pHCContext, 
                                              UCHAR              SlotIndex);


    // SDHCDPowerUpDown - Indicate a power up/down event
    //                             
    // Input:   pHCContext - host controller context   
    //          PowerUp    - set to TRUE if powering up
    //          SlotKeepPower - set to TRUE if the slot maintains power to the
    //                          slot during power down
    // Output:
    // Return:        
    // Notes:   This function notifies the bus driver of a power up/down event.
    //          The host controller driver can indicate to the bus driver that power
    //          can be maintained for the slot.  If power is removed, the bus driver
    //          will unload the device driver on the next power up event.
    //          This function can only be called from the host controller's XXX_PowerOn
    //          and XXX_PowerOff function.
VOID SDHCDPowerUpDown(PSDCARD_HC_CONTEXT  pHCContext, 
                      BOOL                PowerUp, 
                      BOOL                SlotKeepPower);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif

#endif
