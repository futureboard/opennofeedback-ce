# ============================================================================
#  OpenDeFeedback plugin target
#
#  This directory holds config.h and resources/, so it is the natural place to
#  call iplug_add_plugin(): PROJECT_DIR (== CMAKE_CURRENT_SOURCE_DIR) and its
#  resources/ subdir are placed on the include path automatically.
# ============================================================================

# DSP / ML / common sources live one level up under src/.
set(ODF_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")

set(ODF_SOURCES
  # plugin
  OpenDeFeedback.cpp
  OpenDeFeedback.h
  OpenDeFeedback_Controls.cpp
  OpenDeFeedback_Controls.h
  OpenDeFeedback_UI.cpp
  config.h

  # dsp
  ${ODF_SRC_DIR}/dsp/Processor.cpp
  ${ODF_SRC_DIR}/dsp/Processor.h
  ${ODF_SRC_DIR}/dsp/FeedbackDetector.cpp
  ${ODF_SRC_DIR}/dsp/FeedbackDetector.h
  ${ODF_SRC_DIR}/dsp/AdaptiveNotchBank.cpp
  ${ODF_SRC_DIR}/dsp/AdaptiveNotchBank.h
  ${ODF_SRC_DIR}/dsp/Biquad.cpp
  ${ODF_SRC_DIR}/dsp/Biquad.h
  ${ODF_SRC_DIR}/dsp/EnvelopeFollower.cpp
  ${ODF_SRC_DIR}/dsp/EnvelopeFollower.h
  ${ODF_SRC_DIR}/dsp/NoiseEstimator.cpp
  ${ODF_SRC_DIR}/dsp/NoiseEstimator.h
  ${ODF_SRC_DIR}/dsp/DereverbTailSuppressor.cpp
  ${ODF_SRC_DIR}/dsp/DereverbTailSuppressor.h
  ${ODF_SRC_DIR}/dsp/ArtifactGuard.cpp
  ${ODF_SRC_DIR}/dsp/ArtifactGuard.h
  ${ODF_SRC_DIR}/dsp/SmoothedValue.h

  # ml (placeholders)
  ${ODF_SRC_DIR}/ml/FeatureExtractor.cpp
  ${ODF_SRC_DIR}/ml/FeatureExtractor.h
  ${ODF_SRC_DIR}/ml/TinyModelStub.cpp
  ${ODF_SRC_DIR}/ml/TinyModelStub.h
  ${ODF_SRC_DIR}/ml/ModelWeightsStub.h

  # common
  ${ODF_SRC_DIR}/common/Parameters.h
  ${ODF_SRC_DIR}/common/RealtimeUtils.h
  ${ODF_SRC_DIR}/common/CircularBuffer.h
)

# On Windows, the resource script embeds the UI font into *every* binary
# (VST3 module + standalone app) so IGraphics/NanoVG can render text. Point the
# RC compiler at the fonts directory so `ROBOTO_FN TTF ROBOTO_FN` resolves.
if(WIN32)
  set(ODF_RC "${CMAKE_CURRENT_SOURCE_DIR}/resources/OpenDeFeedback.rc")
  set_source_files_properties(${ODF_RC} PROPERTIES
    COMPILE_FLAGS "/I\"${CMAKE_CURRENT_SOURCE_DIR}/resources/fonts\" /I\"${CMAKE_CURRENT_SOURCE_DIR}/resources\"")
  list(APPEND ODF_SOURCES ${ODF_RC})
endif()

# Build the standalone app + VST3. CLAP/LV2/AU/AAX are intentionally left out of
# the MVP (add e.g. FORMATS APP VST3 CLAP once those SDKs are fetched).
iplug_add_plugin(OpenDeFeedback
  SOURCES
    ${ODF_SOURCES}
  RESOURCES
    resources/fonts/Roboto-Regular.ttf
  FORMATS APP VST3
  UI IGRAPHICS
)
