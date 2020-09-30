/*******************************************************************************
*   XYM Wallet
*   (c) 2017 Ledger
*   (c) 2020 FDS
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/
#include <string.h>
#include <inttypes.h>
#include "format.h"
#include "fields.h"
#include "readers.h"
#include "xym/xym_helpers.h"
#include "common.h"
#include "base32.h"

typedef void (*field_formatter_t)(field_t* field, char* dst);

void int8_formatter(field_t* field, char *dst) {
    int8_t value = read_int8(field->data);
    if (value > 0) {
        SNPRINTF(dst, "%s %d %s", "Add", value, "address(es)");
    } else if (value < 0) {
        SNPRINTF(dst, "%s %d %s", "Remove", ~value + 1, "address(es)");
    } else {
        SNPRINTF(dst, "%s", "Not change");
    }
}

void uint8_formatter(field_t* field, char *dst) {
    uint8_t value = read_uint8(field->data);
    if (field->id == XYM_UINT8_MOSAIC_COUNT) {
        SNPRINTF(dst, "Found %d txs", value);
    } else if (field->id == XYM_UINT8_TXN_MESSAGE_TYPE) {
        if (value == 0x00) {
            SNPRINTF(dst, "%s", "Plain text");
        } else if (value == 0x01) {
            SNPRINTF(dst, "%s", "Encrypted text");
        } else if (value == 0xFE) {
            SNPRINTF(dst, "%s", "Persistent harvesting delegation");
        }
    } else if (field->id == XYM_UINT8_AA_TYPE) {
        if (value == 0) {
            SNPRINTF(dst, "%s", "Unlink address");
        } else if (value == 1) {
            SNPRINTF(dst, "%s", "Link address");
        }
    } else if (field->id == XYM_UINT8_NS_REG_TYPE) {
        if (value == 0) {
            SNPRINTF(dst, "%s", "Root namespace");
        } else if (value == 1) {
            SNPRINTF(dst, "%s", "Sub namespace");
        }
    } else if (field->id == XYM_UINT8_MSC_ACTION) {
        if (value == 0) {
            SNPRINTF(dst, "%s", "Decrease");
        } else if (value == 1) {
            SNPRINTF(dst, "%s", "Increase");
        }
    } else if (field->id == XYM_UINT8_MD_SUPPLY_FLAG) {
        if ((value & 0x01) != 0) {
            SNPRINTF(dst, "%s", "Yes");
        } else {
            SNPRINTF(dst, "%s", "No");
        }
    } else if (field->id == XYM_UINT8_MD_TRANS_FLAG) {
        if ((value & 0x02) != 0) {
            SNPRINTF(dst, "%s", "Yes");
        } else {
            SNPRINTF(dst, "%s", "No");
        }
    } else if (field->id == XYM_UINT8_MD_RESTRICT_FLAG) {
        if ((value & 0x04) != 0) {
            SNPRINTF(dst, "%s", "Yes");
        } else {
            SNPRINTF(dst, "%s", "No");
        }
    } else {
        SNPRINTF(dst, "%d", value);
    }
}

void uint16_formatter(field_t* field, char *dst) {
    uint16_t value = read_uint16(field->data);
    switch (value) {
        CASE_FIELDVALUE(XYM_TXN_TRANSFER, "Transfer")
        CASE_FIELDVALUE(XYM_TXN_REGISTER_NAMESPACE, "Register Namespace")
        CASE_FIELDVALUE(XYM_TXN_ADDRESS_ALIAS, "Address Alias")
        CASE_FIELDVALUE(XYM_TXN_MOSAIC_ALIAS, "Mosaic Alias")
        CASE_FIELDVALUE(XYM_TXN_MOSAIC_DEFINITION, "Mosaic definition")
        CASE_FIELDVALUE(XYM_TXN_MOSAIC_SUPPLY_CHANGE, "Mosaic Supply Change")
        CASE_FIELDVALUE(XYM_TXN_MODIFY_MULTISIG_ACCOUNT, "Modify Multisig Account")
        CASE_FIELDVALUE(XYM_TXN_AGGREGATE_COMPLETE, "Aggregate Complete")
        CASE_FIELDVALUE(XYM_TXN_AGGREGATE_BONDED, "Aggregate Bonded")
        CASE_FIELDVALUE(XYM_TXN_HASH_LOCK, "Hash Lock")
        default:
            SNPRINTF(dst, "%s", "Unknown");
    }
}

void uint32_formatter(field_t* field, char *dst) {
    uint32_t value = read_uint32(field->data);
    SNPRINTF(dst, "%x", value);
}

void hash_formatter(field_t* field, char *dst) {
    sprintf_hex(dst, MAX_FIELD_LEN, field->data, field->length, 0);
}

void uint64_formatter(field_t* field, char *dst) {
    if (field->id == XYM_UINT64_DURATION) {
        uint64_t duration = read_uint64(field->data);
        if (duration == 0) {
            SNPRINTF(dst, "%s", "Unlimited");
        } else {
            uint8_t day = duration / 5760;
            uint8_t hour = (duration % 5760) / 240;
            uint8_t min = (duration % 240) / 4;
            SNPRINTF(dst, "%d%s%d%s%d%s", day, "d ", hour, "h ", min, "m");
        }
    } else if (field->id == XYM_UINT64_MSC_AMOUNT) {
        xym_print_amount(read_uint64(field->data), 0, "", dst);
    } else {
        sprintf_hex(dst, MAX_FIELD_LEN, field->data, field->length, 1);
    }
}

void address_formatter(field_t* field, char *dst) {
    base32_encode(field->data, XYM_ADDRESS_LENGTH, dst, XYM_PRETTY_ADDRESS_LENGTH);
    dst[39] = '\0';
}

void mosaic_formatter(field_t* field, char *dst) {
    if (field->dataType == STI_MOSAIC_CURRENCY) {
        mosaic_t* value = (mosaic_t *)field->data;
        if (value->mosaicId == XYM_MOSAIC_ID || field->id == XYM_MOSAICT_HL_QUANTITY) {
            xym_print_amount(value->amount, 6, "XYM", dst);
        } else {
            sprintf_mosaic(dst, MAX_FIELD_LEN, value, "micro");
        }
    }
}

void xym_formatter(field_t* field, char *dst) {
    if (field->dataType == STI_XYM) {
        xym_print_amount(read_uint64(field->data), 6, "XYM", dst);
    }
}

void msg_formatter(field_t* field, char *dst) {
    if (field->length == 0) {
        SNPRINTF(dst, "%s", "<empty msg>");
    } else if (field->length >= MAX_FIELD_LEN) {
        sprintf_ascii(dst, MAX_FIELD_LEN, &field->data[0], MAX_FIELD_LEN - 1);
    } else {
        sprintf_ascii(dst, MAX_FIELD_LEN, &field->data[0], field->length);
    }
}

void string_formatter(field_t* field, char *dst) {
    if (field->length > MAX_FIELD_LEN) {
        sprintf_ascii(dst, MAX_FIELD_LEN, field->data, MAX_FIELD_LEN - 1);
    } else {
        sprintf_ascii(dst, MAX_FIELD_LEN, field->data, field->length);
    }
}

field_formatter_t get_formatter(field_t* field) {
    switch (field->dataType) {
        case STI_INT8:
            return int8_formatter;
        case STI_UINT8:
            return uint8_formatter;
        case STI_UINT16:
            return uint16_formatter;
        case STI_UINT32:
            return uint32_formatter;
        case STI_UINT64:
            return uint64_formatter;
        case STI_HASH256:
            return hash_formatter;
        case STI_ADDRESS:
            return address_formatter;
        case STI_MOSAIC_COUNT:
        case STI_MOSAIC_CURRENCY:
            return mosaic_formatter;
        case STI_XYM:
            return xym_formatter;
        case STI_MESSAGE:
            return msg_formatter;
        case STI_STR:
            return string_formatter;
        default:
            return NULL;
    }
}

void format_field(field_t* field, char* dst) {
    memset(dst, 0, MAX_FIELD_LEN);

    field_formatter_t formatter = get_formatter(field);
    if (formatter != NULL) {
        formatter(field, dst);
    } else {
        SNPRINTF(dst, "%s", "[Not implemented]");
    }

    // Replace a zero-length string with a space because of rendering issues
    if (dst[0] == 0x00) {
        dst[0] = ' ';
    }
}
