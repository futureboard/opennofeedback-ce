// ============================================================================
//  config.h  — iPlug2 build configuration for OpenDeFeedback
// ============================================================================
#define PLUG_NAME "OpenDeFeedback"
#define PLUG_MFR "OpenDeFeedback"
#define PLUG_VERSION_HEX 0x00000100
#define PLUG_VERSION_STR "0.1.0"
#define PLUG_UNIQUE_ID 'Odfb'
#define PLUG_MFR_ID 'Odfx'
#define PLUG_URL_STR "https://github.com/arizkami/defeedback-ce"
#define PLUG_EMAIL_STR "noreply@example.com"
#define PLUG_COPYRIGHT_STR "Copyright 2026 OpenDeFeedback contributors"
#define PLUG_CLASS_NAME OpenDeFeedback

#define BUNDLE_NAME "OpenDeFeedback"
#define BUNDLE_MFR "OpenDeFeedback"
#define BUNDLE_DOMAIN "com"

#define SHARED_RESOURCES_SUBPATH "OpenDeFeedback"

// 1-in/1-out and 2-in/2-out. Realtime live cleanup is mono or stereo.
#define PLUG_CHANNEL_IO "1-1 2-2"

#define PLUG_LATENCY 0          // strictly causal, zero-latency
#define PLUG_TYPE 0             // 0 == effect
#define PLUG_DOES_MIDI_IN 0
#define PLUG_DOES_MIDI_OUT 0
#define PLUG_DOES_MPE 0
#define PLUG_DOES_STATE_CHUNKS 0
#define PLUG_HAS_UI 1
#define PLUG_WIDTH 600
#define PLUG_HEIGHT 340
#define PLUG_FPS 60
#define PLUG_SHARED_RESOURCES 0
#define PLUG_HOST_RESIZE 0

#define AUV2_ENTRY OpenDeFeedback_Entry
#define AUV2_ENTRY_STR "OpenDeFeedback_Entry"
#define AUV2_FACTORY OpenDeFeedback_Factory
#define AUV2_VIEW_CLASS OpenDeFeedback_View
#define AUV2_VIEW_CLASS_STR "OpenDeFeedback_View"

#define AAX_TYPE_IDS 'ODF1', 'ODF2'
#define AAX_TYPE_IDS_AUDIOSUITE 'ODA1', 'ODA2'
#define AAX_PLUG_MFR_STR "OpenDeFeedback"
#define AAX_PLUG_NAME_STR "OpenDeFeedback\nODFB"
#define AAX_PLUG_CATEGORY_STR "Effect"
#define AAX_DOES_AUDIOSUITE 1

#define VST3_SUBCATEGORY "Fx|Restoration"

#define CLAP_MANUAL_URL "https://github.com/arizkami/defeedback-ce"
#define CLAP_SUPPORT_URL "https://github.com/arizkami/defeedback-ce/issues"
#define CLAP_DESCRIPTION "Realtime live-audio feedback / noise / reverb cleanup"
#define CLAP_FEATURES "audio-effect", "restoration", "utility"

#define APP_NUM_CHANNELS 2
#define APP_N_VECTOR_WAIT 0
#define APP_MULT 1
#define APP_COPY_AUV3 0
#define APP_SIGNAL_VECTOR_SIZE 64

// UI fonts embedded as binary resources (Windows). The UI loads INTER_FN first
// and falls back to the bundled face if absent (see ui/Theme.cpp).
#define INTER_FN "InterVariable.ttf"
#define ROBOTO_FN "Roboto-Regular.ttf"
