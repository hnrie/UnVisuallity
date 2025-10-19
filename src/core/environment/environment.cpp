//
// Created by savage on 18.04.2025.
//

#include <bitset>
#include <mutex>
#include "environment.h"

#include <filesystem>
#include <fstream>

#include "globals.h"
#include "lapi.h"
#include "lstate.h"
#include "src/rbx/taskscheduler/taskscheduler.h"


std::mutex scriptable_map_lock{};
std::optional<bool> get_scriptable_state(uintptr_t instance, const std::string& property_name) noexcept {
    std::unique_lock l{scriptable_map_lock};

    if (g_environment->scriptable_map.contains(instance)) {
        const auto properties = g_environment->scriptable_map[instance];

        if (properties.contains(property_name))
            return properties.at(property_name);
    }

    return std::nullopt;
}

void change_scriptable_state(uintptr_t instance, const std::string &property_name,
                             bool state) noexcept {
    std::unique_lock l{ scriptable_map_lock };

    g_environment->scriptable_map[instance][property_name] = state;
}

static std::vector<std::string> blacklisted_functions = {
        OBF( "DisableDUAR" ),OBF( "DisableDUARAndOpenSurvey" ), OBF( "PerformManagedUpdate"),
        OBF( "GetFilename" ),
        OBF( "Upload" ),
        OBF( "AssetImportSession" ),
        OBF( "AddNewPlace" ),
        OBF( "PublishLinkedSource" ),
        OBF( "SavePlaceAsync" ),
        OBF( "NoPromptCreateOutfit" ),
        OBF( "NoPromptDeleteOutfit" ),
        OBF( "NoPromptRenameOutfit" ),
        OBF( "NoPromptSaveAvatar" ),
        OBF( "NoPromptSaveAvatarThumbnailCustomization" ),
        OBF( "NoPromptSetFavorite" ),
        OBF( "NoPromptUpdateOutfit" ),
        OBF( "PerformCreateOutfitWithDescription" ),
        OBF( "PerformDeleteOutfit" ),
        OBF( "PerformRenameOutfit" ),
        OBF( "PerformSaveAvatarWithDescription" ),
        OBF( "PerformSetFavorite" ),
        OBF( "PerformUpdateOutfit" ),
        OBF( "SetAllowInventoryReadAccess" ),
        OBF( "SignalCreateOutfitFailed" ),
        OBF( "SignalCreateOutfitPermissionDenied" ),
        OBF( "SignalDeleteOutfitFailed" ),
        OBF( "SignalDeleteOutfitPermissionDenied" ),
        OBF( "SignalRenameOutfitFailed" ),
        OBF( "SignalRenameOutfitPermissionDenied" ),
        OBF( "SignalSaveAvatarFailed" ),
        OBF( "SignalSaveAvatarPermissionDenied" ),
        OBF( "SignalSetFavoriteFailed" ),
        OBF( "SignalSetFavoritePermissionDenied" ),
        OBF( "SignalUpdateOutfitFailed" ),
        OBF( "SignalUpdateOutfitPermissionDenied" ),
        OBF( "ImportFBXAnimationFromFilePathUserMayChooseModel" ),
        OBF( "ImportFBXAnimationUserMayChooseModel" ),
        OBF( "ImportFbxRigWithoutSceneLoad" ),
        OBF( "ImportLoadedFBXAnimation" ),
        OBF( "CloseBrowserWindow" ),
        OBF( "CopyAuthCookieFromBrowserToEngine" ),
        OBF( "EmitHybridEvent" ),
        OBF( "ExecuteJavaScript" ),
        OBF("Load"),
        OBF( "OpenBrowserWindow" ),
        OBF( "OpenNativeOverlay" ),
        OBF( "OpenWeChatAuthWindow" ),
        OBF( "ReturnToJavaScript" ),
        OBF( "SendCommand" ),
        OBF( "RetreiveCaptures" ),
        OBF( "SaveCaptureToExternalStorage" ),
        OBF( "SaveCapturesToExternalStorageAsync" ),
        OBF( "SaveScreenshotCapture" ),
        OBF( "DeleteCapturesAsync" ),
        OBF( "DeleteCaptureAsync" ),
        OBF( "DeleteCapture" ),
        OBF( "DeleteCaptures" ),
        OBF( "GetCaptureFilePathAsync" ),
        OBF( "CaptureScreenshot" ),
        OBF( "ChatLocal" ),
        OBF( "RegisterExecutionCallback" ),
        OBF( "CommandInstance" ),
        OBF( "Execute" ),
        OBF( "RegisterCommand" ),
        OBF( "GetFailedRequests" ),
        OBF( "SetBaseUrl" ),
        OBF( "CallFunction" ),
        OBF( "TakeScreenshot" ),
        OBF( "ToggleRecording" ),
        OBF( "GetScriptFilePath" ),
        OBF( "CoreScriptSyncService" ),
        OBF( "DefineFastInt" ),
        OBF( "DefineFastString" ),
        OBF( "OpenScreenshotsFolder" ),
        OBF( "OpenVideosFolder" ),
        OBF( "SetFastFlagForTesting" ),
        OBF( "SetFastIntForTesting" ),
        OBF( "SetFastStringForTesting" ),
        OBF( "ScreenshotReady" ),
        OBF( "SetVideoInfo" ),
        OBF( "ReportInGoogleAnalytics" ),
        OBF( "BroadcastNotification" ),
        OBF( "GetAsync" ),
        OBF( "GetAsyncFullUrl" ),
        OBF( "PostAsync" ),
        OBF( "PostAsyncFullUrl" ),
        OBF( "RequestAsync" ),
        OBF( "RequestLimitedAsync" ),
        OBF( "RequestInternal" ),
        OBF( "RequestInternal" ),
        OBF( "GetLocalFileContents" ),
        OBF( "PromptDownloadGameTableToCSV" ),
        OBF( "PromptExportToCSVs" ),
        OBF( "PromptImportFromCSVs" ),
        OBF( "PromptUploadCSVToGameTable" ),
        OBF( "SetRobloxLocaleId" ),
        OBF( "StartTextScraper" ),
        OBF( "StopTextScraper" ),
        OBF( "Logout" ),
        OBF( "PromptLogin" ),
        OBF( "ExecuteScript" ),
        OBF( "GetHttpResultHistory" ),
        OBF( "RequestHttpResultApproved" ),
        OBF( "RequestServerHttpResult" ),
        OBF( "GetRobuxBalance" ),
        OBF( "PerformPurchaseV2" ),
        OBF( "PrepareCollectiblesPurchase" ),
        OBF( "GetSubscriptionProductInfoAsync" ),
        OBF( "GetSubscriptionPurchaseInfoAsync" ),
        OBF( "GetUserSubscriptionPaymentHistoryAsync" ),
        OBF( "GetUserSubscriptionStatusAsync" ),
        OBF( "PerformPurchase" ),
        OBF( "PromptGamePassPurchase" ),
        OBF( "PromptNativePurchase" ),
        OBF( "PromptProductPurchase" ),
        OBF( "PromptThirdPartyPurchase" ),
        OBF( "ReportAssetSale" ),
        OBF( "ReportRobuxUpsellStarted" ),
        OBF( "SignalAssetTypePurchased" ),
        OBF( "SignalClientPurchaseSuccess" ),
        OBF( "SignalMockPurchasePremium" ),
        OBF( "SignalServerLuaDialogClosed" ),
        OBF( "PromptRobloxPurchase" ),
        OBF( "PerformPurchaseV3" ),
        OBF( "PromptBundlePurchase" ),
        OBF( "PromptSubscriptionPurchase" ),
        OBF( "PerformSubscriptionPurchase" ),
        OBF( "PerformBulkPurchase" ),
        OBF( "PromptBulkPurchase" ),
        OBF( "GetUserSubscriptionDetailsInternalAsync" ),
        OBF( "PlayerOwnsAsset" ),
        OBF( "PlayerOwnsBundle" ),
        OBF( "PromptCollectiblesPurchase" ),
        OBF( "PromptCancelSubscription" ),
        OBF( "PromptNativePurchaseWithLocalPlayer" ),
        OBF( "PromptPremiumPurchase" ),
        OBF( "RefillAccountingBalanceAsync" ),
        OBF( "StartSession" ),
        OBF( "GenerateImagesAsync" ),
        OBF( "GenerateMaterialMapsAsync" ),
        OBF( "UploadMaterialAsync" ),
        OBF( "GetLast" ),
        OBF( "GetMessageId" ),
        OBF( "GetProtocolMethodRequestMessageId" ),
        OBF( "GetProtocolMethodResponseMessageId" ),
        OBF( "MakeRequest" ),
        OBF( "Publish" ),
        OBF( "PublishProtocolMethodRequest" ),
        OBF( "PublishProtocolMethodResponse" ),
        OBF( "SetRequestHandler" ),
        OBF( "Subscribe" ),
        OBF( "SubscribeToProtocolMethodRequest" ),
        OBF( "SubscribeToProtocolMethodResponse" ),
        OBF( "Call" ),
        OBF( "SwitchedToAppShellFeature" ),
        OBF( "ClearSessionId" ),
        OBF( "ConvertToPackageUpload" ),
        OBF( "PublishPackage" ),
        OBF( "SetPackageVersion" ),
        OBF( "AddToBlockList" ),
        OBF( "RequestFriendship" ),
        OBF( "RevokeFriendship" ),
        OBF( "UpdatePlayerBlocked" ),
        OBF( "ReportAbuse" ),
        OBF( "ReportAbuseV3" ),
        OBF( "TeamChat" ),
        OBF( "WhisperChat" ),
        OBF( "AddCoreScriptLocal" ),
        OBF( "DeserializeScriptProfilerString" ),
        OBF( "SaveScriptProfilingData" ),
        OBF( "sendRobloxEvent" ),
        OBF( "HttpRequestAsync" ),
        OBF( "DetectUrl" ),
        OBF( "GetAndClearLastPendingUrl" ),
        OBF( "GetLastLuaUrl" ),
        OBF( "IsUrlRegistered" ),
        OBF( "OpenUrl" ),
        OBF( "RegisterLuaUrl" ),
        OBF( "StartLuaUrlDelivery" ),
        OBF( "StopLuaUrlDelivery" ),
        OBF( "SupportsSwitchToSettingsApp" ),
        OBF( "SwitchToSettingsApp" ),
        OBF( "OnLuaUrl" ),
        OBF( "PromptRealWorldCommerceBrowser" ),
        OBF( "InExperienceBrowserRequested" ),
        OBF( "SubscribeBlock" ),
        OBF( "SubscribeUnblock" ),
        OBF( "SaveScriptProfilingData" ),
        OBF( "CreateAssetAndWaitForAssetId" ),
        OBF( "CreateAssetOrAssetVersionAndPollAssetWithTelemetryAsync" ),
        OBF( "PublishCageMeshAsync" ),
        OBF( "PublishDescendantAssets" ),
        OBF( "GetCameraDevices" ),
        OBF( "CanSendCallInviteAsync" ),
        OBF( "CanSendGameInviteAsync" ),
        OBF( "InvokeGameInvitePromptClosed" ),
        OBF( "HideSelfView" ),
        OBF( "InvokeIrisInvite" ),
        OBF( "InvokeIrisInvitePromptClosed" ),
        OBF( "PromptGameInvite" ),
        OBF( "PromptPhoneBook" ),
        OBF( "ShowSelfView" ),
        OBF( "CallInviteStateChanged" ),
        OBF( "GameInvitePromptClosed" ),
        OBF( "IrisInviteInitiated" ),
        OBF( "PhoneBookPromptClosed" ),
        OBF( "PromptInviteRequested" ),
        OBF( "PromptIrisInviteRequested" ),
};

