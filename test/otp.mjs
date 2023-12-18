#!/usr/bin/env node
import { readFile } from "node:fs/promises";
import { strict as assert } from "node:assert";
import { describe, it } from "node:test";

describe("OTP", () => {
    // This test depends on you manually encrypting the README.md with otp.c first.
    it("Should produce decryptable output", async () => {
        const plaintext = await readFile("README.md", { encoding: "utf-8" });
        const enc = await readFile("README.md.enc");
        const key = await readFile("README.md.otpkey");
        assert(key.length >= enc.length);
        const len = Math.min(key.length, enc.length);
        const dec = Buffer.allocUnsafe(len);
        for (let i = 0; i < len; i++) {
            dec[i] = enc[i] ^ key[i]; // XORing the key with the encrypted output reverses the OTP encryption.
        }
        const decrypted = dec.toString("utf-8");
        assert.equal(decrypted, plaintext);
    });  
}); 