#include "subghz_txrx_i.h"

#include <lib/subghz/protocols/protocol_items.h>
#include <applications/drivers/subghz/cc1101_ext/cc1101_ext_interconnect.h>
#include <lib/subghz/devices/cc1101_int/cc1101_int_interconnect.h>

#define TAG "SubGhz"

static void subghz_txrx_radio_device_power_on(SubGhzTxRx* instance) {
    UNUSED(instance);
    uint8_t attempts = 5;
    while(--attempts > 0) {
        if(furi_hal_power_enable_otg()) break;
    }
    if(attempts == 0) {
        if(furi_hal_power_get_usb_voltage() < 4.5f) {
            FURI_LOG_E(
                TAG,
                "Error power otg enable. BQ2589 check otg fault = %d",
                furi_hal_power_check_otg_fault() ? 1 : 0);
        }
    }
}

static void subghz_txrx_radio_device_power_off(SubGhzTxRx* instance) {
    UNUSED(instance);
    if(furi_hal_power_is_otg_enabled()) furi_hal_power_disable_otg();
}


SubGhzTxRx* subghz_txrx_alloc() {
    SubGhzTxRx* instance = malloc(sizeof(SubGhzTxRx));
    instance->setting = subghz_setting_alloc();
    subghz_setting_load(instance->setting, EXT_PATH("subghz/assets/setting_user"));

    instance->preset = malloc(sizeof(SubGhzRadioPreset));
    instance->preset->name = furi_string_alloc();
    subghz_txrx_set_preset(
        instance, "AM650", subghz_setting_get_default_frequency(instance->setting), NULL, 0);

    instance->txrx_state = SubGhzTxRxStateSleep;

    subghz_txrx_hopper_set_state(instance, SubGhzHopperStateOFF);
    subghz_txrx_speaker_set_state(instance, SubGhzSpeakerStateDisable);

    instance->worker = subghz_worker_alloc();
    instance->fff_data = flipper_format_string_alloc();

    instance->environment = subghz_environment_alloc();
    instance->is_database_loaded =
        subghz_environment_load_keystore(instance->environment, SUBGHZ_KEYSTORE_DIR_NAME);
    subghz_environment_load_keystore(instance->environment, SUBGHZ_KEYSTORE_DIR_USER_NAME);
    subghz_environment_set_came_atomo_rainbow_table_file_name(
        instance->environment, SUBGHZ_CAME_ATOMO_DIR_NAME);
    subghz_environment_set_alutech_at_4n_rainbow_table_file_name(
        instance->environment, SUBGHZ_ALUTECH_AT_4N_DIR_NAME);
    subghz_environment_set_nice_flor_s_rainbow_table_file_name(
        instance->environment, SUBGHZ_NICE_FLOR_S_DIR_NAME);
    subghz_environment_set_protocol_registry(
        instance->environment, (void*)&subghz_protocol_registry);
    instance->receiver = subghz_receiver_alloc_init(instance->environment);

    subghz_worker_set_overrun_callback(
        instance->worker, (SubGhzWorkerOverrunCallback)subghz_receiver_reset);
    subghz_worker_set_pair_callback(
        instance->worker, (SubGhzWorkerPairCallback)subghz_receiver_decode);
    subghz_worker_set_context(instance->worker, instance->receiver);

    //set default device External
    subghz_devices_init();
    instance->radio_device_type = SubGhzRadioDeviceTypeInternal;
    instance->radio_device_type =
        subghz_txrx_radio_device_set(instance, SubGhzRadioDeviceTypeExternalCC1101);

    return instance;
}

void subghz_txrx_free(SubGhzTxRx* instance) {
    furi_assert(instance);

    if(instance->radio_device_type != SubGhzRadioDeviceTypeInternal) {
        subghz_txrx_radio_device_power_off(instance);
        subghz_devices_end(instance->radio_device);
    }

    subghz_devices_deinit();

    subghz_worker_free(instance->worker);
    subghz_receiver_free(instance->receiver);
    subghz_environment_free(instance->environment);
    flipper_format_free(instance->fff_data);
    furi_string_free(instance->preset->name);
    subghz_setting_free(instance->setting);

    free(instance->preset);
    free(instance);
}

FlipperFormat* subghz_txrx_get_fff_data(SubGhzTxRx* instance) {
    furi_assert(instance);
    return instance->fff_data;
}

void subghz_txrx_set_preset(
    SubGhzTxRx* instance,
    const char* preset_name,
    uint32_t frequency,
    uint8_t* preset_data,
    size_t preset_data_size) {
    furi_assert(instance);
    furi_string_set(instance->preset->name, preset_name);
    SubGhzRadioPreset* preset = instance->preset;
    preset->frequency = frequency;
    preset->data = preset_data;
    preset->data_size = preset_data_size;
}

const char* subghz_txrx_radio_device_get_name(SubGhzTxRx* instance) {
    furi_assert(instance);
    return subghz_devices_get_name(instance->radio_device);
}

void subghz_txrx_hopper_set_state(SubGhzTxRx* instance, SubGhzHopperState state) {
    furi_assert(instance);
    instance->hopper_state = state;
}

void subghz_txrx_speaker_set_state(SubGhzTxRx* instance, SubGhzSpeakerState state) {
    furi_assert(instance);
    instance->speaker_state = state;
}

bool subghz_txrx_radio_device_is_external_connected(SubGhzTxRx* instance, const char* name) {
    furi_assert(instance);

    bool is_connect = false;
    bool is_otg_enabled = furi_hal_power_is_otg_enabled();

    if(!is_otg_enabled) {
        subghz_txrx_radio_device_power_on(instance);
    }

    const SubGhzDevice* device = subghz_devices_get_by_name(name);
    if(device) {
        is_connect = subghz_devices_is_connect(device);
    }

    if(!is_otg_enabled) {
        subghz_txrx_radio_device_power_off(instance);
    }
    return is_connect;
}

SubGhzRadioDeviceType
    subghz_txrx_radio_device_set(SubGhzTxRx* instance, SubGhzRadioDeviceType radio_device_type) {
    furi_assert(instance);

    if(radio_device_type == SubGhzRadioDeviceTypeExternalCC1101 &&
       subghz_txrx_radio_device_is_external_connected(instance, SUBGHZ_DEVICE_CC1101_EXT_NAME)) {
        subghz_txrx_radio_device_power_on(instance);
        instance->radio_device = subghz_devices_get_by_name(SUBGHZ_DEVICE_CC1101_EXT_NAME);
        subghz_devices_begin(instance->radio_device);
        instance->radio_device_type = SubGhzRadioDeviceTypeExternalCC1101;
    } else {
        subghz_txrx_radio_device_power_off(instance);
        if(instance->radio_device_type != SubGhzRadioDeviceTypeInternal) {
            subghz_devices_end(instance->radio_device);
        }
        instance->radio_device = subghz_devices_get_by_name(SUBGHZ_DEVICE_CC1101_INT_NAME);
        instance->radio_device_type = SubGhzRadioDeviceTypeInternal;
    }

    return instance->radio_device_type;
}
