-------------------------------------------------------------------------------
-- vim: ts=4 sw=4 sts=4 et tw=78
--
-- Copyright (c) 2009 James R. McKaskill
--
-- Permission is hereby granted, free of charge, to any person obtaining a
-- copy of this software and associated documentation files (the "Software"),
-- to deal in the Software without restriction, including without limitation
-- the rights to use, copy, modify, merge, publish, distribute, sublicense,
-- and/or sell copies of the Software, and to permit persons to whom the
-- Software is furnished to do so, subject to the following conditions:
--
-- The above copyright notice and this permission notice shall be included in
-- all copies or substantial portions of the Software.
--
-- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
-- IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
-- FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
-- AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
-- LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
-- FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
-- DEALINGS IN THE SOFTWARE.
-------------------------------------------------------------------------------

smodule 'adbus'
require 'adbuslua_core'
require 'string'
require 'table'
require 'os'
require 'package'

-------------------------------------------------------------------------------
-- Connection API
-------------------------------------------------------------------------------

local mt = {}
mt.__index = mt

_M.connect_address = adbuslua_core.connect_address
_M.bind_address = adbuslua_core.bind_address

--- Creates a connection
-- \param bus       The bus to connect to.
-- \param debug     boolean flag to enable/disable debugging (default
--                  disabled)
--
-- \return The new connection object
--
-- The bus can be one of:
-- - nil - we then connect to the default bus ('session')
-- - 'session' or 'system' to connect to the session or system buses
--   respectively
-- - A string giving the bus address in dbus form eg.
--   'tcp:host=localhost,port=12345'


function _M.connect(bus)
    local self = setmetatable({}, mt)

    self._connection = adbuslua_core.connection.new()
    self._socket     = adbuslua_core.socket.new(bus or 'session')

    self._connection:set_sender(function(data)
        self._socket:send(data)
    end)

    local connected
    self._connection:connect_to_bus(function(name)
        if callback then callback(name) end
        connected = true
    end)

    while not connected do
        self:process()
    end

    return self
end

function mt:close()
    self._connection:close()
    self._socket:close()
end

--- Returns whether we successfully connected to the bus.
function mt:is_connected_to_bus()
    return self._connection:is_connected_to_bus()
end

--- Returns our unique name if we are connected to the bus or nil.
function mt:unique_name()
    return self._connection:unique_name()
end

--- Returns a new serial for use with messages.
function mt:serial()
    return self._connection:serial()
end

function mt:process()
    local data = self._socket:receive()
    self._connection:parse(data)
end

