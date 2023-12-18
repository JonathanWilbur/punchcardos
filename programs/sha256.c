//usr/bin/env clang -Ofast -Wall -Wextra -pedantic ${0} -o ${0%%.c*} $* ;exit $?
//
//  SHA-256 implementation, Mark 2
//
//  Copyright (c) 2010,2014 Literatecode, http://www.literatecode.com
//  Copyright (c) 2022 Ilia Levin (ilia@levin.sg)
//
//  Permission to use, copy, modify, and distribute this software for any
//  purpose with or without fee is hereby granted, provided that the above
//  copyright notice and this permission notice appear in all copies.
//
//  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
//  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
//  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
//  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
//  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
//  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
//  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint8_t  buf[64];
    uint32_t hash[8];
    uint32_t bits[2];
    uint32_t len;
    uint32_t rfu__;
    uint32_t W[64];
} sha256_context;

void sha256_init(sha256_context *ctx);
void sha256_hash(sha256_context *ctx, const void *data, size_t len);
void sha256_done(sha256_context *ctx, uint8_t *hash);
static void sha256(const void *data, size_t len, uint8_t *hash);

#ifndef _cbmc_
#define __CPROVER_assume(...) do {} while(0)
#endif

#define FN_ static inline __attribute__((const))

static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};


FN_ uint8_t _shb(uint32_t x, uint32_t n)
{
    return ((x >> (n & 31)) & 0xff);
} // _shb


FN_ uint32_t _shw(uint32_t x, uint32_t n)
{
    return ((x << (n & 31)) & 0xffffffff);
} // _shw


FN_ uint32_t _r(uint32_t x, uint8_t n)
{
    return ((x >> n) | _shw(x, 32 - n));
} // _r


FN_ uint32_t _Ch(uint32_t x, uint32_t y, uint32_t z)
{
    return ((x & y) ^ ((~x) & z));
} // _Ch


FN_ uint32_t _Ma(uint32_t x, uint32_t y, uint32_t z)
{
    return ((x & y) ^ (x & z) ^ (y & z));
} // _Ma


FN_ uint32_t _S0(uint32_t x)
{
    return (_r(x, 2) ^ _r(x, 13) ^ _r(x, 22));
} // _S0

FN_ uint32_t _S1(uint32_t x)
{
    return (_r(x, 6) ^ _r(x, 11) ^ _r(x, 25));
} // _S1

FN_ uint32_t _G0(uint32_t x)
{
    return (_r(x, 7) ^ _r(x, 18) ^ (x >> 3));
} // _G0

FN_ uint32_t _G1(uint32_t x)
{
    return (_r(x, 17) ^ _r(x, 19) ^ (x >> 10));
} // _G1

FN_ uint32_t _word(uint8_t *c)
{
    return (_shw(c[0], 24) | _shw(c[1], 16) | _shw(c[2], 8) | (c[3]));
} // _word

static void _addbits(sha256_context *ctx, uint32_t n)
{
    __CPROVER_assume(__CPROVER_DYNAMIC_OBJECT(ctx));

    if (ctx->bits[0] > (0xffffffff - n)) {
        ctx->bits[1] = (ctx->bits[1] + 1) & 0xFFFFFFFF;
    }
    ctx->bits[0] = (ctx->bits[0] + n) & 0xFFFFFFFF;
} // _addbits

