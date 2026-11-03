const assert = require("node:assert/strict");
const fs = require("node:fs");
const vm = require("node:vm");
const path = require("node:path");

const elements = new Map();
function element(id) {
  if (!elements.has(id)) {
    elements.set(id, {
      checked: false, hidden: false, value: "", style: {}, dataset: {},
      add() {}, removeAttribute() {}, closest() { return true; },
      addEventListener(event, callback) { this[event] = callback; },
    });
  }
  return elements.get(id);
}
const characterPanel = { hidden: true };
const document = {
  activeElement: null,
  getElementById: element,
  querySelector() { return characterPanel; },
  querySelectorAll() { return []; },
  documentElement: { style: { setProperty() {} } },
  body: { classList: { add() {}, remove() {} } },
  addEventListener() {},
};
let nextId = 0;
const frames = new Map();
const timers = new Map();
const calls = [];
let gamepadReads = 0;
const pad = { buttons: Array.from({ length: 16 }, () => ({ pressed: false })) };
const navigator = { getGamepads() { gamepadReads++; return [pad]; } };
const window = {
  innerWidth: 1920, innerHeight: 1080,
  addEventListener() {},
  setTimeout(callback) { callback(); },
  requestAnimationFrame(callback) { const id = ++nextId; frames.set(id, callback); return id; },
  cancelAnimationFrame(id) { frames.delete(id); },
  pm_command(value) { calls.push(value); },
};
function setIntervalMock(callback) { const id = ++nextId; timers.set(id, callback); return id; }
function clearIntervalMock(id) { timers.delete(id); }
function runFrame() {
  const [id, callback] = frames.entries().next().value;
  frames.delete(id);
  callback();
}
class Option {}
vm.runInNewContext(fs.readFileSync(path.join(__dirname, "../assets/views/PrismaPhotoMode/app.js"), "utf8"), {
  document, window, navigator, Option, setInterval: setIntervalMock, clearInterval: clearIntervalMock,
});

assert.equal(gamepadReads, 0);
assert.equal(frames.size, 0);
assert.equal(timers.size, 0);
window.pmSetState(JSON.stringify({ active: true, focused: false }));
assert.equal(frames.size, 0);
assert.equal(timers.size, 1);
let clicks = 0;
document.activeElement = Object.assign(element("focus-btn"), { tagName: "BUTTON", click() { clicks++; } });
pad.buttons[0].pressed = true;
window.pmSetState(JSON.stringify({ active: true, focused: true }));
window.pmSetState(JSON.stringify({ active: true, focused: true }));
assert.equal(frames.size, 1);
runFrame();
assert.equal(clicks, 0);
pad.buttons[0].pressed = false;
runFrame();
pad.buttons[0].pressed = true;
runFrame();
assert.equal(clicks, 1);
window.pmSetState(JSON.stringify({ active: true, focused: false }));
assert.equal(frames.size, 0);
window.pmSetState(JSON.stringify({ active: true, focused: true }));
runFrame();
runFrame();
assert.equal(clicks, 1);
pad.buttons[0].pressed = false;
runFrame();
pad.buttons[0].pressed = true;
runFrame();
assert.equal(clicks, 2);
window.pmSetState(JSON.stringify({ active: true, focused: false }));
assert.equal(frames.size, 0);

calls.length = 0;
element("dof-auto").checked = true;
element("fx-dof-toggle").checked = false;
timers.values().next().value();
assert.deepEqual(calls, []);
element("fx-dof-toggle").checked = true;
characterPanel.hidden = false;
timers.values().next().value();
assert.deepEqual(calls, ["autofocus|1", "pick|1"]);
window.pmSetState(JSON.stringify({ active: false, focused: false }));
assert.equal(timers.size, 0);
assert.equal(frames.size, 0);
calls.length = 0;
element("frame-category").value = "Fallout frames";
element("frame-name").value = "vault 111.png";
element("screenshot-btn").click();
assert.deepEqual(calls, ["focus|0", "screenshot|Fallout%20frames|vault%20111.png"]);
console.log("PrismaPhotoMode UI polling and screenshot command checks PASS");
