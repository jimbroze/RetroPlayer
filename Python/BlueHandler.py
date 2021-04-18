#!/usr/bin/env python3

"""Copyright (c) 2015, Douglas Otwell
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
"""

import dbus
import dbus.service
import dbus.mainloop.glib

import logging
import json
import time
from functools import partial
import asyncio

SERVICE_NAME = "org.bluez"
AGENT_IFACE = SERVICE_NAME + ".Agent1"
ADAPTER_IFACE = SERVICE_NAME + ".Adapter1"
DEVICE_IFACE = SERVICE_NAME + ".Device1"
PLAYER_IFACE = SERVICE_NAME + ".MediaPlayer1"
TRANSPORT_IFACE = SERVICE_NAME + ".MediaTransport1"
IFACE = "org.freedesktop.DBus"
MANAGER_IFACE = IFACE + ".ObjectManager"
PROP_IFACE = IFACE + ".Properties"


async def event_setter(connectingEvent):
    connectingEvent.set()


"""Utility functions from bluezutils.py"""


def get_managed_objects():
    bus = dbus.SystemBus()
    manager = dbus.Interface(bus.get_object(SERVICE_NAME, "/"), MANAGER_IFACE)
    return manager.GetManagedObjects()


def find_adapter():
    objects = get_managed_objects()
    bus = dbus.SystemBus()
    for path, ifaces in objects.items():
        adapter = ifaces.get(ADAPTER_IFACE)
        if adapter is None:
            continue
        obj = bus.get_object(SERVICE_NAME, path)
        return dbus.Interface(obj, ADAPTER_IFACE)
    raise Exception("Bluetooth adapter not found")


def dbus_decode(data):
    """
        convert dbus data types to python native data types
    """
    if isinstance(data, dbus.String):
        data = str(data)
    elif isinstance(data, dbus.Boolean):
        data = bool(data)
    elif (
        isinstance(data, dbus.UInt32)
        or isinstance(data, dbus.UInt64)
        or isinstance(data, dbus.UInt16)
        or isinstance(data, dbus.Int32)
        or isinstance(data, dbus.Int64)
        or isinstance(data, dbus.UInt16)
    ):
        data = int(data)
    elif isinstance(data, dbus.Double):
        data = float(data)
    elif isinstance(data, dbus.Array):
        data = [dbus_decode(value) for value in data]
    elif isinstance(data, dbus.Dictionary):
        new_data = dict()
        for key in data.keys():
            new_data[dbus_decode(key)] = dbus_decode(data[key])
        data = new_data
    return data


