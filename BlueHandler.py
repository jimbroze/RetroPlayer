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

SERVICE_NAME = "org.bluez"
AGENT_IFACE = SERVICE_NAME + ".Agent1"
ADAPTER_IFACE = SERVICE_NAME + ".Adapter1"
DEVICE_IFACE = SERVICE_NAME + ".Device1"
PLAYER_IFACE = SERVICE_NAME + ".MediaPlayer1"
TRANSPORT_IFACE = SERVICE_NAME + ".MediaTransport1"
IFACE = "org.freedesktop.DBus"
MANAGER_IFACE = IFACE + ".ObjectManager"
PROP_IFACE = IFACE + ".Properties"

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


class BlueHandler(dbus.service.Object):
    """A class that handles media player bluetooth operations. Takes dbus object,
    media player object and bluetooth io capability as arguments."""

    AGENT_PATH = "/CarPooter/agent"
    #    CAPABILITY = "DisplayOnly"
    capability = "NoInputNoOutput"

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
    track = None
    mainloop = None

    def __init__(self, bus, updatePlayer, capability="NoInputNoOutput"):
        """Initialize gobject and find any current media players"""
        self.capability = capability
        self.updatePlayer = updatePlayer
        self.bus = bus

        dbus.service.Object.__init__(self, bus, BlueHandler.AGENT_PATH)

        self.bus.add_signal_receiver(
            self.signal_handler,
            bus_name=SERVICE_NAME,
            dbus_interface=PROP_IFACE,
            signal_name="PropertiesChanged",
            path_keyword="path",
        )

        self.registerAgent()
        self.findPlayer()

        # adapter_path = find_adapter().object_path
        # self.bus.add_signal_receiver(
        #     self.signal_handler,
        #     bus_name="org.bluez",
        #     path=adapter_path,
        #     dbus_interface=PROP_IFACE,
        #     signal_name="PropertiesChanged",
        #     path_keyword="path",
        # )

    def registerAgent(self):
        """Register BlueHandler as the default agent"""
        manager = dbus.Interface(
            self.bus.get_object(SERVICE_NAME, "/org/bluez"), "org.bluez.AgentManager1"
        )
        manager.RegisterAgent(BlueHandler.AGENT_PATH, self.capability)
        manager.RequestDefaultAgent(BlueHandler.AGENT_PATH)
        logging.debug("BlueHandler is registered as the default agent")

    def findPlayer(self):
        """Find any current media players and associated device"""
        manager = dbus.Interface(
            self.bus.get_object(SERVICE_NAME, "/"), "org.freedesktop.DBus.ObjectManager"
        )
        objects = manager.GetManagedObjects()

        player_path = None
        transport_path = None
        for path, interfaces in objects.items():
            if PLAYER_IFACE in interfaces:
                player_path = path
            if TRANSPORT_IFACE in interfaces:
                transport_path = path

        if player_path:
            logging.debug("Found player on path [{}]".format(player_path))
            self.connected = True
            self.getPlayer(player_path)
            player_properties = self.player.GetAll(
                PLAYER_IFACE, dbus_interface=PROP_IFACE
            )
            if "Status" in player_properties:
                self.status = player_properties["Status"]
            if "Track" in player_properties:
                self.track = player_properties["Track"]
        else:
            logging.debug("Could not find player")

        if transport_path:
            logging.debug("Found transport on path [{}]".format(player_path))
            self.transport = self.bus.get_object(SERVICE_NAME, transport_path)
            logging.debug("Transport [{}] has been set".format(transport_path))
            transport_properties = self.transport.GetAll(
                TRANSPORT_IFACE, dbus_interface=PROP_IFACE
            )
            if "State" in transport_properties:
                self.state = transport_properties["State"]

    def getPlayer(self, path):
        """Get a media player from a dbus path, and the associated device"""
        self.player = self.bus.get_object(SERVICE_NAME, path)
        logging.debug("Player [{}] has been set".format(path))
        device_path = self.player.Get(
            "org.bluez.MediaPlayer1", "Device", dbus_interface=PROP_IFACE,
        )
        self.getDevice(device_path)

    def getDevice(self, path):
        """Get a device from a dbus path"""
        self.device = self.bus.get_object(SERVICE_NAME, path)
        self.deviceAlias = self.device.Get(
            DEVICE_IFACE, "Alias", dbus_interface=PROP_IFACE
        )

    def signal_handler(self, interface, changed, invalidated, path):
        """Handle relevant property change signals"""
        logging.debug(
            "Interface [{}] changed [{}] on path [{}]".format(interface, changed, path)
        )
        iface = interface[interface.rfind(".") + 1 :]
        if "Connected" in changed:
            self.connected = changed["Connected"]
            if iface == "MediaControl1":
                self.findPlayer()
        if "State" in changed:
            self.state = changed["State"]
        if "Track" in changed:
            self.track = changed["Track"]
        if "Status" in changed:
            self.status = changed["Status"]
        if "Discoverable" in changed:
            self.discoverable = changed["Discoverable"]

        # self.updatePlayer()

    # TODO timeout?
    def setDiscoverable(self, on=True):
        """Make the adapter discoverable"""
        adapter_path = find_adapter().object_path
        adapter = dbus.Interface(
            self.bus.get_object(SERVICE_NAME, adapter_path), PROP_IFACE,
        )
        adapter.Set(ADAPTER_IFACE, "Discoverable", on)
        logging.debug(
            "Bluetooth is discoverable" if on else "Bluetooth is no longer discoverable"
        )

    def sendCommand(self, command):
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

    @dbus.service.method(AGENT_IFACE, in_signature="ou", out_signature="")
    def RequestConfirmation(self, device, passkey):
        """Always confirm"""
        logging.debug("RequestConfirmation returns")
        self.trustDevice(device)

    @dbus.service.method(AGENT_IFACE, in_signature="os", out_signature="")
    def AuthorizeService(self, device, uuid):
        """Always authorize"""
        logging.debug("Authorize service returns")

    def trustDevice(self, path):
        """Set the device to trusted"""
        device_properties = dbus.Interface(
            self.bus.get_object(SERVICE_NAME, path), PROP_IFACE
        )
        device_properties.Set(DEVICE_IFACE, "Trusted", True)
