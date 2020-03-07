# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES )
set(COMPONENT_PRIV_REQUIRES )

set(COMPONENT_SRCS "main.c" "wifi.c" "http_srv.c" "utils.c" "params.c" "cayenne.c" "display/epdif.c" "display/epd1in54.c" "display/epdpaint.c" "display.c" "display/font39pixel_h_digit.c" "display/fontStreched72.c" "ulp_sensor.c")

set(COMPONENT_ADD_INCLUDEDIRS "")

register_component()

#
# ULP support additions to component CMakeLists.txt.
#
# 1. The ULP app name must be unique (if multiple components use ULP).
set(ulp_app_name ulp_${COMPONENT_NAME})
#
# 2. Specify all assembly source files.
#    Files should be placed into a separate directory (in this case, ulp/),
#    which should not be added to COMPONENT_SRCS.
set(ulp_s_sources "ulp/sensor.S")
#
# 3. List all the component source files which include automatically
#    generated ULP export file, ${ulp_app_name}.h:
set(ulp_exp_dep_srcs "main.c")
#
# 4. Call function to build ULP binary and embed in project using the argument
#    values above.
ulp_embed_binary(${ulp_app_name} ${ulp_s_sources} ${ulp_exp_dep_srcs})

# Create a SPIFFS image from the contents of the 'spiffs_image' directory
# that fits the partition named 'storage'. FLASH_IN_PROJECT indicates that
# the generated image should be flashed when the entire project is flashed to
# the target with 'idf.py -p PORT flash'. 
#spiffs_create_partition_image(storage ../html FLASH_IN_PROJECT)

