// sha3.c
// 19-Nov-11  Markku-Juhani O. Saarinen <mjos@iki.fi>

// Revised 07-Aug-15 to match with official release of FIPS PUB 202 "SHA3"
// Revised 03-Sep-15 for portability + OpenSSL - style API

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#ifndef KECCAKF_ROUNDS
#define KECCAKF_ROUNDS 24
#endif

#ifndef ROTL64
#define ROTL64(x, y) (((x) << (y)) | ((x) >> (64 - (y))))
#endif

// state context
typedef struct {
    union {                                 // state:
        uint8_t b[200];                     // 8-bit bytes
        uint64_t q[25];                     // 64-bit words
    } st;
    int pt, rsiz, mdlen;                    // these don't overflow
} sha3_ctx_t;

// Compression function.
void sha3_keccakf(uint64_t st[25]);

// OpenSSL - like interfece
int sha3_init(sha3_ctx_t *c, int mdlen);    // mdlen = hash output in bytes
int sha3_update(sha3_ctx_t *c, const void *data, size_t len);
int sha3_final(void *md, sha3_ctx_t *c);    // digest goes to md

// compute a sha3 hash (md) of given byte length from "in"
void *sha3(const void *in, size_t inlen, void *md, int mdlen);

// SHAKE128 and SHAKE256 extensible-output functions
#define shake128_init(c) sha3_init(c, 16)
#define shake256_init(c) sha3_init(c, 32)
#define shake_update sha3_update

void shake_xof(sha3_ctx_t *c);
void shake_out(sha3_ctx_t *c, void *out, size_t len);

// update the state with given number of rounds

void sha3_keccakf(uint64_t st[25])
{
    // constants
    const uint64_t keccakf_rndc[24] = {
        0x0000000000000001, 0x0000000000008082, 0x800000000000808a,
        0x8000000080008000, 0x000000000000808b, 0x0000000080000001,
        0x8000000080008081, 0x8000000000008009, 0x000000000000008a,
        0x0000000000000088, 0x0000000080008009, 0x000000008000000a,
        0x000000008000808b, 0x800000000000008b, 0x8000000000008089,
        0x8000000000008003, 0x8000000000008002, 0x8000000000000080,
        0x000000000000800a, 0x800000008000000a, 0x8000000080008081,
        0x8000000000008080, 0x0000000080000001, 0x8000000080008008
    };
    const int keccakf_rotc[24] = {
        1,  3,  6,  10, 15, 21, 28, 36, 45, 55, 2,  14,
        27, 41, 56, 8,  25, 43, 62, 18, 39, 61, 20, 44
    };
    const int keccakf_piln[24] = {
        10, 7,  11, 17, 18, 3, 5,  16, 8,  21, 24, 4,
        15, 23, 19, 13, 12, 2, 20, 14, 22, 9,  6,  1
    };

    // variables
    int i, j, r;
    uint64_t t, bc[5];

#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
    uint8_t *v;

    // endianess conversion. this is redundant on little-endian targets
    for (i = 0; i < 25; i++) {
        v = (uint8_t *) &st[i];
        st[i] = ((uint64_t) v[0])     | (((uint64_t) v[1]) << 8) |
            (((uint64_t) v[2]) << 16) | (((uint64_t) v[3]) << 24) |
            (((uint64_t) v[4]) << 32) | (((uint64_t) v[5]) << 40) |
            (((uint64_t) v[6]) << 48) | (((uint64_t) v[7]) << 56);
    }
#endif

    // actual iteration
    for (r = 0; r < KECCAKF_ROUNDS; r++) {

        // Theta
        for (i = 0; i < 5; i++)
            bc[i] = st[i] ^ st[i + 5] ^ st[i + 10] ^ st[i + 15] ^ st[i + 20];

        for (i = 0; i < 5; i++) {
            t = bc[(i + 4) % 5] ^ ROTL64(bc[(i + 1) % 5], 1);
            for (j = 0; j < 25; j += 5)
                st[j + i] ^= t;
        }

        // Rho Pi
        t = st[1];
        for (i = 0; i < 24; i++) {
            j = keccakf_piln[i];
            bc[0] = st[j];
            st[j] = ROTL64(t, keccakf_rotc[i]);
            t = bc[0];
        }

        //  Chi
        for (j = 0; j < 25; j += 5) {
            for (i = 0; i < 5; i++)
                bc[i] = st[j + i];
            for (i = 0; i < 5; i++)
                st[j + i] ^= (~bc[(i + 1) % 5]) & bc[(i + 2) % 5];
        }

        //  Iota
        st[0] ^= keccakf_rndc[r];
    }

#if __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
    // endianess conversion. this is redundant on little-endian targets
    for (i = 0; i < 25; i++) {
        v = (uint8_t *) &st[i];
        t = st[i];
        v[0] = t & 0xFF;
        v[1] = (t >> 8) & 0xFF;
        v[2] = (t >> 16) & 0xFF;
        v[3] = (t >> 24) & 0xFF;
        v[4] = (t >> 32) & 0xFF;
        v[5] = (t >> 40) & 0xFF;
        v[6] = (t >> 48) & 0xFF;
        v[7] = (t >> 56) & 0xFF;
    }
#endif
}

