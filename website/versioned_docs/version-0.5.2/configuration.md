---
sidebar_position: 3
---

# Configuration

When you run Palworld, it'll generate a `config.json` in a config folder where Pal Schema resides. This config file can be used to customize how Pal Schema functions and more settings will be added as development progresses further.

I will present an example of a default config.json after it has been generated and explain what each value does.

```json
{
    "languageOverride": "",
    "enableAutoReload": false,
    "enableDebugLogging": false
}
```

`languageOverride` can be used to load custom translation mods for languages that the game doesn't normally support, this could in theory be anything, but it's preferred to follow the [ISO-639](https://en.wikipedia.org/wiki/List_of_ISO_639_language_codes) two-letter codes. If it's a made up language like pirate speak for example then you can just do `pirate`. Setting `languageOverride` to `""` will default to using whatever language Steam is set to.

`enableAutoReload` is used to enable or disable automatic reloading of schema mods in-game when you save the mod file you're editing. This is mainly meant for Pal Schema mod developers. Defaults to `false`.

`enableDebugLogging` is used to enable or disable more detailed logs for debugging purposes. Defaults to `false`.