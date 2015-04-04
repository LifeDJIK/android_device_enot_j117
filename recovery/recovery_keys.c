#include <linux/input.h>

#include "recovery_ui.h"
#include "common.h"
#include "extendedcommands.h"

// LifeDJIK: device keys
#define DEVICE_KEY_MENU 59
#define DEVICE_KEY_BACK 114
#define DEVICE_KEY_POWER 116
#define DEVICE_KEY_TOUCH 330

int device_toggle_display(volatile char* key_pressed, int key_code) {
    if (ui_get_showing_back_button()) {
        return 0;
    }
    return get_allow_toggle_display() && (key_code == DEVICE_KEY_POWER || key_code == DEVICE_KEY_MENU || key_code == DEVICE_KEY_BACK);
}

int device_handle_key(int key_code, int visible) {
    if (visible) {
        switch (key_code) {
            case DEVICE_KEY_MENU:
                return HIGHLIGHT_DOWN;

            case DEVICE_KEY_POWER:
                if (ui_get_showing_back_button()) {
                    return SELECT_ITEM;
                }
                if (!get_allow_toggle_display() && !ui_root_menu) {
                    return GO_BACK;
                }
                break;

            case DEVICE_KEY_TOUCH:
                return SELECT_ITEM;

            case DEVICE_KEY_BACK:
                if (!ui_root_menu) {
                    return GO_BACK;
                }
        }
    }

    return NO_ACTION;
}
// LifeDJIK: ---
