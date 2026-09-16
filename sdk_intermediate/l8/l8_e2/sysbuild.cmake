# Add external project as sysbuild image for the FLPR core or Network Core
ExternalZephyrProject_Add(
  APPLICATION custom_image
  SOURCE_DIR ${APP_DIR}/../custom_image
  BOARD nrf54l15dk/nrf54l15/cpuflpr
)