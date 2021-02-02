#include "modbus/modbus-helpers.h"

#include "modbus-private.h"

/******************
 * HELPER FUNCTIONS
 *****************/

void
print_shim_info(const char *file, const char *function)
{
    int length = strlen(file) + strlen(function) + 3;
    printf("%s", DISPLAY_MARKER);
    printf("%s:%s()\n", file, function);
    for(int i = 0; i < length; ++i) {
        printf("-");
    }
    printf("\n");
}

/**
 * gets the name of a modbus function
 *
 * modifies READ_COILS to READ_SINGLE_COIL or READ_MULTIPLE_COILS
 * */
char *
modbus_get_function_name(modbus_t *ctx, const uint8_t *req)
{
#if defined(__freertos__)
    int *function = (int *)pvPortMalloc(sizeof(int));
    int *offset = (int *)pvPortMalloc(sizeof(int));
    int *slave_id = (int *)pvPortMalloc(sizeof(int));
    uint16_t *addr = (uint16_t *)pvPortMalloc(sizeof(uint16_t));
    int *nb = (int *)pvPortMalloc(sizeof(int));
    uint16_t *addr_wr = (uint16_t *)pvPortMalloc(sizeof(uint16_t));
    int *nb_wr = (int *)pvPortMalloc(sizeof(int));
#else
    int *function = (int *)malloc(sizeof(int));
    int *offset = (int *)malloc(sizeof(int));
    int *slave_id = (int *)malloc(sizeof(int));
    uint16_t *addr = (uint16_t *)malloc(sizeof(uint16_t));
    int *nb = (int *)malloc(sizeof(int));
    uint16_t *addr_wr = (uint16_t *)malloc(sizeof(uint16_t));
    int *nb_wr = (int *)malloc(sizeof(int));
#endif

    modbus_decompose_request(ctx, req, offset, slave_id, function, addr, nb, addr_wr, nb_wr);
    char *name = modbus_get_function_name_from_fc(*function);

    /* distinguish between reading a single or multiple coils */
    if(strcmp(name, "MODBUS_FC_READ_COILS") == 0) {
        if(*nb == 1) {
            name = "MODBUS_FC_READ_SINGLE_COIL";
        } else {
            name = "MODBUS_FC_READ_MULTIPLE_COILS";
        }
    } else if(strcmp(name, "MODBUS_FC_READ_HOLDING_REGISTERS") == 0) {
        if(*nb == 1) {
            name = "MODBUS_FC_READ_SINGLE_HOLDING_REGISTER";
        } else {
            name = "MODBUS_FC_READ_MULTIPLE_HOLDING_REGISTERS";
        }
    } else if(strcmp(name, "MODBUS_FC_READ_DISCRETE_INPUTS") == 0) {
        if(*nb == 1) {
            name = "MODBUS_FC_READ_SINGLE_DISCRETE_INPUT";
        } else {
            name = "MODBUS_FC_READ_MULTIPLE_DISCRETE_INPUTS";
        }
    }

    return name;
}

char *
modbus_get_function_name_from_fc(int function)
{
    switch(function) {
        case MODBUS_FC_READ_COILS:
            return "MODBUS_FC_READ_COILS";
        case MODBUS_FC_READ_DISCRETE_INPUTS:
            return "MODBUS_FC_READ_DISCRETE_INPUTS";
        case MODBUS_FC_READ_HOLDING_REGISTERS:
            return "MODBUS_FC_READ_HOLDING_REGISTERS";
        case MODBUS_FC_READ_INPUT_REGISTERS:
            return "MODBUS_FC_READ_INPUT_REGISTERS";
        case MODBUS_FC_WRITE_SINGLE_COIL:
            return "MODBUS_FC_WRITE_SINGLE_COIL";
        case MODBUS_FC_WRITE_SINGLE_REGISTER:
            return "MODBUS_FC_WRITE_SINGLE_REGISTER";
        case MODBUS_FC_WRITE_MULTIPLE_COILS:
            return "MODBUS_FC_WRITE_MULTIPLE_COILS";
        case MODBUS_FC_WRITE_MULTIPLE_REGISTERS:
            return "MODBUS_FC_WRITE_MULTIPLE_REGISTERS";
        case MODBUS_FC_REPORT_SLAVE_ID:
            return "MODBUS_FC_REPORT_SLAVE_ID";
        case MODBUS_FC_READ_EXCEPTION_STATUS:
            return "MODBUS_FC_READ_EXCEPTION_STATUS";
        case MODBUS_FC_MASK_WRITE_REGISTER:
            return "MODBUS_FC_MASK_WRITE_REGISTER";
        case MODBUS_FC_WRITE_AND_READ_REGISTERS:
            return "MODBUS_FC_WRITE_AND_READ_REGISTERS";
        case MODBUS_FC_READ_STRING:
            return "MODBUS_FC_READ_STRING";
        case MODBUS_FC_WRITE_STRING:
            return "MODBUS_FC_WRITE_STRING";
        default:
            return "ILLEGAL FUNCTION";
    }
}

