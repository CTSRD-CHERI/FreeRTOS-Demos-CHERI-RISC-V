# Modbus Macaroon network capabilities

This library implements network capabilities for `libmodbus` backed by Macaroon network tokens.

The library is designed as a shim layer between a Modbus server or client and `libmodbus`:

The library functions are designed to be called byteh server or client *instead of* the `libmodbus` functions.  They will wrap the `libmodbus` function with code that either sends an applicable Macaroon (from the client) or verifies a received Macaroon (at the server) before performing the applicable `libmodbus` function.

Both the Modbus server and client have initialisation functions to generate or obtain the base Macaroon that will be used for subsequent communication.

The client functions add the Modbus function code and memory address to the Macaroon as caveats, which are protected by the Macaroon's HMAC.  The server verifies the HMAC and then confirms that the function code and memory address match those in the actual Modbus request from the client.  The HMAC prevents an attacker from tampering with the Macaroon, and the Macaroon prevents an attacker from tampering with the Modbus request.

## Usage

`MODBUS_NETWORK_CAPABILITIES` must be defined.

Example code snip from a Modbus server:

```
/* Initialise Macaroon */
int rc;
char *key = "a bad secret";
char *id = "id for a bad secret";
char *location = "https://www.modbus.com/macaroons/";
rc = initialise_server_macaroon(ctx, location, key, id);

/* Preprocess a Modbus request from a client, verifying a valid Macaroon has been received */
rc = modbus_preprocess_request_macaroons(ctx, req, mb_mapping);
```

Example code snip from a Modbus client:
```
/* Use modbus_read_string() to request the serialised base Macaroon from the server */
rc = modbus_read_string(ctx, tab_rp_string);

/* Use the serialised base Macaroon to initialise the client's Macaroon */
rc = initialise_client_network_caps(ctx, (char *)tab_rp_string, rc);

/* Send a request to change the state of a single coil */
rc = modbus_write_bit_network_caps(ctx, UT_BITS_ADDRESS, ON);
```

