#include "prx/libSceSaveData/include/Api.hpp"
#include "prx/libSceSaveData.native/include/Api.hpp"
#include "SceTypes.hpp"
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

constexpr int SaveDataOk = 0;
constexpr int SaveDataErrorParameter = -2137063424;
constexpr int SaveDataErrorNotInitialized = -2137063423;
constexpr int SaveDataErrorNotMounted = -2137063420;
constexpr int SaveDataErrorNotFound = -2137063416;

constexpr std::uint32_t ParamTypeAll = 0;
constexpr std::uint32_t ParamTypeTitle = 1;
constexpr std::uint32_t ParamTypeSubTitle = 2;
constexpr std::uint32_t ParamTypeDetail = 3;
constexpr std::uint32_t ParamTypeUserParam = 4;
constexpr std::uint32_t ParamTypeMtime = 5;

constexpr std::uint32_t MountModeRdWr = 2;
constexpr std::uint32_t MountModeCreate2 = 32;

void Require(bool condition) {
    if (!condition) {
        std::abort();
    }
}

template <typename Function>
void RequireThrowsUnimplemented(Function function, std::string_view expectedName) {
    try {
        function();
    } catch (const std::runtime_error& error) {
        Require(std::string_view(error.what()).find(expectedName) != std::string_view::npos);
        Require(std::string_view(error.what()).find("not implemented") != std::string_view::npos);
        return;
    }
    std::abort();
}

void TestUnmountedAndNullBounds() {
    SaveDataMountPoint point{};
    std::memcpy(point.data, "unmounted", sizeof("unmounted"));

    char buf[256]{};
    std::size_t gotSize = 0;

    Require(sceSaveDataGetParam(nullptr, ParamTypeTitle, buf, sizeof(buf), &gotSize) == SaveDataErrorParameter);
    Require(sceSaveDataGetParam(&point, ParamTypeTitle, nullptr, sizeof(buf), &gotSize) == SaveDataErrorParameter);
    Require(sceSaveDataGetParam(&point, ParamTypeTitle, buf, sizeof(buf), &gotSize) == SaveDataErrorNotMounted);

    Require(sceSaveDataSetParam(nullptr, ParamTypeTitle, buf, sizeof(buf)) == SaveDataErrorParameter);
    Require(sceSaveDataSetParam(&point, ParamTypeTitle, nullptr, sizeof(buf)) == SaveDataErrorParameter);
    Require(sceSaveDataSetParam(&point, ParamTypeTitle, buf, sizeof(buf)) == SaveDataErrorNotMounted);

    SaveDataMountPoint unterminatedPoint{};
    std::memset(unterminatedPoint.data, 'x', sizeof(unterminatedPoint.data));
    Require(sceSaveDataGetParam(&unterminatedPoint, ParamTypeTitle, buf, sizeof(buf), &gotSize) == SaveDataErrorParameter);
    Require(sceSaveDataSetParam(&unterminatedPoint, ParamTypeTitle, buf, sizeof(buf)) == SaveDataErrorParameter);

    SaveDataIcon icon{};
    icon.buf = buf;
    icon.buf_size = sizeof(buf);
    icon.data_size = sizeof(buf);

    Require(sceSaveDataLoadIcon(nullptr, &icon) == SaveDataErrorParameter);
    Require(sceSaveDataLoadIcon(&point, nullptr) == SaveDataErrorParameter);
    SaveDataIcon nullBufIcon{};
    Require(sceSaveDataLoadIcon(&point, &nullBufIcon) == SaveDataErrorParameter);
    Require(sceSaveDataLoadIcon(&unterminatedPoint, &icon) == SaveDataErrorParameter);
    Require(sceSaveDataLoadIcon(&point, &icon) == SaveDataErrorNotMounted);

    Require(sceSaveDataSaveIcon(nullptr, &icon) == SaveDataErrorParameter);
    Require(sceSaveDataSaveIcon(&point, nullptr) == SaveDataErrorParameter);
    Require(sceSaveDataSaveIcon(&point, &nullBufIcon) == SaveDataErrorParameter);
    Require(sceSaveDataSaveIcon(&unterminatedPoint, &icon) == SaveDataErrorParameter);
    Require(sceSaveDataSaveIcon(&point, &icon) == SaveDataErrorNotMounted);

    Require(sceSaveDataSaveIconByPath(nullptr, "icon0.png") == SaveDataErrorParameter);
    Require(sceSaveDataSaveIconByPath(&unterminatedPoint, "icon0.png") == SaveDataErrorParameter);
    Require(sceSaveDataSaveIconByPath(&point, "icon0.png") == SaveDataOk);
}

