"use strict";

const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const vm = require("node:vm");

const source = fs.readFileSync(path.join(__dirname, "../assets/views/PrismaPhotoMode/preview.js"), "utf8");
const listings = {
  "overlays/frames/": ["../", "Fallout/"],
  "overlays/frames/Fallout/": ["../", "vault_111.png", "Space%20Frame.PNG", "notes.txt"],
};

async function run(hostname) {
  const calls = [];
  const window = { location: { protocol: "http:", hostname }, pmSetOverlays: (payload) => calls.push(JSON.parse(payload)) };
  class DOMParser {
    parseFromString(text) {
      return { querySelectorAll: () => JSON.parse(text).map((href) => ({ getAttribute: () => href })) };
    }
  }
  const fetch = async (url) => ({ ok: Object.hasOwn(listings, url), text: async () => JSON.stringify(listings[url]) });
  vm.runInNewContext(source, { window, fetch, DOMParser, console });
  await new Promise((resolve) => setImmediate(resolve));
  return calls;
}

(async () => {
  for (const host of ["127.0.0.1", "localhost"]) {
    const result = await run(host);
    assert.deepEqual(result, [{ frames: [{ name: "Fallout", overlays: ["vault_111.png", "Space Frame.PNG"] }], textures: [] }]);
  }
  assert.deepEqual(await run("example.com"), []);
  console.log("PrismaPhotoMode preview overlay checks PASS");
})().catch((error) => { console.error(error); process.exitCode = 1; });
