(() => {
  "use strict";

  if (window.location.protocol !== "http:" || !(["127.0.0.1", "localhost"].includes(window.location.hostname))) return;

  async function listing(path) {
    const response = await fetch(path);
    if (!response.ok) return [];
    const html = new DOMParser().parseFromString(await response.text(), "text/html");
    return Array.from(html.querySelectorAll("a[href]"), (link) => link.getAttribute("href"))
      .filter((href) => href && href !== "../" && /^[^/]+\/?$/.test(href));
  }

  async function categories(kind) {
    const root = `overlays/${kind}/`;
    const entries = await listing(root);
    return Promise.all(entries.filter((entry) => entry.endsWith("/")).map(async (entry) => ({
      name: decodeURIComponent(entry.slice(0, -1)),
      overlays: (await listing(root + entry)).filter((file) => /\.png$/i.test(file)).map(decodeURIComponent),
    })));
  }

  Promise.all([categories("frames"), categories("textures")]).then(([frames, textures]) => {
    if (typeof window.pm_command !== "function") window.pmSetOverlays(JSON.stringify({ frames, textures }));
  }).catch((error) => console.error("Preview overlays unavailable", error));
})();