void TestMountedParamBoundsAndRoundtrip() {
    SceSaveDataDirName dirName{};
    std::memcpy(dirName.data, "param_bounds_save", sizeof("param_bounds_save"));

    SaveDataMount3 mount{};
    mount.user_id = 1;
    mount.dir_name = &dirName;
    mount.mount_mode = MountModeRdWr | MountModeCreate2;

    SaveDataMountResult mountResult{};
    Require(sceSaveDataMount3(&mount, &mountResult) == SaveDataOk);

    char buf[512]{};
    std::size_t gotSize = 0;

    for (const std::uint32_t invalidType : {6u, 7u, 10u, 100u, 0xFFFFFFFFu}) {
        Require(sceSaveDataGetParam(&mountResult.mount_point, invalidType, buf, sizeof(buf), &gotSize) == SaveDataErrorParameter);
        Require(sceSaveDataSetParam(&mountResult.mount_point, invalidType, buf, sizeof(buf)) == SaveDataErrorParameter);
    }

    std::uint32_t userParam = 0x12345678u;
    Require(sceSaveDataSetParam(&mountResult.mount_point, ParamTypeUserParam, &userParam, 0) == SaveDataErrorParameter);
    Require(sceSaveDataSetParam(&mountResult.mount_point, ParamTypeUserParam, &userParam, sizeof(userParam) - 1) == SaveDataErrorParameter);
    Require(sceSaveDataGetParam(&mountResult.mount_point, ParamTypeUserParam, buf, sizeof(userParam) - 1, &gotSize) == SaveDataErrorParameter);

    std::int64_t mtimeVal = 1700000000LL;
    Require(sceSaveDataSetParam(&mountResult.mount_point, ParamTypeMtime, &mtimeVal, sizeof(mtimeVal) - 1) == SaveDataErrorParameter);
    Require(sceSaveDataGetParam(&mountResult.mount_point, ParamTypeMtime, buf, sizeof(mtimeVal) - 1, &gotSize) == SaveDataErrorParameter);

    Require(sceSaveDataSetParam(&mountResult.mount_point, ParamTypeAll, buf, sizeof(SaveDataParam) - 1) == SaveDataErrorParameter);
    Require(sceSaveDataGetParam(&mountResult.mount_point, ParamTypeAll, buf, sizeof(SaveDataParam) - 1, &gotSize) == SaveDataErrorParameter);

    const char titleText[] = "Sample Save Title";
    Require(sceSaveDataSetParam(&mountResult.mount_point, ParamTypeTitle, titleText, sizeof(titleText)) == SaveDataOk);

    char readTitle[128]{};
    Require(sceSaveDataGetParam(&mountResult.mount_point, ParamTypeTitle, readTitle, sizeof(readTitle), &gotSize) == SaveDataOk);
    Require(gotSize == 128);
    Require(std::strcmp(readTitle, titleText) == 0);

    const char subTitleText[] = "Sample Sub Title";
    Require(sceSaveDataSetParam(&mountResult.mount_point, ParamTypeSubTitle, subTitleText, sizeof(subTitleText)) == SaveDataOk);
    char readSubTitle[128]{};
    Require(sceSaveDataGetParam(&mountResult.mount_point, ParamTypeSubTitle, readSubTitle, sizeof(readSubTitle), &gotSize) == SaveDataOk);
    Require(std::strcmp(readSubTitle, subTitleText) == 0);

    const char detailText[] = "Detailed progress summary in save container.";
    Require(sceSaveDataSetParam(&mountResult.mount_point, ParamTypeDetail, detailText, sizeof(detailText)) == SaveDataOk);
    char readDetail[1024]{};
    Require(sceSaveDataGetParam(&mountResult.mount_point, ParamTypeDetail, readDetail, sizeof(readDetail), &gotSize) == SaveDataOk);
    Require(std::strcmp(readDetail, detailText) == 0);

    Require(sceSaveDataSetParam(&mountResult.mount_point, ParamTypeUserParam, &userParam, sizeof(userParam)) == SaveDataOk);
    std::uint32_t readUserParam = 0;
    Require(sceSaveDataGetParam(&mountResult.mount_point, ParamTypeUserParam, &readUserParam, sizeof(readUserParam), &gotSize) == SaveDataOk);
    Require(gotSize == sizeof(userParam));
    Require(readUserParam == userParam);

    Require(sceSaveDataSetParam(&mountResult.mount_point, ParamTypeMtime, &mtimeVal, sizeof(mtimeVal)) == SaveDataOk);
    std::int64_t readMtime = 0;
    Require(sceSaveDataGetParam(&mountResult.mount_point, ParamTypeMtime, &readMtime, sizeof(readMtime), &gotSize) == SaveDataOk);
    Require(gotSize == sizeof(mtimeVal));
    Require(readMtime == mtimeVal);

    SaveDataParam allParam{};
    Require(sceSaveDataGetParam(&mountResult.mount_point, ParamTypeAll, &allParam, sizeof(allParam), &gotSize) == SaveDataOk);
    Require(gotSize == sizeof(SaveDataParam));
    Require(std::strcmp(allParam.title, titleText) == 0);
    Require(std::strcmp(allParam.sub_title, subTitleText) == 0);
    Require(allParam.user_param == userParam);
    Require(allParam.mtime == mtimeVal);

    SaveDataIcon loadIcon{};
    char iconBuffer[256]{};
    loadIcon.buf = iconBuffer;
    loadIcon.buf_size = sizeof(iconBuffer);
    Require(sceSaveDataLoadIcon(&mountResult.mount_point, &loadIcon) == SaveDataErrorNotFound);

    char fakePngData[64]{};
    std::memcpy(fakePngData, "\x89PNG\r\n\x1a\n", 8);
    SaveDataIcon saveIcon{};
    saveIcon.buf = fakePngData;
    saveIcon.buf_size = sizeof(fakePngData);
    saveIcon.data_size = sizeof(fakePngData);
    Require(sceSaveDataSaveIcon(&mountResult.mount_point, &saveIcon) == SaveDataOk);

    Require(sceSaveDataLoadIcon(&mountResult.mount_point, &loadIcon) == SaveDataOk);
    Require(loadIcon.data_size == sizeof(fakePngData));
    Require(std::memcmp(loadIcon.buf, "\x89PNG\r\n\x1a\n", 8) == 0);

    Require(sceSaveDataSaveIconByPath(&mountResult.mount_point, "icon0.png") == SaveDataOk);

    char foreignTitle[16] = "CUSA99999";
    SaveDataMountPs4 mountPs4{};
    mountPs4.title_id = foreignTitle;
    mountPs4.dir_name = &dirName;
    mountPs4.mount_mode = MountModeRdWr | MountModeCreate2;
    RequireThrowsUnimplemented([&] { static_cast<void>(sceSaveDataMount(&mountPs4, &mountResult)); }, "sceSaveDataMount of another title's save data");

    SceSaveDataTitleId foreignTitleId{};
    std::memcpy(foreignTitleId.data, "CUSA99999", sizeof("CUSA99999"));
    SaveDataCheckBackupData check{};
    check.dir_name = &dirName;
    check.title_id = &foreignTitleId;
    Require(sceSaveDataCheckBackupData(&check) == SaveDataErrorNotFound);

    Require(sceSaveDataUmount2(0, &mountResult.mount_point) == SaveDataOk);

    Require(sceSaveDataGetParam(&mountResult.mount_point, ParamTypeTitle, buf, sizeof(buf), &gotSize) == SaveDataErrorNotMounted);
    Require(sceSaveDataSetParam(&mountResult.mount_point, ParamTypeTitle, buf, sizeof(buf)) == SaveDataErrorNotMounted);
    Require(sceSaveDataLoadIcon(&mountResult.mount_point, &loadIcon) == SaveDataErrorNotMounted);
    Require(sceSaveDataSaveIcon(&mountResult.mount_point, &saveIcon) == SaveDataErrorNotMounted);
}

} // namespace

int main() {
    const auto sandbox = std::filesystem::temp_directory_path() / ("anyps5-savedata-param-icon-" + std::to_string(std::random_device{}()));
    std::filesystem::create_directories(sandbox);
    const auto previous = std::filesystem::current_path();
    std::filesystem::current_path(sandbox);

    TestUnmountedAndNullBounds();
    Require(sceSaveDataInitialize3(nullptr) == SaveDataOk);
    TestMountedParamBoundsAndRoundtrip();
    Require(sceSaveDataTerminate() == SaveDataOk);
    Require(sceSaveDataTerminate() == SaveDataErrorNotInitialized);

    std::filesystem::current_path(previous);
    std::error_code error;
    std::filesystem::remove_all(sandbox, error);
    return 0;
}
