# Introduction

This project aims to provide a simple, but optimized, serialization library for C.
I have designed it to be somewhat modular, so that features can be added later
without incompatibility.
In it's base form, it is optimized for compactness of the serialized data.
If you use the WITHTABLE feature, a small meta data table is added to the
start of the data payload section. The meta data table contains entries that
describe the serialized items' type and precise offset in the serial
data payload. Most importantly, this table allows the deserializer
to access items in the packed serialized data in constant time.

You should note that I implemented this as a subproject of a school project.
This implies some parts were implemented very quickly and documentation
may be scant.

# General Usage

The general idea is to "pack" your data, send the packed data, receive packed data, 
and then "iterate" through the packed data.

You "pack" your data items using one of the serial_pack_* functions. These functions
produce a serial_data_t object. This object is what you can send and receive as 
serialized data. You can get a pointer to the start of this data by calling
serial_data_flat_ptr() on the serial data object AND you can get the total size of
the data by calling the serial_data_size(). These two parameters are sufficient to
to send or save the serialized data.

When you receive or open the packed data, you can read the data elements by using
the facilitated iterator. The idea is that you "begin" the iterator at the start
of the serialized data and progress through the elements via a "next" operation.
At each iterator position, you can read the data item using the serial_iit_get_*
functions. There are also iterator functions for detecting the current item's
type, size, presence.

# Dev Notes and Misc Info

Checkout the more complete development notes on the GitHub Wiki

The type control mechanism should be fixed. Currently, we handle the data as if it is unsigned(in most places).
The real issue arises when you consider that variable arguments only handle sizes of at least int and 
will type casts char and shorts to int upon invocation. The fear is that we may truncate a sign extension on
the deserialization end, instead of properly casting down the signed data. This could spell danger in a few places.
I have not verified this behavior yet, so it may be a non-issue or a big issue.

Pull requests are always welcome.
