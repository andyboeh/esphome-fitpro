
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import ble_client, time
from esphome.const import (
    CONF_ID,
    CONF_TIME_ID,
)

CODEOWNERS = ["@andyboeh"]
DEPENDENCIES = ["ble_client", "time", "sensor", "text_sensor", "button"]

MULTI_CONF = True

CONF_FITPRO_ID = "fitpro_id"

fitpro_ns = cg.esphome_ns.namespace("fitpro")
fitpro = fitpro_ns.class_("fitpro", ble_client.BLEClientNode, cg.Component)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(fitpro), 
            cv.GenerateID(CONF_TIME_ID): cv.use_id(time.RealTimeClock),           
        }
    )
    .extend(ble_client.BLE_CLIENT_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    time_ = await cg.get_variable(config[CONF_TIME_ID])
    cg.add(var.set_time(time_))
    await cg.register_component(var, config)
    await ble_client.register_ble_node(var, config)

