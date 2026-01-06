#include <extdll.h>     // Standard Half-Life Engine definitions
#include <meta_api.h>   // Metamod API for plugin management
#include <rehlds_api.h> // ReHLDS API for advanced engine hooking

// Global variables to store the engine interfaces
IRehldsApi* g_RehldsApi;
IRehldsHookchains* g_RehldsHooks;

// This function intercepts the server's response to info queries (A2S_INFO)
void Hook_HandleQuery(IRehldsHook_HandleQuery* chain, netadr_t from, char* query, char* buffer, int* buffer_size) {
    // 1. Let the engine fill the response buffer with original data first
    chain->callNext(from, query, buffer, buffer_size);

    // 2. Check if the response is an "INFO" packet (A2S_INFO starts with 'I' at index 4)
    if (buffer[4] == 'I') {
        // We need to skip 4 variable-length strings (Name, Map, Folder, Game) 
        // to find the byte that stores the bot count.
        int offset = 5;
        for (int i = 0; i < 4; i++) {
            while (buffer[offset++] != '\0'); // Move past each null-terminated string
        }

        // Skip over the protocol, steam ID, and player count bytes
        offset += 4;

        // 3. FORCE BOT COUNT TO ZERO
        // This hides the "(Bot)" tag in the server browser player list.
        buffer[offset] = 0;
    }
}

// Meta_Attach: Runs when Metamod loads the DLL
C_EXP_DET bool Meta_Attach(PLID plid, GETENTITYAPI_FN pfnGetEntityAPI, ...) {
    // 4. FORCE 1000 FPS
    // This sets the Windows system timer resolution to 1ms.
    // This is required to achieve a stable 1000 FPS on Windows servers.
    timeBeginPeriod(1);

    // Get the ReHLDS API interface
    g_RehldsApi = (IRehldsApi*)pfnGetEntityAPI(VREHLDS_API_VERSION, NULL, NULL);
    if (!g_RehldsApi) {
        return false; // Fail if ReHLDS is not found
    }

    // Register our "HandleQuery" hook into the engine's network chain
    g_RehldsHooks = g_RehldsApi->GetHookchains();
    g_RehldsHooks->HandleQuery()->registerHook(Hook_HandleQuery, 0);

    return true;
}

// Meta_Detach: Runs when the plugin is unloaded
C_EXP_DET bool Meta_Detach(PLID plid, ...) {
    // Restore the Windows system timer to default
    timeEndPeriod(1);
    return true;
}
