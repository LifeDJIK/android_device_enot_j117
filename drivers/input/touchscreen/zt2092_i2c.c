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
    int pending_interrupt;
    s16 x_limit;
    s16 y_limit;
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

static void zt2092_i2c_get_limits(void)
{
    s16 min_x = 0, min_y = 0, max_x = 0, max_y = 0;
    u8 buffer[2];
    zt2092_i2c_write(ZT2092_I2C_REG_SEQUENCE,
        ZT2092_I2C_SEQM_X2 |
        ZT2092_I2C_ADC_10_TIMES |
        ZT2092_I2C_INTERVAL_500US);
    msleep(1);
    memset(buffer, 0x0, 2);
    zt2092_i2c_read(ZT2092_I2C_REG_DATA, buffer, 2);
    max_x = ((((s16) buffer[0]) << 4) | (((s16) buffer[1]) >> 4));
    printk("%s: max_x = %d (%#x)\n", __func__, max_x, max_x);
    zt2092_i2c_write(ZT2092_I2C_REG_SEQUENCE,
        ZT2092_I2C_SEQM_Y2 |
        ZT2092_I2C_ADC_10_TIMES |
        ZT2092_I2C_INTERVAL_500US);
    msleep(1);
    memset(buffer, 0x0, 2);
    zt2092_i2c_read(ZT2092_I2C_REG_DATA, buffer, 2);
    max_y = ((((s16) buffer[0]) << 4) | (((s16) buffer[1]) >> 4));
    printk("%s: max_y = %d (%#x)\n", __func__, max_y, max_y);
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

#ifdef CONFIG_HAS_EARLYSUSPEND
static void zt2092_i2c_early_suspend(struct early_suspend *handler)
{
    printk("%s: called\n", __func__);
    flush_scheduled_work();
    if (zt2092_i2c_instance->input_dev->users) {
        cancel_delayed_work_sync(&zt2092_i2c_instance->delayed_work);
    }
    zt2092_i2c_write(ZT2092_I2C_REG_SETUP, ZT2092_I2C_MODE_SLEEP2); // rewrite?
}

static void zt2092_i2c_late_resume(struct early_suspend *handler)
{
    printk("%s: called\n", __func__);
    zt2092_i2c_write(ZT2092_I2C_REG_SETUP, ZT2092_I2C_MODE_NORMAL); // rewrite?
}
#endif

static void zt2092_i2c_delayed_work(struct work_struct *work)
{
    int counter;
    char buffer[12];
    s16 x, y, z1, z2, pressure;
    s16 x2_ref, y2_ref;
    //~ printk("%s: called\n", __func__);
    zt2092_i2c_write(ZT2092_I2C_REG_SEQUENCE,
        ZT2092_I2C_SEQM_ALL |
        ZT2092_I2C_ADC_10_TIMES |
        ZT2092_I2C_INTERVAL_500US);
    for (counter = 0; counter < 14; counter++) {
        u8 status = 0;
        zt2092_i2c_read(ZT2092_I2C_REG_STATUS, (char *) &status, 1);
        if (!status) {
            break;
        }
    }
    memset(buffer, 0x0, 12);
    zt2092_i2c_read(ZT2092_I2C_REG_DATA, buffer, 12);

    x = (((s16) buffer[0]) << 4 | ((s16) buffer[1]) >> 4);
    y = (((s16) buffer[2]) << 4 | ((s16) buffer[3]) >> 4);
    z1 = (((s16) buffer[4]) << 4 | ((s16) buffer[5]) >> 4);
    z2 = (((s16) buffer[6]) << 4 | ((s16) buffer[7]) >> 4);
    x2_ref = (((s16) buffer[8]) << 4 | ((s16) buffer[9]) >> 4);
    y2_ref = (((s16) buffer[10]) << 4 | ((s16) buffer[11]) >> 4);

    if ((z1 > 0) && (z2 > z1)) {
        /* Rx*x/4096*(z2/z1 - 1), assume Rx=1024 */
        pressure = ((x)*(z2-z1)/z1) >> 2;
    } else {
        pressure = 0x7FFF;
    }
    
    //~ printk("%s: x = %d, y = %d, z1 = %d, z2 = %d, x2_ref = %d, y2_ref = %d, pressure = %d\n", __func__, x, y, z1, z2, x2_ref, y2_ref, pressure);
    
    //~ const int CONFIG_ZT2092_MIN_X = 60;
    //~ const int CONFIG_ZT2092_MAX_X = 3950;
    //~ const int CONFIG_ZT2092_MIN_Y = 160;
    //~ const int CONFIG_ZT2092_MAX_Y = 3850;
    const int min_pressure = 100;
    const int max_pressure = 3500;
    const int invalid_threshold = 5000;
    
    if (pressure > invalid_threshold) { return; }
    
#ifndef CONFIG_ZT2092_FLIP_X
    int current_x = x;
#else
    int current_x = CONFIG_ZT2092_MAX_X - x + CONFIG_ZT2092_MIN_X;
#endif
    //~ if (current_x < CONFIG_ZT2092_MIN_X) { current_x = CONFIG_ZT2092_MIN_X; }
    //~ if (current_x > CONFIG_ZT2092_MAX_X) { current_x = CONFIG_ZT2092_MAX_X; }
    //~ current_x = (CONFIG_ZT2092_MAX_X - current_x) * SCREEN_CONFIG_ZT2092_MAX_X / (CONFIG_ZT2092_MAX_Y - CONFIG_ZT2092_MIN_X);
    //~ if (current_x > SCREEN_CONFIG_ZT2092_MAX_X) { current_x = SCREEN_CONFIG_ZT2092_MAX_X; }

#ifndef CONFIG_ZT2092_FLIP_Y
    int current_y = y;
#else
    int current_y = CONFIG_ZT2092_MAX_Y - y + CONFIG_ZT2092_MIN_Y;
#endif
    //~ if (current_y < CONFIG_ZT2092_MIN_Y) { current_y = CONFIG_ZT2092_MIN_Y; }
    //~ if (current_y > CONFIG_ZT2092_MAX_Y) { current_y = CONFIG_ZT2092_MAX_Y; }
    //~ current_y = (CONFIG_ZT2092_MAX_Y - current_y) * SCREEN_CONFIG_ZT2092_MAX_Y / (CONFIG_ZT2092_MAX_Y - CONFIG_ZT2092_MIN_Y);
    //~ if (current_y > SCREEN_CONFIG_ZT2092_MAX_Y) { current_y = SCREEN_CONFIG_ZT2092_MAX_Y; }
    
    int current_pressure = pressure;
    //~ if (current_pressure < min_pressure) { current_pressure = min_pressure; }
    //~ if (current_pressure > max_pressure) { current_pressure = max_pressure; }
    //~ current_pressure = (max_pressure - current_pressure) * SCREEN_MAX_Z / (max_pressure - min_pressure);
    //~ if (current_pressure > SCREEN_MAX_Z) { current_pressure = SCREEN_MAX_Z; }

    //~ printk("%s: current_x = %d, current_y = %d, current_pressure = %d\n", __func__, current_x, current_y, current_pressure);

    if (zt2092_i2c_is_pressed()) {
        input_report_abs(zt2092_i2c_instance->input_dev, ABS_X, current_x);
        input_report_abs(zt2092_i2c_instance->input_dev, ABS_Y, current_y);
        //~ input_report_abs(zt2092_i2c_instance->input_dev, ABS_PRESSURE, current_pressure);
        input_report_key(zt2092_i2c_instance->input_dev, BTN_TOUCH, 1);
        input_sync(zt2092_i2c_instance->input_dev);
        queue_delayed_work(zt2092_i2c_instance->workqueue,
            &zt2092_i2c_instance->delayed_work, 1);
    } else {
        //~ input_report_abs(zt2092_i2c_instance->input_dev, ABS_PRESSURE, 0);
        input_report_key(zt2092_i2c_instance->input_dev, BTN_TOUCH, 0);
        input_sync(zt2092_i2c_instance->input_dev);
    }
}

static void zt2092_i2c_work(struct work_struct *work)
{
    //~ printk("%s: called\n", __func__);
    if (zt2092_i2c_instance->pending_interrupt) {
        zt2092_i2c_instance->pending_interrupt = 0;
        //enable_irq(zt2092_i2c_instance->i2c_client->irq);
    }
    if (zt2092_i2c_is_pressed()) {
        queue_delayed_work(zt2092_i2c_instance->workqueue,
            &zt2092_i2c_instance->delayed_work, 0);
        irq_set_irq_type(zt2092_i2c_instance->i2c_client->irq,
            IRQ_TYPE_EDGE_RISING);
    } else {
        cancel_delayed_work_sync(&zt2092_i2c_instance->delayed_work);
        input_report_abs(zt2092_i2c_instance->input_dev, ABS_PRESSURE, 0);
        input_report_key(zt2092_i2c_instance->input_dev, BTN_TOUCH, 0);
        input_sync(zt2092_i2c_instance->input_dev);
        //~ zt2092_ts_release();
        //~ //printk("xylimits: %d %d\n", x_limits, y_limits);
        //~ counter = 0;
//~ 
//~ #if defined(CONFIG_ZT2092_GESTURE)
        //~ zt2092_get_gesture();
        //~ data_struct_flush();
//~ #endif
//~ 
        //~ /*
         //~ * calculate xylimits, interrupt pin may change its state,
         //~ * therefore interrupt needs to be disabled
         //~ */
        //~ disable_irq(this_client->irq); /* __gpio_mask_irq(pdata->intr); */
        //~ zt2092_startup();
        //~ Filter_Coord_Struct_Flush();
        //~ Temp_Buffer_Struct_Flush();
        irq_set_irq_type(zt2092_i2c_instance->i2c_client->irq,
            IRQ_TYPE_EDGE_FALLING);
        //~ enable_irq(this_client->irq); /* __gpio_unmask_irq(pdata->intr); */
    }    
}

static irqreturn_t zt2092_i2c_interrupt_handler(int irq, void *dev_id)
{
    //~ printk("%s: called\n", __func__);
    //disable_irq(zt2092_i2c_instance->i2c_client->irq);
    if (!work_pending(&zt2092_i2c_instance->work)) {
        zt2092_i2c_instance->pending_interrupt = 1;
        queue_work(zt2092_i2c_instance->workqueue, &zt2092_i2c_instance->work);
    }
    return IRQ_HANDLED;
}

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

#ifdef CONFIG_ZT2092_MULTITOUCH
    //~ set_bit(ABS_MT_POSITION_X, zt2092_i2c_instance->input_dev->absbit);
    //~ input_set_abs_params(zt2092_i2c_instance->input_dev,
        //~ ABS_MT_POSITION_X, 0, SCREEN_CONFIG_ZT2092_MAX_X + 1, 0, 0);
    //~ set_bit(ABS_MT_POSITION_Y, zt2092_i2c_instance->input_dev->absbit);
    //~ input_set_abs_params(zt2092_i2c_instance->input_dev,
        //~ ABS_MT_POSITION_Y, 0, SCREEN_CONFIG_ZT2092_MAX_Y + 1, 0, 0);
    //~ set_bit(ABS_MT_TOUCH_MAJOR, zt2092_i2c_instance->input_dev->absbit);
    //~ input_set_abs_params(zt2092_i2c_instance->input_dev,
        //~ ABS_MT_TOUCH_MAJOR, 0, SCREEN_MAX_Z + 1, 0, 0);
    //~ set_bit(ABS_MT_WIDTH_MAJOR, zt2092_i2c_instance->input_dev->absbit);
    //~ input_set_abs_params(zt2092_i2c_instance->input_dev,
        //~ ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
#else
    set_bit(ABS_X, zt2092_i2c_instance->input_dev->absbit);
    input_set_abs_params(zt2092_i2c_instance->input_dev,
        ABS_X, CONFIG_ZT2092_MIN_X, CONFIG_ZT2092_MAX_X, 0, 0);
    set_bit(ABS_Y, zt2092_i2c_instance->input_dev->absbit);
    input_set_abs_params(zt2092_i2c_instance->input_dev,
        ABS_Y, CONFIG_ZT2092_MIN_Y, CONFIG_ZT2092_MAX_Y, 0, 0);
    //~ set_bit(ABS_PRESSURE, zt2092_i2c_instance->input_dev->absbit);
    //~ input_set_abs_params(zt2092_i2c_instance->input_dev,
        //~ ABS_PRESSURE, 0, SCREEN_MAX_Z + 1, 0, 0);
    set_bit(BTN_TOUCH, zt2092_i2c_instance->input_dev->keybit);
#endif
    set_bit(EV_ABS, zt2092_i2c_instance->input_dev->evbit);
    set_bit(EV_KEY, zt2092_i2c_instance->input_dev->evbit);
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
    //~ printk("%s: called\n", __func__);
    return 0; // Do you really want to remove touchscreen driver?! :-)
}

static int zt2092_i2c_resume(struct i2c_client *client)
{
    //~ printk("%s: called\n", __func__);
    return 0;
}

static int zt2092_i2c_suspend(struct i2c_client *client, pm_message_t mesg)
{
    //~ printk("%s: called\n", __func__);
    return 0;
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
    .resume = zt2092_i2c_resume,
    .suspend = zt2092_i2c_suspend,
};

static int __init zt2092_i2c_init(void)
{
    //~ printk("%s: called\n", __func__);
    return i2c_add_driver(&zt2092_i2c_driver);
}

static void __exit zt2092_i2c_exit(void)
{
    //~ printk("%s: called\n", __func__);
    i2c_del_driver(&zt2092_i2c_driver);
}

module_init(zt2092_i2c_init);
module_exit(zt2092_i2c_exit);

MODULE_AUTHOR("LifeDJIK");
MODULE_DESCRIPTION("ZT2092 I2C touchscreen driver");
MODULE_DEVICE_TABLE(i2c, zt2092_i2c_id);
MODULE_LICENSE("GPL");
