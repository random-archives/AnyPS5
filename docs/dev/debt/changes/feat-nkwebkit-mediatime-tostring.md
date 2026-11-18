# libSceNKWebKit

## change [Media and playback engine stubs]

- [Media and playback engine stubs](../../../core/libs/prx/libSceNKWebKit/src/Media.cpp) (libSceNKWebKit) - WebCore::MediaPlayer, HTMLMediaElement::fastSeek, TextTrackCueGeneric::create, MediaTime::toJSONObject, MediaTime::toJSONString, MediaTimeRange::toJSONString, and dump(PrintStream&) exports remain throwing stubs in Export.cpp because they require the WebCore playback pipeline, DOM/engine objects, JSON object model, or PrintStream runtime.

## remove `_ZNK3WTF9MediaTime8toStringEv`
