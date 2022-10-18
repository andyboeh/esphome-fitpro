import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import (
    CONF_ID,
    CONF_STATUS,
)

CODEOWNERS = ["@andyboeh"]
DEPENDENCIES = ["fitpro"]

from . import fitpro

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.use_id(fitpro),
            cv.Optional(CONF_STATUS): text_sensor.text_sensor_schema(),
        }
    )
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_ID])

    if CONF_STATUS in config:
        sens = await text_sensor.new_text_sensor(config[CONF_STATUS])
        cg.add(parent.set_status_sensor(sens))