/* Print CHERI capability details of mb_mapping pointers */
void
print_mb_mapping(modbus_mapping_t* mb_mapping)
{
    printf("mb_mapping:\t\t");
    if(mb_mapping != NULL) {
        CHERI_PRINT_CAP_LITE(mb_mapping);
    } else { printf ("NULL\n"); }

    printf("->tab_bits:\t\t");
    if(mb_mapping->tab_bits != NULL) {
        CHERI_PRINT_CAP_LITE(mb_mapping->tab_bits);
    } else { printf ("NULL\n"); }

    printf("->tab_input_bits:\t");
    if(mb_mapping->tab_input_bits != NULL) {
        CHERI_PRINT_CAP_LITE(mb_mapping->tab_input_bits);
    } else { printf ("NULL\n"); }

    printf("->tab_input_registers:\t");
    if(mb_mapping->tab_input_registers != NULL) {
        CHERI_PRINT_CAP_LITE(mb_mapping->tab_input_registers);
    } else { printf ("NULL\n"); }

    printf("->tab_registers:\t");
    if(mb_mapping->tab_registers != NULL) {
        CHERI_PRINT_CAP_LITE(mb_mapping->tab_registers);
    } else { printf ("NULL\n"); }

    printf(">tab_string:\t\t");
    if(mb_mapping->tab_string != NULL) {
        CHERI_PRINT_CAP_LITE(mb_mapping->tab_string);
    } else { printf ("NULL\n"); }

    // printf("mb_mapping:\t\t%#p\n", (void *)mb_mapping);
    // printf("->tab_bits:\t\t%#p\n", (void *)mb_mapping->tab_bits);
    // printf("->tab_input_bits:\t%#p\n", (void *)mb_mapping->tab_input_bits);
    // printf("->tab_input_registers:\t%#p\n", (void *)mb_mapping->tab_input_registers);
    // printf("->tab_registers:\t%#p\n", (void *)mb_mapping->tab_registers);
    // printf("->tab_string:\t\t%#p\n", (void *)mb_mapping->tab_string);
}

/**
 * Get the function code.
 * */
int
modbus_get_function_code(modbus_t *ctx, const uint8_t *req)
{
    if (ctx == NULL) {
        errno = EINVAL;
        return -1;
    }

    int offset = ctx->backend->header_length;
    int function = req[offset];

    return function;
}

/**
 * Decomposes the request.
 *
 * Obtains slave_id, function, address, and nb from the request.
 *
 * addr_wr and nb_wr are only used for MODBUS_FC_WRITE_AND_READ_REGISTERS
 * */
int
modbus_decompose_request(modbus_t *ctx, const uint8_t *req, int *offset,
                         int *slave, int *function, uint16_t *addr, int *nb,
                         uint16_t *addr_wr, int *nb_wr)
{
    if (ctx == NULL) {
        errno = EINVAL;
        return -1;
    }

    *offset = ctx->backend->header_length;
    *slave = req[*offset - 1];
    *function = req[*offset];
    *addr = (req[*offset + 1] << 8) + req[*offset + 2];
    *addr_wr = 0x0;
    *nb_wr = 0x0;

    /* Data are flushed on illegal number of values errors. */
    switch (*function) {
    case MODBUS_FC_READ_COILS:
    case MODBUS_FC_READ_DISCRETE_INPUTS:
    case MODBUS_FC_WRITE_MULTIPLE_REGISTERS:
    case MODBUS_FC_WRITE_STRING:
    case MODBUS_FC_READ_HOLDING_REGISTERS:
    case MODBUS_FC_READ_INPUT_REGISTERS:
    case MODBUS_FC_WRITE_MULTIPLE_COILS:
        *nb = (req[*offset + 3] << 8) + req[*offset + 4];
        break;
    case MODBUS_FC_WRITE_SINGLE_COIL:
    case MODBUS_FC_WRITE_SINGLE_REGISTER:
    case MODBUS_FC_MASK_WRITE_REGISTER:
        *nb = 1;
        break;
    case MODBUS_FC_WRITE_AND_READ_REGISTERS:
        *nb = (req[*offset + 3] << 8) + req[*offset + 4];
        *addr_wr = (req[*offset + 5] << 8) + req[*offset + 6];
        *nb_wr = (req[*offset + 7] << 8) + req[*offset + 8];
        break;
    default:
        *nb = 0;
        break;
    }

    return 0;
}

/* Helper function to print the elements of a request. */
void
print_modbus_decompose_request(modbus_t *ctx, const uint8_t *req)
{
#if defined(__freertos__)
    int *function = (int *)pvPortMalloc(sizeof(int));
    int *offset = (int *)pvPortMalloc(sizeof(int));
    int *slave_id = (int *)pvPortMalloc(sizeof(int));
    uint16_t *addr = (uint16_t *)pvPortMalloc(sizeof(uint16_t));
    int *nb = (int *)pvPortMalloc(sizeof(int));
    uint16_t *addr_wr = (uint16_t *)pvPortMalloc(sizeof(uint16_t));
    int *nb_wr = (int *)pvPortMalloc(sizeof(int));
#else
    int *function = (int *)malloc(sizeof(int));
    int *offset = (int *)malloc(sizeof(int));
    int *slave_id = (int *)malloc(sizeof(int));
    uint16_t *addr = (uint16_t *)malloc(sizeof(uint16_t));
    int *nb = (int *)malloc(sizeof(int));
    uint16_t *addr_wr = (uint16_t *)malloc(sizeof(uint16_t));
    int *nb_wr = (int *)malloc(sizeof(int));
#endif

    modbus_decompose_request(ctx, req, offset, slave_id, function, addr, nb, addr_wr, nb_wr);

    printf("decompose request:\n");
    printf("> offset:\t\t0x%.4X\n", *offset);
    printf("> slave_id:\t\t0x%.4X\n", *slave_id);
    printf("> function:\t\t0x%.4X (%s)\n", *function, modbus_get_function_name_from_fc(*function));
    printf("> addr:\t\t\t0x%.4X\n", *addr);
    printf("> nb:\t\t\t0x%.4X\n", *nb);
    printf("> addr_wr:\t\t0x%.4X\n", *addr_wr);
    printf("> nb_wr:\t\t0x%.4X\n", *nb_wr);
}
