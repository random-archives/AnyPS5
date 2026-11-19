# libSceSystemService

## add Functional

- [Running-app and status guest outputs](../../../core/libs/prx/libSceSystemService/Export.cpp) (libSceSystemService) - `sceLncUtilGetAppLaunchedUser`, `sceSystemServiceGetAppLaunchedUser`, `sceLncUtilIsAppSuspended`, `sceSystemServiceIsAppSuspended` and `sceShellCoreUtilGetSocialScreenStatus` now write their output through `GuestCopyOut_nid_no_patch`: an inaccessible non-null pointer returns `0x80940005` (from the shared helper, or `0x80e30003` from the `sceShellCoreUtil` getters that check the output themselves) instead of faulting the host. The fallback is covered by guest tests but not measured on a console, and neither is whether the console checks the output pointer before the app ID; closes when measured on a PS5.
