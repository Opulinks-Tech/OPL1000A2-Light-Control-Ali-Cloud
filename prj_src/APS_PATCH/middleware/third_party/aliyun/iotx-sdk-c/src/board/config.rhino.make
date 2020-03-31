-include $(TOP_DIR)/aos_board_conf.mk

CONFIG_ENV_CFLAGS   += \
    -Os -DBUILD_AOS \
    -I$(TOP_DIR)/../../../../../include \
    -I$(TOP_DIR)/../../../../activation \

CONFIG_ENV_CFLAGS   += \
    -DCONFIG_HTTP_AUTH_TIMEOUT=500 \
    -DCONFIG_MID_HTTP_TIMEOUT=500 \
    -DCONFIG_GUIDER_AUTH_TIMEOUT=500 \
    -DCONFIG_MQTT_TX_MAXLEN=640 \
    -DCONFIG_MQTT_RX_MAXLEN=1200 \

CONFIG_ENV_CFLAGS               += -Werror
CONFIG_src/ref-impl/tls         :=
CONFIG_src/ref-impl/hal         :=
CONFIG_examples                 :=
CONFIG_src/services/uOTA        :=
CONFIG_tests                    :=
CONFIG_src/tools/linkkit_tsl_convert :=
