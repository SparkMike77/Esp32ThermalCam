"""ESPHome Python bindings for mlx90640_display component."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID,
    CONF_UPDATE_INTERVAL,
    UNIT_CELSIUS,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
)

# ---- Namespace & class ----
mlx90640_ns = cg.esphome_ns.namespace("mlx90640_display")
MLX90640DisplayComponent = mlx90640_ns.class_(
    "MLX90640DisplayComponent", cg.PollingComponent
)

# ---- Config keys ----
CONF_SDA_PIN       = "sda"
CONF_SCL_PIN       = "scl"
CONF_FREQUENCY     = "frequency"
CONF_ADDRESS       = "address"
CONF_MIN_TEMP      = "mintemp"
CONF_MAX_TEMP      = "maxtemp"
CONF_REFRESH_RATE  = "refresh_rate"
CONF_MIN_TEMP_SENSOR  = "min_temperature"
CONF_MAX_TEMP_SENSOR  = "max_temperature"
CONF_MEAN_TEMP_SENSOR = "mean_temperature"

# ---- Schema ----
CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(MLX90640DisplayComponent),
        cv.Required(CONF_SDA_PIN):   cv.int_,
        cv.Required(CONF_SCL_PIN):   cv.int_,
        cv.Optional(CONF_FREQUENCY,  default=400000): cv.int_,
        cv.Optional(CONF_ADDRESS,    default=0x33):   cv.int_,
        cv.Optional(CONF_MIN_TEMP,   default=15.0):   cv.float_,
        cv.Optional(CONF_MAX_TEMP,   default=40.0):   cv.float_,
        cv.Optional(CONF_REFRESH_RATE, default=0x04): cv.int_,
        cv.Optional(CONF_MIN_TEMP_SENSOR):  sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_MAX_TEMP_SENSOR):  sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
        cv.Optional(CONF_MEAN_TEMP_SENSOR): sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            accuracy_decimals=1,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
        ),
    }
).extend(cv.polling_component_schema("500ms"))


async def to_code(config):
    var = cg.new_Pvariable(config[cv.GenerateID()])
    await cg.register_component(var, config)

    cg.add(var.set_sda_pin(config[CONF_SDA_PIN]))
    cg.add(var.set_scl_pin(config[CONF_SCL_PIN]))
    cg.add(var.set_i2c_frequency(config[CONF_FREQUENCY]))
    cg.add(var.set_min_temp(config[CONF_MIN_TEMP]))
    cg.add(var.set_max_temp(config[CONF_MAX_TEMP]))
    cg.add(var.set_refresh_rate(config[CONF_REFRESH_RATE]))

    if CONF_MIN_TEMP_SENSOR in config:
        sens = await sensor.new_sensor(config[CONF_MIN_TEMP_SENSOR])
        cg.add(var.set_min_temperature_sensor(sens))
    if CONF_MAX_TEMP_SENSOR in config:
        sens = await sensor.new_sensor(config[CONF_MAX_TEMP_SENSOR])
        cg.add(var.set_max_temperature_sensor(sens))
    if CONF_MEAN_TEMP_SENSOR in config:
        sens = await sensor.new_sensor(config[CONF_MEAN_TEMP_SENSOR])
        cg.add(var.set_mean_temperature_sensor(sens))

    # Pull in the SparkFun MLX90640 Arduino library
    cg.add_library(
        "SparkFun MLX90640 Arduino Library",
        None,
        "https://github.com/sparkfun/SparkFun_MLX90640_Arduino_Example.git",
    )
