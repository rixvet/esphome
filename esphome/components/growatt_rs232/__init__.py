import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import (
    CONF_ID,
    CONF_UART_ID,
    CONF_RECEIVE_TIMEOUT,
)

CODEOWNERS = ["@rixvet"]

DEPENDENCIES = ["uart"]
AUTO_LOAD = ["sensor"]

CONF_REQUEST_INTERVAL = "request_interval"

# Hack to prevent compile error due to ambiguity with lib namespace
growatt_rs232_ns = cg.esphome_ns.namespace("esphome::growatt_rs232")
Growatt_rs232 = growatt_rs232_ns.class_("Growatt_rs232", cg.Component, uart.UARTDevice)


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(Growatt_rs232),
            cv.Optional(
                CONF_REQUEST_INTERVAL, default="10000ms"
            ): cv.positive_time_period_milliseconds,
            cv.Optional(
                CONF_RECEIVE_TIMEOUT, default="50000ms"
            ): cv.positive_time_period_milliseconds,
        }
    ).extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    uart_component = await cg.get_variable(config[CONF_UART_ID])
    var = cg.new_Pvariable(config[CONF_ID], uart_component)
    await cg.register_component(var, config)

    cg.add(var.set_request_interval(config[CONF_REQUEST_INTERVAL].total_milliseconds))
    cg.add(var.set_receive_timeout(config[CONF_RECEIVE_TIMEOUT].total_milliseconds))