--- Adds a match rule to the connection
--
-- The param is a table detailing the fields of messages to match against.
-- Only the callback is required. Optional fields not specified will always
-- match.
--
-- Arguments:
-- \param m.callback            Callback for when the match is hit
-- \param[opt] m.type           type of message. Valid values are:
--                               - 'method_call'
--                               - 'method_return'
--                               - 'error'
--                               - 'signal'
-- \param[opt] m.sender         Service name
-- \param[opt] m.destination    Service name
-- \param[opt] m.interface      String
-- \param[opt] m.reply_serial   Number
-- \param[opt] m.path           String
-- \param[opt] m.member         String
-- \param[opt] m.error          String
-- \param[opt] m.remove_on_first_match      Boolean
-- \param[opt] m.add_match_to_bus_daemon    Boolean
-- \param[opt] m.object         Anything
-- \param[opt] m.id             Number - should be an id returned from
--                              connection:match_id.
-- \param[opt] m.unpack_message Boolean
-- \return The match id. This will be the value of m.id if it is set.
--
-- Details:
-- The sender and destination fields should be valid dbus names (ascii dot
-- seperated string). They can either be a unique name (begins with : eg
-- :1.34) or a service name (reverse dns eg org.freedesktop.DBus) in which
-- case the service will be tracked.
--
-- The path should be a valid dbus path name (unix style path eg
-- /org/freedesktop/DBus).
--
-- The id can be filled out with a return number from connection:match_id
-- or it can be left blank in which case an id will be generated. Either way
-- the id is returned and should be used as the paramater to remove_match.
--
-- By default the match will be kept active until the match is explicitely
-- removed or the connection is destroyed. Alternatively setting
-- m.remove_on_first_match will remove the match the first time it hits a
-- match. This is most often used to match return values.
--
-- The match rule will normally be a local only match and thus can match
-- method_calls (method_calls should really be redirected via an interface),
-- method_returns and errors, but in order to match a signal the match rule
-- must be added to the bus daemon by setting m.add_match_to_bus_daemon.
-- Otherwise the bus will not redirect the signal message our way. This is not
-- necessary for non bus connection.
--
-- The arguments to the callback depend on two params (unpack_message and
-- object). If object is set then the first argument to the function will be
-- that object. If unpack_message is set to true or not set (ie default), the
-- message arguments will be unpacked so the call to the callback will be of
-- the form:
--
-- callback(object, arg0, arg1, ...) or callback(arg0, arg1, ...)
--
-- unpacked error message take a special form. In this case the callback is
-- callback is called with the object (optional), a nil, the message error
-- name, and then the message description (if provided). eg:
--
-- callback(object, nil, 'org.freedesktop.DBus.Error.UnknownMethod',
--          'org.freedesktop.DBus does not understand message foo')
--
-- If unpack_message is set to false then the second argument (if object is
-- set) will be a message object. See connection.send for the format of 
-- this object. eg
-- 
-- callback(object, message) or callback(message)
--
-- Example:
-- \code
-- local function owner_changed(name, from, to)
-- end
--
-- c:add_match{
--      type = 'signal',
--      add_match_to_bus_daemon = true,
--      sender = 'org.freedesktop.DBus',
--      member = NameOwnerChanged
-- }
-- \endcode
--          
--
-- \sa  connection:remove_match
--      connection:send_messsage
--
function mt:add_match(m)
    return self._connection:add_match(m)
end

function mt:add_reply(r)
    return self._connection:add_reply(r)
end

local bind = {}

function bind:emit(signal, ...)
    local sig = self._interface._signal_signature[member]
    if not sig then
        error("Invalid signal name")
    end

    local msg = {
        path        = self._path,
        interface   = self._interface.name,
        signature   = sig,
        member      = signal,
        ...
    }

    self._connection:send(msg)
end

function mt:add_bind(path, interface)
    local b         = setmetatable({}, bind)
    b._bind         = self._connection:add_bind(path, interface._interface)
    b._interface    = interface
    b._path         = path
    b._connection   = self
    return b
end

--- Create a new proxy object
-- 
-- Proxies objects are the main interface to call methods across dbus. On
-- creation the proxy will introspect the target path and create wrapper
-- methods and properties. Methods can be called as a member function which
-- take the arguments to proxy and return the return arguments. Properties can
-- be gotten by accessing a table index and set by setting a table index. The
-- proxy also contains some convenience functions to hook up a callback to a
-- signal from that remote object.
--
-- All method_calls, and property gets/sets are blocking calls in order to
-- return the values returned in the method_return message. The proxy will
-- also assert on a returned error.
--
-- There are a number of method functions for the proxy itself which (eg
-- getting the connection or path, or connecting signals). Since member
-- functions are used to call method calls, these are provided calling the
-- function with the proxy as the first argument (eg adbuslua.interface(p)).
--
-- For example:
-- \code
-- p = mt:proxy{
--      service = 'org.freedesktop.DBus',
--      path    = '/org/freedesktop/DBus',
-- }
--
-- names = p:ListNames()
-- \endcode
--
-- The argument is a table of fields:
-- \param t.service         The service name of the dbus object to proxy
-- \param t.path            The path of the dbus object to proxy
-- \param[opt] t.interface  An interface on the target path to restrict the
--                          proxy to
--
-- \return A proxy object - see \ref proxy for more details
--
-- \sa proxy
--
function mt:proxy(service, path, interface)
    if path == nil then
        return service_proxy._new(self, service)
    else
        return proxy._new(self, service, path, interface)
    end
end