void *datamodel;
lua_CFunction old_index;
lua_CFunction old_newindex;
lua_CFunction old_namecall;

int index_hook(lua_State* L) {
    if (!L->userdata || !L->userdata->script.expired())
        return old_index(L);

    int atom{};
    std::string data = lua_tostringatom(L, 2, &atom);

    std::string aa = data;
    aa[0] = toupper(data[0]);

    if (std::ranges::find(blacklisted_functions, aa) != blacklisted_functions.end())
        luaL_error(L, OBF("malicious function"));


    if (lua_isuserdata(L, 1) && lua_touserdata(L, 1) == datamodel && lua_isstring(L, 2)) {


        if (data == OBF("HttpGet") || data == OBF("HttpGetAsync")) {
            lua_pushcclosurek(L, environment::http_get, nullptr, 0, nullptr);
            return 1;
        } else if (data == OBF("GetObjects")) {
            lua_pushcclosurek(L, environment::get_objects, nullptr, 0, nullptr);
            return 1;
        }
    } else if (lua_isuserdata(L, 1) && lua_isstring(L, 2)) {
        const auto instance = *reinterpret_cast<uintptr_t*>(lua_touserdata(L, 1));

        if (std::optional<bool> cached_scriptable = get_scriptable_state(instance, data); cached_scriptable.has_value()) {


            std::uintptr_t prop_desc = rbx::property_descriptor::ktable[atom];
            if (!prop_desc)
                luaL_argerrorL(L, 2, OBF("property does not exist"));

            uintptr_t class_desc = *reinterpret_cast<uintptr_t *>(instance + 0x18);
            const auto property_descriptor = rbx::property_descriptor::get_property(class_desc + 0x3B8, &prop_desc);
            if (!property_descriptor)
                luaL_argerrorL(L, 2, OBF("property does not exist"));

            DWORD scriptable_value = *reinterpret_cast<DWORD*>(*property_descriptor + 0x48);
            *reinterpret_cast<DWORD*>(*property_descriptor + 0x48) = std::bitset< 32 >{ scriptable_value }.set(5, cached_scriptable.value()).to_ulong();

            int res = old_index(L);

            *reinterpret_cast<DWORD*>(*property_descriptor + 0x48) = scriptable_value;

            return res;

        }
    }

    return old_index(L);
}