static void _hash(sha256_context *ctx)
{
    __CPROVER_assume(__CPROVER_DYNAMIC_OBJECT(ctx));

    register uint32_t a, b, c, d, e, f, g, h;
    uint32_t t[2];

    a = ctx->hash[0];
    b = ctx->hash[1];
    c = ctx->hash[2];
    d = ctx->hash[3];
    e = ctx->hash[4];
    f = ctx->hash[5];
    g = ctx->hash[6];
    h = ctx->hash[7];

    for (uint32_t i = 0; i < 64; i++) {
        if (i < 16) {
            ctx->W[i] = _word(&ctx->buf[_shw(i, 2)]);
        } else {
            ctx->W[i] = _G1(ctx->W[i - 2])  + ctx->W[i - 7] +
                        _G0(ctx->W[i - 15]) + ctx->W[i - 16];
        }

        t[0] = h + _S1(e) + _Ch(e, f, g) + K[i] + ctx->W[i];
        t[1] = _S0(a) + _Ma(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t[0];
        d = c;
        c = b;
        b = a;
        a = t[0] + t[1];
    }

    ctx->hash[0] += a;
    ctx->hash[1] += b;
    ctx->hash[2] += c;
    ctx->hash[3] += d;
    ctx->hash[4] += e;
    ctx->hash[5] += f;
    ctx->hash[6] += g;
    ctx->hash[7] += h;
} // _hash

void sha256_init(sha256_context *ctx)
{
    if (ctx != NULL) {
        ctx->bits[0] = ctx->bits[1] = ctx->len = 0;
        ctx->hash[0] = 0x6a09e667;
        ctx->hash[1] = 0xbb67ae85;
        ctx->hash[2] = 0x3c6ef372;
        ctx->hash[3] = 0xa54ff53a;
        ctx->hash[4] = 0x510e527f;
        ctx->hash[5] = 0x9b05688c;
        ctx->hash[6] = 0x1f83d9ab;
        ctx->hash[7] = 0x5be0cd19;
    }
} // sha256_init

void sha256_hash(sha256_context *ctx, const void *data, size_t len)
{
    const uint8_t *bytes = (const uint8_t *)data;

    if ((ctx != NULL) && (bytes != NULL) && (ctx->len < sizeof(ctx->buf))) {
        __CPROVER_assume(__CPROVER_DYNAMIC_OBJECT(bytes));
        __CPROVER_assume(__CPROVER_DYNAMIC_OBJECT(ctx));
        for (size_t i = 0; i < len; i++) {
            ctx->buf[ctx->len++] = bytes[i];
            if (ctx->len == sizeof(ctx->buf)) {
                _hash(ctx);
                _addbits(ctx, sizeof(ctx->buf) * 8);
                ctx->len = 0;
            }
        }
    }
} // sha256_hash

void sha256_done(sha256_context *ctx, uint8_t *hash)
{
    register uint32_t i, j;

    if (ctx != NULL) {
        j = ctx->len % sizeof(ctx->buf);
        ctx->buf[j] = 0x80;
        for (i = j + 1; i < sizeof(ctx->buf); i++) {
            ctx->buf[i] = 0x00;
        }

        if (ctx->len > 55) {
            _hash(ctx);
            for (j = 0; j < sizeof(ctx->buf); j++) {
                ctx->buf[j] = 0x00;
            }
        }

        _addbits(ctx, ctx->len * 8);
        ctx->buf[63] = _shb(ctx->bits[0],  0);
        ctx->buf[62] = _shb(ctx->bits[0],  8);
        ctx->buf[61] = _shb(ctx->bits[0], 16);
        ctx->buf[60] = _shb(ctx->bits[0], 24);
        ctx->buf[59] = _shb(ctx->bits[1],  0);
        ctx->buf[58] = _shb(ctx->bits[1],  8);
        ctx->buf[57] = _shb(ctx->bits[1], 16);
        ctx->buf[56] = _shb(ctx->bits[1], 24);
        _hash(ctx);

        if (hash != NULL) {
            for (i = 0, j = 24; i < 4; i++, j -= 8) {
                hash[i +  0] = _shb(ctx->hash[0], j);
                hash[i +  4] = _shb(ctx->hash[1], j);
                hash[i +  8] = _shb(ctx->hash[2], j);
                hash[i + 12] = _shb(ctx->hash[3], j);
                hash[i + 16] = _shb(ctx->hash[4], j);
                hash[i + 20] = _shb(ctx->hash[5], j);
                hash[i + 24] = _shb(ctx->hash[6], j);
                hash[i + 28] = _shb(ctx->hash[7], j);
            }
        }
    }
} // sha256_done

/***** COMMAND-LINE INTERFACE *****/

#define HASH_SIZE_BYTES 32
#define HASH_NAME "SHA-256"
const char* PROGRAM_NAME = "sha256";
const char* USAGE_MSG = "Invalid arguments. Correct usage: sha256 [-s] file-or-string\n";

static void sha256(const void *data, size_t len, uint8_t *hash)
{
    sha256_context ctx;

    sha256_init(&ctx);
    sha256_hash(&ctx, data, len);
    sha256_done(&ctx, hash);
}

/**
 * @brief Hashes a string or any byte array.
 * 
 * @param data A pointer to the first byte of the array of data to sign.
 * @param len The length of the data to sign.
 * @param hash A pointer to a chunk of memory to which the hash may be written.
 *  It is assumed that this chunk of memory is large enough to fit the bytes of
 *  the hash.
 */
static void hash_bytes(const void *data, size_t len, uint8_t *hash)
{
    return sha256(data, len, hash);
}

/**
 * @brief Hashes a file.
 * 
 * @param fd The file descriptor of the file to hash.
 * @param hash A pointer to a chunk of memory to which the hash may be written.
 *  It is assumed that this chunk of memory is large enough to fit the bytes of
 *  the hash.
 */
static void hash_file(int fd, uint8_t *hash)
{
    ssize_t n;
    char buf[BUFSIZ];
    sha256_context ctx;
    sha256_init(&ctx);
    while ((n = read(fd, buf, BUFSIZ)) > 0) {
        sha256_hash(&ctx, buf, n);
    }
    sha256_done(&ctx, &hash[0]);
}

static void print_hash (uint8_t *hash) {
    /* I chose to print out the SHA hash in groups of eight */
    for (int i = 0; i < 8; i++) {
        int j0 = i << 2; // This is the same thing as multiplying by 4.
        for (int j = 0; j < HASH_SIZE_BYTES / 8; j++) {
            printf("%02hhX", hash[j0 + j] & 0xff);
        }
        putchar(' ');
    }
}

static void print_output (uint8_t *hash, int is_file) {
    fputs("The ", stdout);
    fputs(HASH_NAME, stdout);
    fputs(" hash of the ", stdout);
    if (is_file) {
        puts("file is:");
    } else {
        puts("string is:");
    }
    print_hash(hash);
    putchar('\n');
}

int main(int argc, char *argv[]) {
    int fd;
    uint8_t hash[HASH_SIZE_BYTES];
    if (argc == 2) // "command file" format
    {
        // If the "file" is "-s", the user probably made an error.
        if (strcmp(argv[1], "-s") == 0) {
            write(STDERR_FILENO, USAGE_MSG, strlen(USAGE_MSG));
            return EXIT_FAILURE;
        }
        if ((fd = open(argv[1], O_RDONLY)) == -1) {
            perror(PROGRAM_NAME);
            return EXIT_FAILURE;
        }
        hash_file(fd, &hash[0]);
        print_output(&hash[0], 1); // 1 = We are printing a file's hash
        (void)close(fd);
    }
    else if (argc == 3) // "command -s string" format
    {
        if (strcmp(argv[1], "-s") != 0) {
            write(STDERR_FILENO, USAGE_MSG, strlen(USAGE_MSG));
            return EXIT_FAILURE;
        }
        hash_bytes(argv[2], strlen(argv[2]), &hash[0]);
        print_output(&hash[0], 0); // 0 = We are printing a string argument's hash
    }
    else
    {
        write(STDERR_FILENO, USAGE_MSG, strlen(USAGE_MSG));
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}