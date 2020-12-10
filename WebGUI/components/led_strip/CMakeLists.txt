set(srcs
    "led_strip.c")

set(includes "include")

idf_component_register(SRCS "led_strip.c"
                    INCLUDE_DIRS "include"
                    REQUIRES freertos driver) #cannot totally hide soc headers, since there are a lot arguments in the driver are chip-dependent
