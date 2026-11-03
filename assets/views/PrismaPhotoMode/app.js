(() => {
  "use strict";

  const SHOT_SETTLE_MS = 180;
  const SHOT_HOLD_MS = 700;
  const DEFAULTS = {
    grid: false,
    gridKind: "thirds",
    vignette: { on: false, strength: 45 },
    letterbox: { on: false, aspect: "2.39" },
    tint: { on: false, color: "#c08040", opacity: 20 },
    grain: { on: false, amount: 35 },
    frame: { on: false, width: 40 },
    dof: { on: false, distance: 500, auto: false },
  };

  const $ = (id) => document.getElementById(id);
  const state = { focused: false, shooting: false, active: false, panelScaleDefault: 100, overlays: [] };

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
    $("fx-grid").dataset.kind = $("grid-kind").value;
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
      bar.style.width = "0px";
      bar.style.height = "0px";
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
    $("grid-kind").value = DEFAULTS.gridKind;
    $("overlay-category").value = "";
    setOverlayNames("");
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
    $("fx-dof-toggle").checked = DEFAULTS.dof.on;
    $("dof-distance").value = DEFAULTS.dof.distance;
    $("dof-distance-out").textContent = DEFAULTS.dof.distance;
    $("dof-auto").checked = DEFAULTS.dof.auto;
    command("dof|0");
    command("autofocus|0");
    $("filter-select").value = "";
    $("filter-strength").value = 1;
    $("filter-strength-out").textContent = "1.00";
    command("filter|0");
    applyEffects();
  }

  function setFocusUi(focused) {
    state.focused = focused;
    $("focus-state").textContent = focused ? "Panel has mouse" : "Camera has input";
    $("focus-btn").textContent = focused ? "Release to camera" : "Take mouse";
  }

  function applyPanelScale(percent) {
    $("panel-scale").value = percent;
    $("panel-scale-out").textContent = percent;
    document.documentElement.style.setProperty("--panel-scale", String(percent / 100));
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

  const TIME_CONTROLS = ["timemult", "hour", "timescale", "weather-select", "weather-clear-btn"];

  function formatHour(value) {
    const hours = Math.floor(value);
    const minutes = Math.min(59, Math.round((value - hours) * 60));
    return `${String(hours).padStart(2, "0")}:${String(minutes).padStart(2, "0")}`;
  }

  function setTimeEnabled(enabled) {
    TIME_CONTROLS.forEach((id) => {
      $(id).disabled = !enabled;
    });
  }

  function setWeatherOptions(weathers) {
    const select = $("weather-select");
    const current = select.value;
    select.textContent = "";
    select.add(new Option("Current", ""));
    weathers.forEach((weather) => select.add(new Option(weather.name, String(weather.id))));
    select.value = current;
  }

  window.pmSetState = (payload) => {
    let data;
    try {
      data = JSON.parse(payload);
    } catch (error) {
      console.error("pmSetState: bad payload", error);
      return;
    }
    const leaving = state.active && !data.active;
    state.active = Boolean(data.active);
    if (typeof data.panelScale === "number") state.panelScaleDefault = data.panelScale;
    if (leaving) resetEffects();
    if (!state.active) applyPanelScale(state.panelScaleDefault);
    setFocusUi(Boolean(data.focused));
    if (typeof data.fov === "number") syncSlider("fov", "fov-out", data.fov, 0);
    if (typeof data.speed === "number") syncSlider("speed", "speed-out", data.speed, 2);
    if (typeof data.frozen === "boolean" && document.activeElement !== $("time-freeze")) $("time-freeze").checked = data.frozen;
    setTimeEnabled(Boolean(data.timeReady));
    if (typeof data.timeMult === "number") syncSlider("timemult", "timemult-out", data.timeMult, 2);
    if (typeof data.timescale === "number") syncSlider("timescale", "timescale-out", data.timescale, 0);
    if (typeof data.hour === "number") {
      if (document.activeElement !== $("hour")) $("hour").value = data.hour;
      $("hour-out").textContent = formatHour(data.hour);
    }
  };

  function setOverlayNames(categoryName) {
    const select = $("overlay-name");
    select.textContent = "";
    select.add(new Option("None", ""));
    const category = state.overlays.find((entry) => entry.name === categoryName);
    if (category) category.overlays.forEach((name) => select.add(new Option(name, name)));
    select.value = "";
    applyOverlay();
  }

  function setOverlayCategories(categories) {
    const select = $("overlay-category");
    select.textContent = "";
    select.add(new Option("None", ""));
    categories.forEach((category) => select.add(new Option(category.name, category.name)));
    state.overlays = categories;
    select.value = "";
    setOverlayNames("");
  }

  function applyOverlay() {
    const category = $("overlay-category").value;
    const name = $("overlay-name").value;
    const overlay = $("fx-overlay");
    if (!category || !name) {
      overlay.hidden = true;
      overlay.removeAttribute("src");
      return;
    }
    overlay.src = `overlays/${encodeURIComponent(category)}/${encodeURIComponent(name)}`;
    overlay.hidden = false;
  }

  window.pmSetOverlays = (payload) => {
    let data;
    try {
      data = JSON.parse(payload);
    } catch (error) {
      console.error("pmSetOverlays: bad payload", error);
      return;
    }
    setOverlayCategories(Array.isArray(data.categories) ? data.categories : []);
  };

  window.pmSetLists = (payload) => {
    let data;
    try {
      data = JSON.parse(payload);
    } catch (error) {
      console.error("pmSetLists: bad payload", error);
      return;
    }
    setWeatherOptions(Array.isArray(data.weathers) ? data.weathers : []);
  };

  function fillSelect(select, items) {
    const current = select.value;
    select.textContent = "";
    select.add(new Option("None", ""));
    items.forEach((item) => select.add(new Option(item.name, String(item.id))));
    select.value = current;
  }

  window.pmSetFilters = (payload) => {
    let data;
    try {
      data = JSON.parse(payload);
    } catch (error) {
      console.error("pmSetFilters: bad payload", error);
      return;
    }
    fillSelect($("filter-select"), Array.isArray(data.filters) ? data.filters : []);
  };

  window.pmSetCharacters = (payload) => {
    let data;
    try {
      data = JSON.parse(payload);
    } catch (error) {
      console.error("pmSetCharacters: bad payload", error);
      return;
    }
    fillSelect($("idle-select"), Array.isArray(data.idles) ? data.idles : []);
    fillSelect($("art-select"), Array.isArray(data.arts) ? data.arts : []);
    fillSelect($("shader-select"), Array.isArray(data.shaders) ? data.shaders : []);
  };

  window.pmSetPick = (payload) => {
    let data;
    try {
      data = JSON.parse(payload);
    } catch (error) {
      console.error("pmSetPick: bad payload", error);
      return;
    }
    $("pick-name").textContent = data.name || "No reference picked";
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
    "grid-kind",
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

  $("time-freeze").addEventListener("change", (event) => command(`freeze|${event.target.checked ? 1 : 0}`));
  $("timemult").addEventListener("input", (event) => {
    $("timemult-out").textContent = Number(event.target.value).toFixed(2);
    command(`timemult|${event.target.value}`);
  });
  $("hour").addEventListener("input", (event) => {
    $("hour-out").textContent = formatHour(Number(event.target.value));
    command(`hour|${event.target.value}`);
  });
  $("timescale").addEventListener("input", (event) => {
    $("timescale-out").textContent = event.target.value;
    command(`timescale|${event.target.value}`);
  });
  $("weather-select").addEventListener("change", (event) => command(`weather|${event.target.value}`));
  $("overlay-category").addEventListener("change", (event) => setOverlayNames(event.target.value));
  $("overlay-name").addEventListener("change", applyOverlay);
  $("filter-select").addEventListener("change", (event) => command(`filter|${event.target.value || 0}`));
  $("filter-strength").addEventListener("input", (event) => {
    $("filter-strength-out").textContent = Number(event.target.value).toFixed(2);
    command(`filterstrength|${event.target.value}`);
  });
  $("freeze-btn").addEventListener("click", () => command("freeze|1"));
  $("release-btn").addEventListener("click", () => command("freeze|0"));
  $("face-btn").addEventListener("click", () => command("face|1"));
  $("idle-btn").addEventListener("click", () => command(`idle|${$("idle-select").value || 0}`));
  $("art-btn").addEventListener("click", () => command(`art|${$("art-select").value || 0}`));
  $("shader-btn").addEventListener("click", () => command(`shader|${$("shader-select").value || 0}`));
  $("weather-clear-btn").addEventListener("click", () => {
    $("weather-select").value = "";
    command("weather|0");
  });

  document.querySelectorAll(".tab").forEach((tab) => {
    tab.addEventListener("click", () => {
      document.querySelectorAll(".tab").forEach((other) => other.setAttribute("aria-selected", String(other === tab)));
      document.querySelectorAll("[data-tab-panel]").forEach((panel) => {
        panel.hidden = panel.dataset.tabPanel !== tab.dataset.tab;
      });
    });
  });

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
    if (event.key === "ArrowUp" || event.key === "ArrowDown") {
      event.preventDefault();
      moveFocus(event.key === "ArrowUp" ? -1 : 1);
      return;
    }
    if (event.key !== "Escape" && event.key !== "Tab") return;
    event.preventDefault();
    if (!event.repeat) command("focus|0");
  });

  function focusable() {
    return Array.from(document.querySelectorAll("#panel button, #panel input, #panel select"))
      .filter((el) => !el.disabled && el.getClientRects().length > 0);
  }

  function moveFocus(step) {
    const items = focusable();
    if (!items.length) return;
    const current = items.indexOf(document.activeElement);
    let next = step > 0 ? 0 : items.length - 1;
    if (current >= 0) next = (current + step + items.length) % items.length;
    items[next].focus();
  }

  function focusedInPanel() {
    const el = document.activeElement;
    return el && el.closest && el.closest("#panel") ? el : null;
  }

  function stepFocused(direction) {
    const el = focusedInPanel();
    if (!el) return;
    if (el.tagName === "INPUT" && el.type === "range") {
      if (direction > 0) {
        el.stepUp();
      } else {
        el.stepDown();
      }
    } else if (el.tagName === "SELECT") {
      el.selectedIndex = Math.max(0, Math.min(el.options.length - 1, el.selectedIndex + direction));
    } else {
      return;
    }
    el.dispatchEvent(new Event("input"));
    el.dispatchEvent(new Event("change"));
  }

  function activateFocused() {
    const el = focusedInPanel();
    if (el && (el.tagName === "BUTTON" || el.type === "checkbox")) el.click();
  }

  function switchTab(direction) {
    const tabs = Array.from(document.querySelectorAll(".tab"));
    const current = tabs.findIndex((tab) => tab.getAttribute("aria-selected") === "true");
    tabs[(current + direction + tabs.length) % tabs.length].click();
  }

  const PAD = { a: 0, b: 1, lb: 4, rb: 5, up: 12, down: 13, left: 14, right: 15 };
  const padHeld = [];

  function padEdge(pad, index) {
    const now = Boolean(pad?.buttons?.[index]?.pressed);
    const was = padHeld[index];
    padHeld[index] = now;
    return now && !was;
  }

  function pollGamepad() {
    const pad = Array.from(navigator.getGamepads?.() ?? []).find(Boolean);
    const up = padEdge(pad, PAD.up);
    const down = padEdge(pad, PAD.down);
    const left = padEdge(pad, PAD.left);
    const right = padEdge(pad, PAD.right);
    const a = padEdge(pad, PAD.a);
    const b = padEdge(pad, PAD.b);
    const lb = padEdge(pad, PAD.lb);
    const rb = padEdge(pad, PAD.rb);
    if (state.focused && !state.shooting) {
      if (up) moveFocus(-1);
      if (down) moveFocus(1);
      if (left) stepFocused(-1);
      if (right) stepFocused(1);
      if (a) activateFocused();
      if (b) command("focus|0");
      if (lb) switchTab(-1);
      if (rb) switchTab(1);
    }
    window.requestAnimationFrame(pollGamepad);
  }

  $("fx-dof-toggle").addEventListener("change", (event) => command(`dof|${event.target.checked ? 1 : 0}`));
  $("dof-distance").addEventListener("input", (event) => {
    $("dof-distance-out").textContent = event.target.value;
    command(`dofdist|${event.target.value}`);
  });
  $("dof-auto").addEventListener("change", (event) => command(`autofocus|${event.target.checked ? 1 : 0}`));
  setInterval(() => {
    if (state.active && $("dof-auto").checked) command("autofocus|1");
    if (state.active && !document.querySelector('[data-tab-panel="characters"]').hidden) command("pick|1");
  }, 250);

  $("panel-scale").addEventListener("input", (event) => applyPanelScale(Number(event.target.value)));

  window.addEventListener("resize", applyLetterbox);
  resetEffects();
  pollGamepad();
})();