// Initialize the context for SHA3

int sha3_init(sha3_ctx_t *c, int mdlen)
{
    int i;

    for (i = 0; i < 25; i++)
        c->st.q[i] = 0;
    c->mdlen = mdlen;
    c->rsiz = 200 - 2 * mdlen;
    c->pt = 0;

    return 1;
}

// update state with more data

int sha3_update(sha3_ctx_t *c, const void *data, size_t len)
{
    size_t i;
    int j;

    j = c->pt;
    for (i = 0; i < len; i++) {
        c->st.b[j++] ^= ((const uint8_t *) data)[i];
        if (j >= c->rsiz) {
            sha3_keccakf(c->st.q);
            j = 0;
        }
    }
    c->pt = j;

    return 1;
}

// finalize and output a hash

int sha3_final(void *md, sha3_ctx_t *c)
{
    int i;

    c->st.b[c->pt] ^= 0x06;
    c->st.b[c->rsiz - 1] ^= 0x80;
    sha3_keccakf(c->st.q);

    for (i = 0; i < c->mdlen; i++) {
        ((uint8_t *) md)[i] = c->st.b[i];
    }

    return 1;
}

// compute a SHA-3 hash (md) of given byte length from "in"

void *sha3(const void *in, size_t inlen, void *md, int mdlen)
{
    sha3_ctx_t sha3;

    sha3_init(&sha3, mdlen);
    sha3_update(&sha3, in, inlen);
    sha3_final(md, &sha3);

    return md;
}

// SHAKE128 and SHAKE256 extensible-output functionality

void shake_xof(sha3_ctx_t *c)
{
    c->st.b[c->pt] ^= 0x1F;
    c->st.b[c->rsiz - 1] ^= 0x80;
    sha3_keccakf(c->st.q);
    c->pt = 0;
}

void shake_out(sha3_ctx_t *c, void *out, size_t len)
{
    size_t i;
    int j;

    j = c->pt;
    for (i = 0; i < len; i++) {
        if (j >= c->rsiz) {
            sha3_keccakf(c->st.q);
            j = 0;
        }
        ((uint8_t *) out)[i] = c->st.b[j++];
    }
    c->pt = j;
}

/***** COMMAND-LINE INTERFACE *****/

#define HASH_SIZE_BYTES 64
#define HASH_NAME "SHA3-512"
const char* PROGRAM_NAME = "sha3";
const char* USAGE_MSG = "Invalid arguments. Correct usage: sha3 [-s] file-or-string\n";

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
    sha3(data, len, &hash[0], HASH_SIZE_BYTES);
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
    sha3_ctx_t sha3_ctx;
    sha3_init(&sha3_ctx, HASH_SIZE_BYTES);
    while ((n = read(fd, buf, BUFSIZ)) > 0) {
        sha3_update(&sha3_ctx, buf, n);
    }
    sha3_final(hash, &sha3_ctx);
}

static void print_hash (uint8_t *hash) {
    for (int i = 0; i < HASH_SIZE_BYTES / 4; i++) {
        int j0 = i << 2; // This is the same thing as multiplying by 4.
        for (int j = 0; j < 4; j++) {
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