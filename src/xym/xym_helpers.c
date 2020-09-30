/*******************************************************************************
*   XYM Wallet
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
#include "base32.h"
#include "xym_helpers.h"

void xym_print_amount(uint64_t amount, uint8_t divisibility, char *asset, char *out) {
    char buffer[AMOUNT_MAX_SIZE];
    uint64_t dVal = amount;
    int i, j;
    uint8_t MAX_DIVISIBILITY = (divisibility == 0) ? 0 : 6;

    // If the amount can't be represented safely in JavaScript, signal an error
    //if (MAX_SAFE_INTEGER < amount) THROW(0x6a80);

    memset(buffer, 0, AMOUNT_MAX_SIZE);
    for (i = 0; dVal > 0 || i < MAX_DIVISIBILITY + 1; i++) {
        if (dVal > 0) {
            buffer[i] = (dVal % 10) + '0';
            dVal /= 10;
        } else {
            buffer[i] = '0';
        }
        if (i == divisibility - 1) { // divisibility
            i += 1;
            buffer[i] = '.';
            if (dVal == 0) {
                i += 1;
                buffer[i] = '0';
            }
        }
        if (i >= AMOUNT_MAX_SIZE) {
            THROW(0x6700);
        }
    }
    // reverse order
    for (i -= 1, j = 0; i >= 0 && j < AMOUNT_MAX_SIZE-1; i--, j++) {
        out[j] = buffer[i];
    }
    // strip trailing 0s
    if (MAX_DIVISIBILITY != 0)
    {
        for (j -= 1; j > 0; j--) {
            if (out[j] != '0') break;
        }
        j += 1;
    }

    // strip trailing .
    if (out[j-1] == '.') j -= 1;

    if (asset) {
        // qualify amount
        out[j++] = ' ';
        strcpy(out + j, asset);
        out[j+strlen(asset)] = '\0';
    } else {
        out[j] = '\0';
    }
}

void sha_calculation(uint8_t algorithm, uint8_t *in, uint8_t inlen, uint8_t *out) {
    cx_sha3_t hash;
    if (algorithm == CX_KECCAK) {
        cx_keccak_init(&hash, 256);
    } else {
        cx_sha3_init(&hash, 256);
    }
    cx_hash(&hash.header, CX_LAST, in, inlen, out);
}

void ripemd(uint8_t *in, uint8_t inlen, uint8_t *out) {
    cx_ripemd160_t hash;
    cx_ripemd160_init(&hash);
    cx_hash(&hash.header, CX_LAST, in, inlen, out);
}

void xym_public_key_and_address(cx_ecfp_public_key_t *inPublicKey, uint8_t inNetworkId, unsigned int inAlgo, uint8_t *outPublicKey, char *outAddress) {
    uint8_t buffer1[32];
    uint8_t buffer2[20];
    uint8_t rawAddress[32];

    for (uint8_t i=0; i<32; i++) {
        outPublicKey[i] = inPublicKey->W[64 - i];
    }
    if ((inPublicKey->W[32] & 1) != 0) {
        outPublicKey[31] |= 0x80;
    }
    sha_calculation(inAlgo, outPublicKey, 32, buffer1);
    ripemd(buffer1, 32, buffer2);
    //step1: add network prefix char
    rawAddress[0] = inNetworkId;   //152:,,,,,
    //step2: add ripemd160 hash
    os_memmove(rawAddress + 1, buffer2, sizeof(buffer2));
    sha_calculation(inAlgo, rawAddress, 21, buffer1);
    //step3: add checksum
    os_memmove(rawAddress + 21, buffer1, 4);
    base32_encode((const uint8_t *)rawAddress, 25, (char *) outAddress, 40);
}