class BlueHandler(dbus.service.Object):
    """A class that handles media player bluetooth operations. Takes dbus object,
    media player object and bluetooth io capability as arguments."""

    AGENT_PATH = "/RetroPlayer/agent"
    #    CAPABILITY = "DisplayOnly"
    capability = None

    bus = None
    adapter = None
    device = None
    deviceAlias = None
    player = None
    transport = None
    connected = None
    state = None
    status = None
    discoverable = None
    discoverTimeout = None
    track = None
    position = None
    mainLoop = None
    connecting = False
    # connectingEvent = asyncio.Event()

    def __init__(
        self,
        bus,
        loop,
        updatePlayer,
        capability="NoInputNoOutput",
        discoverTimeout="180",
    ):
        """Initialize gobject and find any current media players"""
        self.capability = capability
        self.update_player = updatePlayer
        self.bus = bus
        self.discoverTimeout = discoverTimeout
        self.mainLoop = loop

        # asyncio.ensure_future(self.setup_async(), loop=self.mainLoop)

        dbus.service.Object.__init__(self, bus, BlueHandler.AGENT_PATH)

        # self.bus.add_signal_receiver(
        #     self.interfaces_added,
        #     dbus_interface=MANAGER_IFACE,
        #     signal_name="InterfacesAdded",
        # )

        self.bus.add_signal_receiver(
            self.signal_handler,
            bus_name=SERVICE_NAME,
            dbus_interface=PROP_IFACE,
            signal_name="PropertiesChanged",
            path_keyword="path",
        )

        self.register_agent()
        self.adapter = find_adapter()
        self.find_player()

        # self.adapter.StartDiscovery()
        # time.sleep(5)
        # objects = get_managed_objects()

        # print(json.dumps(objects, indent=4))

        if self.device == None:
            asyncio.ensure_future(self.connect_devices(), loop=self.mainLoop)

    def register_agent(self):
        """Register BlueHandler as the default agent"""
        manager = dbus.Interface(
            self.bus.get_object(SERVICE_NAME, "/org/bluez"), "org.bluez.AgentManager1"
        )
        manager.RegisterAgent(BlueHandler.AGENT_PATH, self.capability)
        manager.RequestDefaultAgent(BlueHandler.AGENT_PATH)
        logging.debug("BlueHandler is registered as the default agent")

    def find_player(self):
        """Find any current media players and associated device"""

        objects = get_managed_objects()

        # print(json.dumps(objects, indent=4))

        player_path = None
        transport_path = None
        for path, interfaces in objects.items():
            if PLAYER_IFACE in interfaces:
                player_path = path
            if TRANSPORT_IFACE in interfaces:
                transport_path = path

        if player_path:
            logging.debug(f"Found player on path [{player_path}]")
            self.connected = True
            self.get_player(player_path)
            player_properties = self.player.GetAll(
                PLAYER_IFACE, dbus_interface=PROP_IFACE
            )
            if "Status" in player_properties:
                self.status = player_properties["Status"]
                self.update_player("Status", dbus_decode(self.status))
            if "Track" in player_properties:
                self.track = player_properties["Track"]
                self.update_player("Track", dbus_decode(self.track))
        else:
            logging.debug("Could not find player")
            self.player = None

        if transport_path:
            logging.debug(f"Found transport on path [{transport_path}]")
            self.transport = self.bus.get_object(SERVICE_NAME, transport_path)
            logging.debug(f"Transport [{transport_path}] has been set")
            transport_properties = self.transport.GetAll(
                TRANSPORT_IFACE, dbus_interface=PROP_IFACE
            )
            if "State" in transport_properties:
                self.state = transport_properties["State"]

    def reply_handler(self, devicePath, connectingEvent):
        logging.info(f"successfully connected to device: {devicePath}")
        self.connected = True
        # remove_pending_connection(device_path)
        asyncio.run_coroutine_threadsafe(event_setter(connectingEvent), self.mainLoop)

    def error_handler(self, e, devicePath, connectingEvent):
        self.connected = False
        logging.warning(
            f"Error connecting to device: {devicePath}. Error: {e.get_dbus_message()}"
        )
        asyncio.run_coroutine_threadsafe(event_setter(connectingEvent), self.mainLoop)

    async def connect_devices(self):
        self.connecting = True
        connectingEvent = asyncio.Event()
        objects = get_managed_objects()
        for path, ifaces in objects.items():
            connectingEvent.clear()
            deviceData = ifaces.get(DEVICE_IFACE)
            if deviceData is None:
                continue
            device = dbus.Interface(
                self.bus.get_object(SERVICE_NAME, path), DEVICE_IFACE
            )
            logging.debug(f"trying to connect to {path}")
            device.Connect(
                reply_handler=partial(
                    self.reply_handler, devicePath=path, connectingEvent=connectingEvent
                ),
                error_handler=partial(
                    self.error_handler, devicePath=path, connectingEvent=connectingEvent
                ),
            )
            # await self.waiting_func(connectingEvent)
            await connectingEvent.wait()
            # await connectingEvent.wait()
            if self.connected == True:
                self.connecting = False
                logging.debug("Successfully connected")
                return
        await asyncio.sleep(3)  # Wait for connected status to stabilise
        logging.debug("Could not connect to any devices")
        self.connecting = False
        # raise Exception("????")

    def get_player(self, path):
        """Get a media player from a dbus path, and the associated device"""
        self.player = self.bus.get_object(SERVICE_NAME, path)
        logging.debug("Player [{}] has been set".format(path))
        device_path = self.player.Get(
            "org.bluez.MediaPlayer1", "Device", dbus_interface=PROP_IFACE,
        )
        self.get_device(device_path)

    def get_device(self, path):
        """Get a device from a dbus path """
        self.device = self.bus.get_object(SERVICE_NAME, path)
        self.deviceAlias = self.device.Get(
            DEVICE_IFACE, "Alias", dbus_interface=PROP_IFACE
        )

    def interfaces_added(self, path, interfaces):
        print(path)
        print(json.dumps(interfaces, indent=4))

    def signal_handler(self, interface, changed, invalidated, path):
        """Handle relevant property change signals"""
        logging.debug(f"Interface [{interface}] changed [{changed}] on path [{path}]")
        iface = interface[interface.rfind(".") + 1 :]
        if "Connected" in changed:
            self.connected = changed["Connected"]
            if changed["Connected"] == False:
                # FIXME check if anything trying to connect?
                if self.connecting == False:
                    asyncio.ensure_future(self.connect_devices(), loop=self.mainLoop)
            if iface == "MediaControl1":
                self.find_player()
                self.update_player(
                    "Connected", dbus_decode([self.connected, self.deviceAlias])
                )
        if "State" in changed:
            self.state = changed["State"]
            self.update_player("State", dbus_decode(self.state))
        if "Track" in changed:
            self.track = changed["Track"]
            self.update_player("Track", dbus_decode(self.track))
        if "Status" in changed:
            self.status = changed["Status"]
            self.update_player("Status", dbus_decode(self.status))
        if "Position" in changed:
            self.position = changed["Position"]
            self.update_player("Position", dbus_decode(self.position))
        if "Discoverable" in changed:
            self.discoverable = changed["Discoverable"]
            self.update_player("Discoverable", dbus_decode(self.discoverable))

    def set_discoverable(self, on=True):
        """Make the adapter discoverable"""
        adapter_path = self.adapter.object_path
        adapter = dbus.Interface(
            self.bus.get_object(SERVICE_NAME, adapter_path), PROP_IFACE,
        )
        adapter.Set(
            ADAPTER_IFACE, "DiscoverableTimeout", dbus.UInt32(self.discoverTimeout)
        )
        adapter.Set(ADAPTER_IFACE, "Discoverable", on)
        logging.debug(
            "Bluetooth is discoverable" if on else "Bluetooth is no longer discoverable"
        )

    def send_command(self, command):
        commands = {
            "Play": self.player.Play(dbus_interface=PLAYER_IFACE),
            "Pause": self.player.Pause(dbus_interface=PLAYER_IFACE),
            "Next": self.player.Next(dbus_interface=PLAYER_IFACE),
            "Previous": self.player.Previous(dbus_interface=PLAYER_IFACE),
            "FastForward": self.player.FastForward(dbus_interface=PLAYER_IFACE),
            "Rewind": self.player.Rewind(dbus_interface=PLAYER_IFACE),
        }
        # TODO: Fix volume adjustments
        # if command == "VolumeUp":
        #     self.transport.Volume(dbus_interface=TRANSPORT_IFACE) += 5
        # elif command == "VolumeDown":
        #     self.transport.Volume(dbus_interface=TRANSPORT_IFACE) -= 5
        # else:
        commands[command]()

    """Pairing agent methods"""

    @dbus.service.method(AGENT_IFACE, in_signature="o", out_signature="s")
    def RequestPinCode(self, device):
        print("RequestPinCode (%s)" % (device))
        self.trustDevice(device)
        return "0000"

    @dbus.service.method(AGENT_IFACE, in_signature="ouq", out_signature="")
    def DisplayPasskey(self, device, passkey, entered):
        print(f"DisplayPasskey ({device}, {passkey} entered {entered})")
        self.trustDevice(device)

    @dbus.service.method(AGENT_IFACE, in_signature="os", out_signature="")
    def DisplayPinCode(self, device, pincode):
        print(f"DisplayPinCode ({device}, {pincode})")
        self.trustDevice(device)

    @dbus.service.method(AGENT_IFACE, in_signature="ou", out_signature="")
    def RequestConfirmation(self, device, passkey):
        """Always confirm"""
        logging.debug("RequestConfirmation returns")
        self.trustDevice(device)

    @dbus.service.method(AGENT_IFACE, in_signature="os", out_signature="")
    def AuthorizeService(self, device, uuid):
        """Always authorize"""
        logging.debug("Authorize service returns")
        self.trustDevice(device)

    def trustDevice(self, path):
        """Set the device to trusted"""
        device_properties = dbus.Interface(
            self.bus.get_object(SERVICE_NAME, path), PROP_IFACE
        )
        device_properties.Set(DEVICE_IFACE, "Trusted", True)
