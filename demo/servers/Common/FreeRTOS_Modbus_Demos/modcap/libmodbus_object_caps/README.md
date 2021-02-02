# Modbus CHERI object capabilities

This library implements object capabilities for `libmodbus` backed by CHERI architectural capabilities.

The library is designed as a shim layer between a Modbus server and `libmodbus`:

`modbus_mapping_new_start_address_object_caps()` is called *instead of* `libmodbus:modbus_mapping_new_start_address()`.  This prevents `libmodbus` from performing the initial state allocation, which would give `libmodbus` full permissions over the CHERI capabilities pointing to all the memory objects storing state.  Instead, this shim will be the only software layer or compartment will full permissions to the memory objects storing state.

`modbus_preprocess_request_object_caps()` is called *before* `modbus_preprocess_request()` to reduce permissions on the `mb_mapping` structure before passing it to `libmodbus`, limiting permissions on each object to only those required to execute a given Modbus function.

`modbus_preprocess_request_object_caps_stub()` can be called instead of `modubs_preprocess_request_object_caps()` during benchmarking to measure the cost of calling into the shim layer without actually performing any operations on the state object.  This is useful to measure the actual cost of `modbus_preprocess_request_object_caps()`, because you can measure and then subtract the overhead of calling into the shim.

## Usage

If `__CHERI_PURE_CAPABILITY__` and `MODBUS_OBJECT_CAPABILITIES` are not defined, this shim simply acts as a pass through to `libmodbus`.

Example code snip from a Modbus server:

```
/* Create mb_mapping struct to hold Modbus state objects */
mb_mapping = modbus_mapping_new_start_address_object_caps(
        ctx,
        UT_BITS_ADDRESS, UT_BITS_NB,
        UT_INPUT_BITS_ADDRESS, UT_INPUT_BITS_NB,
        UT_REGISTERS_ADDRESS, UT_REGISTERS_NB_MAX,
        UT_INPUT_REGISTERS_ADDRESS, UT_INPUT_REGISTERS_NB);

/* Receive a request from a Modbus client */
req_length = modbus_receive(ctx, req);

/* Preprocess the request, reducing permissions on objects to those required */
rc = modbus_preprocess_request_object_caps(ctx, req, mb_mapping);

/* Process the request: read or modify state and construct the response */
rc = modbus_process_request(ctx, req, req_length, rsp, &rsp_length, mb_mapping);

/* Reply to the Modbus client */
rc = modbus_reply(ctx, rsp, rsp_length);
```