--- Sends a new message
-- 
-- If arguments are provided, then the signature must be set. The format of
-- the signature can be either:
-- - A single string of the dbus signatures of each argument concatanated
--   together
-- - A table of strings of dbus signatures for each argument
--
-- The dbus signature is an ascii string that represents the marshalled type.
-- Valid values for this are:
--
-- The simple types are:
-- y    - 8 bit unsigned integer
-- b    - Boolean value
-- n    - 16 bit signed integer
-- q    - 16 bit unsigned integer
-- i    - 32 bit signed integer
-- u    - 32 bit unsigned integer
-- x    - 64 bit signed integer
-- t    - 64 bit unsigned integer
-- d    - 64 bit floating point number (double)
-- s    - string
-- o    - dbus object path
-- g    - dbus signature
-- 
-- The compound types are (using X as a placeholder for the inner type):
-- aX    - an array of values of type X
--       - X must be a single conceptual type
-- (X)   - a structure of values of type X 
--       - X can be one or more types in sequence
-- v     - a variant where each occurance dictates the type with the data
-- a{XY} - a dictionary/table/map with keys of type X and values of type Y
--
-- The marshalling from lua values can be grouped into a number of categories:
-- - numeric dbus values (8, 16, 32, 64 bit integers and double) expect a lua
--   number argument
-- - boolean dbus values expect a lua boolean argument
-- - string dbus values (string, object path, and signature) expect a lua
--   string
-- - dbus arrays expect a table of the appropriate values of length #table
--   using integer indices starting at 1 (standard lua arrays)
-- - dbus maps expect a table with the appropriate key and value types
-- - dbus variants will marshall in a number of ways
--   - a lua number is marshalled as a double
--   - a lua boolean is marshalled as a boolean
--   - a lua string is marshalled as a string
--   - a lua table is marshalled as an array of variants if table[1] ~= nil 
--     or as a dbus map with keys and values as variants
--   - if the value
--
--
-- The argument is a message object. The same format is also used for packed
-- match callbacks. This is a table with a number of fields:
-- \param m.type                The type of message. This can be one of:
--                               - 'method_call'
--                               - 'method_return'
--                               - 'error'
--                               - 'signal'
-- \param[opt] m.serial         The serial of the message. If set this should
--                              be set using a return of connection.next_serial.
-- \param[opt] m.interface      Interface field
-- \param[opt] m.path           Path field
-- \param[opt] m.member         Member field
-- \param[opt] m.error          error name field
-- \param[opt] m.destination    Destination field
-- \param[opt] m.sender         Sender field
-- \param[opt] m.signature      Signature of the message. See above.
-- \param[opt] m.no_reply_expected  If set to true, sets the NO_REPLY_EXPECTED
--                              flag in the message. This is an optimisation
--                              where the remote side can then choose to not
--                              send back a reply.
-- \param[opt] m.no_auto_start  If set to true, sets the NO_AUTO_START flag in
--                              the message. This is an indication to the bus
--                              to not auto start a daemon to handle the
--                              message.
-- \param[opt] m[1], m[2] ...   The arguments of the message. They are
--                              marshalled using the signature field.
--
-- \sa  connection.add_match 
--      http://dbus.freedesktop.org/doc/dbus-specification.html
--
function mt:send(m)
    self._connection:send(m)
end

-- Method callers

function mt:async_call(msg)
    local serial = self:serial()

    local reply
    if msg.callback then
        reply = self:add_reply{
            serial          = serial,
            remote          = msg.destination,
            unpack_message  = msg.unpack_message,
            callback        = msg.callback,
            object          = msg.object,
        }
    end

    msg.type = "method_call"
    msg.serial = serial
    msg.unpack_message = nil
    msg.callback = nil
    msg.object = nil

    self:send(msg)

    return reply
end

local function method_callback(self, message)
    self._method_return_message = message
    self._yield = true
end

function mt:call(msg)
    local unpack_message = msg.unpack_message 

    if msg.reply == nil or msg.reply then
        msg.callback = method_callback
        msg.object = self
        msg.unpack_message = false
    end
    msg.reply = nil

    local reply = self:async_call(msg)

    if reply then
        self._yield = nil
        while not self._yield do
            self:process()
        end

        reply:close()

        local ret = self._method_return_message
        self._method_return_message = nil

        if unpack_message == nil or not unpack_message then
            if ret.type == "error" then
                return nil, ret.error, ret[1]
            else
                return unpack(ret)
            end
        else
            return ret
        end
    end
end


