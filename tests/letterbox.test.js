const assert = require("node:assert/strict");
const fs = require("node:fs");
const vm = require("node:vm");
const path = require("node:path");

const elements = new Map();
function element(id) {
  if (!elements.has(id)) {
    elements.set(id, {
      checked: false, hidden: false, value: "", style: {},
      addEventListener(event, callback) { this[event] = callback; },
    });
  }
  return elements.get(id);
}
const document = {
  getElementById: element,
  documentElement: { style: { setProperty() {} } },
  addEventListener() {},
};
const window = { innerWidth: 1920, innerHeight: 1080, addEventListener() {} };
vm.runInNewContext(fs.readFileSync(path.join(__dirname, "../assets/views/PrismaPhotoMode/app.js"), "utf8"), { document, window });

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
console.log("PrismaPhotoMode letterbox checks PASS");
