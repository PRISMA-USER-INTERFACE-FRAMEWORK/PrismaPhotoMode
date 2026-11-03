const assert = require("node:assert/strict");
const fs = require("node:fs");
const vm = require("node:vm");
const path = require("node:path");

const elements = new Map();
function element(id) {
  if (!elements.has(id)) {
    elements.set(id, {
      checked: false, hidden: false, value: "", style: {}, dataset: {},
      add() {}, removeAttribute() {},
      addEventListener(event, callback) { this[event] = callback; },
    });
  }
  return elements.get(id);
}
const properties = new Map();
const document = {
  getElementById: element,
  querySelectorAll() { return []; },
  documentElement: { style: { setProperty(name, value) { properties.set(name, value); } } },
  addEventListener() {},
};
const window = { innerWidth: 1920, innerHeight: 1080, addEventListener(event, callback) { this[event] = callback; }, requestAnimationFrame() {} };
const navigator = { getGamepads() { return []; } };
class Option { constructor(text, value) { this.text = text; this.value = value; } }
vm.runInNewContext(fs.readFileSync(path.join(__dirname, "../assets/views/PrismaPhotoMode/app.js"), "utf8"), { document, window, navigator, Option, setInterval() {} });

element("fx-letterbox-toggle").checked = true;
element("letterbox-aspect").value = "2.39";
element("fx-letterbox-toggle").input();
for (const id of ["fx-letterbox-top", "fx-letterbox-bottom"]) {
  assert.equal(element(id).hidden, false);
  assert.equal(element(id).style.height, "138px");
  assert.notEqual(element(id).style.width, "0px");
}
element("letterbox-aspect").value = "1.33";
element("letterbox-aspect").input();
for (const id of ["fx-letterbox-left", "fx-letterbox-right"]) {
  assert.equal(element(id).hidden, false);
  assert.equal(element(id).style.width, "242px");
  assert.notEqual(element(id).style.height, "0px");
}
assert.equal(element("fx-letterbox-top").hidden, true);
element("fx-letterbox-toggle").checked = false;
element("fx-letterbox-toggle").input();
assert.equal(element("fx-letterbox-left").hidden, true);
element("panel-scale").input({ target: { value: "200" } });
assert.equal(properties.get("--panel-max-height"), "516px");
assert.equal(properties.get("--panel-max-width"), "936px");
window.innerHeight = 768;
window.innerWidth = 320;
window.resize();
assert.equal(properties.get("--panel-max-width"), "280px");
assert.ok(Number(properties.get("--panel-scale")) < 1);
assert.match(element("panel-scale-out").textContent, /fit/);
const options = [];
element("frame-name").add = (option) => options.push(option);
window.pmSetOverlays(JSON.stringify({ frames: [{ name: "Fallout", overlays: ["greetings_from_the_commonwealth.png", "vault_111.PNG"] }], textures: [] }));
element("frame-category").value = "Fallout";
element("frame-category").change();
assert.deepEqual(options.slice(-2).map(({ text, value }) => [text, value]), [
  ["Greetings From The Commonwealth", "greetings_from_the_commonwealth.png"],
  ["Vault 111", "vault_111.PNG"],
]);
console.log("PrismaPhotoMode letterbox and frame checks PASS");
