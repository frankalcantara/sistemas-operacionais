/*
 * Driver.cpp
 *
 * Implementação do driver UMDF (Ring 3).
 * Demonstra a hierarquia de objetos WDF.
 */

#include "driver.h" // Nosso .h com o IOCTL
#include <wdf.h>    // O Windows Driver Framework

 // Assinaturas das nossas funções de callback (eventos)
DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD EvtDriverDeviceAdd;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL EvtIoDeviceControl;

//
// DriverEntry: O ponto de entrada (como o 'main' de um driver)
//
extern "C" NTSTATUS DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    WDF_DRIVER_CONFIG config;
    NTSTATUS status;

    WDF_DRIVER_CONFIG_INIT(&config, EvtDriverDeviceAdd);

    // 1. Criar o objeto WDFDRIVER [cite: 110, 118]
    // Este é o objeto raiz da nossa hierarquia
    status = WdfDriverCreate(
        DriverObject,
        RegistryPath,
        WDF_NO_OBJECT_ATTRIBUTES,
        &config,
        WDF_NO_HANDLE
    );

    return status;
}

//
// EvtDriverDeviceAdd: Evento "Plug and Play"
// Chamado quando o PnP Manager detecta nosso dispositivo[cite: 122].
//
NTSTATUS EvtDriverDeviceAdd(
    _In_ WDFDRIVER       Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
)
{
    WDFDEVICE hDevice;
    NTSTATUS status;

    // 2. Criar o objeto WDFDEVICE [cite: 112, 120]
    // Representa a instância do nosso hardware (virtual)
    status = WdfDeviceCreate(&DeviceInit, WDF_NO_OBJECT_ATTRIBUTES, &hDevice);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // 3. Criar a Interface do Dispositivo
    // Isso "publica" nosso dispositivo usando o GUID do Driver.h,
    // permitindo que o cliente (Parte 2) o encontre.
    status = WdfDeviceCreateDeviceInterface(hDevice, &GUID_INTERFACE_ECHO_DRIVER, nullptr);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    // 4. Criar a WDFQUEUE (Fila de I/O) [cite: 114]
    // Aqui configuramos como queremos receber requisições do cliente
    WDF_IO_QUEUE_CONFIG queueConfig;
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchParallel);

    // 5. Registrar nosso callback para IOCTLs
    // "Quando uma WDFREQUEST [cite: 116] do tipo IOCTL chegar, chame EvtIoDeviceControl"
    queueConfig.EvtIoDeviceControl = EvtIoDeviceControl;

    WDFQUEUE hQueue;
    status = WdfIoQueueCreate(hDevice, &queueConfig, WDF_NO_OBJECT_ATTRIBUTES, &hQueue);

    return status;
}

///
// EvtIoDeviceControl: O coração da lógica
// Chamado quando o cliente envia um IOCTL (DeviceIoControl).
//
VOID EvtIoDeviceControl(
    _In_ WDFQUEUE   Queue,
    _In_ WDFREQUEST Request, // A requisição do cliente
    _In_ size_t     OutputBufferLength,
    _In_ size_t     InputBufferLength,
    _In_ ULONG      IoControlCode
)
{
    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
    PVOID inputBuffer = nullptr;
    PVOID outputBuffer = nullptr;
    size_t inBufSize = 0;
    size_t outBufSize = 0;

    // Verifica se é o nosso IOCTL
    if (IoControlCode == IOCTL_ECHO_DRIVER_ECHO)
    {
        // 6. Obter os buffers da Requisição
        // CORREÇÃO: Usar as funções de UMDF (User-Mode), não KMDF.

        // ANTERIOR (KMDF): status = WdfRequestGetInputBuffer(...)
        status = WdfRequestRetrieveInputBuffer(Request, InputBufferLength, &inputBuffer, &inBufSize);

        if (!NT_SUCCESS(status)) {
            WdfRequestComplete(Request, status);
            return;
        }

        // ANTERIOR (KMDF): status = WdfRequestGetOutputBuffer(...)
        status = WdfRequestRetrieveOutputBuffer(Request, OutputBufferLength, &outputBuffer, &outBufSize);

        if (!NT_SUCCESS(status)) {
            WdfRequestComplete(Request, status);
            return;
        }

        // 7. A Lógica "Eco"
        // Garante que o buffer de saída é grande o suficiente
        if (outBufSize < inBufSize) {
            status = STATUS_BUFFER_TOO_SMALL;
        }
        else {
            // Copia os dados do buffer de entrada para o de saída
            RtlCopyMemory(outputBuffer, inputBuffer, inBufSize);

            // Informa ao cliente quantos bytes foram escritos
            // WdfRequestCompleteWithInformation é segura para KMDF e UMDF
            WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, inBufSize);
            return; // Já completamos a requisição
        }
    }

    // Completa a requisição (mesmo com erro)
    WdfRequestComplete(Request, status);
}