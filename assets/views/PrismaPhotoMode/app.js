(() => {
  "use strict";

  const SHOT_SETTLE_MS = 180;
  const SHOT_HOLD_MS = 700;
  const DEFAULTS = {
    grid: false,
    vignette: { on: false, strength: 45 },
    letterbox: { on: false, aspect: "2.39" },
    tint: { on: false, color: "#c08040", opacity: 20 },
    grain: { on: false, amount: 35 },
    frame: { on: false, width: 40 },
  };

  const $ = (id) => document.getElementById(id);
  const state = { focused: false, shooting: false };

  function nativeCall(name, arg = "") {
    const fn = window[name];
    if (typeof fn !== "function") return false;
    try {
      fn(String(arg));
      return true;
    } catch (error) {
      console.error(`${name} failed`, error);
      return false;
    }
  }

  function command(text) {
    nativeCall("pm_command", text);
  }

  function setHidden(id, hidden) {
    $(id).hidden = hidden;
  }

  function applyEffects() {
    setHidden("fx-grid", !$("fx-grid-toggle").checked);

    const vignetteOn = $("fx-vignette-toggle").checked;
    $("vignette-out").textContent = $("vignette-strength").value;
    document.documentElement.style.setProperty("--vignette", String($("vignette-strength").value / 100));
    setHidden("fx-vignette", !vignetteOn);

    $("tint-out").textContent = $("tint-opacity").value;
    document.documentElement.style.setProperty("--tint", $("tint-color").value);
    document.documentElement.style.setProperty("--tint-alpha", String($("tint-opacity").value / 100));
    setHidden("fx-tint", !$("fx-tint-toggle").checked);

    $("grain-out").textContent = $("grain-amount").value;
    document.documentElement.style.setProperty("--grain", String($("grain-amount").value / 100));
    setHidden("fx-grain", !$("fx-grain-toggle").checked);

    $("frame-out").textContent = $("frame-width").value;
    document.documentElement.style.setProperty("--frame", `${$("frame-width").value}px`);
    setHidden("fx-frame", !$("fx-frame-toggle").checked);

    applyLetterbox();
  }

  function applyLetterbox() {
    const enabled = $("fx-letterbox-toggle").checked;
    const target = Number($("letterbox-aspect").value);
    const width = window.innerWidth;
    const height = window.innerHeight;
    const top = $("fx-letterbox-top");
    const bottom = $("fx-letterbox-bottom");
    const left = $("fx-letterbox-left");
    const right = $("fx-letterbox-right");
    [top, bottom, left, right].forEach((bar) => {
      bar.hidden = true;
    });
    if (!enabled) return;
    if (target > width / height) {
      const bar = Math.round((height - width / target) / 2);
      [top, bottom].forEach((el) => {
        el.style.height = `${bar}px`;
        el.hidden = false;
      });
    } else {
      const bar = Math.round((width - height * target) / 2);
      [left, right].forEach((el) => {
        el.style.width = `${bar}px`;
        el.hidden = false;
      });
    }
  }

  function resetEffects() {
    $("fx-grid-toggle").checked = DEFAULTS.grid;
    $("fx-vignette-toggle").checked = DEFAULTS.vignette.on;
    $("vignette-strength").value = DEFAULTS.vignette.strength;
    $("fx-letterbox-toggle").checked = DEFAULTS.letterbox.on;
    $("letterbox-aspect").value = DEFAULTS.letterbox.aspect;
    $("fx-tint-toggle").checked = DEFAULTS.tint.on;
    $("tint-color").value = DEFAULTS.tint.color;
    $("tint-opacity").value = DEFAULTS.tint.opacity;
    $("fx-grain-toggle").checked = DEFAULTS.grain.on;
    $("grain-amount").value = DEFAULTS.grain.amount;
    $("fx-frame-toggle").checked = DEFAULTS.frame.on;
    $("frame-width").value = DEFAULTS.frame.width;
    applyEffects();
  }

  function setFocusUi(focused) {
    state.focused = focused;
    $("focus-state").textContent = focused ? "Panel has mouse" : "Camera has input";
    $("focus-btn").textContent = focused ? "Release to camera" : "Take mouse";
  }

  function toggleFocus() {
    command(state.focused ? "focus|0" : "focus|1");
  }

  function takeScreenshot() {
    if (state.shooting) return;
    state.shooting = true;
    document.body.classList.add("shooting");
    window.setTimeout(() => {
      command("focus|0");
      command("screenshot|");
      window.setTimeout(() => {
        document.body.classList.remove("shooting");
        state.shooting = false;
      }, SHOT_HOLD_MS);
    }, SHOT_SETTLE_MS);
  }

  function syncSlider(id, outId, value, digits) {
    const el = $(id);
    if (document.activeElement !== el) el.value = value;
    $(outId).textContent = Number(value).toFixed(digits);
  }

  window.pmSetState = (payload) => {
    let data;
    try {
      data = JSON.parse(payload);
    } catch (error) {
      console.error("pmSetState: bad payload", error);
      return;
    }
    setFocusUi(Boolean(data.focused));
    if (typeof data.fov === "number") syncSlider("fov", "fov-out", data.fov, 0);
    if (typeof data.speed === "number") syncSlider("speed", "speed-out", data.speed, 2);
  };

  $("fov").addEventListener("input", (event) => {
    $("fov-out").textContent = event.target.value;
    command(`fov|${event.target.value}`);
  });
  $("speed").addEventListener("input", (event) => {
    $("speed-out").textContent = Number(event.target.value).toFixed(2);
    command(`speed|${event.target.value}`);
  });

  [
    "fx-grid-toggle",
    "fx-vignette-toggle",
    "vignette-strength",
    "fx-letterbox-toggle",
    "letterbox-aspect",
    "fx-tint-toggle",
    "tint-color",
    "tint-opacity",
    "fx-grain-toggle",
    "grain-amount",
    "fx-frame-toggle",
    "frame-width",
  ].forEach((id) => $(id).addEventListener("input", applyEffects));

  $("focus-btn").addEventListener("click", toggleFocus);
  $("screenshot-btn").addEventListener("click", takeScreenshot);
  $("reset-btn").addEventListener("click", () => command("reset|"));
  $("reset-effects-btn").addEventListener("click", resetEffects);
  $("exit-btn").addEventListener("click", () => command("exit|"));
  $("hide-panel-btn").addEventListener("click", () => {
    $("panel").hidden = true;
    $("show-panel").hidden = false;
  });
  $("show-panel").addEventListener("click", () => {
    $("panel").hidden = false;
    $("show-panel").hidden = true;
  });

  document.addEventListener("keydown", (event) => {
    if (!state.focused) return;
    if (event.key !== "Escape" && event.key !== "Tab") return;
    event.preventDefault();
    if (!event.repeat) command("focus|0");
  });

  window.addEventListener("resize", applyLetterbox);
  resetEffects();
})();
