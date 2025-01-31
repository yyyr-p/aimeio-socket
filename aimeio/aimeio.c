#include <windows.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "aimeio/aimeio.h"

struct IPCMemoryInfo
{
    bool polling;
    bool cardPresent;
    bool isFeliCa;
    uint8_t cachedLuid[10];
    uint8_t cachedIDmPMm[16];
    uint8_t led[3];
};
typedef struct IPCMemoryInfo IPCMemoryInfo;
static HANDLE FileMappingHandle;
IPCMemoryInfo* FileMapping;

HRESULT initSharedMemory()
{
    if (FileMapping)
    {
        return S_OK;
    }
    if ((FileMappingHandle = CreateFileMapping(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, sizeof(IPCMemoryInfo), "Local\\AIMEIO_SOCKET_SHARED_BUFFER")) == 0)
    {
        return E_OUTOFMEMORY;
    }

    if ((FileMapping = (IPCMemoryInfo*)MapViewOfFile(FileMappingHandle, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(IPCMemoryInfo))) == 0)
    {
        return E_FAIL;
    }

    memset(FileMapping, 0, sizeof(IPCMemoryInfo));
    SetThreadExecutionState(ES_DISPLAY_REQUIRED | ES_CONTINUOUS);
    return S_OK;
}

uint16_t aime_io_get_api_version(void)
{
    printf("aimeio-socket: aime_io_get_api_version - 0x0100\n");
    return 0x0100;
}

HRESULT aime_io_init(void)
{
    int ret;
    FILE* fp;
    HRESULT hr;

    ret = AllocConsole();

    // someone might already allocated a console - seeing this on fufubot's segatools
    if (ret != 0) {
        // only when we allocate a console, we need to redirect stdout
        freopen_s(&fp, "CONOUT$", "w", stdout);
    }

    hr = initSharedMemory();
    if (hr != S_OK) {
        printf("aimeio-socket: failed to initialize: 0x%x\n", hr);
        return hr;
    }

    printf("aimeio-socket: aime_io_init\n");

    return S_OK;
}

HRESULT aime_io_nfc_poll(uint8_t unit_no)
{
    if (unit_no != 0) {
        return E_FAIL;
    }
    
    printf("aimeio-socket: aime_io_nfc_poll\n");
    FileMapping->polling = true;

    return S_OK;
}

HRESULT aime_io_nfc_get_aime_id(
        uint8_t unit_no,
        uint8_t *luid,
        size_t luid_size)
{
    assert(luid != NULL);

    if (unit_no != 0 || !FileMapping->cardPresent || FileMapping->isFeliCa) {
        printf("aimeio-socket: aime_io_nfc_get_aime_id = S_FALSE\n");
        return S_FALSE;
    }

    printf("aimeio-socket: aime_io_nfc_get_aime_id = S_OK\n");

    memcpy(luid, FileMapping->cachedLuid, luid_size);
    
    FileMapping->cardPresent = false;
    memset(FileMapping->cachedLuid, 0, sizeof(FileMapping->cachedLuid));

    return S_OK;
}

HRESULT aime_io_nfc_get_felica_id(uint8_t unit_no, uint64_t *IDm)
{
    assert(IDm != NULL);

    uint64_t val;
    size_t i;
    assert(IDm != NULL);

    if (unit_no != 0 || !FileMapping->cardPresent || !FileMapping->isFeliCa) {
        printf("aimeio-socket: aime_io_nfc_get_felica_id = S_FALSE\n", *IDm);
        return S_FALSE;
    }

    val = 0;
    for (i = 0 ; i < 8 ; i++) {
        val = (val << 8) | FileMapping->cachedIDmPMm[i];
    }

    *IDm = val;
    printf("aimeio-socket: aime_io_nfc_get_felica_id = S_OK - %llx\n", *IDm);

    FileMapping->cardPresent = false;
    memset(FileMapping->cachedIDmPMm, 0, sizeof(FileMapping->cachedIDmPMm));

    return S_OK;
}

void aime_io_led_set_color(uint8_t unit_no, uint8_t r, uint8_t g, uint8_t b)
{
    if (unit_no != 0) {
        return;
    }

    printf("aimeio-socket: aime_io_led_set_color(%d, %d, %d)\n", r, g, b);

    FileMapping->led[0] = r;
    FileMapping->led[1] = g;
    FileMapping->led[2] = b;
}