int newindex_hook(lua_State *L) {
    if (!L->userdata || !L->userdata->script.expired())
        return old_newindex(L);

    int atom{};
    const auto instance = *static_cast<uintptr_t*>(lua_touserdata(L, 1));
    const std::string data = lua_tostringatom(L, 2, &atom);

    if (lua_isuserdata(L, 1) && lua_isstring(L, 2)) {
        if (std::optional<bool> cached_scriptable = get_scriptable_state(instance, data); cached_scriptable.has_value()) {


            std::uintptr_t prop_desc = rbx::property_descriptor::ktable[atom];
            if (!prop_desc)
                luaL_argerrorL(L, 2, OBF("property does not exist"));

            const uintptr_t class_desc = *reinterpret_cast<uintptr_t *>(instance + 0x18);
            const auto property_descriptor = rbx::property_descriptor::get_property(class_desc + 0x3B8, &prop_desc);
            if (!property_descriptor)
                luaL_argerrorL(L, 2, OBF("property does not exist"));

            const DWORD scriptable_value = *reinterpret_cast<DWORD*>(*property_descriptor + 0x48);
            *reinterpret_cast<DWORD*>(*property_descriptor + 0x48) = std::bitset< 32 >{ scriptable_value }.set(5, cached_scriptable.value()).to_ulong();

            const int res = old_newindex(L);

            *reinterpret_cast<DWORD*>(*property_descriptor + 0x48) = scriptable_value;

            return res;

        }
    }

    return old_newindex(L);
}

