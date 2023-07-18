
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <m-array.h>
#include <toolbox/path.h>
#include <storage/storage.h>
#include <core/common_defines.h>
#include <flipper_format/flipper_format.h>
#include <lib/subghz/protocols/raw.h>
//#include <lib/toolbox/path.h>
//#include <flipper_format/flipper_format_i.h>
#include <lib/toolbox/stream/stream.h>
#include <lib/subghz/protocols/protocol_items.h>
#include <lib/subghz/subghz_worker.h>
#include <lib/subghz/subghz_setting.h>
#include <lib/subghz/receiver.h>
#include <lib/subghz/transmitter.h>
#include <lib/subghz/devices/devices.h>
//#include <lib/subghz/blocks/custom_btn.h>

#include <flipper_format/flipper_format_i.h>

#include "subghz_types.h"
#include "txrx\subghz_txrx.h"

extern const SubGhzProtocolRegistry subghz_protocol_registry;

typedef struct SubGhzTxRx SubGhzTxRx;


