#!/usr/bin/env node
import { readFile } from "node:fs/promises";
import { strict as assert } from "node:assert";
import { describe, it } from "node:test";
import { execSync } from "node:child_process";

const PLAINTEXT_FILE_NAME = "README.md";

describe("enc", () => {
    // This test depends on you manually encrypting the README.md with enc.c first.
    // It also depends upon openssl being installed.
    it("Should produce decryptable output", async () => {
        const plaintext = await readFile(PLAINTEXT_FILE_NAME);
        const key = await readFile(`${PLAINTEXT_FILE_NAME}.enc.key`);
        const iv = await readFile(`${PLAINTEXT_FILE_NAME}.enc.iv`);
        const cmd = `openssl enc -d -aes-128-cbc -iv '${iv.toString("hex")}' -in ./${PLAINTEXT_FILE_NAME}.enc -K '${key.toString("hex")}'`;
        const out = execSync(cmd, { maxBuffer: 1_000_000 });
        assert.deepEqual(out, plaintext);
    });  
}); 