int namecall_hook(lua_State* L) {
    if (!L->userdata || !L->userdata->script.expired())
        return old_namecall(L);

    const auto script_pointer = L->userdata->script;

    std::string method = getstr(L->namecall);

    method[0] = toupper(method[0]);

    if (std::ranges::find(blacklisted_functions, method) != blacklisted_functions.end())
        luaL_error(L, OBF("malicious function"));

    if ((lua_isuserdata(L, 1) && lua_touserdata(L, 1) == datamodel))
    {

        if (method == OBF("HttpGet") || method == OBF("HttpGetAsync"))
        {
            return environment::http_get(L);
        } else if (method == OBF("GetObjects") || method == OBF("GetObjectsAsync")) {
            return environment::get_objects(L);
        }
    }

    return old_namecall(L);
}

std::vector<std::string> environment::teleport_queue = {};
void environment::initialize(lua_State *L) {
    const auto hook_game_metamethod = [L](const char* const metamethod, const lua_CFunction hook)
    {
        lua_getglobal(L, OBF("game"));
        lua_getmetatable(L, -1);
        lua_getfield(L, -1, metamethod);

        Closure *metamethod_closure = clvalue(luaA_toobject(L, -1));

        const lua_CFunction original = metamethod_closure->c.f;
        metamethod_closure->c.f = hook;

        lua_pop(L, 3);

        return original;
    };

   // lua_pop(L, lua_gettop(L)); // ensure empty stackk

    lua_getglobal(L, OBF("game"));
    datamodel = lua_touserdata(L, -1);
    lua_pop(L, 1);

    lua_newtable(L);
    lua_setglobal(L, OBF("shared"));

    lua_newtable(L);
    lua_setglobal(L, OBF("_G"));

    g_environment->load_misc_lib(L);
    g_environment->load_http_lib(L);
    g_environment->load_closure_lib(L);
    g_environment->load_debug_lib(L);
    g_environment->load_filesystem_lib(L);
    g_environment->load_scripts_lib(L);
    g_environment->load_cache_lib(L);
    g_environment->load_crypt_lib(L);
    g_environment->load_metatables_lib(L);
    g_environment->load_signals_lib(L);
    g_environment->load_input_lib(L);
    g_environment->load_drawing_lib(L);
    g_environment->load_misc_lib(L);
    g_environment->load_console_lib(L);
    g_environment->load_actor_lib(L);
    g_environment->load_websockets_lib(L);
  //  g_environment->load_raknet_lib(L);


    old_namecall = hook_game_metamethod(OBF("__namecall"), &namecall_hook);
    old_index = hook_game_metamethod(OBF("__index"), &index_hook);
    old_newindex = hook_game_metamethod(OBF("__newindex"), &newindex_hook);


    for (const std::filesystem::directory_entry& directory_entry : std::filesystem::directory_iterator(globals::autoexec_path)) {
        if (std::filesystem::exists(directory_entry) && std::filesystem::is_regular_file(directory_entry)) {
            const std::filesystem::path& FilePath = directory_entry.path();
            std::ifstream fs_stream(FilePath, std::ios::binary);

            if (fs_stream.is_open()) {
                std::string f_content;
                std::copy(std::istreambuf_iterator<char>(fs_stream), std::istreambuf_iterator<char>(), std::back_inserter(f_content));
                fs_stream.close();

                g_taskscheduler->queue_script(f_content);
            }
        }
    }

    for (const auto& scripto: environment::teleport_queue) {
        g_taskscheduler->queue_script(scripto);
    }
    environment::teleport_queue.clear();


}

void environment::reset()
{
    this->reset_closure_lib();
    this->reset_drawing_lib();
    this->reset_scripts_lib();
    this->reset_signals_lib();
    this->reset_websocket_lib();
}