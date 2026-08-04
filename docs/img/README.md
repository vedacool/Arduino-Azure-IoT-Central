# Screenshots for the Example Cookbook

Drop workshop screenshots here, then wire them into [`../../GUIDE.md`](../../GUIDE.md)
by replacing the matching *"Screenshot slot"* line with an image, e.g.:

```markdown
![Add capability](docs/img/add-capability.png)
```

(Paths in `GUIDE.md` are relative to the repo root, so use `docs/img/<file>.png`.)

## Shot list — what to capture (from **your own** IoT Central app)

| Save as | Screen | Should clearly show |
|---|---|---|
| `add-capability.png` | Device templates → your template → model → **+ Add capability** panel | The **Name** field (matching a key like `touch`), **Capability type** (Telemetry/Property), **Schema** (Double/Boolean), and the **Writable** checkbox for a property |
| `generate-views-publish.png` | Template → **Views → Generate default views** (and the **Publish** button) | The **Generate** button, and the **Publish** button in the top bar |
| `device-page.png` | Devices → your device | A telemetry **chart/tile** and a **writable switch** with its **Save** button |

Optional extras that help beginners: the **Devices → your device → Connect** panel
(ID scope / Device ID / key — blur the key), and **Permissions → API tokens** (for
the React-To-Another-Device examples).

> Tip: crop tightly to the relevant control, and blur any real keys/tokens before
> committing. PNG or JPG both fine.
