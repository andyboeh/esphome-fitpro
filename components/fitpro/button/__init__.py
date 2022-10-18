import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button

from esphome.const import CONF_ID
from .. import fitpro_ns, fitpro, CONF_FITPRO_ID

DEPENDENCIES = ["fitpro", "button"]
CODEOWNERS = ["@andyboeh"]

FitProButton = fitpro_ns.class_("FitProButton", button.Button, cg.Component)

CONFIG_SCHEMA = button.BUTTON_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(FitProButton),
        cv.GenerateID(CONF_FITPRO_ID): cv.use_id(fitpro),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await button.register_button(var, config)

    parent = await cg.get_variable(config[CONF_FITPRO_ID])
    cg.add(var.set_fitpro_parent(parent))
