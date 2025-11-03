/*
 * Driver.h
 *
 * Este arquivo define a interface pública entre o nosso
 * driver UMDF e a aplicação cliente (User-Mode).
 */

#pragma once

#include <windows.h>
#include <winioctl.h> // Para CTL_CODE

 // 1. Definir o GUID da Interface do Dispositivo
 // O cliente usará este GUID para encontrar o nosso dispositivo.
 // {AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE}
DEFINE_GUID(GUID_INTERFACE_ECHO_DRIVER,
    0xAAAAAAAA, 0xBBBB, 0xCCCC, 0xDD, 0xDD, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE, 0xEE);

// 2. Definir o Código de Controle de I/O (IOCTL)
// Este é o "comando" customizado que o cliente enviará.
// Ele deve ser único.
#define IOCTL_ECHO_DRIVER_ECHO CTL_CODE( \
    FILE_DEVICE_UNKNOWN,  /* Tipo de dispositivo */ \
    0x800,                /* Função (0x800+ são para clientes) */ \
    METHOD_BUFFERED,      /* Método de buffer (o mais simples) */ \
    FILE_ANY_ACCESS       /* Acesso requerido */ \
)