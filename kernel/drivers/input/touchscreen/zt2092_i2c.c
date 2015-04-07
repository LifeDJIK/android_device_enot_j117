/*
 *  ZT2092 I2C touchscreen driver
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/freezer.h>
#include <linux/kthread.h>
#include <linux/interrupt.h>
#include <linux/earlysuspend.h>

#include <asm/gpio.h>
#include <asm/uaccess.h>

#define ZT2092_I2C_NAME "zt2092_i2c"

typedef struct {
    struct i2c_client *i2c_client;
    struct delayed_work delayed_work;
    struct work_struct work;
    struct workqueue_struct *workqueue;
    struct mutex lock;
    struct input_dev *input_dev;
    struct early_suspend early_suspend;
    s16 x_limit;
    s16 y_limit;
    int last_x[2], last_y[2], last_pressure[2], last_touch;
} zt2092_i2c_instance_t;

static zt2092_i2c_instance_t *zt2092_i2c_instance;

typedef enum {
    ZT2092_I2C_REG_RESET = 0x00,
    ZT2092_I2C_REG_SETUP = 0x01,
    ZT2092_I2C_REG_SEQUENCE = 0x02,
    ZT2092_I2C_REG_STATUS = 0x10,
    ZT2092_I2C_REG_DATA  = 0x11,
    ZT2092_I2C_REG_DATA_X1 = 0x11,
    ZT2092_I2C_REG_DATA_Y1 = 0x13,
    ZT2092_I2C_REG_DATA_Z1 = 0x15,
    ZT2092_I2C_REG_DATA_Z2 = 0x17,
    ZT2092_I2C_REG_DATA_X2 = 0x19,
    ZT2092_I2C_REG_DATA_Y2 = 0x1B,
} zt2092_i2c_reg_t;

typedef enum {
    ZT2092_I2C_MODE_NORMAL = 0x00,
    ZT2092_I2C_MODE_SLEEP1 = 0x10,
    ZT2092_I2C_MODE_SLEEP2 = 0x20,
} zt2092_i2c_mode_t;

typedef enum {
    ZT2092_I2C_SEQM_XYZ = 0x00,
    ZT2092_I2C_SEQM_XY = 0x10,
    ZT2092_I2C_SEQM_X = 0x20,
    ZT2092_I2C_SEQM_Y = 0x30,
    ZT2092_I2C_SEQM_Z = 0x40,
    ZT2092_I2C_SEQM_X2 = 0x50,
    ZT2092_I2C_SEQM_ALL = 0x60,
    ZT2092_I2C_SEQM_Y2 = 0x70,
} zt2092_i2c_seqm_t;

typedef enum {
    ZT2092_I2C_ADC_6_TIMES  = 0x00,
    ZT2092_I2C_ADC_10_TIMES = 0x08,
} zt2092_i2c_adc_t;

typedef enum {
    ZT2092_I2C_INTERVAL_0US = 0x00,
    ZT2092_I2C_INTERVAL_5US = 0x01,
    ZT2092_I2C_INTERVAL_10US = 0x02,
    ZT2092_I2C_INTERVAL_20US = 0x03,
    ZT2092_I2C_INTERVAL_50US = 0x04,
    ZT2092_I2C_INTERVAL_100US = 0x05,
    ZT2092_I2C_INTERVAL_200US = 0x06,
    ZT2092_I2C_INTERVAL_500US = 0x07,
} zt2092_i2c_interval_t;

static int zt2092_i2c_master_send(char *buffer, int size)
{
    struct i2c_msg zt2092_i2c_message[] = {
        {
            .addr = zt2092_i2c_instance->i2c_client->addr,
            .flags = 0,
            .len = size,
            .buf = buffer,
            .scl_rate = 250000,
        },
    };
    return i2c_transfer(zt2092_i2c_instance->i2c_client->adapter,
        zt2092_i2c_message, 1);
}

static int zt2092_i2c_master_recv(zt2092_i2c_reg_t reg, char *buffer, int size)
{
    struct i2c_msg zt2092_i2c_message[] = {
        {
            .addr = (zt2092_i2c_instance->i2c_client->addr * 512) + reg,
            .flags = I2C_M_RD | I2C_M_SPECIAL_REG,
            .len = size,
            .buf = buffer,
            .scl_rate = 250000,
        },
    };
    return i2c_transfer(zt2092_i2c_instance->i2c_client->adapter,
        zt2092_i2c_message, 1);
}

static int zt2092_i2c_write(zt2092_i2c_reg_t reg, int value)
{
    u8 buffer[3];
    int size = 2;
    buffer[0] = reg;
    buffer[1] = value;
    if (reg == ZT2092_I2C_REG_SEQUENCE) {
        size = 3;
        buffer[2] = 0x4;
    }
    return zt2092_i2c_master_send((char *) buffer, size);
}

static int zt2092_i2c_read(zt2092_i2c_reg_t reg, char *buffer, int size)
{
    return zt2092_i2c_master_recv(reg, buffer, size);
}

static s16 zt2092_i2c_noise_filter(s16 *samples)
{
    s16 result = 0;
    s16 minimum, maximum;
    int skip_minimum = 1, skip_maximum = 1, total_samples = 0;
    int counter;
    // Find minimum
    minimum = samples[0];
    for (counter = 1; counter < CONFIG_ZT2092_NOISE_FILTER_SAMPLE_COUNT; counter++) {
        if (samples[counter] < minimum) {
            minimum = samples[counter];
        }
    }
    // Find maximum
    maximum = samples[0];
    for (counter = 1; counter < CONFIG_ZT2092_NOISE_FILTER_SAMPLE_COUNT; counter++) {
        if (samples[counter] > maximum) {
            maximum = samples[counter];
        }
    }
    // Compute average (skipping first minimum and first maximum)
    for (counter = 0; counter < CONFIG_ZT2092_NOISE_FILTER_SAMPLE_COUNT; counter++) {
        if ((skip_minimum) && (samples[counter] == minimum)) {
            skip_minimum = 0;
            continue;
        }
        if ((skip_maximum) && (samples[counter] == maximum)) {
            skip_maximum = 0;
            continue;
        }
        result += samples[counter];
        total_samples++;
    }
    return result / total_samples;
}

static void zt2092_i2c_get_limits(void)
{
    int sample_counter, status_counter;
    s16 x_limit_samples[CONFIG_ZT2092_NOISE_FILTER_SAMPLE_COUNT];
    s16 y_limit_samples[CONFIG_ZT2092_NOISE_FILTER_SAMPLE_COUNT];
    char buffer[2];
    u8 status;

    for (sample_counter = 0; sample_counter < CONFIG_ZT2092_NOISE_FILTER_SAMPLE_COUNT; sample_counter++) {
        zt2092_i2c_write(ZT2092_I2C_REG_SEQUENCE,
            ZT2092_I2C_SEQM_X2 |
            ZT2092_I2C_ADC_10_TIMES |
            ZT2092_I2C_INTERVAL_50US);
        for (status_counter = 0; status_counter < 15; status_counter++) {
            zt2092_i2c_read(ZT2092_I2C_REG_STATUS, (char *) &status, 1);
            if (!status) {
                break;
            }
        }
        zt2092_i2c_read(ZT2092_I2C_REG_DATA, buffer, 2);
        x_limit_samples[sample_counter] = ((((s16) buffer[0]) << 4) | (((s16) buffer[1]) >> 4));
    }

    for (sample_counter = 0; sample_counter < CONFIG_ZT2092_NOISE_FILTER_SAMPLE_COUNT; sample_counter++) {
        zt2092_i2c_write(ZT2092_I2C_REG_SEQUENCE,
            ZT2092_I2C_SEQM_Y2 |
            ZT2092_I2C_ADC_10_TIMES |
            ZT2092_I2C_INTERVAL_50US);
        for (status_counter = 0; status_counter < 15; status_counter++) {
            zt2092_i2c_read(ZT2092_I2C_REG_STATUS, (char *) &status, 1);
            if (!status) {
                break;
            }
        }
        zt2092_i2c_read(ZT2092_I2C_REG_DATA, buffer, 2);
        y_limit_samples[sample_counter] = ((((s16) buffer[0]) << 4) | (((s16) buffer[1]) >> 4));
    }

    zt2092_i2c_instance->x_limit = zt2092_i2c_noise_filter(x_limit_samples);
    zt2092_i2c_instance->y_limit = zt2092_i2c_noise_filter(y_limit_samples);
}

static void zt2092_i2c_initialize(void)
{
    zt2092_i2c_write(ZT2092_I2C_REG_RESET, 0x1);
    zt2092_i2c_write(ZT2092_I2C_REG_SETUP, ZT2092_I2C_MODE_NORMAL);
    zt2092_i2c_get_limits();
}

static int zt2092_i2c_set_irq_state(int enabled)
{
    if (enabled) {
        return zt2092_i2c_write(ZT2092_I2C_REG_SETUP, ZT2092_I2C_MODE_NORMAL);
    } else {
        return zt2092_i2c_write(ZT2092_I2C_REG_SETUP, ZT2092_I2C_MODE_SLEEP1);
    }
}

static int zt2092_i2c_is_pressed(void)
{
    int result = 
        gpio_get_value(irq_to_gpio(zt2092_i2c_instance->i2c_client->irq));
    if (result > 1) {
        result = 0;
    } else {
        result = 1 - result;
    }
    return result;
}

static void zt2092_i2c_delayed_work(struct work_struct *work)
{
    s16 x_samples[CONFIG_ZT2092_NOISE_FILTER_SAMPLE_COUNT];
    s16 y_samples[CONFIG_ZT2092_NOISE_FILTER_SAMPLE_COUNT];
    s16 z1_samples[CONFIG_ZT2092_NOISE_FILTER_SAMPLE_COUNT];
    s16 z2_samples[CONFIG_ZT2092_NOISE_FILTER_SAMPLE_COUNT];
    s16 x2_ref_samples[CONFIG_ZT2092_NOISE_FILTER_SAMPLE_COUNT];
    s16 y2_ref_samples[CONFIG_ZT2092_NOISE_FILTER_SAMPLE_COUNT];
    s16 x, y, z1, z2, x2_ref, y2_ref, raw_pressure, pressure;
    int current_x[2], current_y[2], current_pressure[2];
    s16 number_of_touches;
    int sample_counter, status_counter;
    char buffer[12];
    u8 status;

    for (sample_counter = 0; sample_counter < CONFIG_ZT2092_NOISE_FILTER_SAMPLE_COUNT; sample_counter++) {
        if (!zt2092_i2c_is_pressed()) {
            input_mt_sync(zt2092_i2c_instance->input_dev);
            if (zt2092_i2c_instance->last_touch != 0) {
                input_report_key(zt2092_i2c_instance->input_dev, BTN_TOUCH, 0);
                zt2092_i2c_instance->last_touch = 0;
            }
            input_sync(zt2092_i2c_instance->input_dev);
            enable_irq(zt2092_i2c_instance->i2c_client->irq);
            return;
        }
        zt2092_i2c_write(ZT2092_I2C_REG_SEQUENCE,
            ZT2092_I2C_SEQM_ALL |
            ZT2092_I2C_ADC_10_TIMES |
            ZT2092_I2C_INTERVAL_50US);
        for (status_counter = 0; status_counter < 15; status_counter++) {
            zt2092_i2c_read(ZT2092_I2C_REG_STATUS, (char *) &status, 1);
            if (!status) {
                break;
            }
        }
        zt2092_i2c_read(ZT2092_I2C_REG_DATA, buffer, 12);
        x_samples[sample_counter] = ((((s16) buffer[0]) << 4) | (((s16) buffer[1]) >> 4));
        y_samples[sample_counter] = ((((s16) buffer[2]) << 4) | (((s16) buffer[3]) >> 4));
        z1_samples[sample_counter] = ((((s16) buffer[4]) << 4) | (((s16) buffer[5]) >> 4));
        z2_samples[sample_counter] = ((((s16) buffer[6]) << 4) | (((s16) buffer[7]) >> 4));
        x2_ref_samples[sample_counter] = ((((s16) buffer[8]) << 4) | (((s16) buffer[9]) >> 4));
        y2_ref_samples[sample_counter] = ((((s16) buffer[10]) << 4) | (((s16) buffer[11]) >> 4));
    }

    x = zt2092_i2c_noise_filter(x_samples);
    y = zt2092_i2c_noise_filter(y_samples);
    z1 = zt2092_i2c_noise_filter(z1_samples);
    z2 = zt2092_i2c_noise_filter(z2_samples);
    x2_ref = zt2092_i2c_noise_filter(x2_ref_samples);
    y2_ref = zt2092_i2c_noise_filter(y2_ref_samples);

    raw_pressure = 0xFFFF;
    if ((z1 > 0) && (z2 > z1)) {
        raw_pressure = (x * (z2 - z1)) / (4 * z1);
    }
    pressure = (((raw_pressure - CONFIG_ZT2092_MIN_RAW_PRESSURE) * (CONFIG_ZT2092_MAX_PRESSURE - CONFIG_ZT2092_MIN_PRESSURE)) / (CONFIG_ZT2092_MAX_RAW_PRESSURE - CONFIG_ZT2092_MIN_RAW_PRESSURE)) + CONFIG_ZT2092_MIN_PRESSURE;

    number_of_touches = 1;
    if ((((zt2092_i2c_instance->x_limit - x2_ref) >= CONFIG_ZT2092_MULTITOUCH_THRESHOLD) || ((zt2092_i2c_instance->y_limit - y2_ref) >= CONFIG_ZT2092_MULTITOUCH_THRESHOLD)) && (raw_pressure <= CONFIG_ZT2092_MIN_RAW_PRESSURE)) {
        number_of_touches = 2;
    } else if ((((zt2092_i2c_instance->x_limit - x2_ref) < CONFIG_ZT2092_MULTITOUCH_THRESHOLD) && ((zt2092_i2c_instance->y_limit - y2_ref) < CONFIG_ZT2092_MULTITOUCH_THRESHOLD)) && (raw_pressure <= CONFIG_ZT2092_MAX_RAW_PRESSURE)) {
        number_of_touches = 1;
    }
    if ((raw_pressure == 0) || (raw_pressure == 0xFFFF)) {
        number_of_touches = 0;
    }

    if (number_of_touches == 2) {
        // TODO: double-touch support
        number_of_touches = 0;
    }

    if (number_of_touches == 1) {
#ifndef CONFIG_ZT2092_FLIP_X
        current_x[0] = x;
#else
        current_x[0] = CONFIG_ZT2092_MAX_X - x + CONFIG_ZT2092_MIN_X;
#endif
#ifndef CONFIG_ZT2092_FLIP_Y
        current_y[0] = y;
#else
        current_y[0] = CONFIG_ZT2092_MAX_Y - y + CONFIG_ZT2092_MIN_Y;
#endif
#ifndef CONFIG_ZT2092_FLIP_PRESSURE
        current_pressure[0] = pressure;
#else
        current_pressure[0] = CONFIG_ZT2092_MAX_PRESSURE - pressure + CONFIG_ZT2092_MIN_PRESSURE;
#endif
    }

    if (number_of_touches > 0) {
        for (sample_counter = 0; sample_counter < number_of_touches; sample_counter++) {
            if (((current_x[sample_counter] > CONFIG_ZT2092_MAX_X) && ((current_x[sample_counter] - CONFIG_ZT2092_MAX_X) > CONFIG_ZT2092_NOISE_BLOCK_LIMIT_THRESHOLD)) ||
                ((current_x[sample_counter] < CONFIG_ZT2092_MIN_X) && ((CONFIG_ZT2092_MIN_X - current_x[sample_counter]) > CONFIG_ZT2092_NOISE_BLOCK_LIMIT_THRESHOLD)) ||
                ((current_y[sample_counter] > CONFIG_ZT2092_MAX_Y) && ((current_y[sample_counter] - CONFIG_ZT2092_MAX_Y) > CONFIG_ZT2092_NOISE_BLOCK_LIMIT_THRESHOLD)) ||
                ((current_y[sample_counter] < CONFIG_ZT2092_MIN_Y) && ((CONFIG_ZT2092_MIN_Y - current_y[sample_counter]) > CONFIG_ZT2092_NOISE_BLOCK_LIMIT_THRESHOLD))) {
                number_of_touches = 0;
                break;
            }
            if ((((zt2092_i2c_instance->last_x[sample_counter] >= current_x[sample_counter]) && ((zt2092_i2c_instance->last_x[sample_counter] - current_x[sample_counter]) < CONFIG_ZT2092_NOISE_BLOCK_MIN_DELTA_THRESHOLD)) ||
                ((zt2092_i2c_instance->last_x[sample_counter] < current_x[sample_counter]) && ((current_x[sample_counter] - zt2092_i2c_instance->last_x[sample_counter]) < CONFIG_ZT2092_NOISE_BLOCK_MIN_DELTA_THRESHOLD))) &&
                (((zt2092_i2c_instance->last_y[sample_counter] >= current_y[sample_counter]) && ((zt2092_i2c_instance->last_y[sample_counter] - current_y[sample_counter]) < CONFIG_ZT2092_NOISE_BLOCK_MIN_DELTA_THRESHOLD)) ||
                ((zt2092_i2c_instance->last_y[sample_counter] < current_y[sample_counter]) && ((current_y[sample_counter] - zt2092_i2c_instance->last_y[sample_counter]) < CONFIG_ZT2092_NOISE_BLOCK_MIN_DELTA_THRESHOLD)))) {
                number_of_touches = 0;
                break;
            }
        }
    }

    if (number_of_touches > 0) {
        for (sample_counter = 0; sample_counter < number_of_touches; sample_counter++) {
            input_report_abs(zt2092_i2c_instance->input_dev, ABS_MT_POSITION_X, current_x[sample_counter]);
            zt2092_i2c_instance->last_x[sample_counter] = current_x[sample_counter];
            input_report_abs(zt2092_i2c_instance->input_dev, ABS_MT_POSITION_Y, current_y[sample_counter]);
            zt2092_i2c_instance->last_y[sample_counter] = current_y[sample_counter];
            input_report_abs(zt2092_i2c_instance->input_dev, ABS_MT_PRESSURE, current_pressure[sample_counter]);
            zt2092_i2c_instance->last_pressure[sample_counter] = current_pressure[sample_counter];
            input_mt_sync(zt2092_i2c_instance->input_dev);
        }
        if (zt2092_i2c_instance->last_touch != 1) {
            input_report_key(zt2092_i2c_instance->input_dev, BTN_TOUCH, 1);
            zt2092_i2c_instance->last_touch = 1;
        }
        input_sync(zt2092_i2c_instance->input_dev);
    }

    if (zt2092_i2c_is_pressed()) {
        queue_delayed_work(zt2092_i2c_instance->workqueue,
            &zt2092_i2c_instance->delayed_work, msecs_to_jiffies(10));
    } else {
        input_mt_sync(zt2092_i2c_instance->input_dev);
        if (zt2092_i2c_instance->last_touch != 0) {
            input_report_key(zt2092_i2c_instance->input_dev, BTN_TOUCH, 0);
            zt2092_i2c_instance->last_touch = 0;
        }
        input_sync(zt2092_i2c_instance->input_dev);
        enable_irq(zt2092_i2c_instance->i2c_client->irq);
    }
}

static void zt2092_i2c_work(struct work_struct *work)
{
    if (zt2092_i2c_is_pressed()) {
        queue_delayed_work(zt2092_i2c_instance->workqueue,
            &zt2092_i2c_instance->delayed_work, 0);
    } else {
        cancel_delayed_work_sync(&zt2092_i2c_instance->delayed_work);
        input_mt_sync(zt2092_i2c_instance->input_dev);
        if (zt2092_i2c_instance->last_touch != 0) {
            input_report_key(zt2092_i2c_instance->input_dev, BTN_TOUCH, 0);
            zt2092_i2c_instance->last_touch = 0;
        }
        input_sync(zt2092_i2c_instance->input_dev);
        enable_irq(zt2092_i2c_instance->i2c_client->irq);
    }
}

static irqreturn_t zt2092_i2c_interrupt_handler(int irq, void *dev_id)
{
    if (!work_pending(&zt2092_i2c_instance->work)) {
        disable_irq_nosync(zt2092_i2c_instance->i2c_client->irq);
        queue_work(zt2092_i2c_instance->workqueue, &zt2092_i2c_instance->work);
    }
    return IRQ_HANDLED;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void zt2092_i2c_early_suspend(struct early_suspend *handler)
{
    bool cancelled_work = cancel_work_sync(&zt2092_i2c_instance->work);
    bool cancelled_delayed_work = cancel_delayed_work_sync(&zt2092_i2c_instance->delayed_work);
    if (cancelled_work || cancelled_delayed_work) {
        enable_irq(zt2092_i2c_instance->i2c_client->irq);
    }
    zt2092_i2c_write(ZT2092_I2C_REG_SETUP, ZT2092_I2C_MODE_SLEEP2);
}

static void zt2092_i2c_late_resume(struct early_suspend *handler)
{
    zt2092_i2c_write(ZT2092_I2C_REG_SETUP, ZT2092_I2C_MODE_NORMAL);
}
#endif

static int zt2092_i2c_probe(struct i2c_client *client,
    const struct i2c_device_id *id)
{
    int result;

    zt2092_i2c_instance = NULL;
    zt2092_i2c_instance = kzalloc(sizeof(zt2092_i2c_instance_t), GFP_KERNEL);
    if (!zt2092_i2c_instance) {
        printk("%s: memory allocation failed (%d bytes)\n", __func__,
            sizeof(zt2092_i2c_instance_t));
        return -ENOMEM;
    }
    zt2092_i2c_instance->i2c_client = client;

    INIT_DELAYED_WORK(&zt2092_i2c_instance->delayed_work,
        zt2092_i2c_delayed_work);
    INIT_WORK(&zt2092_i2c_instance->work, zt2092_i2c_work);

    zt2092_i2c_instance->workqueue =
        create_singlethread_workqueue(ZT2092_I2C_NAME);
    if (!zt2092_i2c_instance->workqueue) {
        printk("%s: workqueue creation failed\n", __func__);
        cancel_work_sync(&zt2092_i2c_instance->work);
        cancel_delayed_work_sync(&zt2092_i2c_instance->delayed_work);
        kfree(zt2092_i2c_instance);
        return -ESRCH;
    }

    mutex_init(&zt2092_i2c_instance->lock);

    zt2092_i2c_set_irq_state(0);
    for (result = 0; result < 3; result++) {
        if (!zt2092_i2c_is_pressed()) {
            zt2092_i2c_initialize();
        }
    }
    zt2092_i2c_set_irq_state(1);

    result = request_irq(zt2092_i2c_instance->i2c_client->irq,
        zt2092_i2c_interrupt_handler, IRQF_DISABLED,
        ZT2092_I2C_NAME, zt2092_i2c_instance);
    if (result < 0) {
        printk("%s: IRQ requesting failed\n", __func__);
        mutex_destroy(&zt2092_i2c_instance->lock);
        destroy_workqueue(zt2092_i2c_instance->workqueue);
        cancel_work_sync(&zt2092_i2c_instance->work);
        cancel_delayed_work_sync(&zt2092_i2c_instance->delayed_work);
        kfree(zt2092_i2c_instance);
        return result;
    }
    irq_set_irq_type(zt2092_i2c_instance->i2c_client->irq, IRQ_TYPE_EDGE_FALLING);

    zt2092_i2c_instance->input_dev = input_allocate_device();
    if (!zt2092_i2c_instance->input_dev) {
        printk("%s: input device allocation failed\n", __func__);
        free_irq(zt2092_i2c_instance->i2c_client->irq, zt2092_i2c_instance);
        mutex_destroy(&zt2092_i2c_instance->lock);
        destroy_workqueue(zt2092_i2c_instance->workqueue);
        cancel_work_sync(&zt2092_i2c_instance->work);
        cancel_delayed_work_sync(&zt2092_i2c_instance->delayed_work);
        kfree(zt2092_i2c_instance);
        return -ENOMEM;
    }

    set_bit(EV_ABS, zt2092_i2c_instance->input_dev->evbit);
    set_bit(ABS_MT_POSITION_X, zt2092_i2c_instance->input_dev->absbit);
    set_bit(ABS_MT_POSITION_Y, zt2092_i2c_instance->input_dev->absbit);
    set_bit(ABS_MT_PRESSURE, zt2092_i2c_instance->input_dev->absbit);
    set_bit(EV_KEY, zt2092_i2c_instance->input_dev->evbit);
    set_bit(BTN_TOUCH, zt2092_i2c_instance->input_dev->keybit);
    set_bit(EV_SYN, zt2092_i2c_instance->input_dev->evbit);

    input_set_abs_params(zt2092_i2c_instance->input_dev, ABS_MT_POSITION_X,
        CONFIG_ZT2092_MIN_X, CONFIG_ZT2092_MAX_X, 0, 0);
    input_set_abs_params(zt2092_i2c_instance->input_dev, ABS_MT_POSITION_Y,
        CONFIG_ZT2092_MIN_Y, CONFIG_ZT2092_MAX_Y, 0, 0);
    input_set_abs_params(zt2092_i2c_instance->input_dev, ABS_MT_PRESSURE,
        CONFIG_ZT2092_MIN_PRESSURE, CONFIG_ZT2092_MAX_PRESSURE, 0, 0);

    zt2092_i2c_instance->input_dev->name = ZT2092_I2C_NAME;
    zt2092_i2c_instance->input_dev->id.bustype = BUS_I2C;

    result = input_register_device(zt2092_i2c_instance->input_dev);
    if (result) {
        printk("%s: input device registration failed\n", __func__);
        input_free_device(zt2092_i2c_instance->input_dev);
        free_irq(zt2092_i2c_instance->i2c_client->irq, zt2092_i2c_instance);
        mutex_destroy(&zt2092_i2c_instance->lock);
        destroy_workqueue(zt2092_i2c_instance->workqueue);
        cancel_work_sync(&zt2092_i2c_instance->work);
        cancel_delayed_work_sync(&zt2092_i2c_instance->delayed_work);
        kfree(zt2092_i2c_instance);
        return result;
    }

#ifdef CONFIG_HAS_EARLYSUSPEND
    zt2092_i2c_instance->early_suspend.level =
        EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
    zt2092_i2c_instance->early_suspend.resume = zt2092_i2c_late_resume;
    zt2092_i2c_instance->early_suspend.suspend = zt2092_i2c_early_suspend;
    register_early_suspend(&zt2092_i2c_instance->early_suspend);
#endif

    return 0;
}

static int __devexit zt2092_i2c_remove(struct i2c_client *client)
{
    printk("%s: called\n", __func__);
    return 0; // TODO: Do you really want to remove touchscreen driver?! :-)
}

/*
 * Kernel module inlet
 */

static const struct i2c_device_id zt2092_i2c_id_table[] = {
    { ZT2092_I2C_NAME, 0 }, {}
};

static struct i2c_driver zt2092_i2c_driver = {
    .driver = {
        .name = ZT2092_I2C_NAME,
        .owner = THIS_MODULE,
    },
    .id_table = zt2092_i2c_id_table,
    .probe  = zt2092_i2c_probe,
    .remove = __devexit_p(zt2092_i2c_remove),
};

static int __init zt2092_i2c_init(void)
{
    return i2c_add_driver(&zt2092_i2c_driver);
}

static void __exit zt2092_i2c_exit(void)
{
    i2c_del_driver(&zt2092_i2c_driver);
}

module_init(zt2092_i2c_init);
module_exit(zt2092_i2c_exit);

MODULE_AUTHOR("");
MODULE_DESCRIPTION("ZT2092 I2C touchscreen driver");
MODULE_DEVICE_TABLE(i2c, zt2092_i2c_id);
MODULE_LICENSE("GPL");
